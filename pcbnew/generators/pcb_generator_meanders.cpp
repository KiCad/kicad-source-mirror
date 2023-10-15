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
#include <router/pns_tune_status_popup.h>

#include <dialogs/dialog_meander_properties.h>


enum LENGTH_TUNING_MODE
{
    SINGLE,
    DIFF_PAIR,
    DIFF_PAIR_SKEW
};


class PCB_GENERATOR_MEANDERS : public PCB_GENERATOR
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_GENERATOR_MEANDERS( BOARD_ITEM* aParent = nullptr, PCB_LAYER_ID aLayer = F_Cu,
                            LENGTH_TUNING_MODE aMode = LENGTH_TUNING_MODE::SINGLE );

    wxString GetGeneratorType() const override { return wxS( "meanders" ); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    wxString GetFriendlyName() const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    static PCB_GENERATOR_MEANDERS* CreateNew( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                              BOARD_CONNECTED_ITEM* aStartItem,
                                              LENGTH_TUNING_MODE aMode );

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
    }

    const BOX2I GetBoundingBox() const override { return getRectShape().BBox(); }

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

    EDA_ITEM* Clone() const override { return new PCB_GENERATOR_MEANDERS( *this ); }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

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

    long long int GetTargetLength() const { return m_targetLength; }
    void          SetTargetLength( long long int aValue ) { m_targetLength = aValue; }

    int  GetTargetSkew() const { return m_targetSkew; }
    void SetTargetSkew( int aValue ) { m_targetSkew = aValue; }

    bool GetOverrideCustomRules() const { return m_overrideCustomRules; }
    void SetOverrideCustomRules( bool aOverride ) { m_overrideCustomRules = aOverride; }

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

    const STRING_ANY_MAP GetProperties() const override;
    void SetProperties( const STRING_ANY_MAP& aProps ) override;

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override;

    void UpdateStatus( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                       STATUS_TEXT_POPUP* aPopup ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    void swapData( BOARD_ITEM* aImage ) override
    {
        wxASSERT( aImage->Type() == PCB_GENERATOR_T );

        std::swap( *this, *static_cast<PCB_GENERATOR_MEANDERS*>( aImage ) );
    }

    PNS::MEANDER_SETTINGS toMeanderSettings();
    void                  fromMeanderSettings( const PNS::MEANDER_SETTINGS& aSettings );

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
    VECTOR2I           m_end;

    int                m_minAmplitude;
    int                m_maxAmplitude;
    int                m_spacing;
    long long int      m_targetLength;
    int                m_targetSkew;
    bool               m_overrideCustomRules;
    int                m_cornerRadiusPercentage;

    PNS::MEANDER_SIDE  m_initialSide;

    std::optional<SHAPE_LINE_CHAIN> m_baseLine;
    std::optional<SHAPE_LINE_CHAIN> m_baseLineCoupled;

    bool               m_singleSide;
    bool               m_rounded;

    LENGTH_TUNING_MODE m_tuningMode;

    wxString           m_lastNetName;
    wxString           m_tuningInfo;

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
        wxFAIL_MSG( wxS( "Unknown meander side token" ) );
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


static NETINFO_ITEM* getCoupledNet( PNS::ROUTER* aRouter, NETINFO_ITEM* aNet )
{
    PNS::RULE_RESOLVER* resolver = aRouter->GetRuleResolver();

    return static_cast<NETINFO_ITEM*>( resolver->DpCoupledNet( aNet ) );
}


PCB_GENERATOR_MEANDERS::PCB_GENERATOR_MEANDERS( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer,
                                                LENGTH_TUNING_MODE aMode ) :
        PCB_GENERATOR( aParent, aLayer ),
        m_singleSide( false ),
        m_rounded( true ),
        m_tuningMode( aMode ),
        m_tuningStatus( PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED )
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;

    m_minAmplitude = pcbIUScale.mmToIU( 0.1 );
    m_maxAmplitude = pcbIUScale.mmToIU( 2.0 );
    m_spacing = pcbIUScale.mmToIU( 0.6 );
    m_targetLength = pcbIUScale.mmToIU( 100 );
    m_targetSkew = pcbIUScale.mmToIU( 0 );
    m_overrideCustomRules = false;
    m_end = VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 );
    m_cornerRadiusPercentage = 100;
    m_initialSide = PNS::MEANDER_SIDE_DEFAULT;
}


