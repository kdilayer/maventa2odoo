#include "finvoice_invoice.h"
#include <rapidxml.hpp>
#include <sstream>
#include "util.h"
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <iomanip>


std::string formatTaxCode(const SellerPartyDetails& seller, const std::string& epiBei_) {
    std::string sellerCountryCode = seller.SellerCountryCode;
    std::string epiBei = string_trim(epiBei_, " \t\n\r"); // Trim whitespace

    //try to extract country code from vat number
    if(sellerCountryCode == "" && startsWithCountryCode(epiBei)){
         sellerCountryCode = getFirstTwoChars(epiBei);
    }
    //try to extract country country name
    if(sellerCountryCode == "" && seller.SellerCountryName != "") {
        sellerCountryCode = getCountryCodeFromName(seller.SellerCountryName);
    }
    //try to extract country code from IBAN if available
    if(sellerCountryCode == "" && seller.SellerAccountID != "") {
        // Extract country code from IBAN if available
        if(seller.SellerAccountID.size() >= 2) {
            sellerCountryCode = getFirstTwoChars(seller.SellerAccountID);
        }
    }
    // If no country code is available, default to FI
    if(sellerCountryCode == "") {
        sellerCountryCode = "FI";
    }
    if(startsWithCountryCode(epiBei)){
        //remove country code prefix if it exists
        epiBei = epiBei.substr(2);
    }
    
    string_replaceall(epiBei, "-", ""); // Remove any dashes
    epiBei = sellerCountryCode+""+epiBei; // Prepend country code

    return epiBei;
}
bool FinvoiceInvoice::parseFromXml(const std::string& xml) {

    std::string xml_copy2 = xml;
    WriteFileContent("/tmp/finvoice_invoice.xml", xml_copy2, true);
    using namespace rapidxml;
    std::vector<char> xml_copy(xml.begin(), xml.end());
    xml_copy.push_back('\0');
    xml_document<> doc;
    try {
        doc.parse<0>(&xml_copy[0]);
        xml_node<>* root = doc.first_node("Finvoice");
        if (!root) return false;

        if(auto mtd = root->first_node("MessageTransmissionDetails")) {
            if(auto msd = mtd->first_node("MessageSenderDetails")) {
                if (auto n1 = msd->first_node("FromIdentifier")) seller.SellerOVT = n1->value();    
                if (auto n2 = msd->first_node("FromIntermediator")) seller.SellerIntermediator = n2->value();    
            }
        }
        // Parse InvoiceDetails
        if (auto details = root->first_node("InvoiceDetails")) {
            
            
            if (auto n = details->first_node("InvoiceNumber")) InvoiceNumber = n->value();
            if (auto d = details->first_node("InvoiceDate")) InvoiceDate = d->value();
            if (auto t = details->first_node("InvoiceTypeCode")) InvoiceTypeCode = t->value();
            if (auto c = details->first_node("OriginCode")) OriginCode = c->value();
            if (auto s = details->first_node("InvoiceTypeText")) InvoiceTypeText = s->value();
            if (auto r = details->first_node("InvoiceRecipientCode")) InvoiceRecipientCode = r->value();
            if (auto e = details->first_node("InvoiceRecipientText")) InvoiceRecipientText = e->value();
            if (auto f = details->first_node("InvoiceRecipientLanguageCode")) InvoiceRecipientLanguageCode = f->value();
            if (auto g = details->first_node("InvoiceCurrencyCode")) InvoiceCurrencyCode = g->value();
            if (auto h = details->first_node("InvoiceTotalVatExcludedAmount")) InvoiceTotalVatExcludedAmount = h->value();
            if (auto i = details->first_node("InvoiceTotalVatAmount")) InvoiceTotalVatAmount = i->value();
            if (auto j = details->first_node("InvoiceTotalVatIncludedAmount")) InvoiceTotalVatIncludedAmount = j->value();
            if (auto l = details->first_node("RowsTotalVatExcludedAmount")) RowsTotalVatExcludedAmount = l->value();
            if (auto n1 = details->first_node("BuyerReferenceIdentifier")) BuyerReferenceIdentifier = n1->value();
            if (auto n2 = details->first_node("OrderIdentifier")) OrderIdentifier = n2->value();
            
            if (auto k = details->first_node("PaymentTermsDetails")) {
                // Parse all PaymentTermsFreeText nodes
                for (auto freeTextNode = k->first_node("PaymentTermsFreeText"); 
                    freeTextNode; 
                    freeTextNode = freeTextNode->next_sibling("PaymentTermsFreeText")) 
                {
                    if (freeTextNode->value()) {
                        if(PaymentTermsFreeText != "") PaymentTermsFreeText+="\n";
                        PaymentTermsFreeText+=freeTextNode->value();
                    }
                }
                if (auto m = k->first_node("PaymentOverDueFinePercent")) PaymentOverDueFinePercent = m->value();
                if (auto l = k->first_node("PaymentOverDueFineFreeText")) PaymentOverDueFineFreeText = l->value();
                if (auto o = k->first_node("InvoiceDueDate")) InvoiceDueDate = o->value();
            }
            
            // Parse all InvoiceFreeText nodes
            if (auto details = root->first_node("InvoiceDetails")) {
                for (auto freeTextNode = details->first_node("InvoiceFreeText"); 
                    freeTextNode; 
                    freeTextNode = freeTextNode->next_sibling("InvoiceFreeText")) 
                {
                    if (freeTextNode->value()) {
                        if(InvoiceFreeText != "") InvoiceFreeText+="\n";
                        InvoiceFreeText+=freeTextNode->value();
                    }
                }
            }

            if (auto o = details->first_node("InvoiceUrlText")) InvoiceUrlText = o->value();
            if (auto p = details->first_node("InvoiceUrlNameText")) InvoiceUrlNameText = p->value();
            // Add more fields as needed from the Finvoice 3.0 standard
        }

        // Parse SellerPartyDetails
        if (auto sellerNode = root->first_node("SellerPartyDetails")) {
            if (auto n = sellerNode->first_node("SellerOrganisationName")) seller.SellerOrganisationName = n->value();
            if (auto t = sellerNode->first_node("SellerOrganisationTaxCode")) seller.SellerOrganisationTaxCode = t->value();
            if (auto a = sellerNode->first_node("SellerOrganisationIdentifier")) seller.SellerOrganisationIdentifier = a->value();
            if (auto b = sellerNode->first_node("SellerDepartment")) seller.SellerDepartment = b->value();
            if (auto c = sellerNode->first_node("SellerStreetName")) seller.SellerStreetName = c->value();
            if (auto d = sellerNode->first_node("SellerTownName")) seller.SellerTownName = d->value();
            if (auto e = sellerNode->first_node("SellerPostCodeIdentifier")) seller.SellerPostCodeIdentifier = e->value();
            if (auto f = sellerNode->first_node("SellerCountryCode")) seller.SellerCountryCode = f->value();
            if (auto g = sellerNode->first_node("SellerPhoneNumberIdentifier")) seller.SellerPhoneNumberIdentifier = g->value();
            if (auto h = sellerNode->first_node("SellerEmailaddressIdentifier")) seller.SellerEmailaddressIdentifier = h->value();
            if (auto i = sellerNode->first_node("SellerWebaddressIdentifier")) seller.SellerWebaddressIdentifier = i->value();

            if(seller.SellerStreetName == "") {
                if (auto j = sellerNode->first_node("SellerPostalAddressDetails")) {
                    if (auto a1 = j->first_node("SellerTownName")) seller.SellerTownName = a1->value();
                    if (auto a = j->first_node("SellerStreetName")) seller.SellerStreetName = a->value();
                    if (auto b = j->first_node("SellerPostCodeIdentifier")) seller.SellerPostCodeIdentifier = b->value();
                    if (auto c = j->first_node("SellerCountryCode")) seller.SellerCountryCode = c->value();
                    if (auto d = j->first_node("SellerCountryName")) seller.SellerCountryName = d->value();
                }
            }
            
            if (auto k = sellerNode->first_node("SellerVatRegistrationDetails")) {
                if (auto vat = k->first_node("SellerVatRegistrationId")) seller.SellerVatRegistrationId = vat->value();
            }
            // Add more SellerPartyDetails fields as needed from the Finvoice 3.0 standard
        }
        



        if (auto oun = root->first_node("SellerOrganisationUnitNumber")) seller.SellerOrganisationUnitNumber = oun->value();

        if (auto sid = root->first_node("SellerInformationDetails")) {
            if(seller.SellerTownName=="") {
                if (auto n = sid->first_node("SellerHomeTownName")) seller.SellerTownName = n->value();
            }
            if(seller.SellerPhoneNumberIdentifier=="") {
                if (auto n = sid->first_node("SellerPhoneNumber")) seller.SellerPhoneNumberIdentifier = n->value();
            }
            if(seller.SellerEmailaddressIdentifier=="") {
                if (auto n = sid->first_node("SellerCommonEmailaddressIdentifier")) seller.SellerEmailaddressIdentifier = n->value();
            }
            if(seller.SellerWebaddressIdentifier=="") {
                if (auto n = sid->first_node("SellerWebaddressIdentifier")) seller.SellerWebaddressIdentifier = n->value();
            }   
            
            //TODO add support for many accounts
            if (auto j = sid->first_node("SellerAccountDetails")) {
                if (auto san = j->first_node("SellerAccountName")) seller.SellerAccountName = san->value();
                if (auto acc = j->first_node("SellerAccountID")) seller.SellerAccountID = acc->value();
                if (auto bic = j->first_node("SellerBic")) seller.SellerBic = bic->value();
            }
        }

        // Parse BuyerPartyDetails
        if (auto buyerNode = root->first_node("BuyerPartyDetails")) {
            if (auto n = buyerNode->first_node("BuyerOrganisationName")) buyer.BuyerOrganisationName = n->value();
            if (auto t = buyerNode->first_node("BuyerOrganisationTaxCode")) buyer.BuyerOrganisationTaxCode = t->value();
            if (auto t = buyerNode->first_node("BuyerPartyIdentifier")) buyer.BuyerPartyIdentifier = t->value();
            if (auto a = buyerNode->first_node("BuyerOrganisationIdentifier")) buyer.BuyerOrganisationIdentifier = a->value();
            if (auto b = buyerNode->first_node("BuyerDepartment")) buyer.BuyerDepartment = b->value();
            if (auto c = buyerNode->first_node("BuyerStreetName")) buyer.BuyerStreetName = c->value();
            if (auto d = buyerNode->first_node("BuyerTownName")) buyer.BuyerTownName = d->value();
            if (auto e = buyerNode->first_node("BuyerPostCodeIdentifier")) buyer.BuyerPostCodeIdentifier = e->value();
            if (auto f = buyerNode->first_node("BuyerCountryCode")) buyer.BuyerCountryCode = f->value();
            if (auto g = buyerNode->first_node("BuyerPhoneNumberIdentifier")) buyer.BuyerPhoneNumberIdentifier = g->value();
            if (auto h = buyerNode->first_node("BuyerEmailaddressIdentifier")) buyer.BuyerEmailaddressIdentifier = h->value();
            if (auto i = buyerNode->first_node("BuyerWebaddressIdentifier")) buyer.BuyerWebaddressIdentifier = i->value();
            if (auto j = buyerNode->first_node("BuyerAccountDetails")) {
                if (auto acc = j->first_node("BuyerAccountID")) buyer.BuyerAccountID = acc->value();
                if (auto bic = j->first_node("BuyerBic")) buyer.BuyerBic = bic->value();
            }
            if (auto k = buyerNode->first_node("BuyerVatRegistrationDetails")) {
                if (auto vat = k->first_node("BuyerVatRegistrationId")) buyer.BuyerVatRegistrationId = vat->value();
            }
           
            // Add more BuyerPartyDetails fields as needed from the Finvoice 3.0 standard
        }
        if (auto boun = root->first_node("BuyerOrganisationUnitNumber")) buyer.BuyerOrganisationUnitNumber = boun->value();

        // Parse EpiDetails
        if (auto epiNode = root->first_node("EpiDetails")) {
            
            if (auto epiDate = epiNode->first_node("EpiIdentificationDetails")->first_node("EpiDate")) EpiDate = epiDate->value();
            if (auto epiRef = epiNode->first_node("EpiIdentificationDetails")->first_node("EpiReference")) EpiReference = epiRef->value();

            if (auto epiBfiIdentifier = epiNode->first_node("EpiPartyDetails")->first_node("EpiBfiPartyDetails")->first_node("EpiBfiIdentifier")) EpiBfiIdentifier = epiBfiIdentifier->value();
            if (auto epiNameAddressDetails = epiNode->first_node("EpiPartyDetails")->first_node("EpiBeneficiaryPartyDetails")->first_node("EpiNameAddressDetails")) EpiNameAddressDetails = epiNameAddressDetails->value();
            if (auto epiBei = epiNode->first_node("EpiPartyDetails")->first_node("EpiBeneficiaryPartyDetails")->first_node("EpiBei")) EpiBei = formatTaxCode(seller, epiBei->value());
            if (auto epiAccountID = epiNode->first_node("EpiPartyDetails")->first_node("EpiBeneficiaryPartyDetails")->first_node("EpiAccountID")) EpiAccountID = epiAccountID->value();

            if (auto epiRemittanceInfoIdentifier = epiNode->first_node("EpiPaymentInstructionDetails")->first_node("EpiRemittanceInfoIdentifier")) EpiRemittanceInfoIdentifier = epiRemittanceInfoIdentifier->value();
            if (auto epiInstructedAmount = epiNode->first_node("EpiPaymentInstructionDetails")->first_node("EpiInstructedAmount")) EpiInstructedAmount = epiInstructedAmount->value();
            if (auto epiInstructedAmountCurrencyIdentifier = epiNode->first_node("EpiPaymentInstructionDetails")->first_node("EpiInstructedAmount")->first_attribute("AmountCurrencyIdentifier")) EpiInstructedAmountCurrencyIdentifier = epiInstructedAmountCurrencyIdentifier->value();
            if (auto epiDateOptionDate = epiNode->first_node("EpiPaymentInstructionDetails")->first_node("EpiDateOptionDate")) EpiDateOptionDate = epiDateOptionDate->value();

        }
        // Parse InvoiceRows
        if (auto rowsNode = root->first_node("InvoiceRow")) {
            for (auto row = rowsNode; row; row = row->next_sibling("InvoiceRow")) {
                InvoiceRow invoiceRow;

                if(row->first_node("SubInvoiceRow")) {
                    if (auto a = row->first_node("SubInvoiceRow")->first_node("SubRowFreeText"))  invoiceRow.SubRowFreeText = a->value();
                }
                if (auto a = row->first_node("ArticleIdentifier")) invoiceRow.ArticleIdentifier = a->value();
                if (auto a = row->first_node("ArticleName")) invoiceRow.ArticleName = a->value();
                if (auto a = row->first_node("ArticleDescription")) invoiceRow.ArticleDescription = a->value();
                if (auto a = row->first_node("UnitPriceNetAmount")) invoiceRow.UnitPriceNetAmount = a->value();
                if (auto a = row->first_node("RowFreeText")) invoiceRow.RowFreeText = a->value();

                if (auto a = row->first_node("DeliveredQuantity")) invoiceRow.DeliveredQuantity = a->value();
                if (auto a = row->first_node("InvoicedQuantity")) invoiceRow.InvoicedQuantity = a->value();
                if (auto a = row->first_node("RowAmount")) invoiceRow.RowAmount = a->value();

                
                
                

                if (auto q = row->first_node("OrderedQuantity")) invoiceRow.OrderedQuantity = q->value();
                if (auto u = row->first_node("UnitPriceAmount")) invoiceRow.UnitPriceAmount = u->value();
                if (auto v = row->first_node("RowVatRatePercent")) invoiceRow.RowVatRatePercent = v->value();
                if (auto w = row->first_node("RowVatAmount")) invoiceRow.RowVatAmount = w->value();
                if (auto x = row->first_node("RowVatExcludedAmount")) invoiceRow.RowVatExcludedAmount = x->value();
                if(row->first_node("RowVatExcludedAmount") ) {
                    if(row->first_node("RowVatExcludedAmount")->first_attribute("AmountCurrencyIdentifier")) {
                        if (auto x = row->first_node("RowVatExcludedAmount")->first_attribute("AmountCurrencyIdentifier")) invoiceRow.AmountCurrencyIdentifier = x->value();
                    }
                }

                if (auto y = row->first_node("RowVatIncludedAmount")) invoiceRow.RowVatIncludedAmount = y->value();
                if (auto z = row->first_node("RowDiscountPercent")) invoiceRow.RowDiscountPercent = z->value();
                if (auto aa = row->first_node("RowDiscountAmount")) invoiceRow.RowDiscountAmount = aa->value();
                if (auto ab = row->first_node("RowUnitCode")) invoiceRow.RowUnitCode = ab->value();
                if (auto ac = row->first_node("RowDescription")) invoiceRow.RowDescription = ac->value();
                if (auto ad = row->first_node("RowOrderLineReference")) invoiceRow.RowOrderLineReference = ad->value();
                if (auto ae = row->first_node("RowDeliveryDate")) invoiceRow.RowDeliveryDate = ae->value();
                if (auto af = row->first_node("RowBuyerArticleIdentifier")) invoiceRow.RowBuyerArticleIdentifier = af->value();
                if (auto ag = row->first_node("RowSellerArticleIdentifier")) invoiceRow.RowSellerArticleIdentifier = ag->value();
                if (auto ah = row->first_node("RowCommentText")) invoiceRow.RowCommentText = ah->value();
                // Add more fields as needed from the Finvoice 3.0 standard

                rows.push_back(invoiceRow);
            }
        }
        
        // ...parse all other Finvoice fields
        if(EpiBei.empty()) {
            EpiBei = seller.SellerOrganisationTaxCode;
        }
        //fix 
        std::string sellerCountryCode = seller.SellerCountryCode;
        if(sellerCountryCode == ""){
            sellerCountryCode = getFirstTwoChars(EpiBei);
            seller.SellerCountryCode= sellerCountryCode;
        }
        std::string sellerCountryName =seller.SellerCountryName;
        if(sellerCountryName == "") {
            sellerCountryName = getCountryNameForCode(sellerCountryCode);
            seller.SellerCountryName=sellerCountryName;
        }
        return true;
    } catch (...) {
        return false;
    }
}
std::string FinvoiceInvoice::getXmlMessageTransmissionDetails() {
    return formattedString(R"(<MessageTransmissionDetails>
	<MessageSenderDetails>
		<FromIdentifier SchemeID="0037">%s</FromIdentifier>
		<FromIntermediator>%s</FromIntermediator>
	</MessageSenderDetails>
	<MessageReceiverDetails>
		<ToIdentifier SchemeID="0037">%s</ToIdentifier>
		<ToIntermediator>%s</ToIntermediator>
	</MessageReceiverDetails>
    <MessageDetails>
      <MessageIdentifier>M2O0002%s</MessageIdentifier>
      <MessageTimeStamp>%s</MessageTimeStamp>
    </MessageDetails>
  </MessageTransmissionDetails>)",
    seller.SellerOVT.c_str(),
    seller.SellerIntermediator.c_str(),
    buyer.BuyerOVT.c_str(),
    buyer.BuyerIntermediator.c_str(),
    messageId.c_str(),
    getTimestamp("").c_str()
    );
}
std::string FinvoiceInvoice::getXmlSellerPartyDetails() {
    return formattedString(R"(<SellerPartyDetails>
    <SellerPartyIdentifier>%s</SellerPartyIdentifier>
    <SellerOrganisationName>%s</SellerOrganisationName>
    <SellerOrganisationTaxCode>%s</SellerOrganisationTaxCode>
    <SellerPostalAddressDetails>
      <SellerStreetName>%s</SellerStreetName>
      <SellerTownName>%s</SellerTownName>
      <SellerPostCodeIdentifier>%s</SellerPostCodeIdentifier>
    </SellerPostalAddressDetails>
  </SellerPartyDetails>)",
    seller.SellerOrganisationTaxCode.c_str(),
    seller.SellerOrganisationName.c_str(),
    seller.SellerOrganisationTaxCode.c_str(),
    seller.SellerStreetName.c_str(),
    seller.SellerTownName.c_str(),
    seller.SellerPostCodeIdentifier.c_str()
);
}
std::string FinvoiceInvoice::getXmlSellerComdetails() {
    
    return formattedString(R"(<SellerContactPersonName>%s</SellerContactPersonName>
  <SellerCommunicationDetails>
    <SellerPhoneNumberIdentifier>%s</SellerPhoneNumberIdentifier>
    <SellerEmailaddressIdentifier>%s</SellerEmailaddressIdentifier>
  </SellerCommunicationDetails>)",
    
    seller.SellerContactPersonName.c_str(),
    seller.SellerPhoneNumberIdentifier.c_str(),
    seller.SellerEmailaddressIdentifier.c_str()
    );
}
std::string FinvoiceInvoice::getXmlSellerInformationDetails() {
    std::string selleraccountid = seller.SellerAccountID;
    string_replaceall(selleraccountid, " ", "");
    return formattedString(R"(<SellerInformationDetails>
    <SellerCommonEmailaddressIdentifier>%s</SellerCommonEmailaddressIdentifier>
    <SellerAccountDetails>
      <SellerAccountID IdentificationSchemeName="IBAN">%s</SellerAccountID>
      <SellerBic IdentificationSchemeName="BIC">%s</SellerBic>
      <SellerAccountName>%s</SellerAccountName>
    </SellerAccountDetails>
  </SellerInformationDetails>)",
    
  seller.SellerEmailaddressIdentifier.c_str(),
  selleraccountid.c_str(),
  seller.SellerBic.c_str(),
  seller.SellerAccountName.c_str()
    );
}
std::string FinvoiceInvoice::getXmlBuyerPartyDetails() {
    return formattedString(R"(<BuyerPartyDetails>
    <BuyerPartyIdentifier>%s</BuyerPartyIdentifier>
    <BuyerOrganisationName>%s</BuyerOrganisationName>
    <BuyerOrganisationTaxCode>%s</BuyerOrganisationTaxCode>
    <BuyerPostalAddressDetails>
      <BuyerStreetName>%s</BuyerStreetName>
      <BuyerTownName>%s</BuyerTownName>
      <BuyerPostCodeIdentifier>%s</BuyerPostCodeIdentifier>
    </BuyerPostalAddressDetails>
  </BuyerPartyDetails>
  <BuyerOrganisationUnitNumber/>
  <BuyerContactPersonName/>)",
    buyer.BuyerOrganisationTaxCode.c_str(),
    buyer.BuyerOrganisationName.c_str(),
    buyer.BuyerOrganisationTaxCode.c_str(),
    buyer.BuyerStreetName.c_str(),
    buyer.BuyerTownName.c_str(),
    buyer.BuyerPostCodeIdentifier.c_str()
    );
}
std::string FinvoiceInvoice::getXmlDeliveryDetails() {
    return formattedString(R"(<DeliveryDetails>
    <DeliveryDate Format="CCYYMMDD">%s</DeliveryDate>
  </DeliveryDetails>)",
        getTimestamp("YYYYMMDD").c_str()
    );
}
std::string FinvoiceInvoice::getXmlInvoiceDetails() {
    //<SellersBuyerIdentifier>1001</SellersBuyerIdentifier>
    return formattedString(R"(<InvoiceDetails>
    <InvoiceTypeCode>INV01</InvoiceTypeCode>
    <InvoiceTypeText>INVOICE</InvoiceTypeText>
    <OriginCode>Original</OriginCode>
    <InvoiceNumber>%s</InvoiceNumber>
    <InvoiceDate Format="CCYYMMDD">%s</InvoiceDate>
    <OrderIdentifier>%s</OrderIdentifier>
    <AgreementIdentifier></AgreementIdentifier>
    <BuyerReferenceIdentifier>%s</BuyerReferenceIdentifier>
    <InvoiceTotalVatExcludedAmount AmountCurrencyIdentifier="EUR">%s</InvoiceTotalVatExcludedAmount>
    <InvoiceTotalVatAmount AmountCurrencyIdentifier="EUR">%s</InvoiceTotalVatAmount>
    <InvoiceTotalVatIncludedAmount AmountCurrencyIdentifier="EUR">%s</InvoiceTotalVatIncludedAmount>
    <VatSpecificationDetails/>
    <PaymentTermsDetails>
      <PaymentTermsFreeText/>
      <InvoiceDueDate Format="CCYYMMDD">%s</InvoiceDueDate>
      <PaymentOverDueFineDetails>
        <PaymentOverDueFineFreeText>%s</PaymentOverDueFineFreeText>
        <PaymentOverDueFinePercent>%s</PaymentOverDueFinePercent>
      </PaymentOverDueFineDetails>
    </PaymentTermsDetails>
  </InvoiceDetails>)",
    InvoiceNumber.c_str(),
    getTimestamp("YYYYMMDD").c_str(),
    OrderIdentifier.c_str(),
    BuyerReferenceIdentifier.c_str(),
    InvoiceTotalVatExcludedAmount.c_str(),
    InvoiceTotalVatAmount.c_str(),
    InvoiceTotalVatIncludedAmount.c_str(),
    InvoiceDueDate.c_str(),
    PaymentOverDueFineFreeText.c_str(),
    PaymentOverDueFinePercent.c_str()   
    );
}
std::string FinvoiceInvoice::getXmlFactoringAgreementDetails() {
    return "";
    /*TODO factoring
    return formattedString(R"(<FactoringAgreementDetails>
    <FactoringAgreementIdentifier>011</FactoringAgreementIdentifier>
    <TransmissionListIdentifier>1</TransmissionListIdentifier>
    <EndorsementClauseCode>F</EndorsementClauseCode>
    <FactoringFreeText>Tämä laskusaatava on siirretty Noja Rahoitus Oy:lle ja voidaan pätevä</FactoringFreeText>
    <FactoringFreeText>sti maksaa vain tilille:&#13;</FactoringFreeText>
    <FactoringFreeText>BIC: NDEAFIHH IBAN: FI67 1733 3000 0145 69&#13;</FactoringFreeText>
    <FactoringPartyIdentifier>2597279-7</FactoringPartyIdentifier>
    <FactoringPartyName>Noja Rahoitus Oy</FactoringPartyName>
    <FactoringPartyPostalAddressDetails>
      <FactoringPartyStreetName>Aurakatu 12 b</FactoringPartyStreetName>
      <FactoringPartyTownName>Turku</FactoringPartyTownName>
      <FactoringPartyPostCodeIdentifier>20100</FactoringPartyPostCodeIdentifier>
    </FactoringPartyPostalAddressDetails>
  </FactoringAgreementDetails>)");
  */
}
std::string FinvoiceInvoice::getXmlInvoiceRows() {
    std::string rows_xml;
    for (const auto& row : rows) {
        rows_xml += formattedString(R"(<InvoiceRow>
    <ArticleIdentifier>%s</ArticleIdentifier>
    <ArticleName>%s</ArticleName>
    <DeliveredQuantity QuantityUnitCode="kpl">%s</DeliveredQuantity>
    <InvoicedQuantity QuantityUnitCode="kpl">%s</InvoicedQuantity>
    <UnitPriceAmount AmountCurrencyIdentifier="EUR">%s</UnitPriceAmount>
    <RowPositionIdentifier/>
    <RowProposedAccountText/>
    <RowFreeText>%s</RowFreeText>
    <RowVatRatePercent>%s</RowVatRatePercent>
    <RowVatAmount AmountCurrencyIdentifier="EUR">%s</RowVatAmount>
    <RowVatExcludedAmount AmountCurrencyIdentifier="EUR">%s</RowVatExcludedAmount>
  </InvoiceRow>)",
    row.ArticleIdentifier.c_str(),
    row.ArticleName.c_str(),
    row.DeliveredQuantity.c_str(),
    row.InvoicedQuantity.c_str(),
        row.UnitPriceAmount.c_str(),
        row.RowFreeText.c_str(),
        row.RowVatRatePercent.c_str(),
        row.RowVatAmount.c_str(),
        row.RowVatExcludedAmount.c_str()
        );
        
    }
    return rows_xml;
}
std::string FinvoiceInvoice::getXmlEpiDetails() {
    return formattedString(R"(<EpiDetails>
    <EpiIdentificationDetails>
      <EpiDate Format="CCYYMMDD">%s</EpiDate>
      <EpiReference/>
    </EpiIdentificationDetails>
    <EpiPartyDetails>
      <EpiBfiPartyDetails>
        <EpiBfiIdentifier IdentificationSchemeName="BIC">%s</EpiBfiIdentifier>
      </EpiBfiPartyDetails>
      <EpiBeneficiaryPartyDetails>
        <EpiNameAddressDetails>%s</EpiNameAddressDetails>
        <EpiAccountID IdentificationSchemeName="IBAN">%s</EpiAccountID>
      </EpiBeneficiaryPartyDetails>
    </EpiPartyDetails>
    <EpiPaymentInstructionDetails>
      <EpiRemittanceInfoIdentifier IdentificationSchemeName="SPY">%s</EpiRemittanceInfoIdentifier>
      <EpiInstructedAmount AmountCurrencyIdentifier="EUR">%s</EpiInstructedAmount>
      <EpiCharge ChargeOption="SLEV"/>
      <EpiDateOptionDate Format="CCYYMMDD">%s</EpiDateOptionDate>
    </EpiPaymentInstructionDetails>
  </EpiDetails>)",
    EpiDate.c_str(),
    EpiBfiIdentifier.c_str(),
    EpiNameAddressDetails.c_str(),
    EpiAccountID.c_str(),
    EpiRemittanceInfoIdentifier.c_str(),
    EpiInstructedAmount.c_str(),
    EpiDateOptionDate.c_str());
}
std::string FinvoiceInvoice::getXmlAttachmentsDetails(std::vector<FinvoiceAttachment> &attachments) {
    if(attachments.size()==0)  {
        return "";
    }
    std::string attachments_xml="<AttachmentMessageDetails>";
    
    /*
    for(int i=0; i<attachments.size(); i++) {
        const auto& attachment = attachments[i];
        attachments_xml += formattedString(R"(
            <AttachmentMessageIdentifier>ATTM2O0002%s::attachments</AttachmentMessageIdentifier>
)",
        messageId.c_str()
        );
    }
        */

    attachments_xml += formattedString(R"(
        <AttachmentMessageIdentifier>ATTM2O0002%s::attachments</AttachmentMessageIdentifier>
)",
    messageId.c_str()
    );
    attachments_xml +="</AttachmentMessageDetails>";
    return attachments_xml;
}
std::string FinvoiceInvoice::getXmlAttachmentMessageTransmissionDetails(std::vector<FinvoiceAttachment> &attachments) {
    /*RefToMessageIdentifier contains the identifier (MessageId) of the invoice
    message that is linked to the attachment message.
    */
    std::string xml=formattedString(R"(<MessageTransmissionDetails>
	<MessageSenderDetails>
		<FromIdentifier SchemeID="0037">%s</FromIdentifier>
		<FromIntermediator>%s</FromIntermediator>
	</MessageSenderDetails>
	<MessageReceiverDetails>
		<ToIdentifier SchemeID="0037">%s</ToIdentifier>
		<ToIntermediator>%s</ToIntermediator>
	</MessageReceiverDetails>
        <MessageDetails>
            <MessageIdentifier>ATTM2O0002%s::attachments</MessageIdentifier>
            <MessageTimeStamp>%s</MessageTimeStamp>
            <RefToMessageIdentifier>M2O0002%s</RefToMessageIdentifier>
        </MessageDetails>
    </MessageTransmissionDetails>)",
    seller.SellerOVT.c_str(),
    seller.SellerIntermediator.c_str(),
    buyer.BuyerOVT.c_str(),
    buyer.BuyerIntermediator.c_str(),
    messageId.c_str(),
    getTimestamp("").c_str(),
    messageId.c_str()
    );
    return xml;
}
std::string calculate_sha1(std::string input){
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::stringstream ss;
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
std::string FinvoiceInvoice::getXmlAttachmentDetails(std::vector<FinvoiceAttachment> &attachments) {
    std::string attachment_details_xml;
    for(int i=0; i<attachments.size(); i++) {
        const auto& attachment = attachments[i];
        std::string sha1 = calculate_sha1(attachment.AttachmentContent);
        
        //YV1199015::attachments::
        attachment_details_xml += formattedString(R"(<AttachmentDetails>
        <AttachmentIdentifier>ATTM2O0002%s::attachments::%s</AttachmentIdentifier>
        <AttachmentContent>%s</AttachmentContent>
        <AttachmentName>%s</AttachmentName>
        <AttachmentMimeType>%s</AttachmentMimeType>
        <AttachmentSecureHash>%s</AttachmentSecureHash>
    </AttachmentDetails>)",
            messageId.c_str(),
            sha1.c_str(),
            attachment.AttachmentContent.c_str(),
            attachment.AttachmentName.c_str(),  
            attachment.AttachmentMimeType.c_str(),
            sha1.c_str()
                );
    }
    LOG(INFO) << "Attachment details xml: " << attachment_details_xml;
    return attachment_details_xml;
}
std::string FinvoiceInvoice::getXmlAttachmentMessage(std::vector<FinvoiceAttachment> &attachments, bool includetransmissiondetails) {

    if(attachments.size()==0)  {
        return "";
    }
    if(includetransmissiondetails == false){
        std::string attachment_xml=formattedString(R"(<FinvoiceAttachments xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="FinvoiceAttachments.xsd" Version="1.0">
    %s
</FinvoiceAttachments>)",
        getXmlAttachmentDetails(attachments).c_str()
        );
        return attachment_xml;

    }
    std::string attachment_xml=formattedString(R"(<?xml version="1.0" encoding="ISO-8859-15"?>
<FinvoiceAttachments xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="FinvoiceAttachments.xsd" Version="1.0">
    %s
    %s
</FinvoiceAttachments>)",
    getXmlAttachmentMessageTransmissionDetails(attachments).c_str(),
    getXmlAttachmentDetails(attachments).c_str()
    );
    return attachment_xml;
}
std::string FinvoiceInvoice::getXmlFinvoiceMessage(std::vector<FinvoiceAttachment> &attachments) {
   std::string invoice_xml=formattedString(R"(<?xml version="1.0" encoding="ISO-8859-15"?>
<?xml-stylesheet type="text/xsl" href="Finvoice.xsl"?>
<Finvoice xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="Finvoice3.0.xsd" Version="3.0">
  %s
  %s
  %s
  %s
  %s
  %s
  %s
  %s
  %s
  %s
</Finvoice>   
)",
    getXmlMessageTransmissionDetails().c_str(),
    getXmlSellerPartyDetails().c_str(),
    getXmlSellerComdetails().c_str(),
    getXmlSellerInformationDetails().c_str(),
    getXmlBuyerPartyDetails().c_str(),
    getXmlDeliveryDetails().c_str(),
    getXmlInvoiceDetails().c_str(),
    getXmlFactoringAgreementDetails().c_str(),
    getXmlInvoiceRows().c_str(),
    getXmlEpiDetails().c_str()
    //getXmlAttachmentsDetails(attachments).c_str()
    //getXmlAttachmentMessage(attachments, false).c_str()
);

    //WriteFileContent("/tmp/finvoice.xml", invoice_xml, true);
   return invoice_xml;
}

