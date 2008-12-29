/***********************************************************************/
/* Methodes de base de gestion des classes des elements de schematique */
/***********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "trigo.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"


/************************/
/* class SCH_TEXT */
/* class SCH_LABEL */
/* class SCH_GLOBALLABEL */
/* class SCH_HIERLABEL */
/************************/

/**************************************************************************/
SCH_TEXT::SCH_TEXT( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_ITEM( NULL, aType )
    , EDA_TextStruct( text )
/**************************************************************************/
{
    m_Layer      = LAYER_NOTES;
    m_Pos        = pos;
    m_Shape      = 0;
    m_IsDangling = FALSE;
}


/*********************************************/
SCH_TEXT* SCH_TEXT::GenCopy()
/*********************************************/
{
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

    return newitem;
}


/********************************************************/
void SCH_TEXT::SwapData( SCH_TEXT* copyitem )
/********************************************************/
{
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
}


/***************************************************************/
void SCH_TEXT::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
/***************************************************************/
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy && ( (m_Flags & IS_NEW) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (SCH_TEXT*) g_ItemToUndoCopy );

        /* save in undo list */
        frame->SaveCopyInUndoList( this, IS_CHANGED );

        /* restore new values */
        SwapData( (SCH_TEXT*) g_ItemToUndoCopy );

        SAFE_DELETE( g_ItemToUndoCopy );
    }

    SCH_ITEM::Place( frame, DC );
}

/*******************************************************************************************/
void SCH_TEXT::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                           int DrawMode, int Color )
/*******************************************************************************************/

