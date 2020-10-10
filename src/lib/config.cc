/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * config.cc
 */

#include <ctype.h>

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

}  // namespace dicom