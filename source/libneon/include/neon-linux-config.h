/*
 * The configuration of WinSCP's neon (libs/neon) on Linux (LinSCP), what
 * neon's configure script would detect with glibc, OpenSSL 3 or 4 and expat.
 * libs/neon/src/config.h includes it on platforms other than Windows; the
 * rest of the configuration is defined by the build (source/libneon), as
 * WinSCP's makefile does (NE_HAVE_SSL, HAVE_OPENSSL, HAVE_EXPAT, ...).
 */
#ifndef NEON_LINUX_CONFIG_H
#define NEON_LINUX_CONFIG_H

#define NEON_VERSION "0.37.1"
#define NE_VERSION_MAJOR (0)
#define NE_VERSION_MINOR (37)

#define PACKAGE_NAME "neon"

#define HAVE_ERRNO_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_NETDB_H 1
#define HAVE_NET_IF_H 1

#define HAVE_MEMCPY 1
#define HAVE_SETSOCKOPT 1
#define HAVE_GETSOCKOPT 1
#define HAVE_SENDMSG 1
#define HAVE_SIGNAL 1
#define HAVE_SETVBUF 1
#define HAVE_FCNTL 1
#define HAVE_STPCPY 1
#define HAVE_STRERROR_R 1
#define STRERROR_R_CHAR_P 1
#define HAVE_HSTRERROR 1
#define HAVE_DECL_H_ERRNO 1
#define HAVE_GMTIME_R 1
#define HAVE_TIMEGM 1
#define HAVE_STRUCT_TM_TM_GMTOFF 1
#define HAVE_SOCKLEN_T 1
#define HAVE_GETRANDOM 1
#define HAVE_EXPLICIT_BZERO 1

#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_IF_INDEXTONAME 1
#define HAVE_IF_NAMETOINDEX 1
#define USE_GETADDRINFO 1
#define USE_GAI_ADDRCONFIG 1
#define NE_HAVE_IPV6 1

#define NE_HAVE_TS_SSL 1
/* OpenSSL 1.1.1 and later; without it, neon reads the fields of ASN1_TIME,
   which OpenSSL 4 makes opaque */
#define HAVE_ASN1_TIME_TO_TM 1
/* As WinSCP's Windows configuration */
#define HAVE_CRYPTO_SET_IDPTR_CALLBACK 1

/* Define to enable debugging */
#define NE_DEBUGGING 1

#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T 8

#define NE_FMT_SIZE_T "zu"
#define NE_FMT_SSIZE_T "zd"
#define NE_FMT_OFF_T "ld"
#define NE_FMT_OFF64_T "ld"
#define NE_FMT_NE_OFF_T NE_FMT_OFF_T
#define NE_FMT_TIME_T "ld"
#ifndef NE_FMT_XML_SIZE
#define NE_FMT_XML_SIZE "d"
#endif

/* Windows calls in WinSCP's changes of neon (under WINSCP) */
#include <sys/ioctl.h>
#define ioctlsocket(s, cmd, argp) ioctl((s), (cmd), (argp))
/* SSPI (Windows authentication) does not exist, see linux-compat.c */
int ne_sspi_init(void);

/* WinSCP's neon calls the 64-bit position functions of expat 2.9.0 (the
   former ones are deprecated there); an older system expat has the former
   ones only, which return the same 64-bit XML_Size on Linux */
#include <expat.h>
#if (XML_MAJOR_VERSION < 2) || ((XML_MAJOR_VERSION == 2) && (XML_MINOR_VERSION < 9))
#define XML_GetCurrentLineNumber64 XML_GetCurrentLineNumber
#define XML_GetCurrentColumnNumber64 XML_GetCurrentColumnNumber
#endif

#endif
