/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <font/font.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotter_hpgl.h>
#include <plotters/plotters_pslike.h>
#include <plotters/plotter_gerber.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_draw_item.h>
#include <title_block.h>
#include <wx/filename.h>


wxString GetDefaultPlotExtension( PLOT_FORMAT aFormat )
{
    switch( aFormat )
    {
    case PLOT_FORMAT::DXF:    return DXF_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT::POST:   return PS_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT::PDF:    return PDF_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT::HPGL:   return HPGL_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT::GERBER: return GERBER_PLOTTER::GetDefaultFileExtension();
    case PLOT_FORMAT::SVG:    return SVG_PLOTTER::GetDefaultFileExtension();
    default:    wxFAIL;       return wxEmptyString;
    }
}


void PlotDrawingSheet( PLOTTER* plotter, const PROJECT* aProject, const TITLE_BLOCK& aTitleBlock,
                       const PAGE_INFO& aPageInfo, const std::map<wxString, wxString>* aProperties,
                       const wxString& aSheetNumber, int aSheetCount, const wxString& aSheetName,
                       const wxString& aSheetPath, const wxString& aFilename, COLOR4D aColor,
                       bool aIsFirstPage )
{
    /* Note: Page sizes values are given in mils
     */
    double           iusPerMil = plotter->GetIUsPerDecimil() * 10.0;
    COLOR4D          plotColor = plotter->GetColorMode() ? aColor : COLOR4D::BLACK;
    RENDER_SETTINGS* settings = plotter->RenderSettings();
    int              defaultPenWidth = settings->GetDefaultPenWidth();

    if( plotColor == COLOR4D::UNSPECIFIED )
        plotColor = COLOR4D( RED );

    DS_DRAW_ITEM_LIST drawList( unityScale );

    // Print only a short filename, if aFilename is the full filename
    wxFileName fn( aFilename );

    // Prepare plot parameters
    drawList.SetDefaultPenSize( PLOTTER::USE_DEFAULT_LINE_WIDTH );
    drawList.SetPlotterMilsToIUfactor( iusPerMil );
    drawList.SetPageNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( fn.GetFullName() );   // Print only the short filename
    drawList.SetSheetName( aSheetName );
    drawList.SetSheetPath( aSheetPath );
    drawList.SetSheetLayer( settings->GetLayerName() );
    drawList.SetProject( aProject );
    drawList.SetIsFirstPage( aIsFirstPage );
    drawList.SetProperties( aProperties );

    drawList.BuildDrawItemsList( aPageInfo, aTitleBlock );

    // Draw bitmaps first
    for( DS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
    {
        if( item->Type() == WSG_BITMAP_T )
        {
            DS_DRAW_ITEM_BITMAP* drawItem = (DS_DRAW_ITEM_BITMAP*) item;
            DS_DATA_ITEM_BITMAP* bitmap = (DS_DATA_ITEM_BITMAP*) drawItem->GetPeer();

            if( bitmap->m_ImageBitmap == nullptr )
                continue;

            bitmap->m_ImageBitmap->PlotImage( plotter, drawItem->GetPosition(), plotColor,
                                              PLOTTER::USE_DEFAULT_LINE_WIDTH );
        }
    }

    // Draw other items
    for( DS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
    {
        if( item->Type() == WSG_BITMAP_T )
            continue;

        plotter->SetColor( plotColor );
        plotter->SetCurrentLineWidth( PLOTTER::USE_DEFAULT_LINE_WIDTH );

        switch( item->Type() )
        {
        case WSG_LINE_T:
        {
            DS_DRAW_ITEM_LINE* line = (DS_DRAW_ITEM_LINE*) item;
            plotter->SetCurrentLineWidth( std::max( line->GetPenWidth(), defaultPenWidth ) );
            plotter->MoveTo( line->GetStart() );
            plotter->FinishTo( line->GetEnd() );
        }
            break;

        case WSG_RECT_T:
        {
            DS_DRAW_ITEM_RECT* rect = (DS_DRAW_ITEM_RECT*) item;
            plotter->SetCurrentLineWidth( std::max( rect->GetPenWidth(), defaultPenWidth ) );
            plotter->MoveTo( rect->GetStart() );
            plotter->LineTo( VECTOR2I( rect->GetEnd().x, rect->GetStart().y ) );
            plotter->LineTo( VECTOR2I( rect->GetEnd().x, rect->GetEnd().y ) );
            plotter->LineTo( VECTOR2I( rect->GetStart().x, rect->GetEnd().y ) );
            plotter->FinishTo( rect->GetStart() );
        }
            break;

        case WSG_TEXT_T:
        {
            DS_DRAW_ITEM_TEXT* text = (DS_DRAW_ITEM_TEXT*) item;
            KIFONT::FONT*      font = text->GetFont();
            COLOR4D            color = plotColor;

            if( !font )
            {
                font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), text->IsBold(),
                                              text->IsItalic() );
            }

            if( plotter->GetColorMode() && text->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = text->GetTextColor();

            int penWidth = std::max( text->GetEffectiveTextPenWidth(), defaultPenWidth );

            plotter->Text( text->GetTextPos(), color, text->GetShownText( true ),
                           text->GetTextAngle(), text->GetTextSize(), text->GetHorizJustify(),
                           text->GetVertJustify(), penWidth, text->IsItalic(), text->IsBold(),
                           text->IsMultilineAllowed(), font );
        }
            break;

        case WSG_POLY_T:
        {
            DS_DRAW_ITEM_POLYPOLYGONS* poly = (DS_DRAW_ITEM_POLYPOLYGONS*) item;
            int                        penWidth = poly->GetPenWidth();
            std::vector<VECTOR2I>      points;

            for( int idx = 0; idx < poly->GetPolygons().OutlineCount(); ++idx )
            {
                points.clear();
                SHAPE_LINE_CHAIN& outline = poly->GetPolygons().Outline( idx );

                for( int ii = 0; ii < outline.PointCount(); ii++ )
                    points.emplace_back( outline.CPoint( ii ).x, outline.CPoint( ii ).y );

                plotter->PlotPoly( points, FILL_T::FILLED_SHAPE, penWidth );
            }
        }
            break;

        default:
            wxFAIL_MSG( "PlotDrawingSheet(): Unknown drawing sheet item." );
            break;
        }
    }
}
