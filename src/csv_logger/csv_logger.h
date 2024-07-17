#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace obd2_server {
    class csv_logger {
        private:
            std::ofstream file;

            void write_header(const std::vector<std::string> &header);
            
        public:
            csv_logger();
            csv_logger(const std::vector<std::string> &header);
            csv_logger(const std::vector<std::string> &header, const std::string &filename);

            void write_row(const std::vector<float> &data);
    };
}
