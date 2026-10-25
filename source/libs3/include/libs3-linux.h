// What WinSCP's changes of libs3 expect from Windows (LinSCP), included
// in front of every libs3 source: GetSystemTime (request.c) and strnicmp
// (response_headers_handler.c).
#ifndef LIBS3_LINUX_H
#define LIBS3_LINUX_H

#include <winapi/TimeApi.h>

#include <strings.h>

#define strnicmp strncasecmp

#endif
