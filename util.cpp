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
#include "util.h"
#include <cstdarg>
#include <cstdio>       
#include <string>   
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

static constexpr char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static constexpr char reverse_table[128] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};
std::string base64_encode(const ::std::string &bindata){
    using ::std::string;
    using ::std::numeric_limits;

    if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
        throw ::std::length_error("Converting too large a string to base64.");
    }

    const ::std::size_t binlen = bindata.size();
    // Use = signs so the end is properly padded.
    string retval((((binlen + 2) / 3) * 4), '=');
    ::std::size_t outpos = 0;
    int bits_collected = 0;
    unsigned int accumulator = 0;
    const string::const_iterator binend = bindata.end();

    for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
        accumulator = (accumulator << 8) | (*i & 0xffu);
        bits_collected += 8;
        while (bits_collected >= 6) {
            bits_collected -= 6;
            retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
        }
    }
    if (bits_collected > 0) { // Any trailing bits that are missing.
        //assert(bits_collected < 6);
        accumulator <<= 6 - bits_collected;
        retval[outpos++] = b64_table[accumulator & 0x3fu];
    }
    //assert(outpos >= (retval.size() - 2));
    //assert(outpos <= retval.size());
    return retval;
}

std::string base64_decode(const ::std::string &ascdata) {
    using ::std::string;
    string retval;
    const string::const_iterator last = ascdata.end();
    int bits_collected = 0;
    unsigned int accumulator = 0;

    for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
        const int c = *i;
        if (::std::isspace(c) || c == '=') {
            // Skip whitespace and padding. Be liberal in what you accept.
            continue;
        }
        if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
            throw ::std::invalid_argument("This contains characters not legal in a base64 encoded string.");
        }
        accumulator = (accumulator << 6) | reverse_table[c];
        bits_collected += 6;
        if (bits_collected >= 8) {
            bits_collected -= 8;
            retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
        }
    }
    return retval;
}
std::string string_fmt_money(double value, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    std::string out= oss.str();
    string_replaceall(out, ".", ","); // Ensure comma as decimal separator
    return out;
}
std::string getTimestamp(std::string format) {
    //2025-08-26T12:53:54
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    if(format == "YYYYMMDD") {
        oss << std::put_time(std::localtime(&in_time_t), "%Y%m%d");
        return oss.str();
    }
    oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}
std::string getLastDigits(const std::string& str) {
    if (str.empty()) return str;
    size_t pos = str.size();
    while (pos > 0 && std::isdigit(str[pos - 1])) {
        --pos;
    }
    return str.substr(pos);
}
std::string generateRandomMessageId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    //if no input was provided, generate random
    std::ostringstream oss;
    // Generate 9 random digits 
    for (int i = 0; i < 9; ++i) {
        oss << dis(gen);
    }
    return  oss.str();
}
std::string generateRandomEpiRef(std::string input) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    std::string base = getLastDigits(input);
    if(base == "") {
        //if no input was provided, generate random
        std::ostringstream oss;
        // Generate 9 random digits (leave last for checksum)
        for (int i = 0; i < 9; ++i) {
            oss << dis(gen);
        }
        base = oss.str();
    }
    // Calculate checksum using your generateInvoiceChecksum function
    std::string checksum = generateInvoiceChecksum(base);

    // Append checksum
    std::string epiRef = base + checksum;
    return epiRef;
}
std::string generateInvoiceChecksum(const std::string& input) {
    int counter = 0;
    int totalSum = 0;
    int totalChars = input.size();

    for (int i = totalChars; i >= 1; i--) {
        int multiplier = 1;
        if (counter == 0) {
            multiplier = 7;
            counter++;
        } else if (counter == 1) {
            multiplier = 3;
            counter++;
        } else if (counter == 2) {
            multiplier = 1;
            counter = 0;
        }
        // Convert char to int (assuming input contains only digits)
        int digit = input[i - 1] - '0';
        totalSum += multiplier * digit;
    }
    // Take last digit from totalSum
    int lastDigit = totalSum % 10;
    int checkSum = 10 - lastDigit;
    if (checkSum == 10) checkSum = 0;
    return std::to_string(checkSum);
}
std::string formattedString(const char *format, ...)
{
    std::string strBuffer(128, 0);
    va_list ap, backup_ap;
    va_start(ap, format);
    va_copy(backup_ap, ap);
    auto result = vsnprintf((char *)strBuffer.data(),
                            strBuffer.size(),
                            format,
                            backup_ap);
    va_end(backup_ap);
    if ((result >= 0) && ((std::string::size_type)result < strBuffer.size()))
    {
        strBuffer.resize(result);
    }
    else
    {
        while (true)
        {
            if (result < 0)
            {
                // Older snprintf() behavior. Just try doubling the buffer size
                strBuffer.resize(strBuffer.size() * 2);
            }
            else
            {
                strBuffer.resize(result + 1);
            }

            va_copy(backup_ap, ap);
            auto result = vsnprintf((char *)strBuffer.data(),
                                    strBuffer.size(),
                                    format,
                                    backup_ap);
            va_end(backup_ap);

            if ((result >= 0) &&
                ((std::string::size_type)result < strBuffer.size()))
            {
                strBuffer.resize(result);
                break;
            }
        }
    }
    va_end(ap);
    return strBuffer;
}

