/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include <pcb_generator.h>
#include <generators_mgr.h>

#include <functional>
#include <optional>
#include <magic_enum.hpp>

#include <wx/debug.h>
#include <wx/log.h>

#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_circle.h>
#include <geometry/geometry_utils.h>
#include <kiplatform/ui.h>
#include <dialogs/dialog_unit_entry.h>
#include <collectors.h>
#include <scoped_set_reset.h>
#include <core/mirror.h>
#include <string_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_group.h>

#include <tool/edit_points.h>
#include <tool/tool_manager.h>
#include <tools/drawing_tool.h>
#include <tools/generator_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>
#include <view/view.h>
#include <view/view_controls.h>

#include <router/pns_dp_meander_placer.h>
#include <router/pns_meander_placer_base.h>
#include <router/pns_meander.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_segment.h>
#include <router/pns_arc.h>
#include <router/pns_solid.h>
#include <router/pns_topology.h>
#include <router/router_preview_item.h>

#include <dialogs/dialog_tuning_pattern_properties.h>

#include <generators/pcb_tuning_pattern.h>

TUNING_STATUS_VIEW_ITEM::TUNING_STATUS_VIEW_ITEM( PCB_BASE_EDIT_FRAME* aFrame ) :
        EDA_ITEM( NOT_USED ), // Never added to anything - just a preview
        m_frame( aFrame ), m_min( 0.0 ), m_max( 0.0 ), m_current( 0.0 ), m_isTimeDomain( false )
{ }

wxString TUNING_STATUS_VIEW_ITEM::GetClass() const  { return wxT( "TUNING_STATUS" ); }

#if defined(DEBUG)
void TUNING_STATUS_VIEW_ITEM::Show( int nestLevel, std::ostream& os ) const  {}
#endif

VECTOR2I TUNING_STATUS_VIEW_ITEM::GetPosition() const  { return m_pos; }
void     TUNING_STATUS_VIEW_ITEM::SetPosition( const VECTOR2I& aPos )  { m_pos = aPos; };

void TUNING_STATUS_VIEW_ITEM::SetMinMax( const double aMin, const double aMax )
{
    const EDA_DATA_TYPE unitType = m_isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

    m_min = aMin;
    m_minText = m_frame->MessageTextFromValue( m_min, false, unitType );
    m_max = aMax;
    m_maxText = m_frame->MessageTextFromValue( m_max, false, unitType );
}

void TUNING_STATUS_VIEW_ITEM::ClearMinMax()
{
    m_min = 0.0;
    m_minText = wxT( "---" );
    m_max = std::numeric_limits<double>::max();
    m_maxText = wxT( "---" );
}

void TUNING_STATUS_VIEW_ITEM::SetCurrent( const double aCurrent, const wxString& aLabel )
{
    const EDA_DATA_TYPE unitType = m_isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

    m_current = aCurrent;
    m_currentText = m_frame->MessageTextFromValue( aCurrent, true, unitType );
    m_currentLabel = aLabel;
}

void TUNING_STATUS_VIEW_ITEM::SetIsTimeDomain( const bool aIsTimeDomain ) { m_isTimeDomain = aIsTimeDomain; }

const BOX2I TUNING_STATUS_VIEW_ITEM::ViewBBox() const
{
    BOX2I tmp;

    // this is an edit-time artefact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}

std::vector<int> TUNING_STATUS_VIEW_ITEM::ViewGetLayers() const
{
    return { LAYER_UI_START, LAYER_UI_START + 1 };
}

void TUNING_STATUS_VIEW_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL* gal = aView->GetGAL();
    bool        viewFlipped = gal->IsFlippedX();
    bool        drawingDropShadows = ( aLayer == LAYER_UI_START );

    gal->Save();
    gal->Scale( { 1., 1. } );

    KIGFX::PREVIEW::TEXT_DIMS headerDims = KIGFX::PREVIEW::GetConstantGlyphHeight( gal, -2 );
    KIGFX::PREVIEW::TEXT_DIMS textDims = KIGFX::PREVIEW::GetConstantGlyphHeight( gal, -1 );
    KIFONT::FONT*             font = KIFONT::FONT::GetFont();
    const KIFONT::METRICS&    fontMetrics = KIFONT::METRICS::Default();
    TEXT_ATTRIBUTES           textAttrs;

    int      glyphWidth = textDims.GlyphSize.x;
    VECTOR2I margin( KiROUND( glyphWidth * 0.4 ), KiROUND( glyphWidth ) );
    VECTOR2I size( glyphWidth * 25 + margin.x * 2, headerDims.GlyphSize.y + textDims.GlyphSize.y );
    VECTOR2I offset( margin.x * 2, -( size.y + margin.y * 2 ) );

    if( drawingDropShadows )
    {
        gal->SetIsFill( true );
        gal->SetIsStroke( true );
        gal->SetLineWidth( gal->GetScreenWorldMatrix().GetScale().x * 2 );
        gal->SetStrokeColor( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
        KIGFX::COLOR4D bgColor( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
        gal->SetFillColor( bgColor.WithAlpha( 0.9 ) );

        gal->DrawRectangle( GetPosition() + offset - margin,
                            GetPosition() + offset + size + margin );
        gal->Restore();
        return;
    }

    COLOR4D bg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
    COLOR4D normal = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
    COLOR4D red;

    // Choose a red with reasonable contrasting with the background
    double  bg_h, bg_s, bg_l;
    bg.ToHSL( bg_h, bg_s, bg_l );
    red.FromHSL( 0, 1.0, bg_l < 0.5 ? 0.7 : 0.3 );

    if( viewFlipped )
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    else
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;

    gal->SetIsFill( false );
    gal->SetIsStroke( true );
    gal->SetStrokeColor( normal );
    textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;

    // Prevent text flipping when view is flipped
    if( gal->IsFlippedX() )
    {
        textAttrs.m_Mirrored = true;
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    }

    textAttrs.m_Size = headerDims.GlyphSize;
    textAttrs.m_StrokeWidth = headerDims.StrokeWidth;

    VECTOR2I textPos = GetPosition() + offset;
    font->Draw( gal, m_currentLabel, textPos, textAttrs, KIFONT::METRICS::Default() );

    textPos.x += glyphWidth * 11 + margin.x;
    font->Draw( gal, _( "min" ), textPos, textAttrs, fontMetrics );

    textPos.x += glyphWidth * 7 + margin.x;
    font->Draw( gal, _( "max" ), textPos, textAttrs, fontMetrics );

    textAttrs.m_Size = textDims.GlyphSize;
    textAttrs.m_StrokeWidth = textDims.StrokeWidth;

    textPos = GetPosition() + offset;
    textPos.y += KiROUND( headerDims.LinePitch * 1.3 );
    font->Draw( gal, m_currentText, textPos, textAttrs, KIFONT::METRICS::Default() );

    textPos.x += glyphWidth * 11 + margin.x;
    gal->SetStrokeColor( m_current < m_min ? red : normal );
    font->Draw( gal, m_minText, textPos, textAttrs, fontMetrics );

    textPos.x += glyphWidth * 7 + margin.x;
    gal->SetStrokeColor( m_current > m_max ? red : normal );
    font->Draw( gal, m_maxText, textPos, textAttrs, fontMetrics );

    gal->Restore();
}


static LENGTH_TUNING_MODE tuningFromString( const std::string& aStr )
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


static std::string tuningToString( const LENGTH_TUNING_MODE aTuning )
{
    switch( aTuning )
    {
    case LENGTH_TUNING_MODE::SINGLE:         return "single";
    case LENGTH_TUNING_MODE::DIFF_PAIR:      return "diff_pair";
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return "diff_pair_skew";
    default:            wxFAIL;              return "";
    }
}


static LENGTH_TUNING_MODE fromPNSMode( PNS::ROUTER_MODE aRouterMode )
{
    switch( aRouterMode )
    {
    case PNS::PNS_MODE_TUNE_SINGLE:         return LENGTH_TUNING_MODE::SINGLE;
    case PNS::PNS_MODE_TUNE_DIFF_PAIR:      return LENGTH_TUNING_MODE::DIFF_PAIR;
    case PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW: return LENGTH_TUNING_MODE::DIFF_PAIR_SKEW;
    default:                                return LENGTH_TUNING_MODE::SINGLE;
    }
}


static PNS::MEANDER_SIDE sideFromString( const std::string& aStr )
{
    if( aStr == "default" )
        return PNS::MEANDER_SIDE_DEFAULT;
    else if( aStr == "left" )
        return PNS::MEANDER_SIDE_LEFT;
    else if( aStr == "right" )
        return PNS::MEANDER_SIDE_RIGHT;
    else
    {
        wxFAIL_MSG( wxS( "Unknown length-tuning side token" ) );
        return PNS::MEANDER_SIDE_DEFAULT;
    }
}


static std::string statusToString( const PNS::MEANDER_PLACER_BASE::TUNING_STATUS aStatus )
{
    switch( aStatus )
    {
    case PNS::MEANDER_PLACER_BASE::TOO_LONG:  return "too_long";
    case PNS::MEANDER_PLACER_BASE::TOO_SHORT: return "too_short";
    case PNS::MEANDER_PLACER_BASE::TUNED:     return "tuned";
    default:           wxFAIL;                return "";
    }
}


static PNS::MEANDER_PLACER_BASE::TUNING_STATUS statusFromString( const std::string& aStr )
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


static std::string sideToString( const PNS::MEANDER_SIDE aValue )
{
    switch( aValue )
    {
    case PNS::MEANDER_SIDE_DEFAULT: return "default";
    case PNS::MEANDER_SIDE_LEFT:    return "left";
    case PNS::MEANDER_SIDE_RIGHT:   return "right";
    default:        wxFAIL;         return "";
    }
}


PCB_TUNING_PATTERN::PCB_TUNING_PATTERN( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer,
                                        LENGTH_TUNING_MODE aMode ) :
        PCB_GENERATOR( aParent, aLayer ),
        m_trackWidth( 0 ),
        m_diffPairGap( 0 ),
        m_tuningMode( aMode ),
        m_tuningStatus( PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED ),
        m_updateSideFromEnd(false)
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;
    m_end = VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 );
    m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
}