/* Texts type Comment (text on layer "NOTE") have 4 directions, and the Text origin is the first letter
 */
{
    EDA_Colors color;
    int width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = (EDA_Colors)Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    switch( m_Orient )
    {
    case 0: /* Horiz Normal Orientation (left justified) */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y - TXTMARGE + offset.y ),
                         color, m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_LEFT,
                         GR_TEXT_VJUSTIFY_BOTTOM, width, m_Italic );
        break;

    case 1: /* Vert Orientation UP */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x - TXTMARGE + offset.x,
                                  m_Pos.y + offset.y ),
                         color, m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT,
                         GR_TEXT_VJUSTIFY_BOTTOM, width, m_Italic );
        break;

    case 2: /* Horiz Orientation - Right justified */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y -
                                  TXTMARGE + offset.y ),
                         color, m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT,
                         GR_TEXT_VJUSTIFY_BOTTOM, width, m_Italic );
        break;

    case 3: /*  Vert Orientation BOTTOM */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x - TXTMARGE + offset.x,
                                  m_Pos.y + offset.y ),
                         color, m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT,
                         GR_TEXT_VJUSTIFY_TOP, width, m_Italic );
        break;
    }

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_TEXT::Save( FILE* aFile ) const
{
    bool success = true;
    const char * shape = "~";
    if (m_Italic )
        shape = "Italic";
    if( fprintf( aFile, "Text Notes %-4d %-4d %-4d %-4d %s %d\n%s\n",
            m_Pos.x, m_Pos.y,
            m_Orient, m_Size.x,
            shape, m_Width,
            CONV_TO_UTF8( m_Text ) ) == EOF )
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
/****************************************************************************/
{
    m_Layer      = LAYER_LOCLABEL;
    m_Shape      = NET_INPUT;
    m_IsDangling = TRUE;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_LABEL::Save( FILE* aFile ) const
{
    bool success = true;
    const char * shape = "~";
    if (m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text Label %-4d %-4d %-4d %-4d %s %d\n%s\n",
            m_Pos.x, m_Pos.y,
            m_Orient, m_Size.x, shape, m_Width,
            CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/***********************************************************************************/
SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, TYPE_SCH_GLOBALLABEL )
/***********************************************************************************/
{
    m_Layer      = LAYER_GLOBLABEL;
    m_Shape      = NET_BIDI;
    m_IsDangling = TRUE;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_GLOBALLABEL::Save( FILE* aFile ) const
{
    bool success = true;
    const char * shape = "~";
    if (m_Italic )
        shape = "Italic";
    if( fprintf( aFile, "Text GLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
            m_Pos.x, m_Pos.y,
            m_Orient, m_Size.x,
            SheetLabelType[m_Shape],
            shape, m_Width,
            CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/***********************************************************************************/
SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, TYPE_SCH_HIERLABEL )
/***********************************************************************************/
{
    m_Layer      = LAYER_HIERLABEL;
    m_Shape      = NET_INPUT;
    m_IsDangling = TRUE;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_HIERLABEL::Save( FILE* aFile ) const
{
    bool success = true;
    const char * shape = "~";
    if (m_Italic )
        shape = "Italic";
    if( fprintf( aFile, "Text HLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
            m_Pos.x, m_Pos.y,
            m_Orient, m_Size.x,
            SheetLabelType[m_Shape],
            shape, m_Width,
            CONV_TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


/*********************************************************************************************/
void SCH_LABEL::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                            int DrawMode, int Color )
/*********************************************************************************************/
{
    SCH_TEXT::Draw( panel, DC, offset, DrawMode, Color );
}


/*******************************************************************************************/
void SCH_HIERLABEL::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                int DrawMode, int Color )
/******************************************************************************************/

/* Texts type Global Label  have 4 directions, and the Text origin is the graphic icon
 */
{
    int     Poly[40];
    int     ii;
    EDA_Colors color;
    wxPoint AnchorPos = m_Pos + offset;;

    int     width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = (EDA_Colors)Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    ii = m_Size.x + TXTMARGE;

    switch( m_Orient )
    {
    case 0:             /* Orientation horiz normale */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x - ii, AnchorPos.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER, width, m_Italic );
        break;

    case 1:             /* Orientation vert UP */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x, AnchorPos.y + ii ), color,
                         m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, width, m_Italic );
        break;

    case 2:             /* Orientation horiz inverse */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x + ii, AnchorPos.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width, m_Italic );
        break;

    case 3:             /* Orientation vert BOTTOM */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x, AnchorPos.y - ii ), color,
                         m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM, width, m_Italic );
        break;
    }

    CreateGraphicShape( Poly, AnchorPos );
    GRPoly( &panel->m_ClipBox, DC, Poly[0], (wxPoint*)(Poly + 1), 0, width, color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );
}


/**
 * Function CreateGraphicShape
 * calculates the graphic shape (a polygon) associated to the text
 * @param corner_list = coordinates list fill with polygon corners ooordinates (size > 20)
 * @param Pos = Postion of the shape
 * format list is
 * corner_count, x0, y0, ... xn, yn
 */
void SCH_HIERLABEL::CreateGraphicShape( int* corner_list, const wxPoint& Pos )
{
    int* Template = TemplateShape[m_Shape][m_Orient];
    int  HalfSize = m_Size.x / 2;

    int  imax = *Template; Template++;

    *corner_list++ = imax;
    for( int ii = 0; ii < imax; ii++ )
    {
        *corner_list++ = ( HalfSize * (*Template) ) + Pos.x;
        Template++;

        *corner_list++ = ( HalfSize * (*Template) ) + Pos.y;
        Template++;
    }
}

EDA_Rect SCH_HIERLABEL::GetBoundingBox()
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    dx = dy = 0;

    int     width = MAX( m_Width, g_DrawMinimunLineWidth );
    height  = m_Size.y + 2*TXTMARGE;
    length = ( Pitch(width) * GetLength() ) + height + 2*DANGLING_SYMBOL_SIZE; // add height for triangular shapes

    switch( m_Orient ) // respect orientation
    {
        case 0: /* Horiz Normal Orientation (left justified) */
            dx = -length;
            dy = height;
            x += DANGLING_SYMBOL_SIZE;
            y -= height/2;
            break;

        case 1: /* Vert Orientation UP */
            dx = height;
            dy = length;
            x -= height/2;
            y -= DANGLING_SYMBOL_SIZE;
            break;

        case 2: /* Horiz Orientation - Right justified */
            dx = length;
            dy = height;
            x -= DANGLING_SYMBOL_SIZE;
            y -= height/2;
            break;

        case 3: /*  Vert Orientation BOTTOM */
            dx = height;
            dy = -length;
            x -= height/2;
            y += DANGLING_SYMBOL_SIZE;
            break;
    }

    EDA_Rect box(wxPoint(x,y), wxSize(dx,dy) );
    box.Normalize();
    return box;
}

/*******************************************************************************************/
void SCH_GLOBALLABEL::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& draw_offset,
                                  int DrawMode, int Color )
