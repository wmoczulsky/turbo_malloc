#pragma once

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>



typedef struct _allocator allocator;


#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) 
    #include "platform/windows.h"
#else
    #include "platform/posix.h"
#endif


// I boldly assume, that gcc always interns string literals, 
// so I can use string ptr as canary value, which is much simpler 
// than converting string into an int compile-time
#ifndef NDEBUG
    #define PUT_CANARY(NAME) char *canary = #NAME ;
    #define CHECK_CANARY(THIS, NAME) assert(THIS->canary == #NAME) ;
#elif
    #define PUT_CANARY(NAME) ;
    #define CHECK_CANARY(THIS, NAME) ;
#endif 