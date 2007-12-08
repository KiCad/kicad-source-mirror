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

#define TRACE printf

#define ASSERT(x)		// todo : change to DEBUG, under wxWidgets

#endif 		// ifndef DEFS_MACROS_H
