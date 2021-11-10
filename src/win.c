#include <stddef.h>

#include "win.h"
#include "common.h"
#include "terms.h"

TermiteHandle
get_stdin(void)
{
  return (TermiteHandle)GetStdHandle(STD_INPUT_HANDLE);
}

TermiteHandle
get_stdout(void)
{
  return (TermiteHandle)GetStdHandle(STD_OUTPUT_HANDLE);
}

_Bool
open_file(const char* path, TermiteHandle* result, FileOpenIntents intent)
{
  if (count_cstring(path) > FILEPATH_LIMIT)
    return (_Bool)0;

  OFSTRUCT file_struct;
  HFILE file;
  switch (intent) {
    case foFileRead:
      file = OpenFile(path, &file_struct, OF_READ);
      break;
    case foFileWrite:
      file = OpenFile(path, &file_struct, OF_WRITE);
      break;
  }
  (void)file_struct;

  if (file == HFILE_ERROR)
    return (_Bool)0;

  *result = (HANDLE)file; // HFILE corresponds to HANDLE when it's not HFILE_ERROR, ignore the warning
  return (_Bool)1;
}

_Bool
close_file(TermiteHandle file)
{
  BOOL status = CloseHandle((HANDLE)file);
  return status == (BOOL)0 ? (_Bool)0 : (_Bool)1;
}

_Bool
write_file(TermiteHandle file, const char* msg, unsigned int len)
{
  DWORD chars_written;
  BOOL status = WriteFile(file, msg, len, &chars_written, NULL);
  return status == (BOOL)0 || chars_written != len ? (_Bool)0 : (_Bool)1;
}

_Bool
read_file(TermiteHandle file,
          char* restrict buff,
          unsigned int limit,
          unsigned int* restrict read_result)
{
  DWORD chars_read;
  if (ReadFile((HANDLE)file, buff, limit, &chars_read, NULL) == (BOOL)0) {
    *read_result = 0U; // todo: is it necessary?
    return (_Bool)0;
  }
  *read_result = (unsigned int)chars_read;
  return (_Bool)1;
}