static NETINFO_ITEM* snapToNearestTrackPoint( VECTOR2I& aP, BOARD* aBoard, NETINFO_ITEM* aNet )
{
    SEG::ecoord   minDistSq = VECTOR2I::ECOORD_MAX;
    VECTOR2I      closestPt = aP;
    NETINFO_ITEM* closestNet = nullptr;

    for( PCB_TRACK *track : aBoard->Tracks() )
    {
        if( aNet && track->GetNet() != aNet )
            continue;

        SEG seg( track->GetStart(), track->GetEnd() );

        VECTOR2I    nearest = seg.NearestPoint( aP );
        SEG::ecoord distSq = ( nearest - aP ).SquaredEuclideanNorm();

        if( distSq < minDistSq )
        {
            minDistSq = distSq;
            closestPt = nearest;
            closestNet = track->GetNet();
        }
    }

    if( minDistSq != VECTOR2I::ECOORD_MAX )
    {
        aP = closestPt;
        return closestNet;
    }

    return nullptr;
}


bool PCB_GENERATOR_MEANDERS::baselineValid()
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


PCB_GENERATOR_MEANDERS* PCB_GENERATOR_MEANDERS::CreateNew( GENERATOR_TOOL* aTool,
                                                           PCB_BASE_EDIT_FRAME* aFrame,
                                                           BOARD_CONNECTED_ITEM* aStartItem,
                                                           LENGTH_TUNING_MODE aMode )
{
    BOARD*                       board = aStartItem->GetBoard();
    std::shared_ptr<DRC_ENGINE>& drcEngine = board->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT               constraint;
    PCB_LAYER_ID                 layer = aStartItem->GetLayer();

    if( aMode == SINGLE && getCoupledNet( aTool->Router(), aStartItem->GetNet() ) )
        aMode = DIFF_PAIR;

    PCB_GENERATOR_MEANDERS* meander = new PCB_GENERATOR_MEANDERS( board, layer, aMode );

    constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, aStartItem, nullptr, layer );

    if( aMode == DIFF_PAIR_SKEW )
    {
        if( constraint.IsNull() )
        {
            WX_UNIT_ENTRY_DIALOG dlg( aFrame, _( "Tune Skew" ), _( "Target skew:" ), 0 );

            if( dlg.ShowModal() != wxID_OK )
                return nullptr;

            meander->m_targetSkew = dlg.GetValue();
            meander->m_overrideCustomRules = true;
        }
        else
        {
            meander->m_targetSkew = constraint.GetValue().Opt();
            meander->m_overrideCustomRules = false;
        }
    }
    else
    {
        if( constraint.IsNull() )
        {
            WX_UNIT_ENTRY_DIALOG dlg( aFrame, _( "Tune Length" ), _( "Target length:" ),
                                      100 * PCB_IU_PER_MM );

            if( dlg.ShowModal() != wxID_OK )
                return nullptr;

            meander->m_targetLength = dlg.GetValue();
            meander->m_overrideCustomRules = true;
        }
        else
        {
            meander->m_targetLength = constraint.GetValue().Opt();
            meander->m_overrideCustomRules = false;
        }
    }

    meander->SetFlags( IS_NEW );

    return meander;
}

