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
#include <dialogs/dialog_unit_entry.h>
#include <status_popup.h>
#include <collectors.h>
#include <scoped_set_reset.h>

#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_group.h>

#include <tool/edit_points.h>
#include <tools/drawing_tool.h>
#include <tools/generator_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>

#include <preview_items/draw_context.h>
#include <view/view.h>

#include <router/pns_meander_placer_base.h>
#include <router/pns_meander.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_segment.h>
#include <router/pns_arc.h>
#include <router/pns_topology.h>

#include <dialogs/dialog_tuning_pattern_properties.h>


enum LENGTH_TUNING_MODE
{
    SINGLE,
    DIFF_PAIR,
    DIFF_PAIR_SKEW
};


class PCB_TUNING_PATTERN : public PCB_GENERATOR
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_TUNING_PATTERN( BOARD_ITEM* aParent = nullptr, PCB_LAYER_ID aLayer = F_Cu,
                        LENGTH_TUNING_MODE aMode = LENGTH_TUNING_MODE::SINGLE );

    wxString GetGeneratorType() const override { return wxS( "tuning_pattern" ); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    wxString GetFriendlyName() const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    wxString GetPluralName() const override
    {
        return wxString( _( "Tuning Patterns" ) );
    }

    static PCB_TUNING_PATTERN* CreateNew( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                          BOARD_CONNECTED_ITEM* aStartItem,
                                          LENGTH_TUNING_MODE aMode, bool aAllowGUI = true );

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                    BOARD_COMMIT* aCommit ) override;

    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                 BOARD_COMMIT* aCommit ) override;

    void EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                   BOARD_COMMIT* aCommit, const wxString& aCommitMsg = wxEmptyString,
                   int aCommitFlags = 0 ) override;

    void EditRevert( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                     BOARD_COMMIT* aCommit ) override;

    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                 BOARD_COMMIT* aCommit ) override;

    bool MakeEditPoints( std::shared_ptr<EDIT_POINTS> points ) const override;

    bool UpdateFromEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints,
                               BOARD_COMMIT* aCommit ) override;

    bool UpdateEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints ) override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_origin += aMoveVector;
        m_end += aMoveVector;

        if( !this->HasFlag( IN_EDIT ) )
        {
            PCB_GROUP::Move( aMoveVector );

            if( m_baseLine )
                m_baseLine->Move( aMoveVector );

            if( m_baseLineCoupled )
                m_baseLineCoupled->Move( aMoveVector );
        }
    }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override
    {
        if( !this->HasFlag( IN_EDIT ) )
        {
            RotatePoint( m_origin, aRotCentre, aAngle );
            RotatePoint( m_end, aRotCentre, aAngle );
            PCB_GROUP::Rotate( aRotCentre, aAngle );

            if( m_baseLine )
                m_baseLine->Rotate( aAngle, aRotCentre );

            if( m_baseLineCoupled )
                m_baseLineCoupled->Rotate( aAngle, aRotCentre );
        }
    }

    const BOX2I GetBoundingBox() const override
    {
        return getRectShape().BBox();
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override
    {
        aCount = 0;
        aLayers[aCount++] = LAYER_ANCHOR;
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        return getRectShape().Collide( aPosition, aAccuracy );
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const override
    {
        return GetBoundingBox().Intersects( aRect );
    }

    const BOX2I ViewBBox() const override { return GetBoundingBox(); }

    EDA_ITEM* Clone() const override { return new PCB_TUNING_PATTERN( *this ); }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

    const VECTOR2I& GetEnd() const { return m_end; }
    void            SetEnd( const VECTOR2I& aValue ) { m_end = aValue; }

    int  GetEndX() const { return m_end.x; }
    void SetEndX( int aValue ) { m_end.x = aValue; }

    int  GetEndY() const { return m_end.y; }
    void SetEndY( int aValue ) { m_end.y = aValue; }

    LENGTH_TUNING_MODE GetTuningMode() const { return m_tuningMode; }

    int  GetMinAmplitude() const { return m_settings.m_minAmplitude; }
    void SetMinAmplitude( int aValue ) { m_settings.m_minAmplitude = aValue; }

    int  GetMaxAmplitude() const { return m_settings.m_maxAmplitude; }
    void SetMaxAmplitude( int aValue ) { m_settings.m_maxAmplitude = aValue; }

    PNS::MEANDER_SIDE GetInitialSide() const { return m_settings.m_initialSide; }
    void              SetInitialSide( PNS::MEANDER_SIDE aValue ) { m_settings.m_initialSide = aValue; }

    int  GetSpacing() const { return m_settings.m_spacing; }
    void SetSpacing( int aValue ) { m_settings.m_spacing = aValue; }

    long long int GetTargetLength() const { return m_settings.m_targetLength.Opt(); }
    void          SetTargetLength( long long int aValue ) { m_settings.SetTargetLength( aValue ); }

    int  GetTargetSkew() const { return m_settings.m_targetSkew.Opt(); }
    void SetTargetSkew( int aValue ) { m_settings.SetTargetSkew( aValue ); }

    bool GetOverrideCustomRules() const { return m_settings.m_overrideCustomRules; }
    void SetOverrideCustomRules( bool aOverride ) { m_settings.m_overrideCustomRules = aOverride; }

    int  GetCornerRadiusPercentage() const { return m_settings.m_cornerRadiusPercentage; }
    void SetCornerRadiusPercentage( int aValue ) { m_settings.m_cornerRadiusPercentage = aValue; }

    bool IsSingleSided() const { return m_settings.m_singleSided; }
    void SetSingleSided( bool aValue ) { m_settings.m_singleSided = aValue; }

    bool IsRounded() const { return m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND; }
    void SetRounded( bool aFlag ) { m_settings.m_cornerStyle = aFlag ? PNS::MEANDER_STYLE_ROUND
                                                                     : PNS::MEANDER_STYLE_CHAMFER; }

    std::vector<std::pair<wxString, wxVariant>> GetRowData() override
    {
        std::vector<std::pair<wxString, wxVariant>> data = PCB_GENERATOR::GetRowData();
        data.emplace_back( _HKI( "Net" ), m_lastNetName );
        data.emplace_back( _HKI( "Tuning" ), m_tuningInfo );
        return data;
    }

    const STRING_ANY_MAP GetProperties() const override;
    void SetProperties( const STRING_ANY_MAP& aProps ) override;

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override;

    void UpdateStatus( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                       STATUS_MIN_MAX_POPUP* aPopup ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    void swapData( BOARD_ITEM* aImage ) override
    {
        wxASSERT( aImage->Type() == PCB_GENERATOR_T );

        std::swap( *this, *static_cast<PCB_TUNING_PATTERN*>( aImage ) );
    }

    PNS::ROUTER_MODE toPNSMode();

    bool baselineValid();

    bool initBaseLine( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard, VECTOR2I& aStart,
                       VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                       std::optional<SHAPE_LINE_CHAIN>& aBaseLine );

    bool initBaseLines( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard );

    void removeToBaseline( PNS::ROUTER* aRouter, int aLayer, SHAPE_LINE_CHAIN& aBaseLine );

    bool resetToBaseline( PNS::ROUTER* aRouter, int aLayer, PCB_BASE_EDIT_FRAME* aFrame,
                          SHAPE_LINE_CHAIN& aBaseLine, bool aPrimary );

    SHAPE_LINE_CHAIN getRectShape() const;

protected:
    VECTOR2I              m_end;

    PNS::MEANDER_SETTINGS m_settings;
    bool                  m_unconstrained;

    std::optional<SHAPE_LINE_CHAIN> m_baseLine;
    std::optional<SHAPE_LINE_CHAIN> m_baseLineCoupled;

    int                   m_trackWidth;
    int                   m_diffPairGap;

    LENGTH_TUNING_MODE    m_tuningMode;

    wxString              m_lastNetName;
    wxString              m_tuningInfo;

    PNS::MEANDER_PLACER_BASE::TUNING_STATUS m_tuningStatus;

    // Temp storage during editing
    std::set<BOARD_ITEM*> m_removedItems;
};


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
        m_unconstrained( false ),
        m_trackWidth( 0 ),
        m_diffPairGap( 0 ),
        m_tuningMode( aMode ),
        m_tuningStatus( PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED )
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

        SEG seg( track->GetStart(), track->GetEnd() );

        VECTOR2I    nearest = seg.NearestPoint( aP );
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
                                                   LENGTH_TUNING_MODE aMode, bool aAllowGUI )
{
    BOARD*                       board = aStartItem->GetBoard();
    std::shared_ptr<DRC_ENGINE>& drcEngine = board->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT               constraint;
    PCB_LAYER_ID                 layer = aStartItem->GetLayer();

    if( aMode == SINGLE && board->DpCoupledNet( aStartItem->GetNet() ) )
        aMode = DIFF_PAIR;

    PCB_TUNING_PATTERN* pattern = new PCB_TUNING_PATTERN( board, layer, aMode );

    constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, aStartItem, nullptr, layer );

    if( aMode == DIFF_PAIR_SKEW )
    {
        if( !constraint.IsNull() )
        {
            pattern->m_settings.SetTargetSkew( constraint.GetValue() );
            pattern->m_settings.m_overrideCustomRules = false;
        }
        else if( aAllowGUI )
        {
            WX_UNIT_ENTRY_DIALOG dlg( aFrame, _( "Tune Skew" ), _( "Target skew:" ), 0 );

            if( dlg.ShowModal() != wxID_OK )
                return nullptr;

            pattern->m_settings.SetTargetSkew( dlg.GetValue() );
            pattern->m_settings.m_overrideCustomRules = true;
        }
        else
        {
            pattern->m_unconstrained = true;
        }
    }
    else
    {
        if( !constraint.IsNull() )
        {
            pattern->m_settings.SetTargetLength( constraint.GetValue() );
            pattern->m_settings.m_overrideCustomRules = false;
        }
        else if( aAllowGUI )
        {
            WX_UNIT_ENTRY_DIALOG dlg( aFrame, _( "Tune Length" ), _( "Target length:" ),
                                      100 * PCB_IU_PER_MM );

            if( dlg.ShowModal() != wxID_OK )
                return nullptr;

            pattern->m_settings.SetTargetLength( dlg.GetValue() );
            pattern->m_settings.m_overrideCustomRules = true;
        }
        else
        {
            pattern->m_unconstrained = true;
        }
    }

    pattern->SetFlags( IS_NEW );

    return pattern;
}

