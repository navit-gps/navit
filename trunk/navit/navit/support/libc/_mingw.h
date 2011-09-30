#ifndef __MINGW_H
/*
 * _mingw.h
 *
 * Mingw specific macros included by ALL include files.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Mumit Khan  <khan@xraylith.wisc.edu>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#define __MINGW_H

#define __MINGW32_VERSION           3.16
#define __MINGW32_MAJOR_VERSION     3
#define __MINGW32_MINOR_VERSION     16
#define __MINGW32_PATCHLEVEL        0

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

/* These are defined by the user (or the compiler)
   to specify how identifiers are imported from a DLL.

   __DECLSPEC_SUPPORTED            Defined if dllimport attribute is supported.
   __MINGW_IMPORT                  The attribute definition to specify imported
                                   variables/functions.
   _CRTIMP                         As above.  For MS compatibility.
   __MINGW32_VERSION               Runtime version.
   __MINGW32_MAJOR_VERSION         Runtime major version.
   __MINGW32_MINOR_VERSION         Runtime minor version.
   __MINGW32_BUILD_DATE            Runtime build date.

   Macros to enable MinGW features which deviate from standard MSVC
   compatible behaviour; these may be specified directly in user code,
   activated implicitly, (e.g. by specifying _POSIX_C_SOURCE or such),
   or by inclusion in __MINGW_FEATURES__:

   __USE_MINGW_ANSI_STDIO          Select a more ANSI C99 compatible
                                   implementation of printf() and friends.

   Other macros:

   __int64                         define to be long long.  Using a typedef
                                   doesn't work for "unsigned __int64"

   All headers should include this first, and then use __DECLSPEC_SUPPORTED
   to choose between the old ``__imp__name'' style or __MINGW_IMPORT
   style declarations.  */


/* Manifest definitions identifying the flag bits, controlling activation
 * of MinGW features, as specified by the user in __MINGW_FEATURES__.
 */
#define __MINGW_ANSI_STDIO__		0x0000000000000001ULL
/*
 * The following three are not yet formally supported; they are
 * included here, to document anticipated future usage.
 */
#define __MINGW_LC_EXTENSIONS__ 	0x0000000000000050ULL
#define __MINGW_LC_MESSAGES__		0x0000000000000010ULL
#define __MINGW_LC_ENVVARS__		0x0000000000000040ULL

/* Try to avoid problems with outdated checks for GCC __attribute__ support.  */
#undef __attribute__

#if defined (__PCC__)
#  undef __DECLSPEC_SUPPORTED
# ifndef __MINGW_IMPORT
#  define __MINGW_IMPORT extern
# endif
# ifndef _CRTIMP
#  define _CRTIMP
# endif
# ifndef __cdecl 
#  define __cdecl  _Pragma("cdecl")
# endif
# ifndef __stdcall
#  define __stdcall _Pragma("stdcall")
# endif
# ifndef __int64
#  define __int64 long long
# endif
# ifndef __int32
#  define __int32 long
# endif
# ifndef __int16
#  define __int16 short
# endif
# ifndef __int8
#  define __int8 char
# endif
# ifndef __small
#  define __small char
# endif
# ifndef __hyper
#  define __hyper long long
# endif
# ifndef __volatile__
#  define __volatile__ volatile
# endif
# ifndef __restrict__
#  define __restrict__ restrict
# endif
# define NONAMELESSUNION
#elif defined(__GNUC__)
# ifdef __declspec
#  ifndef __MINGW_IMPORT
   /* Note the extern. This is needed to work around GCC's
      limitations in handling dllimport attribute.  */
#   define __MINGW_IMPORT  extern __attribute__ ((__dllimport__))
#  endif
#  ifndef _CRTIMP
#   ifdef __USE_CRTIMP
#    define _CRTIMP  __attribute__ ((dllimport))
#   else
#    define _CRTIMP
#   endif
#  endif
#  define __DECLSPEC_SUPPORTED
# else /* __declspec */
#  undef __DECLSPEC_SUPPORTED
#  undef __MINGW_IMPORT
#  ifndef _CRTIMP
#   define _CRTIMP
#  endif
# endif /* __declspec */
/*
 * The next two defines can cause problems if user code adds the
 * __cdecl attribute like so:
 * void __attribute__ ((__cdecl)) foo(void); 
 */
