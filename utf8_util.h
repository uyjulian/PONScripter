#ifndef __UTF8_UTIL__
#define __UTF8_UTIL__

#include "stdio.h"

const int Default = 0;
const int Italic  = 1;
const int Bold    = 2;
const int Sans    = 4;
const int Altern  = 8;

char
CharacterBytes(const char* string);

unsigned short
UnicodeOfUTF8(const char* string);

int
UTF8OfUnicode(const unsigned short ch, char* out);

const char*
PreviousCharacter(const char* string);

unsigned long int
UTF8Length(const char* string);

unsigned short
get_encoded_char(const int encoding, const unsigned short original);

void SetEncoding(int& encoding, const char flag);

#endif
