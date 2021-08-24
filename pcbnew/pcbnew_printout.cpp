/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "pcbnew_printout.h"
#include <board.h>
#include <math/util.h>      // for KiROUND
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <view/view.h>
#include <pcbplot.h>
#include <advanced_config.h>

PCBNEW_PRINTOUT_SETTINGS::PCBNEW_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
    : BOARD_PRINTOUT_SETTINGS( aPageInfo )
{
    m_DrillMarks = SMALL_DRILL_SHAPE;
    m_Pagination = ALL_LAYERS;
    m_PrintEdgeCutsOnAllPages = true;
    m_AsItemCheckboxes = false;
}


void PCBNEW_PRINTOUT_SETTINGS::Load( APP_SETTINGS_BASE* aConfig )
{
    BOARD_PRINTOUT_SETTINGS::Load( aConfig );

    if( auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aConfig ) )
    {
        m_DrillMarks = static_cast<DRILL_MARK_SHAPE_T>( cfg->m_Plot.pads_drill_mode );
        m_Pagination = static_cast<PAGINATION_T>( cfg->m_Plot.all_layers_on_one_page );
        m_Mirror     = cfg->m_Plot.mirror;
    }
}


void PCBNEW_PRINTOUT_SETTINGS::Save( APP_SETTINGS_BASE* aConfig )
{
    BOARD_PRINTOUT_SETTINGS::Save( aConfig );

    if( auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aConfig ) )
    {
        cfg->m_Plot.pads_drill_mode        = m_DrillMarks;
        cfg->m_Plot.all_layers_on_one_page = m_Pagination;
        cfg->m_Plot.mirror                 = m_Mirror;
    }
}


PCBNEW_PRINTOUT::PCBNEW_PRINTOUT( BOARD* aBoard, const PCBNEW_PRINTOUT_SETTINGS& aParams,
        const KIGFX::VIEW* aView, const wxString& aTitle ) :
    BOARD_PRINTOUT( aParams, aView, aTitle ), m_pcbnewSettings( aParams )
{
    m_board = aBoard;
}


bool PCBNEW_PRINTOUT::OnPrintPage( int aPage )
{
    // Store the layerset, as it is going to be modified below and the original settings are
    // needed.
    LSET         lset = m_settings.m_LayerSet;
    int          pageCount = lset.count();
    wxString     layerName;
    PCB_LAYER_ID extractLayer;

    // compute layer mask from page number if we want one page per layer
    if( m_pcbnewSettings.m_Pagination == PCBNEW_PRINTOUT_SETTINGS::LAYER_PER_PAGE )
    {
        // This sequence is TBD, call a different sequencer if needed, such as Seq().
        // Could not find documentation on page order.
        LSEQ seq = lset.UIOrder();

        // aPage starts at 1, not 0
        if( unsigned( aPage - 1 ) < seq.size() )
            m_settings.m_LayerSet = LSET( seq[ aPage - 1] );
    }

    if( !m_settings.m_LayerSet.any() )
        return false;

    extractLayer = m_settings.m_LayerSet.ExtractLayer();

    if( extractLayer == UNDEFINED_LAYER )
        layerName = _( "Multiple Layers" );
    else
        layerName = LSET::Name( extractLayer );

    // In Pcbnew we can want the layer EDGE always printed
    if( m_pcbnewSettings.m_PrintEdgeCutsOnAllPages )
        m_settings.m_LayerSet.set( Edge_Cuts );

    DrawPage( layerName, aPage, pageCount );

    // Restore the original layer set, so the next page can be printed
    m_settings.m_LayerSet = lset;

    return true;
}


int PCBNEW_PRINTOUT::milsToIU( double aMils ) const
{
    return KiROUND( IU_PER_MILS * aMils );
}


