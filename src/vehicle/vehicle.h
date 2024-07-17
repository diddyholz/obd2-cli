#pragma once

#include <cstdint>
#include <list>
#include <uuid_v4.h>
#include "request/request.h"

namespace obd2_server {
    class vehicle {
        private:
            UUIDv4::UUID id;

            std::string make;
            std::string model;
            std::list<request> requests;

        public:
            vehicle();
            vehicle(const std::string &definition_file);
            vehicle(const std::string &make, const std::string &model);

            bool operator==(const vehicle &v) const;

            void add_request(const request &r);
            void remove_request(const request &r);

            const UUIDv4::UUID &get_id() const;
            std::string get_make() const;
            std::string get_model() const;
            const request &get_request(const UUIDv4::UUID &id) const;
            const std::list<request> &get_requests() const;

            friend void to_json(nlohmann::json& j, const vehicle& v);
            friend void from_json(const nlohmann::json& j, vehicle& v);
    };
            
    void to_json(nlohmann::json& j, const vehicle& v);
    void from_json(const nlohmann::json& j, vehicle& v);
};
