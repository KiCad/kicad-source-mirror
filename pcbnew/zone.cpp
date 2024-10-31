/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <bitmaps.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_null.h>
#include <pcb_edit_frame.h>
#include <pcb_screen.h>
#include <board.h>
#include <board_design_settings.h>
#include <lset.h>
#include <pad.h>
#include <zone.h>
#include <footprint.h>
#include <string_utils.h>
#include <math_for_graphics.h>
#include <properties/property_validators.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <i18n_utility.h>
#include <mutex>


ZONE::ZONE( BOARD_ITEM_CONTAINER* aParent ) :
        BOARD_CONNECTED_ITEM( aParent, PCB_ZONE_T ),
        m_Poly( nullptr ),
        m_cornerRadius( 0 ),
        m_priority( 0 ),
        m_isRuleArea( false ),
        m_ruleAreaPlacementEnabled( false ),
        m_ruleAreaPlacementSourceType( RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME ),
        m_teardropType( TEARDROP_TYPE::TD_NONE ),
        m_PadConnection( ZONE_CONNECTION::NONE ),
        m_ZoneClearance( 0 ),
        m_ZoneMinThickness( 0 ),
        m_islandRemovalMode( ISLAND_REMOVAL_MODE::ALWAYS ),
        m_isFilled( false ),
        m_thermalReliefGap( 0 ),
        m_thermalReliefSpokeWidth( 0 ),
        m_fillMode( ZONE_FILL_MODE::POLYGONS ),
        m_hatchThickness( 0 ),
        m_hatchGap( 0 ),
        m_hatchOrientation( ANGLE_0 ),
        m_hatchSmoothingLevel( 0 ),
        m_hatchHoleMinArea( 0 ),
        m_CornerSelection( nullptr ),
        m_area( 0.0 ),
        m_outlinearea( 0.0 )
{
    m_Poly = new SHAPE_POLY_SET();    // Outlines
    SetLocalFlags( 0 );               // flags temporary used in zone calculations
    m_fillVersion = 5;                // set the "old" way to build filled polygon areas (< 6.0.x)

    if( GetParentFootprint() )
        SetIsRuleArea( true );        // Zones living in footprints have the rule area option

    if( aParent->GetBoard() )
        aParent->GetBoard()->GetDesignSettings().GetDefaultZoneSettings().ExportSetting( *this );
    else
        ZONE_SETTINGS().ExportSetting( *this );

    m_needRefill = false;   // True only after edits.
}


ZONE::ZONE( const ZONE& aZone ) :
        BOARD_CONNECTED_ITEM( aZone ),
        m_Poly( nullptr ),
        m_CornerSelection( nullptr )
{
    InitDataFromSrcInCopyCtor( aZone );
}


ZONE& ZONE::operator=( const ZONE& aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    InitDataFromSrcInCopyCtor( aOther );

    return *this;
}


ZONE::~ZONE()
{
    delete m_Poly;
    delete m_CornerSelection;

    if( BOARD* board = GetBoard() )
        board->IncrementTimeStamp();
}


void ZONE::InitDataFromSrcInCopyCtor( const ZONE& aZone )
{
    // members are expected non initialize in this.
    // InitDataFromSrcInCopyCtor() is expected to be called only from a copy constructor.

    // Copy only useful EDA_ITEM flags:
    m_flags                   = aZone.m_flags;
    m_forceVisible            = aZone.m_forceVisible;

    // Replace the outlines for aZone outlines.
    delete m_Poly;
    m_Poly = new SHAPE_POLY_SET( *aZone.m_Poly );

    m_cornerSmoothingType     = aZone.m_cornerSmoothingType;
    m_cornerRadius            = aZone.m_cornerRadius;
    m_zoneName                = aZone.m_zoneName;
    m_priority                = aZone.m_priority;
    m_isRuleArea              = aZone.m_isRuleArea;
    m_ruleAreaPlacementEnabled = aZone.m_ruleAreaPlacementEnabled;
    m_ruleAreaPlacementSourceType = aZone.m_ruleAreaPlacementSourceType;
    m_ruleAreaPlacementSource = aZone.m_ruleAreaPlacementSource;
    SetLayerSet( aZone.GetLayerSet() );

    m_doNotAllowCopperPour    = aZone.m_doNotAllowCopperPour;
    m_doNotAllowVias          = aZone.m_doNotAllowVias;
    m_doNotAllowTracks        = aZone.m_doNotAllowTracks;
    m_doNotAllowPads          = aZone.m_doNotAllowPads;
    m_doNotAllowFootprints    = aZone.m_doNotAllowFootprints;

    m_PadConnection           = aZone.m_PadConnection;
    m_ZoneClearance           = aZone.m_ZoneClearance;     // clearance value
    m_ZoneMinThickness        = aZone.m_ZoneMinThickness;
    m_fillVersion             = aZone.m_fillVersion;
    m_islandRemovalMode       = aZone.m_islandRemovalMode;
    m_minIslandArea           = aZone.m_minIslandArea;

    m_isFilled                = aZone.m_isFilled;
    m_needRefill              = aZone.m_needRefill;
    m_teardropType            = aZone.m_teardropType;

    m_thermalReliefGap        = aZone.m_thermalReliefGap;
    m_thermalReliefSpokeWidth = aZone.m_thermalReliefSpokeWidth;

    m_fillMode                = aZone.m_fillMode;         // solid vs. hatched
    m_hatchThickness          = aZone.m_hatchThickness;
    m_hatchGap                = aZone.m_hatchGap;
    m_hatchOrientation        = aZone.m_hatchOrientation;
    m_hatchSmoothingLevel     = aZone.m_hatchSmoothingLevel;
    m_hatchSmoothingValue     = aZone.m_hatchSmoothingValue;
    m_hatchBorderAlgorithm    = aZone.m_hatchBorderAlgorithm;
    m_hatchHoleMinArea        = aZone.m_hatchHoleMinArea;

    // For corner moving, corner index to drag, or nullptr if no selection
    delete m_CornerSelection;
    m_CornerSelection         = nullptr;

    aZone.GetLayerSet().RunOnLayers(
            [&]( PCB_LAYER_ID layer )
            {
                std::shared_ptr<SHAPE_POLY_SET> fill = aZone.m_FilledPolysList.at( layer );

                if( fill )
                    m_FilledPolysList[layer] = std::make_shared<SHAPE_POLY_SET>( *fill );
                else
                    m_FilledPolysList[layer] = std::make_shared<SHAPE_POLY_SET>();

                m_filledPolysHash[layer]  = aZone.m_filledPolysHash.at( layer );
                m_insulatedIslands[layer] = aZone.m_insulatedIslands.at( layer );
            } );

    m_borderStyle             = aZone.m_borderStyle;
    m_borderHatchPitch        = aZone.m_borderHatchPitch;
    m_borderHatchLines        = aZone.m_borderHatchLines;

    SetLocalFlags( aZone.GetLocalFlags() );

    m_netinfo                 = aZone.m_netinfo;
    m_area                    = aZone.m_area;
    m_outlinearea             = aZone.m_outlinearea;
}


EDA_ITEM* ZONE::Clone() const
{
    return new ZONE( *this );
}


bool ZONE::HigherPriority( const ZONE* aOther ) const
{
    // Teardrops are always higher priority than regular zones, so if one zone is a teardrop
    // and the other is not, then return higher priority as the teardrop
    if( ( m_teardropType == TEARDROP_TYPE::TD_NONE )
            ^ ( aOther->m_teardropType == TEARDROP_TYPE::TD_NONE ) )
    {
        return static_cast<int>( m_teardropType ) > static_cast<int>( aOther->m_teardropType );
    }

    if( m_priority != aOther->m_priority )
        return m_priority > aOther->m_priority;

    return m_Uuid > aOther->m_Uuid;
}


