#include "common.h"

typedef char* LPSTR; // string of bytes

_Noreturn extern void __stdcall ExitProcess(unsigned long exit_code);
extern char* __stdcall GetCommandLineA(void);
extern int  __cdecl term_main(int argc, const char** argv);

#ifdef __GNUC__
// gcc compains about it for reasons
void __main(void) {}
#endif

#if defined(__amd64__)
__asm__(
    ".global ___chkstk_ms\n"
    "___chkstk_ms:pushq %rcx\n"
    "pushq %rax\n"
    "cmpq $0x1000, %rax\n"
    "leaq 24(%rsp), %rcx\n"
    "jb 2f\n"
    "1:subq $0x1000, %rcx\n"
    "orl $0, (%rcx)\n"
    "subq $0x1000, %rax\n"
    "cmpq $0x1000, %rax\n"
    "ja 1b\n"
    "2:subq %rax, %rcx\n"
    "orl $0, (%rcx)\n"
    "popq %rax\n"
    "popq %rcx\n"
    "ret\n"
);
#elif defined(__i386__)
__asm__(
    ".global ___chkstk_ms\n"
    "___chkstk_ms:pushl %ecx\n"
    "pushl %eax\n"
    "cmpl $0x1000, %eax\n"
    "leal 12(%esp), %ecx\n"
    "jb 2f\n"
    "1:subl $0x1000, %ecx\n"
    "orl $0, (%ecx)\n"
    "subl $0x1000, %eax\n"
    "cmpl $0x1000, %eax\n"
    "ja 1b\n"
    "2:subl %eax, %ecx\n"
    "orl $0, (%ecx)\n"
    "popl %eax\n"
    "popl %ecx\n"
    "ret\n"
);
#endif

// parsing reference: https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-160
static int
count_args(const char* command_line)
{
  int result = 0;

  _Bool is_quoted = (_Bool)0;
  _Bool is_counting = (_Bool)0;

  for (const char* ptr = command_line; *ptr;) {
    char ch = *ptr++;
    if (ch == ' ' || ch == '\t') {
      if (!is_quoted) {
        result++;
        is_counting = (_Bool)0;
      }
    } else if (ch == '"') {
      is_quoted = is_quoted? (_Bool)0 : (_Bool)1;
    } else if ((ch == '\\') && (*(ptr + 1) == '"')) {
      ptr++;
      is_counting = (_Bool)1;
    } else {
      is_counting = (_Bool)1;
    }
  }
  if (is_counting)
    result++;

  return result;
}

typedef struct {
  char** ptrs;
} ArgArray;

static void
parse_args(char* command_line, ArgArray* argv)
{
  // inserts '\0' in place of spaces and tabs to get individual NULL terminated args
  // populates 'argv' parameter with pointers to formed arguments

  char** cur_arg = argv->ptrs;
  _Bool is_quoted = (_Bool)0;
  _Bool is_counting = (_Bool)0;

  for (char* ptr = command_line; *ptr;) {
    char ch = *ptr;
    if (ch == ' ' || ch == '\t') {
      if (!is_quoted) {
        *ptr = '\0';
        is_counting = (_Bool)0;
      }
    } else if (ch == '"') {
      is_quoted = is_quoted? (_Bool)0 : (_Bool)1;
    } else if ((ch == '\\') && (*(ptr + 1) == '"')) {
      ptr++;
      if (!is_counting) {
        *cur_arg++ = ptr;
        is_counting = (_Bool)1;
      }
    } else {
      if (!is_counting) {
        *cur_arg++ = ptr;
        is_counting = (_Bool)1;
      }
    }
    ptr++;
  }
}

_Noreturn extern void
_start(void)
{
  LPSTR szCmd = GetCommandLineA();
  int argc = count_args((const char*)szCmd);
  char* argv[argc];
  ArgArray args = {
    .ptrs = argv
  };
  parse_args(szCmd, &args);
  ExitProcess(term_main(argc, (const char**)argv));
}
