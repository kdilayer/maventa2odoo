#include "maventa_api.h"
#include <curl/curl.h>
#include <iostream>
#include "util.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h> 
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>   
#include "maventa_invoice.h"
#include "finvoice_invoice.h"
#include <iconv.h>
#include "util.h"
#include "zipper.h"

int MaventaAPI::currentTimestampSeconds() {
    return time(nullptr);
}
bool MaventaAPI::saveProfile() {
    // Save the profile to a configuration file or other source
    // For simplicity, we assume the profile is saved in a JSON format
    if(!tokenValid()) {
        LOG(DEBUG) << "Token is not valid, not saving profile.";
        return true;
    }
    rapidjson::Document layout;
    layout.SetObject();
    rapidjson::Document::AllocatorType& allocator = layout.GetAllocator();

    layout.AddMember("access_token", rapidjson::Value(access_token.c_str(), allocator), allocator);
    layout.AddMember("token_type", rapidjson::Value(token_type.c_str(), allocator), allocator);
    layout.AddMember("scope", rapidjson::Value(scope.c_str(), allocator), allocator);

    layout.AddMember("expires_at", expires_at, allocator);

    std::string profile = "/tmp/maventa_api_profile_"+profile_name+".json"; // Example path
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    layout.Accept(writer);  
    std::string content = buffer.GetString();
    return WriteFileContent(profile, content, true);
}
bool MaventaAPI::loadProfile() {
    // Load the profile from a configuration file or other source
    // For simplicity, we assume the profile is already set in the constructor
    if (profile_name.empty()) {
        LOG(DEBUG) << "Profile name is empty, cannot load profile.";
        return false;
    }
    //LOG(DEBUG) << "Loading profile: " << profile_name;
    // Here you would typically read from a config file or database
    // For this example, we assume the profile is already set
    std::string profile = "/tmp/maventa_api_profile_"+profile_name+".json";
    std::string profileContent = ReadFileContent(profile);
    if (profileContent.empty()) {
        LOG(DEBUG) << "Failed to load profile content for: " << profile_name;
        return false;
    }
    rapidjson::Document layout;
    rapidjson::ParseResult iok = layout.Parse(profileContent.c_str());
    if (layout.IsObject() && layout.HasMember("expires_at")  && layout["expires_at"].IsInt()) {
        expires_at = layout["expires_at"].GetInt();
    } else {
        LOG(DEBUG) << "No valid 'expires_at' found in profile: " << profile_name;
        return false;

    }
    if (layout.IsObject() && layout.HasMember("access_token") && layout["access_token"].IsString()) {
        access_token = layout["access_token"].GetString();
    } else {
        LOG(DEBUG) << "No valid 'access_token' found in profile: " << profile_name;
        return false;
    }
    if (layout.IsObject() && layout.HasMember("token_type") && layout["token_type"].IsString()) {
        token_type = layout["token_type"].GetString();
    } else {
        LOG(DEBUG) << "No valid 'token_type' found in profile: " << profile_name;
        return false;
    }
    if (layout.IsObject() && layout.HasMember("scope") && layout["scope"].IsString()) {
        scope = layout["scope"].GetString();
    } else {
        LOG(DEBUG) << "No valid 'scope' found in profile: " << profile_name;
        return false;
    }
    return true;
}
bool MaventaAPI::tokenValid() {
    if (access_token.empty()) {
        LOG(DEBUG) << "Access token is empty, not valid.";
        return false;
    }
    long current_time = currentTimestampSeconds();
    if (current_time + 60 >= expires_at) { // Allow a 60 seconds buffer
        //LOG(DEBUG) << "Access token has expired at " << expires_at << ", current time is " << current_time;
        return false;
    }
    //LOG(DEBUG) << "Access token is valid until " << expires_at;
    return true;
}
bool MaventaAPI::authenticate(const std::string& client_id,
                                          const std::string& client_secret,
                                          const std::string& vendor_api_key) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, "https://ax.maventa.com/oauth2/token");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    std::string postfields = "grant_type=client_credentials"
                             "&client_id=" + client_id +
                             "&client_secret=" + client_secret +
                             "&vendor_api_key=" + vendor_api_key +
                             "&scope=eui";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    //LOG(DEBUG) << "Authentication response: " << response;
    // Parse JSON and extract access_token using rapidjson
    rapidjson::Document doc;
    if (doc.Parse(response.c_str()).HasParseError()) {
        LOG(DEBUG) << "Failed to parse JSON response: " << response << std::endl;
        return false;
    }
    if (doc.HasMember("access_token") && doc["access_token"].IsString()) {
        access_token = doc["access_token"].GetString();
        token_type = doc["token_type"].GetString();
        scope = doc["scope"].GetString();
        expires_in = doc["expires_in"].GetInt();
        expires_at = currentTimestampSeconds() + expires_in;
    } else {
        LOG(DEBUG) << "No access_token in response: " << response << std::endl;
        return false;
    }
    return true;
}