bool ZONE::SameNet( const ZONE* aOther ) const
{
    return GetNetCode() == aOther->GetNetCode();
}


bool ZONE::UnFill()
{
    bool change = false;

    for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& pair : m_FilledPolysList )
    {
        change |= !pair.second->IsEmpty();
        m_insulatedIslands[pair.first].clear();
        pair.second->RemoveAllContours();
    }

    m_isFilled = false;
    m_fillFlags.reset();

    return change;
}


bool ZONE::IsConflicting() const
{
    return HasFlag( COURTYARD_CONFLICT );
}


VECTOR2I ZONE::GetPosition() const
{
    return GetCornerPosition( 0 );
}


PCB_LAYER_ID ZONE::GetLayer() const
{
    if( m_layerSet.count() == 1 )
    {
        return GetFirstLayer();
    }
    return UNDEFINED_LAYER;
}


PCB_LAYER_ID ZONE::GetFirstLayer() const
{
    if( m_layerSet.count() == 0 )
    {
        return UNDEFINED_LAYER;
    }

    const LSEQ uiLayers = m_layerSet.UIOrder();

    // This can't use m_layerSet.count() because it's possible to have a zone on
    // a rescue layer that is not in the UI order.
    if( uiLayers.size() )
    {
        return uiLayers[0];
    }

    // If it's not in the UI set at all, just return the first layer in the set.
    // (we know the count > 0)
    return m_layerSet.Seq()[0];
}


bool ZONE::IsOnCopperLayer() const
{
    return ( m_layerSet & LSET::AllCuMask() ).count() > 0;
}


void ZONE::SetLayer( PCB_LAYER_ID aLayer )
{
    SetLayerSet( LSET( { aLayer } ) );
}


void ZONE::SetLayerSet( const LSET& aLayerSet )
{
    if( aLayerSet.count() == 0 )
        return;

    if( m_layerSet != aLayerSet )
    {
        SetNeedRefill( true );

        UnFill();

        m_FilledPolysList.clear();
        m_filledPolysHash.clear();
        m_insulatedIslands.clear();

        aLayerSet.RunOnLayers(
                [&]( PCB_LAYER_ID layer )
                {
                    m_FilledPolysList[layer]  = std::make_shared<SHAPE_POLY_SET>();
                    m_filledPolysHash[layer]  = {};
                    m_insulatedIslands[layer] = {};
                } );
    }

    m_layerSet = aLayerSet;
}


void ZONE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    m_layerSet.RunOnLayers(
            [&]( PCB_LAYER_ID layer )
            {
                aLayers[ aCount++ ] = layer;
                aLayers[ aCount++ ] = layer + static_cast<int>( LAYER_ZONE_START );
            } );

    if( IsConflicting() )
        aLayers[ aCount++ ] = LAYER_CONFLICTS_SHADOW;
}


double ZONE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    if( !aView )
        return 0;

    if( !aView->IsLayerVisible( LAYER_ZONES ) )
        return HIDE;

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        bool flipped = parentFP->GetLayer() == B_Cu;

        // Handle Render tab switches
        if( !flipped && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return HIDE;

        if( flipped && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return HIDE;
    }

    // Other layers are shown without any conditions
    return 0.0;
}


bool ZONE::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    return m_layerSet.test( aLayer );
}


const BOX2I ZONE::GetBoundingBox() const
{
    if( const BOARD* board = GetBoard() )
    {
        std::unordered_map<const ZONE*, BOX2I>& cache = board->m_ZoneBBoxCache;

        {
            std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

            auto cacheIter = cache.find( this );

            if( cacheIter != cache.end() )
                return cacheIter->second;
        }

        BOX2I bbox = m_Poly->BBox();

        {
            std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
            cache[ this ] = bbox;
        }

        return bbox;
    }

    return m_Poly->BBox();
}


void ZONE::CacheBoundingBox()
{
    // GetBoundingBox() will cache it for us, and there's no sense duplicating the somewhat tricky
    // locking code.
    GetBoundingBox();
}


int ZONE::GetThermalReliefGap( PAD* aPad, wxString* aSource ) const
{
    if( aPad->GetLocalThermalGapOverride() == 0 )
    {
        if( aSource )
            *aSource = _( "zone" );

        return m_thermalReliefGap;
    }

    return aPad->GetLocalThermalGapOverride( aSource );

}


void ZONE::SetCornerRadius( unsigned int aRadius )
{
    if( m_cornerRadius != aRadius )
        SetNeedRefill( true );

    m_cornerRadius = aRadius;
}


static SHAPE_POLY_SET g_nullPoly;


HASH_128 ZONE::GetHashValue( PCB_LAYER_ID aLayer )
{
    if( !m_filledPolysHash.count( aLayer ) )
        return g_nullPoly.GetHash();
    else
        return m_filledPolysHash.at( aLayer );
}


void ZONE::BuildHashValue( PCB_LAYER_ID aLayer )
{
    if( !m_FilledPolysList.count( aLayer ) )
        m_filledPolysHash[aLayer] = g_nullPoly.GetHash();
    else
        m_filledPolysHash[aLayer] = m_FilledPolysList.at( aLayer )->GetHash();
}


bool ZONE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // When looking for an "exact" hit aAccuracy will be 0 which works poorly for very thin
    // lines.  Give it a floor.
    int accuracy = std::max( aAccuracy, pcbIUScale.mmToIU( 0.1 ) );

    return HitTestForCorner( aPosition, accuracy * 2 ) || HitTestForEdge( aPosition, accuracy );
}


bool ZONE::HitTestForCorner( const VECTOR2I& refPos, int aAccuracy,
                             SHAPE_POLY_SET::VERTEX_INDEX* aCornerHit ) const
{
    return m_Poly->CollideVertex( VECTOR2I( refPos ), aCornerHit, aAccuracy );
}


bool ZONE::HitTestForEdge( const VECTOR2I& refPos, int aAccuracy,
                           SHAPE_POLY_SET::VERTEX_INDEX* aCornerHit ) const
{
    return m_Poly->CollideEdge( VECTOR2I( refPos ), aCornerHit, aAccuracy );
}


bool ZONE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    // Calculate bounding box for zone
    BOX2I bbox = GetBoundingBox();
    bbox.Normalize();

    BOX2I arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    if( aContained )
    {
         return arect.Contains( bbox );
    }
    else
    {
        // Fast test: if aBox is outside the polygon bounding box, rectangles cannot intersect
        if( !arect.Intersects( bbox ) )
            return false;

        int count = m_Poly->TotalVertices();

        for( int ii = 0; ii < count; ii++ )
        {
            VECTOR2I vertex = m_Poly->CVertex( ii );
            VECTOR2I vertexNext = m_Poly->CVertex( ( ii + 1 ) % count );

            // Test if the point is within the rect
            if( arect.Contains( vertex ) )
                return true;

            // Test if this edge intersects the rect
            if( arect.Intersects( vertex, vertexNext ) )
                return true;
        }

        return false;
    }
}


std::optional<int> ZONE::GetLocalClearance() const
{
    return m_isRuleArea ? 0 : m_ZoneClearance;
}


bool ZONE::HitTestFilledArea( PCB_LAYER_ID aLayer, const VECTOR2I& aRefPos, int aAccuracy ) const
{
    // Rule areas have no filled area, but it's generally nice to treat their interior as if it were
    // filled so that people don't have to select them by their outline (which is min-width)
    if( GetIsRuleArea() )
        return m_Poly->Contains( aRefPos, -1, aAccuracy );

    if( !m_FilledPolysList.count( aLayer ) )
        return false;

    return m_FilledPolysList.at( aLayer )->Contains( aRefPos, -1, aAccuracy );
}


