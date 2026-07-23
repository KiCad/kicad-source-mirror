/* libwmf ("wmfdefs.h"): library for wmf conversion
   Copyright (C) 2000,2001 - various; see CREDITS, ChangeLog, and sources

   The libwmf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The libwmf Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the libwmf Library; see the file COPYING.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifndef WMFDEFS_H
#define WMFDEFS_H

#include "libwmf/fund.h"
#include "libwmf/types.h"
#include "libwmf/api.h"
#include "libwmf/defs.h"
#include "libwmf/ipa.h"
#include "libwmf/font.h"
#include "libwmf/color.h"
#include "libwmf/macro.h"

#ifdef LIBWMF_INCLUDE_TRIO_H
#include "extra/trio/trio.h"
#endif

#define WMF_BMP_OPAQUE 0xff /* I think, or should this be zero ?? */

/* API flags: flags to lie in range (1<<20) to (1<<31)
 */
#define API_DEVICE_OPEN        (1<<20)
#define API_FTLIBRARY_OPEN     (1<<21)
#define API_FILE_OPEN          (1<<22)

#define API_ENABLE_EDITING     (1<<30)
#define API_STANDARD_INTERFACE (1U<<31)

/* API-independent defs & macros *only*
 */

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#ifdef ABS
#undef ABS
#endif /* ABS */
#define ABS(X) (((X) < 0) ? (-(X)) : (X))

#ifdef ROUND
#undef ROUND
#endif /* ROUND */
#define ROUND(X) (((X) < 0.0) ? ((X) - 0.5) : ((X) + 0.5))

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef PI
#define PI      3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI   6.28318530717958647692
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#define WMF_U16_U16_to_U32(A,B) ((((U32) (B)) << 16) + (U32) (A));

/* (Simple) API-dependent defs
 */

#define ERR(API)  ((API)->err != wmf_E_None)
#define DIAG(API) ((API)->flags & WMF_OPT_DIAGNOSTICS)

/* Returns 0 if 'requested' more bytes can be read from the current input
 * position, otherwise -1 with API->err set. Position is preserved. */
extern int wmf_can_supply (wmfAPI* API, long requested);

#endif /* ! WMFDEFS_H */
