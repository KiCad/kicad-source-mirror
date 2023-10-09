/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_generator.h>
#include <generators_mgr.h>

#include <optional>
#include <magic_enum.hpp>

#include <wx/debug.h>
#include <geometry/shape_circle.h>
#include <kiplatform/ui.h>
#include <collectors.h>

#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_group.h>

#include <tool/edit_points.h>
#include <tools/drawing_tool.h>
#include <tools/generator_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>

#include <preview_items/draw_context.h>
#include <view/view.h>

#include <router/pns_meander_placer_base.h>
#include <router/pns_meander.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_segment.h>
#include <router/pns_arc.h>
#include <router/pns_topology.h>

#include <dialogs/dialog_meander_properties.h>


enum LENGTH_TUNING_MODE
{
    SINGLE,
    DIFF_PAIR,
    DIFF_PAIR_SKEW
};


static LENGTH_TUNING_MODE TuningFromString( const std::string& aStr )
{
    if( aStr == "single" )
        return LENGTH_TUNING_MODE::SINGLE;
    else if( aStr == "diff_pair" )
        return LENGTH_TUNING_MODE::DIFF_PAIR;
    else if( aStr == "diff_pair_skew" )
        return LENGTH_TUNING_MODE::DIFF_PAIR_SKEW;
    else
    {
        wxFAIL_MSG( wxS( "Unknown length tuning token" ) );
        return LENGTH_TUNING_MODE::SINGLE;
    }
}


static std::string TuningToString( const LENGTH_TUNING_MODE aTuning )
{
    switch( aTuning )
    {
    case LENGTH_TUNING_MODE::SINGLE: return "single";
    case LENGTH_TUNING_MODE::DIFF_PAIR: return "diff_pair";
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return "diff_pair_skew";
    default: wxFAIL; return "";
    }
}


static PNS::MEANDER_SIDE SideFromString( const std::string& aStr )
{
    if( aStr == "default" )
        return PNS::MEANDER_SIDE_DEFAULT;
    else if( aStr == "left" )
        return PNS::MEANDER_SIDE_LEFT;
    else if( aStr == "right" )
        return PNS::MEANDER_SIDE_RIGHT;
    else
    {
        wxFAIL_MSG( wxS( "Unknown meander side token" ) );
        return PNS::MEANDER_SIDE_DEFAULT;
    }
}


static std::string StatusToString( const PNS::MEANDER_PLACER_BASE::TUNING_STATUS aStatus )
{
    switch( aStatus )
    {
    case PNS::MEANDER_PLACER_BASE::TOO_LONG: return "too_long";
    case PNS::MEANDER_PLACER_BASE::TOO_SHORT: return "too_short";
    case PNS::MEANDER_PLACER_BASE::TUNED: return "tuned";
    default: wxFAIL; return "";
    }
}


static PNS::MEANDER_PLACER_BASE::TUNING_STATUS StatusFromString( const std::string& aStr )
{
    if( aStr == "too_long" )
        return PNS::MEANDER_PLACER_BASE::TOO_LONG;
    else if( aStr == "too_short" )
        return PNS::MEANDER_PLACER_BASE::TOO_SHORT;
    else if( aStr == "tuned" )
        return PNS::MEANDER_PLACER_BASE::TUNED;
    else
    {
        wxFAIL_MSG( wxS( "Unknown tuning status token" ) );
        return PNS::MEANDER_PLACER_BASE::TUNED;
    }
}


static std::string SideToString( const PNS::MEANDER_SIDE aValue )
{
    switch( aValue )
    {
    case PNS::MEANDER_SIDE_DEFAULT: return "default";
    case PNS::MEANDER_SIDE_LEFT: return "left";
    case PNS::MEANDER_SIDE_RIGHT: return "right";
    default: wxFAIL; return "";
    }
}


