/* Hershey fonts */

/* This distribution of the Hershey Fonts may be used by anyone for
 *  any purpose, commercial or otherwise, providing that:
 *  1. The following acknowledgements must be distributed with
 *  the font data:
 *  - The Hershey Fonts were originally created by Dr.
 *  A. V. Hershey while working at the U. S.
 *  National Bureau of Standards.
 *  - The format of the Font data in this distribution
 *  was originally created by
 *  James Hurt
 *  Cognition, Inc.
 *  900 Technology Park Drive
 *  Billerica, MA 01821
 *  (mit-eddie!ci-dandelion!hurt)
 *  2. The font data in this distribution may be converted into
 *  any other format *EXCEPT* the format distributed by
 *  the U.S. NTIS (which organization holds the rights
 *  to the distribution and use of the font data in that
 *  particular format). Not that anybody would really
 *  *want* to use their format... each point is described
 *  in eight bytes as "xxx yyy:", where xxx and yyy are
 *  the coordinate values as ASCII numbers.
 */

/*
 *  Hershey fonts are vectored fonts.
 * Note one can find many formats for these vectored fonts
 * here is the format used :
 *  >shapes are a set of polygons.
 *  >A given shape includes one or more polygons.
 *  >corner coordinates are coded by a XY pair.
 *  >The value of each coordinate is <ascii code> - 'R'
 *  >The coordinate (-50,0) or " R" is the Pen Up command (end of the current polygon)
 */

#include "HersheySimplexRoman_sans_normal.h"

#if defined(KICAD_CYRILLIC)
#include "HersheyCyrillic.h"
#endif
