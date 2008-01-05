

#ifndef PAD_SHAPES_H_
#define PAD_SHAPES_H_

/* Pad shape id : ( .m_PadShape member) */
#define	PAD_NONE        0
#define PAD_CIRCLE      1
#define PAD_ROUND       PAD_CIRCLE
#define PAD_RECT        2
#define PAD_OVAL        3
#define PAD_TRAPEZOID   4       // trapezoid
#define PAD_RRECT       5
#define PAD_OCTAGON     6
#define PAD_SQUARE      7


/* PADS attributes */
#define PAD_STANDARD    0       // Usual pad
#define PAD_SMD         1       // Smd pad, appears on the layer paste (default)
#define PAD_CONN        2       // Like smd, does not appear on the layer paste (default)
// reserved, but not yet really used:
#define PAD_P_HOLE      3       // trou simple, utile sur pad stack
#define PAD_MECA        4       // PAD "mecanique" (fixation, zone cuivre...)


#endif  // PAD_SHAPES_H_

