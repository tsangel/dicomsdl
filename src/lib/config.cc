/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * config.cc
 */

#include <ctype.h>
#include <stdlib.h>

#include "dicom.h"

namespace dicom {

const char* Config::_get(const char *key, const char *default_value)
{
    std::string k = key;
    for (auto & c: k) c = toupper(c);

    auto it = dict_.find(k);
    if (it != dict_.end())
        return it->second.c_str();
    else
        return default_value;
}

void Config::_set(const char *key, const char *value)
{
    std::string k = key;
    for (auto & c: k) c = toupper(c);

    std::string v = value;
    for (auto & c: v) c = toupper(c);

    dict_.erase(k);
    dict_[k] = v;
}

long Config::_getInteger(const char *key, long default_value) {
    std::string k = key;
    for (auto & c: k) c = toupper(c);

    auto it = dict_.find(k);
    if (it != dict_.end())
    {
        const char *str = it->second.c_str();
        char *endptr;
        long value;
        // return it->second.c_str();
        value = strtol(str, &endptr, 10);
        if (str != endptr)
          return value;
        else
          return default_value;  // no number in string
    } else
      return default_value;

    return default_value;
}
void Config::_setInteger(const char *key, long value) {
    std::string k = key;
    for (auto & c: k) c = toupper(c);

    char tmp[32];
    snprintf(tmp, 32, "%ld", value);

    dict_.erase(k);
    dict_[k] = std::string(tmp);
}

}  // namespace dicom