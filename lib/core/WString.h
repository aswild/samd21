// WString.h: Arduino string library, but actually just use std::string.
// Generally the standard library is better optimized (with all its template magic)
// and we don't need AVR program memory helpers on ARM.

#ifndef WSTRING_H
#define WSTRING_H

#include <string>
#include <cstring>

using String = std::string;
using __FlashStringHelper = const char*;

#endif