bool ZONE::HitTestCutout( const VECTOR2I& aRefPos, int* aOutlineIdx, int* aHoleIdx ) const
{
    // Iterate over each outline polygon in the zone and then iterate over
    // each hole it has to see if the point is in it.
    for( int i = 0; i < m_Poly->OutlineCount(); i++ )
    {
        for( int j = 0; j < m_Poly->HoleCount( i ); j++ )
        {
            if( m_Poly->Hole( i, j ).PointInside( aRefPos ) )
            {
                if( aOutlineIdx )
                    *aOutlineIdx = i;

                if( aHoleIdx )
                    *aHoleIdx = j;

                return true;
            }
        }
    }

    return false;
}


void ZONE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg = GetFriendlyName();

    // Display Cutout instead of Outline for holes inside a zone (i.e. when num contour !=0).
    // Check whether the selected corner is in a hole; i.e., in any contour but the first one.
    if( m_CornerSelection != nullptr && m_CornerSelection->m_contour > 0 )
        msg << wxT( " " ) << _( "Cutout" );

    aList.emplace_back( _( "Type" ), msg );

    if( GetIsRuleArea() )
    {
        msg.Empty();

        if( GetDoNotAllowVias() )
            AccumulateDescription( msg, _( "No vias" ) );

        if( GetDoNotAllowTracks() )
            AccumulateDescription( msg, _( "No tracks" ) );

        if( GetDoNotAllowPads() )
            AccumulateDescription( msg, _( "No pads" ) );

        if( GetDoNotAllowCopperPour() )
            AccumulateDescription( msg, _( "No copper zones" ) );

        if( GetDoNotAllowFootprints() )
            AccumulateDescription( msg, _( "No footprints" ) );

        if( !msg.IsEmpty() )
            aList.emplace_back( _( "Restrictions" ), msg );

        if( GetRuleAreaPlacementEnabled() )
        {
            aList.emplace_back( _( "Placement source" ),
                                UnescapeString( GetRuleAreaPlacementSource() ) );
        }
    }
    else if( IsOnCopperLayer() )
    {
        if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
        {
            aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

            aList.emplace_back( _( "Resolved Netclass" ),
                                UnescapeString( GetEffectiveNetClass()->GetName() ) );
        }

        // Display priority level
        aList.emplace_back( _( "Priority" ),
                            wxString::Format( wxT( "%d" ), GetAssignedPriority() ) );
    }

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        if( IsLocked() )
            aList.emplace_back( _( "Status" ), _( "Locked" ) );
    }

    LSEQ     layers = m_layerSet.Seq();
    wxString layerDesc;

    if( layers.size() == 1 )
    {
        layerDesc.Printf( _( "%s" ), GetBoard()->GetLayerName( layers[0] ) );
    }
    else if (layers.size() == 2 )
    {
        layerDesc.Printf( _( "%s and %s" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ) );
    }
    else if (layers.size() == 3 )
    {
        layerDesc.Printf( _( "%s, %s and %s" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ),
                          GetBoard()->GetLayerName( layers[2] ) );
    }
    else if( layers.size() > 3 )
    {
        layerDesc.Printf( _( "%s, %s and %d more" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ),
                          static_cast<int>( layers.size() - 2 ) );
    }

    aList.emplace_back( _( "Layer" ), layerDesc );

    if( !m_zoneName.empty() )
        aList.emplace_back( _( "Name" ), m_zoneName );

    switch( m_fillMode )
    {
    case ZONE_FILL_MODE::POLYGONS:      msg = _( "Solid" ); break;
    case ZONE_FILL_MODE::HATCH_PATTERN: msg = _( "Hatched" ); break;
    default:                            msg = _( "Unknown" ); break;
    }

    aList.emplace_back( _( "Fill Mode" ), msg );

    aList.emplace_back( _( "Filled Area" ),
                        aFrame->MessageTextFromValue( m_area, true, EDA_DATA_TYPE::AREA ) );

    wxString source;
    int      clearance = GetOwnClearance( UNDEFINED_LAYER, &source );

    if( !source.IsEmpty() )
    {
        aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                              aFrame->MessageTextFromValue( clearance ) ),
                            wxString::Format( _( "(from %s)" ),
                                              source ) );
    }

    if( !m_FilledPolysList.empty() )
    {
        int count = 0;

        for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& ii: m_FilledPolysList )
            count += ii.second->TotalVertices();

        aList.emplace_back( _( "Corner Count" ), wxString::Format( wxT( "%d" ), count ) );
    }
}


void ZONE::Move( const VECTOR2I& offset )
{
    /* move outlines */
    m_Poly->Move( offset );

    HatchBorder();

    /* move fills */
    for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& pair : m_FilledPolysList )
        pair.second->Move( offset );

    /*
     * move boundingbox cache
     *
     * While the cache will get nuked at the conclusion of the operation, we use it for some
     * things (such as drawing the parent group) during the move.
     */
    if( GetBoard() )
    {
        auto it = GetBoard()->m_ZoneBBoxCache.find( this );

        if( it != GetBoard()->m_ZoneBBoxCache.end() )
            it->second.Move( offset );
    }
}


wxString ZONE::GetFriendlyName() const
{
    if( GetIsRuleArea() )
        return _( "Rule Area" );
    else if( IsTeardropArea() )
        return _( "Teardrop Area" );
    else if( IsOnCopperLayer() )
        return _( "Copper Zone" );
    else
        return _( "Non-copper Zone" );
}


void ZONE::MoveEdge( const VECTOR2I& offset, int aEdge )
{
    int next_corner;

    if( m_Poly->GetNeighbourIndexes( aEdge, nullptr, &next_corner ) )
    {
        m_Poly->SetVertex( aEdge, m_Poly->CVertex( aEdge ) + VECTOR2I( offset ) );
        m_Poly->SetVertex( next_corner, m_Poly->CVertex( next_corner ) + VECTOR2I( offset ) );
        HatchBorder();

        SetNeedRefill( true );
    }
}


void ZONE::Rotate( const VECTOR2I& aCentre, const EDA_ANGLE& aAngle )
{
    m_Poly->Rotate( aAngle, aCentre );
    HatchBorder();

    /* rotate filled areas: */
    for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& pair : m_FilledPolysList )
        pair.second->Rotate( aAngle, aCentre );
}


void ZONE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    Mirror( aCentre, aFlipDirection );

    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> fillsCopy;

    for( auto& [oldLayer, shapePtr] : m_FilledPolysList )
    {
        fillsCopy[oldLayer] = *shapePtr;
    }

    SetLayerSet( GetLayerSet().Flip( GetBoard()->GetCopperLayerCount() ) );

    for( auto& [oldLayer, shape] : fillsCopy )
    {
        PCB_LAYER_ID newLayer = GetBoard()->FlipLayer( oldLayer );
        SetFilledPolysList( newLayer, shape );
    }
}


void ZONE::Mirror( const VECTOR2I& aMirrorRef, FLIP_DIRECTION aFlipDirection )
{
    m_Poly->Mirror( aMirrorRef, aFlipDirection );

    HatchBorder();

    for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& pair : m_FilledPolysList )
        pair.second->Mirror( aMirrorRef, aFlipDirection );
}


void ZONE::RemoveCutout( int aOutlineIdx, int aHoleIdx )
{
    // Ensure the requested cutout is valid
    if( m_Poly->OutlineCount() < aOutlineIdx || m_Poly->HoleCount( aOutlineIdx ) < aHoleIdx )
        return;

    SHAPE_POLY_SET cutPoly( m_Poly->Hole( aOutlineIdx, aHoleIdx ) );

    // Add the cutout back to the zone
    m_Poly->BooleanAdd( cutPoly, SHAPE_POLY_SET::PM_FAST );

    SetNeedRefill( true );
}