std::string ReadFileContent(std::string filename) {
    std::string filecontent;
    FILE *file = fopen(filename.c_str(), "rb");
    if (file == NULL) {
        return filecontent;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    
    // allocate string space and set length
    filecontent.resize(size);

    // read 1*size bytes from sfile into ss
    fread(&filecontent[0], 1, size, file);

    // close the file
    fclose(file);
    return filecontent;
}
bool WriteFileContent(std::string filename, std::string &content, bool overwrite) {

    if(overwrite == false && filename!= "" && file_exists(filename)){
        return false;
    }
    FILE *file = stdout; 
    if(filename != "") 
        file = fopen(filename.c_str(), "wb");
    if (file == NULL) {
        return false;
    }

    size_t written = fwrite(content.c_str(), 1, content.length(), file);
    // close the file
    if(filename != "") 
        fclose(file);

    if(written != content.length())
        return false;

    return true;
}
bool file_exists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}
std::string timestamp_to_string(int ts_seconds, bool print_hours) {
    std::time_t in_t = ts_seconds;
    std::tm * in_lt = std::localtime(&in_t);
    char inb[64];
    if(print_hours) {
        std::strftime(inb, 64, "%d.%m.%Y %H:%M:%S", in_lt);
    }
    else {
        std::strftime(inb, 64, "%d.%m.%Y", in_lt);
    }
    return std::string(inb);
}


bool string_startswith(const char* haystack, size_t haystackSize, const char* needle, size_t needleSize) {
    if (haystackSize < needleSize) {
        return false;
    }
    return strncmp(haystack, needle, needleSize) == 0;
}

