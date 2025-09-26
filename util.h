#pragma once

#include "config_app.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <chrono>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <vector>
#include <string>

#include <iostream>
#include <string>
#include <stdlib.h>

#include "logger.h"

std::string base64_encode(const ::std::string &bindata);
std::string base64_decode(const ::std::string &ascdata);
std::string formattedString(const char *format, ...);
std::string ReadFileContent(std::string filename);
bool file_exists (const std::string& name);
bool WriteFileContent(std::string filename, std::string &content, bool overwrite=false);
std::string timestamp_to_string(int ts_seconds, bool print_hours = false);
bool string_startswith(const char* haystack, size_t haystackSize, const char* needle, size_t needleSize);
bool string_startswith(const std::string haystack, const std::string needle);
bool string_endswith(const std::string& haystack, const std::string& needle);
std::string string_trim(const std::string& str, const std::string& whitespace);
std::string string_fmt_money(double value, int decimals=3);
void string_replaceall(std::string& source, const std::string& from, const std::string& to);
std::string escape_json(const std::string &s);
std::string getCountryCodeFromName(const std::string& name);
bool startsWithCountryCode(const std::string& str);
std::string getFirstTwoChars(const std::string& str);
std::string getCountryNameForCode(const std::string& code);
std::string generateRandomEpiRef(std::string input);
std::string generateRandomMessageId();
std::string generateInvoiceChecksum(const std::string& input);
std::string getTimestamp(std::string format="");