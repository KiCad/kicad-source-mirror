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
#include <pgm_base.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <drawtxt.h>
#include <draw_frame.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <build_version.h>

#include <worksheet_shape_builder.h>

static const wxString productName = wxT( "KiCad E.D.A.  " );

void DrawPageLayout( wxDC* aDC, EDA_RECT* aClipBox,
                     const PAGE_INFO& aPageInfo,
                     const wxString &aFullSheetName,
                     const wxString& aFileName,
                     TITLE_BLOCK& aTitleBlock,
                     int aSheetCount, int aSheetNumber,
                     int aPenWidth, double aScalar,
                     EDA_COLOR_T aColor, EDA_COLOR_T aAltColor )
{
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetPenSize( aPenWidth );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aFullSheetName );

    drawList.BuildWorkSheetGraphicList( aPageInfo,
                               aTitleBlock, aColor, aAltColor );

    // Draw item list
    drawList.Draw( aClipBox, aDC );
}


void EDA_DRAW_FRAME::DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                     double aScalar, const wxString &aFilename )
{
    if( !m_showBorderAndTitleBlock )
        return;

    const PAGE_INFO&  pageInfo = GetPageSettings();
    wxSize  pageSize = pageInfo.GetSizeMils();

    // if not printing, draw the page limits:
    if( !aScreen->m_IsPrinting && m_showPageLimits )
    {
        GRSetDrawMode( aDC, GR_COPY );
        GRRect( m_canvas->GetClipBox(), aDC, 0, 0,
                pageSize.x * aScalar, pageSize.y * aScalar, aLineWidth,
                m_drawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    TITLE_BLOCK t_block = GetTitleBlock();
    EDA_COLOR_T color = RED;

    wxPoint origin = aDC->GetDeviceOrigin();

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetAxisOrientation( true, false );
    }

    DrawPageLayout( aDC, m_canvas->GetClipBox(), pageInfo,
                    GetScreenDesc(), aFilename, t_block,
                    aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber,
                    aLineWidth, aScalar, color, color );

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( origin.x, origin.y );
        aDC->SetAxisOrientation( true, true );
    }
}


wxString EDA_DRAW_FRAME::GetScreenDesc() const
{
    // Virtual function. In basic class, returns
    // an empty string.
    return wxEmptyString;
}

// returns the full text corresponding to the aTextbase,
// after replacing format symbols by the corresponding value
wxString WS_DRAW_ITEM_LIST::BuildFullText( const wxString& aTextbase )
{
    wxString msg;

    /* Known formats
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %Cx = comment (x = 0 to 9 to identify the comment)
     * %F = filename
     * %P = sheet path (sheet full name)
     * %T = title
     */

    for( unsigned ii = 0; ii < aTextbase.Len(); ii++ )
    {
        if( aTextbase[ii] != '%' )
        {
            msg << aTextbase[ii];
            continue;
        }

        if( ++ii >= aTextbase.Len() )
            break;

        wxChar format = aTextbase[ii];
        switch( format )
        {
            case '%':
                msg += '%';
                break;

            case 'D':
                msg += m_titleBlock->GetDate();
                break;

            case 'R':
                msg += m_titleBlock->GetRevision();
                break;

            case 'K':
                msg += productName + Pgm().App().GetAppName();
                msg += wxT( " " ) + GetBuildVersion();
                break;

            case 'Z':
                msg += *m_paperFormat;
                break;

            case 'S':
                msg << m_sheetNumber;
                break;

            case 'N':
                msg << m_sheetCount;
                break;

            case 'F':
                {
                    wxFileName fn( m_fileName );
                    msg += fn.GetFullName();
                }
                break;

            case 'P':
                msg += *m_sheetFullName;
                break;

            case 'Y':
                msg = m_titleBlock->GetCompany();
                break;

            case 'T':
                msg += m_titleBlock->GetTitle();
                break;

            case 'C':
                format = aTextbase[++ii];
                switch( format )
                {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    msg += m_titleBlock->GetComment( format - '0');
                    break;

                default:
                    break;
                }

            default:
                break;
        }
    }

    return msg;
}


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    // Don't write the title block information if there is nothing to write.
    bool isempty = true;
    for( unsigned idx = 0; idx < m_tbTexts.GetCount(); idx++ )
    {
        if( ! m_tbTexts[idx].IsEmpty() )
        {
            isempty = false;
            break;
        }
    }

    if( !isempty  )
    {
        aFormatter->Print( aNestLevel, "(title_block\n" );

        if( !GetTitle().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( GetTitle() ).c_str() );

        if( !GetDate().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( GetDate() ).c_str() );

        if( !GetRevision().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n",
                               aFormatter->Quotew( GetRevision() ).c_str() );

        if( !GetCompany().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n",
                               aFormatter->Quotew( GetCompany() ).c_str() );

        for( int ii = 0; ii < 4; ii++ )
        {
            if( !GetComment(ii).IsEmpty() )
                aFormatter->Print( aNestLevel+1, "(comment %d %s)\n", ii+1,
                                  aFormatter->Quotew( GetComment(ii) ).c_str() );
        }

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}