void PCB_GENERATOR_MEANDERS::EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard,
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

    int          layer = GetLayer();
    PNS::ROUTER* router = aTool->Router();

    aTool->ClearRouterCommit();
    router->SyncWorld();

    if( !baselineValid() )
        initBaseLines( router, layer, aBoard );

    if( baselineValid() && !m_overrideCustomRules )
    {
        PNS::CONSTRAINT     constraint;
        PNS::RULE_RESOLVER* resolver = router->GetRuleResolver();

        NETINFO_ITEM* net = snapToNearestTrackPoint( m_origin, aBoard, nullptr );
        PNS::SEGMENT  pnsItem( m_baseLine->CSegment( 0 ), net );

        if( m_tuningMode == SINGLE )
        {
            if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                           &pnsItem, nullptr, layer, &constraint ) )
            {
                m_targetLength = constraint.m_Value.Opt();
                aFrame->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
            }
        }
        else
        {
            NETINFO_ITEM* coupledNet = static_cast<NETINFO_ITEM*>( resolver->DpCoupledNet( net ) );
            PNS::SEGMENT  coupledItem( m_baseLineCoupled->CSegment( 0 ), coupledNet );

            if( m_tuningMode == DIFF_PAIR )
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_LENGTH,
                                               &pnsItem, &coupledItem, layer, &constraint ) )
                {
                    m_targetLength = constraint.m_Value.Opt();
                    aFrame->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
                }
            }
            else
            {
                if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_SKEW,
                                               &pnsItem, &coupledItem, layer, &constraint ) )
                {
                    m_targetSkew = constraint.m_Value.Opt();
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


bool PCB_GENERATOR_MEANDERS::initBaseLine( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard,
                                           VECTOR2I& aStart, VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                                           std::optional<SHAPE_LINE_CHAIN>& aBaseLine )
{
    PNS::NODE* world = aRouter->GetWorld();

    snapToNearestTrackPoint( aStart, aBoard, aNet );
    snapToNearestTrackPoint( aEnd, aBoard, aNet );

    VECTOR2I startSnapPoint, endSnapPoint;

    PNS::LINKED_ITEM* startItem = pickSegment( aRouter, aStart, aLayer, startSnapPoint );
    PNS::LINKED_ITEM* endItem = pickSegment( aRouter, aEnd, aLayer, endSnapPoint );

    wxASSERT( startItem );
    wxASSERT( endItem );

    if( !startItem || !endItem || startSnapPoint == endSnapPoint )
        return false;

    PNS::LINE               line = world->AssembleLine( startItem, nullptr, false, true );
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


bool PCB_GENERATOR_MEANDERS::initBaseLines( PNS::ROUTER* aRouter, int aLayer, BOARD* aBoard )
{
    m_baseLineCoupled.reset();

    NETINFO_ITEM* net = snapToNearestTrackPoint( m_origin, aBoard, nullptr );

    if( !initBaseLine( aRouter, aLayer, aBoard, m_origin, m_end, net, m_baseLine ) )
        return false;

    // Generate both baselines even if we're skewing.  We need the coupled baseline to run the
    // DRC rules against.
    if( m_tuningMode == DIFF_PAIR || m_tuningMode == DIFF_PAIR_SKEW )
    {
        if( NETINFO_ITEM* coupledNet = getCoupledNet( aRouter, net ) )
        {
            VECTOR2I coupledStart = m_origin;
            VECTOR2I coupledEnd = m_end;

            return initBaseLine( aRouter, aLayer, aBoard, coupledStart, coupledEnd, coupledNet,
                                 m_baseLineCoupled );
        }

        return false;
    }

    return true;
}

void PCB_GENERATOR_MEANDERS::removeToBaseline( PNS::ROUTER* aRouter, int aLayer,
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


void PCB_GENERATOR_MEANDERS::Remove( GENERATOR_TOOL* aTool, BOARD* aBoard,
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

    aCommit->Push( "Remove Meander", undoFlags );
}


PNS::MEANDER_SETTINGS PCB_GENERATOR_MEANDERS::toMeanderSettings()
{
    PNS::MEANDER_SETTINGS settings;

    settings.m_cornerStyle = m_rounded ? PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND
                                       : PNS::MEANDER_STYLE::MEANDER_STYLE_CHAMFER;

    settings.m_minAmplitude = m_minAmplitude;
    settings.m_maxAmplitude = m_maxAmplitude;
    settings.m_spacing = m_spacing;
    settings.m_targetLength = m_targetLength;
    settings.m_targetSkew = m_targetSkew;
    settings.m_overrideCustomRules = m_overrideCustomRules;
    settings.m_singleSided = m_singleSide;
    settings.m_segmentSide = m_initialSide;
    settings.m_cornerRadiusPercentage = m_cornerRadiusPercentage;

    return settings;
}


void PCB_GENERATOR_MEANDERS::fromMeanderSettings( const PNS::MEANDER_SETTINGS& aSettings )
{
    m_rounded = aSettings.m_cornerStyle == PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND;
    m_minAmplitude = aSettings.m_minAmplitude;
    m_maxAmplitude = aSettings.m_maxAmplitude;
    m_spacing = aSettings.m_spacing;
    m_targetLength = aSettings.m_targetLength;
    m_targetSkew = aSettings.m_targetSkew;
    m_overrideCustomRules = aSettings.m_overrideCustomRules;
    m_singleSide = aSettings.m_singleSided;
    m_initialSide = aSettings.m_segmentSide;
    m_cornerRadiusPercentage = aSettings.m_cornerRadiusPercentage;
}


PNS::ROUTER_MODE PCB_GENERATOR_MEANDERS::toPNSMode()
{
    switch( m_tuningMode )
    {
    case LENGTH_TUNING_MODE::SINGLE:         return PNS::PNS_MODE_TUNE_SINGLE;
    case LENGTH_TUNING_MODE::DIFF_PAIR:      return PNS::PNS_MODE_TUNE_DIFF_PAIR;
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW;
    default:                                 return PNS::PNS_MODE_TUNE_SINGLE;
    }
}


bool PCB_GENERATOR_MEANDERS::resetToBaseline( PNS::ROUTER* aRouter, int aLayer,
                                              PCB_BASE_EDIT_FRAME* aFrame,
                                              SHAPE_LINE_CHAIN& aBaseLine, bool aPrimary )
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


bool PCB_GENERATOR_MEANDERS::Update( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                     PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit )
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

    auto placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

    PNS::MEANDER_SETTINGS settings = toMeanderSettings();

    placer->UpdateSettings( settings );
    router->Move( m_end, nullptr );

    m_lastNetName = iface->GetNetName( startItem->Net() );
    m_tuningInfo = placer->TuningInfo( aFrame->GetUserUnits() );
    m_tuningStatus = placer->TuningStatus();

    return true;
}


void PCB_GENERATOR_MEANDERS::EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                       PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit,
                                       const wxString& aCommitMsg, int aCommitFlags )
{
    PNS::ROUTER* router = aTool->Router();

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
            item->SetSelected();
            AddItem( item );
            aCommit->Add( item );
        }
    }

    // Store isNew as BOARD_COMMIT::Push() is going to clear it.
    bool isNew = IsNew();

    if( aCommitMsg.IsEmpty() )
        aCommit->Push( _( "Edit Meander" ), aCommitFlags );
    else
        aCommit->Push( aCommitMsg, aCommitFlags );

    if( isNew && !GetItems().empty() )
    {
        PICKED_ITEMS_LIST undoList;

        for( BOARD_ITEM* member : GetItems() )
            undoList.PushItem( ITEM_PICKER( nullptr, member, UNDO_REDO::REGROUP ) );

        aFrame->AppendCopyToUndoList( undoList, UNDO_REDO::REGROUP );
    }
}