void PCB_TUNING_PATTERN::EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                    PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit )
{
    m_removedItems.clear();

    if( aCommit )
    {
        if( IsNew() )
            aCommit->Add( this );
        else
            aCommit->Modify( this );
    }

    SetFlags( IN_EDIT );

    int          layer = GetLayer();
    PNS::ROUTER* router = aTool->Router();

    aTool->ClearRouterCommit();
    router->SyncWorld();

    PNS::RULE_RESOLVER* resolver = router->GetRuleResolver();
    PNS::CONSTRAINT     constraint;

    if( !baselineValid() )
        initBaseLines( router, layer, aBoard );

    if( baselineValid() && !m_settings.m_overrideCustomRules )
    {
        PCB_TRACK* track = nullptr;

        m_origin = snapToNearestTrack( m_origin, aBoard, nullptr, &track );
        wxCHECK( track, /* void */ );

        NETINFO_ITEM* net = track->GetNet();
        PNS::SEGMENT  pnsItem( m_baseLine->CSegment( 0 ), net );

        if( m_tuningMode == SINGLE )
        {
            if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                           &pnsItem, nullptr, layer, &constraint ) )
            {
                m_settings.SetTargetLength( constraint.m_Value );
                aFrame->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
            }
        }
        else
        {
            NETINFO_ITEM* coupledNet = aBoard->DpCoupledNet( net );
            PNS::SEGMENT  coupledItem( m_baseLineCoupled->CSegment( 0 ), coupledNet );

            if( m_tuningMode == DIFF_PAIR )
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                               &pnsItem, &coupledItem, layer, &constraint ) )
                {
                    m_settings.SetTargetLength( constraint.m_Value );
                    aFrame->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
                }
            }
            else
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_SKEW,
                                               &pnsItem, &coupledItem, layer, &constraint ) )
                {
                    m_settings.m_targetSkew = constraint.m_Value;
                    aFrame->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
                }
            }
        }
    }
}


