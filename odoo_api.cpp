/*
 * Copyright (c) 2025 @https://github.com/kdilayer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "odoo_api.h"
#include <iostream>
#include <map>
#include <sstream>
#include "util.h"

OdooAPI::OdooAPI(const std::string& url,
                 const std::string& db,
                 const std::string& username,
                 const std::string& apikey, const int companyId)
    : url_(url), db_(db), username_(username), apikey_(apikey), loggedOnCompanyId(companyId) {}

bool OdooAPI::authenticate() {
    try {
        xmlrpc_c::clientSimple client;
        xmlrpc_c::value result;

        std::map<std::string, xmlrpc_c::value> empty_map;
        xmlrpc_c::value_struct empty_struct(empty_map);

        xmlrpc_c::paramList params;
        params.add(xmlrpc_c::value_string(db_));
        params.add(xmlrpc_c::value_string(username_));
        params.add(xmlrpc_c::value_string(apikey_));
        params.add(empty_struct);

        client.call(
            url_ + "/xmlrpc/2/common",
            "authenticate",
            params,
            &result
        );
        if (result.type() == xmlrpc_c::value::TYPE_INT) {
            loggedOnUserId = xmlrpc_c::value_int(result);
            //LOG(INFO) << "Athentication ok, got loggedOnUserId: " << loggedOnUserId;
            return true;
        } else {
            LOG(INFO) << "Authentication failed: value not int";
            return false;
        }
    } catch (std::exception const &e) {
        LOG(ERROR) << "XML-RPC Error: " << e.what();
        return false;
    }
}
std::string resultArrayToString(const xmlrpc_c::value& result) {
    xmlrpc_c::value_array result_array(result);
    std::vector<xmlrpc_c::value> vec = result_array.vectorValueValue();
    std::stringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        xmlrpc_c::value_struct rec(vec[i]);
        std::map<std::string, xmlrpc_c::value> rec_map = rec.cvalue();
        oss << "{";
        size_t count = 0;
        for (const auto& kv : rec_map) {
            oss << "\"" << kv.first << "\": ";
            switch(kv.second.type()) {
                case xmlrpc_c::value::TYPE_STRING:
                    oss << "\"" << escape_json(std::string(xmlrpc_c::value_string(kv.second))) << "\"";
                    break;
                case xmlrpc_c::value::TYPE_INT:
                    oss << xmlrpc_c::value_int(kv.second);
                    break;
                case xmlrpc_c::value::TYPE_DOUBLE:
                    oss << xmlrpc_c::value_double(kv.second);
                    break;
                case xmlrpc_c::value::TYPE_BOOLEAN:
                    oss << (xmlrpc_c::value_boolean(kv.second) ? "true" : "false");
                    break;
                    /* TODO FIXME
                case xmlrpc_c::value::TYPE_STRUCT: {
                    std::string ex = "{" + escape_json(std::string(xmlrpc_c::value_string(kv.second))) + "}";
                    oss << ex;
                    break;
                }
                    */
                case xmlrpc_c::value::TYPE_ARRAY: {
                    xmlrpc_c::value_array arr(kv.second);
                    std::vector<xmlrpc_c::value> arr_vec = arr.vectorValueValue();
                    oss << "[";
                    int acount = 0;
                    for (size_t j = 0; j < arr_vec.size(); ++j) {
                        if (arr_vec[j].type() == xmlrpc_c::value::TYPE_STRING)
                            oss << "\"" << escape_json(std::string(xmlrpc_c::value_string(arr_vec[j]))) << "\"";
                        else if (arr_vec[j].type() == xmlrpc_c::value::TYPE_INT)
                            oss << xmlrpc_c::value_int(arr_vec[j]);
                        else if (arr_vec[j].type() == xmlrpc_c::value::TYPE_DOUBLE)
                            oss << xmlrpc_c::value_double(arr_vec[j]);  
                        else {
                            std::cout << "Unsupported type in array " << kv.first << " type: " << std::to_string(arr_vec[j].type()) << std::endl;
                            oss << "\"<complex type>\""; // Handle other types
                        }
                        if (++acount < arr_vec.size()) oss << ", ";
                    }
                    oss << "]";
                    break;
                }
                default:
                    //std::cout << "Unsupported type for key " << kv.first << ", type: " << std::to_string(kv.second.type()) << std::endl;
                    oss << "\"<complex type>\""; // Handle other types
            }
            if (++count < rec_map.size()) oss << ", ";
        }
        oss << "}";
        if (i + 1 < vec.size()) oss << ", ";
    }
    oss << "]";
    std::string ret_str=oss.str();
    return ret_str;
}
bool OdooAPI::convertResultToJson(const xmlrpc_c::value& result, rapidjson::Document &doc) {
    std::string json_result = "";
    switch(result.type()) {
        case xmlrpc_c::value::TYPE_INT:
            json_result = "{\"result\": " + std::to_string(xmlrpc_c::value_int(result)) + "}";
            break;
        case xmlrpc_c::value::TYPE_DOUBLE:
            json_result = "{\"result\": " + std::to_string(xmlrpc_c::value_double(result)) + "}";
            break;
        case xmlrpc_c::value::TYPE_BOOLEAN:
            json_result = "{\"result\": " + std::to_string(xmlrpc_c::value_boolean(result)) + "}";
            break;
        case xmlrpc_c::value::TYPE_STRING: 
            json_result = "{\"result\": \"" + escape_json(xmlrpc_c::value_string(result)) + "\"}";
            break;
        case xmlrpc_c::value::TYPE_ARRAY:
            json_result = resultArrayToString(result);
            break;
        case xmlrpc_c::value::TYPE_STRUCT:
        default:
            LOG(ERROR)<< "Unsupported result type: " << result.type() << std::endl;
            break;
    }
    if(json_result.empty()) {
        LOG(ERROR)<< "Failed to convert result to JSON: empty result" << std::endl;
        return false;
    }
    #if 1
        WriteFileContent("/tmp/odoo_response_1.json", json_result, true);
    #endif
    
    if (doc.Parse(json_result.c_str()).HasParseError()) {
        std::cerr << "Failed to parse JSON response: " << rapidjson::GetParseError_En(doc.GetParseError())
                    << " (offset " << doc.GetErrorOffset() << ")" << std::endl;
        return false;
    }
    return true;
}
void add_val(std::map<std::string, xmlrpc_c::value> &vals, std::string const& field, std::string const& op, std::string const& value) {
    vals[field] = xmlrpc_c::value_string(value);
}
void add_val(std::map<std::string, xmlrpc_c::value> &vals, std::string const& field, std::string const& op, int value) {
    vals[field] = xmlrpc_c::value_int(value);
}
void add_val(std::map<std::string, xmlrpc_c::value> &vals, std::string const& field, std::string const& op, double value) {
    vals[field] = xmlrpc_c::value_double(value);
}
void add_filter(std::vector<xmlrpc_c::value>* domain, const std::string& field, const std::string& op, const std::string& value) {
    std::vector<xmlrpc_c::value> filter;
    filter.push_back(xmlrpc_c::value_string(field));
    filter.push_back(xmlrpc_c::value_string(op));
    filter.push_back(xmlrpc_c::value_string(value));
    domain->push_back(xmlrpc_c::value_array(filter));
}
void add_filter(std::vector<xmlrpc_c::value>* domain, const std::string& field, const std::string& op, int value) {
    std::vector<xmlrpc_c::value> filter;
    filter.push_back(xmlrpc_c::value_string(field));
    filter.push_back(xmlrpc_c::value_string(op));
    filter.push_back(xmlrpc_c::value_int(value));
    domain->push_back(xmlrpc_c::value_array(filter));
}
bool OdooAPI::odooCommand(const std::string& method, const std::string& model, const xmlrpc_c::value_array& domain, xmlrpc_c::value* result, std::map<std::string, xmlrpc_c::value> *options) {
    has_error = false; // Reset error state before command execution
    try {
        xmlrpc_c::clientSimple client;
        xmlrpc_c::paramList params;
        params.add(xmlrpc_c::value_string(db_));
        params.add(xmlrpc_c::value_int(loggedOnUserId));
        params.add(xmlrpc_c::value_string(apikey_));
        params.add(xmlrpc_c::value_string(model));
        params.add(xmlrpc_c::value_string(method));


        params.add(xmlrpc_c::value_array(domain));

        // Empty options dict
        std::map<std::string, xmlrpc_c::value> opts;
        if(options) {
             params.add(xmlrpc_c::value_struct(*options));
        }
        else {
            params.add(xmlrpc_c::value_struct(opts));
        }
        client.call(url_ + "/xmlrpc/2/object", "execute_kw", params, result);

        return true;
    } catch (const std::exception& e) {
        has_error = true;
        std::cerr << "XML-RPC error: " << e.what() << std::endl;
        return false;
    }
}