static VECTOR2I snapToNearestTrack( const VECTOR2I& aP, BOARD* aBoard, NETINFO_ITEM* aNet,
                                    PCB_TRACK** aNearestTrack )
{
    SEG::ecoord   minDist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I      closestPt = aP;

    for( PCB_TRACK *track : aBoard->Tracks() )
    {
        if( aNet && track->GetNet() != aNet )
            continue;

        VECTOR2I nearest;

        if( track->Type() == PCB_ARC_T )
        {
            PCB_ARC*  pcbArc = static_cast<PCB_ARC*>( track );
            SHAPE_ARC arc( pcbArc->GetStart(), pcbArc->GetMid(), pcbArc->GetEnd(),
                           pcbArc->GetWidth() );

            nearest = arc.NearestPoint( aP );
        }
        else
        {
            SEG seg( track->GetStart(), track->GetEnd() );
            nearest = seg.NearestPoint( aP );
        }

        SEG::ecoord dist_sq = ( nearest - aP ).SquaredEuclideanNorm();

        if( dist_sq < minDist_sq )
        {
            minDist_sq = dist_sq;
            closestPt = nearest;

            if( aNearestTrack )
                *aNearestTrack = track;
        }
    }

    return closestPt;
}


bool PCB_TUNING_PATTERN::baselineValid()
{
    if( m_tuningMode == DIFF_PAIR || m_tuningMode == DIFF_PAIR_SKEW )
    {
        return( m_baseLine && m_baseLine->PointCount() > 1
                    && m_baseLineCoupled && m_baseLineCoupled->PointCount() > 1 );
    }
    else
    {
        return( m_baseLine && m_baseLine->PointCount() > 1 );
    }
}


PCB_TUNING_PATTERN* PCB_TUNING_PATTERN::CreateNew( GENERATOR_TOOL* aTool,
                                                   PCB_BASE_EDIT_FRAME* aFrame,
                                                   BOARD_CONNECTED_ITEM* aStartItem,
                                                   LENGTH_TUNING_MODE aMode )
{
    BOARD*                 board = aStartItem->GetBoard();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    DRC_CONSTRAINT         constraint;
    PCB_LAYER_ID           layer = aStartItem->GetLayer();

    PCB_TUNING_PATTERN* pattern = new PCB_TUNING_PATTERN( board, layer, aMode );

    switch( aMode )
    {
    case SINGLE:         pattern->m_settings = bds.m_SingleTrackMeanderSettings; break;
    case DIFF_PAIR:      pattern->m_settings = bds.m_DiffPairMeanderSettings;    break;
    case DIFF_PAIR_SKEW: pattern->m_settings = bds.m_SkewMeanderSettings;        break;
    }

    if( aMode == SINGLE || aMode == DIFF_PAIR )
    {
        constraint = bds.m_DRCEngine->EvalRules( LENGTH_CONSTRAINT, aStartItem, nullptr, layer );

        if( !constraint.IsNull() )
        {
            if( constraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN ) )
            {
                pattern->m_settings.SetTargetLengthDelay( constraint.GetValue() );
                pattern->m_settings.SetTargetLength( MINOPTMAX<int>() );
                pattern->m_settings.m_isTimeDomain = true;
            }
            else
            {
                pattern->m_settings.SetTargetLengthDelay( MINOPTMAX<int>() );
                pattern->m_settings.SetTargetLength( constraint.GetValue() );
                pattern->m_settings.m_isTimeDomain = false;
            }
        }
    }
    else
    {
        constraint = bds.m_DRCEngine->EvalRules( SKEW_CONSTRAINT, aStartItem, nullptr, layer );

        if( !constraint.IsNull() )
        {
            if( constraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN ) )
            {
                pattern->m_settings.SetTargetSkew( MINOPTMAX<int>() );
                pattern->m_settings.SetTargetLengthDelay( constraint.GetValue() );
                pattern->m_settings.m_isTimeDomain = true;
            }
            else
            {
                pattern->m_settings.SetTargetSkew( constraint.GetValue() );
                pattern->m_settings.SetTargetSkewDelay( MINOPTMAX<int>() );
                pattern->m_settings.m_isTimeDomain = true;
            }
        }
    }

    pattern->SetFlags( IS_NEW );
    pattern->m_settings.m_netClass = aStartItem->GetEffectiveNetClass();

    return pattern;
}

void PCB_TUNING_PATTERN::EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( aCommit )
    {
        if( IsNew() )
            aCommit->Add( this );
        else
            aCommit->Modify( this );
    }

    SetFlags( IN_EDIT );

    PNS::ROUTER* router = aTool->Router();
    int          layer = router->GetInterface()->GetPNSLayerFromBoardLayer( GetLayer() );

    aTool->ClearRouterChanges();
    router->SyncWorld();

    PNS::RULE_RESOLVER* resolver = router->GetRuleResolver();
    PNS::CONSTRAINT     constraint;

    if( !baselineValid() )
        initBaseLines( router, layer, aBoard );

    if( m_updateSideFromEnd )
    {
        VECTOR2I centerlineOffsetEnd;

        if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled
            && m_baseLineCoupled->SegmentCount() > 0 )
        {
            centerlineOffsetEnd =
                    ( m_baseLineCoupled->CLastPoint() - m_baseLine->CLastPoint() ) / 2;
        }

        SEG baseEnd = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( -1 )
                                                                   : SEG( m_origin, m_end );

        baseEnd.A += centerlineOffsetEnd;
        baseEnd.B += centerlineOffsetEnd;

        if( baseEnd.A != baseEnd.B )
        {
            int side = baseEnd.Side( m_end );

            if( side < 0 )
                m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
            else
                m_settings.m_initialSide = PNS::MEANDER_SIDE_RIGHT;
        }

        m_updateSideFromEnd = false;
    }

    PCB_TRACK* track = nullptr;
    m_origin = snapToNearestTrack( m_origin, aBoard, nullptr, &track );
    wxCHECK( track, /* void */ );

    m_settings.m_netClass = track->GetEffectiveNetClass();

    if( !m_settings.m_overrideCustomRules )
    {
        PNS::SEGMENT  pnsItem;
        NETINFO_ITEM* net = track->GetNet();

        pnsItem.SetParent( track );
        pnsItem.SetNet( net );

        if( m_tuningMode == SINGLE )
        {
            if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                           &pnsItem, nullptr, layer, &constraint ) )
            {
                if( constraint.m_IsTimeDomain )
                {
                    m_settings.SetTargetLengthDelay( constraint.m_Value );
                    m_settings.SetTargetLength( MINOPTMAX<int>() );
                }
                else
                {
                    m_settings.SetTargetLengthDelay( MINOPTMAX<int>() );
                    m_settings.SetTargetLength( constraint.m_Value );
                }

                m_settings.m_isTimeDomain = constraint.m_IsTimeDomain;
                aTool->GetManager()->PostEvent( EVENTS::SelectedItemsModified );
            }
        }
        else
        {
            PCB_TRACK*    coupledTrack = nullptr;
            PNS::SEGMENT  pnsCoupledItem;
            NETINFO_ITEM* coupledNet = aBoard->DpCoupledNet( net );

            if( coupledNet )
                snapToNearestTrack( m_origin, aBoard, coupledNet, &coupledTrack );

            pnsCoupledItem.SetParent( coupledTrack );
            pnsCoupledItem.SetNet( coupledNet );

            if( m_tuningMode == DIFF_PAIR )
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                               &pnsItem, &pnsCoupledItem, layer, &constraint ) )
                {
                    if( constraint.m_IsTimeDomain )
                    {
                        m_settings.SetTargetLengthDelay( constraint.m_Value );
                        m_settings.SetTargetLength( MINOPTMAX<int>() );
                    }
                    else
                    {
                        m_settings.SetTargetLengthDelay( MINOPTMAX<int>() );
                        m_settings.SetTargetLength( constraint.m_Value );
                    }

                    m_settings.m_isTimeDomain = constraint.m_IsTimeDomain;
                    aTool->GetManager()->PostEvent( EVENTS::SelectedItemsModified );
                }
            }
            else
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_SKEW,
                                               &pnsItem, &pnsCoupledItem, layer, &constraint ) )
                {
                    if( constraint.m_IsTimeDomain )
                    {
                        m_settings.SetTargetSkewDelay( constraint.m_Value );
                        m_settings.SetTargetSkew( MINOPTMAX<int>() );
                    }
                    else
                    {
                        m_settings.SetTargetSkewDelay( MINOPTMAX<int>() );
                        m_settings.SetTargetSkew( constraint.m_Value );
                    }

                    m_settings.m_isTimeDomain = constraint.m_IsTimeDomain;
                    aTool->GetManager()->PostEvent( EVENTS::SelectedItemsModified );
                }
            }
        }
    }
}