static PNS::LINKED_ITEM* pickSegment( PNS::ROUTER* aRouter, const VECTOR2I& aWhere, int aLayer,
                                      VECTOR2I& aPointOut )
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

    auto haveCandidates =
            [&]()
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


static std::optional<PNS::LINE> getPNSLine( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                            PNS::ROUTER* router, int layer, VECTOR2I& aStartOut,
                                            VECTOR2I& aEndOut )
{
    PNS::NODE* world = router->GetWorld();

    PNS::LINKED_ITEM* startItem = pickSegment( router, aStart, layer, aStartOut );
    PNS::LINKED_ITEM* endItem = pickSegment( router, aEnd, layer, aEndOut );

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


bool PCB_TUNING_PATTERN::initBaseLine( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard,
                                       VECTOR2I& aStart, VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                                       std::optional<SHAPE_LINE_CHAIN>& aBaseLine )
{
    PNS::NODE* world = aRouter->GetWorld();

    aStart = snapToNearestTrack( aStart, aBoard, aNet, nullptr );
    aEnd = snapToNearestTrack( aEnd, aBoard, aNet, nullptr );

    VECTOR2I startSnapPoint, endSnapPoint;

    PNS::LINKED_ITEM* startItem = pickSegment( aRouter, aStart, aLayer, startSnapPoint );
    PNS::LINKED_ITEM* endItem = pickSegment( aRouter, aEnd, aLayer, endSnapPoint );

    wxASSERT( startItem );
    wxASSERT( endItem );

    if( !startItem || !endItem || startSnapPoint == endSnapPoint )
        return false;

    PNS::LINE               line = world->AssembleLine( startItem );
    const SHAPE_LINE_CHAIN& chain = line.CLine();

    wxASSERT( line.ContainsLink( endItem ) );

    wxASSERT( chain.PointOnEdge( startSnapPoint, 1 ) );
    wxASSERT( chain.PointOnEdge( endSnapPoint, 1 ) );

    SHAPE_LINE_CHAIN pre;
    SHAPE_LINE_CHAIN mid;
    SHAPE_LINE_CHAIN post;

    chain.Split( startSnapPoint, endSnapPoint, pre, mid, post );

    aBaseLine = mid;

    return true;
}


bool PCB_TUNING_PATTERN::initBaseLines( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard )
{
    m_baseLineCoupled.reset();

    PCB_TRACK* track = nullptr;

    m_origin = snapToNearestTrack( m_origin, aBoard, nullptr, &track );
    wxCHECK( track, false );

    NETINFO_ITEM* net = track->GetNet();

    if( !initBaseLine( aRouter, aLayer, aBoard, m_origin, m_end, net, m_baseLine ) )
        return false;

    // Generate both baselines even if we're skewing.  We need the coupled baseline to run the
    // DRC rules against.
    if( m_tuningMode == DIFF_PAIR || m_tuningMode == DIFF_PAIR_SKEW )
    {
        if( NETINFO_ITEM* coupledNet = aBoard->DpCoupledNet( net ) )
        {
            VECTOR2I coupledStart = snapToNearestTrack( m_origin, aBoard, coupledNet, nullptr );
            VECTOR2I coupledEnd = snapToNearestTrack( m_end, aBoard, coupledNet, nullptr );

            return initBaseLine( aRouter, aLayer, aBoard, coupledStart, coupledEnd, coupledNet,
                                 m_baseLineCoupled );
        }

        return false;
    }

    return true;
}

void PCB_TUNING_PATTERN::removeToBaseline( PNS::ROUTER* aRouter, int aLayer,
                                           SHAPE_LINE_CHAIN& aBaseLine )
{
    VECTOR2I startSnapPoint, endSnapPoint;

    std::optional<PNS::LINE> pnsLine = getPNSLine( aBaseLine.CPoint( 0 ), aBaseLine.CPoint( -1 ),
                                                   aRouter, aLayer, startSnapPoint, endSnapPoint );

    wxCHECK( pnsLine, /* void */ );

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
}


void PCB_TUNING_PATTERN::Remove( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                 PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit )
{
    aTool->Router()->SyncWorld();

    PNS::ROUTER* router = aTool->Router();
    int          layer = GetLayer();
    int          undoFlags = 0;

    // Ungroup first so that undo works
    if( !GetItems().empty() )
    {
        PCB_GENERATOR*    group = this;
        PICKED_ITEMS_LIST undoList;

        for( BOARD_ITEM* member : group->GetItems() )
            undoList.PushItem( ITEM_PICKER( nullptr, member, UNDO_REDO::UNGROUP ) );

        group->RemoveAll();

        aFrame->SaveCopyInUndoList( undoList, UNDO_REDO::UNGROUP );
        undoFlags |= APPEND_UNDO;
    }

    aCommit->Remove( this );

    aTool->ClearRouterCommit();

    if( baselineValid() )
    {
        removeToBaseline( router, layer, *m_baseLine );

        if( m_tuningMode == DIFF_PAIR )
            removeToBaseline( router, layer, *m_baseLineCoupled );
    }

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

    aCommit->Push( "Remove Tuning Pattern", undoFlags );
}


PNS::ROUTER_MODE PCB_TUNING_PATTERN::toPNSMode()
{
    switch( m_tuningMode )
    {
    case LENGTH_TUNING_MODE::SINGLE:         return PNS::PNS_MODE_TUNE_SINGLE;
    case LENGTH_TUNING_MODE::DIFF_PAIR:      return PNS::PNS_MODE_TUNE_DIFF_PAIR;
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW;
    default:                                 return PNS::PNS_MODE_TUNE_SINGLE;
    }
}


bool PCB_TUNING_PATTERN::resetToBaseline( PNS::ROUTER* aRouter, int aLayer,
                                          PCB_BASE_EDIT_FRAME* aFrame, SHAPE_LINE_CHAIN& aBaseLine,
                                          bool aPrimary )
{
    PNS::NODE* world = aRouter->GetWorld();
    VECTOR2I   startSnapPoint, endSnapPoint;

    std::optional<PNS::LINE> pnsLine = getPNSLine( aBaseLine.CPoint( 0 ),
                                                   aBaseLine.CPoint( -1 ), aRouter, aLayer,
                                                   startSnapPoint, endSnapPoint );

    wxCHECK( pnsLine, false );

    SHAPE_LINE_CHAIN straightChain;
    {
        SHAPE_LINE_CHAIN pre, mid, post;
        pnsLine->CLine().Split( startSnapPoint, endSnapPoint, pre, mid, post );

        straightChain.Append( pre );
        straightChain.Append( aBaseLine );
        straightChain.Append( post );
        straightChain.Simplify();
    }

    for( PNS::LINKED_ITEM* pnsItem : pnsLine->Links() )
    {
        if( BOARD_ITEM* item = pnsItem->Parent() )
        {
            aFrame->GetCanvas()->GetView()->Hide( item, true, true );
            m_removedItems.insert( item );
        }
    }

    world->Remove( *pnsLine );

    PNS::LINE straightLine( *pnsLine, straightChain );

    world->Add( straightLine, false );

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

            m_baseLineCoupled = mid;
        }
    }

    return true;
}