void ZONE::AddPolygon( const SHAPE_LINE_CHAIN& aPolygon )
{
    wxASSERT( aPolygon.IsClosed() );

    // Add the outline as a new polygon in the polygon set
    if( m_Poly->OutlineCount() == 0 )
        m_Poly->AddOutline( aPolygon );
    else
        m_Poly->AddHole( aPolygon );

    SetNeedRefill( true );
}


void ZONE::AddPolygon( std::vector<VECTOR2I>& aPolygon )
{
    if( aPolygon.empty() )
        return;

    SHAPE_LINE_CHAIN outline;

    // Create an outline and populate it with the points of aPolygon
    for( const VECTOR2I& pt : aPolygon )
        outline.Append( pt );

    outline.SetClosed( true );

    AddPolygon( outline );
}


bool ZONE::AppendCorner( VECTOR2I aPosition, int aHoleIdx, bool aAllowDuplication )
{
    // Ensure the main outline exists:
    if( m_Poly->OutlineCount() == 0 )
        m_Poly->NewOutline();

    // If aHoleIdx >= 0, the corner musty be added to the hole, index aHoleIdx.
    // (remember: the index of the first hole is 0)
    // Return error if it does not exist.
    if( aHoleIdx >= m_Poly->HoleCount( 0 ) )
        return false;

    m_Poly->Append( aPosition.x, aPosition.y, -1, aHoleIdx, aAllowDuplication );

    SetNeedRefill( true );

    return true;
}


wxString ZONE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    LSEQ     layers = m_layerSet.Seq();
    wxString layerDesc;

    if( layers.size() == 1 )
    {
        layerDesc.Printf( _( "on %s" ), GetBoard()->GetLayerName( layers[0] ) );
    }
    else if (layers.size() == 2 )
    {
        layerDesc.Printf( _( "on %s and %s" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ) );
    }
    else if (layers.size() == 3 )
    {
        layerDesc.Printf( _( "on %s, %s and %s" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ),
                          GetBoard()->GetLayerName( layers[2] ) );
    }
    else if( layers.size() > 3 )
    {
        layerDesc.Printf( _( "on %s, %s and %zu more" ),
                          GetBoard()->GetLayerName( layers[0] ),
                          GetBoard()->GetLayerName( layers[1] ),
                          layers.size() - 2 );
    }

    // Check whether the selected contour is a hole (contour index > 0)
    if( m_CornerSelection != nullptr &&  m_CornerSelection->m_contour > 0 )
    {
        if( GetIsRuleArea() )
            return wxString::Format( _( "Rule Area Cutout %s" ), layerDesc  );
        else
            return wxString::Format( _( "Zone Cutout %s" ), layerDesc  );
    }
    else
    {
        if( GetIsRuleArea() )
            return wxString::Format( _( "Rule Area %s" ), layerDesc );
        else
            return wxString::Format( _( "Zone %s %s" ), GetNetnameMsg(), layerDesc );
    }
}


int ZONE::GetBorderHatchPitch() const
{
    return m_borderHatchPitch;
}


void ZONE::SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE aBorderHatchStyle,
                                  int aBorderHatchPitch, bool aRebuildBorderHatch )
{
    aBorderHatchPitch = std::max( aBorderHatchPitch,
                                  pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MINDIST_MM ) );
    aBorderHatchPitch = std::min( aBorderHatchPitch,
                                  pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MAXDIST_MM ) );
    SetBorderHatchPitch( aBorderHatchPitch );
    m_borderStyle = aBorderHatchStyle;

    if( aRebuildBorderHatch )
        HatchBorder();
}


void ZONE::SetBorderHatchPitch( int aPitch )
{
    m_borderHatchPitch = aPitch;
}


void ZONE::UnHatchBorder()
{
    m_borderHatchLines.clear();
}


// Creates hatch lines inside the outline of the complex polygon
// sort function used in ::HatchBorder to sort points by descending VECTOR2I.x values
bool sortEndsByDescendingX( const VECTOR2I& ref, const VECTOR2I& tst )
{
    return tst.x < ref.x;
}


void ZONE::HatchBorder()
{
    UnHatchBorder();

    if( m_borderStyle == ZONE_BORDER_DISPLAY_STYLE::NO_HATCH
            || m_borderHatchPitch == 0
            || m_Poly->IsEmpty() )
    {
        return;
    }

    // define range for hatch lines
    int min_x = m_Poly->CVertex( 0 ).x;
    int max_x = m_Poly->CVertex( 0 ).x;
    int min_y = m_Poly->CVertex( 0 ).y;
    int max_y = m_Poly->CVertex( 0 ).y;

    for( auto iterator = m_Poly->IterateWithHoles(); iterator; iterator++ )
    {
        if( iterator->x < min_x )
            min_x = iterator->x;

        if( iterator->x > max_x )
            max_x = iterator->x;

        if( iterator->y < min_y )
            min_y = iterator->y;

        if( iterator->y > max_y )
            max_y = iterator->y;
    }

    // Calculate spacing between 2 hatch lines
    int spacing;

    if( m_borderStyle == ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE )
        spacing = m_borderHatchPitch;
    else
        spacing = m_borderHatchPitch * 2;

    // set the "length" of hatch lines (the length on horizontal axis)
    int  hatch_line_len = m_borderHatchPitch;

    // To have a better look, give a slope depending on the layer
    int              layer = GetFirstLayer();
    std::vector<int> slope_flags;

    if( IsTeardropArea() )
        slope_flags = { 1, -1 };
    else if( layer & 1 )
        slope_flags = { 1 };
    else
        slope_flags = { -1 };

    for( int slope_flag : slope_flags )
    {
        double  slope = 0.707106 * slope_flag;      // 45 degrees slope
        int64_t max_a, min_a;

        if( slope_flag == 1 )
        {
            max_a = KiROUND<double, int64_t>( max_y - slope * min_x );
            min_a = KiROUND<double, int64_t>( min_y - slope * max_x );
        }
        else
        {
            max_a = KiROUND<double, int64_t>( max_y - slope * max_x );
            min_a = KiROUND<double, int64_t>( min_y - slope * min_x );
        }

        min_a = (min_a / spacing) * spacing;

        // calculate an offset depending on layer number,
        // for a better look of hatches on a multilayer board
        int offset = (layer * 7) / 8;
        min_a += offset;

        // loop through hatch lines
        std::vector<VECTOR2I> pointbuffer;
        pointbuffer.reserve( 256 );

        for( int64_t a = min_a; a < max_a; a += spacing )
        {
            pointbuffer.clear();

            // Iterate through all vertices
            for( auto iterator = m_Poly->IterateSegmentsWithHoles(); iterator; iterator++ )
            {
                const SEG seg = *iterator;
                double    x, y;

                if( FindLineSegmentIntersection( a, slope, seg.A.x, seg.A.y, seg.B.x, seg.B.y, x, y ) )
                    pointbuffer.emplace_back( KiROUND( x ), KiROUND( y ) );
            }

            // sort points in order of descending x (if more than 2) to
            // ensure the starting point and the ending point of the same segment
            // are stored one just after the other.
            if( pointbuffer.size() > 2 )
                sort( pointbuffer.begin(), pointbuffer.end(), sortEndsByDescendingX );

            // creates lines or short segments inside the complex polygon
            for( size_t ip = 0; ip + 1 < pointbuffer.size(); ip += 2 )
            {
                int dx = pointbuffer[ip + 1].x - pointbuffer[ip].x;

                // Push only one line for diagonal hatch,
                // or for small lines < twice the line length
                // else push 2 small lines
                if( m_borderStyle == ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL
                    || std::abs( dx ) < 2 * hatch_line_len )
                {
                    m_borderHatchLines.emplace_back( SEG( pointbuffer[ip], pointbuffer[ ip + 1] ) );
                }
                else
                {
                    double dy = pointbuffer[ip + 1].y - pointbuffer[ip].y;
                    slope = dy / dx;

                    if( dx > 0 )
                        dx = hatch_line_len;
                    else
                        dx = -hatch_line_len;

                    int x1 = KiROUND( pointbuffer[ip].x + dx );
                    int x2 = KiROUND( pointbuffer[ip + 1].x - dx );
                    int y1 = KiROUND( pointbuffer[ip].y + dx * slope );
                    int y2 = KiROUND( pointbuffer[ip + 1].y - dx * slope );

                    m_borderHatchLines.emplace_back( SEG( pointbuffer[ip].x, pointbuffer[ip].y,
                                                          x1, y1 ) );

                    m_borderHatchLines.emplace_back( SEG( pointbuffer[ip+1].x, pointbuffer[ip+1].y,
                                                          x2, y2 ) );
                }
            }
        }
    }
}


