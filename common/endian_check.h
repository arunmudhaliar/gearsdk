//
//  Copyright 2024 homenet25
//  endian_check.h
//  common
//
//  Created by Arun A on 07/11/23.
//

#ifndef endian_check_h
#define endian_check_h

// GSDK_ENDIAN
// Took from https://github.com/Tencent/rapidjson/blob/master/include/rapidjson/rapidjson.h
#define GSDK_LITTLEENDIAN 0	 //!< Little endian machine
#define GSDK_BIGENDIAN 1	 //!< Big endian machine

//! Endianness of the machine.
/*!
	\def GSDK_ENDIAN
	\ingroup GSDK_CONFIG

	GCC 4.6 provided macro for detecting endianness of the target machine. But other
	compilers may not have this. User can define GSDK_ENDIAN to either
	\ref GSDK_LITTLEENDIAN or \ref GSDK_BIGENDIAN.

	Default detection implemented with reference to
	\li https://gcc.gnu.org/onlinedocs/gcc-4.6.0/cpp/Common-Predefined-Macros.html
	\li http://www.boost.org/doc/libs/1_42_0/boost/detail/endian.hpp
*/
#ifndef GSDK_ENDIAN
// Detect with GCC 4.6's macro
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define GSDK_ENDIAN GSDK_LITTLEENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define GSDK_ENDIAN GSDK_BIGENDIAN
#else
#error Unknown machine endianness detected. User needs to define GSDK_ENDIAN.
#endif	// __BYTE_ORDER__
// Detect with GLIBC's endian.h
#elif defined(__GLIBC__)
#include <endian.h>
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define GSDK_ENDIAN GSDK_LITTLEENDIAN
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define GSDK_ENDIAN GSDK_BIGENDIAN
#else
#error Unknown machine endianness detected. User needs to define GSDK_ENDIAN.
#endif	// __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#define GSDK_ENDIAN GSDK_LITTLEENDIAN
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#define GSDK_ENDIAN GSDK_BIGENDIAN
// Detect with architecture macros
#elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#define GSDK_ENDIAN GSDK_BIGENDIAN
#elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || \
	defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#define GSDK_ENDIAN GSDK_LITTLEENDIAN
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#define GSDK_ENDIAN GSDK_LITTLEENDIAN
#elif defined(GSDK_DOXYGEN_RUNNING)
#define GSDK_ENDIAN
#else
#error Unknown machine endianness detected. User needs to define GSDK_ENDIAN.
#endif
#endif	// GSDK_ENDIAN

// #if GSDK_ENDIAN == GSDK_LITTLEENDIAN
// #pragma message ( "Little endian machine !!!" )
// #else
// #pragma message ( "Big endian machine !!!" )
// #endif

#endif /* endian_check_h */