static PNS::LINKED_ITEM* pickSegment( PNS::ROUTER* aRouter, const VECTOR2I& aWhere, int aLayer,
                                      VECTOR2I&               aPointOut,
                                      const SHAPE_LINE_CHAIN& aBaseline = SHAPE_LINE_CHAIN() )
{
    int  maxSlopRadius = aRouter->Sizes().Clearance() + aRouter->Sizes().TrackWidth() / 2;

    static const int  candidateCount = 2;
    PNS::LINKED_ITEM* prioritized[candidateCount];
    SEG::ecoord       dist[candidateCount];
    SEG::ecoord       distBaseline[candidateCount];
    VECTOR2I          point[candidateCount];

    for( int i = 0; i < candidateCount; i++ )
    {
        prioritized[i] = nullptr;
        dist[i] = VECTOR2I::ECOORD_MAX;
        distBaseline[i] = VECTOR2I::ECOORD_MAX;
    }

    for( int slopRadius : { 0, maxSlopRadius } )
    {
        PNS::ITEM_SET candidates = aRouter->QueryHoverItems( aWhere, slopRadius );

        for( PNS::ITEM* item : candidates.Items() )
        {
            if( !item->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
                continue;

            if( !item->IsRoutable() )
                continue;

            if( !item->Layers().Overlaps( aLayer ) )
                continue;

            PNS::LINKED_ITEM* linked = static_cast<PNS::LINKED_ITEM*>( item );

            if( item->Kind() & PNS::ITEM::ARC_T )
            {
                PNS::ARC* pnsArc = static_cast<PNS::ARC*>( item );

                VECTOR2I    nearest = pnsArc->Arc().NearestPoint( aWhere );
                SEG::ecoord d0 = ( nearest - aWhere ).SquaredEuclideanNorm();

                if( d0 > dist[1] )
                    continue;

                if( aBaseline.PointCount() > 0 )
                {
                    SEG::ecoord dcBaseline;
                    VECTOR2I    target = pnsArc->Arc().GetArcMid();

                    if( aBaseline.SegmentCount() > 0 )
                        dcBaseline = aBaseline.SquaredDistance( target );
                    else
                        dcBaseline = ( aBaseline.CPoint( 0 ) - target ).SquaredEuclideanNorm();

                    if( dcBaseline > distBaseline[1] )
                        continue;

                    distBaseline[1] = dcBaseline;
                }

                prioritized[1] = linked;
                dist[1] = d0;
                point[1] = nearest;
            }
            else if( item->Kind() & PNS::ITEM::SEGMENT_T )
            {
                PNS::SEGMENT* segm = static_cast<PNS::SEGMENT*>( item );

                VECTOR2I    nearest = segm->CLine().NearestPoint( aWhere, false );
                SEG::ecoord dd = ( aWhere - nearest ).SquaredEuclideanNorm();

                if( dd > dist[1] )
                    continue;

                if( aBaseline.PointCount() > 0 )
                {
                    SEG::ecoord dcBaseline;
                    VECTOR2I    target = segm->Shape( -1 )->Centre();

                    if( aBaseline.SegmentCount() > 0 )
                        dcBaseline = aBaseline.SquaredDistance( target );
                    else
                        dcBaseline = ( aBaseline.CPoint( 0 ) - target ).SquaredEuclideanNorm();

                    if( dcBaseline > distBaseline[1] )
                        continue;

                    distBaseline[1] = dcBaseline;
                }

                prioritized[1] = segm;
                dist[1] = dd;
                point[1] = nearest;
            }
        }
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


static std::optional<PNS::LINE> getPNSLine( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                            PNS::ROUTER* router, int layer, VECTOR2I& aStartOut,
                                            VECTOR2I& aEndOut )
{
    PNS::NODE* world = router->GetWorld();

    PNS::LINKED_ITEM* startItem = pickSegment( router, aStart, layer, aStartOut );
    PNS::LINKED_ITEM* endItem = pickSegment( router, aEnd, layer, aEndOut );

    for( PNS::LINKED_ITEM* testItem : { startItem, endItem } )
    {
        if( !testItem )
            continue;

        PNS::LINE        line = world->AssembleLine( testItem, nullptr, false, false );
        SHAPE_LINE_CHAIN oldChain = line.CLine();

        if( oldChain.PointOnEdge( aStartOut, 1 ) && oldChain.PointOnEdge( aEndOut, 1 ) )
            return line;
    }

    return std::nullopt;
}


bool PCB_TUNING_PATTERN::initBaseLine( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard,
                                       VECTOR2I& aStart, VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                                       std::optional<SHAPE_LINE_CHAIN>& aBaseLine )
{
    PNS::NODE* world = aRouter->GetWorld();

    aStart = snapToNearestTrack( aStart, aBoard, aNet, nullptr );
    aEnd = snapToNearestTrack( aEnd, aBoard, aNet, nullptr );

    VECTOR2I startSnapPoint, endSnapPoint;

    PNS::LINKED_ITEM* startItem = pickSegment( aRouter, aStart, aPNSLayer, startSnapPoint );
    PNS::LINKED_ITEM* endItem = pickSegment( aRouter, aEnd, aPNSLayer, endSnapPoint );

    wxASSERT( startItem );
    wxASSERT( endItem );

    if( !startItem || !endItem )
        return false;

    PNS::LINE               line = world->AssembleLine( startItem );
    const SHAPE_LINE_CHAIN& chain = line.CLine();

    wxASSERT( line.ContainsLink( endItem ) );

    wxASSERT( chain.PointOnEdge( startSnapPoint, 40000 ) );
    wxASSERT( chain.PointOnEdge( endSnapPoint, 40000 ) );

    SHAPE_LINE_CHAIN pre;
    SHAPE_LINE_CHAIN mid;
    SHAPE_LINE_CHAIN post;

    chain.Split( startSnapPoint, endSnapPoint, pre, mid, post );

    aBaseLine = mid;

    return true;
}


bool PCB_TUNING_PATTERN::initBaseLines( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard )
{
    m_baseLineCoupled.reset();

    PCB_TRACK* track = nullptr;

    m_origin = snapToNearestTrack( m_origin, aBoard, nullptr, &track );
    wxCHECK( track, false );

    NETINFO_ITEM* net = track->GetNet();

    if( !initBaseLine( aRouter, aPNSLayer, aBoard, m_origin, m_end, net, m_baseLine ) )
        return false;

    // Generate both baselines even if we're skewing.  We need the coupled baseline to run the
    // DRC rules against.
    if( m_tuningMode == DIFF_PAIR || m_tuningMode == DIFF_PAIR_SKEW )
    {
        if( NETINFO_ITEM* coupledNet = aBoard->DpCoupledNet( net ) )
        {
            VECTOR2I coupledStart = snapToNearestTrack( m_origin, aBoard, coupledNet, nullptr );
            VECTOR2I coupledEnd = snapToNearestTrack( m_end, aBoard, coupledNet, nullptr );

            return initBaseLine( aRouter, aPNSLayer, aBoard, coupledStart, coupledEnd, coupledNet,
                                 m_baseLineCoupled );
        }

        return false;
    }

    return true;
}

bool PCB_TUNING_PATTERN::removeToBaseline( PNS::ROUTER* aRouter, int aPNSLayer,
                                           SHAPE_LINE_CHAIN& aBaseLine )
{
    VECTOR2I startSnapPoint, endSnapPoint;

    std::optional<PNS::LINE> pnsLine = getPNSLine( aBaseLine.CPoint( 0 ), aBaseLine.CLastPoint(),
                                                   aRouter, aPNSLayer, startSnapPoint, endSnapPoint );

    wxCHECK( pnsLine, false );

    SHAPE_LINE_CHAIN pre;
    SHAPE_LINE_CHAIN mid;
    SHAPE_LINE_CHAIN post;
    pnsLine->CLine().Split( startSnapPoint, endSnapPoint, pre, mid, post );

    for( PNS::LINKED_ITEM* li : pnsLine->Links() )
        aRouter->GetInterface()->RemoveItem( li );

    aRouter->GetWorld()->Remove( *pnsLine );

    SHAPE_LINE_CHAIN straightChain;
    straightChain.Append( pre );
    straightChain.Append( aBaseLine );
    straightChain.Append( post );
    straightChain.Simplify();

    PNS::LINE straightLine( *pnsLine, straightChain );

    aRouter->GetWorld()->Add( straightLine, false );

    for( PNS::LINKED_ITEM* li : straightLine.Links() )
        aRouter->GetInterface()->AddItem( li );

    return true;
}


void PCB_TUNING_PATTERN::Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    SetFlags( IN_EDIT );

    aTool->Router()->SyncWorld();

    PNS::ROUTER* router = aTool->Router();
    PNS_KICAD_IFACE* iface = aTool->GetInterface();

    aCommit->Remove( this );

    aTool->ClearRouterChanges();

    // PNS layers and PCB layers have different coding. so convert PCB layer to PNS layer
    int pnslayer = iface->GetPNSLayerFromBoardLayer( GetLayer() );

    if( baselineValid() )
    {
        bool success = true;

        success &= removeToBaseline( router, pnslayer, *m_baseLine );

        if( m_tuningMode == DIFF_PAIR )
            success &= removeToBaseline( router, pnslayer, *m_baseLineCoupled );

        if( !success )
            recoverBaseline( router );
    }

    const std::vector<GENERATOR_PNS_CHANGES>& allPnsChanges = aTool->GetRouterChanges();

    for( const GENERATOR_PNS_CHANGES& pnsChanges : allPnsChanges )
    {
        const std::set<BOARD_ITEM*> routerRemovedItems = pnsChanges.removedItems;
        const std::set<BOARD_ITEM*> routerAddedItems = pnsChanges.addedItems;

        /*std::cout << "Push commits << " << allPnsChanges.size() << " routerRemovedItems "
                  << routerRemovedItems.size() << " routerAddedItems " << routerAddedItems.size()
                  << " m_removedItems " << m_removedItems.size() << std::endl;*/

        for( BOARD_ITEM* item : routerRemovedItems )
        {
            item->ClearSelected();
            aCommit->Remove( item );
        }

        for( BOARD_ITEM* item : routerAddedItems )
            aCommit->Add( item );
    }

    aCommit->Push( "Remove Tuning Pattern" );
}


bool PCB_TUNING_PATTERN::recoverBaseline( PNS::ROUTER* aRouter )
{
    PNS::SOLID queryItem;

    SHAPE_LINE_CHAIN* chain = static_cast<SHAPE_LINE_CHAIN*>( getOutline().Clone() );
    queryItem.SetShape( chain );        // PNS::SOLID takes ownership
    queryItem.SetLayer( m_layer );

    int lineWidth = 0;

    PNS::NODE::OBSTACLES          obstacles;
    PNS::COLLISION_SEARCH_OPTIONS opts;
    opts.m_useClearanceEpsilon = false;

    PNS::NODE* world = aRouter->GetWorld();
    PNS::NODE* branch = world->Branch();

    branch->QueryColliding( &queryItem, obstacles, opts );

    for( const PNS::OBSTACLE& obs : obstacles )
    {
        PNS::ITEM* item = obs.m_item;

        if( !item->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
            continue;

        if( PNS::LINKED_ITEM* li = dynamic_cast<PNS::LINKED_ITEM*>( item ) )
        {
            if( lineWidth == 0 || li->Width() < lineWidth )
                lineWidth = li->Width();
        }

        if( chain->PointInside( item->Anchor( 0 ), 10 )
            && chain->PointInside( item->Anchor( 1 ), 10 ) )
        {
            branch->Remove( item );
        }
    }

    if( lineWidth == 0 )
        lineWidth = pcbIUScale.mmToIU( 0.1 ); // Fallback

    if( baselineValid() )
    {
        NETINFO_ITEM* recoverNet = GetBoard()->FindNet( m_lastNetName );
        PNS::LINE     recoverLine;

        recoverLine.SetLayer( m_layer );
        recoverLine.SetWidth( lineWidth );
        recoverLine.Line() = *m_baseLine;
        recoverLine.SetNet( recoverNet );
        branch->Add( recoverLine, false );

        if( m_tuningMode == DIFF_PAIR || m_tuningMode == DIFF_PAIR_SKEW )
        {
            NETINFO_ITEM* recoverCoupledNet = GetBoard()->DpCoupledNet( recoverNet );
            PNS::LINE recoverLineCoupled;

            recoverLineCoupled.SetLayer( m_layer );
            recoverLineCoupled.SetWidth( lineWidth );
            recoverLineCoupled.Line() = *m_baseLineCoupled;
            recoverLineCoupled.SetNet( recoverCoupledNet );
            branch->Add( recoverLineCoupled, false );
        }
    }

    aRouter->CommitRouting( branch );

    //wxLogWarning( "PNS baseline recovered" );

    return true;
}


bool PCB_TUNING_PATTERN::resetToBaseline( GENERATOR_TOOL* aTool, int aPNSLayer,
                                          SHAPE_LINE_CHAIN& aBaseLine, bool aPrimary )
{
    PNS_KICAD_IFACE* iface = aTool->GetInterface();
    PNS::ROUTER*     router = aTool->Router();
    PNS::NODE*       world = router->GetWorld();
    VECTOR2I         startSnapPoint, endSnapPoint;

    std::optional<PNS::LINE> pnsLine = getPNSLine( aBaseLine.CPoint( 0 ), aBaseLine.CLastPoint(),
                                                   router, aPNSLayer, startSnapPoint, endSnapPoint );

    if( !pnsLine )
    {
        // TODO
        //recoverBaseline( aRouter );
        return true;
    }

    PNS::NODE* branch = world->Branch();

    SHAPE_LINE_CHAIN straightChain;
    {
        SHAPE_LINE_CHAIN pre, mid, post;
        pnsLine->CLine().Split( startSnapPoint, endSnapPoint, pre, mid, post );

        straightChain.Append( pre );
        straightChain.Append( aBaseLine );
        straightChain.Append( post );
        straightChain.Simplify();
    }

    branch->Remove( *pnsLine );

    SHAPE_LINE_CHAIN newLineChain;

    if( aPrimary )
    {
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

            newLineChain.Append( pre );
            newLineChain.Append( mid );
            newLineChain.Append( post );

            m_baseLine = mid;
        }
    }
    else
    {
        VECTOR2I start = straightChain.NearestPoint( m_origin );
        VECTOR2I end = straightChain.NearestPoint( m_end );

        {
            SHAPE_LINE_CHAIN pre, mid, post;
            straightChain.Split( start, end, pre, mid, post );

            newLineChain.Append( pre );
            newLineChain.Append( mid );
            newLineChain.Append( post );

            m_baseLineCoupled = mid;
        }
    }

    PNS::LINE newLine( *pnsLine, newLineChain );

    branch->Add( newLine, false );
    router->CommitRouting( branch );

    int clearance = router->GetRuleResolver()->Clearance( &newLine, nullptr );

    iface->DisplayItem( &newLine, clearance, true, PNS_COLLISION );

    return true;
}


bool PCB_TUNING_PATTERN::Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return false;

    KIGFX::VIEW*     view = aTool->GetManager()->GetView();
    PNS::ROUTER*     router = aTool->Router();
    PNS_KICAD_IFACE* iface = aTool->GetInterface();
    PCB_LAYER_ID     pcblayer = GetLayer();

    auto hideRemovedItems = [&]( bool aHide )
    {
        if( view )
        {
            for( const GENERATOR_PNS_CHANGES& pnsCommit : aTool->GetRouterChanges() )
            {
                for( BOARD_ITEM* item : pnsCommit.removedItems )
                {
                    if( view )
                        view->Hide( item, aHide, aHide );
                }
            }
        }
    };

    iface->SetStartLayerFromPCBNew( pcblayer );

    if( router->RoutingInProgress() )
    {
        router->StopRouting();
    }

    // PNS layers and PCB layers have different coding. so convert PCB layer to PNS layer
    int pnslayer = iface->GetPNSLayerFromBoardLayer( pcblayer );

    if( !baselineValid() )
    {
        initBaseLines( router, pnslayer, aBoard );
    }
    else
    {
        if( resetToBaseline( aTool, pnslayer, *m_baseLine, true ) )
        {
            m_origin = m_baseLine->CPoint( 0 );
            m_end = m_baseLine->CLastPoint();
        }
        else
        {
            //initBaseLines( router, layer, aBoard );
            return false;
        }

        if( m_tuningMode == DIFF_PAIR )
        {
            if( !resetToBaseline( aTool, pnslayer, *m_baseLineCoupled, false ) )
            {
                initBaseLines( router, pnslayer, aBoard );
                return false;
            }
        }
    }

    hideRemovedItems( true );
    // Snap points
    VECTOR2I startSnapPoint, endSnapPoint;

    wxCHECK( m_baseLine, false );

    PNS::LINKED_ITEM* startItem = pickSegment( router, m_origin, pnslayer, startSnapPoint, *m_baseLine);
    PNS::LINKED_ITEM* endItem = pickSegment( router, m_end, pnslayer, endSnapPoint, *m_baseLine );

    wxASSERT( startItem );
    wxASSERT( endItem );

    if( !startItem || !endItem )
        return false;

    router->SetMode( GetPNSMode() );

    if( !router->StartRouting( startSnapPoint, startItem, pnslayer ) )
    {
        //recoverBaseline( router );
        return false;
    }

    PNS::MEANDER_PLACER_BASE* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

    m_settings.m_keepEndpoints = true; // Required for re-grouping
    placer->UpdateSettings( m_settings );

    router->Move( m_end, nullptr );

    if( PNS::DP_MEANDER_PLACER* dpPlacer = dynamic_cast<PNS::DP_MEANDER_PLACER*>( placer ) )
    {
        m_trackWidth = dpPlacer->GetOriginPair().Width();
        m_diffPairGap = dpPlacer->GetOriginPair().Gap();
    }
    else
    {
        m_trackWidth = startItem->Width();
        m_diffPairGap = router->Sizes().DiffPairGap();
    }

    m_settings = placer->MeanderSettings();
    m_lastNetName = iface->GetNetName( startItem->Net() );
    m_tuningStatus = placer->TuningStatus();

    wxString statusMessage;

    switch ( m_tuningStatus )
    {
    case PNS::MEANDER_PLACER_BASE::TOO_LONG:  statusMessage = _( "too long" );  break;
    case PNS::MEANDER_PLACER_BASE::TOO_SHORT: statusMessage = _( "too short" ); break;
    case PNS::MEANDER_PLACER_BASE::TUNED:     statusMessage = _( "tuned" );     break;
    default:                                  statusMessage = _( "unknown" );   break;
    }

    wxString  result;
    EDA_UNITS userUnits = EDA_UNITS::MM;

    if( aTool->GetManager()->GetSettings() )
        userUnits = static_cast<EDA_UNITS>( aTool->GetManager()->GetSettings()->m_System.units );

    if( m_settings.m_isTimeDomain )
    {
        result = EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, EDA_UNITS::PS,
                                                           (double) placer->TuningLengthResult() );
    }
    else
    {
        result = EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, userUnits,
                                                           (double) placer->TuningLengthResult() );
    }

    m_tuningInfo.Printf( wxS( "%s (%s)" ), result, statusMessage );

    return true;
}


