#include <stddef.h>

#include "win.h"
#include "common.h"
#include "terms.h"

typedef void*           HANDLE;
typedef int             HFILE;
typedef const void*     LPCVOID;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef DWORD*          LPDWORD;
typedef int             BOOL;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef char            CHAR;
typedef const char*     LPCSTR;

#define OFS_MAXPATHNAME 128
#define HFILE_ERROR     -1

#define OF_READ         0x00000000
#define OF_WRITE        0x00000001

typedef struct _OFSTRUCT {
  BYTE cBytes;
  BYTE fFixedDisk;
  WORD nErrCode;
  WORD Reserved1;
  WORD Reserved2;
  CHAR szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT;

// overlapped structure is left as void* as we will not use it in any way as console output couldn't be async
extern BOOL   __stdcall WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, void* lpOverlapped);
extern BOOL   __stdcall ReadFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* lpOverlapped);
extern HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
extern HFILE  __stdcall OpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
extern BOOL   __stdcall CloseHandle(HANDLE hObject);

#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)

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
