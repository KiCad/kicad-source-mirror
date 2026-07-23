/* libwmf (<libwmf/fund.h>): library for wmf conversion
   Copyright (C) 2000 - various; see CREDITS, ChangeLog, and sources

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


#ifndef LIBWMF_FUND_H
#define LIBWMF_FUND_H

/* Other fundamental types: is this the best way to do it??
 * Potential conflicts with wv, or other, if not careful...
 */

#ifndef U32
#define U32 unsigned int
#endif

#ifndef S32
#define S32 int
#endif

#ifndef U16
#define U16 unsigned short
#endif

#ifndef S16
#define S16 short
#endif

#ifndef U8
#define U8 unsigned char
#endif

/* How to convert U16 to S16 in an architecturally independent manner ??
 * Casting with `(S16)' may work if `short' is 16 bits, but not in general
 */
#ifdef U16_2_S16
#undef U16_2_S16
#endif /* U16_2_S16 */
#define U16_2_S16(X) (((X) & 0x8000) ? ((S16) ((S32) (X) - (S32) 0x10000)) : ((S16) (X)))

#ifdef U16_2_S32
#undef U16_2_S32
#endif /* U16_2_S32 */
#define U16_2_S32(X) (((X) & 0x8000) ? ((S32) (X) - (S32) 0x10000) : ((S32) (X)))

#endif /* ! LIBWMF_FUND_H */