void PCBNEW_PRINTOUT::setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet )
{
    BOARD_PRINTOUT::setupViewLayers( aView, aLayerSet );

    for( PCB_LAYER_ID layer : m_settings.m_LayerSet.Seq() )
    {
        aView.SetLayerVisible( PCBNEW_LAYER_ID_START + layer, true );

        // Enable the corresponding zone layer (copper layers and other layers)
        aView.SetLayerVisible( LAYER_ZONE_START + layer, true );
    }

    RENDER_SETTINGS* renderSettings = aView.GetPainter()->GetSettings();
    // A color to do not print objects on some layers, when the layer must be enabled
    // to print some other objects
    COLOR4D invisible_color( 0.0, 0.0, 0.0, 0.0 );

    if( m_pcbnewSettings.m_AsItemCheckboxes )
    {
        auto setVisibility =
                [&]( GAL_LAYER_ID aLayer )
                {
                    if( m_board->IsElementVisible( aLayer ) )
                        aView.SetLayerVisible( aLayer, true );
                    else
                        renderSettings->SetLayerColor( aLayer, invisible_color );
                };

        setVisibility( LAYER_MOD_FR );
        setVisibility( LAYER_MOD_BK );
        setVisibility( LAYER_MOD_VALUES );
        setVisibility( LAYER_MOD_REFERENCES );
        setVisibility( LAYER_MOD_TEXT_FR );
        setVisibility( LAYER_MOD_TEXT_BK );
        setVisibility( LAYER_MOD_TEXT_INVISIBLE );
        setVisibility( LAYER_PADS );
        setVisibility( LAYER_PAD_FR );
        setVisibility( LAYER_PAD_BK );
        setVisibility( LAYER_PADS_TH );

        setVisibility( LAYER_TRACKS );
        setVisibility( LAYER_VIAS );
        setVisibility( LAYER_VIA_MICROVIA );
        setVisibility( LAYER_VIA_BBLIND );
        setVisibility( LAYER_VIA_THROUGH );
        setVisibility( LAYER_ZONES );

        setVisibility( LAYER_NO_CONNECTS );
        setVisibility( LAYER_DRC_WARNING );
        setVisibility( LAYER_DRC_ERROR );
        setVisibility( LAYER_DRC_EXCLUSION );
        setVisibility( LAYER_ANCHOR );
        setVisibility( LAYER_DRAWINGSHEET );
        setVisibility( LAYER_GRID );
    }
    else
    {
        // Draw layers that must be not visible on printing are set to an invisible layer
        // LAYER_PAD_FR, LAYER_PAD_BK and LAYER_PADS_TH must be enabled to print pads on
        // technical layers, but not the pad on copper layer(s) if they are not enabled

        if( !aLayerSet.test( F_Cu ) )
            renderSettings->SetLayerColor( LAYER_PAD_FR, invisible_color );

        if( !aLayerSet.test( B_Cu ) )
            renderSettings->SetLayerColor( LAYER_PAD_BK, invisible_color );

        // Enable items on copper layers, but do not draw holes
        for( GAL_LAYER_ID layer : { LAYER_PADS_TH, LAYER_VIA_THROUGH } )
        {
            if( ( aLayerSet & LSET::AllCuMask() ).any() )   // Items visible on any copper layer
                aView.SetLayerVisible( layer, true );
            else
                renderSettings->SetLayerColor( layer, invisible_color );
        }

        // Keep certain items always enabled/disabled and just rely on the layer visibility
        // Note LAYER_PAD_FR, LAYER_PAD_BK, LAYER_PADS_TH are enabled here because paths must
        // be drawn on some other (technical) layers.
        const int alwaysEnabled[] =
                {
                    LAYER_MOD_TEXT_FR, LAYER_MOD_TEXT_BK, LAYER_MOD_FR, LAYER_MOD_BK,
                    LAYER_MOD_VALUES, LAYER_MOD_REFERENCES, LAYER_TRACKS, LAYER_ZONES, LAYER_PADS,
                    LAYER_VIAS, LAYER_VIA_MICROVIA, LAYER_VIA_BBLIND,
                    LAYER_PAD_FR, LAYER_PAD_BK, LAYER_PADS_TH
                };

        for( int layer : alwaysEnabled )
            aView.SetLayerVisible( layer, true );
    }

    if( m_pcbnewSettings.m_DrillMarks != PCBNEW_PRINTOUT_SETTINGS::NO_DRILL_SHAPE )
    {
        // Enable hole layers to draw drill marks
        for( int layer : { LAYER_PAD_PLATEDHOLES, LAYER_NON_PLATEDHOLES, LAYER_VIA_HOLES } )
        {
            aView.SetLayerVisible( layer, true );
            aView.SetTopLayer( layer, true );
        }
    }
}


