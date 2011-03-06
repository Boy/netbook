/** 
 * @file llpreprocessor.h
 * @brief This file should be included in all Linden Lab files and
 * should only contain special preprocessor directives
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LLPREPROCESSOR_H
#define LLPREPROCESSOR_H

// Figure out endianness of platform
#ifdef LL_LINUX
#define __ENABLE_WSTRING
#include <endian.h>
#endif	//	LL_LINUX

#if (defined(LL_WINDOWS) || (defined(LL_LINUX) && (__BYTE_ORDER == __LITTLE_ENDIAN)) || (defined(LL_DARWIN) && defined(__LITTLE_ENDIAN__)))
#define LL_LITTLE_ENDIAN 1
#else
#define LL_BIG_ENDIAN 1
#endif

// Per-compiler switches
#ifdef __GNUC__
#define LL_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LL_FORCE_INLINE __forceinline
#endif

// Per-OS feature switches.

#if LL_DARWIN
	#define LL_QUICKTIME_ENABLED	1
	#define LL_LIBXUL_ENABLED		1
#elif LL_WINDOWS
	#define LL_QUICKTIME_ENABLED	1
	#define LL_LIBXUL_ENABLED		1
#elif LL_LINUX
	#define LL_QUICKTIME_ENABLED	0
        #ifndef LL_LIBXUL_ENABLED
                #define LL_LIBXUL_ENABLED		1
        #endif // def LL_LIBXUL_ENABLED
#elif LL_SOLARIS
	#define LL_QUICKTIME_ENABLED    0
	#ifndef LL_LIBXUL_ENABLED
		#define LL_LIBXUL_ENABLED               0
	#endif // def LL_LIBXUL_ENABLED
#endif

#if LL_LIBXUL_ENABLED && !defined(MOZILLA_INTERNAL_API)
	// Without this, nsTAString.h errors out with:
	// "Cannot use internal string classes without MOZILLA_INTERNAL_API defined. Use the frozen header nsStringAPI.h instead."
	// It might be worth our while to figure out if we can use the frozen apis at some point...
	#define MOZILLA_INTERNAL_API 1
#endif

// Figure out differences between compilers
#if defined(__GNUC__)
	#define GCC_VERSION (__GNUC__ * 10000 \
						+ __GNUC_MINOR__ * 100 \
						+ __GNUC_PATCHLEVEL__)
	#ifndef LL_GNUC
		#define LL_GNUC 1
	#endif
#elif defined(__MSVC_VER__) || defined(_MSC_VER)
	#ifndef LL_MSVC
		#define LL_MSVC 1
	#endif
#endif

// Deal with minor differences on Unixy OSes.
#if LL_DARWIN || LL_LINUX
	// Different name, same functionality.
	#define stricmp strcasecmp
	#define strnicmp strncasecmp

	// Not sure why this is different, but...
	#ifndef MAX_PATH
		#define MAX_PATH PATH_MAX
	#endif	//	not MAX_PATH

#endif

// Deal with the differeneces on Windows
#if LL_MSVC
#define snprintf safe_snprintf		/* Flawfinder: ignore */
#endif	// LL_MSVC

// Static linking with apr on windows needs to be declared.
#ifdef LL_WINDOWS
#ifndef APR_DECLARE_STATIC
#define APR_DECLARE_STATIC // For APR on Windows
#endif
#ifndef APU_DECLARE_STATIC
#define APU_DECLARE_STATIC // For APR util on Windows
#endif
#endif

#if defined(LL_WINDOWS)
#define BOOST_REGEX_NO_LIB 1
#define CURL_STATICLIB 1

//#define LL_LCD_COMPILE 0

#endif	//	LL_WINDOWS


// Deal with VC6 problems
#if LL_MSVC
#pragma warning( 3	     : 4701 )	// "local variable used without being initialized"  Treat this as level 3, not level 4.
#pragma warning( 3	     : 4702 )	// "unreachable code"  Treat this as level 3, not level 4.
#pragma warning( 3	     : 4189 )	// "local variable initialized but not referenced"  Treat this as level 3, not level 4.
//#pragma warning( 3	: 4018 )	// "signed/unsigned mismatch"  Treat this as level 3, not level 4.
#pragma warning( 3       : 4265 )	// "class has virtual functions, but destructor is not virtual"
#pragma warning( disable : 4786 )	// silly MS warning deep inside their <map> include file
#pragma warning( disable : 4284 )	// silly MS warning deep inside their <map> include file
#pragma warning( disable : 4503 )	// 'decorated name length exceeded, name was truncated'. Does not seem to affect compilation.
#pragma warning( disable : 4800 )	// 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( disable : 4996 )	// warning: deprecated
#endif	//	LL_MSVC

#endif	//	not LL_LINDEN_PREPROCESSOR_H