int ZONE::GetDefaultHatchPitch()
{
    return pcbIUScale.mmToIU( ZONE_BORDER_HATCH_DIST_MM );
}


BITMAPS ZONE::GetMenuImage() const
{
    return BITMAPS::add_zone;
}


void ZONE::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_ZONE_T );

    std::swap( *static_cast<ZONE*>( this ), *static_cast<ZONE*>( aImage) );
}


void ZONE::CacheTriangulation( PCB_LAYER_ID aLayer )
{
    if( aLayer == UNDEFINED_LAYER )
    {
        for( auto& [ layer, poly ] : m_FilledPolysList )
            poly->CacheTriangulation();

        m_Poly->CacheTriangulation( false );
    }
    else
    {
        if( m_FilledPolysList.count( aLayer ) )
            m_FilledPolysList[ aLayer ]->CacheTriangulation();
    }
}


bool ZONE::IsIsland( PCB_LAYER_ID aLayer, int aPolyIdx ) const
{
    if( GetNetCode() < 1 )
        return true;

    if( !m_insulatedIslands.count( aLayer ) )
        return false;

    return m_insulatedIslands.at( aLayer ).count( aPolyIdx );
}


void ZONE::GetInteractingZones( PCB_LAYER_ID aLayer, std::vector<ZONE*>* aSameNetCollidingZones,
                                std::vector<ZONE*>* aOtherNetIntersectingZones ) const
{
    int   epsilon = pcbIUScale.mmToIU( 0.001 );
    BOX2I bbox = GetBoundingBox();

    bbox.Inflate( epsilon );

    for( ZONE* candidate : GetBoard()->Zones() )
    {
        if( candidate == this )
            continue;

        if( !candidate->GetLayerSet().test( aLayer ) )
            continue;

        if( candidate->GetIsRuleArea() || candidate->IsTeardropArea() )
            continue;

        if( !candidate->GetBoundingBox().Intersects( bbox ) )
            continue;

        if( candidate->GetNetCode() == GetNetCode() )
        {
            if( m_Poly->Collide( candidate->m_Poly ) )
                aSameNetCollidingZones->push_back( candidate );
        }
        else
        {
            aOtherNetIntersectingZones->push_back( candidate );
        }
    }
}


bool ZONE::BuildSmoothedPoly( SHAPE_POLY_SET& aSmoothedPoly, PCB_LAYER_ID aLayer,
                              SHAPE_POLY_SET* aBoardOutline,
                              SHAPE_POLY_SET* aSmoothedPolyWithApron ) const
{
    if( GetNumCorners() <= 2 )  // malformed zone. polygon calculations will not like it ...
        return false;

    // Processing of arc shapes in zones is not yet supported because Clipper can't do boolean
    // operations on them.  The poly outline must be converted to segments first.
    SHAPE_POLY_SET flattened = m_Poly->CloneDropTriangulation();
    flattened.ClearArcs();

    if( GetIsRuleArea() )
    {
        // We like keepouts just the way they are....
        aSmoothedPoly = flattened;
        return true;
    }

    const BOARD* board = GetBoard();
    int          maxError = ARC_HIGH_DEF;
    bool         keepExternalFillets = false;
    bool         smooth_requested = m_cornerSmoothingType == ZONE_SETTINGS::SMOOTHING_CHAMFER
                                    || m_cornerSmoothingType == ZONE_SETTINGS::SMOOTHING_FILLET;

    if( IsTeardropArea() )
    {
        // We use teardrop shapes with no smoothing; these shapes are already optimized
        smooth_requested = false;
    }

    if( board )
    {
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

        maxError = bds.m_MaxError;
        keepExternalFillets = bds.m_ZoneKeepExternalFillets;
    }

    auto smooth =
            [&]( SHAPE_POLY_SET& aPoly )
            {
                if( !smooth_requested )
                    return;

                switch( m_cornerSmoothingType )
                {
                case ZONE_SETTINGS::SMOOTHING_CHAMFER:
                    aPoly = aPoly.Chamfer( (int) m_cornerRadius );
                    break;

                case ZONE_SETTINGS::SMOOTHING_FILLET:
                    aPoly = aPoly.Fillet( (int) m_cornerRadius, maxError );
                    break;

                default:
                    break;
                }
            };

    SHAPE_POLY_SET* maxExtents = &flattened;
    SHAPE_POLY_SET  withFillets;

    aSmoothedPoly = flattened;

    // Should external fillets (that is, those applied to concave corners) be kept?  While it
    // seems safer to never have copper extend outside the zone outline, 5.1.x and prior did
    // indeed fill them so we leave the mode available.
    if( keepExternalFillets && smooth_requested )
    {
        withFillets = flattened;
        smooth( withFillets );
        withFillets.BooleanAdd( flattened, SHAPE_POLY_SET::PM_FAST );
        maxExtents = &withFillets;
    }

    // We now add in the areas of any same-net, intersecting zones.  This keeps us from smoothing
    // corners at an intersection (which often produces undesired divots between the intersecting
    // zones -- see #2752).
    //
    // After smoothing, we'll subtract back out everything outside of our zone.
    std::vector<ZONE*> sameNetCollidingZones;
    std::vector<ZONE*> diffNetIntersectingZones;
    GetInteractingZones( aLayer, &sameNetCollidingZones, &diffNetIntersectingZones );

    for( ZONE* sameNetZone : sameNetCollidingZones )
    {
        BOX2I sameNetBoundingBox = sameNetZone->GetBoundingBox();

        // Note: a two-pass algorithm could use sameNetZone's actual fill instead of its outline.
        // This would obviate the need for the below wrinkles, in addition to fixing both issues
        // in #16095.
        // (And we wouldn't need to collect all the diffNetIntersectingZones either.)

        SHAPE_POLY_SET sameNetPoly = sameNetZone->Outline()->CloneDropTriangulation();
        SHAPE_POLY_SET diffNetPoly;

        // Of course there's always a wrinkle.  The same-net intersecting zone *might* get knocked
        // out along the border by a higher-priority, different-net zone.  #12797
        for( ZONE* diffNetZone : diffNetIntersectingZones )
        {
            if( diffNetZone->HigherPriority( sameNetZone )
                    && diffNetZone->GetBoundingBox().Intersects( sameNetBoundingBox ) )
            {
                diffNetPoly.BooleanAdd( *diffNetZone->Outline(), SHAPE_POLY_SET::PM_FAST );
            }
        }

        // Second wrinkle.  After unioning the higher priority, different net zones together, we
        // need to check to see if they completely enclose our zone.  If they do, then we need to
        // treat the enclosed zone as isolated, not connected to the outer zone.  #13915
        bool isolated = false;

        if( diffNetPoly.OutlineCount() )
        {
            SHAPE_POLY_SET thisPoly = Outline()->CloneDropTriangulation();

            thisPoly.BooleanSubtract( diffNetPoly, SHAPE_POLY_SET::PM_FAST );
            isolated = thisPoly.OutlineCount() == 0;
        }

        if( !isolated )
        {
            sameNetPoly.ClearArcs();
            aSmoothedPoly.BooleanAdd( sameNetPoly, SHAPE_POLY_SET::PM_FAST );
        }
    }

    if( aBoardOutline )
        aSmoothedPoly.BooleanIntersection( *aBoardOutline, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    SHAPE_POLY_SET withSameNetIntersectingZones = aSmoothedPoly.CloneDropTriangulation();

    smooth( aSmoothedPoly );

    if( aSmoothedPolyWithApron )
    {
        // The same-net intersecting-zone code above makes sure the corner-smoothing algorithm
        // doesn't produce divots.  But the min-thickness algorithm applied in fillCopperZone()
        // is *also* going to perform a deflate/inflate cycle, again leading to divots.  So we
        // pre-inflate the contour by the min-thickness within the same-net-intersecting-zones
        // envelope.
        SHAPE_POLY_SET poly = maxExtents->CloneDropTriangulation();
        poly.Inflate( m_ZoneMinThickness, CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError );
        poly.BooleanIntersection( withSameNetIntersectingZones, SHAPE_POLY_SET::PM_FAST );

        *aSmoothedPolyWithApron = aSmoothedPoly;
        aSmoothedPolyWithApron->BooleanIntersection( poly, SHAPE_POLY_SET::PM_FAST );
    }

    aSmoothedPoly.BooleanIntersection( *maxExtents, SHAPE_POLY_SET::PM_FAST );

    return true;
}


double ZONE::CalculateFilledArea()
{
    m_area = 0.0;

    // Iterate over each outline polygon in the zone and then iterate over
    // each hole it has to compute the total area.
    for( std::pair<const PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>>& pair : m_FilledPolysList )
    {
        std::shared_ptr<SHAPE_POLY_SET>& poly = pair.second;

        for( int i = 0; i < poly->OutlineCount(); i++ )
        {
            m_area += poly->Outline( i ).Area();

            for( int j = 0; j < poly->HoleCount( i ); j++ )
                m_area -= poly->Hole( i, j ).Area();
        }
    }

    return m_area;
}


double ZONE::CalculateOutlineArea()
{
    m_outlinearea = std::abs( m_Poly->Area() );
    return m_outlinearea;
}


void ZONE::TransformSmoothedOutlineToPolygon( SHAPE_POLY_SET& aBuffer, int aClearance,
                                              int aMaxError, ERROR_LOC aErrorLoc,
                                              SHAPE_POLY_SET* aBoardOutline ) const
{
    // Creates the zone outline polygon (with holes if any)
    SHAPE_POLY_SET polybuffer;

    // TODO: using GetFirstLayer() means it only works for single-layer zones....
    BuildSmoothedPoly( polybuffer, GetFirstLayer(), aBoardOutline );

    // Calculate the polygon with clearance
    // holes are linked to the main outline, so only one polygon is created.
    if( aClearance )
    {
        const BOARD* board = GetBoard();
        int          maxError = ARC_HIGH_DEF;

        if( board )
            maxError = board->GetDesignSettings().m_MaxError;

        if( aErrorLoc == ERROR_OUTSIDE )
            aClearance += maxError;

        polybuffer.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError );
    }

    polybuffer.Fracture( SHAPE_POLY_SET::PM_FAST );
    aBuffer.Append( polybuffer );
}


