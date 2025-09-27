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
#include <vector>

struct SellerPartyDetails {
    std::string SellerOrganisationName;
    std::string SellerOrganisationTaxCode;
    std::string SellerOrganisationIdentifier;
    std::string SellerDepartment;
    std::string SellerStreetName;
    std::string SellerTownName;
    std::string SellerPostCodeIdentifier;
    std::string SellerCountryCode;
    std::string SellerCountryName;
    std::string SellerPhoneNumberIdentifier;
    std::string SellerEmailaddressIdentifier;
    std::string SellerWebaddressIdentifier;
    std::string SellerAccountID;
    std::string SellerAccountName;
    std::string SellerBic;
    std::string SellerVatRegistrationId;
    std::string SellerOrganisationUnitNumber;
    std::string SellerOVT; ///OVT tunnus
    std::string SellerIntermediator; //välittäjätunnus

    std::string SellerContactPersonName;

};

struct BuyerPartyDetails {
    std::string BuyerOrganisationName;
    std::string BuyerOrganisationTaxCode;
    std::string BuyerPartyIdentifier; 
    std::string BuyerOrganisationIdentifier;
    std::string BuyerDepartment;
    std::string BuyerStreetName;
    std::string BuyerTownName;
    std::string BuyerPostCodeIdentifier;
    std::string BuyerCountryCode;
    std::string BuyerPhoneNumberIdentifier;
    std::string BuyerEmailaddressIdentifier;
    std::string BuyerWebaddressIdentifier;
    std::string BuyerAccountID;
    std::string BuyerBic;
    std::string BuyerVatRegistrationId;
    std::string BuyerOrganisationUnitNumber;
    std::string BuyerOVT; ///OVT tunnus
    std::string BuyerIntermediator; //välittäjätunnus
};

struct InvoiceRow {
    std::string ArticleIdentifier;
    std::string ArticleName;
    std::string ArticleDescription;
    std::string RowFreeText;
    std::string SubRowFreeText;
    std::string UnitPriceNetAmount;
    std::string DeliveredQuantity;
    std::string InvoicedQuantity;
    std::string RowAmount;
    std::string AmountCurrencyIdentifier;


    std::string OrderedQuantity;
    std::string UnitPriceAmount;
    std::string RowVatRatePercent;
    std::string RowVatAmount;
    std::string RowVatExcludedAmount;
    std::string RowVatIncludedAmount;
    std::string RowDiscountPercent;
    std::string RowDiscountAmount;
    std::string RowUnitCode;
    std::string RowDescription;
    std::string RowOrderLineReference;
    std::string RowDeliveryDate;
    std::string RowBuyerArticleIdentifier;
    std::string RowSellerArticleIdentifier;
    std::string RowCommentText;
};
struct FinvoiceAttachment {
    std::string AttachmentName;
    std::string AttachmentMimeType;
    std::string AttachmentContent;
    int odooAttachmentId = 0; // id in odoo system
};
class FinvoiceInvoice {
    std::string getXmlMessageTransmissionDetails();
    std::string getXmlSellerPartyDetails();
    std::string getXmlSellerComdetails();
    std::string getXmlSellerInformationDetails();
    std::string getXmlBuyerPartyDetails();
    std::string getXmlDeliveryDetails();
    std::string getXmlInvoiceDetails();
    std::string getXmlFactoringAgreementDetails();
    std::string getXmlInvoiceRows();
    std::string getXmlEpiDetails();
    std::string getXmlAttachmentsDetails(std::vector<FinvoiceAttachment> &attachments);
    std::string getXmlAttachmentMessageTransmissionDetails(std::vector<FinvoiceAttachment> &attachments);
    std::string getXmlAttachmentDetails(std::vector<FinvoiceAttachment> &attachments);
public:
    // InvoiceDetails fields
    std::string messageId; // Unique message identifier for the invoice message (when sending)
    std::string InvoiceNumber;
    std::string InvoiceDate; //format: YYYYMMDD
    std::string InvoiceTypeCode;
    std::string OriginCode;
    std::string InvoiceTypeText;
    std::string InvoiceRecipientCode;
    std::string InvoiceRecipientText;
    std::string InvoiceRecipientLanguageCode;
    std::string InvoiceCurrencyCode;
    std::string InvoiceTotalVatExcludedAmount;
    std::string InvoiceTotalVatAmount;
    std::string InvoiceTotalVatIncludedAmount;
    std::string RowsTotalVatExcludedAmount;
    std::string InvoiceFreeText;
    std::string PaymentTermsFreeText;
    std::string PaymentOverDueFinePercent;
    std::string PaymentOverDueFineFreeText;
    std::string InvoiceDueDate;
    std::string InvoiceUrlText;
    std::string InvoiceUrlNameText;

    std::string OrderIdentifier;
    std::string BuyerReferenceIdentifier;
    
    std::string EpiDate; //e.g. 20250814, format: YYYYMMDD
    std::string EpiReference;
    std::string EpiBfiIdentifier; //e.g DABAFIHH
    std::string EpiNameAddressDetails; //eg. Elisa Oyj
    std::string EpiBei; //Tax number
    std::string EpiAccountID; //IBAN

    std::string EpiRemittanceInfoIdentifier; //viitenumero
    std::string EpiInstructedAmount;
    std::string EpiDateOptionDate;
    std::string EpiInstructedAmountCurrencyIdentifier; //e.g. EUR

    std::string eio_invoice_identifier; // Unique identifier for the invoice in Maventa (einvoice operator invoice identifier)

    SellerPartyDetails seller;
    BuyerPartyDetails buyer;
    std::vector<InvoiceRow> rows;
    std::vector<FinvoiceAttachment> attachments;
    // ... all other Finvoice fields

    bool parseFromXml(const std::string& xml);
    void setEIOInvoiceIdentifier(const std::string& identifier) {
        eio_invoice_identifier = identifier;
    }
    std::string getEIOInvoiceIdentifier() const {
        return eio_invoice_identifier;
    }

    std::string getXmlFinvoiceMessage(std::vector<FinvoiceAttachment> &attachments);
    std::string getXmlAttachmentMessage(std::vector<FinvoiceAttachment> &attachments, bool includetransmissiondetails=true);
    std::string getXmlFinvoiceEnvelopeHeader();

};