/******************/
/*  pad_shapes.h  */
/******************/

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
#define PAD_SMD         1       // Smd pad, appears on the solder paste layer (default)
#define PAD_CONN        2       // Like smd, does not appear on the solder paste layer (default)
#define PAD_HOLE_NOT_PLATED 3   // like PAD_STANDARD, but not plated
                                // mechanical used only
                                // no connection allowed


#endif  /* #ifndef PAD_SHAPES_H_ */
