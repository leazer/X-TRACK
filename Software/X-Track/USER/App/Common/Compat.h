/*
 * C++ compatibility helpers for older toolchains (e.g. KEIL without C++11).
 */
#ifndef __APP_COMMON_COMPAT_H
#define __APP_COMMON_COMPAT_H

#include <stddef.h> /* NULL */

/* KEIL (or other legacy compilers) may not support C++11 nullptr. */
#if defined(__cplusplus) && !defined(__cpp_nullptr)
/* If the compiler doesn't support nullptr, map it to NULL. */
#ifndef nullptr
#define nullptr NULL
#endif
#endif

#endif /* __APP_COMMON_COMPAT_H */