bool PCB_TUNING_PATTERN::Update( GENERATOR_TOOL* aTool, BOARD* aBoard, PCB_BASE_EDIT_FRAME* aFrame,
                                 BOARD_COMMIT* aCommit )
{
    PNS::ROUTER*     router = aTool->Router();
    PNS_KICAD_IFACE* iface = aTool->GetInterface();
    int              layer = GetLayer();

    iface->SetStartLayer( layer );

    if( router->RoutingInProgress() )
    {
        router->StopRouting();
    }

    if( !baselineValid() )
    {
        initBaseLines( router, layer, aBoard );
    }
    else
    {
        if( resetToBaseline( router, layer, aFrame, *m_baseLine, true ) )
        {
            m_origin = m_baseLine->CPoint( 0 );
            m_end = m_baseLine->CPoint( -1 );
        }
        else
        {
            initBaseLines( router, layer, aBoard );
            return false;
        }

        if( m_tuningMode == DIFF_PAIR )
        {
            if( !resetToBaseline( router, layer, aFrame, *m_baseLineCoupled, false ) )
            {
                initBaseLines( router, layer, aBoard );
                return false;
            }
        }
    }

    // Snap points
    VECTOR2I startSnapPoint, endSnapPoint;

    PNS::LINKED_ITEM* startItem = pickSegment( router, m_origin, layer, startSnapPoint );
    PNS::LINKED_ITEM* endItem = pickSegment( router, m_end, layer, endSnapPoint );

    wxASSERT( startItem );
    wxASSERT( endItem );

    if( !startItem || !endItem )
        return false;

    router->SetMode( toPNSMode() );

    if( !router->StartRouting( startSnapPoint, startItem, layer ) )
        return false;

    PNS::MEANDER_PLACER_BASE* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

    m_settings.m_keepEndpoints = true; // Required for re-grouping
    placer->UpdateSettings( m_settings );
    router->Move( m_end, nullptr );

    m_trackWidth = router->Sizes().TrackWidth();
    m_diffPairGap = router->Sizes().DiffPairGap();
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

    m_tuningInfo.Printf( wxS( "%s (%s)" ),
                         aFrame->MessageTextFromValue( (double) placer->TuningResult() ),
                         statusMessage );

    return true;
}


