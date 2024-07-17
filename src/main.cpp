#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <csignal>
#include <exception>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <limits>
#include <map>
#include <vector>
#include <future>
#include <obd2.h>
#include "vehicle/vehicle.h"
#include "csv_logger/csv_logger.h"

void print_info(obd2::obd2 &instance);
void print_dtcs(obd2::obd2 &instance);
void clear_dtcs(obd2::obd2 &instance);
void print_pids(obd2::obd2 &instance);
void log_requests(obd2::obd2 &instance, int argc, const char *argv[]);
std::map<const obd2_server::request *, obd2::request> create_requests(obd2::obd2 &instance, obd2_server::vehicle &vehicle);
void print_requests(std::map<const obd2_server::request *, obd2::request> &requests, uint32_t refresh_ms);
float print_request(std::pair<const obd2_server::request *const, obd2::request> &req);
void clear_screen();
void error_invalid_arguments();
void error_exit(const char *error_title, const char *error_desc);

const char ARG_SEPERATOR = ':';
std::string app_name;

int main(int argc, const char *argv[]) {
    app_name = argv[0];
    
    if (argc < 3) {
        error_invalid_arguments();
    }

    std::string command = argv[2];
    std::unique_ptr<obd2::obd2> obd_instance;

    try {
        obd_instance = std::make_unique<obd2::obd2>(argv[1]);
    }
    catch (std::exception &e) {
        error_exit("Cannot create OBD2 instance", e.what());
    }

    if (command == "info") {
        print_info(*obd_instance);
    }
    else if (command == "dtc_list") {
        print_dtcs(*obd_instance);
    }
    else if (command == "dtc_clear") {
        clear_dtcs(*obd_instance);
    }
    else if (command == "pids") {
        print_pids(*obd_instance);
    }
    else if (command == "log") {
        log_requests(*obd_instance, argc, argv);
    } 
    else {
        error_invalid_arguments();
    }

    return 0;
}

void print_info(obd2::obd2 &instance) {
    std::cout << "Reading vehicle information..." << std::endl;

    obd2::vehicle_info info = instance.get_vehicle_info();

    std::cout << "VIN:\t\t" << info.vin << std::endl;
    std::cout << "Ignition Type:\t" << info.ign_type << std::endl;
    std::cout << "ECUs:\t\t" << std::hex << std::setfill('0') << std::setw(3);

    if (info.ecus.size() == 0) {
        std::cout << "None" << std::endl;
    }

    std::cout << std::endl;

    for (obd2::vehicle_info::ecu &ecu : info.ecus) {
        std::cout << "\t" << ecu.id << ": " << ecu.name << std::endl;
    }
}

void print_dtcs(obd2::obd2 &instance) {
    std::cout << "Reading DTCs..." << std::endl;

    obd2::vehicle_info info = instance.get_vehicle_info();

    std::vector<std::future<std::vector<obd2::dtc>>> dtc_futures;
    dtc_futures.reserve(info.ecus.size());

    // Asynchronously get DTCs for each ECU
    for (obd2::vehicle_info::ecu &ecu : info.ecus) {
        dtc_futures.emplace_back(
            std::async(
                std::launch::async, 
                &obd2::obd2::get_dtcs, 
                &instance, 
                ecu.id
            )
        );
    }

    for (size_t i = 0; i < dtc_futures.size(); i++) {
        obd2::vehicle_info::ecu &ecu = info.ecus[i];
        std::vector<obd2::dtc> dtcs = dtc_futures[i].get();

        std::cout << "ECU " << ecu.name << " (" << std::hex << std::setfill('0') << std::setw(3) 
            << ecu.id << std::dec << std::setw(0) << "): " << std::endl;
        
        if (dtcs.size() == 0) {
            std::cout << "\tNo DTCs" << std::endl;
        }

        for (obd2::dtc &dtc : dtcs) {
            std::cout << "\t\t\t" << dtc << std::endl;
        }
    }
}

void clear_dtcs(obd2::obd2 &instance) {
    std::cout << "Clearing DTCs..." << std::endl;

    obd2::vehicle_info info = instance.get_vehicle_info();

    for (obd2::vehicle_info::ecu &ecu : info.ecus) {
        obd2::request req = obd2::request(ecu.id, 0x04, 0x00, instance);
    }
}

