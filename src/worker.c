/*
  Termite interpreter

  You can compile this file with TERM_NO_WORKER_MAIN to get embeddable no main version
    then you can just call read_input directly without worrying about main

  // todo: description + explanation of certain design choices
*/

#include "io.h"
#include "common.h"
#include "terms.h"

// todo: catch infinitely conveyoring loops
// todo: do not include sequential pushes in debug stack output

typedef struct {
  _Bool print_stack_steps;
  _Bool print_stack_on_exit;
  _Bool catch_infinite_recursion;
} WorkerArgs;

// todo: move these funcs to common.h?
static _Bool
is_hex_char(char ch)
{
 return (ch >= 'A' && ch <= 'F') || (ch >= '0' && ch <= '9') ? (_Bool)1 : (_Bool)0;
}

// todo: signal reasoning behind failure? for example non ascii chars
static _Bool
parse_hex(char* input_low, char* input_high, unsigned int pos)
{
  if ((input_low + pos + 1U <= input_high) &&
      ((input_low[pos] >= 'A' && input_low[pos] <= 'F') ||
        (input_low[pos] >= '0' && input_low[pos] <= '9')) &&
      ((input_low[pos + 1U] >= 'A' && input_low[pos + 1U] <= 'F') ||
        (input_low[pos + 1U] >= '0' && input_low[pos + 1U] <= '9'))) {
    return (_Bool)1;
  }
  return (_Bool)0;
}

// todo: should it be 0 based?
static unsigned int
count_tokens(char* input_low, char* input_high, unsigned int pos)
{
  unsigned int result = 0U;
  unsigned int cur = pos;

  if (input_low + pos > input_high)
    return 0U;

  while (1) {
    switch (input_low[cur]) {
      case  ' ':
      case '\n':
      case '\r':
      case '\t': break;
      default: {
        if (is_hex_char(input_low[cur])) {
          if ((cur != 0U) && parse_hex(input_low, input_high, cur - 1U))
            cur--;
          // else // todo: signal error
        }
        result++;
      }
    }
    if (cur == 0U)
      break;
    cur--;
  }
  return result;
}