void PCB_TUNING_PATTERN::EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                   PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit,
                                   const wxString& aCommitMsg, int aCommitFlags )
{
    ClearFlags( IN_EDIT );

    PNS::ROUTER*      router = aTool->Router();
    SHAPE_LINE_CHAIN  bounds = getRectShape();
    PICKED_ITEMS_LIST groupUndoList;
    int               epsilon = aBoard->GetDesignSettings().GetDRCEpsilon();

    if( router->RoutingInProgress() )
    {
        router->FixRoute( m_end, nullptr, true );
        router->StopRouting();

        std::set<BOARD_ITEM*> routerRemovedItems = aTool->GetRouterCommitRemovedItems();
        std::set<BOARD_ITEM*> routerAddedItems = aTool->GetRouterCommitAddedItems();

        for( BOARD_ITEM* item : m_removedItems )
        {
            aFrame->GetCanvas()->GetView()->Hide( item, false );
            aCommit->Remove( item );
        }

        m_removedItems.clear();

        for( BOARD_ITEM* item : routerRemovedItems )
        {
            aCommit->Remove( item );
        }

        for( BOARD_ITEM* item : routerAddedItems )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
            {
                if( bounds.PointInside( track->GetPosition(), epsilon )
                    && bounds.PointInside( track->GetEnd(), epsilon ) )
                {
                    AddItem( item );
                    groupUndoList.PushItem( ITEM_PICKER( nullptr, item, UNDO_REDO::REGROUP ) );
                }
            }

            aCommit->Add( item );
        }
    }

    if( aCommitMsg.IsEmpty() )
        aCommit->Push( _( "Edit Tuning Pattern" ), aCommitFlags );
    else
        aCommit->Push( aCommitMsg, aCommitFlags );

    aFrame->AppendCopyToUndoList( groupUndoList, UNDO_REDO::REGROUP );
}


void PCB_TUNING_PATTERN::EditRevert( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                     PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit )
{
    ClearFlags( IN_EDIT );

    for( BOARD_ITEM* item : m_removedItems )
        aFrame->GetCanvas()->GetView()->Hide( item, false );

    m_removedItems.clear();

    aTool->Router()->StopRouting();

    if( aCommit )
        aCommit->Revert();
}


bool PCB_TUNING_PATTERN::MakeEditPoints( std::shared_ptr<EDIT_POINTS> points ) const
{
    VECTOR2I centerlineOffset;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;

    points->AddPoint( m_origin + centerlineOffset );
    points->AddPoint( m_end + centerlineOffset );

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    int amplitude = m_settings.m_maxAmplitude + KiROUND( m_trackWidth / 2.0 );

    if( m_tuningMode == DIFF_PAIR )
        amplitude += KiROUND( m_diffPairGap * 1.5 ) + m_trackWidth;

    if( m_settings.m_initialSide == -1 )
        amplitude *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( amplitude );

    points->AddPoint( base.A + widthHandleOffset );
    points->Point( 2 ).SetGridConstraint( IGNORE_GRID );

    VECTOR2I spacingHandleOffset =
            widthHandleOffset + ( base.B - base.A ).Resize( KiROUND( m_settings.m_spacing * 1.5 ) );

    points->AddPoint( base.A + spacingHandleOffset );
    points->Point( 3 ).SetGridConstraint( IGNORE_GRID );

    return true;
}


bool PCB_TUNING_PATTERN::UpdateFromEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints,
                                               BOARD_COMMIT* aCommit )
{
    VECTOR2I centerlineOffset;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    m_origin = aEditPoints->Point( 0 ).GetPosition() - centerlineOffset;
    m_end = aEditPoints->Point( 1 ).GetPosition() - centerlineOffset;

    if( aEditPoints->Point( 2 ).IsActive() )
    {
        VECTOR2I wHandle = aEditPoints->Point( 2 ).GetPosition();

        int value = base.LineDistance( wHandle );

        value -= KiROUND( m_trackWidth / 2.0 );

        if( m_tuningMode == DIFF_PAIR )
            value -= KiROUND( m_diffPairGap * 1.5 ) + m_trackWidth;

        SetMaxAmplitude( KiROUND( value / pcbIUScale.mmToIU( 0.1 ) ) * pcbIUScale.mmToIU( 0.1 ) );

        int side = base.Side( wHandle );

        if( side < 0 )
            m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
        else
            m_settings.m_initialSide = PNS::MEANDER_SIDE_RIGHT;
    }

    if( aEditPoints->Point( 3 ).IsActive() )
    {
        VECTOR2I wHandle = aEditPoints->Point( 2 ).GetPosition();
        VECTOR2I sHandle = aEditPoints->Point( 3 ).GetPosition();

        int value = KiROUND( SEG( base.A, wHandle ).LineDistance( sHandle ) / 1.5 );

        SetSpacing( KiROUND( value / pcbIUScale.mmToIU( 0.01 ) ) * pcbIUScale.mmToIU( 0.01 ) );
    }

    return true;
}


