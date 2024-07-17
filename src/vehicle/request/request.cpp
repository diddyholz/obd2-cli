#include "request.h"

namespace obd2_server {
    request::request() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) { }

    bool request::operator==(const request &r) const {
        return id == r.id;
    }
    
    void to_json(nlohmann::json& j, const request& r) {
        j = nlohmann::json{
            {"id", r.id.str()}, 
            {"name", r.name},
            {"description", r.description},
            {"category", r.category},
            {"ecu", r.ecu}, 
            {"service", r.service},
            {"pid", r.pid},
            {"formula", r.formula},
            {"unit", r.unit}
        };
    }

    void from_json(const nlohmann::json& j, request& r) {
        r.id = UUIDv4::UUID::fromStrFactory(j.at("id"));
        r.name = j.at("name");
        r.description = j.at("description");
        r.category = j.at("category");
        r.ecu = j.at("ecu");
        r.service = j.at("service");
        r.pid = j.at("pid");
        r.formula = j.at("formula");
        r.unit = j.at("unit");
    }
}