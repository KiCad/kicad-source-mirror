/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#include <board_printout.h>

#include <lset.h>
#include <view/view.h>
#include <gal/gal_print.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <pcbplot.h>
#include <settings/app_settings.h>


BOARD_PRINTOUT_SETTINGS::BOARD_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
    : PRINTOUT_SETTINGS( aPageInfo )
{
    m_LayerSet.set();
    m_Mirror = false;
}


void BOARD_PRINTOUT_SETTINGS::Load( APP_SETTINGS_BASE* aConfig )
{
    PRINTOUT_SETTINGS::Load( aConfig );

    m_LayerSet.reset();

    for( int layer : aConfig->m_Printing.layers )
        m_LayerSet.set( layer, true );

    m_Mirror = aConfig->m_Printing.mirror;
}


void BOARD_PRINTOUT_SETTINGS::Save( APP_SETTINGS_BASE* aConfig )
{
    PRINTOUT_SETTINGS::Save( aConfig );

    aConfig->m_Printing.layers.clear();

    for( unsigned layer = 0; layer < m_LayerSet.size(); ++layer )
        if( m_LayerSet.test( layer ) )
            aConfig->m_Printing.layers.push_back( layer );

    aConfig->m_Printing.mirror = m_Mirror;
}


BOARD_PRINTOUT::BOARD_PRINTOUT( const BOARD_PRINTOUT_SETTINGS& aParams,
                                const KIGFX::VIEW* aView, const wxString& aTitle ) :
    wxPrintout( aTitle ),
    m_settings( aParams )
{
    m_view = aView;
    m_gerbviewPrint = false;
}


void BOARD_PRINTOUT::GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
{
    *minPage     = 1;
    *selPageFrom = 1;

    *maxPage   = m_settings.m_pageCount;
    *selPageTo = m_settings.m_pageCount;
}


void BOARD_PRINTOUT::DrawPage( const wxString& aLayerName, int aPageNum, int aPageCount )
{
    wxDC* dc = GetDC();
    KIGFX::GAL_DISPLAY_OPTIONS options;
    std::unique_ptr<KIGFX::GAL_PRINT> galPrint = KIGFX::GAL_PRINT::Create( options, dc );
    KIGFX::GAL* gal = galPrint->GetGAL();
    KIGFX::PRINT_CONTEXT* printCtx = galPrint->GetPrintCtx();
    std::unique_ptr<KIGFX::PAINTER> painter = getPainter( gal );
    std::unique_ptr<KIGFX::VIEW> view( m_view->DataReference() );

    // Target paper size
    wxRect         pageSizePx = GetLogicalPageRect();
    const VECTOR2D pageSizeIn( (double) pageSizePx.width / dc->GetPPI().x,
                               (double) pageSizePx.height / dc->GetPPI().y );
    const VECTOR2D pageSizeIU( milsToIU( pageSizeIn.x * 1000 ), milsToIU( pageSizeIn.y * 1000 ) );

    galPrint->SetSheetSize( pageSizeIn );

    view->SetGAL( gal );
    view->SetPainter( painter.get() );
    view->SetScaleLimits( 10e9, 0.0001 );
    view->SetScale( 1.0 );


    // Set the color scheme
    RENDER_SETTINGS* dstSettings = view->GetPainter()->GetSettings();
    dstSettings->LoadColors( m_settings.m_colorSettings );

    if( m_settings.m_blackWhite )
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
            dstSettings->SetLayerColor( i, COLOR4D::BLACK );

        // In B&W mode, draw the background only in wxhite, because any other color
        // will be replaced by a black background
        dstSettings->SetBackgroundColor( COLOR4D::WHITE );
    }
    else // color enabled
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
        {
            // Cairo does not support translucent colors on PostScript surfaces
            // see 'Features support by the PostScript surface' on
            // https://www.cairographics.org/documentation/using_the_postscript_surface/
            dstSettings->SetLayerColor( i, dstSettings->GetLayerColor( i ).WithAlpha( 1.0 ) );
        }
    }

    dstSettings->SetIsPrinting( true );

    setupPainter( *painter );
    setupViewLayers( *view, m_settings.m_LayerSet );
    dstSettings->SetPrintLayers( m_settings.m_LayerSet );

    dstSettings->SetLayerName( aLayerName );

    VECTOR2I sheetSizeMils = m_settings.m_pageInfo.GetSizeMils();
    VECTOR2I sheetSizeIU( milsToIU( sheetSizeMils.x ),
                          milsToIU( sheetSizeMils.y ) );
    BOX2I    drawingAreaBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( sheetSizeIU ) );

    // When printing the board without worksheet items, move board center to the
    // drawing area center.
    if( !m_settings.PrintBorderAndTitleBlock() )
        drawingAreaBBox = getBoundingBox();

    view->SetLayerVisible( LAYER_DRAWINGSHEET, m_settings.PrintBorderAndTitleBlock() );

    // Fit to page (drawingAreaBBox)
    if( m_settings.m_scale <= 0.0 )
    {
        if( drawingAreaBBox.GetWidth() == 0 || drawingAreaBBox.GetHeight() == 0 )
        {
            // Nothing to print (empty board and no worksheet)
            m_settings.m_scale = 1.0;
        }
        else
        {
            double scaleX = (double) pageSizeIU.x / drawingAreaBBox.GetWidth();
            double scaleY = (double) pageSizeIU.y / drawingAreaBBox.GetHeight();
            m_settings.m_scale = std::min( scaleX, scaleY );
        }
    }

    setupGal( gal );
    galPrint->SetNativePaperSize( pageSizeIn, printCtx->HasNativeLandscapeRotation() );
    gal->SetLookAtPoint( drawingAreaBBox.Centre() );
    gal->SetZoomFactor( m_settings.m_scale );
    gal->SetClearColor( dstSettings->GetBackgroundColor() );

    // Clearing the screen for the background color needs the screen set to the page size
    // in pixels.  This can ?somehow? prevent some but not all foreground elements from being printed
    // TODO: figure out what's going on here and fix printing.  See also sch_printout
    VECTOR2I size = gal->GetScreenPixelSize();
    gal->ResizeScreen( pageSizePx.GetWidth(),pageSizePx.GetHeight() );
    gal->ClearScreen();
    gal->ResizeScreen( size.x, size.y );

    if( m_gerbviewPrint )
        // Mandatory in Gerbview to use the same order for printing as for screen redraw
        // due to negative objects that need a specific order
        view->UseDrawPriority( true );

    {
        KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
        view->Redraw();
    }
}


void BOARD_PRINTOUT::setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet )
{
    // Disable all layers by default, let specific implementations enable required layers
    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; ++i )
    {
        aView.SetLayerVisible( i, false );
        aView.SetTopLayer( i, false );
        aView.SetLayerTarget( i, KIGFX::TARGET_NONCACHED );
    }
}


void BOARD_PRINTOUT::setupPainter( KIGFX::PAINTER& aPainter )
{
    if( !m_settings.m_background )
        aPainter.GetSettings()->SetBackgroundColor( COLOR4D::WHITE );
}


void BOARD_PRINTOUT::setupGal( KIGFX::GAL* aGal )
{
    aGal->SetFlip( m_settings.m_Mirror, false );
}
