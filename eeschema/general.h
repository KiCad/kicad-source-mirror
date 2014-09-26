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
#define DANGLING_SYMBOL_SIZE 12

///< The thickness to draw busses that do not have a specific width
//</ (can be changed in preference menu)
#define DEFAULTBUSTHICKNESS 12

///< The thickness to draw lines that thickness is set to 0 (default thickness)
///< (can be changed in preference menu)
#define DEFAULTDRAWLINETHICKNESS 6

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULTPINLENGTH 200

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULTPINNUMSIZE 50

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULTPINNAMESIZE 50

#define GR_DEFAULT_DRAWMODE GR_COPY

// this enum is for color management
// Using here "LAYER" in name is due to historical reasons.
// Eeschema does not actually use layers. It just uses "LAYER_XX" as identifier
// mainly for item color
typedef enum {
    LAYER_FIRST,
    LAYER_WIRE = LAYER_FIRST,
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
    LAYER_BACKGROUND,
    LAYERSCH_ID_COUNT
} LAYERSCH_ID;

inline LAYERSCH_ID operator++( LAYERSCH_ID& a )
{
    a = LAYERSCH_ID( int( a ) + 1 );
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
 * Default size for text in general
 */
int GetDefaultTextSize();
void SetDefaultTextSize( int aSize );

/**
 * Default line thickness used to draw/plot busses.
 */
int GetDefaultBusThickness();
void SetDefaultBusThickness( int aThickness );

EDA_COLOR_T GetLayerColor( LAYERSCH_ID aLayer );
void        SetLayerColor( EDA_COLOR_T aColor, LAYERSCH_ID aLayer );

// Color to draw selected items
EDA_COLOR_T GetItemSelectedColor();

// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema
EDA_COLOR_T GetInvisibleItemColor();

#endif    // _GENERAL_H_
