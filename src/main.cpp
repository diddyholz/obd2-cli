#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <vector>
#include <obd2.h>

struct request_wrapper {
    obd2::request &req;
    std::string name;
};

void print_request(request_wrapper &req);
void parse_argument(obd2::instance &obd, std::vector<request_wrapper> &requests, const char *arg);
int split_str(std::string str, std::vector<std::string> &out, char c);
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

    obd2::instance *obd_instance;
    std::vector<request_wrapper> requests;

    try {
        obd_instance = new obd2::instance(argv[1]);
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
    std::list<obd2::ecu> &ecus = reqw.req.get_ecus();

    if (reqw.name.empty()) {
        std::cout << std::hex << reqw.req.get_tx_id() << ARG_SEPERATOR << unsigned(reqw.req.get_sid()) << ARG_SEPERATOR << reqw.req.get_pid() << ": " << std::dec;
    }
    else {
        std::cout << reqw.name << ": ";
    }

    if (ecus.size() == 0) {
        std::cout << "\tNo response" << std::endl;
        return;
    }

    for (obd2::ecu &e : ecus) {
        const std::vector<uint8_t> &res = e.get_current_response_buf();

        std::cout << "\tECU " << std::hex << e.get_rx_id() << ": ";

        for (uint8_t byte : res) {
            std::cout << unsigned(byte) << " ";
        }

        std::cout << std::dec << std::endl << "\t";
    }

    std::cout << std::endl;
}

void parse_argument(obd2::instance &obd, std::vector<request_wrapper> &requests, const char *arg) {
    std::vector<std::string> options;
    std::string name = "";

    if (split_str(arg, options, ARG_SEPERATOR) < 3) {
        error_invalid_arguments();
    }

    try {
        obd2::request &new_req = obd.add_request(
            std::stoul(options[0], nullptr, 16), 
            std::stoi(options[1], nullptr, 16), 
            std::stoi(options[2], nullptr, 16), 
            true
        );

        if (options.size() > 3) {
            name = options[3];
        }

        request_wrapper reqw = { .req = new_req, .name = name };
        requests.push_back(reqw);
    }
    catch (std::exception &e) {
        error_exit("Cannot start OBD2 request: ", e.what());
    }
}

int split_str(std::string str, std::vector<std::string> &out, char c) {
    out.clear();

    while (str.length() > 0)
    {
        int c_pos = str.find(c);
        bool str_complete = false;

        // If character cannot be found, read until end of string
        if (c_pos < 0) {
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