std::shared_ptr<SHAPE> ZONE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( m_FilledPolysList.find( aLayer ) == m_FilledPolysList.end() )
        return std::make_shared<SHAPE_NULL>();
    else
        return m_FilledPolysList.at( aLayer );
}


void ZONE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                    int aError, ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
{
    wxASSERT_MSG( !aIgnoreLineWidth, wxT( "IgnoreLineWidth has no meaning for zones." ) );

    if( !m_FilledPolysList.count( aLayer ) )
        return;

    if( !aClearance )
    {
        aBuffer.Append( *m_FilledPolysList.at( aLayer ) );
        return;
    }

    SHAPE_POLY_SET temp_buf = m_FilledPolysList.at( aLayer )->CloneDropTriangulation();

    // Rebuild filled areas only if clearance is not 0
    if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
    {
        if( aErrorLoc == ERROR_OUTSIDE )
            aClearance += aError;

        temp_buf.InflateWithLinkedHoles( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aError,
                                         SHAPE_POLY_SET::PM_FAST );
    }

    aBuffer.Append( temp_buf );
}


void ZONE::TransformSolidAreasShapesToPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aBuffer ) const
{
    if( m_FilledPolysList.count( aLayer ) && !m_FilledPolysList.at( aLayer )->IsEmpty() )
        aBuffer.Append( *m_FilledPolysList.at( aLayer ) );
}


bool ZONE::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const ZONE& other = static_cast<const ZONE&>( aOther );
    return *this == other;
}


bool ZONE::operator==( const ZONE& aOther ) const

{
    if( aOther.Type() != Type() )
        return false;

    const ZONE& other = static_cast<const ZONE&>( aOther );

    if( GetIsRuleArea() != other.GetIsRuleArea() )
        return false;

     if( GetIsRuleArea() )
     {
         if( GetDoNotAllowCopperPour() != other.GetDoNotAllowCopperPour() )
             return false;

         if( GetDoNotAllowTracks() != other.GetDoNotAllowTracks() )
             return false;

         if( GetDoNotAllowVias() != other.GetDoNotAllowVias() )
             return false;

         if( GetDoNotAllowFootprints() != other.GetDoNotAllowFootprints() )
             return false;

         if( GetDoNotAllowPads() != other.GetDoNotAllowPads() )
             return false;

         if( GetRuleAreaPlacementEnabled() != other.GetRuleAreaPlacementEnabled() )
             return false;

         if( GetRuleAreaPlacementSourceType() != other.GetRuleAreaPlacementSourceType() )
             return false;

         if( GetRuleAreaPlacementSource() != other.GetRuleAreaPlacementSource() )
             return false;
    }
    else
    {
        if( GetAssignedPriority() != other.GetAssignedPriority() )
            return false;

        if( GetMinThickness() != other.GetMinThickness() )
            return false;

        if( GetCornerSmoothingType() != other.GetCornerSmoothingType() )
            return false;

        if( GetCornerRadius() != other.GetCornerRadius() )
            return false;

        if( GetTeardropParams() != other.GetTeardropParams() )
            return false;
    }

    if( GetNumCorners() != other.GetNumCorners() )
        return false;

    for( int ii = 0; ii < GetNumCorners(); ii++ )
    {
        if( GetCornerPosition( ii ) != other.GetCornerPosition( ii ) )
            return false;
    }

    return true;
}


