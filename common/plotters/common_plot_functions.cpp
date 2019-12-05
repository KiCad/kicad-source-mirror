/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_struct.h>
#include <plotter.h>
#include <ws_painter.h>
#include <base_screen.h>
#include <gr_text.h>
#include <title_block.h>
#include "ws_draw_item.h"
#include "ws_data_item.h"
#include <wx/filename.h>



wxString GetDefaultPlotExtension( PlotFormat aFormat )
{
    switch( aFormat )
    {
    case PLOT_FORMAT_DXF:    return DXF_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT_POST:   return PS_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT_PDF:    return PDF_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT_HPGL:   return HPGL_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT_GERBER: return GERBER_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT_SVG:    return SVG_PLOTTER::GetDefaultFileExtension();
    default:                 wxASSERT( false ); return wxEmptyString;
    }
}



void PlotWorkSheet( PLOTTER* plotter, const TITLE_BLOCK& aTitleBlock,
                    const PAGE_INFO& aPageInfo, int aSheetNumber, int aNumberOfSheets,
                    const wxString &aSheetDesc, const wxString &aFilename, const COLOR4D aColor )
{
    /* Note: Page sizes values are given in mils
     */
    double   iusPerMil = plotter->GetIUsPerDecimil() * 10.0;

    COLOR4D plotColor = plotter->GetColorMode() ? aColor : COLOR4D::BLACK;

    if( plotColor == COLOR4D::UNSPECIFIED )
        plotColor = COLOR4D( RED );

    plotter->SetColor( plotColor );
    WS_DRAW_ITEM_LIST drawList;

    // Print only a short filename, if aFilename is the full filename
    wxFileName fn( aFilename );

    // Prepare plot parameters
    drawList.SetDefaultPenSize( PLOTTER::USE_DEFAULT_LINE_WIDTH );
    drawList.SetMilsToIUfactor( iusPerMil );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aNumberOfSheets );
    drawList.SetFileName( fn.GetFullName() );   // Print only the short filename
    drawList.SetSheetName( aSheetDesc );


    drawList.BuildWorkSheetGraphicList( aPageInfo, aTitleBlock );

    // Draw item list
    for( WS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
    {
        plotter->SetCurrentLineWidth( PLOTTER::USE_DEFAULT_LINE_WIDTH );

        switch( item->Type() )
        {
        case WSG_LINE_T:
            {
                WS_DRAW_ITEM_LINE* line = (WS_DRAW_ITEM_LINE*) item;
                plotter->SetCurrentLineWidth( line->GetPenWidth() );
                plotter->MoveTo( line->GetStart() );
                plotter->FinishTo( line->GetEnd() );
            }
            break;

        case WSG_RECT_T:
            {
                WS_DRAW_ITEM_RECT* rect = (WS_DRAW_ITEM_RECT*) item;
                plotter->Rect( rect->GetStart(), rect->GetEnd(), NO_FILL, rect->GetPenWidth() );
            }
            break;

        case WSG_TEXT_T:
            {
                WS_DRAW_ITEM_TEXT* text = (WS_DRAW_ITEM_TEXT*) item;
                plotter->Text( text->GetTextPos(), plotColor, text->GetShownText(),
                               text->GetTextAngle(), text->GetTextSize(),
                               text->GetHorizJustify(), text->GetVertJustify(),
                               text->GetPenWidth(), text->IsItalic(), text->IsBold(),
                               text->IsMultilineAllowed() );
            }
            break;

        case WSG_POLY_T:
            {
                WS_DRAW_ITEM_POLYPOLYGONS* poly = (WS_DRAW_ITEM_POLYPOLYGONS*) item;
                std::vector<wxPoint> points;

                for( int idx = 0; idx < poly->GetPolygons().OutlineCount(); ++idx )
                {
                    points.clear();
                    SHAPE_LINE_CHAIN& outline = poly->GetPolygons().Outline( idx );

                    for( int ii = 0; ii < outline.PointCount(); ii++ )
                        points.emplace_back( outline.Point( ii ).x ,
                                                   outline.Point( ii ).y );

                    plotter->PlotPoly(  points, FILLED_SHAPE, poly->GetPenWidth() );
                }
            }
            break;

        case WSG_BITMAP_T:
            {
                WS_DRAW_ITEM_BITMAP* drawItem = (WS_DRAW_ITEM_BITMAP*) item;
                auto*                bitmap = (WS_DATA_ITEM_BITMAP*) drawItem->GetPeer();

                if( bitmap->m_ImageBitmap == NULL )
                    break;

                bitmap->m_ImageBitmap->PlotImage( plotter, drawItem->GetPosition(), plotColor,
                                                  PLOTTER::USE_DEFAULT_LINE_WIDTH );
            }
            break;

        default:
            wxFAIL_MSG( "PlotWorkSheet(): Unknown worksheet item." );
            break;
        }
    }
}
