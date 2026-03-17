/*
 * C++ compatibility helpers for older toolchains (e.g. KEIL without C++11).
 */
#ifndef __APP_COMMON_COMPAT_H
#define __APP_COMMON_COMPAT_H

#include <stddef.h> /* NULL */

/* KEIL/ARMCC5 may not support C++11 nullptr.
 * IMPORTANT: Avoid macroizing nullptr for Clang/GCC/MSVC toolchains. */
#if defined(__cplusplus) && (defined(__CC_ARM) || defined(__ARMCC_VERSION)) && !defined(__clang__)
#if !defined(__cpp_nullptr)
#ifndef nullptr
#define nullptr NULL
#endif
#endif
#endif

#endif /* __APP_COMMON_COMPAT_H */