class PCB_GENERATOR_MEANDERS : public PCB_GENERATOR
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_GENERATOR_MEANDERS( BOARD_ITEM* aParent = nullptr, PCB_LAYER_ID aLayer = F_Cu ) :
            PCB_GENERATOR( aParent, aLayer )
    {
        m_generatorType = GENERATOR_TYPE;
        m_name = DISPLAY_NAME;

        m_minAmplitude = pcbIUScale.mmToIU( 0.1 );
        m_maxAmplitude = pcbIUScale.mmToIU( 2.0 );
        m_spacing = pcbIUScale.mmToIU( 0.6 );
        m_targetLength = pcbIUScale.mmToIU( 100 );
        m_targetSkew = pcbIUScale.mmToIU( 0 );
        m_end = VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 );
        m_cornerRadiusPercentage = 100;
        m_initialSide = PNS::MEANDER_SIDE_DEFAULT;
    }

    wxString GetGeneratorType() const override { return wxS( "meanders" ); }

    int snapToNearestTrackPoint( VECTOR2I& aP, BOARD* aBoard, int aNet )
    {
        SEG::ecoord minDistSq = VECTOR2I::ECOORD_MAX;
        VECTOR2I    closestPt = aP;
        int         closestNet = -1;

        for( PCB_TRACK *track : aBoard->Tracks() )
        {
            if( aNet >= 0 && track->GetNetCode() != aNet )
                continue;

            SEG seg ( track->GetStart(), track->GetEnd() );

            VECTOR2I    nearest = seg.NearestPoint( aP );
            SEG::ecoord distSq = ( nearest - aP ).SquaredEuclideanNorm();

            if( distSq < minDistSq )
            {
                minDistSq = distSq;
                closestPt = nearest;
                closestNet = track->GetNetCode();
            }
        }

        if( minDistSq != VECTOR2I::ECOORD_MAX )
        {
            aP = closestPt;
            return closestNet;
        }

        return -1;
    }

    bool baselineValid() { return m_baseLine && m_baseLine->PointCount() > 1; }

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                    BOARD_COMMIT* aCommit ) override
    {
        if( aCommit )
            aCommit->Modify( this );

        int          layer = GetLayer();
        PNS::ROUTER* router = aTool->Router();

        aTool->ClearRouterCommit();
        router->SyncWorld();

        if( !baselineValid() )
        {
            InitBaseLine( router, layer, aBoard );
        }
    }

    PNS::LINKED_ITEM* PickSegment( PNS::ROUTER* aRouter, const VECTOR2I& aWhere,
                                   PNS::NET_HANDLE aNet, int aLayer, VECTOR2I& aPointOut,
                                   const std::set<PNS::ITEM*> aAvoidItems = {} )
    {
        static const int  candidateCount = 2;
        PNS::LINKED_ITEM* prioritized[candidateCount];
        SEG::ecoord       dist[candidateCount];
        VECTOR2I          point[candidateCount];

        for( int i = 0; i < candidateCount; i++ )
        {
            prioritized[i] = nullptr;
            dist[i] = VECTOR2I::ECOORD_MAX;
        }

        auto haveCandidates = [&]()
        {
            for( PNS::ITEM* item : prioritized )
            {
                if( item )
                    return true;
            }

            return false;
        };

        for( bool useClearance : { false, true } )
        {
            PNS::ITEM_SET candidates = aRouter->QueryHoverItems( aWhere, useClearance );

            for( PNS::ITEM* item : candidates.Items() )
            {
                if( !item->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
                    continue;

                if( !item->IsRoutable() )
                    continue;

                if( !item->Layers().Overlaps( aLayer ) )
                    continue;

                if( aAvoidItems.find( item ) != aAvoidItems.end() )
                    continue;

                if( aNet && item->Net() != aNet )
                    continue;

                PNS::LINKED_ITEM* linked = static_cast<PNS::LINKED_ITEM*>( item );

                if( item->Kind() & PNS::ITEM::ARC_T )
                {
                    SEG::ecoord d0 = ( item->Anchor( 0 ) - aWhere ).SquaredEuclideanNorm();
                    SEG::ecoord d1 = ( item->Anchor( 1 ) - aWhere ).SquaredEuclideanNorm();

                    if( d0 <= dist[1] )
                    {
                        prioritized[1] = linked;
                        dist[1] = d0;
                        point[1] = item->Anchor( 0 );
                    }

                    if( d1 <= dist[1] )
                    {
                        prioritized[1] = linked;
                        dist[1] = d1;
                        point[1] = item->Anchor( 1 );
                    }
                }
                else if( item->Kind() & PNS::ITEM::SEGMENT_T )
                {
                    PNS::SEGMENT* segm = static_cast<PNS::SEGMENT*>( item );

                    VECTOR2I    nearest = segm->CLine().NearestPoint( aWhere, false );
                    SEG::ecoord dd = ( aWhere - nearest ).SquaredEuclideanNorm();

                    if( dd <= dist[1] )
                    {
                        prioritized[1] = segm;
                        dist[1] = dd;
                        point[1] = nearest;
                    }
                }
            }

            if( haveCandidates() )
                break;
        }

        PNS::LINKED_ITEM* rv = nullptr;

        for( int i = 0; i < candidateCount; i++ )
        {
            PNS::LINKED_ITEM* item = prioritized[i];

            if( item && ( aLayer < 0 || item->Layers().Overlaps( aLayer ) ) )
            {
                rv = item;
                aPointOut = point[i];
                break;
            }
        }

        return rv;
    }

    bool InitBaseLine( PNS::ROUTER* router, int layer, BOARD* aBoard )
    {
        PNS::NODE* world = router->GetWorld();

        int netCode = snapToNearestTrackPoint( m_origin, aBoard, -1 );
        snapToNearestTrackPoint( m_end, aBoard, netCode );

        VECTOR2I startSnapPoint, endSnapPoint;

        PNS::LINKED_ITEM* startItem = PickSegment( router, m_origin, nullptr, layer, startSnapPoint );
        PNS::LINKED_ITEM* endItem = PickSegment( router, m_end, nullptr, layer, endSnapPoint );

        wxASSERT( startItem );
        wxASSERT( endItem );

        if( !startItem || !endItem )
            return false;

        PNS::LINE        line = world->AssembleLine( startItem, nullptr, false, true );
        SHAPE_LINE_CHAIN chain = line.CLine();

        wxASSERT( line.ContainsLink( endItem ) );

        wxASSERT( chain.PointOnEdge( startSnapPoint, 1 ) );
        wxASSERT( chain.PointOnEdge( endSnapPoint, 1 ) );

        SHAPE_LINE_CHAIN pre;
        SHAPE_LINE_CHAIN mid;
        SHAPE_LINE_CHAIN post;

        chain.Split( startSnapPoint, endSnapPoint, pre, mid, post );

        m_baseLine = mid;

        return true;
    }

    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                 BOARD_COMMIT* aCommit ) override
    {
        aTool->Router()->SyncWorld();

        PNS::ROUTER*     router = aTool->Router();
        PNS::NODE*       world = router->GetWorld();
        PNS_KICAD_IFACE* iface = aTool->GetInterface();
        int              layer = GetLayer();

        // Ungroup first so that undo works
        if( !GetItems().empty() )
        {
            PCB_GENERATOR*    group = this;
            PICKED_ITEMS_LIST undoList;

            for( BOARD_ITEM* member : group->GetItems() )
                undoList.PushItem( ITEM_PICKER( nullptr, member, UNDO_REDO::UNGROUP ) );

            group->RemoveAll();

            aFrame->SaveCopyInUndoList( undoList, UNDO_REDO::UNGROUP );
        }
        else
        {
            aCommit->Push( "" );
        }

        aCommit->Remove( this );

        if( baselineValid() )
        {
            const SHAPE_LINE_CHAIN& baseLine = *m_baseLine;

            VECTOR2I startSnapPoint, endSnapPoint;

            std::optional<PNS::LINE> line =
                    getLineBetweenPoints( baseLine.CPoint( 0 ), baseLine.CPoint( -1 ), router,
                                          layer, startSnapPoint, endSnapPoint );

            wxCHECK( line, /* void */ );

            SHAPE_LINE_CHAIN pre;
            SHAPE_LINE_CHAIN mid;
            SHAPE_LINE_CHAIN post;
            line->CLine().Split( startSnapPoint, endSnapPoint, pre, mid, post );

            aTool->ClearRouterCommit();

            // LINE does not have a separate remover, as LINEs are never truly a member of the tree
            for( PNS::LINKED_ITEM* li : line->Links() )
                iface->RemoveItem( li );
            world->Remove( *line );

            SHAPE_LINE_CHAIN straightChain;
            straightChain.Append( pre );
            straightChain.Append( baseLine );
            straightChain.Append( post );
            straightChain.Simplify();

            PNS::LINE straightLine( *line, straightChain );

            // LINE does not have a separate remover, as LINEs are never truly a member of the tree
            world->Add( straightLine, false );
            for( PNS::LINKED_ITEM* li : straightLine.Links() )
                iface->AddItem( li );

            std::set<BOARD_ITEM*> clearRouterRemovedItems = aTool->GetRouterCommitRemovedItems();
            std::set<BOARD_ITEM*> clearRouterAddedItems = aTool->GetRouterCommitAddedItems();

            for( BOARD_ITEM* item : clearRouterRemovedItems )
            {
                item->ClearSelected();
                aCommit->Remove( item );
            }

            for( BOARD_ITEM* item : clearRouterAddedItems )
            {
                aCommit->Add( item );
            }
        }

        aCommit->Push( "Remove meander", APPEND_UNDO );
    }

    std::optional<PNS::LINE> getLineBetweenPoints( VECTOR2I origStart, VECTOR2I origEnd,
                                                   PNS::ROUTER* router, int layer,
                                                   VECTOR2I& aStartOut, VECTOR2I& aEndOut )
    {
        PNS::NODE* world = router->GetWorld();

        PNS::LINKED_ITEM* startItem = PickSegment( router, origStart, nullptr, layer, aStartOut );
        PNS::LINKED_ITEM* endItem = PickSegment( router, origEnd, nullptr, layer, aEndOut );

        wxASSERT( startItem );
        wxASSERT( endItem );

        if( !startItem || !endItem )
            return std::nullopt;

        PNS::LINE        line = world->AssembleLine( startItem, nullptr, false, true );
        SHAPE_LINE_CHAIN oldChain = line.CLine();

        wxCHECK( line.ContainsLink( endItem ), std::nullopt );

        wxASSERT( oldChain.PointOnEdge( aStartOut, 1 ) );
        wxASSERT( oldChain.PointOnEdge( aEndOut, 1 ) );

        return line;
    }

    PNS::MEANDER_SETTINGS ToMeanderSettings()
    {
        PNS::MEANDER_SETTINGS settings;

        settings.m_cornerStyle = m_rounded ? PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND
                                           : PNS::MEANDER_STYLE::MEANDER_STYLE_CHAMFER;

        settings.m_minAmplitude = m_minAmplitude;
        settings.m_maxAmplitude = m_maxAmplitude;
        settings.m_spacing = m_spacing;
        settings.m_targetLength = m_targetLength;
        settings.m_targetSkew = m_targetSkew;
        settings.m_singleSided = m_singleSide;
        settings.m_segmentSide = m_initialSide;
        settings.m_cornerRadiusPercentage = m_cornerRadiusPercentage;

        return settings;
    }

    void FromMeanderSettings( const PNS::MEANDER_SETTINGS& aSettings )
    {
        m_rounded = aSettings.m_cornerStyle == PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND;
        m_minAmplitude = aSettings.m_minAmplitude;
        m_maxAmplitude = aSettings.m_maxAmplitude;
        m_spacing = aSettings.m_spacing;
        m_targetLength = aSettings.m_targetLength;
        m_targetSkew = aSettings.m_targetSkew;
        m_singleSide = aSettings.m_singleSided;
        m_initialSide = aSettings.m_segmentSide;
        m_cornerRadiusPercentage = aSettings.m_cornerRadiusPercentage;
    }

    PNS::ROUTER_MODE ToPNSMode()
    {
        switch( m_tuningMode )
        {
        case LENGTH_TUNING_MODE::SINGLE: return PNS::PNS_MODE_TUNE_SINGLE;

        case LENGTH_TUNING_MODE::DIFF_PAIR: return PNS::PNS_MODE_TUNE_DIFF_PAIR;

        case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW;

        default: return PNS::PNS_MODE_TUNE_SINGLE;
        }
    }

    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                 BOARD_COMMIT* aCommit ) override
    {
        PNS::ROUTER*     router = aTool->Router();
        PNS::NODE*       world = router->GetWorld();
        PNS_KICAD_IFACE* iface = aTool->GetInterface();
        int              layer = GetLayer();

        iface->SetStartLayer( layer );

        if( router->RoutingInProgress() )
        {
            router->StopRouting();
        }

        if( !baselineValid() )
        {
            InitBaseLine( router, layer, aBoard );
        }
        else
        {
            const SHAPE_LINE_CHAIN& baseLine = *m_baseLine;

            VECTOR2I startSnapPoint, endSnapPoint;

            std::optional<PNS::LINE> line =
                    getLineBetweenPoints( baseLine.CPoint( 0 ), baseLine.CPoint( -1 ), router,
                                          layer, startSnapPoint, endSnapPoint );

            wxASSERT( line );

            if( !line )
            {
                InitBaseLine( router, layer, aBoard );
                return false;
            }

            SHAPE_LINE_CHAIN straightChain;
            {
                SHAPE_LINE_CHAIN pre, mid, post;
                line->CLine().Split( startSnapPoint, endSnapPoint, pre, mid, post );

                straightChain.Append( pre );
                straightChain.Append( baseLine );
                straightChain.Append( post );
                straightChain.Simplify();
            }

            // LINE does not have a separate remover, as LINEs are never truly a member of the tree
            for( PNS::LINKED_ITEM* li : line->Links() )
            {
                if( li->Parent() && aCommit )
                    aCommit->Remove( li->Parent() );
            }

            world->Remove( *line );

            PNS::LINE straightLine( *line, straightChain );

            world->Add( straightLine, false );

            m_origin = straightChain.NearestPoint( m_origin );
            m_end = straightChain.NearestPoint( m_end );

            // Don't allow points too close
            if( ( m_end - m_origin ).EuclideanNorm() < pcbIUScale.mmToIU( 0.1 ) )
            {
                m_origin = startSnapPoint;
                m_end = endSnapPoint;
            }

            {
                SHAPE_LINE_CHAIN pre, mid, post;
                straightChain.Split( m_origin, m_end, pre, mid, post );

                m_baseLine = mid;
            }
        }

        // Snap points
        VECTOR2I startSnapPoint, endSnapPoint;

        PNS::LINKED_ITEM* startItem =
                PickSegment( router, m_origin, nullptr, layer, startSnapPoint );
        PNS::LINKED_ITEM* endItem = PickSegment( router, m_end, nullptr, layer, endSnapPoint );

        wxASSERT( startItem );
        wxASSERT( endItem );

        if( !startItem || !endItem )
            return false;

        router->SetMode( ToPNSMode() );

        if( !router->StartRouting( startSnapPoint, startItem, layer ) )
            return false;

        auto placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

        PNS::MEANDER_SETTINGS settings = ToMeanderSettings();

        placer->UpdateSettings( settings );
        router->Move( m_end, nullptr );

        m_lastNetName = iface->GetNetName( startItem->Net() );
        m_tuningInfo = placer->TuningInfo( aFrame->GetUserUnits() );
        m_tuningStatus = placer->TuningStatus();

        return true;
    }

    void EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                   BOARD_COMMIT* aCommit, const wxString& aCommitMsg = wxEmptyString,
                   int aCommitFlags = 0 ) override
    {
        PNS::ROUTER* router = aTool->Router();

        if( router->RoutingInProgress() )
        {
            router->FixRoute( m_end, nullptr, true );
            router->StopRouting();

            std::set<BOARD_ITEM*> routerRemovedItems = aTool->GetRouterCommitRemovedItems();
            std::set<BOARD_ITEM*> routerAddedItems = aTool->GetRouterCommitAddedItems();

            for( BOARD_ITEM* item : routerRemovedItems )
            {
                item->ClearSelected();
                aCommit->Remove( item );
            }

            for( BOARD_ITEM* item : routerAddedItems )
            {
                AddItem( item );
                aCommit->Add( item );
            }
        }

        aCommit->Push( aCommitMsg, aCommitFlags );
    }

    bool MakeEditPoints( std::shared_ptr<EDIT_POINTS> points ) const override
    {
        points->AddPoint( m_origin );
        points->AddPoint( m_end );

        SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                                : SEG( m_origin, m_end );

        int offset = m_maxAmplitude;

        if( m_initialSide == -1 )
            offset *= -1;

        VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( offset );

        points->AddPoint( m_origin + widthHandleOffset );
        points->Point( 2 ).SetGridConstraint( IGNORE_GRID );

        VECTOR2I spacingHandleOffset =
                widthHandleOffset + ( base.B - base.A ).Resize( m_spacing * 1.5 );

        points->AddPoint( m_origin + spacingHandleOffset );
        points->Point( 3 ).SetGridConstraint( IGNORE_GRID );

        return true;
    }

    bool UpdateFromEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints,
                               BOARD_COMMIT*                aCommit ) override
    {
        SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                                : SEG( m_origin, m_end );

        m_origin = aEditPoints->Point( 0 ).GetPosition();
        m_end = aEditPoints->Point( 1 ).GetPosition();

        if( aEditPoints->Point( 2 ).IsActive() )
        {
            VECTOR2I wHandle = aEditPoints->Point( 2 ).GetPosition();

            int value = base.LineDistance( wHandle );
            SetMaxAmplitude( KiROUND( value / pcbIUScale.mmToIU( 0.1 ) ) * pcbIUScale.mmToIU( 0.1 ) );

            int side = base.Side( wHandle );

            if( side < 0 )
                m_initialSide = PNS::MEANDER_SIDE_LEFT;
            else
                m_initialSide = PNS::MEANDER_SIDE_RIGHT;
        }

        if( aEditPoints->Point( 3 ).IsActive() )
        {
            VECTOR2I wHandle = aEditPoints->Point( 2 ).GetPosition();
            VECTOR2I sHandle = aEditPoints->Point( 3 ).GetPosition();

            int value = SEG( m_origin, wHandle ).LineDistance( sHandle ) / 1.5;

            SetSpacing( KiROUND( value / pcbIUScale.mmToIU( 0.01 ) ) * pcbIUScale.mmToIU( 0.01 ) );
        }

        return true;
    }

    bool UpdateEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints ) override
    {
        SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                                : SEG( m_origin, m_end );

        int offset = m_maxAmplitude;

        if( m_initialSide == -1 )
            offset *= -1;

        VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( offset );

        aEditPoints->Point( 0 ).SetPosition( m_origin );
        aEditPoints->Point( 1 ).SetPosition( m_end );

        aEditPoints->Point( 2 ).SetPosition( m_origin + widthHandleOffset );

        VECTOR2I spacingHandleOffset =
                widthHandleOffset + ( base.B - base.A ).Resize( m_spacing * 1.5 );

        aEditPoints->Point( 3 ).SetPosition( m_origin + spacingHandleOffset );

        return true;
    }

    SHAPE_LINE_CHAIN GetRectShape() const
    {
        SHAPE_LINE_CHAIN chain;

        if( m_baseLine )
        {
            bool singleSided = m_singleSide;

            if( singleSided )
            {
                SHAPE_LINE_CHAIN left, right;

                if( m_baseLine->OffsetLine( m_maxAmplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                            ARC_LOW_DEF, left, right, true ) )
                {
                    chain.Append( m_baseLine->CPoint( 0 ) );
                    chain.Append( m_initialSide >= 0 ? right : left );
                    chain.Append( m_baseLine->CPoint( -1 ) );

                    return chain;
                }
                else
                {
                    singleSided = false;
                }
            }

            if( !singleSided )
            {
                SHAPE_POLY_SET poly;

                poly.OffsetLineChain( *m_baseLine, m_maxAmplitude * 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                  ARC_LOW_DEF, false );

                if( poly.OutlineCount() > 0 )
                {
                    chain = poly.Outline( 0 );
                }
            }
        }

        return chain;
    }

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_origin += aMoveVector;
        m_end += aMoveVector;

        PCB_GROUP::Move( aMoveVector );
    }

    const BOX2I GetBoundingBox() const override { return GetRectShape().BBox(); }

    void ViewGetLayers( int aLayers[], int& aCount ) const override
    {
        aCount = 0;
        aLayers[aCount++] = LAYER_ANCHOR;
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        return GetRectShape().Collide( aPosition, aAccuracy );
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const override
    {
        return GetBoundingBox().Intersects( aRect );
    }

    const BOX2I ViewBBox() const override { return GetBoundingBox(); }

    EDA_ITEM* Clone() const override { return new PCB_GENERATOR_MEANDERS( *this ); }

    void swapData( BOARD_ITEM* aImage ) override
    {
        wxASSERT( aImage->Type() == PCB_GENERATOR_T );

        std::swap( *this, *static_cast<PCB_GENERATOR_MEANDERS*>( aImage ) );
    }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final
    {
        if( !IsSelected() )
            return;

        KIGFX::PREVIEW::DRAW_CONTEXT ctx( *aView );

        if( m_baseLine )
        {
            for( int i = 0; i < m_baseLine->SegmentCount(); i++ )
            {
                SEG seg = m_baseLine->CSegment( i );
                ctx.DrawLine( seg.A, seg.B, false );
            }
        }
        else
        {
            ctx.DrawLine( m_origin, m_end, false );
        }

        SHAPE_LINE_CHAIN chain = GetRectShape();

        for( int i = 0; i < chain.SegmentCount(); i++ )
        {
            SEG seg = chain.Segment( i );

            ctx.DrawLineDashed( seg.A, seg.B, pcbIUScale.mmToIU( 0.2 ), pcbIUScale.mmToIU( 0.1 ),
                                false );
        }
    }


    const VECTOR2I& GetEnd() const { return m_end; }
    void            SetEnd( const VECTOR2I& aValue ) { m_end = aValue; }

    int  GetEndX() const { return m_end.x; }
    void SetEndX( int aValue ) { m_end.x = aValue; }

    int  GetEndY() const { return m_end.y; }
    void SetEndY( int aValue ) { m_end.y = aValue; }

    LENGTH_TUNING_MODE GetTuningMode() const { return m_tuningMode; }
    void               SetTuningMode( LENGTH_TUNING_MODE aValue ) { m_tuningMode = aValue; }

    int  GetMinAmplitude() const { return m_minAmplitude; }
    void SetMinAmplitude( int aValue ) { m_minAmplitude = aValue; }

    int  GetMaxAmplitude() const { return m_maxAmplitude; }
    void SetMaxAmplitude( int aValue ) { m_maxAmplitude = aValue; }

    PNS::MEANDER_SIDE GetInitialSide() const { return m_initialSide; }
    void              SetInitialSide( PNS::MEANDER_SIDE aValue ) { m_initialSide = aValue; }

    int  GetSpacing() const { return m_spacing; }
    void SetSpacing( int aValue ) { m_spacing = aValue; }

    int  GetTargetLength() const { return m_targetLength; }
    void SetTargetLength( int aValue ) { m_targetLength = aValue; }

    int  GetTargetSkew() const { return m_targetSkew; }
    void SetTargetSkew( int aValue ) { m_targetSkew = aValue; }

    int  GetCornerRadiusPercentage() const { return m_cornerRadiusPercentage; }
    void SetCornerRadiusPercentage( int aValue ) { m_cornerRadiusPercentage = aValue; }

    bool IsSingleSided() const { return m_singleSide; }
    void SetSingleSided( bool aValue ) { m_singleSide = aValue; }

    bool IsRounded() const { return m_rounded; }
    void SetRounded( bool aValue ) { m_rounded = aValue; }

    std::vector<std::pair<wxString, wxVariant>> GetRowData() override
    {
        std::vector<std::pair<wxString, wxVariant>> data = PCB_GENERATOR::GetRowData();
        data.emplace_back( _HKI( "Net" ), m_lastNetName );
        data.emplace_back( _HKI( "Tuning" ), m_tuningInfo );
        return data;
    }

    const STRING_ANY_MAP GetProperties() const override
    {
        STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

        props.set( "tuning_mode", TuningToString( m_tuningMode ) );
        props.set( "initial_side", SideToString( m_initialSide ) );
        props.set( "last_status", StatusToString( m_tuningStatus ) );

        props.set( "end", m_end );
        props.set( "corner_radius_percent", m_cornerRadiusPercentage );
        props.set( "single_sided", m_singleSide );
        props.set( "rounded", m_rounded );

        props.set_iu( "max_amplitude", m_maxAmplitude );
        props.set_iu( "min_spacing", m_spacing );
        props.set_iu( "target_length", m_targetLength );
        props.set_iu( "target_skew", m_targetSkew );

        props.set( "last_netname", m_lastNetName );
        props.set( "last_tuning", m_tuningInfo );

        if( m_baseLine )
            props.set( "base_line", wxAny( *m_baseLine ) );

        return props;
    }

    void SetProperties( const STRING_ANY_MAP& aProps ) override
    {
        PCB_GENERATOR::SetProperties( aProps );

        wxString tuningMode;
        aProps.get_to( "tuning_mode", tuningMode );
        m_tuningMode = TuningFromString( tuningMode.utf8_string() );

        wxString side;
        aProps.get_to( "initial_side", side );
        m_initialSide = SideFromString( side.utf8_string() );

        wxString status;
        aProps.get_to( "last_status", status );
        m_tuningStatus = StatusFromString( status.utf8_string() );

        aProps.get_to( "end", m_end );
        aProps.get_to( "corner_radius_percent", m_cornerRadiusPercentage );
        aProps.get_to( "single_sided", m_singleSide );
        aProps.get_to( "side", m_initialSide );
        aProps.get_to( "rounded", m_rounded );

        aProps.get_to_iu( "max_amplitude", m_maxAmplitude );
        aProps.get_to_iu( "min_spacing", m_spacing );
        aProps.get_to_iu( "target_length", m_targetLength );
        aProps.get_to_iu( "target_skew", m_targetSkew );

        aProps.get_to( "last_netname", m_lastNetName );
        aProps.get_to( "last_tuning", m_tuningInfo );

        if( auto baseLine = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line" ) )
            m_baseLine = *baseLine;
    }

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override
    {
        PNS::MEANDER_SETTINGS settings = ToMeanderSettings();

        DIALOG_MEANDER_PROPERTIES dlg( aEditFrame, settings, ToPNSMode() );

        if( dlg.ShowModal() == wxID_OK )
        {
            BOARD_COMMIT commit( aEditFrame );
            commit.Modify( this );

            FromMeanderSettings( settings );

            commit.Push( _( "Edit meander properties" ) );
        }

        aEditFrame->GetToolManager()->PostAction<PCB_GENERATOR*>( PCB_ACTIONS::regenerateItem,
                                                                  this );
    }

protected:
    VECTOR2I m_end;

    int m_minAmplitude;
    int m_maxAmplitude;
    int m_spacing;
    int m_targetLength;
    int m_targetSkew;
    int m_cornerRadiusPercentage;

    PNS::MEANDER_SIDE m_initialSide;

    std::optional<SHAPE_LINE_CHAIN> m_baseLine;

    bool m_singleSide = false;
    bool m_rounded = true;

    LENGTH_TUNING_MODE m_tuningMode = LENGTH_TUNING_MODE::SINGLE;

    wxString m_lastNetName;
    wxString m_tuningInfo;

    PNS::MEANDER_PLACER_BASE::TUNING_STATUS m_tuningStatus =
            PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED;
};

const wxString PCB_GENERATOR_MEANDERS::DISPLAY_NAME = _HKI( "Meanders" );
const wxString PCB_GENERATOR_MEANDERS::GENERATOR_TYPE = wxS( "meanders" );


#define HITTEST_THRESHOLD_PIXELS 5


int DRAWING_TOOL::PlaceMeander( const TOOL_EVENT& aEvent )
{
    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    m_pickerItem = nullptr;
    m_meander = nullptr;

    picker->SetCursor( KICURSOR::BULLSEYE );

    picker->SetClickHandler(
            [this]( const VECTOR2D& aPosition ) -> bool
            {
                if( m_pickerItem )
                {
                    if( !m_meander )
                    {
                        // First click; create a meander generator

                        m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );
                        m_frame->SetActiveLayer( m_pickerItem->GetLayer() );

                        m_meander = new PCB_GENERATOR_MEANDERS( m_board, m_pickerItem->GetLayer() );

                        int      dummyDist;
                        int      dummyClearance = std::numeric_limits<int>::max() / 2;
                        VECTOR2I closestPt;

                        m_pickerItem->GetEffectiveShape()->Collide( aPosition, dummyClearance,
                                                                    &dummyDist, &closestPt );
                        m_meander->SetPosition( closestPt );
                    }
                    else
                    {
                        // Second click; we're done
                        BOARD_COMMIT    commit( m_frame );
                        GENERATOR_TOOL* generatorTool = m_toolMgr->GetTool<GENERATOR_TOOL>();

                        m_meander->EditStart( generatorTool, m_board, m_frame, &commit );
                        m_meander->Update( generatorTool, m_board, m_frame, &commit );
                        m_meander->EditPush( generatorTool, m_board, m_frame, &commit,
                                             _( "Place Meander" ) );

                        return false;   // exit picker tool
                    }
                }

                return true;
            } );

    picker->SetMotionHandler(
            [this]( const VECTOR2D& aPos )
            {
                BOARD*                   board = m_frame->GetBoard();
                PCB_SELECTION_TOOL*      selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
                GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
                GENERAL_COLLECTOR        collector;

                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
                collector.Collect( board, { PCB_TRACE_T, PCB_ARC_T }, aPos, guide );

                if( collector.GetCount() > 1 )
                    selTool->GuessSelectionCandidates( collector, aPos );

                BOARD_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

                if( !m_meander )
                {
                    // First click not yet made; we're in brighten-track-under-cursor mode

                    if( m_pickerItem != item )
                    {
                        if( m_pickerItem )
                            selTool->UnbrightenItem( m_pickerItem );

                        m_pickerItem = item;

                        if( m_pickerItem )
                            selTool->BrightenItem( m_pickerItem );
                    }
                }
                else
                {
                    // First click alread made; we're in preview-meander mode

                    m_meander->SetEnd( aPos );

                    if( m_meander->GetPosition() != m_meander->GetEnd() )
                    {
                        GENERATOR_TOOL* generatorTool = m_toolMgr->GetTool<GENERATOR_TOOL>();

                        m_meander->EditStart( generatorTool, m_board, m_frame, nullptr );
                        m_meander->Update( generatorTool, m_board, m_frame, nullptr );
                    }
                }
            } );

    picker->SetCancelHandler(
            [this]()
            {
                if( m_pickerItem )
                    m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                delete m_meander;
                m_meander = nullptr;
            } );

    picker->SetFinalizeHandler(
            [this]( const int& aFinalState )
            {
                // Ensure the cursor gets changed & updated
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
                m_frame->GetCanvas()->Refresh();
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


static struct PCB_GENERATOR_MEANDERS_DESC
{
    PCB_GENERATOR_MEANDERS_DESC()
    {
        ENUM_MAP<LENGTH_TUNING_MODE>::Instance()
                .Map( LENGTH_TUNING_MODE::SINGLE, _HKI( "Single track" ) )
                /*.Map( LENGTH_TUNING_MODE::DIFF_PAIR, _HKI( "Diff. pair" ) )*/ // Not supported
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR_SKEW, _HKI( "Diff. pair Skew" ) );

        ENUM_MAP<PNS::MEANDER_SIDE>::Instance()
                .Map( PNS::MEANDER_SIDE_LEFT, _HKI( "Left" ) )
                .Map( PNS::MEANDER_SIDE_RIGHT, _HKI( "Right" ) )
                .Map( PNS::MEANDER_SIDE_DEFAULT, _HKI( "Default" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_GENERATOR_MEANDERS );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATOR_MEANDERS, PCB_GENERATOR> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATOR_MEANDERS, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GENERATOR_MEANDERS ), TYPE_HASH( PCB_GENERATOR ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GENERATOR_MEANDERS ), TYPE_HASH( BOARD_ITEM ) );

        const wxString groupTab = _HKI( "Meander Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "End X" ), &PCB_GENERATOR_MEANDERS::SetEndX,
                                     &PCB_GENERATOR_MEANDERS::GetEndX, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "End Y" ), &PCB_GENERATOR_MEANDERS::SetEndY,
                                     &PCB_GENERATOR_MEANDERS::GetEndY, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_GENERATOR_MEANDERS, LENGTH_TUNING_MODE>(
                                     _HKI( "Tuning mode" ),
                                     &PCB_GENERATOR_MEANDERS::SetTuningMode,
                                     &PCB_GENERATOR_MEANDERS::GetTuningMode ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Min amplitude" ),
                                     &PCB_GENERATOR_MEANDERS::SetMinAmplitude,
                                     &PCB_GENERATOR_MEANDERS::GetMinAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Max amplitude" ),
                                     &PCB_GENERATOR_MEANDERS::SetMaxAmplitude,
                                     &PCB_GENERATOR_MEANDERS::GetMaxAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_GENERATOR_MEANDERS, PNS::MEANDER_SIDE>(
                                     _HKI( "Initial side" ),
                                     &PCB_GENERATOR_MEANDERS::SetInitialSide,
                                     &PCB_GENERATOR_MEANDERS::GetInitialSide ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Min spacing" ), &PCB_GENERATOR_MEANDERS::SetSpacing,
                                     &PCB_GENERATOR_MEANDERS::GetSpacing, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Corner radius %" ),
                                     &PCB_GENERATOR_MEANDERS::SetCornerRadiusPercentage,
                                     &PCB_GENERATOR_MEANDERS::GetCornerRadiusPercentage,
                                     PROPERTY_DISPLAY::PT_DEFAULT, ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Target length" ),
                                     &PCB_GENERATOR_MEANDERS::SetTargetLength,
                                     &PCB_GENERATOR_MEANDERS::GetTargetLength,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, int>(
                                     _HKI( "Target skew" ), &PCB_GENERATOR_MEANDERS::SetTargetSkew,
                                     &PCB_GENERATOR_MEANDERS::GetTargetSkew,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, bool>(
                                     _HKI( "Single-sided" ),
                                     &PCB_GENERATOR_MEANDERS::SetSingleSided,
                                     &PCB_GENERATOR_MEANDERS::IsSingleSided ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, bool>(
                                     _HKI( "Rounded" ), &PCB_GENERATOR_MEANDERS::SetRounded,
                                     &PCB_GENERATOR_MEANDERS::IsRounded ),
                             groupTab );
    }
} _PCB_GENERATOR_MEANDERS_DESC;

ENUM_TO_WXANY( LENGTH_TUNING_MODE )
ENUM_TO_WXANY( PNS::MEANDER_SIDE )

static GENERATORS_MGR::REGISTER<PCB_GENERATOR_MEANDERS> registerMe;