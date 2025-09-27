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
#include "config_profile.h"

ConfigProfile::ConfigProfile(const rapidjson::Value& profile, int i) {
    if (profile.IsObject() && profile.HasMember("name") && profile["name"].IsString()) {
        name = profile["name"].GetString();
    } else {
        LOG(DEBUG) << "Name not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("maventa_client_id") && profile["maventa_client_id"].IsString()) {
        maventa_client_id = profile["maventa_client_id"].GetString();
    } else {
        LOG(DEBUG) << "Maventa Client ID not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("maventa_client_secret") && profile["maventa_client_secret"].IsString()) {
        maventa_client_secret = profile["maventa_client_secret"].GetString();
    } else {
        LOG(DEBUG) << "Maventa Client Secret not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("maventa_vendor_api_key") && profile["maventa_vendor_api_key"].IsString()) {
        maventa_vendor_api_key = profile["maventa_vendor_api_key"].GetString();
    } else {
        LOG(DEBUG) << "Maventa Vendor API Key not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("odoo_url") && profile["odoo_url"].IsString()) {
        odoo_url = profile["odoo_url"].GetString();
    } else {
        LOG(DEBUG) << "Odoo URL not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("odoo_db") && profile["odoo_db"].IsString()) {
        odoo_db = profile["odoo_db"].GetString();
    } else {
        LOG(DEBUG) << "Odoo Database not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("odoo_username") && profile["odoo_username"].IsString()) {
        odoo_username = profile["odoo_username"].GetString();
    } else {
        LOG(DEBUG) << "Odoo Username not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("odoo_api_key") && profile["odoo_api_key"].IsString()) {
        odoo_api_key = profile["odoo_api_key"].GetString();
    } else {
        LOG(DEBUG) << "Odoo API Key not found in profile " << i;
    }
    if (profile.IsObject() && profile.HasMember("odoo_company_id") && profile["odoo_company_id"].IsInt()) {
        odoo_company_id = profile["odoo_company_id"].GetInt();
    } else {
        LOG(DEBUG) << "Odoo Company ID not found in profile " << i;
    }
}