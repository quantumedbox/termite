/*
  Termite compiler
  Produces NASM x86 assembly

  Relative jumps are accomplished by making all operations contained in separate "functions", thus making them equal sized
  Pushes require 1 byte argument, because of that all simple calls should have NOP after them for alignment

  On execution start current stack pointer is saved to know the beginning of termite stack
  It probably should be stored in global value so every operator function could retrieve it for checks
  Stack limit should also be calculated as offset from that original point
*/

#include "io.h"
#include "common.h"
#include "terms.h"

static const prelude[] =
  "\textern write_file\n"
  "\textern read_file\n"
  "\textern get_stdout\n"

  "section .data\n"
  "\tstack_base: dd\n"
  "\tstack_limit: dd\n";

static const op_add[] =
  "termite_add:"
  // todo: check stack_base for availability of values
  "\taddb ";

static const op_print[] =
  "termite_print:"
  // todo: check stack_base for availability of value, call get_stdout, then write_file
  "\t";

static int
read_input(const char* source_path, const char* output_path)
{
  char input[INPUT_LIMIT + 1U];
  unsigned int size;
  {
    TermiteHandle file;
    if (!open_file(source_path, &file, foFileRead))
      return OC_FILE_ERROR;

    if (!read_file(file, input, INPUT_LIMIT, &size)) {
      close_file(file);
      return OC_FILE_ERROR;
    }

    if (size == INPUT_LIMIT + 1U) {
      close_file(file);
      return OC_INPUT_OVERFLOW;
    }

    if (!close_file(file))
      return OC_FILE_ERROR;
  }

  TermiteHandle output;
  if (!open_file(output_path, &output, foFileWrite))
    return OC_FILE_ERROR;

  unsigned int op_written = 0U;

}

int
main(int argc, char** argv, char** envp)
{
  (void)envp;
  
  int return_code = read_input();
  return return_code;
}