bool string_startswith(const std::string haystack, const std::string needle) {
    return string_startswith((const char*)haystack.c_str(), (size_t) haystack.size(), (const char*)needle.c_str(), (size_t)needle.size());
}
bool string_endswith(const std::string& haystack, const std::string& needle) {
    if (needle.size() > haystack.size())
        return false;
    else
        return (haystack.substr(haystack.size() - needle.size()) == needle);
}
std::string string_trim(const std::string& str, const std::string& whitespace){
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}
void string_replaceall(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}
std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}
const std::map<std::string, std::string> CountryNameToCode = {
    {"Afghanistan", "AF"},
    {"Aland Islands", "AX"},
    {"Albania", "AL"},
    {"Algeria", "DZ"},
    {"American Samoa", "AS"},
    {"Andorra", "AD"},
    {"Angola", "AO"},
    {"Anguilla", "AI"},
    {"Antarctica", "AQ"},
    {"Antigua and Barbuda", "AG"},
    {"Argentina", "AR"},
    {"Armenia", "AM"},
    {"Aruba", "AW"},
    {"Australia", "AU"},
    {"Austria", "AT"},
    {"Azerbaijan", "AZ"},
    {"Bahamas", "BS"},
    {"Bahrain", "BH"},
    {"Bangladesh", "BD"},
    {"Barbados", "BB"},
    {"Belarus", "BY"},
    {"Belgium", "BE"},
    {"Belize", "BZ"},
    {"Benin", "BJ"},
    {"Bermuda", "BM"},
    {"Bhutan", "BT"},
    {"Bolivia", "BO"},
    {"Bonaire, Sint Eustatius and Saba", "BQ"},
    {"Bosnia and Herzegovina", "BA"},
    {"Botswana", "BW"},
    {"Bouvet Island", "BV"},
    {"Brazil", "BR"},
    {"British Indian Ocean Territory", "IO"},
    {"Brunei Darussalam", "BN"},
    {"Bulgaria", "BG"},
    {"Burkina Faso", "BF"},
    {"Burundi", "BI"},
    {"Cambodia", "KH"},
    {"Cameroon", "CM"},
    {"Canada", "CA"},
    {"Cape Verde", "CV"},
    {"Cayman Islands", "KY"},
    {"Central African Republic", "CF"},
    {"Chad", "TD"},
    {"Chile", "CL"},
    {"China", "CN"},
    {"Christmas Island", "CX"},
    {"Cocos (Keeling) Islands", "CC"},
    {"Colombia", "CO"},
    {"Comoros", "KM"},
    {"Congo", "CG"},
    {"Congo, Democratic Republic of the", "CD"},
    {"Cook Islands", "CK"},
    {"Costa Rica", "CR"},
    {"Cote d'Ivoire", "CI"},
    {"Croatia", "HR"},
    {"Cuba", "CU"},
    {"Curacao", "CW"},
    {"Cyprus", "CY"},
    {"Czech Republic", "CZ"},
    {"Denmark", "DK"},
    {"Djibouti", "DJ"},
    {"Dominica", "DM"},
    {"Dominican Republic", "DO"},
    {"Ecuador", "EC"},
    {"Egypt", "EG"},
    {"El Salvador", "SV"},
    {"Equatorial Guinea", "GQ"},
    {"Eritrea", "ER"},
    {"Estonia", "EE"},
    {"Eswatini", "SZ"},
    {"Ethiopia", "ET"},
    {"Falkland Islands (Malvinas)", "FK"},
    {"Faroe Islands", "FO"},
    {"Fiji", "FJ"},
    {"Finland", "FI"},
    {"France", "FR"},
    {"French Guiana", "GF"},
    {"French Polynesia", "PF"},
    {"French Southern Territories", "TF"},
    {"Gabon", "GA"},
    {"Gambia", "GM"},
    {"Georgia", "GE"},
    {"Germany", "DE"},
    {"Ghana", "GH"},
    {"Gibraltar", "GI"},
    {"Greece", "GR"},
    {"Greenland", "GL"},
    {"Grenada", "GD"},
    {"Guadeloupe", "GP"},
    {"Guam", "GU"},
    {"Guatemala", "GT"},
    {"Guernsey", "GG"},
    {"Guinea", "GN"},
    {"Guinea-Bissau", "GW"},
    {"Guyana", "GY"},
    {"Haiti", "HT"},
    {"Heard Island and McDonald Islands", "HM"},
    {"Holy See (Vatican City State)", "VA"},
    {"Honduras", "HN"},
    {"Hong Kong", "HK"},
    {"Hungary", "HU"},
    {"Iceland", "IS"},
    {"India", "IN"},
    {"Indonesia", "ID"},
    {"Iran, Islamic Republic of", "IR"},
    {"Iraq", "IQ"},
    {"Ireland", "IE"},
    {"Isle of Man", "IM"},
    {"Israel", "IL"},
    {"Italy", "IT"},
    {"Jamaica", "JM"},
    {"Japan", "JP"},
    {"Jersey", "JE"},
    {"Jordan", "JO"},
    {"Kazakhstan", "KZ"},
    {"Kenya", "KE"},
    {"Kiribati", "KI"},
    {"Korea, Democratic People's Republic of", "KP"},
    {"Korea, Republic of", "KR"},
    {"Kuwait", "KW"},
    {"Kyrgyzstan", "KG"},
    {"Lao People's Democratic Republic", "LA"},
    {"Latvia", "LV"},
    {"Lebanon", "LB"},
    {"Lesotho", "LS"},
    {"Liberia", "LR"},
    {"Libya", "LY"},
    {"Liechtenstein", "LI"},
    {"Lithuania", "LT"},
    {"Luxembourg", "LU"},
    {"Macao", "MO"},
    {"Madagascar", "MG"},
    {"Malawi", "MW"},
    {"Malaysia", "MY"},
    {"Maldives", "MV"},
    {"Mali", "ML"},
    {"Malta", "MT"},
    {"Marshall Islands", "MH"},
    {"Martinique", "MQ"},
    {"Mauritania", "MR"},
    {"Mauritius", "MU"},
    {"Mayotte", "YT"},
    {"Mexico", "MX"},
    {"Micronesia, Federated States of", "FM"},
    {"Moldova, Republic of", "MD"},
    {"Monaco", "MC"},
    {"Mongolia", "MN"},
    {"Montenegro", "ME"},
    {"Montserrat", "MS"},
    {"Morocco", "MA"},
    {"Mozambique", "MZ"},
    {"Myanmar", "MM"},
    {"Namibia", "NA"},
    {"Nauru", "NR"},
    {"Nepal", "NP"},
    {"Netherlands", "NL"},
    {"New Caledonia", "NC"},
    {"New Zealand", "NZ"},
    {"Nicaragua", "NI"},
    {"Niger", "NE"},
    {"Nigeria", "NG"},
    {"Niue", "NU"},
    {"Norfolk Island", "NF"},
    {"North Macedonia", "MK"},
    {"Northern Mariana Islands", "MP"},
    {"Norway", "NO"},
    {"Oman", "OM"},
    {"Pakistan", "PK"},
    {"Palau", "PW"},
    {"Palestine, State of", "PS"},
    {"Panama", "PA"},
    {"Papua New Guinea", "PG"},
    {"Paraguay", "PY"},
    {"Peru", "PE"},
    {"Philippines", "PH"},
    {"Pitcairn", "PN"},
    {"Poland", "PL"},
    {"Portugal", "PT"},
    {"Puerto Rico", "PR"},
    {"Qatar", "QA"},
    {"Reunion", "RE"},
    {"Romania", "RO"},
    {"Russian Federation", "RU"},
    {"Rwanda", "RW"},
    {"Saint Barthelemy", "BL"},
    {"Saint Helena, Ascension and Tristan da Cunha", "SH"},
    {"Saint Kitts and Nevis", "KN"},
    {"Saint Lucia", "LC"},
    {"Saint Martin (French part)", "MF"},
    {"Saint Pierre and Miquelon", "PM"},
    {"Saint Vincent and the Grenadines", "VC"},
    {"Samoa", "WS"},
    {"San Marino", "SM"},
    {"Sao Tome and Principe", "ST"},
    {"Saudi Arabia", "SA"},
    {"Senegal", "SN"},
    {"Serbia", "RS"},
    {"Seychelles", "SC"},
    {"Sierra Leone", "SL"},
    {"Singapore", "SG"},
    {"Sint Maarten (Dutch part)", "SX"},
    {"Slovakia", "SK"},
    {"Slovenia", "SI"},
    {"Solomon Islands", "SB"},
    {"Somalia", "SO"},
    {"South Africa", "ZA"},
    {"South Georgia and the South Sandwich Islands", "GS"},
    {"South Sudan", "SS"},
    {"Spain", "ES"},
    {"Sri Lanka", "LK"},
    {"Sudan", "SD"},
    {"Suriname", "SR"},
    {"Svalbard and Jan Mayen", "SJ"},
    {"Sweden", "SE"},
    {"Switzerland", "CH"},
    {"Syrian Arab Republic", "SY"},
    {"Taiwan, Province of China", "TW"},
    {"Tajikistan", "TJ"},
    {"Tanzania, United Republic of", "TZ"},
    {"Thailand", "TH"},
    {"Timor-Leste", "TL"},
    {"Togo", "TG"},
    {"Tokelau", "TK"},
    {"Tonga", "TO"},
    {"Trinidad and Tobago", "TT"},
    {"Tunisia", "TN"},
    {"Turkey", "TR"},
    {"Turkmenistan", "TM"},
    {"Turks and Caicos Islands", "TC"},
    {"Tuvalu", "TV"},
    {"Uganda", "UG"},
    {"Ukraine", "UA"},
    {"United Arab Emirates", "AE"},
    {"United Kingdom", "GB"},
    {"United States", "US"},
    {"United States Minor Outlying Islands", "UM"},
    {"Uruguay", "UY"},
    {"Uzbekistan", "UZ"},
    {"Vanuatu", "VU"},
    {"Venezuela", "VE"},
    {"Viet Nam", "VN"},
    {"Virgin Islands, British", "VG"},
    {"Virgin Islands, U.S.", "VI"},
    {"Wallis and Futuna", "WF"},
    {"Western Sahara", "EH"},
    {"Yemen", "YE"},
    {"Zambia", "ZM"}
};
std::string getFirstTwoChars(const std::string& str) {
    if (str.size() < 2) return str;
    return str.substr(0, 2);
}

// Returns true if str starts with any of the given country codes (case-insensitive)
bool startsWithCountryCode(const std::string& str) {
    std::string firstTwoChars = getFirstTwoChars(str);
    for (const auto& kv : CountryNameToCode) {
        if (std::equal(kv.second.begin(), kv.second.end(), firstTwoChars.begin(), firstTwoChars.end(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            return true;
        }
    }
    return false;        
}

std::string getCountryCodeFromName(const std::string& name) {
    // Case-insensitive search
    for (const auto& kv : CountryNameToCode) {
        if (std::equal(kv.first.begin(), kv.first.end(), name.begin(), name.end(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            return kv.second;
        }
    }
    return "";
}
std::string getCountryNameForCode(const std::string& code) {
    for (const auto& kv : CountryNameToCode) {
        if (kv.second == code) {
            return kv.first;
        }
    }
    return "";
}