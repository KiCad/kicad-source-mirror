/*********************************************/
/* Code for handling schematic sheet labels. */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "program.h"
#include "general.h"
#include "protos.h"


/************************/
/* class SCH_TEXT */
/* class SCH_LABEL */
/* class SCH_GLOBALLABEL */
/* class SCH_HIERLABEL */
/************************/

/* Names of sheet label types. */
const char* SheetLabelType[] =
{
    "Input",
    "Output",
    "BiDi",
    "3State",
    "UnSpc",
    "?????"
};

/* Coding polygons for global symbol graphic shapes.
 *  the first parml is the number of corners
 *  others are the corners coordinates in reduced units
 *  the real coordinate is the reduced coordinate * text half size
 */
static int  TemplateIN_HN[] = { 6, 0, 0, -1, -1, -2, -1, -2, 1, -1, 1, 0, 0 };
static int  TemplateIN_HI[] = { 6, 0, 0, 1, 1, 2, 1, 2, -1, 1, -1, 0, 0 };
static int  TemplateIN_UP[] = { 6, 0, 0, 1, -1, 1, -2, -1, -2, -1, -1, 0, 0 };
static int  TemplateIN_BOTTOM[] = { 6, 0, 0, 1, 1, 1, 2, -1, 2, -1, 1, 0, 0 };

static int  TemplateOUT_HN[] = { 6, -2, 0, -1, 1, 0, 1, 0, -1, -1, -1, -2, 0 };
static int  TemplateOUT_HI[] = { 6, 2, 0, 1, -1, 0, -1, 0, 1, 1, 1, 2, 0 };
static int  TemplateOUT_UP[] = { 6, 0, -2, 1, -1, 1, 0, -1, 0, -1, -1, 0, -2 };
static int  TemplateOUT_BOTTOM[] = { 6, 0, 2, 1, 1, 1, 0, -1, 0, -1, 1, 0, 2 };

