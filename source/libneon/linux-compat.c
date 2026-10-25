/*
 * What WinSCP's neon expects from Windows (LinSCP).
 */

#include "config.h"

/* WinSCP initializes SSPI (Windows authentication, Negotiate and NTLM)
 * through ne_sock_sspi_init; there is no SSPI here */
int ne_sspi_init(void)
{
    return 0;
}
