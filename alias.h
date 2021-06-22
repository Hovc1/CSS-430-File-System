#ifndef ALIAS_H
#define ALIAS_H

// P5  Bothell File System
// Christopher Hovsepian and Nick Naslund

// gcc *.c *.h 

// Start with implementing fsRead. To do so, 
// look back at slides 31-33 in Lecture-16-FS-2.pptx.

// in  fs.c  implement  fsRead and fsWrite

// ==================================================================
// alias.h - typdefs and constants used within BFS (Bothell
// File System)
// ==================================================================

#include <stdint.h>

typedef uint8_t  u8;    // 0 - 255
typedef uint16_t u16;   // 0 - 65,536
typedef uint32_t u32;   // 0 - 4.29 Billion
typedef uint64_t u64;   // 0 - 18446.7 Quadrillion   18.4 Quintillion   .018 Sextillion

typedef int8_t   i8;    // -128 to 127
typedef int16_t  i16;   // +/- 32k
typedef int32_t  i32;   //normal int +/- 3.2 Billion
typedef int64_t  i64;

typedef char* str;                    // string type

#endif
