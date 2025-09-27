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