static int
read_input(TermiteHandle input_handle,
           TermiteHandle out_handle,
           TermiteHandle in_handle,
           WorkerArgs args)
{
  int exit_code = 0;

  char input[INPUT_LIMIT + 1U]; // todo: initialize it with zeroes?
  unsigned int size;
  unsigned int cursor = 0U;

  if (!read_file(input_handle, input, INPUT_LIMIT, &size)) {
    return OC_FILE_ERROR;
  }

  if (size == INPUT_LIMIT + 1U) {
    return OC_INPUT_OVERFLOW;
  }

  unsigned char stack[STACK_LIMIT];
  unsigned int stack_head = 0U;

  // EXPERIMENTAL: required for checking of infinite loops on rewinds
  // todo: make it compile-time optional?
  // todo: could be dangerous to just mul to 0
  unsigned char shadow_stack[STACK_LIMIT * args.catch_infinite_recursion];
  unsigned char shadow_stack_rewinded_with = 0U;
  unsigned int  shadow_stack_len = 0U;

  #define crash(code) \
    do { \
      exit_code = code; \
      goto EXIT_LOOP; \
    } while (0)

  char op_char = '\0';

  while (1) {
    if (cursor == size)
      break;

    switch (input[cursor]) {
      case  ' ':
      case '\n':
      case '\r':
      case '\t': cursor++; continue;

      // pop value from stack and return it as exit code
      // this effectively terminates the program in predictable manner
      case '%': {
        op_char = '%';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);

        stack_head--;
        crash(stack[stack_head]);
        break;
      }

      // drop value from stack
      case '.': {
        op_char = '.';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);
        stack_head--;
        cursor++;
        break;
      }

      // duplicate last value on stack
      case '@': {
        op_char = '@';
        if (stack_head == STACK_LIMIT)
          crash(OC_STACK_OVERFLOW);
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head] = stack[stack_head - 1U];
        stack_head++;
        cursor++;
        break;
      }

      // swap two last values on stack
      case '^': {
        op_char = '^';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        unsigned char buff = stack[stack_head - 1U];
        stack[stack_head - 1U] = stack[stack_head - 2U];
        stack[stack_head - 2U] = buff;
        cursor++;
        break;
      }

      // 'conveyor belt' operator
      // place last value on the stack at the beginning
      case '#': {
        op_char = '#';
        if (stack_head == 0U) {
          cursor++;
          break;
        }
        unsigned char buff = stack[stack_head - 1U];
        for (unsigned int i = 0U; i < stack_head; i++) {
          unsigned char convey = stack[i];
          stack[i] = buff;
          buff = convey;
        }
        cursor++;
        break;
      }

      // 'ronveyor belt' operator aka 'reverse conveyor'
      // place first value on the stack at the end
      case '$': {
        op_char = '$';
        if (stack_head == 0U) {
          cursor++;
          break;
        }
        unsigned char buff = stack[0U];
        for (unsigned int i = stack_head; i--;) {
          unsigned char convey = stack[i];
          stack[i] = buff;
          buff = convey;
        }
        cursor++;
        break;
      }

      // not operator, toggles least significant bit
      // todo: replace with proper bitwise operators?
      case '~': {
        op_char = '~';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 1U] ^= 1U;
        cursor++;
        break;
      }

      // compare two stack values, consume them and push 1 or 0 depending on whether they're equal
      case '=': {
        op_char = '=';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 2U] = stack[stack_head - 2U] == stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // compare two stack values, consume them and push 1 or 0 depending on how they compare
      // if last is bigger than next then 0, otherwise 1
      case '?': {
        op_char = '?';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 2U] = stack[stack_head - 2U] < stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // add two stack values, consume them and push result of addition 
      case '+': {
        op_char = '+';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 2U] = stack[stack_head - 2U] + stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // subtract two stack values, consume them and push result of subtraction
      // pops subtractor first, then subtrahend
      case '-': {
        op_char = '-';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 2U] = stack[stack_head - 2U] - stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // multiply two stack values, consume them and push result of multiplication 
      case '*': {
        op_char = '*';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        stack[stack_head - 2U] = stack[stack_head - 2U] * stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // divide two stack values, consume them and push result of division
      // pops divider first, then dividend
      case '/': {
        op_char = '/';
        if (stack_head < 2U)
          crash(OC_STACK_EXHAUSTED);
        if (stack[stack_head - 1U] == 0U) {
          crash(OC_ZERO_DIVISION);
          break;
        }
        stack[stack_head - 2U] = stack[stack_head - 2U] / stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // pop from stack and print
      case '<': {
        op_char = '<';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);
        if (args.print_stack_steps)
          write_cstring(out_handle, "\n");
        write_byte(out_handle, stack[stack_head - 1U]);
        stack_head--;
        cursor++;
        break;
      }

      // push single byte from stdin into stack
      case '>': {
        op_char = '>';
        if (stack_head == STACK_LIMIT - 1U)
          crash(OC_STACK_OVERFLOW);

        char stdin_char;
        unsigned int chars_read;
        if (!read_file(in_handle, &stdin_char, 1U, &chars_read)) // todo: we could probably retrieve STDIN only once per startup
          crash(OC_FILE_ERROR); // todo: could be triggered when there's no input, should give INPUT_EXHAUTED error on such cases

        if (chars_read != 0U) {
          stack[stack_head++] = (unsigned char)stdin_char;
          stack[stack_head++] = 1U;
        } else {
          stack[stack_head++] = 0U; // todo: what about outputting random value here?
          stack[stack_head++] = 0U;
        }

        cursor++;
        break;
      }

      // pop from stack and rewind N tokens back
      case '[': {
        op_char = '[';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);

        unsigned char n_tokens = stack[stack_head - 1U];
        stack_head--;

        if (args.catch_infinite_recursion) {
          if (shadow_stack_rewinded_with != 0U) {
            if (compare_byte_array(shadow_stack, shadow_stack_len, stack, stack_head)) {
                crash(OC_INFINITE_LOOP);
              }
          }
          shadow_stack_rewinded_with = n_tokens;
          shadow_stack_len = stack_head;
          for (unsigned int i = stack_head; i--;)
            shadow_stack[i] = stack[i];
        }

        if (n_tokens != 0U)
          // '[' itself should not count
          cursor--;
        else
          // move along
          cursor++;

        while (n_tokens != 0U) {
          switch (input[cursor]) {
            case  ' ':
            case '\n':
            case '\r':
            case '\t': break;
            default: {
              if (is_hex_char(input[cursor])) {
                if ((cursor != 0U) && parse_hex(input, &input[size - 1U], cursor - 1U))
                  cursor--;
                else
                  crash(OC_INVALID_INPUT);
              }
              n_tokens--;
            }
          }

          if ((cursor == 0U) && (n_tokens != 0U))
            crash(OC_INPUT_EXHAUSTED);
          else if (n_tokens != 0U)
            cursor--;
        }
        break;
      }

      // pop from stack and seek N tokens forward
      case ']': {
        op_char = ']';
        if (stack_head == 0U)
          crash(OC_STACK_EXHAUSTED);

        unsigned char n_tokens = stack[stack_head - 1U];
        stack_head--;
        cursor++;

        while (n_tokens != 0U) {
          if (cursor == size)
            crash(OC_INPUT_EXHAUSTED);

          switch (input[cursor]) {
            case  ' ':
            case '\n':
            case '\r':
            case '\t': break;
            default: {
              if (is_hex_char(input[cursor])) {
                if ((cursor != (size - 1U)) && parse_hex(input, &input[size - 1U], cursor))
                  cursor++;
                else
                  crash(OC_INVALID_INPUT);
              }
              n_tokens--;
            }
          }

          if ((cursor == size) && (n_tokens != 0U))
            crash(OC_INPUT_EXHAUSTED);
          cursor++;
        }
        break;
      }

      // otherwise push it as character or hex value
      default: {
        if (stack_head == STACK_LIMIT)
          crash(OC_STACK_OVERFLOW);

        if (is_hex_char(input[cursor])) {
          if ((cursor != (size - 1U)) && parse_hex(input, &input[size - 1U], cursor)) {
            unsigned char leading = input[cursor] - '0';
            if (leading > 9U)
              leading -= 7U;

            unsigned char following = input[cursor + 1U] - '0';
            if (following > 9U)
              following -= 7U;

            cursor += 2U;
            stack[stack_head] = (leading << 4U) | following;
          } else
            crash(OC_INVALID_INPUT);

        } else
          stack[stack_head] = (unsigned char)input[cursor++];

        op_char = stack[stack_head];
        stack_head++;
      }
    }
    if (args.print_stack_steps == (_Bool)1) {
      write_cstring(out_handle, "\n|");
      write_byte_array(out_handle, stack, stack_head);
      write_cstring(out_handle, "| (");
      write_file(get_stdout(), (const char*)&op_char, 1U);
      write_cstring(out_handle, " ");
      write_uint(out_handle, count_tokens(input, &input[size - 1U], cursor - 1U));
      write_cstring(out_handle, ")");
    }
  }

