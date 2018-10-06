/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <view/view.h>
#include <gal/gal_print.h>
#include <painter.h>
#include <pcbplot.h>


BOARD_PRINTOUT_SETTINGS::BOARD_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
    : PRINTOUT_SETTINGS( aPageInfo )
{
    m_layerSet.set();
    m_mirror = false;
}


void BOARD_PRINTOUT_SETTINGS::Load( wxConfigBase* aConfig )
{
    PRINTOUT_SETTINGS::Load( aConfig );

    for( unsigned layer = 0; layer < m_layerSet.count(); ++layer )
    {
        int tmp;
        wxString key = wxString::Format( OPTKEY_LAYERBASE, layer );
        aConfig->Read( key, &tmp, 1 );
        m_layerSet.set( layer, tmp );
    }
}


void BOARD_PRINTOUT_SETTINGS::Save( wxConfigBase* aConfig )
{
    PRINTOUT_SETTINGS::Save( aConfig );

    for( unsigned layer = 0; layer < m_layerSet.count(); ++layer )
    {
        wxString key = wxString::Format( OPTKEY_LAYERBASE, layer );
        aConfig->Write( key, m_layerSet.test( layer ) );
    }
}


BOARD_PRINTOUT::BOARD_PRINTOUT( const BOARD_PRINTOUT_SETTINGS& aParams,
        const KIGFX::VIEW* aView, const wxString& aTitle ) :
    wxPrintout( aTitle ), m_settings( aParams )
{
    m_view = aView;
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
    auto dc = GetDC();
    KIGFX::GAL_DISPLAY_OPTIONS options;
    auto galPrint = KIGFX::GAL_PRINT::Create( options, dc );
    auto gal = galPrint->GetGAL();
    auto printCtx = galPrint->GetPrintCtx();
    auto painter = getPainter( gal );
    std::unique_ptr<KIGFX::VIEW> view( m_view->DataReference() );

    wxRect pageSizePx = GetLogicalPageRect();
    VECTOR2D pageSizeIn( (double) pageSizePx.width / dc->GetPPI().x,
            (double) pageSizePx.height / dc->GetPPI().y );
    galPrint->SetSheetSize( pageSizeIn );

    view->SetGAL( gal );
    view->SetPainter( painter.get() );
    view->SetScaleLimits( 10e9, 0.0001 );
    view->SetScale( 1.0 );


    // Set the color scheme
    auto srcSettings = m_view->GetPainter()->GetSettings();
    auto dstSettings = view->GetPainter()->GetSettings();

    if( m_settings.m_blackWhite )
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
            dstSettings->SetLayerColor( i, COLOR4D::BLACK );
    }
    else // color enabled
    {
        for( int i = 0; i < LAYER_ID_COUNT; ++i )
            dstSettings->SetLayerColor( i, srcSettings->GetLayerColor( i ) );
    }


    setupViewLayers( view, m_settings.m_layerSet );
    setupPainter( painter );

    BOX2I bBox; // determine printout bounding box
    auto sheetSizeMils = m_settings.m_pageInfo.GetSizeMils();
    VECTOR2I sheetSizeIU( milsToIU( sheetSizeMils.GetWidth() ), milsToIU( sheetSizeMils.GetHeight() ) );

    if( m_settings.PrintBorderAndTitleBlock() )
    {
        bBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( sheetSizeIU ) );
        view->SetLayerVisible( LAYER_WORKSHEET, true );
    }
    else
    {
        EDA_RECT targetBbox = getBoundingBox();
        bBox = BOX2I( targetBbox.GetOrigin(), targetBbox.GetSize() );
        view->SetLayerVisible( LAYER_WORKSHEET, false );
    }


    // Fit to page
    if( m_settings.m_scale <= 0.0 )
    {
        if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        {
            // Nothing to print
            m_settings.m_scale = 1.0;
        }
        else
        {
            double scaleX = (double) sheetSizeIU.x / bBox.GetWidth();
            double scaleY = (double) sheetSizeIU.y / bBox.GetHeight();
            m_settings.m_scale = std::min( scaleX, scaleY );
        }
    }

    setupGal( gal );
    galPrint->SetNativePaperSize( pageSizeIn, printCtx->HasNativeLandscapeRotation() );
    gal->SetLookAtPoint( bBox.Centre() );
    gal->SetZoomFactor( m_settings.m_scale );

    {
    KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
    view->Redraw();
    }
}


void BOARD_PRINTOUT::setupViewLayers( const std::unique_ptr<KIGFX::VIEW>& aView,
        const LSET& aLayerSet )
{
    // Disable all layers by default, let specific implementions enable required layers
    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; ++i )
    {
        aView->SetLayerVisible( i, false );
        aView->SetTopLayer( i, false );
        aView->SetLayerTarget( i, KIGFX::TARGET_NONCACHED );
    }
}


void BOARD_PRINTOUT::setupPainter( const std::unique_ptr<KIGFX::PAINTER>& aPainter )
{
    aPainter->GetSettings()->SetOutlineWidth( m_settings.m_lineWidth );
    aPainter->GetSettings()->SetBackgroundColor( COLOR4D::WHITE );
}


void BOARD_PRINTOUT::setupGal( KIGFX::GAL* aGal )
{
    aGal->SetFlip( m_settings.m_mirror, false );
}