std::string iso_8859_15_to_utf8(const std::string& input) {
    iconv_t cd = iconv_open("UTF-8", "ISO-8859-15");
    if (cd == (iconv_t)-1) {
        LOG(ERROR) << "iconv_open failed for ISO-8859-15 to UTF-8";
        return input;
    }

    size_t inbytesleft = input.size();
    size_t outbytesleft = inbytesleft * 2 + 1; // UTF-8 may use more bytes
    std::string output(outbytesleft, 0);

    char* inbuf = const_cast<char*>(input.data());
    char* outbuf = &output[0];
    char* outbuf_start = outbuf;

    size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    if (result == (size_t)-1) {
        LOG(ERROR) << "iconv conversion failed";
        iconv_close(cd);
        return input;
    }
    iconv_close(cd);
    output.resize(outbuf - outbuf_start);
    return output;
}

int MaventaAPI::processReceivedInvoices(std::string profilename, std::function<bool (FinvoiceInvoice &invoice)> processInvoiceCallback, int lastHowManyDays) {
#if 0 // For debugging, use static file
    std::string invoice_id="1234567890"; // Example invoice ID
    std::vector<FinvoiceInvoice> invoices;
    FinvoiceInvoice finvoice_invoice;
    std::string inv_xml = ReadFileContent("/home/kari/maventa_invoice.xml"); // Example path
    if(inv_xml.empty()) {
        LOG(ERROR) << "Failed to read invoice XML file.";
        return 0;
    }
    
    finvoice_invoice.parseFromXml(inv_xml);
    finvoice_invoice.setEIOInvoiceIdentifier(invoice_id);
    return processInvoiceCallback(finvoice_invoice) ? 1 : 0;
#endif    
    int invoicesAddedCount = 0;
    if (tokenValid() == false) {
        LOG(ERROR) << "No access token available for invoice request.";
        has_error = true;
        return invoicesAddedCount;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        has_error = true;
        return invoicesAddedCount;
    }

    std::ostringstream url;
    url << "https://ax.maventa.com/v1/invoices?direction=RECEIVED"
        <<"&received_at_start=" << timestamp_to_string(currentTimestampSeconds() - 60 * 60 * 24 * lastHowManyDays); // Last 7 days
        //<< "&page=" << 1
        //<< "&per_page=" << 100;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        has_error = true;
        return invoicesAddedCount;
    }
    
    //WriteFileContent("/tmp/maventa_invoice.json", response, true);
    // Parse the JSON response using rapidjson
    rapidjson::Document doc;
    if (doc.Parse(response.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse JSON response: " << rapidjson::GetParseError_En(doc.GetParseError())
                   << " (offset " << doc.GetErrorOffset() << ")";
        has_error = true;
        return invoicesAddedCount;
    }
    //std::cout << "Response: " << response << std::endl;
    // Check for errors in the response
    if (doc.IsObject() && doc.HasMember("code") && doc["code"].IsString() && doc["code"].GetString() != std::string("auth_authorized")){
        std::string message = doc["code"].GetString();
        if(doc.HasMember("message") && doc["message"].IsString()) {
            message = doc["message"].GetString();
        }
        if(doc.HasMember("details") && doc["details"].IsString()) {
            message += " details: ";
            message += doc["details"].GetString();
        } else if (doc["details"].IsArray()) {
            message += " details: ";
            const rapidjson::Value& detailsArr = doc["details"];
            for (rapidjson::SizeType i = 0; i < detailsArr.Size(); ++i) {
                if (detailsArr[i].IsString()) {
                    if (i > 0) message += "; ";
                    message += detailsArr[i].GetString();
                }
            }
        }
        LOG(ERROR) << "Response code not ok, response code: " << doc["code"].GetString() << ", message: " << message;
        has_error = true;
        return invoicesAddedCount;
    }
    if(doc.IsArray() && doc.Size() > 0) {
        //LOG(INFO) << profilename << ": Found #" << doc.Size() << " invoices for the last " << lastHowManyDays << " days.";
    }
    else {
        LOG(INFO) << profilename << ": No invoices found for the last " << lastHowManyDays << " days.";
        return invoicesAddedCount; // No invoices found, but not an error
    }
    
    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
        const rapidjson::Value& invoice = doc[i];
        if (!invoice.IsObject()) {
            LOG(ERROR) << "Invoice is not an object at index " << i;
            continue;
        }
        if (!invoice.HasMember("id") || !invoice["id"].IsString()) {
            LOG(ERROR) << "Invoice at index " << i << " does not have a valid 'id' field.";
            continue;   
        }
        std::string invoice_id = invoice["id"].GetString();
        MaventaInvoice inv(invoice_id);
        inv.setSender(invoice["sender"]);
        inv.setRecipient(invoice["recipient"]);

        FinvoiceInvoice finvoice_invoice;
        finvoice_invoice.setEIOInvoiceIdentifier(invoice_id);
        std::string invoiceXml = getInvoiceXml(inv);
        
        //and the get the rest of the attachments from extended details
        std::string details = getExtendedDetails(inv);
        rapidjson::Document doc;
        rapidjson::ParseResult iok = doc.Parse(details.c_str());
        if (iok) {
            if (doc.IsObject() && doc.HasMember("files") && doc["files"].IsArray()) {
                for (rapidjson::SizeType i = 0; i < doc["files"].Size(); ++i) { 
                    const rapidjson::Value& file = doc["files"][i];
                    if (file.IsObject() && file.HasMember("id") && file["id"].IsString() &&
                        file.HasMember("filename") && file["filename"].IsString() &&
                        file.HasMember("mimetype") && file["mimetype"].IsString() && 
                        file.HasMember("href") && file["href"].IsString()) {

                        std::string attachment_id = file["id"].GetString();
                        std::string attachment_name = file["filename"].GetString();
                        std::string attachment_mime_type = file["mimetype"].GetString();
                        std::string href = file["href"].GetString();
                        //LOG(INFO) << "Found attachment: ID=" << attachment_id << ", Name=" << attachment_name << ", MIME Type=" << attachment_mime_type;
                        std::string attachment_b64 = getInvoiceAttachment(inv, href);
                        if(!attachment_b64.empty()) {
                            FinvoiceAttachment attachment;
                            attachment.AttachmentName = attachment_name;
                            attachment.AttachmentMimeType = attachment_mime_type;
                            attachment.AttachmentContent = attachment_b64;
                            finvoice_invoice.attachments.push_back(attachment);
                        }
                    } else {
                        LOG(ERROR) << "Invalid file object in extended details at index " << i;
                    }
                }
            }
            
        } else {
            LOG(ERROR) << "Failed to parse extended details JSON: " << rapidjson::GetParseError_En(doc.GetParseError())
                       << " (offset " << doc.GetErrorOffset() << ")";
        }
        //lastly get the invoice image
        std::string invoiceImageb64 = getInvoiceImage(inv);
        if(!invoiceImageb64.empty()) {
            FinvoiceAttachment attachment;
            attachment.AttachmentName = "invoice_"+invoice_id+".pdf";
            attachment.AttachmentMimeType = "application/pdf";
            attachment.AttachmentContent = invoiceImageb64;
            finvoice_invoice.attachments.push_back(attachment);
        }
        if(!invoiceXml.empty()) {
            #if 0 // For debugging, save the invoice XML to a file
                WriteFileContent("/tmp/maventa_invoice_"+invoice_id+".xml", invoiceXml, true);
            #endif 
            if (!finvoice_invoice.parseFromXml(invoiceXml)) {
                LOG(ERROR) << "Failed to parse invoice ID: " << invoice_id; 
            }
            else {
                //LOG(INFO) << "Parsed invoice ID: " << invoice_id << ", Invoice Number: " << finvoice_invoice.InvoiceNumber;
                if(processInvoiceCallback(finvoice_invoice)){
                    //LOG(INFO) << "Processed invoice ID: " << invoice_id << ", Invoice Number: " << finvoice_invoice.InvoiceNumber;
                    invoicesAddedCount++;
                }
            }
        }
        else {
            LOG(ERROR) << "Failed to get invoice by ID: " << invoice_id; 
        }
        
    }
    return invoicesAddedCount;
}
bool MaventaAPI::validateXml(std::string xml_content){
    return true;
    /*
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

     std::string response;
    std::string url = "https://validator-stage.maventa.com/validate";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());

    // Prepare multipart form
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filename(part, "invoice.xml");
    curl_mime_data(part, xml_content.c_str(), xml_content.size());
    curl_mime_type(part, "application/xml");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return false;
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return false;
    }
    response = iso_8859_15_to_utf8(response);
    LOG(DEBUG) << "response " << response;
    return true;
    */
}
std::string MaventaAPI::getInvoiceStatus(std::string invoice_id) {
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

    std::ostringstream url;
    url << "https://ax.maventa.com/v1/invoices/" << invoice_id << "/actions";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    return iso_8859_15_to_utf8(response);
}

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size)
{
  size_t i;
  size_t c;
  unsigned int width = 0x10;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i = 0; i < size; i += width) {
    //fprintf(stream, "%4.4lx: ", (long)i);
 
    /* show hex to the left 
    for(c = 0; c < width; c++) {
      if(i + c < size)
        fprintf(stream, "%02x ", ptr[i + c]);
      else
        fputs("   ", stream);
    }
 */
    /* show data on the right */
    for(c = 0; (c < width) && (i + c < size); c++) {
      char x = (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80) ? ptr[i + c] : '.';
      fputc(x, stream);
    }
 
    //fputc('\n', stream); /* newline */
  }
  fputc('\n', stream); /* newline */
}
static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *clientp)
{
  const char *text;
  (void)handle;
  (void)clientp;
 
  switch(type) {
  case CURLINFO_TEXT:
    fputs("== Info: ", stderr);
    fwrite(data, size, 1, stderr);
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }
 
  dump(text, stdout, (unsigned char *)data, size);
  return 0;
}
 
