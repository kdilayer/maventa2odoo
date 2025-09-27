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