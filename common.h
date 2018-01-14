#pragma once

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


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
    #define CANARY_START ;uint16_t canary_start ;
    #define CANARY_END ; uint16_t canary_end ;
    // #define INIT_CANARY(THIS, NAME)  THIS->canary_start = 0x5DEC   ; THIS->canary_end = 0x5DEC  ;
    // #define CHECK_CANARY(THIS, NAME) assert(THIS->canary_start == 0x5DEC ) ; assert(THIS->canary_end == 0x5DEC ) ;
    #define INIT_CANARY(THIS, NAME)  THIS->canary_start = (size_t)#NAME "_start" ; THIS->canary_end = (size_t)#NAME "_end" ;
    #define CHECK_CANARY(THIS, NAME) assert(THIS->canary_start == (uint16_t)(size_t)#NAME "_start") ; assert(THIS->canary_end == (uint16_t)(size_t)#NAME "_end") ;
#else
    #define CANARY_START ;
    #define CANARY_END ;
    #define INIT_CANARY(THIS, NAME) ;
    #define CHECK_CANARY(THIS, NAME) ;
#endif 

