#include <stddef.h>

#include "io.h"
#include "common.h"

void
print_value(unsigned char ch)
{
  write_file(get_stdout(), (const char*)&ch, 1U);
}

void
print_stack(unsigned char* chars, unsigned int len)
{
  const char space = ' ';
  for (unsigned int i = 0U; i < len; i++) {
    print_value(chars[i]);

    if (i != len - 1U)
      write_file(get_stdout(), &space, 1U);
  }
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

void
print_cstring(const char* str) {
  write_file(get_stdout(), str, count_cstring(str));
}

// todo: restrict might be dangerous in this case
_Bool
compare_value_array(unsigned char* restrict first, unsigned int first_len,
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