void PCB_GENERATOR_MEANDERS::EditRevert( GENERATOR_TOOL* aTool, BOARD* aBoard,
                                         PCB_BASE_EDIT_FRAME* aFrame, BOARD_COMMIT* aCommit )
{
    for( BOARD_ITEM* item : m_removedItems )
        aFrame->GetCanvas()->GetView()->Hide( item, false );

    m_removedItems.clear();

    aTool->Router()->StopRouting();

    if( aCommit )
        aCommit->Revert();
}


bool PCB_GENERATOR_MEANDERS::MakeEditPoints( std::shared_ptr<EDIT_POINTS> points ) const
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

    int offset = m_maxAmplitude;

    if( m_initialSide == -1 )
        offset *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( offset );

    points->AddPoint( base.A + widthHandleOffset );
    points->Point( 2 ).SetGridConstraint( IGNORE_GRID );

    VECTOR2I spacingHandleOffset =
            widthHandleOffset + ( base.B - base.A ).Resize( KiROUND( m_spacing * 1.5 ) );

    points->AddPoint( base.A + spacingHandleOffset );
    points->Point( 3 ).SetGridConstraint( IGNORE_GRID );

    return true;
}


bool PCB_GENERATOR_MEANDERS::UpdateFromEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints,
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

        int value = KiROUND( SEG( base.A, wHandle ).LineDistance( sHandle ) / 1.5 );

        SetSpacing( KiROUND( value / pcbIUScale.mmToIU( 0.01 ) ) * pcbIUScale.mmToIU( 0.01 ) );
    }

    return true;
}