/******************************************************************************************/

/* Texts type Global Label  have 4 directions, and the Text origin is the graphic icon
 */
{
    int     Poly[20];
    int     offset;
    EDA_Colors color;
    int HalfSize;
    wxPoint AnchorPos = m_Pos + draw_offset;;

    int     width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = (EDA_Colors)Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    HalfSize = m_Size.x / 2;
    offset = width;

    switch( m_Shape )
    {
    case NET_INPUT:
    case NET_BIDI:
    case NET_TRISTATE:
        offset += HalfSize;
        break;

    case NET_OUTPUT:
        offset += TXTMARGE;
        break;

    default:
        break;
    }

    switch( m_Orient )
    {
    case 0:             /* Orientation horiz normale */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x - offset, AnchorPos.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER, width, m_Italic );
        break;

    case 1:             /* Orientation vert UP */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x, AnchorPos.y + offset ), color,
                         m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, width, m_Italic );
        break;

    case 2:             /* Orientation horiz inverse */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x + offset, AnchorPos.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, m_Size,
                         GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width, m_Italic );
        break;

    case 3:             /* Orientation vert BOTTOM */
        DrawGraphicText( panel, DC,
                         wxPoint( AnchorPos.x, AnchorPos.y - offset ), color,
                         m_Text, TEXT_ORIENT_VERT, m_Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM, width, m_Italic );
        break;
    }

    CreateGraphicShape( Poly, AnchorPos );
    GRPoly( &panel->m_ClipBox, DC, Poly[0], (wxPoint*) (Poly + 1), 0, width, color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, AnchorPos, color );
}


/** function CreateGraphicShape
 * Calculates the graphic shape (a polygon) associated to the text
 * @param corner_list = oordinates list fill with polygon corners ooordinates (size >= 14)
 * @param Pos = Postion of the shape
 * format list is
 * <corner_count>, x0, y0, ... xn, yn
 */
void SCH_GLOBALLABEL::CreateGraphicShape( int* corner_list, const wxPoint& Pos )
{
    int HalfSize = m_Size.x / 2;
    int     width = MAX( m_Width, g_DrawMinimunLineWidth );

    *corner_list = 7; corner_list++;	// 7 corners in list

    int symb_len = Len_Size() + (TXTMARGE * 2);   // Real text len + text margin
    // Create outline shape : 6 points
    int x = symb_len + width + 3;
    int y = HalfSize + width + 3;
    corner_list[0]  = 0; corner_list[1] = 0;        // Starting point (anchor)
    corner_list[2]  = 0; corner_list[3] = -y;       // Up
    corner_list[4]  = -x; corner_list[5] = -y;      // left Up
    corner_list[6]  = -x; corner_list[7] = 0;       // left
    corner_list[8]  = -x; corner_list[9] = y;       // left down
    corner_list[10] = 0; corner_list[11] = y;       // down

    int x_offset = 0;

    switch( m_Shape )
    {
    case NET_INPUT:
        x_offset = -HalfSize;
        corner_list[0] += HalfSize;
        break;

    case NET_OUTPUT:
        corner_list[6] -= HalfSize;
        break;

    case NET_BIDI:
    case NET_TRISTATE:
        x_offset = -HalfSize;
        corner_list[0] += HalfSize;
        corner_list[6] -= HalfSize;
        break;

    case NET_UNSPECIFIED:
    default:
        break;
    }

    int angle = 0;

    switch( m_Orient )
    {
    case 0:             /* Orientation horiz normale */
        break;

    case 1:             /* Orientation vert UP */
        angle = 900;
        break;

    case 2:             /* Orientation horiz inverse */
        angle = 1800;
        break;

    case 3:             /* Orientation vert BOTTOM */
        angle = -900;
        break;
    }

    // Rotate outlines and move in real position
    for( int ii = 0; ii < 12; ii += 2 )
    {
        corner_list[ii] += x_offset;
        if( angle )
            RotatePoint( &corner_list[ii], &corner_list[ii + 1], angle );
        corner_list[ii]     += Pos.x;
        corner_list[ii + 1] += Pos.y;
    }

    corner_list[12] = corner_list[0]; corner_list[13] = corner_list[1]; // closing
}