std::string FinvoiceInvoice::getXmlFinvoiceEnvelopeHeader() {

    std::string sb;
    sb.append("<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:eb=\"http://www.oasis-open.org/committees/ebxml-msg/schema/msg-header-2_0.xsd\">\n");
    sb.append("<SOAP-ENV:Header>\n");
    sb.append("<eb:MessageHeader xmlns:eb=\"http://www.oasis-open.org/committees/ebxml-msg/schema/msg-header-2_0.xsd\" SOAP-ENV:mustUnderstand=\"1\">\n");
    sb.append("<eb:From>\n");
    sb.append("<eb:PartyId>" + seller.SellerOVT+ "</eb:PartyId>\n"); //+*/OVT tunnus
    sb.append("<eb:Role>Sender</eb:Role>\n");
    sb.append("</eb:From>\n");
    sb.append("<eb:From>\n");
    sb.append("<eb:PartyId>" + seller.SellerIntermediator+ "</eb:PartyId>\n");
    sb.append("<eb:Role>Intermediator</eb:Role>\n");
    sb.append("</eb:From>\n");
    sb.append("<eb:To>\n");
    sb.append("<eb:PartyId>" + buyer.BuyerOVT+"</eb:PartyId>\n"); // FI1783093710005039
    sb.append("<eb:Role>Receiver</eb:Role>\n");
    sb.append("</eb:To>\n");
    sb.append("<eb:To>\n");
    sb.append("<eb:PartyId>" + buyer.BuyerIntermediator +"</eb:PartyId>\n"); // DABAFIHH
    sb.append("<eb:Role>Intermediator</eb:Role>\n");
    sb.append("</eb:To>\n");
    sb.append("<eb:CPAId>yoursandmycpa</eb:CPAId>\n");
    sb.append("<eb:ConversationId>1231235</eb:ConversationId>\n");
    sb.append("<eb:Service>Routing</eb:Service>\n");
    sb.append("<eb:Action>ProcessInvoice</eb:Action>\n");
    sb.append("<eb:MessageData>\n");

    sb.append("<eb:MessageId>M2O0002" + EpiRemittanceInfoIdentifier+"</eb:MessageId>\n");
    sb.append("<eb:Timestamp>" + getTimestamp("") + "</eb:Timestamp>\n"); // 2017-09-11T09:13:26
    sb.append("<eb:RefToMessageId/>\n");
    sb.append("</eb:MessageData>\n");
    sb.append("</eb:MessageHeader>\n");
    sb.append("</SOAP-ENV:Header>\n");
    sb.append("<SOAP-ENV:Body>\n");
    sb.append("<eb:Manifest eb:id=\"Manifest\" eb:version=\"2.0\">\n");
    sb.append("<eb:Reference eb:id=\"Finvoice\" xlink:href=\"" + EpiRemittanceInfoIdentifier+ "\">\n");
    sb.append("<eb:Schema eb:location=\"http://www.finvoice.info/finvoice.xsd\" eb:version=\"2.0\"/>\n");
    sb.append("</eb:Reference>\n");
    sb.append("</eb:Manifest>\n");
    sb.append("</SOAP-ENV:Body>\n");
    sb.append("</SOAP-ENV:Envelope>\n");

    return sb;
}