/////////////////////////////////////////////////////////////////////////////

// Name:        class_hierarchical_PIN_sheet.cpp
// Purpose:     member functions SCH_SHEET_PIN
//              header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:   License GNU
// License:
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "program.h"
#include "general.h"
#include "protos.h"


/*******************************************************************/
SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent,
                              const wxPoint& pos,
                              const wxString& text ) :
    SCH_ITEM( parent, DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE ),
    EDA_TextStruct( text )
{
/*******************************************************************/
    wxASSERT( parent );
    wxASSERT( Pnext == NULL );
    m_Layer = LAYER_SHEETLABEL;
    m_Pos   = pos;
    m_Edge  = 0;
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_Number     = 2;
}


/***********************************************************/
SCH_SHEET_PIN* SCH_SHEET_PIN::GenCopy()
{
/***********************************************************/
    SCH_SHEET_PIN* newitem =
        new SCH_SHEET_PIN( (SCH_SHEET*) m_Parent, m_Pos, m_Text );

    newitem->m_Edge   = m_Edge;
    newitem->m_Shape  = m_Shape;
    newitem->m_Number = m_Number;

    return newitem;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_SHEET_PIN::GetPenSize()
{
    return g_DrawDefaultLineThickness;
}


/*****************************************************************************/
void SCH_SHEET_PIN::Draw( WinEDA_DrawPanel* panel,
                          wxDC*             DC,
                          const wxPoint&    offset,
                          int               DrawMode,
                          int               Color )
{
/*****************************************************************************/
/* Routine to create hierarchical labels */
    GRTextHorizJustifyType       side;
    EDA_Colors                   txtcolor;
    int posx, tposx, posy;

    static std::vector <wxPoint> Poly;

    int LineWidth = GetPenSize();

    if( Color >= 0 )
        txtcolor = (EDA_Colors) Color;
    else
        txtcolor = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    posx = m_Pos.x + offset.x;
    posy = m_Pos.y + offset.y;
    wxSize size = m_Size;

    if( !m_Text.IsEmpty() )
    {
        if( m_Edge )
        {
            tposx = posx - size.x;
            side  = GR_TEXT_HJUSTIFY_RIGHT;
        }
        else
        {
            tposx = posx + size.x + (size.x / 8);
            side  = GR_TEXT_HJUSTIFY_LEFT;
        }
        DrawGraphicText( panel, DC, wxPoint( tposx, posy ), txtcolor,
                         m_Text, TEXT_ORIENT_HORIZ, size, side,
                         GR_TEXT_VJUSTIFY_CENTER, LineWidth, false, false );
    }

    /* Draw the graphic symbol */
    CreateGraphicShape( Poly, m_Pos + offset );
    int FillShape = false;
    GRPoly( &panel->m_ClipBox, DC, Poly.size(), &Poly[0],
            FillShape, LineWidth, txtcolor, txtcolor );
}


/** function CreateGraphicShape
 * Calculates the graphic shape (a polygon) associated to the text
 * @param aCorner_list = list to fill with polygon corners coordinates
 * @param Pos = Position of the shape
 */
void SCH_SHEET_PIN::CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint& Pos )
{
    wxSize size = m_Size;

    aCorner_list.clear();
    if( m_Edge )
    {
        size.x = -size.x;
        size.y = -size.y;
    }

    int size2 = size.x / 2;

    aCorner_list.push_back( Pos );
    switch( m_Shape )
    {
    case 0:         /* input |> */
        aCorner_list.push_back( wxPoint( Pos.x, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y ) );
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y + size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x, Pos.y + size2 ) );
        aCorner_list.push_back( Pos );
        break;

    case 1:         /* output <| */
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y + size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y + size2 ) );
        aCorner_list.push_back( Pos );
        break;

    case 2:         /* bidi <> */
    case 3:         /* TriSt <> */
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y ) );
        aCorner_list.push_back( wxPoint( Pos.x + size2, Pos.y + size2 ) );
        aCorner_list.push_back( Pos );
        break;

    default:         /* unsp []*/
        aCorner_list.push_back( wxPoint( Pos.x, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y - size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x + size.x, Pos.y + size2 ) );
        aCorner_list.push_back( wxPoint( Pos.x, Pos.y + size2 ) );
        aCorner_list.push_back( Pos );
        break;
    }
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_SHEET_PIN::Save( FILE* aFile ) const
{
    int type = 'U', side = 'L';

    if( m_Text.IsEmpty() )
        return true;
    if( m_Edge )
        side = 'R';

    switch( m_Shape )
    {
    case NET_INPUT:
        type = 'I'; break;

    case NET_OUTPUT:
        type = 'O'; break;

    case NET_BIDI:
        type = 'B'; break;

    case NET_TRISTATE:
        type = 'T'; break;

    case NET_UNSPECIFIED:
        type = 'U'; break;
    }

    if( fprintf( aFile, "F%d \"%s\" %c %c %-3d %-3d %-3d\n", m_Number,
                 CONV_TO_UTF8( m_Text ), type, side, m_Pos.x, m_Pos.y,
                 m_Size.x ) == EOF )
    {
        return false;
    }

    return true;
}


#if defined(DEBUG)
void SCH_SHEET_PIN::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">"
                                 << " pin_name=\"" << CONV_TO_UTF8( m_Text )
                                 << '"' << "/>\n" << std::flush;

//    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n";
}


#endif
