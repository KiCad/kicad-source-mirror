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
#if defined(KICAD_GOST)
#include "title_block_shapes_gost.h"
#else
#include "title_block_shapes.h"
#endif


void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
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

    wxPoint margin_left_top( pageInfo.GetLeftMarginMils(), pageInfo.GetTopMarginMils() );
    wxPoint margin_right_bottom( pageInfo.GetRightMarginMils(), pageInfo.GetBottomMarginMils() );
    wxString paper = pageInfo.GetType();
    wxString file = aFilename;
    TITLE_BLOCK t_block = GetTitleBlock();
    int number_of_screens = aScreen->m_NumberOfScreens;
    int screen_to_draw = aScreen->m_ScreenNumber;

    TraceWorkSheet( aDC, pageSize, margin_left_top, margin_right_bottom,
                    paper, file, t_block, number_of_screens, screen_to_draw,
                    aLineWidth, aScalar );
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
    if(  !m_title.IsEmpty() || /* !m_date.IsEmpty() || */ !m_revision.IsEmpty()
      || !m_company.IsEmpty() || !m_comment1.IsEmpty() || !m_comment2.IsEmpty()
      || !m_comment3.IsEmpty() || !m_comment4.IsEmpty()  )
    {
        aFormatter->Print( aNestLevel, "(title_block \n" );

        if( !m_title.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( m_title ).c_str() );

        /* version control users were complaining, see mailing list.
        if( !m_date.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( m_date ).c_str() );
        */

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