std::string MaventaAPI::sendFile(std::string xml_content, std::string filename, std::string mimetype){
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }
    std::string response;
    std::string url = "https://ax.maventa.com/v1/invoices";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());

    // Prepare multipart form
    curl_mime* mime = curl_mime_init(curl);

    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filename(part, filename.c_str());
    curl_mime_data(part, xml_content.c_str(), xml_content.size());
    curl_mime_type(part, mimetype.c_str());
/*
    if(attachments.size()>0) {
        for(size_t i=0; i<attachments.size(); i++) {
            auto attachment = &attachments[i];

            curl_mimepart* part = curl_mime_addpart(mime);
            curl_mime_name(part, "file");
            curl_mime_filename(part, attachment->AttachmentName.c_str());
            curl_mime_data(part, attachment->AttachmentContent.c_str(), attachment->AttachmentContent.size());
            curl_mime_type(part, attachment->AttachmentMimeType.c_str());
        }
    }
*/
    //curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    response = iso_8859_15_to_utf8(response);
    return response;
}
std::string MaventaAPI::getInvoiceImage(MaventaInvoice & inv) {
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

    std::ostringstream url;
    //url << "https://ax.maventa.com/v1/invoices/" << inv.getId() << "/attachments/" << attachment_id;
    url << "https://ax.maventa.com/v1/invoices/" << inv.getId() << "?return_format=ORIGINAL_OR_GENERATED_IMAGE";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    //LOG(INFO) << "Attachment response: " << response;
    return base64_encode(response);
}
std::string MaventaAPI::getInvoiceAttachment(MaventaInvoice & inv, std::string href) {
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

    std::ostringstream url;
    //href example "https://ax.maventa.com/v1/invoices/25189cf0-4b3c-4c1b-952d-35b162171042/files/8cadd442-0211-47f9-83a7-c025796581d8;
    url << href;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    //LOG(INFO) << "Attachment response: " << response;
    return base64_encode(response);
}
std::string MaventaAPI::getExtendedDetails(MaventaInvoice & inv) {
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

    std::ostringstream url;
    //url << "https://ax.maventa.com/v1/invoices/" << inv.getId() << "/attachments/" << attachment_id;
    url << "https://ax.maventa.com/v1/invoices/" << inv.getId() << "?return_format=EXTENDED_DETAILS";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    //LOG(INFO) << "Attachment response: " << response;
    return response;
}
std::string MaventaAPI::getInvoiceXml(MaventaInvoice& inv) {
    if (access_token.empty()) {
        LOG(ERROR) << "No access token available for invoice XML request.";
        return "";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG(ERROR) << "Failed to initialize CURL";
        return "";
    }

    std::ostringstream url;
    url << "https://ax.maventa.com/v1/invoices/" << inv.getId() << "?return_format=FINVOICE30";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(res);
        return "";
    }
    if(response.empty()) {
        LOG(ERROR) << "No response received for invoice XML request.";
        return "";
    }
    //hack
    if( response.find("iso_8859") != std::string::npos ||
        response.find("ISO_8859") != std::string::npos ||
        response.find("iso-8859") != std::string::npos ||
        response.find("ISO-8859") != std::string::npos 
    ) {
        return iso_8859_15_to_utf8(response);
    }
    return response;
}
std::string MaventaAPI::uploadInvoice(FinvoiceInvoice &invoice) {
    invoice.messageId = generateRandomMessageId();

    std::vector<FinvoiceAttachment> files;
    std::string xml = invoice.getXmlFinvoiceMessage(invoice.attachments);
    std::string soap = invoice.getXmlFinvoiceEnvelopeHeader();
    std::string content = soap + xml;

    //add all files to a zip file
    std::stringstream buffer;
    zipper::Zipper zipper(buffer);
    std::istringstream stream_content(content); 
    std::tm tm{};
    std::mktime(&tm);

    bool ok = zipper.add(stream_content, tm, "invoice.xml");
    if(!ok) {
        LOG(ERROR) << "Failed to add invoice.xml to zip";
        return std::string("-1");
    }

    for(size_t i=0; i<invoice.attachments.size(); i++) {
        auto attachment = invoice.attachments[i];
        std::string decoded = base64_decode(attachment.AttachmentContent);
        std::istringstream stream_attachment(decoded); 
        ok = zipper.add(stream_attachment, tm, attachment.AttachmentName);
        if(!ok) {
            LOG(ERROR) << "Failed to add " << attachment.AttachmentName << " to zip";
            return std::string("-1");
        }   
    }
    zipper.close();
    
    std::string zip_content = buffer.str();
    //WriteFileContent("/tmp/finvoice.zip", zip_content, true);
    
    std::string invresult = sendFile(zip_content, "finvoice.zip", "application/zip");
    rapidjson::Document resp;
    rapidjson::ParseResult iok = resp.Parse(invresult.c_str());     

    if (!resp.IsObject() || !resp.HasMember("id") || !resp["id"].IsString()) {
        LOG(ERROR) << "Invalid return from uploadInvoice: " << invresult;
        return std::string("-1");
    }
    std::string maventa_invoice_id = resp["id"].GetString();

    return maventa_invoice_id;
       
}