void PCB_TUNING_PATTERN::EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit,
                                   const wxString& aCommitMsg, int aCommitFlags )
{
    if( !( GetFlags() & IN_EDIT ) )
        return;

    ClearFlags( IN_EDIT );

    KIGFX::VIEW*      view = aTool->GetManager()->GetView();
    PNS::ROUTER*      router = aTool->Router();
    PNS_KICAD_IFACE*  iface = aTool->GetInterface();
    SHAPE_LINE_CHAIN  bounds = getOutline();
    int               epsilon = aBoard->GetDesignSettings().GetDRCEpsilon();

    iface->EraseView();

    if( router->RoutingInProgress() )
    {
        bool forceFinish = true;
        bool forceCommit = false;

        router->FixRoute( m_end, nullptr, forceFinish, forceCommit );
        router->StopRouting();
    }

    const std::vector<GENERATOR_PNS_CHANGES>& pnsCommits = aTool->GetRouterChanges();

    for( const GENERATOR_PNS_CHANGES& pnsCommit : pnsCommits )
    {
        const std::set<BOARD_ITEM*> routerRemovedItems = pnsCommit.removedItems;
        const std::set<BOARD_ITEM*> routerAddedItems = pnsCommit.addedItems;

        //std::cout << "Push commits << " << allPnsChanges.size() << " routerRemovedItems "
        //          << routerRemovedItems.size() << " routerAddedItems " << routerAddedItems.size()
        //          << " m_removedItems " << m_removedItems.size() << std::endl;

        for( BOARD_ITEM* item : routerRemovedItems )
        {
            if( view )
                view->Hide( item, false );

            aCommit->Remove( item );
        }

        for( BOARD_ITEM* item : routerAddedItems )
        {
            aCommit->Add( item );

            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
            {
                if( bounds.PointInside( track->GetStart(), epsilon )
                    && bounds.PointInside( track->GetEnd(), epsilon ) )
                {
                    AddItem( item );
                }
            }
        }
    }

    if( aCommitMsg.IsEmpty() )
        aCommit->Push( _( "Edit Tuning Pattern" ), aCommitFlags );
    else
        aCommit->Push( aCommitMsg, aCommitFlags );
}


void PCB_TUNING_PATTERN::EditRevert( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return;

    ClearFlags( IN_EDIT );

    PNS_KICAD_IFACE* iface = aTool->GetInterface();

    iface->EraseView();

    if( KIGFX::VIEW* view = aTool->GetManager()->GetView() )
    {
        for( const GENERATOR_PNS_CHANGES& pnsCommit : aTool->GetRouterChanges() )
        {
            for( BOARD_ITEM* item : pnsCommit.removedItems )
                view->Hide( item, false );
        }
    }

    aTool->Router()->StopRouting();

    if( aCommit )
        aCommit->Revert();
}


bool PCB_TUNING_PATTERN::MakeEditPoints( EDIT_POINTS& aPoints ) const
{
    VECTOR2I centerlineOffset;
    VECTOR2I centerlineOffsetEnd;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
    {
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;
        centerlineOffsetEnd = ( m_baseLineCoupled->CLastPoint() - m_end ) / 2;
    }

    aPoints.AddPoint( m_origin + centerlineOffset );
    aPoints.AddPoint( m_end + centerlineOffsetEnd );

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    int amplitude = m_settings.m_maxAmplitude + KiROUND( m_trackWidth / 2.0 );

    if( m_tuningMode == DIFF_PAIR )
        amplitude += m_trackWidth + m_diffPairGap;

    if( m_settings.m_initialSide == -1 )
        amplitude *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( amplitude );

    aPoints.AddPoint( base.A + widthHandleOffset );
    aPoints.Point( 2 ).SetGridConstraint( IGNORE_GRID );

    VECTOR2I spacingHandleOffset =
            widthHandleOffset + ( base.B - base.A ).Resize( KiROUND( m_settings.m_spacing * 1.5 ) );

    aPoints.AddPoint( base.A + spacingHandleOffset );
    aPoints.Point( 3 ).SetGridConstraint( IGNORE_GRID );

    return true;
}


