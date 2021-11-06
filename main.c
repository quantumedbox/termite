#include "nocrt0/nocrt0c.c"

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

static void print(const char* msg, unsigned int len) {
  DWORD chars_written;
  (void)WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, len, &chars_written, NULL);
  (void)chars_written;
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

static void read_input(void) {
  char input[INPUT_LIMIT + 1U];
  unsigned int size;
  read(input, INPUT_LIMIT, &size);
  if (size == INPUT_LIMIT + 1U) {
    crash(unpack_str(input_overflow));
  }

  unsigned char stack[STACK_LIMIT];
  unsigned int stack_head = 0U;
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
      // place last value on the stack to the beginning
      case '#': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));

        unsigned char buff = stack[stack_head - 1U];
        for (unsigned int i = 0U; i < stack_head; i++) {
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
        stack[stack_head - 2U] = stack[stack_head - 2U] / stack[stack_head - 1U];
        stack_head--;
        cursor++;
        break;
      }

      // pop from stack and print
      case '<': {
        if (stack_head == 0U)
          crash(unpack_str(stack_exhausted));
        print((const char*)&stack[stack_head - 1U], 1U);
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
    }
  }
  put("\n|");
  print((const char*)stack, stack_head);
  put("|");
}

int main(int argc, char** argv, char** envp) {
  (void)argc;
  (void)argv;
  (void)envp;
  read_input();
}
