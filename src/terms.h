#ifndef LIMITS_H
#define LIMITS_H

#define INPUT_LIMIT 66560U // 65KB
#define STACK_LIMIT 66560U // 65KB
#define FILEPATH_LIMIT 128U // 128 bytes

enum OutputCodes {
  OC_OK,
  OC_INPUT_OVERFLOW,
  OC_STACK_OVERFLOW,
  OC_INPUT_EXHAUSTED,
  OC_STACK_EXHAUSTED,
  OC_INVALID_INPUT,
  OC_ZERO_DIVISION,
  OC_INFINITE_LOOP,

  OC_FILE_ERROR = 0x10, // todo: make it generic 'IO error'?

  OC_LAST_RESERVED = 0x1F // 32 values are reserved for termite status exits, other values are free to use
};

#endif