static int  TemplateUNSPC_HN[] = { 5, 0, -1, -2, -1, -2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_HI[] = { 5, 0, -1, 2, -1, 2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_UP[] = { 5, 1, 0, 1, -2, -1, -2, -1, 0, 1, 0 };
static int  TemplateUNSPC_BOTTOM[] = { 5, 1, 0, 1, 2, -1, 2, -1, 0, 1, 0 };

static int  TemplateBIDI_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  TemplateBIDI_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  TemplateBIDI_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  TemplateBIDI_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int  Template3STATE_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  Template3STATE_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  Template3STATE_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  Template3STATE_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int* TemplateShape[5][4] =
{
    { TemplateIN_HN,     TemplateIN_UP,     TemplateIN_HI,
      TemplateIN_BOTTOM         },
    { TemplateOUT_HN,    TemplateOUT_UP,    TemplateOUT_HI,
      TemplateOUT_BOTTOM        },
    { TemplateBIDI_HN,   TemplateBIDI_UP,   TemplateBIDI_HI,
      TemplateBIDI_BOTTOM       },
    { Template3STATE_HN, Template3STATE_UP, Template3STATE_HI,
      Template3STATE_BOTTOM     },
    { TemplateUNSPC_HN,  TemplateUNSPC_UP,  TemplateUNSPC_HI,
      TemplateUNSPC_BOTTOM      }
};


/**************************************************************************/
SCH_TEXT::SCH_TEXT( const wxPoint& pos, const wxString& text,
                    KICAD_T aType ) :
    SCH_ITEM( NULL, aType ), EDA_TextStruct( text )
{
/**************************************************************************/
    m_Layer = LAYER_NOTES;
    m_Pos   = pos;
    m_Shape = 0;
    m_IsDangling = FALSE;
    m_MultilineAllowed     = true;
    m_SchematicOrientation = 0;
}


/** Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
bool SCH_TEXT::HitTest( const wxPoint& aPosRef )
{
    return TextHitTest( aPosRef );
}


/*********************************************/
SCH_TEXT* SCH_TEXT::GenCopy()
{
/*********************************************/
    SCH_TEXT* newitem;

    switch( Type() )
    {
    default:
    case TYPE_SCH_TEXT:
        newitem = new SCH_TEXT( m_Pos, m_Text );
        break;

    case TYPE_SCH_GLOBALLABEL:
        newitem = new SCH_GLOBALLABEL( m_Pos, m_Text );
        break;

    case TYPE_SCH_HIERLABEL:
        newitem = new SCH_HIERLABEL( m_Pos, m_Text );
        break;

    case TYPE_SCH_LABEL:
        newitem = new SCH_LABEL( m_Pos, m_Text );
        break;
    }

    newitem->m_Layer      = m_Layer;
    newitem->m_Shape      = m_Shape;
    newitem->m_Orient     = m_Orient;
    newitem->m_Size       = m_Size;
    newitem->m_Width      = m_Width;
    newitem->m_HJustify   = m_HJustify;
    newitem->m_VJustify   = m_VJustify;
    newitem->m_IsDangling = m_IsDangling;
    newitem->m_Italic     = m_Italic;
    newitem->m_Bold       = m_Bold;
    newitem->m_SchematicOrientation = m_SchematicOrientation;

    return newitem;
}


/** function GetSchematicTextOffset (virtual)
 * @return the offset between the SCH_TEXT position and the text itself
 * position
 * This offset depend on orientation, and the type of text
 * (room to draw an associated graphic symbol, or put the text above a wire)
 */
wxPoint SCH_TEXT::GetSchematicTextOffset()
{
    wxPoint text_offset;

    // add a small offset (TXTMARGE) to x ( or y) position to allow a text to
    // be on a wire or a line and be readable
    switch( m_SchematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation (left justified) */
        text_offset.y = -TXTMARGE;
        break;

    case 1: /* Vert Orientation UP */
        text_offset.x = -TXTMARGE;
        break;

    case 2: /* Horiz Orientation - Right justified */
        text_offset.y = -TXTMARGE;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        text_offset.x = -TXTMARGE;
        break;
    }

    return text_offset;
}


bool SCH_TEXT::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    return SCH_ITEM::Matches( m_Text, aSearchData );
}


/** function GetSchematicTextOffset (virtual)
 * @return the offset between the SCH_TEXT position and the text itself
 * position
 * This offset depend on orientation, and the type of text
 * (room to draw an associated graphic symbol, or put the text above a wire)
 */
wxPoint SCH_LABEL::GetSchematicTextOffset()
{
    return SCH_TEXT::GetSchematicTextOffset();
}


/** virtual function Mirror_Y
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_TEXT::Mirror_Y( int aYaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int px = m_Pos.x;
    int dx;

    switch( GetSchematicTextOrientation() )
    {
    case 0:             /* horizontal text */
        dx = LenSize( m_Text ) / 2;
        break;

    case 1: /* Vert Orientation UP */
        dx = -m_Size.x / 2;
        break;

    case 2:        /* invert horizontal text*/
        dx = -LenSize( m_Text ) / 2;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        dx = m_Size.x / 2;
        break;

    default:
        dx = 0;
        break;
    }

    px += dx;
    px -= aYaxis_position;
    NEGATE( px );
    px += aYaxis_position;
    px -= dx;
    m_Pos.x = px;
}


/** virtual function Mirror_X
 * mirror item relative to an X axis
 * @param aXaxis_position = the x axis position
 */
void SCH_TEXT::Mirror_X( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int py = m_Pos.y;
    int dy;

    switch( GetSchematicTextOrientation() )
    {
    case 0:             /* horizontal text */
        dy = -m_Size.y / 2;
        break;

    case 1: /* Vert Orientation UP */
        dy = -LenSize( m_Text ) / 2;
        break;

    case 2:                 /* invert horizontal text*/
        dy = m_Size.y / 2;  // how to calculate text height?
        break;

    case 3: /*  Vert Orientation BOTTOM */
        dy = LenSize( m_Text ) / 2;
        break;

    default:
        dy = 0;
        break;
    }

    py += dy;
    py -= aXaxis_position;
    NEGATE( py );
    py += aXaxis_position;
    py -= dy;
    m_Pos.y = py;
}


void SCH_TEXT::Rotate( wxPoint rotationPoint )
{
    int dy;

    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetSchematicTextOrientation( (GetSchematicTextOrientation() + 1) % 4 );
    switch( GetSchematicTextOrientation() )
    {
    case 0:             /* horizontal text */
        dy = m_Size.y;
        break;

    case 1: /* Vert Orientation UP */
        dy = 0;
        break;

    case 2:        /* invert horizontal text*/
        dy = m_Size.y;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        dy = 0;
        break;

    default:
        dy = 0;
        break;
    }

    m_Pos.y += dy;
}


