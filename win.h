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
