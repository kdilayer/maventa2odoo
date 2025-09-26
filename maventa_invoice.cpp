#include "maventa_invoice.h"

MaventaInvoice::MaventaInvoice(const std::string& id)
    : id_(id) {}

void MaventaInvoice::setRecipient(const rapidjson::Value& recipientObj) {
    if (recipientObj.HasMember("bid") && recipientObj["bid"].IsString())
        recipient_bid_ = recipientObj["bid"].GetString();
    if (recipientObj.HasMember("eia") && recipientObj["eia"].IsString())
        recipient_eia_ = recipientObj["eia"].GetString();
    if (recipientObj.HasMember("name") && recipientObj["name"].IsString())
        recipient_name_ = recipientObj["name"].GetString();
    if (recipientObj.HasMember("country") && recipientObj["country"].IsString())
        recipient_country_ = recipientObj["country"].GetString();
}

void MaventaInvoice::setSender(const rapidjson::Value& senderObj) {
    if (senderObj.HasMember("bid") && senderObj["bid"].IsString())
        sender_bid_ = senderObj["bid"].GetString();
    if (senderObj.HasMember("eia") && senderObj["eia"].IsString())
        sender_eia_ = senderObj["eia"].GetString();
    if (senderObj.HasMember("name") && senderObj["name"].IsString())
        sender_name_ = senderObj["name"].GetString();
    if (senderObj.HasMember("country") && senderObj["country"].IsString())
        sender_country_ = senderObj["country"].GetString();
}

void MaventaInvoice::setSenderBid(const std::string& bid) { sender_bid_ = bid; }
void MaventaInvoice::setSenderEia(const std::string& eia) { sender_eia_ = eia; }
void MaventaInvoice::setSenderName(const std::string& name) { sender_name_ = name; }
void MaventaInvoice::setSenderCountry(const std::string& country) { sender_country_ = country; }

void MaventaInvoice::setRecipientBid(const std::string& bid) { recipient_bid_ = bid; }
void MaventaInvoice::setRecipientEia(const std::string& eia) { recipient_eia_ = eia; }
void MaventaInvoice::setRecipientName(const std::string& name) { recipient_name_ = name; }
void MaventaInvoice::setRecipientCountry(const std::string& country) { recipient_country_ = country; }
void MaventaInvoice::setRecipientOperator(const std::string& op) { recipient_operator_ = op; }      