bool PCB_TUNING_PATTERN::UpdateFromEditPoints( EDIT_POINTS& aEditPoints )
{
    VECTOR2I centerlineOffset;
    VECTOR2I centerlineOffsetEnd;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
    {
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;
        centerlineOffsetEnd = ( m_baseLineCoupled->CLastPoint() - m_end ) / 2;
    }

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    m_origin = aEditPoints.Point( 0 ).GetPosition() - centerlineOffset;
    m_end = aEditPoints.Point( 1 ).GetPosition() - centerlineOffsetEnd;

    if( aEditPoints.Point( 2 ).IsActive() )
    {
        VECTOR2I wHandle = aEditPoints.Point( 2 ).GetPosition();

        int value = base.LineDistance( wHandle );

        value -= KiROUND( m_trackWidth / 2.0 );

        if( m_tuningMode == DIFF_PAIR )
            value -= m_trackWidth + m_diffPairGap;

        SetMaxAmplitude( KiROUND( value / pcbIUScale.mmToIU( 0.01 ) ) * pcbIUScale.mmToIU( 0.01 ) );

        int side = base.Side( wHandle );

        if( side < 0 )
            m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
        else
            m_settings.m_initialSide = PNS::MEANDER_SIDE_RIGHT;
    }

    if( aEditPoints.Point( 3 ).IsActive() )
    {
        VECTOR2I wHandle = aEditPoints.Point( 2 ).GetPosition();
        VECTOR2I sHandle = aEditPoints.Point( 3 ).GetPosition();

        int value = KiROUND( SEG( base.A, wHandle ).LineDistance( sHandle ) / 1.5 );

        SetSpacing( KiROUND( value / pcbIUScale.mmToIU( 0.01 ) ) * pcbIUScale.mmToIU( 0.01 ) );
    }

    return true;
}


bool PCB_TUNING_PATTERN::UpdateEditPoints( EDIT_POINTS& aEditPoints )
{
    VECTOR2I centerlineOffset;
    VECTOR2I centerlineOffsetEnd;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
    {
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;
        centerlineOffsetEnd = ( m_baseLineCoupled->CLastPoint() - m_end ) / 2;
    }

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    int amplitude = m_settings.m_maxAmplitude + KiROUND( m_trackWidth / 2.0 );

    if( m_tuningMode == DIFF_PAIR )
        amplitude += m_trackWidth + m_diffPairGap;

    if( m_settings.m_initialSide == -1 )
        amplitude *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( amplitude );

    aEditPoints.Point( 0 ).SetPosition( m_origin + centerlineOffset );
    aEditPoints.Point( 1 ).SetPosition( m_end + centerlineOffsetEnd );

    aEditPoints.Point( 2 ).SetPosition( base.A + widthHandleOffset );

    VECTOR2I spacingHandleOffset =
            widthHandleOffset + ( base.B - base.A ).Resize( KiROUND( m_settings.m_spacing * 1.5 ) );

    aEditPoints.Point( 3 ).SetPosition( base.A + spacingHandleOffset );

    return true;
}


SHAPE_LINE_CHAIN PCB_TUNING_PATTERN::getOutline() const
{
    if( m_baseLine )
    {
        int clampedMaxAmplitude = m_settings.m_maxAmplitude;
        int minAllowedAmplitude = 0;
        int baselineOffset = m_tuningMode == DIFF_PAIR ? ( m_diffPairGap + m_trackWidth ) / 2 : 0;

        if( m_settings.m_cornerStyle == PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND )
        {
            minAllowedAmplitude = baselineOffset + m_trackWidth;
        }
        else
        {
            int correction = m_trackWidth * tan( 1 - tan( DEG2RAD( 22.5 ) ) );
            minAllowedAmplitude = baselineOffset + correction;
        }

        clampedMaxAmplitude = std::max( clampedMaxAmplitude, minAllowedAmplitude );

        if( m_settings.m_singleSided )
        {
            SHAPE_LINE_CHAIN clBase = *m_baseLine;
            SHAPE_LINE_CHAIN left, right;

            if( m_tuningMode != DIFF_PAIR )
            {
                int amplitude = clampedMaxAmplitude + KiROUND( m_trackWidth / 2.0 );

                SHAPE_LINE_CHAIN chain;

                if( clBase.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                       true ) )
                {
                    chain.Append( m_settings.m_initialSide >= 0 ? right : left );
                    chain.Append( clBase.Reverse() );
                    chain.SetClosed( true );

                    return chain;
                }
            }
            else if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
            {
                int amplitude = clampedMaxAmplitude + m_trackWidth + KiROUND( m_diffPairGap / 2.0 );

                SHAPE_LINE_CHAIN clCoupled = *m_baseLineCoupled;
                SHAPE_LINE_CHAIN chain1, chain2;

                if( clBase.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                       true ) )
                {
                    if( m_settings.m_initialSide >= 0 )
                        chain1.Append( right );
                    else
                        chain1.Append( left );

                    if( clBase.OffsetLine( KiROUND( m_trackWidth / 2.0 ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                           ARC_LOW_DEF, left, right, true ) )
                    {
                        if( m_settings.m_initialSide >= 0 )
                            chain1.Append( left.Reverse() );
                        else
                            chain1.Append( right.Reverse() );
                    }

                    chain1.SetClosed( true );
                }

                if( clCoupled.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                          true ) )
                {
                    if( m_settings.m_initialSide >= 0 )
                        chain2.Append( right );
                    else
                        chain2.Append( left );

                    if( clCoupled.OffsetLine( KiROUND( m_trackWidth / 2.0 ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                              ARC_LOW_DEF, left, right, true ) )
                    {
                        if( m_settings.m_initialSide >= 0 )
                            chain2.Append( left.Reverse() );
                        else
                            chain2.Append( right.Reverse() );
                    }

                    chain2.SetClosed( true );
                }

                SHAPE_POLY_SET merged;
                merged.BooleanAdd( chain1, chain2 );

                if( merged.OutlineCount() > 0 )
                    return merged.Outline( 0 );
            }
        }

        // Not single-sided / fallback
        SHAPE_POLY_SET   poly;
        SHAPE_LINE_CHAIN cl = *m_baseLine;

        int amplitude = 0;

        if( m_tuningMode == DIFF_PAIR )
            amplitude = clampedMaxAmplitude + m_diffPairGap / 2 + KiROUND( m_trackWidth );
        else
            amplitude = clampedMaxAmplitude + KiROUND( m_trackWidth / 2.0 );

        poly.OffsetLineChain( *m_baseLine, amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, false );

        if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
        {
            SHAPE_POLY_SET polyCoupled;
            polyCoupled.OffsetLineChain( *m_baseLineCoupled, amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                         ARC_LOW_DEF, false );

            poly.ClearArcs();
            polyCoupled.ClearArcs();

            SHAPE_POLY_SET merged;
            merged.BooleanAdd( poly, polyCoupled );

            if( merged.OutlineCount() > 0 )
                return merged.Outline( 0 );
        }

        if( poly.OutlineCount() > 0 )
            return poly.Outline( 0 );
    }

    return SHAPE_LINE_CHAIN();
}


void PCB_TUNING_PATTERN::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( !IsSelected() && !IsNew() )
        return;

    KIGFX::PREVIEW::DRAW_CONTEXT ctx( *aView );

    int size = KiROUND( aView->ToWorld( EDIT_POINT::POINT_SIZE ) * 0.8 );
    size = std::max( size, pcbIUScale.mmToIU( 0.05 ) );

    if( !HasFlag( IN_EDIT ) )
    {
        if( m_baseLine )
        {
            for( int i = 0; i < m_baseLine->SegmentCount(); i++ )
            {
                SEG seg = m_baseLine->CSegment( i );
                ctx.DrawLineDashed( seg.A, seg.B, size, size / 6, true );
            }
        }
        else
        {
            ctx.DrawLineDashed( m_origin, m_end, size, size / 6, false );
        }

        if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
        {
            for( int i = 0; i < m_baseLineCoupled->SegmentCount(); i++ )
            {
                SEG seg = m_baseLineCoupled->CSegment( i );
                ctx.DrawLineDashed( seg.A, seg.B, size, size / 6, true );
            }
        }
    }

    SHAPE_LINE_CHAIN chain = getOutline();

    for( int i = 0; i < chain.SegmentCount(); i++ )
    {
        SEG seg = chain.Segment( i );
        ctx.DrawLineDashed( seg.A, seg.B, size, size / 2, false );
    }
}


