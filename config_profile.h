#pragma once
#include "util.h"
#include "logger.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <string>
class ConfigProfile {
public:
    ConfigProfile(const rapidjson::Value& profile, int i = 0);

    std::string getName() const { return name; }
    std::string getMaventaClientId() const { return maventa_client_id; }
    std::string getMaventaClientSecret() const { return maventa_client_secret; }
    std::string getMaventaVendorApiKey() const { return maventa_vendor_api_key; }
    std::string getOdooUrl() const { return odoo_url; }
    std::string getOdooDb() const { return odoo_db; }
    std::string getOdooUsername() const { return odoo_username; }
    std::string getOdooApiKey() const { return odoo_api_key; }
    int getOdooCompanyId() const { return odoo_company_id; }

private:
    std::string name;
    std::string maventa_client_id;
    std::string maventa_client_secret;
    std::string maventa_vendor_api_key;
    std::string odoo_url;
    std::string odoo_db;
    std::string odoo_username;
    std::string odoo_api_key;
    int odoo_company_id;
};