/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <collectors.h>
#include <board_item.h>             // class BOARD_ITEM

#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <pcb_dimension.h>
#include <zone.h>
#include <pcb_shape.h>
#include <pcb_group.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND


const std::vector<KICAD_T> GENERAL_COLLECTOR::AllBoardItems = {
    PCB_MARKER_T,           // in m_markers
    PCB_TEXT_T,             // in m_drawings
    PCB_REFERENCE_IMAGE_T,  // in m_drawings
    PCB_TEXTBOX_T,          // in m_drawings
    PCB_TABLE_T,            // in m_drawings
    PCB_TABLECELL_T,        // in tables
    PCB_SHAPE_T,            // in m_drawings
    PCB_DIM_ALIGNED_T,      // in m_drawings
    PCB_DIM_CENTER_T,       // in m_drawings
    PCB_DIM_RADIAL_T,       // in m_drawings
    PCB_DIM_ORTHOGONAL_T,   // in m_drawings
    PCB_DIM_LEADER_T,       // in m_drawings
    PCB_TARGET_T,           // in m_drawings
    PCB_VIA_T,              // in m_tracks
    PCB_TRACE_T,            // in m_tracks
    PCB_ARC_T,              // in m_tracks
    PCB_PAD_T,              // in footprints
    PCB_FIELD_T,            // in footprints
    PCB_FOOTPRINT_T,        // in m_footprints
    PCB_GROUP_T,            // in m_groups
    PCB_ZONE_T,             // in m_zones
    PCB_POINT_T,            // in m_points
    PCB_GENERATOR_T         // in m_generators
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::BoardLevelItems = {
    PCB_MARKER_T,
    PCB_REFERENCE_IMAGE_T,
    PCB_TEXT_T,
    PCB_TEXTBOX_T,
    PCB_TABLE_T,
    PCB_SHAPE_T,
    PCB_DIM_ALIGNED_T,
    PCB_DIM_ORTHOGONAL_T,
    PCB_DIM_CENTER_T,
    PCB_DIM_RADIAL_T,
    PCB_DIM_LEADER_T,
    PCB_TARGET_T,
    PCB_POINT_T,
    PCB_VIA_T,
    PCB_ARC_T,
    PCB_TRACE_T,
    PCB_FOOTPRINT_T,
    PCB_GROUP_T,
    PCB_ZONE_T,
    PCB_GENERATOR_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::Footprints = {
    PCB_FOOTPRINT_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::PadsOrTracks = {
    PCB_PAD_T,
    PCB_VIA_T,
    PCB_TRACE_T,
    PCB_ARC_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::FootprintItems = {
    PCB_MARKER_T,
    PCB_FIELD_T,
    PCB_TEXT_T,
    PCB_TEXTBOX_T,
    PCB_TABLE_T,
    PCB_TABLECELL_T,
    PCB_SHAPE_T,
    PCB_DIM_ALIGNED_T,
    PCB_DIM_ORTHOGONAL_T,
    PCB_DIM_CENTER_T,
    PCB_DIM_RADIAL_T,
    PCB_DIM_LEADER_T,
    PCB_PAD_T,
    PCB_ZONE_T,
    PCB_GROUP_T,
    PCB_POINT_T,
    PCB_REFERENCE_IMAGE_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::Tracks = {
    PCB_TRACE_T,
    PCB_ARC_T,
    PCB_VIA_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::Dimensions = {
    PCB_DIM_ALIGNED_T,
    PCB_DIM_LEADER_T,
    PCB_DIM_ORTHOGONAL_T,
    PCB_DIM_CENTER_T,
    PCB_DIM_RADIAL_T
};


const std::vector<KICAD_T> GENERAL_COLLECTOR::DraggableItems = {
    PCB_TRACE_T,
    PCB_VIA_T,
    PCB_FOOTPRINT_T,
    PCB_ARC_T
};


INSPECT_RESULT GENERAL_COLLECTOR::Inspect( EDA_ITEM* aTestItem, void* aTestData )
{
    BOARD_ITEM*         boardItem   = nullptr;
    FOOTPRINT*          footprint   = nullptr;
    PCB_GROUP*          group       = nullptr;
    PAD*                pad         = nullptr;
    bool                pad_through = false;
    PCB_VIA*            via         = nullptr;
    PCB_MARKER*         marker      = nullptr;
    ZONE*               zone        = nullptr;
    PCB_FIELD*          field       = nullptr;
    PCB_TEXT*           text        = nullptr;
    PCB_DIMENSION_BASE* dimension   = nullptr;
    PCB_SHAPE*          shape       = nullptr;

    switch( aTestItem->Type() )
    {
    case PCB_PAD_T:
        // there are pad specific visibility controls.
        // Criteria to select a pad is:
        // for smd pads: the footprint parent must be visible, and pads on the corresponding
        // board side must be visible
        // if pad is a thru hole, then it can be visible when its parent footprint is not.
        // for through pads: pads on Front or Back board sides must be visible
        pad = static_cast<PAD*>( aTestItem );
        boardItem = pad;

        if( ( pad->GetAttribute() != PAD_ATTRIB::SMD ) &&
            ( pad->GetAttribute() != PAD_ATTRIB::CONN ) )  // a hole is present, so multiple layers
        {
            // proceed to the common tests below, but without the parent footprint test,
            // by leaving footprint==NULL, but having pad != null
            pad_through = true;
        }

        break;

    case PCB_VIA_T:     // vias are on many layers, so layer test is specific
        via = static_cast<PCB_VIA*>( aTestItem );
        boardItem = via;
        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        if( m_Guide->IgnoreTracks() )
            return INSPECT_RESULT::CONTINUE;

        boardItem = static_cast<PCB_TRACK*>( aTestItem );
        break;

    case PCB_ZONE_T:
        zone = static_cast<ZONE*>( aTestItem );

        if( m_Guide->IgnoreNoNets() && zone->GetNetCode() == NETINFO_LIST::UNCONNECTED )
            return INSPECT_RESULT::CONTINUE;

        boardItem = zone;
        break;

    case PCB_SHAPE_T:
        shape = static_cast<PCB_SHAPE*>( aTestItem );

        if( m_Guide->IgnoreNoNets() && shape->GetNetCode() == NETINFO_LIST::UNCONNECTED )
            return INSPECT_RESULT::CONTINUE;

        boardItem = shape;
        break;

    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TABLECELL_T:
        if( m_Guide->IgnoreNoNets() )
            return INSPECT_RESULT::CONTINUE;

        boardItem = static_cast<BOARD_ITEM*>( aTestItem );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        if( m_Guide->IgnoreNoNets() )
            return INSPECT_RESULT::CONTINUE;

        dimension = static_cast<PCB_DIMENSION_BASE*>( aTestItem );
        boardItem = dimension;
        break;

    case PCB_TARGET_T:
        if( m_Guide->IgnoreNoNets() )
            return INSPECT_RESULT::CONTINUE;

        boardItem = static_cast<BOARD_ITEM*>( aTestItem );
        break;

    case PCB_POINT_T:
        boardItem = static_cast<BOARD_ITEM*>( aTestItem );
        break;

    case PCB_FIELD_T:
        if( m_Guide->IgnoreNoNets() )
            return INSPECT_RESULT::CONTINUE;

        field = static_cast<PCB_FIELD*>( aTestItem );

        if( !field->IsVisible() )
            return INSPECT_RESULT::CONTINUE;

        if( field->IsReference() && m_Guide->IgnoreFPReferences() )
            return INSPECT_RESULT::CONTINUE;

        if( field->IsValue() && m_Guide->IgnoreFPValues() )
            return INSPECT_RESULT::CONTINUE;

        KI_FALLTHROUGH;

    case PCB_TEXT_T:
        if( m_Guide->IgnoreNoNets() )
            return INSPECT_RESULT::CONTINUE;

        text = static_cast<PCB_TEXT*>( aTestItem );
        boardItem = text;

        if( text->GetParentFootprint() )
        {
            PCB_LAYER_ID layer = text->GetLayer();

            if( m_Guide->IgnoreFPTextOnBack() && IsBackLayer( layer ) )
                return INSPECT_RESULT::CONTINUE;

            if( m_Guide->IgnoreFPTextOnFront() && IsFrontLayer( layer ) )
                return INSPECT_RESULT::CONTINUE;
        }

        break;

    case PCB_FOOTPRINT_T:
        footprint = static_cast<FOOTPRINT*>( aTestItem );
        boardItem = footprint;
        break;

    case PCB_GROUP_T:
        group = static_cast<PCB_GROUP*>( aTestItem );
        boardItem = group;
        break;

    case PCB_MARKER_T:
        marker = static_cast<PCB_MARKER*>( aTestItem );
        boardItem = marker;
        break;

    default:
        if( aTestItem->IsBOARD_ITEM() )
            boardItem = static_cast<BOARD_ITEM*>( aTestItem );

        break;
    }

    if( boardItem && !footprint )
        footprint = boardItem->GetParentFootprint();

    // common tests:

    if( footprint )
    {
        if( m_Guide->IgnoreFootprintsOnBack() && footprint->GetSide() == B_Cu )
            return INSPECT_RESULT::CONTINUE;

        if( m_Guide->IgnoreFootprintsOnFront() && footprint->GetSide() == F_Cu )
            return INSPECT_RESULT::CONTINUE;
    }

    // Pads are not sensitive to the layer visibility controls; they all have their own separate
    // visibility controls.
    if( pad )
    {
        if( m_Guide->IgnorePads() )
            return INSPECT_RESULT::CONTINUE;

        if( ! pad_through )
        {
            if( m_Guide->IgnorePadsOnFront() && pad->IsOnLayer(F_Cu ) )
                return INSPECT_RESULT::CONTINUE;

            if( m_Guide->IgnorePadsOnBack() && pad->IsOnLayer(B_Cu ) )
                return INSPECT_RESULT::CONTINUE;
        }
    }

    if( marker )
    {
        // Markers are not sensitive to the layer
        if( marker->HitTest( m_refPos ) )
            Append( aTestItem );

        return INSPECT_RESULT::CONTINUE;
    }

    if( group )
    {
        // Groups are not sensitive to the layer ... ?
        if( group->HitTest( m_refPos ) )
            Append( aTestItem );

        return INSPECT_RESULT::CONTINUE;
    }

    if( via )
    {
        auto type = via->GetViaType();

        if( ( m_Guide->IgnoreThroughVias() && type == VIATYPE::THROUGH )
                || ( m_Guide->IgnoreBlindBuriedVias() && type == VIATYPE::BLIND_BURIED )
                || ( m_Guide->IgnoreMicroVias() && type == VIATYPE::MICROVIA ) )
        {
            return INSPECT_RESULT::CONTINUE;
        }
    }

    if( boardItem
            && ( boardItem->IsOnLayer( m_Guide->GetPreferredLayer() ) )
            && ( !boardItem->IsLocked() || !m_Guide->IgnoreLockedItems() ) )
    {
        // Footprints and their subcomponents: reference, value and pads are not sensitive to the
        // layer visibility controls; they all have their own separate visibility controls.
        // For vias, GetLayer() has no meaning, but IsOnLayer() works fine.
        // User text and fields in a footprint *are* sensitive to layer visibility but they were
        // already handled.

        int accuracy = m_Guide->Accuracy();

        if( zone )
        {
            if( zone->HitTestForCorner( m_refPos, accuracy * 2 ) || zone->HitTestForEdge( m_refPos, accuracy ) )
            {
                Append( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
            else if( !m_Guide->IgnoreZoneFills() )
            {
                for( PCB_LAYER_ID layer : zone->GetLayerSet() )
                {
                    if( m_Guide->IsLayerVisible( layer ) && zone->HitTestFilledArea( layer, m_refPos ) )
                    {
                        Append( aTestItem );
                        return INSPECT_RESULT::CONTINUE;
                    }
                }
            }
        }
        else if( aTestItem == footprint )
        {
            if( footprint->HitTest( m_refPos, accuracy ) && footprint->HitTestAccurate( m_refPos, accuracy ) )
            {
                Append( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
        }
        else if( pad || via )
        {
            if( aTestItem->HitTest( m_refPos, accuracy ) )
            {
                Append( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
        }
        else
        {
            PCB_LAYER_ID layer = boardItem->GetLayer();

            if( m_Guide->IsLayerVisible( layer ) )
            {
                if( dimension )
                {
                    // Dimensions feel particularly hard to select, probably due to their noisy
                    // shape making it feel like they should have a larger boundary.
                    accuracy = KiROUND( accuracy * 1.5 );
                }

                if( aTestItem->HitTest( m_refPos, accuracy ) )
                {
                    Append( aTestItem );
                    return INSPECT_RESULT::CONTINUE;
                }
            }
        }
    }

    if( m_Guide->IncludeSecondary()
            && ( !boardItem || !boardItem->IsLocked() || !m_Guide->IgnoreLockedItems() ) )
    {
        // For now, "secondary" means "tolerate any visible layer".  It has no effect on other
        // criteria, since there is a separate "ignore" control for those in the COLLECTORS_GUIDE

        // Footprints and their subcomponents: reference, value and pads are not sensitive to the
        // layer visibility controls; they all have their own separate visibility controls.
        // For vias, GetLayer() has no meaning, but IsOnLayer() works fine.
        // User text and fields in a footprint *are* sensitive to layer visibility but they were
        // already handled.

        int accuracy = m_Guide->Accuracy();

        if( zone )
        {
            if( zone->HitTestForCorner( m_refPos, accuracy * 2 ) || zone->HitTestForEdge( m_refPos, accuracy ) )
            {
                Append2nd( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
            else if( !m_Guide->IgnoreZoneFills() )
            {
                for( PCB_LAYER_ID layer : zone->GetLayerSet() )
                {
                    if( m_Guide->IsLayerVisible( layer ) && zone->HitTestFilledArea( layer, m_refPos ) )
                    {
                        Append2nd( aTestItem );
                        return INSPECT_RESULT::CONTINUE;
                    }
                }
            }
        }
        else if( aTestItem->Type() == PCB_FOOTPRINT_T )
        {
            // Already tested above, but Coverity can't figure that out
            wxCHECK( footprint, INSPECT_RESULT::CONTINUE );

            if( footprint->HitTest( m_refPos, accuracy ) && footprint->HitTestAccurate( m_refPos, accuracy ) )
            {
                Append2nd( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
        }
        else if( pad || via )
        {
            if( aTestItem->HitTest( m_refPos, accuracy ) )
            {
                Append2nd( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
        }
        else if( boardItem && m_Guide->IsLayerVisible( boardItem->GetLayer() ) )
        {
            if( dimension )
            {
                // Dimensions feel particularly hard to select, probably due to their noisy shape
                // making it feel like they should have a larger boundary.
                accuracy = KiROUND( accuracy * 1.5 );
            }

            if( aTestItem->HitTest( m_refPos, accuracy ) )
            {
                Append2nd( aTestItem );
                return INSPECT_RESULT::CONTINUE;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE; // always when collecting
}


void GENERAL_COLLECTOR::Collect( BOARD_ITEM* aItem, const std::vector<KICAD_T>& aScanTypes,
                                 const VECTOR2I& aRefPos, const COLLECTORS_GUIDE& aGuide )
{
    Empty();        // empty the collection, primary criteria list
    Empty2nd();     // empty the collection, secondary criteria list

    // remember guide, pass it to Inspect()
    SetGuide( &aGuide );

    SetScanTypes( aScanTypes );

    // remember where the snapshot was taken from and pass refPos to
    // the Inspect() function.
    SetRefPos( aRefPos );

    wxCHECK_RET( aItem, "" );
    aItem->Visit( m_inspector, nullptr, m_scanTypes );

    // append 2nd list onto end of the first list
    for( EDA_ITEM* item : m_List2nd )
        Append( item );

    Empty2nd();
}


INSPECT_RESULT PCB_TYPE_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    // The Visit() function only visits the testItem if its type was in the the scanList,
    // so therefore we can collect anything given to us here.
    Append( testItem );

    return INSPECT_RESULT::CONTINUE; // always when collecting
}


void PCB_TYPE_COLLECTOR::Collect( BOARD_ITEM* aBoard, const std::vector<KICAD_T>& aTypes )
{
    Empty();
    aBoard->Visit( m_inspector, nullptr, aTypes );
}


INSPECT_RESULT PCB_LAYER_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    BOARD_ITEM* item = (BOARD_ITEM*) testItem;

    if( item->IsOnLayer( m_layer_id ) )
        Append( testItem );

    return INSPECT_RESULT::CONTINUE;
}


void PCB_LAYER_COLLECTOR::Collect( BOARD_ITEM* aBoard, const std::vector<KICAD_T>& aTypes )
{
    Empty();
    aBoard->Visit( m_inspector, nullptr, aTypes );
}