EXIT_LOOP:
  if (args.print_stack_on_exit == (_Bool)1 || args.print_stack_steps == (_Bool)1) {
    write_cstring(out_handle, "\n|");
    write_byte_array(out_handle, stack, stack_head);
    write_cstring(out_handle, "|");
  }
  return exit_code;
}

#ifndef TERM_NO_WORKER_MAIN
int
term_main(int argc, const char** argv)
{
  if (argc == 1)
    return OC_FILE_ERROR; //no file given

  WorkerArgs args = {0};

  for (int i = 2; i < argc; i++) {
    // turn all debug switches
    if (compare_cstring(argv[i], "d")) {
      args.print_stack_steps = (_Bool)1;
      args.catch_infinite_recursion = (_Bool)1;
      args.print_stack_on_exit = (_Bool)1;

    // turn on stack step printing
    } else if (compare_cstring(argv[i], "s")) {
      args.print_stack_steps = (_Bool)1;

    // show state of stack on program exit
    } else if (compare_cstring(argv[i], "e")) {
      args.print_stack_on_exit = (_Bool)1;

    // EXPERIMENTAL: try to catch rewinds that don't change state of stack
    // and thus are most likely infinitely looped
    } else if (compare_cstring(argv[i], "l")) {
      args.catch_infinite_recursion = (_Bool)1;
    }
  }

  init_io();

  if (argc < 2)
    return OC_INVALID_INPUT;

  TermiteHandle input_file;
  if (!open_file(argv[1], &input_file, foFileRead))
    return OC_FILE_ERROR;

  int return_code =
    read_input(
      input_file,
      get_stdout(),
      get_stdin(),
      args
    );

  if (!close_file(input_file))
    return OC_FILE_ERROR;

  deinit_io();

  return return_code;
}
#endif
