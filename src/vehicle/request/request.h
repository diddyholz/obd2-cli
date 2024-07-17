#pragma once

#include <cstdint>
#include <uuid_v4.h>
#include <json.hpp>

namespace obd2_server {
    class request {
        public:    
            UUIDv4::UUID id;

            std::string name;
            std::string description;
            std::string category;
        
            uint32_t ecu;
            uint8_t service;
            uint16_t pid;
            std::string formula;
            std::string unit;   

            request();

            bool operator==(const request &r) const;
    };
    
    void to_json(nlohmann::json& j, const request& p);
    void from_json(const nlohmann::json& j, request& p);
}
