#include <stddef.h>
#include <limits.h>

#include "io.h"
#include "common.h"

#define MAX_DECIMAL_CHARS_UINT  3 * sizeof(unsigned int) * CHAR_BIT / 8
#define MAX_DECIMAL_CHARS_INT   3 * sizeof(int) * CHAR_BIT / 8 + 1

// todo: write for signed integers

void
write_byte(TermiteHandle file, unsigned char ch)
{
  write_file(file, (const char*)&ch, 1U);
}

void
write_byte_array(TermiteHandle file, unsigned char* chars, unsigned int len)
{
  const char space = ' ';
  for (unsigned int i = 0U; i < len; i++) {
    write_byte(file, chars[i]);

    if (i != len - 1U)
      write_file(file, &space, 1U);
  }
}

void
write_cstring(TermiteHandle file, const char* str) {
  write_file(file, str, count_cstring(str));
}

void
write_uint(TermiteHandle file, unsigned int value) {
  char builder_buff[MAX_DECIMAL_CHARS_UINT]; // todo: how to get target's maximum character length of base10 representation of unsigned integer?
  unsigned int builder_idx = MAX_DECIMAL_CHARS_UINT;

  for (int reductor = value; reductor != 0;) {
    builder_buff[--builder_idx] = (reductor % 10) + 0x30;
    reductor /= 10; 
  }
  write_file(file, &builder_buff[builder_idx], MAX_DECIMAL_CHARS_UINT - builder_idx);
}

// todo: restrict might be dangerous in this case
_Bool
compare_byte_array(unsigned char* restrict first, unsigned int first_len,
                    unsigned char* restrict second, unsigned int second_len)
{
  if (first_len != second_len)
    return (_Bool)0;

  for (unsigned int i = 0U; i < first_len; i++) {
    if (*first != *second)
      return (_Bool)0;
  }
  return (_Bool)1;
}

// todo: restrict might be dangerous in this case
_Bool
compare_cstring(const char* restrict first, const char* restrict second)
{
  if (first == NULL || second == NULL)
    return (_Bool)0;

  while ((*first != '\0') && (*second != '\0')) {
    if (*first != *second)
      return (_Bool)0;
    first++;
    second++;
  }
  return (_Bool)1;
}

unsigned int
count_cstring(const char* str) {
  if (str == NULL)
    return 0U;

  unsigned int len = 0U;
  while (*str++ != '\0')
    len++;

  return len;
}
