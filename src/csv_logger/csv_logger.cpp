#include "csv_logger.h"

#include <chrono>
#include <fstream>

namespace obd2_server {
    csv_logger::csv_logger() {}

    csv_logger::csv_logger(const std::vector<std::string> &header) : 
        csv_logger(header, "obd2_log_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + ".csv") {}

    csv_logger::csv_logger(const std::vector<std::string> &header, const std::string &filename) {
        file.open(filename);

        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file " + filename);
        }
        
        write_header(header);
    }

    void csv_logger::write_row(const std::vector<float> &data) {
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        file << get_time_string(timestamp);

        for (float d : data) {
            file  << "," << d;
        }

        file << std::endl;
    }

    void csv_logger::write_header(const std::vector<std::string> &header) {
        const size_t header_count = header.size();

        for (size_t i = 0; i < header_count; i++) {
            file << "\"" << header[i] << "\"";

            if (i < header_count - 1) {
                file << ",";
            }
        }

        file << std::endl;
    }
    
    std::string csv_logger::get_time_string(uint64_t timestamp) const {
        std::time_t time = timestamp / 1000;
        std::tm *tm = std::localtime(&time);

        char buffer[80];
        std::strftime(buffer, 80, "%H:%M:%S", tm);

        return std::string(buffer);
    }
}
