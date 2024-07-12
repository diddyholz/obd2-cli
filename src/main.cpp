#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>
#include <obd2.h>

struct request_wrapper {
    obd2::request &req;
    std::string name;
};

void print_request(request_wrapper &req);
void parse_argument(obd2::obd2 &obd, std::vector<request_wrapper> &requests, const char *arg);
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

    obd2::obd2 *obd_instance;
    std::vector<request_wrapper> requests;

    try {
        obd_instance = new obd2::obd2(argv[1]);
    }
    catch (std::exception &e) {
        error_exit("Cannot create OBD2 instance", e.what());
    }

    for (int i = 2; i < argc; i++) {
        parse_argument(*obd_instance, requests, argv[i]);
    }

    for (;;) {
        // Print request responses
        clear_screen();

        for (request_wrapper &r : requests) {
            print_request(r);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }    

    delete obd_instance;
    return 0;
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

void parse_argument(obd2::obd2 &obd, std::vector<request_wrapper> &requests, const char *arg) {
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

    try {
        obd2::request &new_req = obd.add_request(
            std::stoul(options[0], nullptr, 16), 
            std::stoi(options[1], nullptr, 16), 
            std::stoi(options[2], nullptr, 16),
            formula,
            true
        );

        request_wrapper reqw = { .req = new_req, .name = name };
        requests.push_back(reqw);
    }
    catch (std::exception &e) {
        error_exit("Cannot start OBD2 request: ", e.what());
    }
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
    std::string desc = "\nUsage: " + app_name + " network msg_id:sid:pid {msg_id:sid:pid}";
    error_exit("Invalid Arguments", desc.c_str());
}

void error_exit(const char *error_title, const char *error_desc) {
    std::cerr << app_name << ARG_SEPERATOR << " " << error_title << ARG_SEPERATOR << " " << error_desc << std::endl;
    std::exit(EXIT_FAILURE);
}
