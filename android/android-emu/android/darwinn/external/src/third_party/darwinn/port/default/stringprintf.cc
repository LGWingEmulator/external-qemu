// Copyright 2002 and onwards Google Inc.
// Author: Sanjay Ghemawat

#include "third_party/darwinn/port/default/stringprintf.h"

#include <errno.h>
#include <stdarg.h>  // For va_list and related operations
#include <stdio.h>  // MSVC requires this for _vsnprintf
#include <vector>

namespace platforms {
namespace darwinn {

#ifdef COMPILER_MSVC
enum { IS_COMPILER_MSVC = 1 };
#else
enum { IS_COMPILER_MSVC = 0 };
#endif

// Lower-level routine that takes a va_list and appends to a specified
// string. All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap) {
  // First try with a small fixed size buffer
  static const int kSpaceLength = 1024;
  char space[kSpaceLength];

  // It's possible for methods that use a va_list to invalidate the data in it
  // upon use. The fix is to make a copy of the structure before using it and
  // use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = vsnprintf(space, kSpaceLength, format, backup_ap);
  va_end(backup_ap);

  if (result < kSpaceLength) {
    if (result >= 0) {
      // Normal case -- everything fit.
      dst->append(space, result);
      return;
    }

    if (IS_COMPILER_MSVC) {
      // Error or MSVC running out of space. MSVC 8.0 and higher can be asked
      // about space needed with the special idiom below:
      va_copy(backup_ap, ap);
      result = vsnprintf(nullptr, 0, format, backup_ap);
      va_end(backup_ap);
    }

    if (result < 0) {
      // Just an error.
      return;
    }
  }

  // Increase the buffer size to the size requested by vsnprintf, plus one for
  // the closing \0.
  int length = result + 1;
  char* buf = new char[length];

  // Restore the va_list before we use it again
  va_copy(backup_ap, ap);
  result = vsnprintf(buf, length, format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < length) {
    // It fit
    dst->append(buf, result);
  }
  delete[] buf;
}

std::string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

}  // namespace darwinn
}  // namespace platforms
