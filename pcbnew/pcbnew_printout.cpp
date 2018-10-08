/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_board.h>

#include <pcb_painter.h>
#include <view/view.h>
#include <pcbplot.h>

PCBNEW_PRINTOUT_SETTINGS::PCBNEW_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
    : BOARD_PRINTOUT_SETTINGS( aPageInfo )
{
    m_drillMarks = SMALL_DRILL_SHAPE;
    m_pagination = ALL_LAYERS;
    m_noEdgeLayer = false;
}


void PCBNEW_PRINTOUT_SETTINGS::Load( wxConfigBase* aConfig )
{
    BOARD_PRINTOUT_SETTINGS::Load( aConfig );
    aConfig->Read( OPTKEY_PRINT_PADS_DRILL, (int*) &m_drillMarks, FULL_DRILL_SHAPE );
    aConfig->Read( OPTKEY_PRINT_PAGE_PER_LAYER, (int*) &m_pagination, ALL_LAYERS );
}


void PCBNEW_PRINTOUT_SETTINGS::Save( wxConfigBase* aConfig )
{
    BOARD_PRINTOUT_SETTINGS::Save( aConfig );
    aConfig->Write( OPTKEY_PRINT_PADS_DRILL, (int) m_drillMarks );
    aConfig->Write( OPTKEY_PRINT_PAGE_PER_LAYER, (int) m_pagination );
}


PCBNEW_PRINTOUT::PCBNEW_PRINTOUT( BOARD* aBoard, const PCBNEW_PRINTOUT_SETTINGS& aParams,
        const KIGFX::VIEW* aView, const wxString& aTitle ) :
    BOARD_PRINTOUT( aParams, aView, aTitle ), m_pcbnewSettings( aParams )
{
    m_board = aBoard;
}


bool PCBNEW_PRINTOUT::OnPrintPage( int aPage )
{
    // Store the layerset, as it is going to be modified below and the original settings are needed
    LSET lset = m_settings.m_layerSet;
    int pageCount = lset.count();
    wxString layer;
    PCB_LAYER_ID extractLayer;

    // compute layer mask from page number if we want one page per layer
    if( m_pcbnewSettings.m_pagination == 0 )  // One page per layer
    {
        // This sequence is TBD, call a different
        // sequencer if needed, such as Seq().  Could not find documentation on
        // page order.
        LSEQ seq = lset.UIOrder();

        // aPage starts at 1, not 0
        if( unsigned( aPage - 1 ) < seq.size() )
            m_settings.m_layerSet = LSET( seq[aPage - 1] );
    }

    if( !m_settings.m_layerSet.any() )
        return false;

    extractLayer = m_settings.m_layerSet.ExtractLayer();

    if( extractLayer == UNDEFINED_LAYER )
        layer = _( "Multiple Layers" );
    else
        layer = LSET::Name( extractLayer );

    // In Pcbnew we can want the layer EDGE always printed
    if( !m_pcbnewSettings.m_noEdgeLayer )
        m_settings.m_layerSet.set( Edge_Cuts );

    DrawPage( layer, aPage, pageCount );

    // Restore the original layer set, so the next page can be printed
    m_settings.m_layerSet = lset;

    return true;
}


int PCBNEW_PRINTOUT::milsToIU( double aMils ) const
{
    return KiROUND( IU_PER_MILS * aMils );
}


