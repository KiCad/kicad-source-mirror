/**********************/
/* Some usual defines */
/**********************/

#ifndef DEFS_MACROS_H
#define DEFS_MACROS_H

#ifndef BOOL
#define BOOL bool
#endif

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef abs
#define abs(x) (((x) >=0) ? (x) : (-(x)))
#endif


#ifndef min
#define min(x,y) (((x) <= (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x,y) (((x) >= (y)) ? (x) : (y))
#endif

#define TRACE printf

#endif 		// ifndef DEFS_MACROS_H
