/**
 * @file worksheet.cpp
 * @brief Common code to draw the title block and frame references
 * @note it should include title_block_shape_gost.h or title_block_shape.h
 * which defines most of draw shapes, and contains a part of the draw code
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <drawtxt.h>
#include <confirm.h>
#include <wxstruct.h>
#include <appl_wxstruct.h>
#include <kicad_string.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <build_version.h>

// include data which defines the shape of a title block
// and frame references
#include <worksheet_shape_builder.h>

#if defined(KICAD_GOST)
#include "title_block_shapes_gost.h"
#else
#include "title_block_shapes.h"
#endif

void DrawPageLayout( wxDC* aDC, EDA_DRAW_PANEL * aCanvas,
                     const PAGE_INFO& aPageInfo,
                     const wxString& aPaperFormat,
                     const wxString &aFullSheetName,
                     const wxString& aFileName,
                     TITLE_BLOCK& aTitleBlock,
                     int aSheetCount, int aSheetNumber,
                     int aPenWidth, double aScalar,
                     EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    GRSetDrawMode( aDC, GR_COPY );
    WS_DRAW_ITEM_LIST drawList;

    wxPoint LTmargin( aPageInfo.GetLeftMarginMils(), aPageInfo.GetTopMarginMils() );
    wxPoint RBmargin( aPageInfo.GetRightMarginMils(), aPageInfo.GetBottomMarginMils() );
    wxSize pagesize = aPageInfo.GetSizeMils();

    drawList.SetMargins( LTmargin, RBmargin );
    drawList.SetPenSize( aPenWidth );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetPageSize( pagesize );

    drawList.BuildWorkSheetGraphicList(
                               aPaperFormat, aFullSheetName, aFileName,
                               aTitleBlock, aSheetCount, aSheetNumber,
                               aLineColor, aTextColor );

    // Draw item list
    for( WS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item;
         item = drawList.GetNext() )
    {
        switch( item->GetType() )
        {
        case WS_DRAW_ITEM_BASE::wsg_line:
            {
                WS_DRAW_ITEM_LINE* line = (WS_DRAW_ITEM_LINE*) item;
                GRLine( aCanvas ? aCanvas->GetClipBox() : NULL, aDC,
                        line->GetStart(), line->GetEnd(),
                        line->GetPenWidth(), line->GetColor() );
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_rect:
            {
                WS_DRAW_ITEM_RECT* rect = (WS_DRAW_ITEM_RECT*) item;
                GRRect( aCanvas ? aCanvas->GetClipBox() : NULL, aDC,
                        rect->GetStart().x, rect->GetStart().y,
                        rect->GetEnd().x, rect->GetEnd().y,
                        rect->GetPenWidth(), rect->GetColor() );
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_text:
            {
                WS_DRAW_ITEM_TEXT* text = (WS_DRAW_ITEM_TEXT*) item;
                DrawGraphicText( aCanvas, aDC, text->GetTextPosition(),
                                 text->GetColor(), text->GetText(),
                                 text->GetOrientation(), text->GetSize(),
                                 text->GetHorizJustify(), text->GetVertJustify(),
                                 text->GetPenWidth(), text->IsItalic(), text->IsBold() );
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_poly:
            {
                WS_DRAW_ITEM_POLYGON* poly = (WS_DRAW_ITEM_POLYGON*) item;
                GRPoly( aCanvas ? aCanvas->GetClipBox() : NULL, aDC,
                        poly->m_Corners.size(), &poly->m_Corners[0],
                        poly->IsFilled() ? FILLED_SHAPE : NO_FILL,
                        poly->GetPenWidth(),
                        poly->GetColor(), poly->GetColor() );
            }
            break;
        }
    }
}


void EDA_DRAW_FRAME::DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                     double aScalar, const wxString &aFilename )
{
    if( !m_showBorderAndTitleBlock )
        return;

    const PAGE_INFO&  pageInfo = GetPageSettings();
    wxSize  pageSize = pageInfo.GetSizeMils();

    // if not printing, draw the page limits:
    if( !aScreen->m_IsPrinting && g_ShowPageLimits )
    {
        GRSetDrawMode( aDC, GR_COPY );
        GRRect( m_canvas->GetClipBox(), aDC, 0, 0,
                pageSize.x * aScalar, pageSize.y * aScalar, aLineWidth,
                g_DrawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    wxString paper = pageInfo.GetType();
    TITLE_BLOCK t_block = GetTitleBlock();
    EDA_COLOR_T color = RED;

    DrawPageLayout( aDC, m_canvas, pageInfo,
                    paper, aFilename, GetScreenDesc(), t_block,
                    aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber,
                    aLineWidth, aScalar, color, color );
}


const wxString EDA_DRAW_FRAME::GetXYSheetReferences( const wxPoint& aPosition ) const
{
    const PAGE_INFO& pageInfo = GetPageSettings();

    int         ii;
    int         xg, yg;
    int         ipas;
    int         gxpas, gypas;
    int         refx, refy;
    wxString    msg;

    // Upper left corner
    refx = pageInfo.GetLeftMarginMils();
    refy = pageInfo.GetTopMarginMils();

    // lower right corner
    xg   = pageInfo.GetSizeMils().x - pageInfo.GetRightMarginMils();
    yg   = pageInfo.GetSizeMils().y - pageInfo.GetBottomMarginMils();

    // Get the Y axis identifier (A symbol A ... Z)
    if( aPosition.y < refy || aPosition.y > yg )  // Outside of Y limits
        msg << wxT( "?" );
    else
    {
        ipas  = ( yg - refy ) / PAS_REF;        // ipas = Y count sections
        gypas = ( yg - refy ) / ipas;           // gypas = Y section size
        ii    = ( aPosition.y - refy ) / gypas;
        msg.Printf( wxT( "%c" ), 'A' + ii );
    }

    // Get the X axis identifier (A number 1 ... n)
    if( aPosition.x < refx || aPosition.x > xg )  // Outside of X limits
        msg << wxT( "?" );
    else
    {
        ipas  = ( xg - refx ) / PAS_REF;        // ipas = X count sections
        gxpas = ( xg - refx ) / ipas;           // gxpas = X section size

        ii = ( aPosition.x - refx ) / gxpas;
        msg << ii + 1;
    }

    return msg;
}


wxString EDA_DRAW_FRAME::GetScreenDesc()
{
    wxString msg;

    msg << GetScreen()->m_ScreenNumber << wxT( "/" )
        << GetScreen()->m_NumberOfScreens;
    return msg;
}


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    // Don't write the title block information if there is nothing to write.
    if(  !m_title.IsEmpty() || !m_date.IsEmpty() || !m_revision.IsEmpty()
      || !m_company.IsEmpty() || !m_comment1.IsEmpty() || !m_comment2.IsEmpty()
      || !m_comment3.IsEmpty() || !m_comment4.IsEmpty()  )
    {
        aFormatter->Print( aNestLevel, "(title_block\n" );

        if( !m_title.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( m_title ).c_str() );

        if( !m_date.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( m_date ).c_str() );

        if( !m_revision.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n",
                               aFormatter->Quotew( m_revision ).c_str() );

        if( !m_company.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n",
                               aFormatter->Quotew( m_company ).c_str() );

        if( !m_comment1.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 1 %s)\n",
                               aFormatter->Quotew( m_comment1 ).c_str() );

        if( !m_comment2.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 2 %s)\n",
                               aFormatter->Quotew( m_comment2 ).c_str() );

        if( !m_comment3.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 3 %s)\n",
                               aFormatter->Quotew( m_comment3 ).c_str() );

        if( !m_comment4.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 4 %s)\n",
                               aFormatter->Quotew( m_comment4 ).c_str() );

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}