bool PCB_TUNING_PATTERN::UpdateEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints )
{
    VECTOR2I centerlineOffset;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    int amplitude = m_settings.m_maxAmplitude + KiROUND( m_trackWidth / 2.0 );

    if( m_tuningMode == DIFF_PAIR )
        amplitude += KiROUND( m_diffPairGap * 1.5 ) + m_trackWidth;

    if( m_settings.m_initialSide == -1 )
        amplitude *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( amplitude );

    aEditPoints->Point( 0 ).SetPosition( m_origin + centerlineOffset );
    aEditPoints->Point( 1 ).SetPosition( m_end + centerlineOffset );

    aEditPoints->Point( 2 ).SetPosition( base.A + widthHandleOffset );

    VECTOR2I spacingHandleOffset =
            widthHandleOffset + ( base.B - base.A ).Resize( KiROUND( m_settings.m_spacing * 1.5 ) );

    aEditPoints->Point( 3 ).SetPosition( base.A + spacingHandleOffset );

    return true;
}


SHAPE_LINE_CHAIN PCB_TUNING_PATTERN::getRectShape() const
{
    SHAPE_LINE_CHAIN chain;

    if( m_baseLine )
    {
        SHAPE_LINE_CHAIN cl = *m_baseLine;

        if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
        {
            for( int i = 0; i < cl.PointCount() - 1 && i < m_baseLineCoupled->PointCount(); ++i )
                cl.SetPoint( i, ( cl.CPoint( i ) + m_baseLineCoupled->CPoint( i ) ) / 2 );

            cl.SetPoint( -1, ( cl.CPoint( -1 ) + m_baseLineCoupled->CPoint( -1 ) ) / 2 );
        }

        bool singleSided = m_settings.m_singleSided;
        int  amplitude = m_settings.m_maxAmplitude + KiROUND( m_trackWidth / 2.0 );

        if( m_tuningMode == DIFF_PAIR )
        {
            singleSided = false;
            amplitude += KiROUND( m_diffPairGap * 1.5 ) + m_trackWidth;
        }

        if( singleSided )
        {
            SHAPE_LINE_CHAIN left, right;

            if( cl.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF,
                               left, right, true ) )
            {
                chain.Append( cl.CPoint( 0 ) );
                chain.Append( m_settings.m_initialSide >= 0 ? right : left );
                chain.Append( cl.CPoint( -1 ) );

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

            poly.OffsetLineChain( cl, amplitude * 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                  ARC_LOW_DEF, false );

            if( poly.OutlineCount() > 0 )
            {
                chain = poly.Outline( 0 );
            }
        }
    }

    return chain;
}


void PCB_TUNING_PATTERN::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( !IsSelected() )
        return;

    KIGFX::PREVIEW::DRAW_CONTEXT ctx( *aView );

    int size = KiROUND( aView->ToWorld( EDIT_POINT::POINT_SIZE ) * 0.8 );
    size = std::max( size, pcbIUScale.mmToIU( 0.05 ) );

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

    SHAPE_LINE_CHAIN chain = getRectShape();

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

    props.set( "end", m_end );
    props.set( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    props.set( "single_sided", m_settings.m_singleSided );
    props.set( "rounded", m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND );

    props.set_iu( "max_amplitude", m_settings.m_maxAmplitude );
    props.set_iu( "min_spacing", m_settings.m_spacing );
    props.set_iu( "target_length_min", m_settings.m_targetLength.Min() );
    props.set_iu( "target_length", m_settings.m_targetLength.Opt() );
    props.set_iu( "target_length_max", m_settings.m_targetLength.Max() );
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

    aProps.get_to( "end", m_end );
    aProps.get_to( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    aProps.get_to( "single_sided", m_settings.m_singleSided );
    aProps.get_to( "side", m_settings.m_initialSide );

    bool rounded;
    aProps.get_to( "rounded", rounded );
    m_settings.m_cornerStyle = rounded ? PNS::MEANDER_STYLE_ROUND : PNS::MEANDER_STYLE_CHAMFER;

    long long int val;

    aProps.get_to_iu( "target_length", val );
    m_settings.SetTargetLength( val );

    if( aProps.get_to_iu( "target_length_min", val ) )
        m_settings.m_targetLength.SetMin( val );

    if( aProps.get_to_iu( "target_length_max", val ) )
        m_settings.m_targetLength.SetMax( val );

    int int_val;

    aProps.get_to_iu( "target_skew", int_val );
    m_settings.SetTargetSkew( int_val );

    if( aProps.get_to_iu( "target_skew_min", int_val ) )
        m_settings.m_targetSkew.SetMin( int_val );

    if( aProps.get_to_iu( "target_skew_max", int_val ) )
        m_settings.m_targetSkew.SetMax( int_val );

    aProps.get_to_iu( "max_amplitude", m_settings.m_maxAmplitude );
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
        BOARD_ITEM*                  startItem = *m_items.begin();
        std::shared_ptr<DRC_ENGINE>& drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, startItem, nullptr, GetLayer() );

        if( !constraint.IsNull() && !settings.m_overrideCustomRules )
            settings.SetTargetLength( constraint.GetValue() );
    }

    DIALOG_TUNING_PATTERN_PROPERTIES dlg( aEditFrame, settings, toPNSMode(), constraint );

    if( dlg.ShowModal() == wxID_OK )
    {
        BOARD_COMMIT commit( aEditFrame );
        commit.Modify( this );
        m_settings = settings;
        commit.Push( _( "Edit Tuning Pattern" ) );
    }

    aEditFrame->GetToolManager()->PostAction<PCB_GENERATOR*>( PCB_ACTIONS::regenerateItem, this );
}


