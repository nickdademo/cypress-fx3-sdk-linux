/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _INTEGER
#define _INTEGER

#ifdef _WIN32	/* FatFs development platform */

#include <windows.h>
#include <tchar.h>

#else			/* Embedded platform */

/* Cypress: Some of these types may have been defined elsewhere. Adding them only where
   it is required. */
#ifndef VOID

/* These types must be 16-bit, 32-bit or larger integer */
typedef int				INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef char			CHAR;
typedef unsigned char	UCHAR;

/* These types must be 16-bit integer */
typedef short			SHORT;
typedef unsigned short	USHORT;

/* These types must be 32-bit integer */
typedef long			LONG;
typedef unsigned long	ULONG;

#endif

/* These types must be 8-bit integer */
typedef unsigned char	BYTE;

/* These types must be 16-bit integer */
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types must be 32-bit integer */
typedef unsigned long	DWORD;

#endif

#endif
