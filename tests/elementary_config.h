/* elementary_config.h.  Generated from elementary_config.h.in by configure.  */
/* elementary_config.h.in.  Generated from configure.ac by autoheader.  */


#ifndef EFL_CONFIG_H__
#define EFL_CONFIG_H__


/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* "Shared extension" */
#define EFL_SHARED_EXTENSION ".so"

/* "subdirectory for all elementary data" */
#define ELEMENTARY_BASE_DIR ".elementary"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* autoscroll with conformant */
#define HAVE_CONFORMANT_AUTOSCROLL 1

/* Define to 1 if you have the <crt_externs.h> header file. */
/* #undef HAVE_CRT_EXTERNS_H */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the `dladdr' function. */
#define HAVE_DLADDR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `dlopen' function. */
/* #undef HAVE_DLOPEN */

/* Use EIO for asynchronous file access */
/* #undef HAVE_EIO */

/* COCOA support for Elementary */
/* #undef HAVE_ELEMENTARY_COCOA */

/* Elementary debug. */
/* #undef HAVE_ELEMENTARY_DEBUG */

/* Have Ecore_Con support */
#define HAVE_ELEMENTARY_ECORE_CON 1

/* Have Ecore_IMF support */
#define HAVE_ELEMENTARY_ECORE_IMF 1

/* EDBus support for Elementary */
#define HAVE_ELEMENTARY_EDBUS 1

/* Efreet support for Elementary */
#define HAVE_ELEMENTARY_EFREET 1

/* EMap support for Elementary */
/* #undef HAVE_ELEMENTARY_EMAP */

/* Ethumb support for Elementary */
#define HAVE_ELEMENTARY_ETHUMB 1

/* EWeather support for Elementary */
/* #undef HAVE_ELEMENTARY_EWEATHER */

/* FB support for Elementary */
#define HAVE_ELEMENTARY_FB 1

/* PSL1GHT support for Elementary */
/* #undef HAVE_ELEMENTARY_PSL1GHT */

/* SDL support for Elementary */
/* #undef HAVE_ELEMENTARY_SDL */

/* Web support for Elementary */
/* #undef HAVE_ELEMENTARY_WEB */

/* Windows XP support for Elementary */
/* #undef HAVE_ELEMENTARY_WIN32 */

/* Windows CE support for Elementary */
/* #undef HAVE_ELEMENTARY_WINCE */

/* X11 support for Elementary */
#define HAVE_ELEMENTARY_X 1

/* Use Emotion for asynchronous file access */
/* #undef HAVE_EMOTION */

/* Set to 1 if evil package is installed. */
/* #undef HAVE_EVIL */

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Have sys/mman.h header file */
#define HAVE_MMAN_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasestr' function. */
#define HAVE_STRCASESTR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if your compiler has __attribute__ */
#define HAVE___ATTRIBUTE__ 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* "Module architecture" */
#define MODULE_ARCH "linux-gnueabi-arm-0.8.0"

/* "Edje module architecture" */
#define MODULE_EDJE "linux-gnueabi-arm-1.0.0"

/* Name of package */
#define PACKAGE "elementary"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "enlightenment-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "elementary"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "elementary 0.8.0.67547"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "elementary"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.8.0.67547"

/* default value since PATH_MAX is not defined */
/* #undef PATH_MAX */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "0.8.0.67547"

/* Major version */
#define VMAJ 0

/* Micro version */
#define VMIC 0

/* Minor version */
#define VMIN 8

/* Revison */
#define VREV 67547

/* "WebKit's data dir" */
/* #undef WEBKIT_DATADIR */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Macro declaring a function argument to be unused */
#define __UNUSED__ __attribute__((unused))

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* "apple doesn't follow POSIX in this case." */
/* #undef environ */


#endif /* EFL_CONFIG_H__ */