EDA_Rect SCH_GLOBALLABEL::GetBoundingBox()
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    dx = dy = 0;

    int     width = MAX( m_Width, g_DrawMinimunLineWidth );
    height  = m_Size.y + 2*TXTMARGE;
    length = ( Pitch(width) * GetLength() ) + 2* height + 2*DANGLING_SYMBOL_SIZE; // add 2*height for triangular shapes (bidirectional)

    switch( m_Orient ) // respect orientation
    {
        case 0: /* Horiz Normal Orientation (left justified) */
            dx = -length;
            dy = height;
            x += DANGLING_SYMBOL_SIZE;
            y -= height/2;
            break;

        case 1: /* Vert Orientation UP */
            dx = height;
            dy = length;
            x -= height/2;
            y -= DANGLING_SYMBOL_SIZE;
            break;

        case 2: /* Horiz Orientation - Right justified */
            dx = length;
            dy = height;
            x -= DANGLING_SYMBOL_SIZE;
            y -= height/2;
            break;

        case 3: /*  Vert Orientation BOTTOM */
            dx = height;
            dy = -length;
            x -= height/2;
            y += DANGLING_SYMBOL_SIZE;
            break;
    }

    EDA_Rect box(wxPoint(x,y), wxSize(dx,dy) );
    box.Normalize();
    return box;
}


EDA_Rect SCH_TEXT::GetBoundingBox()
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    int     width = MAX( m_Width, g_DrawMinimunLineWidth );
    length = ( Pitch(width) * GetLength() );
    height  = m_Size.y;
    dx = dy = 0;

    switch( m_Orient ) // respect orientation
    {
        case 0: /* Horiz Normal Orientation (left justified) */
            dx = 2*DANGLING_SYMBOL_SIZE + length;
            dy = -2*DANGLING_SYMBOL_SIZE - height - TXTMARGE;
            x -= DANGLING_SYMBOL_SIZE;
            y += DANGLING_SYMBOL_SIZE;
            break;

        case 1: /* Vert Orientation UP */
            dx = -2*DANGLING_SYMBOL_SIZE - height - TXTMARGE;
            dy = -2*DANGLING_SYMBOL_SIZE - length;
            x += DANGLING_SYMBOL_SIZE;
            y += DANGLING_SYMBOL_SIZE;
            break;

        case 2: /* Horiz Orientation - Right justified */
            dx = -2*DANGLING_SYMBOL_SIZE - length;
            dy = -2*DANGLING_SYMBOL_SIZE - height - TXTMARGE;
            x += DANGLING_SYMBOL_SIZE;
            y += DANGLING_SYMBOL_SIZE;
            break;

        case 3: /*  Vert Orientation BOTTOM */
            dx = -2*DANGLING_SYMBOL_SIZE - height - TXTMARGE;
            dy = 2*DANGLING_SYMBOL_SIZE + length;
            x += DANGLING_SYMBOL_SIZE;
            y -= DANGLING_SYMBOL_SIZE;
            break;
    }

    EDA_Rect box(wxPoint(x,y), wxSize(dx,dy) );
    box.Normalize();
    return box;
}

