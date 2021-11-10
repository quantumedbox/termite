/*
  Termite interpreter

  // todo: description + explanation of certain design choices
*/

#include "os.h"
#include "common.h"
#include "terms.h"

// todo: catch infinitely conveyoring loops
// todo: clear STDIN after execution
// todo: do not include sequential pushes in debug stack output
// todo: make inserted newline on stack printing consistent
// todo: show some debugging hints about jumps

static int
read_input(const char* filepath,
           _Bool print_stack_steps,
           _Bool print_stack_on_exit,
           _Bool catch_infinite_recursion)
{
  int exit_code = 0;

  char input[INPUT_LIMIT + 1U];
  unsigned int size;
  {
    TermiteHandle file;
    if (!open_file(filepath, &file, foFileRead))
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

  unsigned int  cursor = 0U;

  unsigned char stack[STACK_LIMIT];
  unsigned int  stack_head = 0U;

  // EXPERIMENTAL: required for checking of infinite loops on rewinds
  // todo: make it compile-time optional?
  // todo: could be dangerous to just mul to 0
  unsigned char shadow_stack[STACK_LIMIT * catch_infinite_recursion];
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
        if (print_stack_steps)
          print_cstring("\n");
        print_value(stack[stack_head - 1U]);
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
        if (!read_file(get_stdin(), &stdin_char, 1U, &chars_read)) // todo: we could probably retrieve STDIN only once per startup
          crash(OC_FILE_ERROR);

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

        if (catch_infinite_recursion) {
          if (shadow_stack_rewinded_with != 0U) {
            if (compare_value_array(shadow_stack, shadow_stack_len, stack, stack_head)) {
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
              if (input[cursor] & 0x80U)
                crash(OC_INVALID_INPUT);

              if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                if (cursor == 0U)
                  crash(OC_INVALID_INPUT);

                cursor--;

                if (input[cursor] & 0x80)
                  crash(OC_INVALID_INPUT);

                if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                  n_tokens--;
                } else {
                  crash(OC_INVALID_INPUT);
                }
              } else {
                n_tokens--;
              }
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
              if (input[cursor] & 0x80)
                crash(OC_INVALID_INPUT);

              if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                if (cursor == size)
                  crash(OC_INVALID_INPUT);

                cursor++;

                if (input[cursor] & 0x80)
                  crash(OC_INVALID_INPUT);

                if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                  n_tokens--;
                } else {
                  crash(OC_INVALID_INPUT);
                }
              } else {
                n_tokens--;
              }
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
        if (input[cursor] & 0x80)
          crash(OC_INVALID_INPUT);

        if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
          unsigned char leading = input[cursor] - '0';
          if (leading > 9U)
            leading -= 7U;

          if (cursor == size)
            crash(OC_INVALID_INPUT);
          if (input[cursor] & 0x80)
            crash(OC_INVALID_INPUT);

          cursor++;

          if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
            unsigned char following = input[cursor] - '0';
            if (following > 9U)
              following -= 7U;

            cursor++;
            stack[stack_head] = (leading << 4U) | following;
            op_char = stack[stack_head];
            stack_head++;

          } else
            crash(OC_INVALID_INPUT);

        } else {
          stack[stack_head] = (unsigned char)input[cursor++];
          op_char = stack[stack_head];
          stack_head++;
        }
      }
    }
    if (print_stack_steps == (_Bool)1) {
      print_cstring("\n|");
      print_stack(stack, stack_head);
      print_cstring("|");
      print_cstring(" ");
      write_file(get_stdout(), (const char*)&op_char, 1U);
    }
  }

EXIT_LOOP:
  if (print_stack_on_exit == (_Bool)1 || print_stack_steps == (_Bool)1) {
    print_cstring("\n|");
    print_stack(stack, stack_head);
    print_cstring("|");
  }
  return exit_code;
}

int
main(int argc, char** argv, char** envp)
{
  (void)envp;

  if (argc == 1)
    return OC_FILE_ERROR; //no file given

  const char* filepath = argv[1];
  _Bool print_stack_steps = (_Bool)0;
  _Bool print_stack_on_exit = (_Bool)0;
  _Bool catch_infinite_recursion = (_Bool)0;

  for (int i = 2; i < argc; i++) {
    // turn all debug switches
    if (compare_cstring(argv[i], "d")) {
      print_stack_steps = (_Bool)1;
      catch_infinite_recursion = (_Bool)1;
      // print_stack_on_exit = (_Bool)1;

    // turn on stack step printing
    } else if (compare_cstring(argv[i], "s")) {
      print_stack_steps = (_Bool)1;

    // show state of stack on program exit
    } else if (compare_cstring(argv[i], "e")) {
      print_stack_on_exit = (_Bool)1;

    // EXPERIMENTAL: try to catch rewinds that don't change state of stack
    // and thus are most likely infinitely looped
    } else if (compare_cstring(argv[i], "l")) {
      catch_infinite_recursion = (_Bool)1;
    }
  }

  int return_code =
    read_input(
      filepath,
      print_stack_steps,
      print_stack_on_exit,
      catch_infinite_recursion
    );

  // close_file(get_stdin());
  return return_code;
}