/** function GetSchematicTextOffset (virtual)
 * @return the offset between the SCH_TEXT position and the text itself
 * position
 * This offset depend on orientation, and the type of text
 * (room to draw an associated graphic symbol, or put the text above a wire)
 */
wxPoint SCH_HIERLABEL::GetSchematicTextOffset()
{
    wxPoint text_offset;

    int     width = MAX( m_Width, g_DrawDefaultLineThickness );

    int     ii = m_Size.x + TXTMARGE + width;

    switch( m_SchematicOrientation )
    {
    case 0:             /* Orientation horiz normale */
        text_offset.x = -ii;
        break;

    case 1:             /* Orientation vert UP */
        text_offset.y = -ii;
        break;

    case 2:             /* Orientation horiz inverse */
        text_offset.x = ii;
        break;

    case 3:             /* Orientation vert BOTTOM */
        text_offset.y = ii;
        break;
    }

    return text_offset;
}


/** virtual function Mirror_Y
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_HIERLABEL::Mirror_Y( int aYaxis_position )
{
/* The hierarchical label is NOT really mirrored.
 *   for an horizontal label, the schematic orientation is changed.
 *   for a vericalal label, the schematic orientation is not changed.
 *   and the label is moved to a suitable position
 */

    switch( GetSchematicTextOrientation() )
    {
    case 0:             /* horizontal text */
        SetSchematicTextOrientation( 2 );
        break;

    case 2:        /* invert horizontal text*/
        SetSchematicTextOrientation( 0 );
        break;
    }

    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_HIERLABEL::Mirror_X( int aXaxis_position )
{
    switch( GetSchematicTextOrientation() )
    {
    case 1:             /* vertical text */
        SetSchematicTextOrientation( 3 );
        break;

    case 3:        /* invert vertical text*/
        SetSchematicTextOrientation( 1 );
        break;
    }

    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_HIERLABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetSchematicTextOrientation( (GetSchematicTextOrientation() + 3) % 4 );
}


/** virtual function Mirror_Y
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_GLOBALLABEL::Mirror_Y( int aYaxis_position )
{
    /* The global label is NOT really mirrored.
     *  for an horizontal label, the schematic orientation is changed.
     *  for a vericalal label, the schematic orientation is not changed.
     *  and the label is moved to a suitable position
     */
    switch( GetSchematicTextOrientation() )
    {
    case 0:             /* horizontal text */
        SetSchematicTextOrientation( 2 );
        break;

    case 2:        /* invert horizontal text*/
        SetSchematicTextOrientation( 0 );
        break;
    }

    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_GLOBALLABEL::Mirror_X( int aXaxis_position )
{
    switch( GetSchematicTextOrientation() )
    {
    case 1:             /* vertical text */
        SetSchematicTextOrientation( 3 );
        break;

    case 3:        /* invert vertical text*/
        SetSchematicTextOrientation( 1 );
        break;
    }

    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_GLOBALLABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetSchematicTextOrientation( (GetSchematicTextOrientation() + 3) % 4 );
}


/** function GetSchematicTextOffset (virtual)
 * @return the offset between the SCH_TEXT position and the text itself
 * position
 * This offset depend on orientation, and the type of text
 * (room to draw an associated graphic symbol, or put the text above a wire)
 */
wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset()
{
    wxPoint text_offset;
    int     width = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    width = Clamp_Text_PenSize( width, m_Size, m_Bold );
    int     HalfSize = m_Size.x / 2;
    int     offset   = width;

    switch( m_Shape )
    {
    case NET_INPUT:
    case NET_BIDI:
    case NET_TRISTATE:
        offset += HalfSize;
        break;

    case NET_OUTPUT:
    case NET_UNSPECIFIED:
        offset += TXTMARGE;
        break;

    default:
        break;
    }

    switch( m_SchematicOrientation )
    {
    case 0:             /* Orientation horiz normal */
        text_offset.x -= offset;
        break;

    case 1:             /* Orientation vert UP */
        text_offset.y -= offset;
        break;

    case 2:             /* Orientation horiz inverse */
        text_offset.x += offset;
        break;

    case 3:             /* Orientation vert BOTTOM */
        text_offset.y += offset;
        break;
    }

    return text_offset;
}


