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

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>
#include <iostream>
#include <map>
#include "maventa_api.h"
#include "odoo_api.h"


#include "version_num.h"
#include "build_defs.h"
#include <unistd.h> 

#include "util.h"
#include <rapidxml.hpp>
#include <rapidxpath.hpp>
#include <xml2json.hpp>
#include <rapidjson/error/en.h>
#include "config_profile.h"


INITIALIZE_EASYLOGGINGPP

using namespace std;
void doHelp(const char *app, const char* ver){
     fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "Input data can be piped or use -i option. Output can be a file or stdout.\n"
            "Example: %s -h\n"
            "\n"
             " -h            Print out this help\n"
             "\n\n"
            "%s version %s\n",
            app,
            app,
            app, ver);
}
std::string buildTime = formattedString(
    "%c%c.%c%c.%c%c%c%c-%c%c:%c%c:%c%c", 
    BUILD_DAY_CH0,  BUILD_DAY_CH1,
    BUILD_MONTH_CH0, BUILD_MONTH_CH1,
    BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
    BUILD_HOUR_CH0, BUILD_HOUR_CH1,
    BUILD_MIN_CH0,  BUILD_MIN_CH1,
        BUILD_SEC_CH0, BUILD_SEC_CH1
    );
std::string versionNumber= formattedString("%c.%c-%s%s", 
    VERSION_MAJOR_INIT, 
    VERSION_MINOR_INIT, 
    buildTime.c_str(),
    #ifdef _DEBUG
        "d"
    #else
        "r"
    #endif
);
std::string serverName = "maventa2odoo";
int main(int argc, char *argv[]) {
   int c = 0;
   std::string configFile = "maventa2odoo.conf";
   while ((c = getopt (argc, argv, "c:h")) != -1) {
        switch (c){
            case 'c':
                configFile = optarg;
                break;
            case 'h':
                doHelp(serverName.c_str(), versionNumber.c_str());
                return -1;
            continue;
        
        }
    }   
    if (optind != argc)    {
        fprintf(stderr, "A non option was supplied\n");
        doHelp(serverName.c_str(), versionNumber.c_str());
        return -1;
    }  
    if(configFile == "") {
        fprintf(stderr, "Config file not specified\n");
        doHelp(serverName.c_str(), versionNumber.c_str());
        return -1;  
    }
    LOG(INFO) << "Starting " << serverName << " version " << versionNumber; 
    #ifdef _DEBUG
        configFile += ".debug";
    #endif

    std::string config = ReadFileContent(configFile);

    rapidjson::Document layout;
    rapidjson::ParseResult iok = layout.Parse(config.c_str());
    if(iok) {
        LOG(INFO) << "Config file " << configFile << " loaded successfully";
    } else {
        LOG(ERROR) << "Failed to parse config file " << configFile << ": "
                   << rapidjson::GetParseError_En(layout.GetParseError())
                   << " (offset " << layout.GetErrorOffset() << ")";
        // Optionally, print the line number:
        size_t offset = layout.GetErrorOffset();
        size_t line = 1;
        for (size_t i = 0; i < offset && i < config.size(); ++i) {
            if (config[i] == '\n') ++line;
        }
        LOG(ERROR) << "Parse error occurred at line: " << line;
        return -1;
    }
    if (!layout.IsObject() || !layout.HasMember("profiles") || !layout["profiles"].IsArray()) {
        LOG(ERROR) << "Invalid config file format: 'profiles' array not found";
        return -1;
    }
    const rapidjson::Value& profiles = layout["profiles"];
    //LOG(DEBUG) << "Found " << profiles.Size() << " profiles";
    for (rapidjson::SizeType i = 0; i < profiles.Size(); ++i) {
        const rapidjson::Value& profile = profiles[i];

        ConfigProfile configProfile(profile, i);

        //LOG(INFO) << "Profile " << i << ": " << configProfile.getName();
        OdooAPI odooApi(
            configProfile.getOdooUrl(),
            configProfile.getOdooDb(),
            configProfile.getOdooUsername(),
            configProfile.getOdooApiKey(),
            configProfile.getOdooCompanyId()
        );
        if(odooApi.authenticate()){
            //LOG(INFO) << "Odoo authentication successful for profile " << i  << ": " << configProfile.getName();
            
            MaventaAPI maventaApi(configProfile.getName());
            if(!maventaApi.tokenValid()) {
                //LOG(INFO) << "Maventa token not valid for profile " << i << ", authenticating...";
                bool authok = maventaApi.authenticate(
                    configProfile.getMaventaClientId(),
                    configProfile.getMaventaClientSecret(),
                    configProfile.getMaventaVendorApiKey()
                );
                if (!authok) {
                    LOG(ERROR) << "Failed to get Maventa token for profile " << i;
                    continue;       
                }
            }
            int senttomaventa = odooApi.processUnsentInvoices([&odooApi, &maventaApi, &configProfile](const rapidjson::Value& entry, std::string odoo_maventa_status, std::string maventa_invoice_identifier, FinvoiceInvoice &invoice, bool &send_confirmed, std::string &send_error_msg) {
                    // In draft
                    // "state": "draft", // "posted", "cancelled"
                    // "status_in_payment": "draft", //"in_payment", "paid", "partial", "reversed", "blocked", "invoicing_legacy", "draft", "cancelled"
                    // "is_move_sent": false  -> It indicates that the invoice/payment has been sent or the PDF has been generated."
                    // "is_being_sent": false, -> Is the move being sent asynchronously"
                    // "move_sent_values": "not_sent", //"sent"

                    // after verified
                    // "state": "posted",
                    // "status_in_payment": "not_paid",
                    // "is_move_sent": false,
                    // "is_being_sent": false, 
                    // "move_sent_values": "not_sent",

                    // after sending
                    // "state": "posted",
                    // "status_in_payment": "not_paid",
                    // "is_move_sent": true,
                    // "move_sent_values": "sent",

                    //SendMaventaInvoice Contexctual action
                    //record['x_studio_maventa_status']='sending'
                    //maventa_status:
                    //"draft" -> "None"  or cannot send, missing information"
                    //"sending" -> "Waiting to send" (Invoice will be sent via maventa)
                    
                    //"senddone" -> "Sent" (Invoice has been sent via maventa)
                    //"senderror" -> "Send resulted in error" -> more info from invoice.maventa_error
                    //"missinginfo" -> "Send resulted in missing information check" -> more info from invoice.maventa_error

                    //
                    //    Initial status="draft"
                    //   Press send via Maventa ->
                    //       internal check to see that all info exists
                    //       if ok, status will be "sending"
                    //       else alert dialog and status will go back to "draft"
                    //   once maventa2odoo has picked it up status will be
                    //       senddone -> if all was ok
                    //       senderror -> if some error occured
                    //               -> from here you can go back to draft fix error and resend,
                    //               -> but how to change maventa_status back to "draft" ????
                    
                    if(odoo_maventa_status == "sending" && !maventa_invoice_identifier.empty()){
                        //means we have sent to maventa but we have not get confirmation yet
                        std::string status = maventaApi.getInvoiceStatus(maventa_invoice_identifier);
                        rapidjson::Document stat;
                        rapidjson::ParseResult iok = stat.Parse(status.c_str());
                        
                        //rapidjson::StringBuffer buffer;
                        //rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                        //stat.Accept(writer);
                        //LOG(INFO)<< "buff:" << buffer.GetString();

                        if (stat.IsArray()) {
                            for(int si=0; si < stat.Size(); si++) {
                                const rapidjson::Value& item = stat[si];
                                if (item.IsObject() && item.HasMember("type") && item["type"].IsString() ) {
                                    std::string type = item["type"].GetString();
                                    if(type == "SENT" && item.HasMember("message") && item["message"].IsNull()) {
                                        send_confirmed = true;
                                    }
                                    if(type == "ERROR" && item.HasMember("message") && item["message"].IsString()) {
                                        send_error_msg = item["message"].GetString();
                                    }
                                }
                            }
                        }
                        // If we reach here, it means the invoice is still being processed
                        return maventa_invoice_identifier;
                    }
                    
                    int invoiceId = odooApi.OdooInvoiceToFinvoice(entry, invoice);
                    std::string maventa_invoice_id = maventaApi.uploadInvoice(invoice);
                    if(maventa_invoice_id.empty() || maventa_invoice_id == "-1") {
                        send_error_msg = "Failed to upload invoice to Maventa";
                        LOG(ERROR) << configProfile.getName() << ": Failed to upload invoice " << invoice.InvoiceNumber << " to Maventa";
                        return std::string("-1");
                    }
                    
                    LOG(INFO) << configProfile.getName() << ": Sales invoice sent to to maventa: " << invoice.InvoiceNumber
                            << ", Seller: " << invoice.seller.SellerOrganisationName
                            << ", Buyer: " << invoice.buyer.BuyerOrganisationName
                            << " maventa_id: " << maventa_invoice_id;
                    return maventa_invoice_id;
            }, 7);
            LOG(INFO) << configProfile.getName() << ": Sales invoices sent to maventa: " << std::to_string(senttomaventa);

            int importedttoodoo = maventaApi.processReceivedInvoices(configProfile.getName(), [&odooApi](FinvoiceInvoice &invoice) {
                if(odooApi.vendorBillExists(invoice.getEIOInvoiceIdentifier()) && !odooApi.hasError() ) {
                    //LOG(INFO) << "Vendor bill already exists for invoice " << invoice.InvoiceNumber << ", skipping";
                    return false; // Skip this invoice, continue ok
                }
                // Create this invoice in odoo
                int uid = odooApi.createVendorBill(invoice);
                if (uid > 0) {
                    //LOG(INFO) << "Vendor bill created successfully with UID: " << uid;
                    return true;
                } else {
                    LOG(ERROR) << "Failed to create vendor bill in Odoo";
                }
                return false; // Continue processing next invoices
            });
            LOG(INFO) << configProfile.getName() << ": Purchase invoices imported to Odoo: " << std::to_string(importedttoodoo);

        } else {
            LOG(ERROR) << "Odoo authentication failed for profile " << i << ": " << configProfile.getName();
        }

    }
}
