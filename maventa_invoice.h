#pragma once
#include <string>
#include <rapidjson/document.h>

class MaventaInvoice {
public:
    explicit MaventaInvoice(const std::string& id);

    // Setters
    void setSender(const rapidjson::Value& senderObj);
    void setSenderBid(const std::string& bid);
    void setSenderEia(const std::string& eia);
    void setSenderName(const std::string& name);
    void setSenderCountry(const std::string& country);

    void setRecipient(const rapidjson::Value& senderObj);
    void setRecipientBid(const std::string& bid);
    void setRecipientEia(const std::string& eia);
    void setRecipientName(const std::string& name);
    void setRecipientCountry(const std::string& country);
    void setRecipientOperator(const std::string& op);

    // Getters
    std::string getId() const { return id_; }
    std::string getSenderBid() const { return sender_bid_; }
    std::string getSenderEia() const { return sender_eia_; }
    std::string getSenderName() const { return sender_name_; }
    std::string getSenderCountry() const { return sender_country_; }

    std::string getRecipientBid() const { return recipient_bid_; }
    std::string getRecipientEia() const { return recipient_eia_; }
    std::string getRecipientName() const { return recipient_name_; }
    std::string getRecipientCountry() const { return recipient_country_; }
    std::string getRecipientOperator() const { return recipient_operator_; }

private:
    std::string id_;
    std::string sender_bid_;
    std::string sender_eia_;
    std::string sender_name_;
    std::string sender_country_;

    std::string recipient_bid_;
    std::string recipient_eia_;
    std::string recipient_name_;
    std::string recipient_country_;
    std::string recipient_operator_;
};