void print_pids(obd2::obd2 &instance) {
    std::cout << "Reading supported Service 01 PIDs..." << std::endl;

    obd2::vehicle_info info = instance.get_vehicle_info();

    for (obd2::vehicle_info::ecu &ecu : info.ecus) {
        std::vector<uint8_t> pids = instance.get_supported_pids(ecu.id);

        std::cout << "ECU " << std::hex << std::setfill('0') << std::setw(3) 
            << ecu.id << std::setw(2) << ": " << std::endl;

        for (uint8_t pid : pids) {
            std::cout << "\t" << int(pid) << std::endl;
        }

        std::cout << std::dec << std::setw(0);
    }
}

void log_requests(obd2::obd2 &instance, int argc, const char *argv[]) {
    if (argc < 4) {
        error_invalid_arguments();
    }

    obd2_server::vehicle vehicle;
    uint32_t refresh_ms = 1000;

    try {
        vehicle = obd2_server::vehicle(argv[3]);
    }
    catch (std::exception &e) {
        error_exit("Cannot read vehicle definition", e.what());
    }

    if (argc > 4) {
        refresh_ms = std::atoi(argv[4]);
    }

    instance.set_refresh_ms(refresh_ms);

    std::map<const obd2_server::request *, obd2::request> requests = create_requests(instance, vehicle);
    print_requests(requests, refresh_ms);
}

std::map<const obd2_server::request *, obd2::request> create_requests(obd2::obd2 &instance, obd2_server::vehicle &vehicle) {
    std::map<const obd2_server::request *, obd2::request> requests;

    std::cout << "Fetching supported PIDs..." << std::endl;

    std::vector<uint8_t> pids = instance.get_supported_pids(0x7E0);

    for (const obd2_server::request &req : vehicle.get_requests()) {
        if (req.ecu == 0x7E0 && req.service == 0x01 && std::find(pids.begin(), pids.end(), req.pid) == pids.end()) {
            continue;
        }

        requests.try_emplace(&req, req.ecu, req.service, req.pid, instance, req.formula, true);
    }

    return requests;
}

void print_requests(std::map<const obd2_server::request *, obd2::request> &requests, uint32_t refresh_ms) {
    std::vector<std::string> data_log_headers;
    data_log_headers.reserve(requests.size() + 1);
    data_log_headers.push_back("timestamp");

    for (const auto &p : requests) {
        data_log_headers.push_back(p.first->name);
    }

    obd2_server::csv_logger logger(data_log_headers);

    // Print request responses in infinite loop
    for (;;) {
        clear_screen();

        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::vector<float> data;
        data.reserve(requests.size());

        for (auto &p : requests) {
            float val = print_request(p);
            data.push_back(val);
        }

        logger.write_row(data);

        std::this_thread::sleep_for(std::chrono::milliseconds(refresh_ms));
    }   
}

float print_request(std::pair<const obd2_server::request *const, obd2::request> &req) {
    static uint8_t name_width = 0;
    std::string name = req.first->name;

    if (name.empty()) {
        std::stringstream ss;
        ss << std::hex << req.first->ecu << ARG_SEPERATOR << uint8_t(req.first->service) << ARG_SEPERATOR << req.first->pid;
        name = ss.str();
    }

    name += ": ";

    if (name.size() > name_width) {
        name_width = name.size();
    }

    std::cout << std::setw(name_width) << std::setfill(' ') << std::left << name << std::setw(0);

    // Handle raw values
    if (req.second.get_formula().empty()) {
        const std::vector<uint8_t> &raw = req.second.get_raw();

        if (raw.size() == 0) {
            std::cout << "No response" << std::endl;
            return std::numeric_limits<float>::quiet_NaN();
        }

        for (uint8_t b : raw) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << int(b) << " ";
        }

        std::cout << std::dec << std::setw(0) << std::endl;
        return std::numeric_limits<float>::quiet_NaN();
    }

    float val = req.second.get_value();

    if (std::isnan(val)) {
        std::cout << "No response" << std::endl;
        return std::numeric_limits<float>::quiet_NaN();
    }
    
    std::cout << val << req.first->unit << std::endl;
    return val;
}

void clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void error_invalid_arguments() {
    std::string desc = "\nUsage: " + app_name + " network command\n\n" 
        + "commands: log, info, dtc_list, dtc_clear, pids";
    error_exit("Invalid Arguments", desc.c_str());
}

void error_exit(const char *error_title, const char *error_desc) {
    std::cerr << app_name << ARG_SEPERATOR << " " << error_title << ARG_SEPERATOR << " " << error_desc << std::endl;
    std::exit(EXIT_FAILURE);
}
