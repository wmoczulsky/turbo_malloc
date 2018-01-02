#pragma once

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) 
    #error Unimplemented yet
#else
    #include "platform/posix.h"
#endif