/** function SetTextOrientAndJustifyParmeters (virtual)
 * Set m_SchematicOrientation, and initialize
 * m_orient,m_HJustified and m_VJustified, according to the value of
 * m_SchematicOrientation
 * must be called after changing m_SchematicOrientation
 * @param aSchematicOrientation =
 *  0 = normal (horizontal, left justified).
 *  1 = up (vertical)
 *  2 = (horizontal, right justified). This can be seen as the mirrored
 *      position of 0
 *  3 = bottom . This can be seen as the mirrored position of up
 */
void SCH_TEXT::SetSchematicTextOrientation( int aSchematicOrientation )
{
    m_SchematicOrientation = aSchematicOrientation;

    switch( m_SchematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation (left justified) */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 2: /* Horiz Orientation - Right justified */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;
    }
}


/** function SetTextOrientAndJustifyParmeters
 * Set m_SchematicOrientation, and initialize
 * m_orient,m_HJustified and m_VJustified, according to the value of
 * m_SchematicOrientation (for a label)
 * must be called after changing m_SchematicOrientation
 * @param aSchematicOrientation =
 *  0 = normal (horizontal, left justified).
 *  1 = up (vertical)
 *  2 = (horizontal, right justified). This can be seen as the mirrored
 *      position of 0
 *  3 = bottom . This can be seen as the mirrored position of up
 */
void SCH_LABEL::SetSchematicTextOrientation( int aSchematicOrientation )
{
    SCH_TEXT::SetSchematicTextOrientation( aSchematicOrientation );
}


/** function SetTextOrientAndJustifyParmeters
 * Set m_SchematicOrientation, and initialize
 * m_orient,m_HJustified and m_VJustified, according to the value of
 * m_SchematicOrientation
 * must be called after changing m_SchematicOrientation
 * @param aSchematicOrientation =
 *  0 = normal (horizontal, left justified).
 *  1 = up (vertical)
 *  2 = (horizontal, right justified). This can be seen as the mirrored
 *      position of 0
 *  3 = bottom . This can be seen as the mirrored position of up
 */
