#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>
#include <future>
#include <obd2.h>

struct request_wrapper {
    obd2::request req;
    std::string name;
};

void print_info(obd2::obd2 &instance);
void print_dtcs(obd2::obd2 &instance);
void clear_dtcs(obd2::obd2 &instance);
void print_pids(obd2::obd2 &instance);
void print_requests(obd2::obd2 &instance, std::vector<request_wrapper> &requests);
void print_request(request_wrapper &req);
request_wrapper parse_argument(obd2::obd2 &obd, const char *arg);
size_t split_str(std::string str, std::vector<std::string> &out, char c);
void clear_screen();
void error_invalid_arguments();
void error_exit(const char *error_title, const char *error_desc);

const char ARG_SEPERATOR = ':';
const int REFRESH_TIME = 1000;
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
    else if (command == "read") {
        std::vector<request_wrapper> requests;

        for (int i = 3; i < argc; i++) {
            requests.push_back(parse_argument(*obd_instance, argv[i]));
        }

        print_requests(*obd_instance, requests);
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

void print_requests(obd2::obd2 &instance, std::vector<request_wrapper> &requests) {
    for (;;) {
        // Print request responses
        clear_screen();

        for (request_wrapper &r : requests) {
            print_request(r);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }   
}

void print_request(request_wrapper &reqw) {
    if (reqw.name.empty()) {
        std::cout << std::hex << reqw.req.get_ecu_id() << ARG_SEPERATOR << uint8_t(reqw.req.get_service()) << ARG_SEPERATOR << reqw.req.get_pid() << ": " << std::dec;
    }
    else {
        std::cout << reqw.name << ":\t";
    }

    // Handle raw values
    if (reqw.req.get_formula().empty()) {
        const std::vector<uint8_t> &raw = reqw.req.get_raw();

        if (raw.size() == 0) {
            std::cout << "No response" << std::endl;
            return;
        }

        for (uint8_t b : raw) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << int(b) << " ";
        }

        std::cout << std::dec << std::setw(0) << std::endl;
        return;
    }

    float val = reqw.req.get_value();

    if (std::isnan(val)) {
        std::cout << "No response" << std::endl;
        return;
    }
    
    std::cout << val << std::endl;
}

request_wrapper parse_argument(obd2::obd2 &obd, const char *arg) {
    std::vector<std::string> options;
    std::string name = "";
    std::string formula = "";

    if (split_str(arg, options, ARG_SEPERATOR) < 3) {
        error_invalid_arguments();
    }

    if (options.size() > 3) {
        name = options[3];
    }
    
    if (options.size() > 4) {
        formula = options[4];
    }

    request_wrapper reqw;

    try {
        reqw.req = obd2::request(
            std::stoul(options[0], nullptr, 16), 
            std::stoi(options[1], nullptr, 16), 
            std::stoi(options[2], nullptr, 16),
            obd,
            formula,
            true
        );
    }
    catch (std::exception &e) {
        error_exit("Cannot start OBD2 request: ", e.what());
    }

    reqw.name = name;

    return reqw;
}

size_t split_str(std::string str, std::vector<std::string> &out, char c) {
    out.clear();

    while (str.length() > 0)
    {
        size_t c_pos = str.find(c);
        bool str_complete = false;

        // If character cannot be found, read until end of string
        if (c_pos == std::string::npos) {
            c_pos = str.length();
            str_complete = true;
        }

        out.emplace_back(str.substr(0, c_pos));

        if (str_complete) {
            break;
        }

        str = str.substr(c_pos + 1, str.length() - c_pos - 1);
    }
    
    out.shrink_to_fit();
    return out.size();
}

void clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void error_invalid_arguments() {
    std::string desc = "\nUsage: " + app_name + " network command\n\n" 
        + "commands: read, info, dtc_list, dtc_clear, pids";
    error_exit("Invalid Arguments", desc.c_str());
}

void error_exit(const char *error_title, const char *error_desc) {
    std::cerr << app_name << ARG_SEPERATOR << " " << error_title << ARG_SEPERATOR << " " << error_desc << std::endl;
    std::exit(EXIT_FAILURE);
}