bool PCB_GENERATOR_MEANDERS::UpdateEditPoints( std::shared_ptr<EDIT_POINTS> aEditPoints )
{
    VECTOR2I centerlineOffset;

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled && m_baseLineCoupled->SegmentCount() > 0 )
        centerlineOffset = ( m_baseLineCoupled->CPoint( 0 ) - m_origin ) / 2;

    SEG base = m_baseLine && m_baseLine->SegmentCount() > 0 ? m_baseLine->CSegment( 0 )
                                                            : SEG( m_origin, m_end );

    base.A += centerlineOffset;
    base.B += centerlineOffset;

    int offset = m_maxAmplitude;

    if( m_initialSide == -1 )
        offset *= -1;

    VECTOR2I widthHandleOffset = ( base.B - base.A ).Perpendicular().Resize( offset );

    aEditPoints->Point( 0 ).SetPosition( m_origin + centerlineOffset );
    aEditPoints->Point( 1 ).SetPosition( m_end + centerlineOffset );

    aEditPoints->Point( 2 ).SetPosition( base.A + widthHandleOffset );

    VECTOR2I spacingHandleOffset =  widthHandleOffset
                                        + ( base.B - base.A ).Resize( KiROUND( m_spacing * 1.5 ) );

    aEditPoints->Point( 3 ).SetPosition( base.A + spacingHandleOffset );

    return true;
}


SHAPE_LINE_CHAIN PCB_GENERATOR_MEANDERS::getRectShape() const
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

        bool singleSided = m_tuningMode != DIFF_PAIR && m_singleSide;

        if( singleSided )
        {
            SHAPE_LINE_CHAIN left, right;

            if( cl.OffsetLine( m_maxAmplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF,
                               left, right, true ) )
            {
                chain.Append( cl.CPoint( 0 ) );
                chain.Append( m_initialSide >= 0 ? right : left );
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

            poly.OffsetLineChain( cl, m_maxAmplitude * 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                  ARC_LOW_DEF, false );

            if( poly.OutlineCount() > 0 )
            {
                chain = poly.Outline( 0 );
            }
        }
    }

    return chain;
}