std::string OdooAPI::getCompanyInfoByCompanyId(int companyId, std::string &taxcode, std::string &street, std::string &town, std::string &postCode, std::string &ovt, std::string &intermediator, bool is_seller) {
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "id", "=", companyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    std::string vat;
    if(odooCommand("search_read", is_seller ? "res.company" : "res.partner", domain, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.IsArray() && doc.Size() > 0) {
                const rapidjson::Value& first_entry = doc[0];
                if (first_entry.IsObject() && first_entry.HasMember("vat") && first_entry["vat"].IsString()) {
                    taxcode = first_entry["vat"].GetString();
                }
                if (first_entry.IsObject() && first_entry.HasMember("street") && first_entry["street"].IsString()) {
                    street = first_entry["street"].GetString();
                }
                if (first_entry.IsObject() && first_entry.HasMember("city") && first_entry["city"].IsString()) {
                    town = first_entry["city"].GetString();
                }
                if (first_entry.IsObject() && first_entry.HasMember("zip") && first_entry["zip"].IsString()) {
                    postCode = first_entry["zip"].GetString();
                } 
                if (first_entry.IsObject() && first_entry.HasMember("x_studio_eio_ovt") && first_entry["x_studio_eio_ovt"].IsString()) {
                    ovt = first_entry["x_studio_eio_ovt"].GetString();
                }
                if (first_entry.IsObject() && first_entry.HasMember("x_studio_eio_intermediator") && first_entry["x_studio_eio_intermediator"].IsString()) {
                    intermediator = first_entry["x_studio_eio_intermediator"].GetString();
                }
            }
        }
    }
    return vat;
}
double OdooAPI::getCompanyTaxRatePercentById(int taxId){
    // Get the percentage for the given tax id
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "id", "=", taxId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    if(odooCommand("search_read", "account.tax", domain, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.IsArray() && doc.Size() > 0) {
                const rapidjson::Value& first_entry = doc[0];
                if (first_entry.IsObject() && first_entry.HasMember("amount")&& first_entry["amount"].IsNumber()) {
                    return first_entry["amount"].GetDouble();
                }
            }
        }
    }
    return 0;
}

int OdooAPI::getCompanyTaxId(std::string taxString, int companyId) {

    if(taxString == "25.50") taxString="25.5";
    // Get the tax id for the given tax string
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "name", "=", taxString+"%");
    add_filter(&filters, "company_id", "=", companyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    if(odooCommand("search_read", "account.tax", domain, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.IsArray() && doc.Size() > 0) {
                const rapidjson::Value& first_entry = doc[0];
                if (first_entry.IsObject() && first_entry.HasMember("id") && first_entry["id"].IsNumber()) {
                    return first_entry["id"].GetInt();
                }
            }
        }
    }
    return -1; // Tax id not found
}
bool OdooAPI::vendorBillExists(std::string const& eioInvoiceIdentifier) {
    // Build the domain filter: [[["move_type", "=", "in_invoice"], ["x_studio_eio_invoice_identifier", "=", eioInvoiceIdentifier]]]
    std::vector<xmlrpc_c::value> domain;
    add_filter(&domain, "move_type", "=", "in_invoice");
    add_filter(&domain, "x_studio_eio_invoice_identifier", "=", eioInvoiceIdentifier);
    add_filter(&domain, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain_outer;
    domain_outer.push_back(xmlrpc_c::value_array(domain));
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "account.move", domain_outer, &result);
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
         // Parse ret_str as JSON array to check if any results exist
        bool bill_found = false;
        if (doc.IsArray() ) {
            if(!doc.Empty() && doc.Size() > 0) {
                // Enumerate over array entries
                for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
                    const rapidjson::Value& entry = doc[i];
                    if (entry.IsObject()) {
                        if(entry.HasMember("x_studio_eio_invoice_identifier") && entry["x_studio_eio_invoice_identifier"].IsString()) {
                            std::string eioId = entry["x_studio_eio_invoice_identifier"].GetString();
                            if(eioId != "" && eioId == eioInvoiceIdentifier) {
                                //LOG(DEBUG) << "Found matching EIO Invoice Identifier: " << eioId;
                                bill_found=true; 
                                break;
                            }
                        }
                    }
                }
                return bill_found; // Bill exists
            }
            else {
                //LOG(DEBUG) << "vendorBillExists not found eio_invoice_identifier=" << eioInvoiceIdentifier;
                return false; // No entries found
            }
        } 
        return false; // Bill does not exist
    }
    return false; // Command failed
}

