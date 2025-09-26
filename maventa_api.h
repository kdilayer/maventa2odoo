#pragma once
#include <string>
#include <functional>
#include "maventa_invoice.h"
#include "finvoice_invoice.h"

class MaventaAPI {
    std::string profile_name;
    std::string access_token;
    std::string token_type; // e.g., "Bearer"
    std::string scope; // e.g., "read write"
    int expires_in; // in seconds
    int expires_at; // timestamp when the token expires

    int currentTimestampSeconds();
    bool loadProfile();
    bool saveProfile();
    bool has_error = false;

    std::string sendFile(std::string xml, std::string filename="invoice.xml", std::string mimetype="application/xml");
    bool validateXml(std::string xml);
public:
    MaventaAPI(std::string profileName):
        profile_name(profileName),
        access_token(""),
        token_type(""),
        scope(""),
        expires_in(0) {
            loadProfile();
        }
    ~MaventaAPI() {
        saveProfile();
    }
    bool tokenValid();
    bool authenticate(const std::string& client_id,
                                         const std::string& client_secret,
                                         const std::string& vendor_api_key);
    std::string uploadInvoice(FinvoiceInvoice &invoice);
    int processReceivedInvoices(std::string profilename, std::function<bool (FinvoiceInvoice &invoice)> processInvoiceCallback, int lastHowManyDays=7);
    std::string getInvoiceXml(MaventaInvoice & inv);
    std::string getInvoiceImage(MaventaInvoice & inv);
    std::string getInvoiceAttachment(MaventaInvoice & inv, std::string href);
    std::string getExtendedDetails(MaventaInvoice & inv);
    std::string getInvoiceStatus(std::string invoice_id);
};