double ZONE::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const ZONE& other = static_cast<const ZONE&>( aOther );

    if( GetIsRuleArea() != other.GetIsRuleArea() )
        return 0.0;

    double similarity = 1.0;

    if( GetLayerSet() != other.GetLayerSet() )
        similarity *= 0.9;

    if( GetNetCode() != other.GetNetCode() )
        similarity *= 0.9;

    if( !GetIsRuleArea() )
    {
        if( GetAssignedPriority() != other.GetAssignedPriority() )
            similarity *= 0.9;

        if( GetMinThickness() != other.GetMinThickness() )
            similarity *= 0.9;

        if( GetCornerSmoothingType() != other.GetCornerSmoothingType() )
            similarity *= 0.9;

        if( GetCornerRadius() != other.GetCornerRadius() )
            similarity *= 0.9;

        if( GetTeardropParams() != other.GetTeardropParams() )
            similarity *= 0.9;
    }
    else
    {
        if( GetDoNotAllowCopperPour() != other.GetDoNotAllowCopperPour() )
            similarity *= 0.9;
        if( GetDoNotAllowTracks() != other.GetDoNotAllowTracks() )
            similarity *= 0.9;
        if( GetDoNotAllowVias() != other.GetDoNotAllowVias() )
            similarity *= 0.9;
        if( GetDoNotAllowFootprints() != other.GetDoNotAllowFootprints() )
            similarity *= 0.9;
        if( GetDoNotAllowPads() != other.GetDoNotAllowPads() )
            similarity *= 0.9;
    }

    std::vector<VECTOR2I> corners;
    std::vector<VECTOR2I> otherCorners;
    VECTOR2I lastCorner( 0, 0 );

    for( int ii = 0; ii < GetNumCorners(); ii++ )
    {
        corners.push_back( lastCorner - GetCornerPosition( ii ) );
        lastCorner = GetCornerPosition( ii );
    }

    lastCorner = VECTOR2I( 0, 0 );
    for( int ii = 0; ii < other.GetNumCorners(); ii++ )
    {
        otherCorners.push_back( lastCorner - other.GetCornerPosition( ii ) );
        lastCorner = other.GetCornerPosition( ii );
    }

    size_t longest = alg::longest_common_subset( corners, otherCorners );

    similarity *= std::pow( 0.9, GetNumCorners() + other.GetNumCorners() - 2 * longest );

    return similarity;
}


static struct ZONE_DESC
{
    ZONE_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        ENUM_MAP<ZONE_CONNECTION>& zcMap = ENUM_MAP<ZONE_CONNECTION>::Instance();

        if( zcMap.Choices().GetCount() == 0 )
        {
            zcMap.Undefined( ZONE_CONNECTION::INHERITED );
            zcMap.Map( ZONE_CONNECTION::INHERITED,   _HKI( "Inherited" ) )
                 .Map( ZONE_CONNECTION::NONE,        _HKI( "None" ) )
                 .Map( ZONE_CONNECTION::THERMAL,     _HKI( "Thermal reliefs" ) )
                 .Map( ZONE_CONNECTION::FULL,        _HKI( "Solid" ) )
                 .Map( ZONE_CONNECTION::THT_THERMAL, _HKI( "Thermal reliefs for PTH" ) );
        }

        ENUM_MAP<ZONE_FILL_MODE>& zfmMap = ENUM_MAP<ZONE_FILL_MODE>::Instance();

        if( zfmMap.Choices().GetCount() == 0 )
        {
            zfmMap.Undefined( ZONE_FILL_MODE::POLYGONS );
            zfmMap.Map( ZONE_FILL_MODE::POLYGONS,      _HKI( "Solid fill" ) )
                  .Map( ZONE_FILL_MODE::HATCH_PATTERN, _HKI( "Hatch pattern" ) );
        }

        ENUM_MAP<ISLAND_REMOVAL_MODE>& irmMap = ENUM_MAP<ISLAND_REMOVAL_MODE>::Instance();

        if( irmMap.Choices().GetCount() == 0 )
        {
            irmMap.Undefined( ISLAND_REMOVAL_MODE::ALWAYS );
            irmMap.Map( ISLAND_REMOVAL_MODE::ALWAYS, _HKI( "Always" ) )
                  .Map( ISLAND_REMOVAL_MODE::NEVER,  _HKI( "Never" ) )
                  .Map( ISLAND_REMOVAL_MODE::AREA,   _HKI( "Below area limit" ) );
        }

        ENUM_MAP<RULE_AREA_PLACEMENT_SOURCE_TYPE>& rapstMap =
                ENUM_MAP<RULE_AREA_PLACEMENT_SOURCE_TYPE>::Instance();

        if( rapstMap.Choices().GetCount() == 0 )
        {
            rapstMap.Undefined( RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME );
            rapstMap.Map( RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME, _HKI( "Sheet Name" ) )
                    .Map( RULE_AREA_PLACEMENT_SOURCE_TYPE::COMPONENT_CLASS,
                          _HKI( "Component Class" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( ZONE );
        propMgr.InheritsAfter( TYPE_HASH( ZONE ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        // Mask layer and position properties; they aren't useful in current form
        auto posX = new PROPERTY<ZONE, int>( _HKI( "Position X" ), NO_SETTER( ZONE, int ),
                                             static_cast<int ( ZONE::* )() const>( &ZONE::GetX ),
                                             PROPERTY_DISPLAY::PT_COORD,
                                             ORIGIN_TRANSFORMS::ABS_X_COORD );
        posX->SetIsHiddenFromPropertiesManager();

        auto posY = new PROPERTY<ZONE, int>( _HKI( "Position Y" ), NO_SETTER( ZONE, int ),
                                             static_cast<int ( ZONE::* )() const>( &ZONE::GetY ),
                                             PROPERTY_DISPLAY::PT_COORD,
                                             ORIGIN_TRANSFORMS::ABS_Y_COORD );
        posY->SetIsHiddenFromPropertiesManager();

        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ), posX );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ), posY );

        auto isCopperZone =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
                        return !zone->GetIsRuleArea() && IsCopperLayer( zone->GetFirstLayer() );

                    return false;
                };

        auto isRuleArea =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
                        return zone->GetIsRuleArea();

