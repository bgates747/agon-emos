#ifndef VERSION_H
#define VERSION_H

#define		VERSION_MAJOR		3
#define		VERSION_MINOR		1
#define		VERSION_PATCH		0
#define		VERSION_CANDIDATE	2			// Optional
#define		VERSION_TYPE		"RC "     // RC, Alpha, Beta, etc.

#define		VERSION_VARIANT		"Eirene UART0 NMI API R01"
#ifdef EMOS_BUILD_ID
#define VERSION_BUILD EMOS_BUILD_ID
#endif
//#define     VERSION_SUBTITLE    "Arthur"

#endif /* VERSION_H */