void PCB_GENERATOR_MEANDERS::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( !IsSelected() )
        return;

    KIGFX::PREVIEW::DRAW_CONTEXT ctx( *aView );
    int size = KiROUND( aView->ToWorld( EDIT_POINT::POINT_SIZE ) * 0.8 );

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

    if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
    {
        for( int i = 0; i < m_baseLineCoupled->SegmentCount(); i++ )
        {
            SEG seg = m_baseLineCoupled->CSegment( i );
            ctx.DrawLine( seg.A, seg.B, false );
        }
    }

    SHAPE_LINE_CHAIN chain = getRectShape();

    for( int i = 0; i < chain.SegmentCount(); i++ )
    {
        SEG seg = chain.Segment( i );
        ctx.DrawLineDashed( seg.A, seg.B, size, size / 2, false );
    }
}


const STRING_ANY_MAP PCB_GENERATOR_MEANDERS::GetProperties() const
{
    STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

    props.set( "tuning_mode", tuningToString( m_tuningMode ) );
    props.set( "initial_side", sideToString( m_initialSide ) );
    props.set( "last_status", statusToString( m_tuningStatus ) );

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
    props.set( "override_custom_rules", m_overrideCustomRules );

    if( m_baseLine )
        props.set( "base_line", wxAny( *m_baseLine ) );

    if( m_baseLineCoupled )
        props.set( "base_line_coupled", wxAny( *m_baseLineCoupled ) );

    return props;
}


void PCB_GENERATOR_MEANDERS::SetProperties( const STRING_ANY_MAP& aProps )
{
    PCB_GENERATOR::SetProperties( aProps );

    wxString tuningMode;
    aProps.get_to( "tuning_mode", tuningMode );
    m_tuningMode = tuningFromString( tuningMode.utf8_string() );

    wxString side;
    aProps.get_to( "initial_side", side );
    m_initialSide = sideFromString( side.utf8_string() );

    wxString status;
    aProps.get_to( "last_status", status );
    m_tuningStatus = statusFromString( status.utf8_string() );

    aProps.get_to( "end", m_end );
    aProps.get_to( "corner_radius_percent", m_cornerRadiusPercentage );
    aProps.get_to( "single_sided", m_singleSide );
    aProps.get_to( "side", m_initialSide );
    aProps.get_to( "rounded", m_rounded );

    aProps.get_to_iu( "max_amplitude", m_maxAmplitude );
    aProps.get_to_iu( "min_spacing", m_spacing );
    aProps.get_to_iu( "target_length", m_targetLength );
    aProps.get_to_iu( "target_skew", m_targetSkew );
    aProps.get_to( "override_custom_rules", m_overrideCustomRules );

    aProps.get_to( "last_netname", m_lastNetName );
    aProps.get_to( "last_tuning", m_tuningInfo );

    if( auto baseLine = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line" ) )
        m_baseLine = *baseLine;

    if( auto baseLineCoupled = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line_coupled" ) )
        m_baseLineCoupled = *baseLineCoupled;
}


void PCB_GENERATOR_MEANDERS::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame )
{
    PNS::MEANDER_SETTINGS settings = toMeanderSettings();
    DRC_CONSTRAINT        constraint;

    if( !m_items.empty() )
    {
        BOARD_ITEM*                  startItem = *m_items.begin();
        std::shared_ptr<DRC_ENGINE>& drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, startItem, nullptr, GetLayer() );

        if( !constraint.IsNull() && !settings.m_overrideCustomRules )
            settings.m_targetLength = constraint.GetValue().Opt();
    }

    DIALOG_MEANDER_PROPERTIES dlg( aEditFrame, settings, toPNSMode(), constraint );

    if( dlg.ShowModal() == wxID_OK )
    {
        BOARD_COMMIT commit( aEditFrame );
        commit.Modify( this );

        fromMeanderSettings( settings );

        commit.Push( _( "Edit Meander Properties" ) );
    }

    aEditFrame->GetToolManager()->PostAction<PCB_GENERATOR*>( PCB_ACTIONS::regenerateItem, this );
}