const STRING_ANY_MAP PCB_TUNING_PATTERN::GetProperties() const
{
    STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

    props.set( "tuning_mode", tuningToString( m_tuningMode ) );
    props.set( "initial_side", sideToString( m_settings.m_initialSide ) );
    props.set( "last_status", statusToString( m_tuningStatus ) );
    props.set( "is_time_domain", m_settings.m_isTimeDomain );

    props.set( "end", m_end );
    props.set( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    props.set( "single_sided", m_settings.m_singleSided );
    props.set( "rounded", m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND );

    props.set_iu( "max_amplitude", m_settings.m_maxAmplitude );
    props.set_iu( "min_amplitude", m_settings.m_minAmplitude );
    props.set_iu( "min_spacing", m_settings.m_spacing );
    props.set_iu( "target_length_min", m_settings.m_targetLength.Min() );
    props.set_iu( "target_length", m_settings.m_targetLength.Opt() );
    props.set_iu( "target_length_max", m_settings.m_targetLength.Max() );
    props.set_iu( "target_delay_min", m_settings.m_targetLengthDelay.Min() );
    props.set_iu( "target_delay", m_settings.m_targetLengthDelay.Opt() );
    props.set_iu( "target_delay_max", m_settings.m_targetLengthDelay.Max() );
    props.set_iu( "target_skew_min", m_settings.m_targetSkew.Min() );
    props.set_iu( "target_skew", m_settings.m_targetSkew.Opt() );
    props.set_iu( "target_skew_max", m_settings.m_targetSkew.Max() );
    props.set_iu( "last_track_width", m_trackWidth );
    props.set_iu( "last_diff_pair_gap", m_diffPairGap );

    props.set( "last_netname", m_lastNetName );
    props.set( "last_tuning", m_tuningInfo );
    props.set( "override_custom_rules", m_settings.m_overrideCustomRules );

    if( m_baseLine )
        props.set( "base_line", wxAny( *m_baseLine ) );

    if( m_baseLineCoupled )
        props.set( "base_line_coupled", wxAny( *m_baseLineCoupled ) );

    return props;
}


void PCB_TUNING_PATTERN::SetProperties( const STRING_ANY_MAP& aProps )
{
    PCB_GENERATOR::SetProperties( aProps );

    wxString tuningMode;
    aProps.get_to( "tuning_mode", tuningMode );
    m_tuningMode = tuningFromString( tuningMode.utf8_string() );

    wxString side;
    aProps.get_to( "initial_side", side );
    m_settings.m_initialSide = sideFromString( side.utf8_string() );

    wxString status;
    aProps.get_to( "last_status", status );
    m_tuningStatus = statusFromString( status.utf8_string() );

    aProps.get_to( "is_time_domain", m_settings.m_isTimeDomain );

    aProps.get_to( "end", m_end );
    aProps.get_to( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    aProps.get_to( "single_sided", m_settings.m_singleSided );
    aProps.get_to( "side", m_settings.m_initialSide );

    bool rounded = false;
    aProps.get_to( "rounded", rounded );
    m_settings.m_cornerStyle = rounded ? PNS::MEANDER_STYLE_ROUND : PNS::MEANDER_STYLE_CHAMFER;

    long long int val;

    aProps.get_to_iu( "target_length", val );
    m_settings.SetTargetLength( val );

    if( aProps.get_to_iu( "target_length_min", val ) )
        m_settings.m_targetLength.SetMin( val );

    if( aProps.get_to_iu( "target_length_max", val ) )
        m_settings.m_targetLength.SetMax( val );

    aProps.get_to_iu( "target_delay", val );
    m_settings.SetTargetLengthDelay( val );

    if( aProps.get_to_iu( "target_delay_min", val ) )
        m_settings.m_targetLengthDelay.SetMin( val );

    if( aProps.get_to_iu( "target_delay_max", val ) )
        m_settings.m_targetLengthDelay.SetMax( val );

    int int_val;

    aProps.get_to_iu( "target_skew", int_val );
    m_settings.SetTargetSkew( int_val );

    if( aProps.get_to_iu( "target_skew_min", int_val ) )
        m_settings.m_targetSkew.SetMin( int_val );

    if( aProps.get_to_iu( "target_skew_max", int_val ) )
        m_settings.m_targetSkew.SetMax( int_val );

    aProps.get_to_iu( "max_amplitude", m_settings.m_maxAmplitude );
    aProps.get_to_iu( "min_amplitude", m_settings.m_minAmplitude );
    aProps.get_to_iu( "min_spacing", m_settings.m_spacing );
    aProps.get_to_iu( "last_track_width", m_trackWidth );
    aProps.get_to_iu( "last_diff_pair_gap", m_diffPairGap );
    aProps.get_to( "override_custom_rules", m_settings.m_overrideCustomRules );

    aProps.get_to( "last_netname", m_lastNetName );
    aProps.get_to( "last_tuning", m_tuningInfo );

    if( auto baseLine = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line" ) )
        m_baseLine = *baseLine;

    if( auto baseLineCoupled = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line_coupled" ) )
        m_baseLineCoupled = *baseLineCoupled;
}


void PCB_TUNING_PATTERN::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame )
{
    PNS::MEANDER_SETTINGS settings = m_settings;
    DRC_CONSTRAINT        constraint;

    if( !m_items.empty() )
    {
        BOARD_ITEM*                  startItem = static_cast<BOARD_ITEM*>( *m_items.begin() );
        std::shared_ptr<DRC_ENGINE>& drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        if( m_tuningMode == DIFF_PAIR_SKEW )
        {
            constraint = drcEngine->EvalRules( SKEW_CONSTRAINT, startItem, nullptr, GetLayer() );

            if( !constraint.IsNull() && !settings.m_overrideCustomRules )
            {
                if( constraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN ) )
                {
                    settings.SetTargetLengthDelay( constraint.GetValue() );
                    settings.SetTargetLength( MINOPTMAX<int>() );
                    settings.m_isTimeDomain = true;
                }
                else
                {
                    settings.SetTargetLengthDelay( MINOPTMAX<int>() );
                    settings.SetTargetLength( constraint.GetValue() );
                    settings.m_isTimeDomain = false;
                }
            }
        }
        else
        {
            constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, startItem, nullptr, GetLayer() );

            if( !constraint.IsNull() && !settings.m_overrideCustomRules )
            {
                if( constraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN ) )
                {
                    settings.SetTargetLengthDelay( constraint.GetValue() );
                    settings.SetTargetLength( MINOPTMAX<int>() );
                    settings.m_isTimeDomain = true;
                }
                else
                {
                    settings.SetTargetLengthDelay( MINOPTMAX<int>() );
                    settings.SetTargetLength( constraint.GetValue() );
                    settings.m_isTimeDomain = false;
                }
            }
        }
    }

    DIALOG_TUNING_PATTERN_PROPERTIES dlg( aEditFrame, settings, GetPNSMode(), constraint );

    if( dlg.ShowModal() == wxID_OK )
    {
        BOARD_COMMIT commit( aEditFrame );
        commit.Modify( this );
        m_settings = settings;

        GENERATOR_TOOL* generatorTool = aEditFrame->GetToolManager()->GetTool<GENERATOR_TOOL>();
        EditStart( generatorTool, GetBoard(), &commit );
        Update( generatorTool, GetBoard(), &commit );
        EditPush( generatorTool, GetBoard(), &commit );
    }
}


std::vector<EDA_ITEM*> PCB_TUNING_PATTERN::GetPreviewItems( GENERATOR_TOOL* aTool,
                                                            PCB_BASE_EDIT_FRAME* aFrame,
                                                            bool aStatusItemsOnly )
{
    std::vector<EDA_ITEM*> previewItems;
    KIGFX::VIEW*           view = aFrame->GetCanvas()->GetView();

    if( auto* placer = dynamic_cast<PNS::MEANDER_PLACER_BASE*>( aTool->Router()->Placer() ) )
    {
        if( !aStatusItemsOnly )
        {
            PNS::ITEM_SET items = placer->TunedPath();

            for( PNS::ITEM* item : items )
                previewItems.push_back( new ROUTER_PREVIEW_ITEM( item,
                                                                  aTool->Router()->GetInterface(),
                                                                  view, PNS_HOVER_ITEM ) );
        }

        TUNING_STATUS_VIEW_ITEM* statusItem = new TUNING_STATUS_VIEW_ITEM( aFrame );

        if( m_tuningMode == DIFF_PAIR_SKEW )
        {
            if( m_settings.m_isTimeDomain )
                statusItem->SetMinMax( m_settings.m_targetSkewDelay.Min(), m_settings.m_targetSkewDelay.Max() );
            else
                statusItem->SetMinMax( m_settings.m_targetSkew.Min(), m_settings.m_targetSkew.Max() );
        }
        else
        {
            if( m_settings.m_isTimeDomain )
            {
                if( m_settings.m_targetLengthDelay.Opt() == PNS::MEANDER_SETTINGS::DELAY_UNCONSTRAINED )
                {
                    statusItem->ClearMinMax();
                }
                else
                {
                    statusItem->SetMinMax( static_cast<double>( m_settings.m_targetLengthDelay.Min() ),
                                           static_cast<double>( m_settings.m_targetLengthDelay.Max() ) );
                }
            }
            else
            {
                if( m_settings.m_targetLength.Opt() == PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED )
                {
                    statusItem->ClearMinMax();
                }
                else
                {
                    statusItem->SetMinMax( static_cast<double>( m_settings.m_targetLength.Min() ),
                                           static_cast<double>( m_settings.m_targetLength.Max() ) );
                }
            }
        }

        statusItem->SetIsTimeDomain( m_settings.m_isTimeDomain );

        if( m_tuningMode == DIFF_PAIR_SKEW )
        {
            if( m_settings.m_isTimeDomain )
                statusItem->SetCurrent( static_cast<double>( placer->TuningDelayResult() ), _( "current skew" ) );
            else
                statusItem->SetCurrent( static_cast<double>( placer->TuningLengthResult() ), _( "current skew" ) );
        }
        else
        {
            if( m_settings.m_isTimeDomain )
                statusItem->SetCurrent( static_cast<double>( placer->TuningDelayResult() ), _( "current delay" ) );
            else
                statusItem->SetCurrent( static_cast<double>( placer->TuningLengthResult() ), _( "current length" ) );
        }

        statusItem->SetPosition( aFrame->GetToolManager()->GetMousePosition() );
        previewItems.push_back( statusItem );
    }

    return previewItems;
}