void PCBNEW_PRINTOUT::setupPainter( KIGFX::PAINTER& aPainter )
{
    BOARD_PRINTOUT::setupPainter( aPainter );

    KIGFX::PCB_PRINT_PAINTER& painter = dynamic_cast<KIGFX::PCB_PRINT_PAINTER&>( aPainter );

    switch( m_pcbnewSettings.m_DrillMarks )
    {
    case PCBNEW_PRINTOUT_SETTINGS::NO_DRILL_SHAPE:
        painter.SetDrillMarks( false, 0 );
        break;

    case PCBNEW_PRINTOUT_SETTINGS::SMALL_DRILL_SHAPE:
        painter.SetDrillMarks( false, Millimeter2iu( ADVANCED_CFG::GetCfg().m_SmallDrillMarkSize ) );

        painter.GetSettings()->SetLayerColor( LAYER_PAD_PLATEDHOLES, COLOR4D::BLACK );
        painter.GetSettings()->SetLayerColor( LAYER_NON_PLATEDHOLES, COLOR4D::BLACK );
        painter.GetSettings()->SetLayerColor( LAYER_VIA_HOLES, COLOR4D::BLACK );
        break;

    case PCBNEW_PRINTOUT_SETTINGS::FULL_DRILL_SHAPE:
        painter.SetDrillMarks( true );

        painter.GetSettings()->SetLayerColor( LAYER_PAD_PLATEDHOLES, COLOR4D::BLACK );
        painter.GetSettings()->SetLayerColor( LAYER_NON_PLATEDHOLES, COLOR4D::BLACK );
        painter.GetSettings()->SetLayerColor( LAYER_VIA_HOLES, COLOR4D::BLACK );
        break;
    }

    painter.GetSettings()->SetDrawIndividualViaLayers(
            m_pcbnewSettings.m_Pagination == PCBNEW_PRINTOUT_SETTINGS::LAYER_PER_PAGE );
}


void PCBNEW_PRINTOUT::setupGal( KIGFX::GAL* aGal )
{
    BOARD_PRINTOUT::setupGal( aGal );
    aGal->SetWorldUnitLength( 0.001/IU_PER_MM /* 1 nm */ / 0.0254 /* 1 inch in meters */ );
}


EDA_RECT PCBNEW_PRINTOUT::getBoundingBox()
{
    return m_board->ComputeBoundingBox();
}


std::unique_ptr<KIGFX::PAINTER> PCBNEW_PRINTOUT::getPainter( KIGFX::GAL* aGal )
{
    return std::make_unique<KIGFX::PCB_PRINT_PAINTER>( aGal );
}


KIGFX::PCB_PRINT_PAINTER::PCB_PRINT_PAINTER( GAL* aGal ) :
        PCB_PAINTER( aGal ),
        m_drillMarkReal( false ),
        m_drillMarkSize( 0 )
{
    m_pcbSettings.EnableZoneOutlines( false );
}


int KIGFX::PCB_PRINT_PAINTER::getDrillShape( const PAD* aPad ) const
{
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillShape( aPad ) : PAD_DRILL_SHAPE_CIRCLE;
}


VECTOR2D KIGFX::PCB_PRINT_PAINTER::getDrillSize( const PAD* aPad ) const
{
    // TODO should it depend on the pad size?
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillSize( aPad ) :
        VECTOR2D( m_drillMarkSize, m_drillMarkSize );
}


int KIGFX::PCB_PRINT_PAINTER::getDrillSize( const PCB_VIA* aVia ) const
{
    // TODO should it depend on the via size?
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillSize( aVia ) : m_drillMarkSize;
}
