#ifndef COMMON_H
#define COMMON_H

#include "io.h"

// todo: return success of writing

// todo: rename from 'value' to 'byte'
void
write_byte(TermiteHandle, unsigned char ch);

void
write_stack(TermiteHandle, unsigned char* chars, unsigned int len);

void
write_cstring(TermiteHandle, const char* str);

unsigned int
count_cstring(const char* str);

_Bool
compare_value_array(unsigned char* restrict first, unsigned int first_len,
                    unsigned char* restrict second, unsigned int second_len);

_Bool
compare_cstring(const char* restrict first, const char* restrict second);

#endif
