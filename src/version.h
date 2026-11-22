#ifndef VERSION_H
#define VERSION_H

#define		VERSION_MAJOR		3
#define		VERSION_MINOR		1
#define		VERSION_PATCH		0
#define		VERSION_CANDIDATE	2			// Optional
#define		VERSION_TYPE		"RC "     // RC, Alpha, Beta, etc.

#ifdef EMOS_NMI_BOOT_ID
#define VERSION_VARIANT "Eirene UART0 r06 NMI API R01 [" EMOS_NMI_BOOT_ID "]"
#else
#define VERSION_VARIANT "Eirene UART0 r06 NMI API R01 [unidentified]"
#endif
//#define     VERSION_SUBTITLE    "Arthur"

#endif /* VERSION_H */
