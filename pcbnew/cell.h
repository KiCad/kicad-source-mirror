/* Bits caracterisant une cellule */
#define HOLE 0x00000001L		/* a conducting hole, ou obstacle */
#define CELL_is_MODULE 2		/* autoplacement: occupe par un module */
#define CELL_is_EDGE 0x20		/* zone et autoplacement: cellule limitant un contour (Board, Zone) */
#define CELL_is_FRIEND 0x40		/* zone et autoplacement: cellule faisant partie du net */
#define CELL_is_ZONE 0x80		/* zone et autoplacement: cellule disponible */

/* Bits Masques de presence d'obstacles pour autoroutage */
#define OCCUPE 1				/* autoroutage : obstacle pour pistes et vias */
#define VIA_IMPOSSIBLE 2		/* autoroutage : obsacle pour vias */
#define CURRENT_PAD 4


/* traces radiating outward from a hole to a side or corner */
#define HOLE_NORTH		0x00000002L	/* upward		*/
#define HOLE_NORTHEAST		0x00000004L	/* upward and right	*/
#define HOLE_EAST		0x00000008L	/* to the right		*/
#define HOLE_SOUTHEAST		0x00000010L	/* downward and right	*/
#define HOLE_SOUTH		0x00000020L	/* downward		*/
#define HOLE_SOUTHWEST		0x00000040L	/* downward and left	*/
#define HOLE_WEST		0x00000080L	/* to the left		*/
#define HOLE_NORTHWEST		0x00000100L	/* upward and left	*/

/* straight lines through the center */
#define LINE_HORIZONTAL		0x00000002L	/* left-to-right line	*/
#define LINE_VERTICAL		0x00000004L	/* top-to-bottom line	*/

/* lines cutting across a corner, connecting adjacent sides */
#define CORNER_NORTHEAST	0x00000008L	/* upper right corner	*/
#define CORNER_SOUTHEAST	0x00000010L	/* lower right corner	*/
#define CORNER_SOUTHWEST	0x00000020L	/* lower left corner	*/
#define CORNER_NORTHWEST	0x00000040L	/* upper left corner	*/

/* diagonal lines through the center */
#define DIAG_NEtoSW		0x00000080L	/* northeast to southwest */
#define DIAG_SEtoNW		0x00000100L	/* southeast to northwest */

/* 135 degree angle side-to-far-corner lines */
#define BENT_NtoSE		0x00000200L	/* north to southeast	*/
#define BENT_NtoSW		0x00000400L	/* north to southwest	*/
#define BENT_EtoSW		0x00000800L	/* east to southwest	*/
#define BENT_EtoNW		0x00001000L	/* east to northwest	*/
#define BENT_StoNW		0x00002000L	/* south to northwest	*/
#define BENT_StoNE		0x00004000L	/* south to northeast	*/
#define BENT_WtoNE		0x00008000L	/* west to northeast	*/
#define BENT_WtoSE		0x00010000L	/* west to southeast	*/

/* 90 degree corner-to-adjacent-corner lines */
#define ANGLE_NEtoSE		0x00020000L	/* northeast to southeast */
#define ANGLE_SEtoSW		0x00040000L	/* southeast to southwest */
#define ANGLE_SWtoNW		0x00080000L	/* southwest to northwest */
#define ANGLE_NWtoNE		0x00100000L	/* northwest to northeast */

/* 45 degree angle side-to-near-corner lines */
#define SHARP_NtoNE		0x00200000L	/* north to northeast	*/
#define SHARP_EtoNE		0x00400000L	/* east to northeast	*/
#define SHARP_EtoSE		0x00800000L	/* east to southeast	*/
#define SHARP_StoSE		0x01000000L	/* south to southeast	*/
#define SHARP_StoSW		0x02000000L	/* south to southwest	*/
#define SHARP_WtoSW		0x04000000L	/* west to southwest	*/
#define SHARP_WtoNW		0x08000000L	/* west to northwest	*/
#define SHARP_NtoNW		0x10000000L	/* north to northwest	*/

/* directions the cell can be reached from (point to previous cell) */
#define FROM_NOWHERE		0
#define FROM_NORTH		1
#define FROM_NORTHEAST		2
#define FROM_EAST		3
#define FROM_SOUTHEAST		4
#define FROM_SOUTH		5
#define FROM_SOUTHWEST		6
#define FROM_WEST		7
#define FROM_NORTHWEST		8
#define FROM_OTHERSIDE		9

