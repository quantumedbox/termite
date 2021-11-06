#include "nocrt0/nocrt0c.c"

// todo: make all strings null terminated?
// todo: hex printing switch

typedef void*         HANDLE;
typedef const void*   LPCVOID;
typedef unsigned long DWORD;
typedef DWORD*        LPDWORD;
typedef int           BOOL;

// overlapped structure is left as void* as we will not use it in any way as console output couldn't be async
extern BOOL   __stdcall WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, void* lpOverlapped);
extern BOOL   __stdcall ReadFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* lpOverlapped);
extern HANDLE __stdcall GetStdHandle(DWORD nStdHandle);

#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)

#define INPUT_LIMIT 66560U // 65k
#define STACK_LIMIT 66560U // 65k

#define unpack_str(str) str, (sizeof(str) - 1ULL)

// todo: follow minimalism even in error reporting
static const char input_overflow[]    = "\n!input limit exceeded";
static const char stack_overflow[]    = "\n!stack limit exceeded";
static const char input_exhausted[]   = "\n!input exhausted";
static const char stack_exhausted[]   = "\n!stack exhausted";
static const char non_ascii_char[]    = "\n!on ASCII char encountered";
static const char invalid_hex_value[] = "\n!ill-formed hex value";

static _Bool print_as_hex = (_Bool)0; // todo: i don't want it in global scope, should be on stack

static _Bool cstring_compare(const char* first, const char* second) {
  while ((*first != '\0') && (*second != '\0')) {
    if (*first != *second)
      return (_Bool)0;
    first++;
    second++;
  }
  return (_Bool)1;
}

static void print(const char* msg, unsigned int len) {
  DWORD chars_written;
  (void)WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, len, &chars_written, NULL);
  (void)chars_written;
}

static void
print_value_custom(unsigned char ch,
                   _Bool as_hex)
{
  if (!as_hex) {
    print((const char*)&ch, 1U);
  } else {
    char values[2] = {(ch & 0xF0) >> 4, ch & 0x0F};
    values[0] += values[0] > 0x9 ? 55 : 48;
    values[1] += values[1] > 0x9 ? 55 : 48;
    print((const char*)values, sizeof(values));
  }
}

static void
print_value(unsigned char ch)
{
  print_value_custom(ch, print_as_hex);
}

static void
print_stack(unsigned char* chars,
            unsigned int len)
{
  const char space = ' ';
  for (unsigned int i = 0U; i < len; i++) {
    if (chars[i] < 0x20)
      print_value_custom(chars[i], (_Bool)1);
    else
      print_value_custom(chars[i], (_Bool)0);

    if (i != len - 1U)
      print(&space, 1U);
  }
}

#define put(msg) \
  do { \
    const char msg_impl[] = msg; \
    print(unpack_str(msg_impl)); \
  } while (0)

static _Noreturn void crash(const char* msg, unsigned int len) {
  print(msg, len);
  ExitProcess(1L);
}

static void read(char* msg, unsigned int limit, unsigned int* read) {
  DWORD chars_read;
  if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), msg, limit, &chars_read, NULL) == (BOOL)0) {
    *read = 0U;
  } else {
    *read = (unsigned int)chars_read;
  }
}

static _Bool
value_array_compare(unsigned char* restrict first, unsigned int first_len,
                    unsigned char* restrict second, unsigned int second_len)
{
  if (first_len != second_len)
    return (_Bool)0;
  for (unsigned int i = 0U; i < first_len; i++) {
    if (*first != *second)
      return (_Bool)0;
  }
  return (_Bool)1;
}