void PCB_TUNING_PATTERN::UpdateStatus( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                       STATUS_MIN_MAX_POPUP* aPopup )
{
    auto* placer = dynamic_cast<PNS::MEANDER_PLACER_BASE*>( aTool->Router()->Placer() );

    if( !placer )
        return;

    if( m_unconstrained )
    {
        aPopup->ClearMinMax();
    }
    else if( m_tuningMode == DIFF_PAIR_SKEW )
    {
        aPopup->SetMinMax( m_settings.m_targetSkew.Min(), m_settings.m_targetSkew.Max() );
    }
    else
    {
        aPopup->SetMinMax( (double) m_settings.m_targetLength.Min(),
                           (double) m_settings.m_targetLength.Max() );
    }

    if( m_tuningMode == DIFF_PAIR_SKEW )
        aPopup->SetCurrent( (double) placer->TuningResult(), _( "current skew" ) );
    else
        aPopup->SetCurrent( (double) placer->TuningResult(), _( "current length" ) );
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

    aList.emplace_back( _( "Type" ), GetFriendlyName() );

    for( BOARD_ITEM* member : GetItems() )
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
        aList.emplace_back( _( "Resolved Netclass" ), UnescapeString( netclass->GetName() ) );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    if( width && !mixedWidth )
        aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( width ) );

    BOARD*                       board = GetBoard();
    std::shared_ptr<DRC_ENGINE>& drcEngine = board->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT               constraint;

    // Display full track length (in Pcbnew)
    if( board && primaryItem && primaryItem->GetNetCode() > 0 )
    {
        int    count;
        double trackLen;
        double lenPadToDie;

        std::tie( count, trackLen, lenPadToDie ) = board->GetTrackLength( *primaryItem );

        if( coupledItem && coupledItem->GetNetCode() > 0 )
        {
            double coupledLen;
            std::tie( count, coupledLen, lenPadToDie ) = board->GetTrackLength( *coupledItem );

            aList.emplace_back( _( "Routed Lengths" ), aFrame->MessageTextFromValue( trackLen )
                                                     + wxS( ", " )
                                                     + aFrame->MessageTextFromValue( coupledLen ) );
        }
        else
        {
            aList.emplace_back( _( "Routed Length" ), aFrame->MessageTextFromValue( trackLen ) );
        }

        if( lenPadToDie != 0 )
        {
            msg = aFrame->MessageTextFromValue( lenPadToDie );
            aList.emplace_back( _( "Pad To Die Length" ), msg );

            msg = aFrame->MessageTextFromValue( trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg );
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
            msg = aFrame->MessageTextFromValue( (double) m_settings.m_targetLength.Opt() );

            aList.emplace_back( wxString::Format( _( "Target Length: %s" ), msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = aFrame->MessageTextFromMinOptMax( constraint.GetValue() );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( _( "Length Constraints: %s" ), msg ),
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
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );
    Activate();

    LENGTH_TUNING_MODE       mode = fromPNSMode( aEvent.Parameter<PNS::ROUTER_MODE>() );
    KIGFX::VIEW_CONTROLS*    controls = getViewControls();
    BOARD*                   board = m_frame->GetBoard();
    PCB_SELECTION_TOOL*      selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
    GENERATOR_TOOL*          generatorTool = m_toolMgr->GetTool<GENERATOR_TOOL>();
    PNS::ROUTER*             router = generatorTool->Router();
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

    auto updateHoverStatus =
            [&]()
            {
                std::unique_ptr<PCB_TUNING_PATTERN> dummyPattern;

                if( m_pickerItem )
                {
                    dummyPattern.reset( PCB_TUNING_PATTERN::CreateNew( generatorTool, m_frame,
                                                                       m_pickerItem, mode, false ) );
                }

                if( dummyPattern )
                {
                    m_statusPopup->Popup();
                    canvas()->SetStatusPopup( m_statusPopup.get() );

                    dummyPattern->EditStart( generatorTool, m_board, m_frame, nullptr );
                    dummyPattern->Update( generatorTool, m_board, m_frame, nullptr );

                    dummyPattern->UpdateStatus( generatorTool, m_frame, m_statusPopup.get() );
                    m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
                }
                else
                {
                    m_statusPopup->Hide();
                }
            };

    auto updateTuningPattern =
            [&]()
            {
                if( m_tuningPattern && m_tuningPattern->GetPosition() != m_tuningPattern->GetEnd() )
                {
                    m_tuningPattern->EditStart( generatorTool, m_board, m_frame, nullptr );
                    m_tuningPattern->Update( generatorTool, m_board, m_frame, nullptr );

                    m_statusPopup->Popup();
                    canvas()->SetStatusPopup( m_statusPopup.get() );

                    m_view->Update( &m_preview );

                    m_tuningPattern->UpdateStatus( generatorTool, m_frame, m_statusPopup.get() );
                    m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
                }
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        VECTOR2D cursorPos = controls->GetMousePosition();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_tuningPattern )
            {
                // First click already made; clean up tuning pattern preview
                m_tuningPattern->EditRevert( generatorTool, m_board, m_frame, nullptr );

                m_preview.Clear();

                delete m_tuningPattern;
                m_tuningPattern = nullptr;
            }

            break;
        }
        else if( evt->IsMotion() )
        {
            if( !m_tuningPattern )
            {
                // First click not yet made; we're in highlight-net-under-cursor mode

                GENERAL_COLLECTOR collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
                collector.Collect( board, { PCB_TRACE_T, PCB_ARC_T }, cursorPos, guide );

                if( collector.GetCount() > 1 )
                    selectionTool->GuessSelectionCandidates( collector, cursorPos );

                BOARD_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

                if( !m_pickerItem )
                {
                    m_pickerItem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                    generatorTool->HighlightNets( m_pickerItem );
                }
                else
                {
                    m_pickerItem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                    generatorTool->UpdateHighlightedNets( m_pickerItem );
                }

                updateHoverStatus();
            }
            else
            {
                // First click already made; we're in preview-tuning-pattern mode

                m_tuningPattern->SetEnd( cursorPos );
                updateTuningPattern();
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_pickerItem && !m_tuningPattern )
            {
                // First click; create a tuning pattern

                generatorTool->HighlightNets( nullptr );

                m_frame->SetActiveLayer( m_pickerItem->GetLayer() );
                m_tuningPattern = PCB_TUNING_PATTERN::CreateNew( generatorTool, m_frame,
                                                                 m_pickerItem, mode );

                if( m_tuningPattern )
                {
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

                    m_preview.Add( m_tuningPattern );
                }
            }
            else if( m_pickerItem && m_tuningPattern )
            {
                // Second click; we're done
                BOARD_COMMIT commit( m_frame );

                m_tuningPattern->EditStart( generatorTool, m_board, m_frame, &commit );
                m_tuningPattern->Update( generatorTool, m_board, m_frame, &commit );
                m_tuningPattern->EditPush( generatorTool, m_board, m_frame, &commit, _( "Tune" ) );

                for( BOARD_ITEM* item : m_tuningPattern->GetItems() )
                    item->SetSelected();

                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            PCB_SELECTION dummy;
            m_menu.ShowContextMenu( dummy );
        }
        else if( evt->IsAction( &PCB_ACTIONS::spacingIncrease )
                 || evt->IsAction( &PCB_ACTIONS::spacingDecrease ) )
        {
            if( m_tuningPattern )
            {
                auto* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

                placer->SpacingStep( evt->IsAction( &PCB_ACTIONS::spacingIncrease ) ? 1 : -1 );
                m_tuningPattern->SetSpacing( placer->MeanderSettings().m_spacing );
                updateTuningPattern();
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

    canvas()->SetStatusPopup( nullptr );
    m_statusPopup->Hide();

    generatorTool->HighlightNets( nullptr );

    m_preview.Clear();
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
                                     _HKI( "Tuning mode" ),
                                     NO_SETTER( PCB_TUNING_PATTERN, LENGTH_TUNING_MODE ),
                                     &PCB_TUNING_PATTERN::GetTuningMode ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Min amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMinAmplitude,
                                     &PCB_TUNING_PATTERN::GetMinAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Max amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMaxAmplitude,
                                     &PCB_TUNING_PATTERN::GetMaxAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, PNS::MEANDER_SIDE>(
                                     _HKI( "Initial side" ),
                                     &PCB_TUNING_PATTERN::SetInitialSide,
                                     &PCB_TUNING_PATTERN::GetInitialSide ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Min spacing" ), &PCB_TUNING_PATTERN::SetSpacing,
                                     &PCB_TUNING_PATTERN::GetSpacing, PROPERTY_DISPLAY::PT_SIZE,
                                     ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Corner radius %" ),
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

        auto notIsSkew =
                [&]( INSPECTABLE* aItem ) -> bool
                {
                    return !isSkew( aItem );
                };

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, long long int>(
                                     _HKI( "Target length" ),
                                     &PCB_TUNING_PATTERN::SetTargetLength,
                                     &PCB_TUNING_PATTERN::GetTargetLength,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( notIsSkew );


        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>(
                                     _HKI( "Target skew" ), &PCB_TUNING_PATTERN::SetTargetSkew,
                                     &PCB_TUNING_PATTERN::GetTargetSkew,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( isSkew );


        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>(
                                     _HKI( "Override custom rules" ),
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