void PCB_GENERATOR_MEANDERS::UpdateStatus( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                           STATUS_TEXT_POPUP* aPopup )
{
    auto* placer = dynamic_cast<PNS::MEANDER_PLACER_BASE*>( aTool->Router()->Placer() );

    if( !placer )
        return;

    aPopup->SetText( placer->TuningInfo( aFrame->GetUserUnits() ) );

    // Determine the background color first and choose a contrasting value
    COLOR4D bg( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
    double h, s, l;
    bg.ToHSL( h, s, l );

    switch( placer->TuningStatus() )
    {
    case PNS::MEANDER_PLACER_BASE::TUNED:
        if( l < 0.5 )
            aPopup->SetTextColor( wxColor( 127, 200, 127 ) );
        else
            aPopup->SetTextColor( wxColor( 0, 92, 0 ) );

        break;

    case PNS::MEANDER_PLACER_BASE::TOO_SHORT:
        if( l < 0.5 )
            aPopup->SetTextColor( wxColor( 242, 100, 126 ) );
        else
            aPopup->SetTextColor( wxColor( 122, 0, 0 ) );

        break;

    case PNS::MEANDER_PLACER_BASE::TOO_LONG:
        if( l < 0.5 )
            aPopup->SetTextColor( wxColor( 66, 184, 235 ) );
        else
            aPopup->SetTextColor( wxColor( 19, 19, 195 ) );

        break;
    }
}


void PCB_GENERATOR_MEANDERS::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
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

    auto getMinOptMax =
            [&]( const MINOPTMAX<int>& v )
            {
                wxString msg;

                if( v.HasMin() )
                {
                    msg += wxString::Format( _( "min %s" ), aFrame->MessageTextFromValue( v.Min() ) );
                }

                if( v.HasOpt() )
                {
                    if( !msg.IsEmpty() )
                        msg += wxS( "; " );

                    msg += wxString::Format( _( "opt %s" ), aFrame->MessageTextFromValue( v.Opt() ) );
                }

                if( v.HasMax() )
                {
                    if( !msg.IsEmpty() )
                        msg += wxS( "; " );

                    msg += wxString::Format( _( "max %s" ), aFrame->MessageTextFromValue( v.Max() ) );
                }

                return msg;
            };

    if( m_tuningMode == DIFF_PAIR_SKEW )
    {
        constraint = drcEngine->EvalRules( SKEW_CONSTRAINT, primaryItem, coupledItem, m_layer );

        if( constraint.IsNull() || m_overrideCustomRules )
        {
            msg = aFrame->MessageTextFromValue( m_targetSkew );

            aList.emplace_back( wxString::Format( _( "Target Skew: %s" ), msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = getMinOptMax( constraint.GetValue() );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( _( "Skew Constraints: %s." ), msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
    else
    {
        constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, primaryItem, coupledItem, m_layer );

        if( constraint.IsNull() || m_overrideCustomRules )
        {
            msg = aFrame->MessageTextFromValue( (double) m_targetLength );

            aList.emplace_back( wxString::Format( _( "Target Length: %s" ), msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = getMinOptMax( constraint.GetValue() );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( _( "Length Constraints: %s." ), msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
}


const wxString PCB_GENERATOR_MEANDERS::DISPLAY_NAME = _HKI( "Tuning Pattern" );
const wxString PCB_GENERATOR_MEANDERS::GENERATOR_TYPE = wxS( "meanders" );


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


#define HITTEST_THRESHOLD_PIXELS 5


int DRAWING_TOOL::PlaceMeander( const TOOL_EVENT& aEvent )
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
    SCOPED_DRAW_MODE         scopedDrawMode( m_mode, MODE::MEANDER );

    m_pickerItem = nullptr;
    m_meander = nullptr;

    // Add a VIEW_GROUP that serves as a preview for the new item
    m_preview.Clear();
    m_view->Add( &m_preview );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
                controls->ShowCursor( true );
            };

    auto updateMeander =
            [&]()
            {
                if( m_meander && m_meander->GetPosition() != m_meander->GetEnd() )
                {
                    m_meander->EditStart( generatorTool, m_board, m_frame, nullptr );
                    m_meander->Update( generatorTool, m_board, m_frame, nullptr );

                    m_statusPopup->Popup();
                    canvas()->SetStatusPopup( m_statusPopup.get() );

                    m_view->Update( &m_preview );

                    m_meander->UpdateStatus( generatorTool, m_frame, m_statusPopup.get() );
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
            if( m_meander )
            {
                // First click already made; clean up meander preview
                m_meander->EditRevert( generatorTool, m_board, m_frame, nullptr );

                m_preview.Clear();

                delete m_meander;
                m_meander = nullptr;
            }

            break;
        }
        else if( evt->IsMotion() )
        {
            if( !m_meander )
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
            }
            else
            {
                // First click already made; we're in preview-meander mode

                m_meander->SetEnd( cursorPos );
                updateMeander();
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_pickerItem && !m_meander )
            {
                // First click; create a meander

                generatorTool->HighlightNets( nullptr );

                m_frame->SetActiveLayer( m_pickerItem->GetLayer() );
                m_meander = PCB_GENERATOR_MEANDERS::CreateNew( generatorTool, m_frame,
                                                               m_pickerItem, mode );

                int      dummyDist;
                int      dummyClearance = std::numeric_limits<int>::max() / 2;
                VECTOR2I closestPt;

                m_pickerItem->GetEffectiveShape()->Collide( cursorPos, dummyClearance,
                                                            &dummyDist, &closestPt );
                m_meander->SetPosition( closestPt );
                m_meander->SetEnd( closestPt );

                m_preview.Add( m_meander );
            }
            else if( m_pickerItem && m_meander )
            {
                // Second click; we're done
                BOARD_COMMIT commit( m_frame );

                m_meander->EditStart( generatorTool, m_board, m_frame, &commit );
                m_meander->Update( generatorTool, m_board, m_frame, &commit );
                m_meander->EditPush( generatorTool, m_board, m_frame, &commit, _( "Tune" ) );

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
            if( m_meander )
            {
                auto* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

                placer->SpacingStep( evt->IsAction( &PCB_ACTIONS::spacingIncrease ) ? 1 : -1 );
                m_meander->SetSpacing( placer->MeanderSettings().m_spacing );
                updateMeander();
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::amplIncrease )
                 || evt->IsAction( &PCB_ACTIONS::amplDecrease ) )
        {
            if( m_meander )
            {
                auto* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( router->Placer() );

                placer->AmplitudeStep( evt->IsAction( &PCB_ACTIONS::amplIncrease ) ? 1 : -1 );
                m_meander->SetMaxAmplitude( placer->MeanderSettings().m_maxAmplitude );
                updateMeander();
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

        controls->CaptureCursor( m_meander != nullptr );
        controls->SetAutoPan( m_meander != nullptr );
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

    if( m_meander )
        selectionTool->AddItemToSel( m_meander );

    m_frame->PopTool( aEvent );
    return 0;
}


static struct PCB_GENERATOR_MEANDERS_DESC
{
    PCB_GENERATOR_MEANDERS_DESC()
    {
        ENUM_MAP<LENGTH_TUNING_MODE>::Instance()
                .Map( LENGTH_TUNING_MODE::SINGLE, _HKI( "Single track" ) )
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR, _HKI( "Diff. pair" ) )
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

        const wxString groupTab = _HKI( "Pattern Properties" );

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

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR_MEANDERS, long long int>(
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
                                     _HKI( "Override custom rules" ),
                                     &PCB_GENERATOR_MEANDERS::SetOverrideCustomRules,
                                     &PCB_GENERATOR_MEANDERS::GetOverrideCustomRules ),
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