/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <drill/drill_enumerator.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>


namespace
{

DRILL_POST_MACHINING fromPadstack( const PADSTACK::POST_MACHINING_PROPS& aProps )
{
    DRILL_POST_MACHINING pm;

    pm.m_Mode = aProps.mode.value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
    pm.m_Size = aProps.size;
    pm.m_Depth = aProps.depth;
    pm.m_Angle = aProps.angle;

    return pm;
}


void enumerateVia( PCB_VIA* aVia, const BOARD& aBoard, const DRILL_QUERY& aQuery,
                   std::vector<DRILL_OPERATION>& aOut )
{
    const DRILL_SPAN& span = aQuery.m_Span;

    auto stubLength =
            [&]( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd ) -> std::optional<int>
            {
                if( aStart == UNDEFINED_LAYER || aEnd == UNDEFINED_LAYER )
                    return std::optional<int>();

                const BOARD_STACKUP& stackup = aBoard.GetDesignSettings().GetStackupDescriptor();

                return std::optional<int>( stackup.GetLayerDistance( aStart, aEnd ) );
            };

    if( span.m_IsBackdrill )
    {
        auto tryBackdrill =
                [&]( const PADSTACK::DRILL_PROPS& aDrill, DRILL_OP_KIND aKind )
                {
                    if( aDrill.start == UNDEFINED_LAYER || aDrill.end == UNDEFINED_LAYER )
                        return;

                    DRILL_SPAN drillSpan( aDrill.start, aDrill.end, true, false );

                    if( drillSpan.Pair() != span.Pair() )
                        return;

                    if( aDrill.start != span.DrillStartLayer() || aDrill.end != span.DrillEndLayer() )
                        return;

                    if( aDrill.size.x <= 0 && aDrill.size.y <= 0 )
                        return;

                    DRILL_OPERATION op;
                    op.m_Kind = aKind;
                    op.m_SourceItem = aVia;
                    op.m_SourceId = aVia->m_Uuid;
                    op.m_Attribute = HOLE_ATTRIBUTE::HOLE_VIA_BACKDRILL;
                    op.m_Orientation = ANGLE_0;
                    op.m_NotPlated = true;
                    op.m_Position = aVia->GetStart();
                    op.m_TopLayer = span.TopLayer();
                    op.m_BottomLayer = span.BottomLayer();

                    int diameter = aDrill.size.x;

                    if( aDrill.size.y > 0 )
                        diameter = ( diameter > 0 ) ? std::min( diameter, aDrill.size.y ) : aDrill.size.y;

                    op.m_Diameter = diameter;
                    op.m_SizeXY = aDrill.size;

                    if( aDrill.shape != PAD_DRILL_SHAPE::CIRCLE && aDrill.size.x != aDrill.size.y )
                        op.m_IsSlot = true;

                    op.m_Filled = aDrill.is_filled.value_or( false );
                    op.m_Capped = aDrill.is_capped.value_or( false );
                    op.m_TopCovered = aVia->Padstack().IsCovered( op.m_TopLayer ).value_or( false );
                    op.m_BottomCovered = aVia->Padstack().IsCovered( op.m_BottomLayer ).value_or( false );
                    op.m_TopPlugged = aVia->Padstack().IsPlugged( op.m_TopLayer ).value_or( false );
                    op.m_BottomPlugged = aVia->Padstack().IsPlugged( op.m_BottomLayer ).value_or( false );
                    op.m_TopTented = aVia->Padstack().IsTented( op.m_TopLayer ).value_or( false );
                    op.m_BottomTented = aVia->Padstack().IsTented( op.m_BottomLayer ).value_or( false );
                    op.m_DrillStart = aDrill.start;
                    op.m_DrillEnd = aDrill.end;
                    op.m_StubLength = stubLength( aDrill.start, aDrill.end );

                    aOut.push_back( op );
                };

        tryBackdrill( aVia->Padstack().SecondaryDrill(), DRILL_OP_KIND::SECONDARY_DRILL );
        tryBackdrill( aVia->Padstack().TertiaryDrill(), DRILL_OP_KIND::TERTIARY_DRILL );
        return;
    }

    int holeSize = aVia->GetDrillValue();

    if( holeSize == 0 )
        return;

    PCB_LAYER_ID topLayer;
    PCB_LAYER_ID bottomLayer;
    aVia->LayerPair( &topLayer, &bottomLayer );

    if( DRILL_LAYER_PAIR( topLayer, bottomLayer ) != span.Pair()
        && DRILL_LAYER_PAIR( bottomLayer, topLayer ) != span.Pair() )
    {
        return;
    }

    DRILL_OPERATION op;
    op.m_SourceItem = aVia;
    op.m_SourceId = aVia->m_Uuid;

    if( span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_VIA_THROUGH;
    else
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_VIA_BURIED;

    op.m_Orientation = ANGLE_0;
    op.m_Diameter = holeSize;
    op.m_NotPlated = false;
    op.m_SizeXY = VECTOR2I( holeSize, holeSize );
    op.m_Position = aVia->GetStart();
    op.m_TopLayer = topLayer;
    op.m_BottomLayer = bottomLayer;
    op.m_Filled = aVia->Padstack().IsFilled().value_or( false );
    op.m_Capped = aVia->Padstack().IsCapped().value_or( false );
    op.m_TopCovered = aVia->Padstack().IsCovered( topLayer ).value_or( false );
    op.m_BottomCovered = aVia->Padstack().IsCovered( bottomLayer ).value_or( false );
    op.m_TopPlugged = aVia->Padstack().IsPlugged( topLayer ).value_or( false );
    op.m_BottomPlugged = aVia->Padstack().IsPlugged( bottomLayer ).value_or( false );
    op.m_TopTented = aVia->Padstack().IsTented( topLayer ).value_or( false );
    op.m_BottomTented = aVia->Padstack().IsTented( bottomLayer ).value_or( false );
    op.m_FrontPostMachining = fromPadstack( aVia->Padstack().FrontPostMachining() );
    op.m_BackPostMachining = fromPadstack( aVia->Padstack().BackPostMachining() );
    op.m_DrillStart = aVia->Padstack().Drill().start;
    op.m_DrillEnd = bottomLayer;

    aOut.push_back( op );
}


void enumeratePad( PAD* aPad, const DRILL_QUERY& aQuery, std::vector<DRILL_OPERATION>& aOut )
{
    if( !aQuery.m_MergePTHNPTH )
    {
        if( !aQuery.m_NonPlatedOnly && aPad->GetAttribute() == PAD_ATTRIB::NPTH )
            return;

        if( aQuery.m_NonPlatedOnly && aPad->GetAttribute() != PAD_ATTRIB::NPTH )
            return;
    }

    if( aPad->GetDrillSize().x == 0 )
        return;

    DRILL_OPERATION op;
    op.m_SourceItem = aPad;
    op.m_SourceId = aPad->m_Uuid;
    op.m_NotPlated = ( aPad->GetAttribute() == PAD_ATTRIB::NPTH );

    if( op.m_NotPlated )
    {
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_MECHANICAL;
    }
    else if( aPad->GetProperty() == PAD_PROP::CASTELLATED )
    {
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED;
    }
    else if( aPad->GetProperty() == PAD_PROP::PRESSFIT )
    {
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_PAD_PRESSFIT;
    }
    else
    {
        op.m_Attribute = HOLE_ATTRIBUTE::HOLE_PAD;
    }

    op.m_Orientation = aPad->GetOrientation();
    op.m_Diameter = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    op.m_IsSlot = IsDrillSlot( *aPad );

    op.m_SizeXY = aPad->GetDrillSize();
    op.m_Position = aPad->GetPosition();
    op.m_BottomLayer = B_Cu;
    op.m_TopLayer = F_Cu;

    // Pads have never carried post-machining into the drill files and adding it here would
    // move fabrication output

    aOut.push_back( op );
}

} // namespace


bool IsDrillSlot( const PAD& aPad )
{
    return aPad.HasHole() && aPad.GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE
           && aPad.GetDrillSizeX() != aPad.GetDrillSizeY();
}


std::vector<DRILL_OPERATION> EnumerateDrillOperations( const BOARD& aBoard, const DRILL_QUERY& aQuery )
{
    std::vector<DRILL_OPERATION> operations;

    wxASSERT( IsCopperLayerLowerThan( aQuery.m_Span.BottomLayer(), aQuery.m_Span.TopLayer() ) );

    if( !aQuery.m_NonPlatedOnly )
    {
        for( PCB_TRACK* track : aBoard.Tracks() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            enumerateVia( static_cast<PCB_VIA*>( track ), aBoard, aQuery, operations );
        }
    }

    if( !aQuery.m_Span.m_IsBackdrill && aQuery.m_Span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
    {
        for( FOOTPRINT* footprint : aBoard.Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
                enumeratePad( pad, aQuery, operations );
        }
    }

    return operations;
}


