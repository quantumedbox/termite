#ifndef COMMON_H
#define COMMON_H

#include "io.h"

// todo: return success of writing
// todo: integer printing

void
write_byte(TermiteHandle, unsigned char);

void
write_byte_array(TermiteHandle, unsigned char*, unsigned int len);

void
write_cstring(TermiteHandle, const char*);

void
write_uint(TermiteHandle, unsigned int);

_Bool
compare_byte_array(unsigned char* restrict first, unsigned int first_len,
                    unsigned char* restrict second, unsigned int second_len);

_Bool
compare_cstring(const char* restrict first, const char* restrict second);

unsigned int
count_cstring(const char* str);

#endif