                    return false;
                };

        auto isHatchedFill =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
                        return zone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN;

                    return false;
                };

        auto isAreaBasedIslandRemoval =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
                        return zone->GetIslandRemovalMode() == ISLAND_REMOVAL_MODE::AREA;

                    return false;
                };

        // Layer property is hidden because it only holds a single layer and zones actually use
        // a layer set
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ),
                                 new PROPERTY_ENUM<ZONE, PCB_LAYER_ID>( _HKI( "Layer" ),
                                                                        &ZONE::SetLayer,
                                                                        &ZONE::GetLayer ) )
                .SetIsHiddenFromPropertiesManager();

        propMgr.OverrideAvailability( TYPE_HASH( ZONE ), TYPE_HASH( BOARD_CONNECTED_ITEM ),
                                      _HKI( "Net" ), isCopperZone );
        propMgr.OverrideAvailability( TYPE_HASH( ZONE ), TYPE_HASH( BOARD_CONNECTED_ITEM ),
                                      _HKI( "Net Class" ), isCopperZone );

        propMgr.AddProperty( new PROPERTY<ZONE, unsigned>( _HKI( "Priority" ),
                    &ZONE::SetAssignedPriority, &ZONE::GetAssignedPriority ) )
                .SetAvailableFunc( isCopperZone );

        propMgr.AddProperty( new PROPERTY<ZONE, wxString>( _HKI( "Name" ),
                    &ZONE::SetZoneName, &ZONE::GetZoneName ) );

        const wxString groupKeepout = _HKI( "Keepout" );

        propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Keep Out Tracks" ),
                                                       &ZONE::SetDoNotAllowTracks,
                                                       &ZONE::GetDoNotAllowTracks ),
                             groupKeepout )
                .SetAvailableFunc( isRuleArea );

        propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Keep Out Vias" ),
                                                       &ZONE::SetDoNotAllowVias,
                                                       &ZONE::GetDoNotAllowVias ),
                             groupKeepout )
                .SetAvailableFunc( isRuleArea );

        propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Keep Out Pads" ),
                                                       &ZONE::SetDoNotAllowPads,
                                                       &ZONE::GetDoNotAllowPads ),
                             groupKeepout )
                .SetAvailableFunc( isRuleArea );

        propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Keep Out Copper Pours" ),
                                                       &ZONE::SetDoNotAllowCopperPour,
                                                       &ZONE::GetDoNotAllowCopperPour ),
                             groupKeepout )
                .SetAvailableFunc( isRuleArea );

        propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Keep Out Footprints" ),
                                                       &ZONE::SetDoNotAllowFootprints,
                                                       &ZONE::GetDoNotAllowFootprints ),
                             groupKeepout )
                .SetAvailableFunc( isRuleArea );

        if( ADVANCED_CFG::GetCfg().m_EnableMultichannelTool )
        {
            const wxString groupPlacement = _HKI( "Placement" );

            propMgr.AddProperty( new PROPERTY<ZONE, bool>( _HKI( "Enable" ),
                                                           &ZONE::SetRuleAreaPlacementEnabled,
                                                           &ZONE::GetRuleAreaPlacementEnabled ),
                                 groupPlacement )
                    .SetAvailableFunc( isRuleArea );

            propMgr.AddProperty( new PROPERTY_ENUM<ZONE, RULE_AREA_PLACEMENT_SOURCE_TYPE>(
                                         _HKI( "Source Type" ),
                                         &ZONE::SetRuleAreaPlacementSourceType,
                                         &ZONE::GetRuleAreaPlacementSourceType ),
                                 groupPlacement )
                    .SetAvailableFunc( isRuleArea );

            propMgr.AddProperty( new PROPERTY<ZONE, wxString>( _HKI( "Source Name" ),
                                                               &ZONE::SetRuleAreaPlacementSource,
                                                               &ZONE::GetRuleAreaPlacementSource ),
                                 groupPlacement )
                    .SetAvailableFunc( isRuleArea );
        }

        const wxString groupFill = _HKI( "Fill Style" );

        propMgr.AddProperty( new PROPERTY_ENUM<ZONE, ZONE_FILL_MODE>( _HKI( "Fill Mode" ),
                    &ZONE::SetFillMode, &ZONE::GetFillMode ),
                    groupFill )
                .SetAvailableFunc( isCopperZone );

        propMgr.AddProperty( new PROPERTY<ZONE, EDA_ANGLE>( _HKI( "Orientation" ),
                    &ZONE::SetHatchOrientation, &ZONE::GetHatchOrientation,
                    PROPERTY_DISPLAY::PT_DEGREE ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill );

        // TODO: Switch to translated
        auto atLeastMinWidthValidator =
                []( const wxAny&& aValue, EDA_ITEM* aZone ) -> VALIDATOR_RESULT
                {
                    int   val = aValue.As<int>();
                    ZONE* zone = dynamic_cast<ZONE*>( aZone );
                    wxCHECK( zone, std::nullopt );

                    if( val < zone->GetMinThickness() )
                    {
                        return std::make_unique<VALIDATION_ERROR_MSG>(
                                _( "Cannot be less than zone minimum width" ) );
                    }

                    return std::nullopt;
                };

        propMgr.AddProperty( new PROPERTY<ZONE, int>( _HKI( "Hatch Width" ),
                    &ZONE::SetHatchThickness, &ZONE::GetHatchThickness, PROPERTY_DISPLAY::PT_SIZE ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill )
                .SetValidator( atLeastMinWidthValidator );

        propMgr.AddProperty( new PROPERTY<ZONE, int>( _HKI( "Hatch Gap" ),
                    &ZONE::SetHatchGap, &ZONE::GetHatchGap, PROPERTY_DISPLAY::PT_SIZE ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill )
                .SetValidator( atLeastMinWidthValidator );

        propMgr.AddProperty( new PROPERTY<ZONE, double>( _HKI( "Hatch Minimum Hole Ratio" ),
                     &ZONE::SetHatchHoleMinArea, &ZONE::GetHatchHoleMinArea ),
                     groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill )
                .SetValidator( PROPERTY_VALIDATORS::PositiveRatioValidator );

        // TODO: Smoothing effort needs to change to enum (in dialog too)
        propMgr.AddProperty( new PROPERTY<ZONE, int>( _HKI( "Smoothing Effort" ),
                    &ZONE::SetHatchSmoothingLevel, &ZONE::GetHatchSmoothingLevel ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill );

        propMgr.AddProperty( new PROPERTY<ZONE, double>( _HKI( "Smoothing Amount" ),
                    &ZONE::SetHatchSmoothingValue, &ZONE::GetHatchSmoothingValue ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isHatchedFill );

        propMgr.AddProperty( new PROPERTY_ENUM<ZONE, ISLAND_REMOVAL_MODE>( _HKI( "Remove Islands" ),
                    &ZONE::SetIslandRemovalMode, &ZONE::GetIslandRemovalMode ),
                    groupFill )
                .SetAvailableFunc( isCopperZone );

        propMgr.AddProperty( new PROPERTY<ZONE, long long int>( _HKI( "Minimum Island Area" ),
                    &ZONE::SetMinIslandArea, &ZONE::GetMinIslandArea, PROPERTY_DISPLAY::PT_AREA ),
                    groupFill )
                .SetAvailableFunc( isCopperZone )
                .SetWriteableFunc( isAreaBasedIslandRemoval );

        const wxString groupElectrical = _HKI( "Electrical" );

        auto clearanceOverride = new PROPERTY<ZONE, std::optional<int>>( _HKI( "Clearance" ),
                    &ZONE::SetLocalClearance, &ZONE::GetLocalClearance,
                    PROPERTY_DISPLAY::PT_SIZE );
        clearanceOverride->SetAvailableFunc( isCopperZone );
        constexpr int maxClearance = pcbIUScale.mmToIU( ZONE_CLEARANCE_MAX_VALUE_MM );
        clearanceOverride->SetValidator( PROPERTY_VALIDATORS::RangeIntValidator<0, maxClearance> );

        auto minWidth = new PROPERTY<ZONE, int>( _HKI( "Minimum Width" ),
                    &ZONE::SetMinThickness, &ZONE::GetMinThickness,
                    PROPERTY_DISPLAY::PT_SIZE );
        minWidth->SetAvailableFunc( isCopperZone );
        constexpr int minMinWidth = pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM );
        clearanceOverride->SetValidator( PROPERTY_VALIDATORS::RangeIntValidator<minMinWidth,
                                                                                INT_MAX> );

        auto padConnections = new PROPERTY_ENUM<ZONE, ZONE_CONNECTION>( _HKI( "Pad Connections" ),
                    &ZONE::SetPadConnection, &ZONE::GetPadConnection );
        padConnections->SetAvailableFunc( isCopperZone );

        auto thermalGap = new PROPERTY<ZONE, int>( _HKI( "Thermal Relief Gap" ),
                    &ZONE::SetThermalReliefGap, &ZONE::GetThermalReliefGap,
                    PROPERTY_DISPLAY::PT_SIZE );
        thermalGap->SetAvailableFunc( isCopperZone );
        thermalGap->SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator );

        auto thermalSpokeWidth = new PROPERTY<ZONE, int>( _HKI( "Thermal Relief Spoke Width" ),
                    &ZONE::SetThermalReliefSpokeWidth, &ZONE::GetThermalReliefSpokeWidth,
                    PROPERTY_DISPLAY::PT_SIZE );
        thermalSpokeWidth->SetAvailableFunc( isCopperZone );
        thermalSpokeWidth->SetValidator( atLeastMinWidthValidator );

        propMgr.AddProperty( clearanceOverride, groupElectrical );
        propMgr.AddProperty( minWidth, groupElectrical );
        propMgr.AddProperty( padConnections, groupElectrical );
        propMgr.AddProperty( thermalGap, groupElectrical );
        propMgr.AddProperty( thermalSpokeWidth, groupElectrical );
    }
} _ZONE_DESC;

IMPLEMENT_ENUM_TO_WXANY( RULE_AREA_PLACEMENT_SOURCE_TYPE )
IMPLEMENT_ENUM_TO_WXANY( ZONE_CONNECTION )
IMPLEMENT_ENUM_TO_WXANY( ZONE_FILL_MODE )
IMPLEMENT_ENUM_TO_WXANY( ISLAND_REMOVAL_MODE )
