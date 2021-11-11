#ifndef IO_H
#define IO_H

// todo: what about calling init_io and deinit_io from _start?

typedef void* TermiteHandle;

typedef enum {
  foFileRead,
  foFileWrite,
} FileOpenIntents;

TermiteHandle get_stdout(void);
TermiteHandle get_stdin(void);

// todo: make them return status?
void init_io(void);
void deinit_io(void);

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