# ifndef __cdecl 
#  define __cdecl  __attribute__ ((__cdecl__))
# endif
# ifndef __stdcall
#  define __stdcall __attribute__ ((__stdcall__))
# endif
# ifndef __int64
#  define __int64 long long
# endif
# ifndef __int32
#  define __int32 long
# endif
# ifndef __int16
#  define __int16 short
# endif
# ifndef __int8
#  define __int8 char
# endif
# ifndef __small
#  define __small char
# endif
# ifndef __hyper
#  define __hyper long long
# endif
#else /* ! __GNUC__ && ! __PCC__ */
# ifndef __MINGW_IMPORT
#  define __MINGW_IMPORT  __declspec(dllimport)
# endif
# ifndef _CRTIMP
#  define _CRTIMP  __declspec(dllimport)
# endif
# define __DECLSPEC_SUPPORTED
# define __attribute__(x) /* nothing */
#endif

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
#define __MINGW_GNUC_PREREQ(major, minor) \
  (__GNUC__ > (major) \
   || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define __MINGW_GNUC_PREREQ(major, minor)  0
#endif

#ifdef __cplusplus
#abort1
# define __CRT_INLINE inline
#else
# if __GNUC_STDC_INLINE__
#abort2
#  define __CRT_INLINE extern inline __attribute__((__gnu_inline__))
# else
#  ifdef __COREDLL__
#abort3
   /* There isn't any out-of-line version of most of 
      these functions in coredll.dll, so we need this for -O0,
      or for -fno-inline.  This is still problematic if the user
      tries t	o take the address of these functions.  We will slowly
      add out-of-line copies as those cases are found.
      Note: We can't use static inline here, as most of these functions
      will be declared elsew	here with external linkage, and gcc will
      barf on that.  */
#   define __CRT_INLINE extern __inline__ __attribute__((__always_inline__))
#  else
#   define __CRT_INLINE
#  endif
# endif
#endif

#ifdef __cplusplus
# define __UNUSED_PARAM(x)
#else
# ifdef __GNUC__
#  define __UNUSED_PARAM(x) x __attribute__ ((__unused__))
# else
#  define __UNUSED_PARAM(x) x
# endif
#endif

#ifdef __GNUC__
#define __MINGW_ATTRIB_NORETURN __attribute__ ((__noreturn__))
#define __MINGW_ATTRIB_CONST __attribute__ ((__const__))
#else
#define __MINGW_ATTRIB_NORETURN
#define __MINGW_ATTRIB_CONST
#endif

#if __MINGW_GNUC_PREREQ (3, 0)
#define __MINGW_ATTRIB_MALLOC __attribute__ ((__malloc__))
#define __MINGW_ATTRIB_PURE __attribute__ ((__pure__))
#else
#define __MINGW_ATTRIB_MALLOC
#define __MINGW_ATTRIB_PURE
#endif

/* Attribute `nonnull' was valid as of gcc 3.3.  We don't use GCC's
   variadiac macro facility, because variadic macros cause syntax
   errors with  --traditional-cpp.  */
#if  __MINGW_GNUC_PREREQ (3, 3)
#define __MINGW_ATTRIB_NONNULL(arg) __attribute__ ((__nonnull__ (arg)))
#else
#define __MINGW_ATTRIB_NONNULL(arg)
#endif /* GNUC >= 3.3 */

#if defined(UNDER_CE) && defined(__arm__)
/* ARM Windows CE is not underscored.  */
# define __U(SYM) _ ## SYM
# define __IMP(S) __imp_ ## S
#elif defined(UNDER_CE) && defined(i386)
/* i386 Windows CE versions are underscored.  */
# define __U(SYM) SYM
# define __IMP(S) _imp__ ## S
#else
/* Desktop i386 Windows versions are underscored.  */
# define __U(SYM) SYM
# define __IMP(S) _imp__ ## S
#endif

#if  __MINGW_GNUC_PREREQ (3, 1)
#define __MINGW_ATTRIB_DEPRECATED __attribute__ ((__deprecated__))
#else
#define __MINGW_ATTRIB_DEPRECATED
#endif /* GNUC >= 3.1 */
 
#if  __MINGW_GNUC_PREREQ (3, 3)
#define __MINGW_NOTHROW __attribute__ ((__nothrow__))
#else
#define __MINGW_NOTHROW
#endif /* GNUC >= 3.3 */


/* TODO: Mark (almost) all CRT functions as __MINGW_NOTHROW.  This will
allow GCC to optimize away some EH unwind code, at least in DW2 case.  */

#if defined __MSVCRT__ && !defined (__MSVCRT_VERSION__)
/*  High byte is the major version, low byte is the minor. */
# define __MSVCRT_VERSION__ 0x0600
#endif

/* Activation of MinGW specific extended features:
 */
#ifndef __USE_MINGW_ANSI_STDIO
/*
 * If user didn't specify it explicitly...
 */
# if   defined __STRICT_ANSI__  ||  defined _ISOC99_SOURCE \
   ||  defined _POSIX_SOURCE    ||  defined _POSIX_C_SOURCE \
   ||  defined _XOPEN_SOURCE    ||  defined _XOPEN_SOURCE_EXTENDED \
   ||  defined _GNU_SOURCE      ||  defined _BSD_SOURCE \
   ||  defined _SVID_SOURCE
   /*
    * but where any of these source code qualifiers are specified,
    * then assume ANSI I/O standards are preferred over Microsoft's...
    */
#  define __USE_MINGW_ANSI_STDIO    1
# else
   /*
    * otherwise use whatever __MINGW_FEATURES__ specifies...
    */
#  define __USE_MINGW_ANSI_STDIO    (__MINGW_FEATURES__ & __MINGW_ANSI_STDIO__)
# endif
#endif

#endif /* __MINGW_H */