std::string oldTaxCodeFormat(const std::string& taxCode) {
    /*
        FI12345671-> 12345671-1 (remove country code and add dash after 6 digits)
    */
    if(taxCode.length() != 10 ) {
        std::cerr << "Invalid tax code length: " << taxCode << std::endl;
        return ""; // Return as is if too short
    }
    std::string formatted = taxCode;
    if(startsWithCountryCode(formatted)) {
        // If it starts with country code, remove it
        formatted = formatted.substr(2); // Remove first two characters (country code)  
    }

    if(formatted.find("-") == std::string::npos) {
        //does not contain dash, add one
        if(formatted.length() > 6) {
            // Add a dash after the first 6 characters
            formatted.insert(7, "-");
        } 
        
    }
    return formatted;
}
int OdooAPI::vendorExists(std::string const& taxCode) {
    // Check if vendor exists by TaxCode
    int vendor_id = vendorExists_ex(taxCode);
    if(vendor_id <= 0) {
        vendor_id = vendorExists_ex(oldTaxCodeFormat(taxCode));
        if(vendor_id >= 0) {
            //fixVendorTaxCode(vendor_id, EpiBei);
        }
    }
    return vendor_id;
}
int OdooAPI::vendorExists_ex(std::string const& taxCode) {    
    // Build the domain filter: [[["move_type", "=", "in_invoice"], ["x_studio_eio_invoice_identifier", "=", eioInvoiceIdentifier]]]
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "vat", "=", taxCode);
    add_filter(&filters, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "res.partner", domain, &result);
    rapidjson::Document doc;
    int vendor_id = 0;
    if(success && convertResultToJson(result, doc)) {
         // Parse ret_str as JSON array to check if any results exist
        
        if (doc.IsArray() ) {
            if(!doc.Empty() && doc.Size() > 0) {
                // Enumerate over array entries
                for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
                    const rapidjson::Value& entry = doc[i];
                    if (entry.IsObject()) {
                        if(entry.HasMember("vat") && entry["vat"].IsString()) {
                            std::string vatId = entry["vat"].GetString();
                            if(vatId != "" && vatId == taxCode) {
                                //LOG(DEBUG) << "Found matching Vendor with Vat Identifier: " << vatId << std::endl;
                                vendor_id = entry["id"].GetInt();
                                break;
                            }
                        }
                    }
                }
                return vendor_id; // Vendor exists
            }
            else {
                //LOG(DEBUG) << "Vendor not found with tax code: " << taxCode;
                return -1; // No entries found
            }
        } 
        return -1; // vendor does not exist
    }
    return -1; // Command failed
}
int OdooAPI::vendorBankAccountExists(int vendorId, std::string SellerAccountID) {
    // Check if vendor bank account exists
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "acc_number", "=", SellerAccountID);
    add_filter(&filters, "partner_id", "=", vendorId);
    //add_filter(&filters, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "res.partner.bank", domain, &result);
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if (doc.IsArray() && doc.Size() > 0) {
            const rapidjson::Value& first_entry = doc[0];
            if (first_entry.IsObject() && first_entry.HasMember("id")) {
                return first_entry["id"].GetInt();
            }
        }
    }
    return -1; // Bank account not found
}
bool OdooAPI::getBankAccountBic(int partner_bank_id, std::string &bic, std::string &bankName, std::string &accNumber) {
    // Check if vendor bank account exists
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "id", "=", partner_bank_id);
    //add_filter(&filters, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "res.partner.bank", domain, &result);
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if (doc.IsArray() && doc.Size() > 0) {
            const rapidjson::Value& first_entry = doc[0];
            if (first_entry.IsObject() && first_entry.HasMember("bank_bic") && first_entry["bank_bic"].IsString()) {
                bic = first_entry["bank_bic"].GetString();
            }
            if( first_entry.IsObject() && first_entry.HasMember("bank_name") && first_entry["bank_name"].IsString()) {
                bankName = first_entry["bank_name"].GetString();
            }
            if( first_entry.IsObject() && first_entry.HasMember("acc_number") && first_entry["acc_number"].IsString()) {
                accNumber = first_entry["acc_number"].GetString();
            }
            return true;
        }
    }
    return false; // Bank account not found
}
int OdooAPI::vendorBankExists(int vendorId, std::string SellerAccountName) {
    // Check if vendor bank exists
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "name", "=", SellerAccountName);
    //add_filter(&filters, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "res.bank", domain, &result);
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if (doc.IsArray() && doc.Size() > 0) {
            const rapidjson::Value& first_entry = doc[0];
            if (first_entry.IsObject() && first_entry.HasMember("id")) {
                return first_entry["id"].GetInt();
            }
        }
    }
    return -1; // Bank not found
}
int OdooAPI::createVendorBank(int vendorId, std::string SellerAccountName, std::string SellerBic) {
    // Create vendor bank if not exists
    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, "name", "=", SellerAccountName);
    add_val(vals, "bic", "=", SellerBic);
    //add_val(vals, "company_id", "=", loggedOnCompanyId);

    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_struct(vals));
    
    xmlrpc_c::value result;
    bool success = odooCommand("create", "res.bank", records, &result);
    int bank_id = -1;
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if(doc.HasMember("result") && doc["result"].IsInt()) {
            bank_id = doc["result"].GetInt();
            LOG(INFO) << "New vendor bank created with id = " << bank_id;
        } else {
            LOG(ERROR) << "Failed to create vendor bank: Invalid response format";
            return -1;
        }
    }
    return bank_id;
}
int OdooAPI::createVendorBankAccount(int vendorId, std::string SellerAccountID, std::string SellerBic, std::string SellerAccountName) {

    int bankId = vendorBankExists(vendorId, SellerAccountName);
    if(bankId<=0){
        bankId = createVendorBank(vendorId, SellerAccountName, SellerBic);
    }
    if(bankId <= 0) {
        LOG(ERROR) << "Failed to create vendor bank account, bankId is invalid: " << bankId << std::endl;
        return -1; // Failed to create vendor bank account
    }
    int bank_account_id = -1;
    
    // Create vendor bank account if not exists
    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, "acc_number", "=", SellerAccountID);
    //add_val(vals, "bic", "=", SellerBic);
    add_val(vals, "partner_id", "=", vendorId); 
    //add_val(vals, "bank_name", "=", SellerAccountName);
    add_val(vals, "bank_id", "=", bankId);
    //add_val(vals, "company_id", "=", loggedOnCompanyId);

    //set allow_out_payment by default ???

    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_struct(vals));
    
    xmlrpc_c::value result;
    bool success = odooCommand("create", "res.partner.bank", records, &result);
    
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if(doc.HasMember("result") && doc["result"].IsInt()) {
            bank_account_id = doc["result"].GetInt();
            LOG(INFO) << "New vendor bank account created with id = " << bank_account_id;
        } else {
            LOG(ERROR) << "Failed to create vendor bank account: Invalid response format";
            return -1;
        }
    }
    return bank_account_id;
}
int OdooAPI::findCountryId(std::string countryName) {
    // Find the country id for the given country name
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "name", "=", countryName);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    if(odooCommand("search_read", "res.country", domain, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.IsArray() && doc.Size() > 0) {
                const rapidjson::Value& first_entry = doc[0];
                if (first_entry.IsObject() && first_entry.HasMember("id")) {
                    return first_entry["id"].GetInt();
                }
            }
        }
    }
    return -1; // Country id not found
}
int OdooAPI::createVendor(std::string const& taxCode, const FinvoiceInvoice& inv) {

    // Build the domain filter: [[["move_type", "=", "in_invoice"], ["x_studio_eio_invoice_identifier", "=", eioInvoiceIdentifier]]]
    //LOG(DEBUG) << "Creating new vendor " << inv.seller.SellerOrganisationName <<" with tax code: " << taxCode;
    std::string sellerCountryCode = inv.seller.SellerCountryCode;
    if(sellerCountryCode == ""){
        sellerCountryCode = getFirstTwoChars(taxCode);
    }
    std::string sellerCountryName =inv.seller.SellerCountryName;
    if(sellerCountryName == "") 
        sellerCountryName = getCountryNameForCode(sellerCountryCode);

    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, "vat", "=", taxCode);
    add_val(vals, "company_type", "=", "company");
    add_val(vals, "name", "=", inv.seller.SellerOrganisationName);
    add_val(vals, "street", "=", inv.seller.SellerStreetName);
    add_val(vals, "city", "=", inv.seller.SellerTownName);
    add_val(vals, "zip", "=", inv.seller.SellerPostCodeIdentifier);
    add_val(vals, "country_code", "=", sellerCountryCode);
    int country_id = findCountryId(sellerCountryName);
    if(country_id < 0) {
        country_id = findCountryId("Finland");
    }
    add_val(vals, "country_id", "=", country_id);
    add_val(vals, "supplier_rank", "=", 1);
    
    add_val(vals, "mobile", "=", inv.seller.SellerPhoneNumberIdentifier);
    add_val(vals, "email", "=", inv.seller.SellerEmailaddressIdentifier);
    add_val(vals, "website", "=", inv.seller.SellerWebaddressIdentifier);
    //add_val(vals, "mobile", "=", 1);

    add_val(vals, "x_studio_eio_ovt", "=", inv.seller.SellerOVT);
    add_val(vals, "x_studio_eio_intermediator", "=", inv.seller.SellerIntermediator);
    add_val(vals, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_struct(vals));
    
    xmlrpc_c::value result;
    bool success = odooCommand("create", "res.partner", records, &result);
    int vendor_id = -1;
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {

        if(doc.HasMember("result") && doc["result"].IsInt()) {
            vendor_id = doc["result"].GetInt();
            LOG(INFO) << "New vendor created " << inv.seller.SellerOrganisationName << "with id = " << vendor_id;
        } else {
            LOG(ERROR) << "Failed to create vendor: Invalid response format";
            return -1;
        }
    }

    return vendor_id;
}                  
std::string decimalFormat(const std::string& value) {
    // Convert string "25,5" to "25.5"
    //102.34434,55
    if(value.empty()) return "0"; // Return 0 if input is empty

    std::string formatted_value = value;
    string_replaceall(formatted_value, ",", ".");
    return formatted_value;
}
int OdooAPI::getFiscalPositionId(){
    // Get the fiscal position id for the company
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "company_id", "=", std::to_string(loggedOnCompanyId));
    //add_filter(&filters, "country_id", "=", findCountryId("Finland"));
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));
    
    xmlrpc_c::value result;
    if(odooCommand("search_read", "account.fiscal.position", domain, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.IsArray() && doc.Size() > 0) {
                const rapidjson::Value& first_entry = doc[0];
                if (first_entry.IsObject() && first_entry.HasMember("id")) {
                    return first_entry["id"].GetInt();
                }
            }
        }
    }
    return -1; // Fiscal position id not found
}
std::string insertDashesToDate(const std::string& date) {
    // Convert date from YYYYMMDD to YYYY-MM-DD
    if(date.length() != 8) {
        LOG(ERROR) << "Invalid date format: " << date << std::endl;
        return date; // Return as is if invalid length
    }
    return date.substr(0, 4) + "-" + date.substr(4, 2) + "-" + date.substr(6, 2);
}
int OdooAPI::createVendorBillAttachment(const FinvoiceAttachment &attachment, int res_id) {
    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, "name", "=", attachment.AttachmentName);
    add_val(vals, "type", "=", "binary"); //attachment.AttachmentMimeType);
    add_val(vals, "datas", "=", attachment.AttachmentContent);
    add_val(vals, "res_model", "=", "account.move");
    add_val(vals, "res_id", "=", res_id); // Link to the invoice, if res_id is 0, it can be linked later
    add_val(vals, "mimetype", "=", attachment.AttachmentMimeType);
    add_val(vals, "company_id", "=", loggedOnCompanyId);

    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_struct(vals));

    xmlrpc_c::value result;
    if(odooCommand("create", "ir.attachment", records, &result)) {
        rapidjson::Document doc;
        if(convertResultToJson(result, doc)) {
            if (doc.HasMember("result") && doc["result"].IsInt()) {
                int attachment_id = doc["result"].GetInt();
                LOG(INFO) << "New attachment created " << attachment.AttachmentName << " with id = " << attachment_id;
                return attachment_id;
            }
        }
    }
    return -1;
}
int OdooAPI::createVendorBill(const FinvoiceInvoice& inv) {

    int vendorId = vendorExists(inv.EpiBei);
    if(vendorId <= 0) {
        vendorId = createVendor(inv.EpiBei, inv);
    }
    if(vendorId <= 0) {
        LOG(DEBUG) << "Failed to find vendor for invoice: " << inv.InvoiceNumber << std::endl;
        return -1; // Failed to create vendor
    }

    //create bank account if not exists
    int vendorBankAccountId = -1;
    vendorBankAccountId = vendorBankAccountExists(vendorId, inv.seller.SellerAccountID);
    if(vendorBankAccountId <= 0) {
        vendorBankAccountId = createVendorBankAccount(vendorId, inv.seller.SellerAccountID, inv.seller.SellerBic, inv.seller.SellerAccountName);
    }

    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, "move_type", "=", "in_invoice");
    add_val(vals, "company_id", "=", loggedOnCompanyId);
    add_val(vals, "partner_id", "=", vendorId);

    int fiscal_position_id = getFiscalPositionId();
    if(fiscal_position_id >= 0) 
        add_val(vals, "fiscal_position_id", "=",  fiscal_position_id);

    add_val(vals, "date", "=", insertDashesToDate(inv.InvoiceDate));
    add_val(vals, "invoice_date", "=", insertDashesToDate(inv.InvoiceDate));    //format 2025-08-15    
    add_val(vals, "invoice_date_due", "=", insertDashesToDate(inv.InvoiceDueDate));    //format 2025-08-15
    add_val(vals, "payment_reference", "=", inv.EpiRemittanceInfoIdentifier);  // supplier bill number.
    add_val(vals, "ref", "=", inv.InvoiceNumber);  // supplier bill number.

    add_val(vals, "x_studio_eio_invoice_identifier", "=", inv.getEIOInvoiceIdentifier());
    add_val(vals, "x_studio_epiref", "=", inv.EpiRemittanceInfoIdentifier); 
    add_val(vals, "x_studio_buyerref", "=", inv.BuyerReferenceIdentifier);
    add_val(vals, "x_studio_orderid", "=", inv.OrderIdentifier);
    
    //set invoice lines from InvoiceRow
    std::vector<xmlrpc_c::value> invoice_lines;
    for (const auto& row : inv.rows) {
        std::map<std::string, xmlrpc_c::value> line_vals;

        std::string articlename = row.ArticleName;
        
        if(!row.ArticleDescription.empty()) {
            if(!articlename.empty()) articlename +="\n";
            articlename += row.ArticleDescription;
        }
        if(!row.RowFreeText.empty()) {
            if(!articlename.empty()) articlename +="\n";
            articlename += row.RowFreeText;
        }
        if(!row.SubRowFreeText.empty()) {
            if(!articlename.empty()) articlename +="\n";
            articlename += row.SubRowFreeText;
        }
        add_val(line_vals, "name", "=", articlename);

        if (!row.InvoicedQuantity.empty()) add_val(line_vals, "quantity", "=", std::stod(row.InvoicedQuantity));
        else if (!row.DeliveredQuantity.empty()) add_val(line_vals, "quantity", "=", std::stod(row.DeliveredQuantity));
        else if (!row.OrderedQuantity.empty()) add_val(line_vals, "quantity", "=", std::stod(row.OrderedQuantity));

        add_val(line_vals, "price_unit", "=",  std::stod(decimalFormat(row.UnitPriceAmount)));

        int tax_id = getCompanyTaxId(decimalFormat(row.RowVatRatePercent), loggedOnCompanyId);

        if(tax_id > 0) {
            std::vector<xmlrpc_c::value> tax_ids_vec;
            tax_ids_vec.push_back(xmlrpc_c::value_int(tax_id));
            line_vals["tax_ids"] = xmlrpc_c::value_array(tax_ids_vec);
        }
        //add_val(line_vals, "tax_ids", "=",  tax_ids);
        // Add more fields as needed

        
        invoice_lines.push_back(xmlrpc_c::value_struct(line_vals));
    }
    if (!invoice_lines.empty()) {
        // Odoo expects a list of tuples: [(0, 0, {...}), ...]
        std::vector<xmlrpc_c::value> lines_tuple;
        for (const auto& line : invoice_lines) {
            std::vector<xmlrpc_c::value> tuple;
            tuple.push_back(xmlrpc_c::value_int(0));
            tuple.push_back(xmlrpc_c::value_int(0));
            tuple.push_back(line);
            lines_tuple.push_back(xmlrpc_c::value_array(tuple));
        }
        vals["invoice_line_ids"] = xmlrpc_c::value_array(lines_tuple);
    }
    
    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_struct(vals));


    xmlrpc_c::value result;
    bool success = odooCommand("create", "account.move", records, &result);
    int billId = -1;
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {

        if(doc.HasMember("result") && doc["result"].IsInt()) {
            billId = doc["result"].GetInt();
            LOG(INFO) << inv.buyer.BuyerOrganisationName << ": New purchase invoice added to Odoo, seller " << inv.seller.SellerOrganisationName << " odoo id = " << billId;
        } else {
            LOG(ERROR) << "Failed to create vendor bill: Invalid response format";
            return -1;
        }
    }
    //add attachments if any
    if(!inv.attachments.empty()) {
        std::vector<int> att_odoo_ids;
        for(const auto& att : inv.attachments) {
            int aid = createVendorBillAttachment(att, billId);
            att_odoo_ids.push_back(aid);
        }
        /*
        std::vector<xmlrpc_c::value> attachment_ids;
        for(const auto& attid : att_odoo_ids) {
            attachment_ids.push_back(xmlrpc_c::value_int(attid));
        }
        vals["attachment_ids"] = xmlrpc_c::value_array(attachment_ids);
        */
    }
    return billId;
}
bool OdooAPI::getInvoiceRowById(rapidjson::Document &doc, int rowId) {
    //
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "id", "=", rowId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));

    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "account.move.line", domain, &result);
    if(success && convertResultToJson(result, doc)) {
         // Parse ret_str as JSON array to check if any results exist
         return true;
    }
    LOG(ERROR) << "Failed to get invoice row by id: " << rowId;
    return false;
}

