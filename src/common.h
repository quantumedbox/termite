#ifndef COMMON_H
#define COMMON_H

void
print_value(unsigned char ch);

void
print_stack(unsigned char* chars, unsigned int len);

unsigned int
count_cstring(const char* str);

void
print_cstring(const char* str);

_Bool
compare_value_array(unsigned char* restrict first, unsigned int first_len,
                    unsigned char* restrict second, unsigned int second_len);

_Bool
compare_cstring(const char* restrict first, const char* restrict second);

#endif