void PCBNEW_PRINTOUT::setupViewLayers( const std::unique_ptr<KIGFX::VIEW>& aView,
        const LSET& aLayerSet )
{
    BOARD_PRINTOUT::setupViewLayers( aView, aLayerSet );

    for( LSEQ layerSeq = m_settings.m_layerSet.Seq(); layerSeq; ++layerSeq )
        aView->SetLayerVisible( PCBNEW_LAYER_ID_START + *layerSeq, true );

    // Enable pad layers corresponding to the selected copper layers
    if( aLayerSet.test( F_Cu ) )
        aView->SetLayerVisible( LAYER_PAD_FR, true );

    if( aLayerSet.test( B_Cu ) )
        aView->SetLayerVisible( LAYER_PAD_BK, true );

    if( ( aLayerSet & LSET::AllCuMask() ).any() )   // Items visible on any copper layer
    {
        // Enable items on copper layers, but do not draw holes
        for( auto item : { LAYER_PADS_TH, LAYER_VIA_MICROVIA,
                LAYER_VIA_BBLIND, LAYER_VIA_THROUGH } )
        {
            aView->SetLayerVisible( item, true );
        }

        if( m_pcbnewSettings.m_drillMarks != PCBNEW_PRINTOUT_SETTINGS::NO_DRILL_SHAPE )
        {
            // Enable hole layers to draw drill marks
            for( auto holeLayer : { LAYER_PADS_PLATEDHOLES,
                    LAYER_NON_PLATEDHOLES, LAYER_VIAS_HOLES })
            {
                aView->SetLayerVisible( holeLayer, true );
                aView->SetTopLayer( holeLayer, true );
            }
        }

    }


    // Keep certain items always enabled/disabled and just rely on the layer visibility
    const int alwaysEnabled[] = {
        LAYER_MOD_TEXT_FR, LAYER_MOD_TEXT_BK, LAYER_MOD_FR, LAYER_MOD_BK,
        LAYER_MOD_VALUES, LAYER_MOD_REFERENCES, LAYER_TRACKS
    };

    for( int item : alwaysEnabled )
        aView->SetLayerVisible( item, true );
}


void PCBNEW_PRINTOUT::setupPainter( const std::unique_ptr<KIGFX::PAINTER>& aPainter )
{
    BOARD_PRINTOUT::setupPainter( aPainter );

    auto painter = static_cast<KIGFX::PCB_PRINT_PAINTER*>( aPainter.get() );

    switch( m_pcbnewSettings.m_drillMarks )
    {
        case PCBNEW_PRINTOUT_SETTINGS::NO_DRILL_SHAPE:
            painter->SetDrillMarks( false, 0 );
            break;

        case PCBNEW_PRINTOUT_SETTINGS::SMALL_DRILL_SHAPE:
            painter->SetDrillMarks( false, Millimeter2iu( 0.3 ) );
            break;

        case PCBNEW_PRINTOUT_SETTINGS::FULL_DRILL_SHAPE:
            painter->SetDrillMarks( true );
            break;
    }

    painter->GetSettings()->SetLayerColor( LAYER_PADS_PLATEDHOLES, COLOR4D::WHITE );
    painter->GetSettings()->SetLayerColor( LAYER_NON_PLATEDHOLES, COLOR4D::WHITE );
    painter->GetSettings()->SetLayerColor( LAYER_VIAS_HOLES, COLOR4D::WHITE );
}


void PCBNEW_PRINTOUT::setupGal( KIGFX::GAL* aGal )
{
    BOARD_PRINTOUT::setupGal( aGal );
    aGal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );
}


EDA_RECT PCBNEW_PRINTOUT::getBoundingBox()
{
    return m_board->ComputeBoundingBox();
}


std::unique_ptr<KIGFX::PAINTER> PCBNEW_PRINTOUT::getPainter( KIGFX::GAL* aGal )
{
    return std::unique_ptr<KIGFX::PAINTER>( new KIGFX::PCB_PRINT_PAINTER( aGal ) );
}


KIGFX::PCB_PRINT_PAINTER::PCB_PRINT_PAINTER( GAL* aGal )
    : PCB_PAINTER( aGal ), m_drillMarkReal( false ), m_drillMarkSize( 0 )
{
    m_pcbSettings.EnableZoneOutlines( false );
}


int KIGFX::PCB_PRINT_PAINTER::getDrillShape( const D_PAD* aPad ) const
{
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillShape( aPad ) : PAD_DRILL_SHAPE_CIRCLE;
}


VECTOR2D KIGFX::PCB_PRINT_PAINTER::getDrillSize( const D_PAD* aPad ) const
{
    // TODO should it depend on the pad size?
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillSize( aPad ) :
        VECTOR2D( m_drillMarkSize, m_drillMarkSize );
}


int KIGFX::PCB_PRINT_PAINTER::getDrillSize( const VIA* aVia ) const
{
    // TODO should it depend on the via size?
    return m_drillMarkReal ? KIGFX::PCB_PAINTER::getDrillSize( aVia ) : m_drillMarkSize;
}
