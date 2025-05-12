#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <atomic>
#include <cstdint>
#include <execinfo.h>

#include "sanitizer_common/sanitizer_stacktrace.h"
#include "sanitizer_common/sanitizer_common.h"
#include "ubsan/ubsan_handlers.h"
#include "ubsan/ubsan_type_hash.h"
#include "ubsan/ubsan_platform.h"

#include "s2lab.h"

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void s2lab() {
  printf("hello s2lab\n");
}
