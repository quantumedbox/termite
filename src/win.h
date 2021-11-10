#ifndef WIN_H
#define WIN_H

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


typedef HANDLE TermiteHandle;

typedef enum {
  foFileRead,
  foFileWrite,
} FileOpenIntents;

TermiteHandle get_stdout(void);
TermiteHandle get_stdin(void);

// returns 0 on file opening error, 1 otherwise
_Bool
open_file(const char* path, TermiteHandle* result, FileOpenIntents intent);

// returns 0 on file opening error, 1 otherwise
_Bool
close_file(TermiteHandle file);

// returns 0 on error or if not all chars were written, otherwise 1
_Bool
write_file(TermiteHandle file, const char* msg, unsigned int len);

// returns 0 on read error, 1 otherwise
_Bool
read_file(TermiteHandle file,
          char* restrict buff,
          unsigned int limit,
          unsigned int* restrict read_result);

#endif
