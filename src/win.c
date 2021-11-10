#include <stddef.h>

#include "win.h"
#include "common.h"

void
print(const char* msg, unsigned int len)
{
  DWORD chars_written;
  (void)WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, len, &chars_written, NULL);
  (void)chars_written;
}

// returns 0 on file opening error, 1 otherwise
_Bool
open_file(const char* path, TermiteHandle* result)
{
  if (count_cstring(path) > 128U)
    return (_Bool)0;

  OFSTRUCT file_struct;
  HFILE file = OpenFile(path, &file_struct, OF_READ);
  if (file == HFILE_ERROR)
    return (_Bool)0;

  *result = (HANDLE)file; // HFILE corresponds to HANDLE when it's not HFILE_ERROR, ignore the warning
  return (_Bool)1;
}

// returns 0 on file opening error, 1 otherwise
_Bool
close_file(TermiteHandle file)
{
  BOOL status = CloseHandle((HANDLE)file);
  return status == (BOOL)0 ? (_Bool)0 : (_Bool)1;
}

// returns 0 on read error, 1 otherwise
_Bool
read_file(TermiteHandle hFile,
          char* restrict buff,
          unsigned int limit,
          unsigned int* restrict read_result)
{
  DWORD chars_read;
  if (ReadFile((HANDLE)hFile, buff, limit, &chars_read, NULL) == (BOOL)0) {
    *read_result = 0U; // todo: is it necessary?
    return (_Bool)0;
  }
  *read_result = (unsigned int)chars_read;
  return (_Bool)1;
}