int OdooAPI::OdooInvoiceToFinvoice(const rapidjson::Value& entry, FinvoiceInvoice& invoice) {
    
    // Read common invoice fields from JSON entry
    int invoiceId = entry.HasMember("id") && entry["id"].IsInt() ? entry["id"].GetInt() : -1;
    invoice.InvoiceNumber = entry.HasMember("name") && entry["name"].IsString() ? entry["name"].GetString() : "";

    invoice.InvoiceDate = entry.HasMember("invoice_date") && entry["invoice_date"].IsString() ? entry["invoice_date"].GetString() : "";
    string_replaceall(invoice.InvoiceDate, "-", "");//YYYY-MM-DD => YYYYMMDD

    invoice.InvoiceDueDate = entry.HasMember("invoice_date_due") && entry["invoice_date_due"].IsString() ? entry["invoice_date_due"].GetString() : "";
    string_replaceall(invoice.InvoiceDueDate, "-", "");//YYYY-MM-DD => YYYYMMDD

    invoice.PaymentOverDueFineFreeText =  "Viivästyskorko 16%";
    invoice.PaymentOverDueFinePercent =  "16,00";
    

    invoice.InvoiceTypeCode = "INV01";  //INV02 HYVITYSLASKU 
    
    invoice.OriginCode ="Original";
    invoice.InvoiceTypeText = "LASKU";
    invoice.InvoiceCurrencyCode = entry.HasMember("currency_id") && entry["currency_id"].IsArray() ? entry["currency_id"][1].GetString() : "EUR";

    invoice.InvoiceRecipientLanguageCode="FI";
    invoice.InvoiceTotalVatExcludedAmount =  string_fmt_money(entry.HasMember("amount_untaxed_signed") && entry["amount_untaxed_signed"].IsNumber() ? entry["amount_untaxed_signed"].GetDouble() : 0);
    invoice.InvoiceTotalVatAmount =  string_fmt_money(entry.HasMember("amount_tax_signed") && entry["amount_tax_signed"].IsNumber() ? entry["amount_tax_signed"].GetDouble() : 0);
    invoice.InvoiceTotalVatIncludedAmount =  string_fmt_money(entry.HasMember("amount_total_signed") && entry["amount_total_signed"].IsNumber() ? entry["amount_total_signed"].GetDouble() : 0);

    double RowsTotalVatExcludedAmount = 0;
    double RowsTotal = 0;
    if(entry.HasMember("invoice_line_ids") && entry["invoice_line_ids"].IsArray() ){
        auto rows = entry["invoice_line_ids"].GetArray();
        for(int i=0; i < rows.Size(); i++) {
           if (rows[i].IsInt()) {
                int rowId = rows[i].GetInt();
                rapidjson::Document docrow;
                if(getInvoiceRowById(docrow, rowId)) {
                   const rapidjson::Value& myrow = docrow[0];
                    InvoiceRow ir;
                    double unitPriceAmount =  myrow.HasMember("price_unit") && myrow["price_unit"].IsNumber() ? myrow["price_unit"].GetDouble() : 0;
                    int quantity =  myrow.HasMember("quantity") && myrow["quantity"].IsNumber() ? myrow["quantity"].GetInt() : 0;
                    double taxrate = getCompanyTaxRatePercentById(myrow.HasMember("tax_ids") && myrow["tax_ids"].IsArray() && myrow["tax_ids"].Size()>0 && myrow["tax_ids"][0].IsInt() ? myrow["tax_ids"][0].GetInt() : -1);
                    double pricesubtotal = myrow.HasMember("price_subtotal") && myrow["price_subtotal"].IsNumber() ? myrow["price_subtotal"].GetDouble() : 0;   

                    RowsTotalVatExcludedAmount += pricesubtotal;
                    ir.RowVatRatePercent = string_fmt_money(taxrate);
                    ir.UnitPriceAmount = string_fmt_money(unitPriceAmount);
                    ir.OrderedQuantity = std::to_string(quantity);
                    ir.DeliveredQuantity=ir.OrderedQuantity;
                    ir.InvoicedQuantity=ir.OrderedQuantity;

                    ir.RowVatAmount = "0";
                    if(taxrate > 0) {
                        ir.RowVatAmount = string_fmt_money(pricesubtotal * (taxrate / 100));
                    }
                    ir.RowVatExcludedAmount = string_fmt_money(pricesubtotal);
                    ir.RowVatIncludedAmount = string_fmt_money(pricesubtotal);
                    if(taxrate > 0) {
                        ir.RowVatIncludedAmount = string_fmt_money(pricesubtotal + pricesubtotal * taxrate / 100);
                    }
                    RowsTotal += (pricesubtotal);
                    if(taxrate > 0) {
                        RowsTotal += (pricesubtotal * taxrate / 100);
                    } 

                    ir.ArticleName = myrow.HasMember("name") && myrow["name"].IsString() ? myrow["name"].GetString() : "";
                    
                    invoice.rows.push_back(ir);
                }
                else {
                    LOG(ERROR) << "ignoring row " << i << " getInvoiceRowById failed"; 
                }
            }
            else {
                LOG(ERROR) << "ignoring row " << i << " not an integer"; 
            }
        }
    }
    invoice.RowsTotalVatExcludedAmount = std::to_string(RowsTotalVatExcludedAmount);


//seller info
    int seller_bank_id = -1;
    int buyer_id = -1;
    if(entry.HasMember("partner_bank_id") && entry["partner_bank_id"].IsArray()) {
        const rapidjson::Value& bank = entry["partner_bank_id"];
        if(bank.Size() > 0 && bank[0].IsInt()) {
            seller_bank_id = bank[0].GetInt();
        }
    }
    int seller_id = -1;
    if(entry.HasMember("company_id") && entry["company_id"].IsArray()) {
        const rapidjson::Value& partner = entry["company_id"];
        if(partner.Size() > 0 && partner[0].IsInt()) {
            seller_id = partner[0].GetInt();
        }
        if(partner.Size() > 1 && partner[1].IsString()) {
            invoice.seller.SellerOrganisationName = partner[1].GetString();
        }
    }
    if(entry.HasMember("partner_id") && entry["partner_id"].IsArray()) {
        const rapidjson::Value& partner = entry["partner_id"];
        if(partner.Size() > 0 && partner[0].IsInt()) {
            buyer_id = partner[0].GetInt();
        }
        if(partner.Size() > 1 && partner[1].IsString()) {
            invoice.buyer.BuyerOrganisationName = partner[1].GetString();
        }
    }
    int creator_uuid = 0;
    if(entry.HasMember("create_uid") && entry["create_uid"].IsArray()) {
        const rapidjson::Value& partner = entry["create_uid"];
        if(partner.Size() > 0 && partner[0].IsInt()) {
            creator_uuid = partner[0].GetInt();
        }
        if(partner.Size() > 1 && partner[1].IsString()) {
            invoice.seller.SellerContactPersonName = partner[1].GetString();
        }
    }
    invoice.seller.SellerPhoneNumberIdentifier = ""; 
    invoice.seller.SellerEmailaddressIdentifier = "";

    invoice.BuyerReferenceIdentifier = entry.HasMember("x_studio_buyerref") && entry["x_studio_buyerref"].IsString() ? entry["x_studio_buyerref"].GetString() : "";
    invoice.OrderIdentifier = entry.HasMember("x_studio_orderid") && entry["x_studio_orderid"].IsString() ? entry["x_studio_orderid"].GetString() : "";
    
    /*this is already done
    getCompanyInfoByCompanyId(buyer_id, 
        invoice.buyer.BuyerOrganisationTaxCode,
        invoice.buyer.BuyerStreetName,
        invoice.buyer.BuyerTownName,
        invoice.buyer.BuyerPostCodeIdentifier,
        invoice.buyer.BuyerOVT, 
        invoice.buyer.BuyerIntermediator);
    */
    invoice.EpiDate = invoice.InvoiceDate;

    //TODO if using factoring this should be set to factoring details

    getBankAccountBic(seller_bank_id, invoice.seller.SellerBic, invoice.seller.SellerAccountName, invoice.seller.SellerAccountID); //e.g. ITELFIHH
    invoice.EpiBfiIdentifier = invoice.seller.SellerBic;
    invoice.EpiNameAddressDetails = invoice.seller.SellerOrganisationName; //e.g. Comtec SMD Oy
    
    getCompanyInfoByCompanyId(seller_id, 
        invoice.seller.SellerOrganisationTaxCode,
        invoice.seller.SellerStreetName,
        invoice.seller.SellerTownName,
        invoice.seller.SellerPostCodeIdentifier,
        invoice.seller.SellerOVT, invoice.seller.SellerIntermediator,
        true);

    invoice.EpiBei = invoice.seller.SellerOrganisationTaxCode;
    std::string selleraccountid = invoice.seller.SellerAccountID;
    string_replaceall(selleraccountid, " ", "");
    invoice.EpiAccountID = selleraccountid; //IBAN FI9082134783847

    invoice.EpiInstructedAmount = string_fmt_money(RowsTotal, 2); //how much
    invoice.EpiDateOptionDate = invoice.InvoiceDueDate;
    invoice.EpiInstructedAmountCurrencyIdentifier = invoice.InvoiceCurrencyCode; //e.g. EUR

    invoice.EpiRemittanceInfoIdentifier = entry.HasMember("x_studio_epiref") && entry["x_studio_epiref"].IsString() ? entry["x_studio_epiref"].GetString() : "";
    if(invoice.EpiRemittanceInfoIdentifier =="") {
        std::string payref = entry.HasMember("payment_reference") && entry["payment_reference"].IsString() ? entry["payment_reference"].GetString() : "";
        if(payref == "") {
            invoice.EpiRemittanceInfoIdentifier = generateRandomEpiRef(invoice.InvoiceNumber);    
        }
        else {
            invoice.EpiRemittanceInfoIdentifier = payref;
        }
        //create 
        updateDomainField("account.move", invoiceId, "x_studio_epiref", invoice.EpiRemittanceInfoIdentifier);
    }
    //attachments
    if(entry.HasMember("attachment_ids") && entry["attachment_ids"].IsArray() ){
        auto atts = entry["attachment_ids"].GetArray();
        for(int i=0; i < atts.Size(); i++) {
           if (atts[i].IsInt()) {
                int attId = atts[i].GetInt();
                FinvoiceAttachment att;
                if(getVendorBillAttachmentById(attId, invoiceId, att)) {
                    invoice.attachments.push_back(att);
                }
                else {
                    LOG(ERROR) << "ignoring attachment " << i << " getVendorBillAttachmentById failed"; 
                }
            }
            else {
                LOG(ERROR) << "ignoring attachment " << i << " not an integer"; 
            }
        }
    }
    return invoiceId;
}
std::string formatFinvoiceAttachmentName(const std::string& attname) {
    /*
        Maventa attachments follow a strict naming convention: 
            use only lowercase letters (a-z), 
            numbers (0-9), 
            periods (.), 
            and underscores (_), 
            with a maximum length of 50 characters. 
            Special characters and spaces are not allowed. 
            For attachments to be correctly identified in a ZIP package 
            containing an XML, the invoice image and the XML file must 
            have the same filename (e.g., 12345.xml and 12345.pdf)
    */
    std::string formattedName = attname;
    // Convert to lowercase
    std::transform(formattedName.begin(), formattedName.end(), formattedName.begin(), ::tolower);
    // Replace spaces with underscores
    string_replaceall(formattedName, " ", "_");
    // replace any characters that are not alphanumeric, underscore, dot or dash with underscore
    for(int i=0; i < formattedName.size(); i++) {
        char c  = formattedName[i];
        if(!(std::isalnum(c) || c == '_' || c == '.' )){
            // Valid character, do nothing
            formattedName[i] = '_';
        }
    }
    return formattedName;
}
bool OdooAPI::getVendorBillAttachmentById(int attId, int res_id, FinvoiceAttachment& att) {
#if 1
    std::vector<xmlrpc_c::value> filters;
    //add_filter(&filters, "type", "=", "raw");
    add_filter(&filters, "id", "=", attId);


    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));


    std::map<std::string, xmlrpc_c::value> options;
    std::vector<xmlrpc_c::value> fields;
    fields.push_back(xmlrpc_c::value_string("name"));
    fields.push_back(xmlrpc_c::value_string("datas"));
    fields.push_back(xmlrpc_c::value_string("mimetype"));
    options["fields"] = xmlrpc_c::value_array(fields);
    
    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "ir.attachment", domain, &result, &options);
    rapidjson::Document doc;
    if (success && convertResultToJson(result, doc)) {
        if (doc.IsArray() && doc.Size() > 0) {
            const rapidjson::Value& entry = doc[0];
            if (entry.IsObject()) {
                if (entry.HasMember("name") && entry["name"].IsString()){
                    std::string attname = entry["name"].GetString();

                    //convert name to lowercase and replace spaces with underscores
                    att.AttachmentName = formatFinvoiceAttachmentName(attname);
                }
                if (entry.HasMember("mimetype") && entry["mimetype"].IsString())
                    att.AttachmentMimeType = entry["mimetype"].GetString();
                if (entry.HasMember("datas") && entry["datas"].IsString())
                    att.AttachmentContent = entry["datas"].GetString();
                if (entry.HasMember("id") && entry["id"].IsInt())
                    att.odooAttachmentId = entry["id"].GetInt();
                return true;
            }
        }
    }
    return false;