void PCB_TUNING_PATTERN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString      msg;
    NETINFO_ITEM* primaryNet = nullptr;
    NETINFO_ITEM* coupledNet = nullptr;
    PCB_TRACK*    primaryItem = nullptr;
    PCB_TRACK*    coupledItem = nullptr;
    NETCLASS*     netclass = nullptr;
    int           width = 0;
    bool          mixedWidth = false;

    EDA_DATA_TYPE unitType = m_settings.m_isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

    aList.emplace_back( _( "Type" ), GetFriendlyName() );

    for( EDA_ITEM* member : GetItems() )
    {
        if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( member ) )
        {
            if( !primaryNet )
            {
                primaryItem = track;
                primaryNet = track->GetNet();
            }
            else if( !coupledNet && track->GetNet() != primaryNet )
            {
                coupledItem = track;
                coupledNet = track->GetNet();
            }

            if( !netclass )
                netclass = track->GetEffectiveNetClass();

            if( !width )
                width = track->GetWidth();
            else if( width != track->GetWidth() )
                mixedWidth = true;
        }
    }

    if( coupledNet )
    {
        aList.emplace_back( _( "Nets" ), UnescapeString( primaryNet->GetNetname() )
                                         + wxS( ", " )
                                         + UnescapeString( coupledNet->GetNetname() ) );
    }
    else if( primaryNet )
    {
        aList.emplace_back( _( "Net" ), UnescapeString( primaryNet->GetNetname() ) );
    }

    if( netclass )
        aList.emplace_back( _( "Resolved Netclass" ),
                            UnescapeString( netclass->GetHumanReadableName() ) );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    if( width && !mixedWidth )
        aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( width ) );

    BOARD*                       board = GetBoard();
    std::shared_ptr<DRC_ENGINE>& drcEngine = board->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT               constraint;

    // Display full track length (in Pcbnew)
    if( board && primaryItem && primaryItem->GetNetCode() > 0 )
    {
        int    count = 0;
        double trackLen = 0.0;
        double lenPadToDie = 0.0;
        double trackDelay = 0.0;
        double delayPadToDie = 0.0;

        std::tie( count, trackLen, lenPadToDie, trackDelay, delayPadToDie ) = board->GetTrackLength( *primaryItem );

        if( coupledItem && coupledItem->GetNetCode() > 0 )
        {
            double coupledLen = 0.0;
            double coupledLenPadToDie = 0.0;
            double coupledTrackDelay = 0.0;
            double doubledDelayPadToDie = 0.0;

            std::tie( count, coupledLen, coupledLenPadToDie, coupledTrackDelay, doubledDelayPadToDie ) =
                    board->GetTrackLength( *coupledItem );

            if( trackDelay == 0.0 || coupledTrackDelay == 0.0 )
            {
                aList.emplace_back( _( "Routed Lengths" ), aFrame->MessageTextFromValue( trackLen ) + wxS( ", " )
                                                                   + aFrame->MessageTextFromValue( coupledLen ) );
            }
            else
            {
                aList.emplace_back(
                        _( "Routed Delays" ),
                        aFrame->MessageTextFromValue( trackDelay, true, EDA_DATA_TYPE::TIME ) + wxS( ", " )
                                + aFrame->MessageTextFromValue( coupledTrackDelay, true, EDA_DATA_TYPE::TIME ) );
            }
        }
        else
        {
            if( trackDelay == 0.0 )
            {
                aList.emplace_back( _( "Routed Length" ), aFrame->MessageTextFromValue( trackLen ) );
            }
            else
            {
                aList.emplace_back( _( "Routed Delay" ),
                                    aFrame->MessageTextFromValue( trackDelay, true, EDA_DATA_TYPE::TIME ) );
            }
        }

        if( lenPadToDie != 0 && delayPadToDie == 0.0 )
        {
            msg = aFrame->MessageTextFromValue( lenPadToDie );
            aList.emplace_back( _( "Pad To Die Length" ), msg );

            msg = aFrame->MessageTextFromValue( trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg );
        }
        else if( delayPadToDie > 0.0 )
        {
            msg = aFrame->MessageTextFromValue( delayPadToDie, true, EDA_DATA_TYPE::TIME );
            aList.emplace_back( _( "Pad To Die Delay" ), msg );

            msg = aFrame->MessageTextFromValue( trackDelay + delayPadToDie, true, EDA_DATA_TYPE::TIME );
            aList.emplace_back( _( "Full Delay" ), msg );
        }
    }

    if( m_tuningMode == DIFF_PAIR_SKEW )
    {
        constraint = drcEngine->EvalRules( SKEW_CONSTRAINT, primaryItem, coupledItem, m_layer );

        if( constraint.IsNull() || m_settings.m_overrideCustomRules )
        {
            msg = aFrame->MessageTextFromValue( m_settings.m_targetSkew.Opt() );

            aList.emplace_back( wxString::Format( _( "Target Skew: %s" ), msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = aFrame->MessageTextFromMinOptMax( constraint.GetValue() );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( _( "Skew Constraints: %s" ), msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
    else
    {
        constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, primaryItem, coupledItem, m_layer );

        if( constraint.IsNull() || m_settings.m_overrideCustomRules )
        {
            wxString caption;

            if( m_settings.m_isTimeDomain )
            {
                caption = _( "Target Delay: %s" );
                msg = aFrame->MessageTextFromValue( static_cast<double>( m_settings.m_targetLengthDelay.Opt() ), true,
                                                    EDA_DATA_TYPE::TIME );
            }
            else
            {
                caption = _( "Target Length: %s" );
                msg = aFrame->MessageTextFromValue( static_cast<double>( m_settings.m_targetLength.Opt() ) );
            }

            aList.emplace_back( wxString::Format( caption, msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = aFrame->MessageTextFromMinOptMax( constraint.GetValue(), unitType );

            wxString caption = m_settings.m_isTimeDomain ? _( "Delay Constraints: %s" ) : _( "Length Constraints: %s" );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( caption, msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
}


const wxString PCB_TUNING_PATTERN::DISPLAY_NAME = _HKI( "Tuning Pattern" );
const wxString PCB_TUNING_PATTERN::GENERATOR_TYPE = wxS( "tuning_pattern" );


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


#define HITTEST_THRESHOLD_PIXELS 5


int DRAWING_TOOL::PlaceTuningPattern( const TOOL_EVENT& aEvent )
{
    // TODO: (JJ) Reserving before v9 string freeze
    wxLogDebug( _( "Tune Skew" ) );

    if( m_isFootprintEditor )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );
    Activate();

    BOARD*                       board = m_frame->GetBoard();
    BOARD_DESIGN_SETTINGS&       bds = board->GetDesignSettings();
    std::shared_ptr<DRC_ENGINE>& drcEngine = bds.m_DRCEngine;
    GENERATOR_TOOL*              generatorTool = m_toolMgr->GetTool<GENERATOR_TOOL>();
    PNS::ROUTER*                 router = generatorTool->Router();
    PNS::ROUTER_MODE             routerMode = aEvent.Parameter<PNS::ROUTER_MODE>();
    LENGTH_TUNING_MODE           mode = fromPNSMode( routerMode );
    PNS::MEANDER_SETTINGS        meanderSettings;

    switch( mode )
    {
    case LENGTH_TUNING_MODE::SINGLE: meanderSettings = bds.m_SingleTrackMeanderSettings; break;
    case DIFF_PAIR:                  meanderSettings = bds.m_DiffPairMeanderSettings;    break;
    case DIFF_PAIR_SKEW:             meanderSettings = bds.m_SkewMeanderSettings;        break;
    }

    KIGFX::VIEW_CONTROLS*    controls = getViewControls();
    PCB_SELECTION_TOOL*      selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
    SCOPED_DRAW_MODE         scopedDrawMode( m_mode, MODE::TUNING );

    m_pickerItem = nullptr;
    m_tuningPattern = nullptr;

    // Add a VIEW_GROUP that serves as a preview for the new item
    m_preview.Clear();
    m_view->Add( &m_preview );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
                controls->ShowCursor( true );
            };

    auto applyCommonSettings =
            [&]( PCB_TUNING_PATTERN* aPattern )
            {
                const auto& origTargetLength = aPattern->GetSettings().m_targetLength;
                const auto& origTargetSkew   = aPattern->GetSettings().m_targetSkew;

                aPattern->GetSettings() = meanderSettings;

                if( meanderSettings.m_targetLength.IsNull() )
                    aPattern->GetSettings().m_targetLength = origTargetLength;

                if( meanderSettings.m_targetSkew.IsNull() )
                    aPattern->GetSettings().m_targetSkew = origTargetSkew;
            };

    auto updateHoverStatus =
            [&]()
            {
                std::unique_ptr<PCB_TUNING_PATTERN> dummyPattern;

                if( m_pickerItem )
                {
                    dummyPattern.reset( PCB_TUNING_PATTERN::CreateNew( generatorTool, m_frame,
                                                                       m_pickerItem, mode ) );
                    dummyPattern->SetPosition( m_pickerItem->GetFocusPosition() );
                    dummyPattern->SetEnd( m_pickerItem->GetFocusPosition() );
                }

                if( dummyPattern )
                {
                    applyCommonSettings( dummyPattern.get() );

                    dummyPattern->EditStart( generatorTool, m_board, nullptr );
                    dummyPattern->Update( generatorTool, m_board, nullptr );

                    m_preview.FreeItems();

                    for( EDA_ITEM* item : dummyPattern->GetPreviewItems( generatorTool, m_frame ) )
                        m_preview.Add( item );

                    generatorTool->Router()->StopRouting();

                    m_view->Update( &m_preview );
                }
                else
                {

                    m_preview.FreeItems();
                    m_view->Update( &m_preview );
                }
            };

    auto updateTuningPattern =
            [&]()
            {
                if( m_tuningPattern && m_tuningPattern->GetPosition() != m_tuningPattern->GetEnd() )
                {
                    m_tuningPattern->EditStart( generatorTool, m_board, nullptr );
                    m_tuningPattern->Update( generatorTool, m_board, nullptr );

                    m_preview.FreeItems();

                    for( EDA_ITEM* item : m_tuningPattern->GetPreviewItems( generatorTool, m_frame, true ) )
                        m_preview.Add( item );

                    m_view->Update( &m_preview );
                }
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        VECTOR2D cursorPos = controls->GetMousePosition();

        if( evt->IsCancelInteractive() || evt->IsActivate()
            || ( m_tuningPattern && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( m_tuningPattern )
            {
                // First click already made; clean up tuning pattern preview
                m_tuningPattern->EditRevert( generatorTool, m_board, nullptr );

                delete m_tuningPattern;
                m_tuningPattern = nullptr;
            }
            else
            {
                break;
            }
        }
        else if( evt->IsMotion() )
        {
            if( !m_tuningPattern )
            {
                // First click not yet made; we're in highlight-net-under-cursor mode

                GENERAL_COLLECTOR collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );

                if( m_frame->GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
                    guide.SetIncludeSecondary( false );
                else
                    guide.SetIncludeSecondary( true );

                guide.SetPreferredLayer( m_frame->GetActiveLayer() );

                collector.Collect( board, { PCB_TRACE_T, PCB_ARC_T }, cursorPos, guide );

                if( collector.GetCount() > 1 )
                    selectionTool->GuessSelectionCandidates( collector, cursorPos );

                m_pickerItem = nullptr;

                if( collector.GetCount() > 0 )
                {
                    double min_dist_sq = std::numeric_limits<double>::max();

                    for( EDA_ITEM* candidate : collector )
                    {
                        VECTOR2I candidatePos;

                        if( candidate->Type() == PCB_TRACE_T )
                        {
                            candidatePos = static_cast<PCB_TRACK*>( candidate )->GetCenter();
                        }
                        else if( candidate->Type() == PCB_ARC_T )
                        {
                            candidatePos = static_cast<PCB_ARC*>( candidate )->GetMid();
                        }

                        double dist_sq = ( cursorPos - candidatePos ).SquaredEuclideanNorm();

                        if( dist_sq < min_dist_sq )
                        {
                            min_dist_sq = dist_sq;
                            m_pickerItem = static_cast<BOARD_CONNECTED_ITEM*>( candidate );
                        }
                    }
                }

                updateHoverStatus();
            }
            else
            {
                // First click already made; we're in preview-tuning-pattern mode

                m_tuningPattern->SetEnd( cursorPos );
                m_tuningPattern->UpdateSideFromEnd();

                updateTuningPattern();
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_pickerItem && !m_tuningPattern )
            {
                // First click; create a tuning pattern

                if( dynamic_cast<PCB_TUNING_PATTERN*>( m_pickerItem->GetParentGroup() ) )
                {
                    m_frame->ShowInfoBarWarning( _( "Unable to tune segments inside other "
                                                    "tuning patterns." ) );
                }
                else
                {
                    m_preview.FreeItems();

                    m_frame->SetActiveLayer( m_pickerItem->GetLayer() );
                    m_tuningPattern = PCB_TUNING_PATTERN::CreateNew( generatorTool, m_frame,
                                                                     m_pickerItem, mode );

                    applyCommonSettings( m_tuningPattern );

                    int      dummyDist;
                    int      dummyClearance = std::numeric_limits<int>::max() / 2;
                    VECTOR2I closestPt;

                    // With an artificially-large clearance this can't *not* collide, but the
                    // if stmt keeps Coverity happy....
                    if( m_pickerItem->GetEffectiveShape()->Collide( cursorPos, dummyClearance,
                                                                    &dummyDist, &closestPt ) )
                    {
                        m_tuningPattern->SetPosition( closestPt );
                        m_tuningPattern->SetEnd( closestPt );
                    }

                    m_preview.Add( m_tuningPattern->Clone() );
                }
            }
            else if( m_pickerItem && m_tuningPattern )
            {
                // Second click; we're done
                BOARD_COMMIT commit( m_frame );

                m_tuningPattern->EditStart( generatorTool, m_board, &commit );
                m_tuningPattern->Update( generatorTool, m_board, &commit );
                m_tuningPattern->EditPush( generatorTool, m_board, &commit, _( "Tune" ) );

                m_tuningPattern = nullptr;
                m_pickerItem = nullptr;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            PCB_SELECTION dummy;
            m_menu->ShowContextMenu( dummy );
        }
        else if( evt->IsAction( &PCB_ACTIONS::spacingIncrease )
                 || evt->IsAction( &PCB_ACTIONS::spacingDecrease ) )
        {
            if( m_tuningPattern )
            {
                auto* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

                placer->SpacingStep( evt->IsAction( &PCB_ACTIONS::spacingIncrease ) ? 1 : -1 );
                m_tuningPattern->SetSpacing( placer->MeanderSettings().m_spacing );
                meanderSettings.m_spacing = placer->MeanderSettings().m_spacing;

                updateTuningPattern();
            }
            else
            {
                m_frame->ShowInfoBarWarning( _( "Select a track to tune first." ) );
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::amplIncrease )
                 || evt->IsAction( &PCB_ACTIONS::amplDecrease ) )
        {
            if( m_tuningPattern )
            {
                auto* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

                placer->AmplitudeStep( evt->IsAction( &PCB_ACTIONS::amplIncrease ) ? 1 : -1 );
                m_tuningPattern->SetMaxAmplitude( placer->MeanderSettings().m_maxAmplitude );
                meanderSettings.m_maxAmplitude = placer->MeanderSettings().m_maxAmplitude;

                updateTuningPattern();
            }
            else
            {
                m_frame->ShowInfoBarWarning( _( "Select a track to tune first." ) );
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties )
                 || evt->IsAction( &PCB_ACTIONS::lengthTunerSettings ) )
        {
            DRC_CONSTRAINT constraint;

            if( m_tuningPattern )
            {
                if( !m_tuningPattern->GetItems().empty() )
                {
                    BOARD_ITEM* startItem = *m_tuningPattern->GetBoardItems().begin();

                    constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, startItem, nullptr,
                                                       startItem->GetLayer() );
                }
            }

            DIALOG_TUNING_PATTERN_PROPERTIES dlg( m_frame, meanderSettings, routerMode, constraint );

            if( dlg.ShowModal() == wxID_OK )
            {
                if( m_tuningPattern )
                    applyCommonSettings( m_tuningPattern );

                updateTuningPattern();
            }
        }
        // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
        // but we don't at present have that, so we just knock out some of the egregious ones.
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->CaptureCursor( m_tuningPattern != nullptr );
        controls->SetAutoPan( m_tuningPattern != nullptr );
    }

    controls->CaptureCursor( false );
    controls->SetAutoPan( false );
    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    m_preview.FreeItems();
    m_view->Remove( &m_preview );

    m_frame->GetCanvas()->Refresh();

    if( m_tuningPattern )
        selectionTool->AddItemToSel( m_tuningPattern );

    m_frame->PopTool( aEvent );
    return 0;
}


static struct PCB_TUNING_PATTERN_DESC
{
    PCB_TUNING_PATTERN_DESC()
    {
        ENUM_MAP<LENGTH_TUNING_MODE>::Instance()
                .Map( LENGTH_TUNING_MODE::SINGLE, _HKI( "Single track" ) )
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR, _HKI( "Differential pair" ) )
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR_SKEW, _HKI( "Diff pair skew" ) );

        ENUM_MAP<PNS::MEANDER_SIDE>::Instance()
                .Map( PNS::MEANDER_SIDE_LEFT, _HKI( "Left" ) )
                .Map( PNS::MEANDER_SIDE_RIGHT, _HKI( "Right" ) )
                .Map( PNS::MEANDER_SIDE_DEFAULT, _HKI( "Default" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TUNING_PATTERN );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TUNING_PATTERN, PCB_GENERATOR> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TUNING_PATTERN, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TUNING_PATTERN ), TYPE_HASH( PCB_GENERATOR ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TUNING_PATTERN ), TYPE_HASH( BOARD_ITEM ) );

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        auto layer = new PROPERTY_ENUM<PCB_TUNING_PATTERN, PCB_LAYER_ID>(
                _HKI( "Layer" ), &PCB_TUNING_PATTERN::SetLayer, &PCB_TUNING_PATTERN::GetLayer );
        layer->SetChoices( layerEnum.Choices() );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layer );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Width" ),
                                     &PCB_TUNING_PATTERN::SetWidth, &PCB_TUNING_PATTERN::GetWidth,
                                     PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, int>( _HKI( "Net" ),
                                     &PCB_TUNING_PATTERN::SetNetCode,
                                     &PCB_TUNING_PATTERN::GetNetCode ) );

        const wxString groupTechLayers = _HKI( "Technical Layers" );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>( _HKI( "Soldermask" ),
                                     &PCB_TUNING_PATTERN::SetHasSolderMask,
                                     &PCB_TUNING_PATTERN::HasSolderMask ), groupTechLayers );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>( _HKI( "Soldermask Margin Override" ),
                                     &PCB_TUNING_PATTERN::SetLocalSolderMaskMargin,
                                     &PCB_TUNING_PATTERN::GetLocalSolderMaskMargin,
                                     PROPERTY_DISPLAY::PT_SIZE ), groupTechLayers );

        const wxString groupTab = _HKI( "Pattern Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "End X" ), &PCB_TUNING_PATTERN::SetEndX,
                                     &PCB_TUNING_PATTERN::GetEndX, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "End Y" ), &PCB_TUNING_PATTERN::SetEndY,
                                     &PCB_TUNING_PATTERN::GetEndY, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, LENGTH_TUNING_MODE>(
                                     _HKI( "Tuning Mode" ),
                                     NO_SETTER( PCB_TUNING_PATTERN, LENGTH_TUNING_MODE ),
                                     &PCB_TUNING_PATTERN::GetTuningMode ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Min Amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMinAmplitude,
                                     &PCB_TUNING_PATTERN::GetMinAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Max Amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMaxAmplitude,
                                     &PCB_TUNING_PATTERN::GetMaxAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, PNS::MEANDER_SIDE>(
                                     _HKI( "Initial Side" ),
                                     &PCB_TUNING_PATTERN::SetInitialSide,
                                     &PCB_TUNING_PATTERN::GetInitialSide ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Min Spacing" ), &PCB_TUNING_PATTERN::SetSpacing,
                                     &PCB_TUNING_PATTERN::GetSpacing, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Corner Radius %" ),
                                     &PCB_TUNING_PATTERN::SetCornerRadiusPercentage,
                                     &PCB_TUNING_PATTERN::GetCornerRadiusPercentage,
                                     PROPERTY_DISPLAY::PT_DEFAULT, ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab );

        auto isSkew =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_TUNING_PATTERN* pattern = dynamic_cast<PCB_TUNING_PATTERN*>( aItem ) )
                        return pattern->GetTuningMode() == DIFF_PAIR_SKEW;

                    return false;
                };

        auto isTimeDomain = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_TUNING_PATTERN* pattern = dynamic_cast<PCB_TUNING_PATTERN*>( aItem ) )
                return pattern->GetSettings().m_isTimeDomain;

            return false;
        };

        auto isLengthIsSpaceDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return !isSkew( aItem ) && !isTimeDomain( aItem );
        };

        auto isLengthIsTimeDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return !isSkew( aItem ) && isTimeDomain( aItem );
        };

        auto isSkewIsSpaceDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return isSkew( aItem ) && !isTimeDomain( aItem );
        };

        auto isSkewIsTimeDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return isSkew( aItem ) && isTimeDomain( aItem );
        };

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>(
                                     _HKI( "Target Length" ), &PCB_TUNING_PATTERN::SetTargetLength,
                                     &PCB_TUNING_PATTERN::GetTargetLength, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( isLengthIsSpaceDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>(
                                     _HKI( "Target Delay" ), &PCB_TUNING_PATTERN::SetTargetDelay,
                                     &PCB_TUNING_PATTERN::GetTargetDelay, PROPERTY_DISPLAY::PT_TIME,
                                     ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab )
                .SetAvailableFunc( isLengthIsTimeDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Target Skew" ), &PCB_TUNING_PATTERN::SetTargetSkew,
                                     &PCB_TUNING_PATTERN::GetTargetSkew, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( isSkewIsSpaceDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Target Skew Delay" ), &PCB_TUNING_PATTERN::SetTargetSkewDelay,
                                     &PCB_TUNING_PATTERN::GetTargetSkewDelay, PROPERTY_DISPLAY::PT_TIME,
                                     ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab )
                .SetAvailableFunc( isSkewIsTimeDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>(
                                     _HKI( "Override Custom Rules" ),
                                     &PCB_TUNING_PATTERN::SetOverrideCustomRules,
                                     &PCB_TUNING_PATTERN::GetOverrideCustomRules ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>(
                                     _HKI( "Single-sided" ),
                                     &PCB_TUNING_PATTERN::SetSingleSided,
                                     &PCB_TUNING_PATTERN::IsSingleSided ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>(
                                     _HKI( "Rounded" ), &PCB_TUNING_PATTERN::SetRounded,
                                     &PCB_TUNING_PATTERN::IsRounded ),
                             groupTab );
    }
} _PCB_TUNING_PATTERN_DESC;

ENUM_TO_WXANY( LENGTH_TUNING_MODE )
ENUM_TO_WXANY( PNS::MEANDER_SIDE )

static GENERATORS_MGR::REGISTER<PCB_TUNING_PATTERN> registerMe;

// Also register under the 7.99 name
template <typename T>
struct REGISTER_LEGACY_TUNING_PATTERN
{
    REGISTER_LEGACY_TUNING_PATTERN()
    {
        GENERATORS_MGR::Instance().Register( wxS( "meanders" ), T::DISPLAY_NAME,
                                             []()
                                             {
                                                 return new T;
                                             } );
    }
};

static REGISTER_LEGACY_TUNING_PATTERN<PCB_TUNING_PATTERN> registerMeToo;
