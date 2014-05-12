/**
 * @file general.h
 */

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <colors.h>     // for EDA_COLOR_T

class TRANSFORM;
class SCH_SHEET;

#define EESCHEMA_VERSION 2
#define SCHEMATIC_HEAD_STRING "Schematic File Version"

#define TXTMARGE 10             // Offset in mils for placement of labels and pin numbers
#define DEFAULT_TEXT_SIZE   50  // Default size for field texts
#define DANGLING_SYMBOL_SIZE 12

#define GR_DEFAULT_DRAWMODE GR_COPY

// this enum is for color management
typedef enum {
    LAYER_WIRE,
    LAYER_BUS,
    LAYER_JUNCTION,
    LAYER_LOCLABEL,
    LAYER_GLOBLABEL,
    LAYER_HIERLABEL,
    LAYER_PINNUM,
    LAYER_PINNAM,
    LAYER_REFERENCEPART,
    LAYER_VALUEPART,
    LAYER_FIELDS,
    LAYER_DEVICE,
    LAYER_NOTES,
    LAYER_NETNAM,
    LAYER_PIN,
    LAYER_SHEET,
    LAYER_SHEETNAME,
    LAYER_SHEETFILENAME,
    LAYER_SHEETLABEL,
    LAYER_NOCONNECT,
    LAYER_ERC_WARN,
    LAYER_ERC_ERR,
    LAYER_DEVICE_BACKGROUND,
    LAYER_GRID,
    NB_SCH_LAYERS
} LayerNumber;

inline LayerNumber operator++( LayerNumber& a )
{
    a = LayerNumber( int( a ) + 1 );
    return a;
}


/* Rotation, mirror of graphic items in components bodies are handled by a
 * transform matrix.  The default matrix is useful to draw lib entries with
 * using this default matrix ( no rotation, no mirror but Y axis is bottom to top, and
 * Y draw axis is to to bottom so we must have a default matix that reverses
 * the Y coordinate and keeps the X coordiate
 */
extern TRANSFORM DefaultTransform;

extern wxSize   g_RepeatStep;
extern int      g_RepeatDeltaLabel;

/* First and main (root) screen */
extern SCH_SHEET*   g_RootSheet;

/**
 * Default line thickness used to draw/plot items having a
 * default thickness line value (i.e. = 0 ).
 */
int GetDefaultLineThickness();
void SetDefaultLineThickness( int aThickness );

/**
 * Default length for new pins in module editor
 */
int GetDefaultPinLength();
void SetDefaultPinLength( int aLength );

/**
 * Default line thickness used to draw/plot busses.
 */
int GetDefaultBusThickness();
void SetDefaultBusThickness( int aThickness );

EDA_COLOR_T GetLayerColor( LayerNumber aLayer );

// Color to draw selected items
EDA_COLOR_T GetItemSelectedColor();

// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema
EDA_COLOR_T GetInvisibleItemColor();

void        SetLayerColor( EDA_COLOR_T aColor, int aLayer );

#endif    // _GENERAL_H_