#else
    xmlrpc_c::value read_res;
    xmlrpc_c::clientSimple client;
    
    xmlrpc_c::paramList params;
    params.add(xmlrpc_c::value_string(db_));
    params.add(xmlrpc_c::value_int(loggedOnUserId));
    params.add(xmlrpc_c::value_string(apikey_));
    params.add(xmlrpc_c::value_string("ir.attachment"));
    params.add(xmlrpc_c::value_string("read"));
    

    std::vector<xmlrpc_c::value> domain;
    std::vector<xmlrpc_c::value> filters;
    //add_filter(&filters, "type", "=", "raw");
    add_filter(&filters, "id", "=", attId);
    domain.push_back(xmlrpc_c::value_array(filters));

    params.add(xmlrpc_c::value_array(domain));

    // Empty options dict
    std::map<std::string, xmlrpc_c::value> options;
    params.add(xmlrpc_c::value_struct(options));
    xmlrpc_c::value result;
    
    client.call(url_ + "/xmlrpc/2/object", "execute_kw", params, result);
    /*
    client.call(url_ + "/xmlrpc/2/object",
                "execute_kw",
                "({s:s,s:i,s:s,s:s,s:s,(i),s:{}})",
                "db", db_.c_str(),
                "uid", loggedOnUserId.c_str(),
                "password", apikey_.c_str(),
                "model", "ir.attachment",
                "method", "read",
                attId,
                "kw", xmlrpc_c::value_nil(),
                &read_res);
                */