void SCH_GLOBALLABEL::SetSchematicTextOrientation( int aSchematicOrientation )
{
    m_SchematicOrientation = aSchematicOrientation;

    switch( m_SchematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 2: /* Horiz Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;
    }
}


/** function SetTextOrientAndJustifyParmeters
 * Set m_SchematicOrientation, and initialize
 * m_orient,m_HJustified and m_VJustified, according to the value of
 * m_SchematicOrientation
 * must be called after changing m_SchematicOrientation
 * @param aSchematicOrientation =
 *  0 = normal (horizontal, left justified).
 *  1 = up (vertical)
 *  2 = (horizontal, right justified). This can be seen as the mirrored
 *      position of 0
 *  3 = bottom . This can be seen as the mirrored position of up
 */
void SCH_HIERLABEL::SetSchematicTextOrientation( int aSchematicOrientation )
{
    m_SchematicOrientation = aSchematicOrientation;

    switch( m_SchematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 2: /* Horiz Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;
    }
}


/********************************************************/
void SCH_TEXT::SwapData( SCH_TEXT* copyitem )
{
/********************************************************/
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Width, copyitem->m_Width );
    EXCHG( m_Shape, copyitem->m_Shape );
    EXCHG( m_Orient, copyitem->m_Orient );

    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
    EXCHG( m_IsDangling, copyitem->m_IsDangling );
    EXCHG( m_SchematicOrientation, copyitem->m_SchematicOrientation );
}


/***************************************************************/
void SCH_TEXT::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
/***************************************************************/
    /* save old text in undo list */
    if( g_ItemToUndoCopy && ( (m_Flags & IS_NEW) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (SCH_TEXT*) g_ItemToUndoCopy );

        /* save in undo list */
        frame->SaveCopyInUndoList( this, UR_CHANGED );

        /* restore new values */
        SwapData( (SCH_TEXT*) g_ItemToUndoCopy );

        SAFE_DELETE( g_ItemToUndoCopy );
    }

    SCH_ITEM::Place( frame, DC );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_TEXT::GetPenSize()
{
    int pensize = m_Width;

    if( pensize == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            pensize = GetPenSizeForBold( m_Size.x );
        else
            pensize = g_DrawDefaultLineThickness;
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, m_Size, m_Bold );
    return pensize;
}


/****************************************************************************/
void SCH_TEXT::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& aOffset,
                     int DrawMode, int Color )
{
/****************************************************************************/

/* Text type Comment (text on layer "NOTE") have 4 directions, and the Text
 * origin is the first letter
 */
    EDA_Colors color;
    int        linewidth =
        (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    wxPoint text_offset = aOffset + GetSchematicTextOffset();

    EXCHG( linewidth, m_Width );            // Set the minimum width
    EDA_TextStruct::Draw( panel, DC, text_offset, color, DrawMode, FILLED,
                          UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Width );            // set initial value
    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_Rect BoundaryBox;
        BoundaryBox = GetBoundingBox();
        int      x1 = BoundaryBox.GetX();
        int      y1 = BoundaryBox.GetY();
        int      x2 = BoundaryBox.GetRight();
        int      y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    }
#endif
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_TEXT::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    wxString text = m_Text;

    for( ; ; )
    {
        int i = text.find( '\n' );
        if( i==wxNOT_FOUND )
            break;

        text.erase( i, 1 );
        text.insert( i, wxT( "\\n" ) );
    }

    if( fprintf( aFile, "Text Notes %-4d %-4d %-4d %-4d %s %d\n%s\n",
                m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
                shape, m_Width, CONV_TO_UTF8( text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


#if defined(DEBUG)

void SCH_TEXT::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " shape=\"" << m_Shape << '"'
                                 << " dangling=\"" << m_IsDangling << '"'
                                 << '>'
                                 << CONV_TO_UTF8( m_Text )
                                 << "</" << s.Lower().mb_str() << ">\n";
}


#endif

/****************************************************************************/
SCH_LABEL::SCH_LABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, TYPE_SCH_LABEL )
{
/****************************************************************************/
    m_Layer = LAYER_LOCLABEL;
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


/** virtual function Mirror_X
 * mirror item relative to an X axis
 * @param aXaxis_position = the x axis position
 */
void SCH_LABEL::Mirror_X( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int py = m_Pos.y;

    py -= aXaxis_position;
    NEGATE( py );
    py += aXaxis_position;
    m_Pos.y = py;
}


void SCH_LABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetSchematicTextOrientation( (GetSchematicTextOrientation() + 1) % 4 );
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_LABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text Label %-4d %-4d %-4d %-4d %s %d\n%s\n",
                m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x, shape,
                m_Width, CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/*****************************************************************************/
SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, TYPE_SCH_GLOBALLABEL )
{
/*****************************************************************************/
    m_Layer = LAYER_GLOBLABEL;
    m_Shape = NET_BIDI;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_GLOBALLABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";
    if( fprintf( aFile, "Text GLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
                m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
                SheetLabelType[m_Shape], shape, m_Width,
                CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/************************************************/
bool SCH_GLOBALLABEL::HitTest( const wxPoint& aPosRef )
{
/************************************************/

/** Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
    EDA_Rect rect = GetBoundingBox();

    return rect.Inside( aPosRef );
}


/*****************************************************************************/
SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_TEXT( pos, text, aType )
{
/*****************************************************************************/
    m_Layer = LAYER_HIERLABEL;
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_HIERLABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";
    if( fprintf( aFile, "Text HLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
                m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
                SheetLabelType[m_Shape], shape, m_Width,
                CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/************************************************/
bool SCH_HIERLABEL::HitTest( const wxPoint& aPosRef )
{
/************************************************/

/** Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
    EDA_Rect rect = GetBoundingBox();

    return rect.Inside( aPosRef );
}


/** Function SCH_LABEL::Draw
 * a label is drawn like a text. So just call SCH_TEXT::Draw
 */
void SCH_LABEL::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                      int DrawMode, int Color )
{
    SCH_TEXT::Draw( panel, DC, offset, DrawMode, Color );
}


/*****************************************************************************/
void SCH_HIERLABEL::Draw( WinEDA_DrawPanel* panel,
                          wxDC*             DC,
                          const wxPoint&    offset,
                          int               DrawMode,
                          int               Color )
{
/*****************************************************************************/

/* Hierarchical Label have a text and a graphic icon.
 * Texts type have 4 directions, and the text origin is the graphic icon
 */
    static std::vector <wxPoint> Poly;
    EDA_Colors color;
    int        linewidth =
        ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    EXCHG( linewidth, m_Width );            // Set the minimum width
    wxPoint text_offset = offset + GetSchematicTextOffset();
    EDA_TextStruct::Draw( panel, DC, text_offset, color, DrawMode, FILLED,
                          UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Width );            // set initial value

    CreateGraphicShape( Poly, m_Pos + offset );
    GRPoly( &panel->m_ClipBox, DC, Poly.size(), &Poly[0], 0, linewidth,
            color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_Rect BoundaryBox;
        BoundaryBox = GetBoundingBox();
        int      x1 = BoundaryBox.GetX();
        int      y1 = BoundaryBox.GetY();
        int      x2 = BoundaryBox.GetRight();
        int      y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    }
#endif
}


/** Function CreateGraphicShape
 * calculates the graphic shape (a polygon) associated to the text
 * @param aCorner_list = a buffer to fill with polygon corners coordinates
 * @param Pos = Postion of the shape
 */
void SCH_HIERLABEL::CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint&         Pos )
{
    int* Template = TemplateShape[m_Shape][m_SchematicOrientation];
    int  HalfSize = m_Size.x / 2;

    int  imax = *Template; Template++;

    aCorner_list.clear();
    for( int ii = 0; ii < imax; ii++ )
    {
        wxPoint corner;
        corner.x = ( HalfSize * (*Template) ) + Pos.x;
        Template++;

        corner.y = ( HalfSize * (*Template) ) + Pos.y;
        Template++;

        aCorner_list.push_back( corner );
    }
}

/** Virtual Function SCH_SHEET_PIN::CreateGraphicShape
 * calculates the graphic shape (a polygon) associated to the text
 * @param aCorner_list = a buffer to fill with polygon corners coordinates
 * @param aPos = Position of the shape
 */
void SCH_SHEET_PIN::CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint&         aPos )
{
     /* This is the same icon shapes as SCH_HIERLABEL
     * but the graphic icon is slightly different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    int tmp = m_Shape;
    switch( m_Shape )
    {
    case NET_INPUT:
        m_Shape = NET_OUTPUT;
        break;

    case NET_OUTPUT:
        m_Shape = NET_INPUT;
        break;

    default:
        break;
    }
    SCH_HIERLABEL::CreateGraphicShape( aCorner_list, aPos );
    m_Shape = tmp;
}

/****************************************/
EDA_Rect SCH_HIERLABEL::GetBoundingBox()
{
/****************************************/
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    height = m_Size.y + width + 2 * TXTMARGE;
    length = LenSize( m_Text )
             + height                 // add height for triangular shapes
             + 2 * DANGLING_SYMBOL_SIZE;

    switch( m_SchematicOrientation )    // respect orientation
    {
    case 0:                             /* Horiz Normal Orientation (left
                                         *justified) */
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     /* Vert Orientation UP */
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
        dx = height;
        dy = length;
        x -= height / 2;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_Rect box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


/*****************************************************************************/
void SCH_GLOBALLABEL::Draw( WinEDA_DrawPanel* panel,
                            wxDC*             DC,
                            const wxPoint&    aOffset,
                            int               DrawMode,
                            int               Color )
{
/*****************************************************************************/

/* Texts type Global Label  have 4 directions, and the Text origin is the
 * graphic icon
 */
    static std::vector <wxPoint> Poly;
    EDA_Colors color;
    wxPoint    text_offset = aOffset + GetSchematicTextOffset();


    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    int linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );
    EXCHG( linewidth, m_Width );            // Set the minimum width
    EDA_TextStruct::Draw( panel, DC, text_offset, color, DrawMode, FILLED,
                          UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Width );            // set initial value

    CreateGraphicShape( Poly, m_Pos + aOffset );
    GRPoly( &panel->m_ClipBox, DC, Poly.size(), &Poly[0], 0, linewidth,
            color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_Rect BoundaryBox;
        BoundaryBox = GetBoundingBox();
        int      x1 = BoundaryBox.GetX();
        int      y1 = BoundaryBox.GetY();
        int      x2 = BoundaryBox.GetRight();
        int      y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    }
#endif
}


/** function CreateGraphicShape
 * Calculates the graphic shape (a polygon) associated to the text
 * @param aCorner_list = a buffer to fill with polygon corners coordinates
 * @param Pos = Position of the shape
 */
void SCH_GLOBALLABEL::CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                          const wxPoint&         Pos )
{
    int HalfSize  = m_Size.y / 2;
    int linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    aCorner_list.clear();

    int symb_len = LenSize( m_Text ) + ( TXTMARGE * 2 );

    // Create outline shape : 6 points
    int x = symb_len + linewidth + 3;

    // 50% more for negation bar
    int y = wxRound( (double) HalfSize * 1.5 + (double) linewidth + 3.0 );

    // Starting point(anchor)
    aCorner_list.push_back( wxPoint( 0, 0 ) );
    aCorner_list.push_back( wxPoint( 0, -y ) );     // Up
    aCorner_list.push_back( wxPoint( -x, -y ) );    // left
    aCorner_list.push_back( wxPoint( -x, 0 ) );     // Up left
    aCorner_list.push_back( wxPoint( -x, y ) );     // left down
    aCorner_list.push_back( wxPoint( 0, y ) );      // down

    int x_offset = 0;

    switch( m_Shape )
    {
    case NET_INPUT:
        x_offset = -HalfSize;
        aCorner_list[0].x += HalfSize;
        break;

    case NET_OUTPUT:
        aCorner_list[3].x -= HalfSize;
        break;

    case NET_BIDI:
    case NET_TRISTATE:
        x_offset = -HalfSize;
        aCorner_list[0].x += HalfSize;
        aCorner_list[3].x -= HalfSize;
        break;

    case NET_UNSPECIFIED:
    default:
        break;
    }

    int angle = 0;

    switch( m_SchematicOrientation )
    {
    case 0:             /* Orientation horiz normal */
        break;

    case 1:             /* Orientation vert UP */
        angle = -900;
        break;

    case 2:             /* Orientation horiz inverse */
        angle = 1800;
        break;

    case 3:             /* Orientation vert BOTTOM */
        angle = 900;
        break;
    }

    // Rotate outlines and move corners in real position
    for( unsigned ii = 0; ii < aCorner_list.size(); ii++ )
    {
        aCorner_list[ii].x += x_offset;
        if( angle )
            RotatePoint( &aCorner_list[ii], angle );
        aCorner_list[ii] += Pos;
    }

    aCorner_list.push_back( aCorner_list[0] ); // closing
}


/******************************************/
EDA_Rect SCH_GLOBALLABEL::GetBoundingBox()
{
/******************************************/
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    height = ( (m_Size.y * 15) / 10 ) + width + 2 * TXTMARGE;

    // text X size add height for triangular shapes(bidirectional)
    length = LenSize( m_Text ) + height + DANGLING_SYMBOL_SIZE;

    switch( m_SchematicOrientation )    // respect orientation
    {
    case 0:                             /* Horiz Normal Orientation (left justified) */
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     /* Vert Orientation UP */
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
        dx = height;
        dy = length;
        x -= height / 2;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_Rect box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


/***********************************/
EDA_Rect SCH_LABEL::GetBoundingBox()
/***********************************/
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    int width = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    length = LenSize( m_Text );
    height = m_Size.y + width;
    dx     = dy = 0;

    switch( m_SchematicOrientation )
    {
    case 0:             /* Horiz Normal Orientation (left justified) */
        dx = 2 * DANGLING_SYMBOL_SIZE + length;
        dy = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        x -= DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 1:     /* Vert Orientation UP */
        dx = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        dy = -2 * DANGLING_SYMBOL_SIZE - length;
        x += DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = -2 * DANGLING_SYMBOL_SIZE - length;
        dy = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        x += DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
        dx = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        dy = 2 * DANGLING_SYMBOL_SIZE + length;
        x += DANGLING_SYMBOL_SIZE;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_Rect box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


/***********************************/
EDA_Rect SCH_TEXT::GetBoundingBox()
/***********************************/
{
    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    int linewidth =
        (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );
    EXCHG( linewidth, m_Width );            // Set the real width
    EDA_Rect rect = GetTextBox( -1 );
    EXCHG( linewidth, m_Width );            // set initial value

    if( m_Orient )                          // Rotate rect
    {
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();
        RotatePoint( &pos, m_Pos, m_Orient );
        RotatePoint( &end, m_Pos, m_Orient );
        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}
