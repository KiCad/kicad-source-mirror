/////////////////////////////////////////////////////////////////////////////

// Name:        class_hierarchical_PIN_sheet.cpp
// Purpose:		member functions Hierarchical_PIN_Sheet_Struct
//				header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/*******************************************************************/
Hierarchical_PIN_Sheet_Struct::Hierarchical_PIN_Sheet_Struct( DrawSheetStruct* parent,
                                            const wxPoint& pos, const wxString& text ) :
    SCH_ITEM( NULL, DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE ),
    EDA_TextStruct( text )
/*******************************************************************/
{
    m_Layer      = LAYER_SHEETLABEL;
    m_Pos        = pos;
    m_Edge       = 0;
    m_Shape      = NET_INPUT;
    m_IsDangling = TRUE;
    m_Number = 2;
}


/***********************************************************/
Hierarchical_PIN_Sheet_Struct* Hierarchical_PIN_Sheet_Struct::GenCopy()
/***********************************************************/
{
    Hierarchical_PIN_Sheet_Struct* newitem =
        new Hierarchical_PIN_Sheet_Struct( (DrawSheetStruct*) m_Parent, m_Pos, m_Text );

    newitem->m_Edge  = m_Edge;
    newitem->m_Shape = m_Shape;
    newitem->m_Number = m_Number;

    return newitem;
}


/********************************************************************************************/
void Hierarchical_PIN_Sheet_Struct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                 int DrawMode, int Color )
/********************************************************************************************/
/* Routine de dessin des Labels type hierarchie */
{
    int    side, txtcolor;
    int    posx, tposx, posy, size2;
    wxSize size;
    int    NbSegm, coord[20];
    int    LineWidth = g_DrawMinimunLineWidth;

    if( Color >= 0 )
        txtcolor = Color;
    else
        txtcolor = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    posx = m_Pos.x + offset.x; posy = m_Pos.y + offset.y; size = m_Size;
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
            m_Text, TEXT_ORIENT_HORIZ, size,
            side, GR_TEXT_VJUSTIFY_CENTER, LineWidth );
    }
    /* dessin du symbole de connexion */

    if( m_Edge )
    {
        size.x = -size.x;
        size.y = -size.y;
    }

    coord[0] = posx; coord[1] = posy; size2 = size.x / 2;
    NbSegm   = 0;

    switch( m_Shape )
    {
    case 0:         /* input |> */
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size2; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = posx; coord[11] = posy + size2;
        coord[12] = coord[0]; coord[13] = coord[1];
        NbSegm    = 7;
        break;

    case 1:         /* output <| */
        coord[2]  = posx + size2; coord[3] = posy - size2;
        coord[4]  = posx + size.x; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy + size2;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = coord[0]; coord[11] = coord[1];
        NbSegm    = 6;
        break;

    case 2:         /* bidi <> */
    case 3:         /* TriSt <> */
        coord[2] = posx + size2; coord[3] = posy - size2;
        coord[4] = posx + size.x; coord[5] = posy;
        coord[6] = posx + size2; coord[7] = posy + size2;
        coord[8] = coord[0];  coord[9] = coord[1];
        NbSegm   = 5;
        break;

    default:         /* unsp []*/
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size.x; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy + size2;
        coord[8]  = posx; coord[9] = posy + size2;
        coord[10] = coord[0]; coord[11] = coord[1];
        NbSegm    = 6;
        break;
    }

    int FillShape = FALSE;
    GRPoly( &panel->m_ClipBox, DC, NbSegm, coord, FillShape, LineWidth, txtcolor, txtcolor ); /* Poly Non rempli */
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool Hierarchical_PIN_Sheet_Struct::Save( FILE* aFile ) const
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
            CONV_TO_UTF8( m_Text ), type, side,
            m_Pos.x, m_Pos.y,
            m_Size.x ) == EOF )
    {
        return false;
    }

    return true;
}