static void
read_input(_Bool print_stack_steps,
           _Bool catch_infinite_recursion)
{
  char input[INPUT_LIMIT + 1U];
  unsigned int size;
  read(input, INPUT_LIMIT, &size);
  if (size == INPUT_LIMIT + 1U) {
    crash(unpack_str(input_overflow));
  }

  unsigned char stack[STACK_LIMIT];
  unsigned int stack_head = 0U;

  // EXPERIMENTAL: required for checking of infinite loops on rewinds
  // todo: make it compile-time optional?
  // todo: could be dangerous
  unsigned char shadow_stack[STACK_LIMIT * catch_infinite_recursion];
  unsigned char shadow_stack_rewinded_with = 0U;
  unsigned int shadow_stack_len = 0U;

  unsigned int cursor = 0U;

  while (1) {
    if (cursor == size)
      break;

    switch (input[cursor]) {
      case  ' ':
      case '\n':
      case '\r':
      case '\t': cursor++; break;

      // drop value from stack
      case '.': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));
        stack_head--;
        cursor++;
        break;
      }

      // duplicate last value on stack
      case '@': {
        if (stack_head == STACK_LIMIT)
          crash(unpack_str(stack_overflow));
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head] = stack[stack_head - 1U];
        stack_head++;
        cursor++;
        break;
      }

      // swap two last values on stack
      case '^': {
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        unsigned char buff = stack[stack_head - 1U];
        stack[stack_head - 1U] = stack[stack_head - 2U];
        stack[stack_head - 2U] = buff;
        cursor++;
        break;
      }

      // 'conveyor belt' operator
      // place last value on the stack at the beginning
      case '#': {
        if (stack_head == 0U)
          break;
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
        if (stack_head == 0U)
          break;
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
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 1U] ^= 1U;
        cursor++;
        break;
      }

      // compare two stack values, consume them and push 1 or 0 depending on whether they're equal
      case '=': {
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 2U] = stack[stack_head - 2U] == stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // compare two stack values, consume them and push 1 or 0 depending on how they compare
      // if last is bigger than next then 0, otherwise 1
      case '?': {
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 2U] = stack[stack_head - 2U] < stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // add two stack values, consume them and push result of addition 
      case '+': {
        // todo: define behaviour of overflow
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 2U] = stack[stack_head - 2U] + stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // subtract two stack values, consume them and push result of subtraction
      // pops subtractor first, then subtrahend
      case '-': {
        // todo: define behaviour of overflow
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 2U] = stack[stack_head - 2U] - stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // multiply two stack values, consume them and push result of multiplication 
      case '*': {
        // todo: define behaviour of overflow
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        stack[stack_head - 2U] = stack[stack_head - 2U] * stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // divide two stack values, consume them and push result of division
      // pops divider first, then dividend
      case '/': {
        // todo: define behaviour of overflow
        if (stack_head < 2U)
          crash(unpack_str(stack_exhausted));
        if (stack[stack_head - 1U] == 0U) {
          ExitProcess(0L); // for now division by 00 terminates program silently
        }
        stack[stack_head - 2U] = stack[stack_head - 2U] / stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // pop from stack and print
      case '<': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));
        print_value(stack[stack_head - 1U]);
        stack_head--;
        cursor++;
        break;
      }

      // push single token from stdin
      case '>': {
        if (stack_head == STACK_LIMIT)
          crash(unpack_str(stack_overflow));

        char token[2];
        unsigned int chars_read;
        read(token, 2U, &chars_read);

        if (token[0] & 0b10000000U)
          crash(unpack_str(non_ascii_char));

        if ((token[0] >= 'A' && token[0] <= 'F') || (token[0] >= '0' && token[0] <= '9')) {
          unsigned char leading = token[0] - '0';
          if (leading > 9U)
            leading -= 7U;

          if (chars_read != 2U)
            crash(unpack_str(invalid_hex_value));
          if (token[1] & 0b10000000U)
            crash(unpack_str(non_ascii_char));

          if ((token[1] >= 'A' && token[1] <= 'F') || (token[1] >= '0' && token[1] <= '9')) {
            unsigned char following = token[1] - '0';
            if (following > 9U)
              following -= 7U;

            stack[stack_head++] = (leading << 4U) | following;

          } else
            crash(unpack_str(invalid_hex_value));

        } else
          stack[stack_head++] = (unsigned char)token[0];

        cursor++;
        break;
      }

      // pop from stack and rewind N tokens back
      case '[': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));

        unsigned char n_tokens = stack[stack_head - 1U];
        stack_head--;

        if (catch_infinite_recursion) {
          if (shadow_stack_rewinded_with != 0U) {
            if (value_array_compare(shadow_stack, shadow_stack_len, stack, stack_head)) {
                put("! HALT ON INFINITE RECURSION !");
                cursor = size;
                break;
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
              if (input[cursor] & 0b10000000U)
                crash(unpack_str(non_ascii_char));

              if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                if (cursor == 0U)
                  crash(unpack_str(invalid_hex_value));

                cursor--;

                if (input[cursor] & 0b10000000U)
                  crash(unpack_str(non_ascii_char));

                if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                  n_tokens--;
                } else {
                  crash(unpack_str(invalid_hex_value));
                }
              } else {
                n_tokens--;
              }
            }
          }

          if ((cursor == 0U) && (n_tokens != 0U))
            crash(unpack_str(input_exhausted));
          else if (n_tokens != 0U)
            cursor--;
        }
        break;
      }

      // pop from stack and seek N tokens forward
      case ']': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));

        unsigned char n_tokens = stack[stack_head - 1U];
        stack_head--;
        cursor++;

        while (n_tokens != 0U) {
          if (cursor == size)
            crash(unpack_str(input_exhausted));

          switch (input[cursor]) {
            case  ' ':
            case '\n':
            case '\r':
            case '\t': break;
            default: {
              if (input[cursor] & 0b10000000U)
                crash(unpack_str(non_ascii_char));

              if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                if (cursor == size)
                  crash(unpack_str(invalid_hex_value));

                cursor++;

                if (input[cursor] & 0b10000000U)
                  crash(unpack_str(non_ascii_char));

                if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
                  n_tokens--;
                } else {
                  crash(unpack_str(invalid_hex_value));
                }
              } else {
                n_tokens--;
              }
            }
          }

          if ((cursor == size) && (n_tokens != 0U))
            crash(unpack_str(input_exhausted));
          cursor++;
        }
        break;
      }

      // otherwise push it as character or hex value
      default: {
        if (stack_head == STACK_LIMIT)
          crash(unpack_str(stack_overflow));
        if (input[cursor] & 0b10000000U)
          crash(unpack_str(non_ascii_char));

        if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
          unsigned char leading = input[cursor] - '0';
          if (leading > 9U)
            leading -= 7U;

          if (cursor == size)
            crash(unpack_str(invalid_hex_value));
          if (input[cursor] & 0b10000000U)
            crash(unpack_str(non_ascii_char));

          cursor++;

          if ((input[cursor] >= 'A' && input[cursor] <= 'F') || (input[cursor] >= '0' && input[cursor] <= '9')) {
            unsigned char following = input[cursor] - '0';
            if (following > 9U)
              following -= 7U;

            cursor++;
            stack[stack_head++] = (leading << 4U) | following;

          } else
            crash(unpack_str(invalid_hex_value));

        } else
          stack[stack_head++] = (unsigned char)input[cursor++];
      }
      if (print_stack_steps == (_Bool)1) {
        put("|");
        print_stack(stack, stack_head);
        put("|\n");
      }
    }
  }
  put("\n|");
  print_stack(stack, stack_head);
  put("|\n");
}

int main(int argc, char** argv, char** envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  _Bool print_stack_steps = (_Bool)0;
  _Bool catch_infinite_recursion = (_Bool)0;

  for (int i = 0; i < argc; i++) {
    // turn all debug switches
    if (cstring_compare(argv[i], "-d")) {
      print_stack_steps = (_Bool)1;
      catch_infinite_recursion = (_Bool)1;

    // turn on stack step printing
    } else if (cstring_compare(argv[i], "-s")) {
      print_stack_steps = (_Bool)1;

    // turn off stack step printing
    } else if (cstring_compare(argv[i], "-ns")) {
      print_stack_steps = (_Bool)0;

    // output characters in hex representation
    } else if (cstring_compare(argv[i], "-h")) {
      print_as_hex = (_Bool)1;

    // EXPERIMENTAL: try to catch rewinds that don't change state of stack
    // and thus are most likely infinitely looped
    } else if (cstring_compare(argv[i], "-l")) {
      catch_infinite_recursion = (_Bool)1;
    }
  }

  read_input(print_stack_steps, catch_infinite_recursion);
}
