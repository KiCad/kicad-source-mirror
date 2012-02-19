/******************/
/*  pad_shapes.h  */
/******************/

#ifndef PAD_SHAPES_H_
#define PAD_SHAPES_H_

/**
 * Enum PAD_SHAPE_T
 * is the set of pad shapes, used with D_PAD::{Set,Get}Shape()
 */
enum PAD_SHAPE_T
{
    PAD_NONE,
    PAD_CIRCLE,
    PAD_ROUND = PAD_CIRCLE,
    PAD_RECT,
    PAD_OVAL,
    PAD_TRAPEZOID,
    PAD_RRECT,
    PAD_OCTAGON,
    PAD_SQUARE,
};


/**
 * Enum PAD_ATTR_T
 * is the set of pad shapes, used with D_PAD::{Set,Get}Attribute()
 */
enum PAD_ATTR_T
{
    PAD_STANDARD,           ///< Usual pad
    PAD_SMD,                ///< Smd pad, appears on the solder paste layer (default)
    PAD_CONN,               ///< Like smd, does not appear on the solder paste layer (default)
    PAD_HOLE_NOT_PLATED,    ///< like PAD_STANDARD, but not plated
                            ///< mechanical use only, no connection allowed
};


#endif  // PAD_SHAPES_H_