#endif
}
bool OdooAPI::updateDomainField(std::string domain, int domain_id, const std::string &fieldName, const std::string &fieldValue) {

    std::vector<xmlrpc_c::value> ids;
    //add all record ids that will be updated
    ids.push_back(xmlrpc_c::value_int(domain_id));

    //add all fields that will be updated
    std::map<std::string, xmlrpc_c::value> vals;
    add_val(vals, fieldName, "=", fieldValue);

    std::vector<xmlrpc_c::value> records;
    records.push_back(xmlrpc_c::value_array(ids));
    records.push_back(xmlrpc_c::value_struct(vals));

    xmlrpc_c::value result;
    bool success = odooCommand("write", domain, records, &result);
    int recordsUpdated = 0;
    rapidjson::Document doc;
    if(success && convertResultToJson(result, doc)) {
        if(doc.HasMember("result") && doc["result"].IsInt()) {
            recordsUpdated = doc["result"].GetInt();
            return true;
        } else {
            LOG(ERROR) << "Failed to update records: Invalid response format";
            return false;
        }


    }
    return false;
}
int OdooAPI::processUnsentInvoices(std::function<std::string (const rapidjson::Value& entry, std::string odoo_maventa_status, std::string maventa_invoice_identifier, FinvoiceInvoice &invoice, bool &send_confirmed, std::string &send_error_msg)> processInvoiceCallback, int lastHowManyDays) {
    // Find unsent invoices
    std::vector<xmlrpc_c::value> filters;
    add_filter(&filters, "move_type", "=", "out_invoice");
    add_filter(&filters, "x_studio_maventa_status", "=", "sending");
    add_filter(&filters, "company_id", "=", loggedOnCompanyId);
    
    std::vector<xmlrpc_c::value> domain;
    domain.push_back(xmlrpc_c::value_array(filters));

    xmlrpc_c::value result;
    bool success = odooCommand("search_read", "account.move", domain, &result);
    rapidjson::Document doc;

    int successfully_sent = 0;
    if(success && convertResultToJson(result, doc)) {
         // Parse ret_str as JSON array to check if any results exist
        
        if (doc.IsArray() ) {
            if(!doc.Empty() && doc.Size() > 0) {
                //LOG(INFO) << "Found " << doc.Size() << " unsent invoices";
                // Enumerate over array entries
                for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
                    const rapidjson::Value& entry = doc[i];
                    std::string x_studio_eio_invoice_identifier="";
                    std::string x_studio_eio_ovt="";
                    std::string BuyerOrganisationTaxCode="";
                    std::string BuyerStreetName="";
                    std::string BuyerTownName="";
                    std::string BuyerPostCodeIdentifier="";
                    std::string BuyerOVT="";
                    std::string BuyerIntermediator="";
                    std::string x_studio_maventa_status="";
                    if (entry.IsObject()) {
                        if(entry.HasMember("x_studio_maventa_status") && entry["x_studio_maventa_status"].IsString()) {
                            x_studio_maventa_status = entry["x_studio_maventa_status"].GetString();
                        }
                        if(entry.HasMember("x_studio_eio_invoice_identifier") && entry["x_studio_eio_invoice_identifier"].IsString()) {
                            x_studio_eio_invoice_identifier = entry["x_studio_eio_invoice_identifier"].GetString();
                        }
                        
                        if(x_studio_maventa_status == "sending") {
                            int buyer_id = 0;
                            if(entry.HasMember("partner_id") && entry["partner_id"].IsArray() && entry["partner_id"].Size() > 0) {
                                buyer_id = entry["partner_id"][0].GetInt();
                                getCompanyInfoByCompanyId(buyer_id, 
                                    BuyerOrganisationTaxCode,
                                    BuyerStreetName,
                                    BuyerTownName,
                                    BuyerPostCodeIdentifier,
                                    BuyerOVT, 
                                    BuyerIntermediator);
                            }
                            if(BuyerOVT !="" && BuyerIntermediator != "") {
                                FinvoiceInvoice invoice;
                                invoice.buyer.BuyerOrganisationTaxCode = BuyerOrganisationTaxCode;
                                invoice.buyer.BuyerStreetName = BuyerStreetName;
                                invoice.buyer.BuyerTownName = BuyerTownName;
                                invoice.buyer.BuyerPostCodeIdentifier = BuyerPostCodeIdentifier;
                                invoice.buyer.BuyerOVT = BuyerOVT;
                                invoice.buyer.BuyerIntermediator = BuyerIntermediator;

                                int invoiceId = entry.HasMember("id") && entry["id"].IsInt() ? entry["id"].GetInt() : -1;
                                if(invoiceId > 0) {
                                    // Call the callback function to process the invoice
                                    bool send_confirmed = false;
                                    std::string send_error_msg;
                                    std::string maventa_invoice_id = processInvoiceCallback(entry, x_studio_maventa_status, x_studio_eio_invoice_identifier, invoice, send_confirmed, send_error_msg);
                                    if(!maventa_invoice_id.empty()) {
                                        // save the maventa invoice id to odoo, and update status
                                        updateDomainField("account.move", invoiceId, "x_studio_eio_invoice_identifier", maventa_invoice_id);
                                        if(send_confirmed){
                                            updateDomainField("account.move", invoiceId, "x_studio_maventa_status", "senddone");
                                            LOG(INFO) << "Invoice processed and send confirmed: " << maventa_invoice_id << " for invoice: " << invoice.InvoiceNumber;
                                        }
                                        if(!send_error_msg.empty()){
                                            updateDomainField("account.move", invoiceId, "x_studio_maventa_status", "senderror");
                                            updateDomainField("account.move", invoiceId, "x_studio_maventa_error", send_error_msg);
                                            LOG(INFO) << "Invoice processed WITH ERROR: " << maventa_invoice_id << " for invoice: " << invoice.InvoiceNumber << " error message: " << send_error_msg;
                                            continue;
                                        }
                                        
                                        //LOG(INFO) << "Invoice processed and updated with maventa_invoice_id: " << maventa_invoice_id << " for invoice: " << invoice.InvoiceNumber;
                                        successfully_sent++;
                                    }
                                    else {
                                        LOG(ERROR) << "Processing invoice callback failed for invoice, did not get a maventa_invoice_id: " << invoice.InvoiceNumber;
                                    }
                                }
                                else {
                                    int invoiceId = entry.HasMember("id") && entry["id"].IsInt() ? entry["id"].GetInt() : -1;
                                    LOG(ERROR) << "Failed to convert Odoo invoice to Finvoice: " << invoiceId;
                                    std::string errormsg = "Failed to convert Odoo invoice to Finvoice: Uknown reason";
                                    updateDomainField("account.move", invoiceId, "x_studio_maventa_status", "senderror");
                                    updateDomainField("account.move", invoiceId, "x_studio_maventa_error", errormsg);
                                }
                            }
                            else {
                                std::string partner_name = "";
                                if(entry.HasMember("partner_id") && entry["partner_id"].IsArray()) {
                                    const rapidjson::Value& partner = entry["partner_id"];
                                    if(partner.Size() > 1 && partner[1].IsString()) {
                                        partner_name = partner[1].GetString();
                                    }
                                }    
                                else if(entry.HasMember("invoice_partner_display_name")) {
                                    partner_name = entry["invoice_partner_display_name"].GetString();
                                }
                                else if(entry.HasMember("invoice_partner_display_name")) {
                                    partner_name = entry["invoice_partner_display_name"].GetString();
                                }
                                int invoiceId = entry.HasMember("id") && entry["id"].IsInt() ? entry["id"].GetInt() : -1;
                                LOG(INFO) << "Skipping invoice (id: " << std::to_string(invoiceId) << ", to: "<< partner_name << "). Missing OVT/Intermediator. Fix and re-send!";
                                std::string errormsg = partner_name + std::string(" is missing OVT/Intermediator");
                                updateDomainField("account.move", invoiceId, "x_studio_maventa_status", "senderror");
                                updateDomainField("account.move", invoiceId, "x_studio_maventa_error", errormsg);
                            }
                        }
                        else {
                            LOG(INFO) << "Skipping invoice not sending: " << x_studio_eio_invoice_identifier << " " << BuyerOVT << " " << BuyerIntermediator << std::endl;    
                        }
                    }
                }
                return successfully_sent; // Number of unsent invoices processed
            }
            else {
                //LOG(DEBUG) << "No entries found in search_read result.";
                return 0; // No entries found
            }
        } 
        return 0; //no invoices to process
    }
    return 0; // Command failed
}