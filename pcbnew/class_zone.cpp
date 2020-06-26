/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <fctsys.h>
#include <geometry/geometry_utils.h>
#include <pcb_base_frame.h>
#include <pcb_screen.h>
#include <class_board.h>
#include <class_zone.h>
#include <pcb_edit_frame.h> // current layer for msgpanel
#include <math_for_graphics.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>


ZONE_CONTAINER::ZONE_CONTAINER( BOARD_ITEM_CONTAINER* aParent, bool aInModule )
        : BOARD_CONNECTED_ITEM( aParent, aInModule ? PCB_MODULE_ZONE_AREA_T : PCB_ZONE_AREA_T ),
          m_area( 0.0 )
{
    m_CornerSelection = nullptr;                // no corner is selected
    m_IsFilled = false;                         // fill status : true when the zone is filled
    m_FillMode = ZONE_FILL_MODE::POLYGONS;
    m_hatchStyle = ZONE_HATCH_STYLE::DIAGONAL_EDGE;
    m_hatchPitch = GetDefaultHatchPitch();
    m_hv45 = false;
    m_HatchFillTypeThickness = 0;
    m_HatchFillTypeGap = 0;
    m_HatchFillTypeOrientation = 0.0;
    m_HatchFillTypeSmoothingLevel = 0;          // Grid pattern smoothing type. 0 = no smoothing
    m_HatchFillTypeSmoothingValue = 0.1;        // Grid pattern chamfer value relative to the gap value
                                                // used only if m_HatchFillTypeSmoothingLevel > 0
    m_priority = 0;
    m_cornerSmoothingType = ZONE_SETTINGS::SMOOTHING_NONE;
    SetIsKeepout( aInModule ? true : false );   // Zones living in modules have the keepout option.
    SetDoNotAllowCopperPour( false );           // has meaning only if m_isKeepout == true
    SetDoNotAllowVias( true );                  // has meaning only if m_isKeepout == true
    SetDoNotAllowTracks( true );                // has meaning only if m_isKeepout == true
    SetDoNotAllowPads( true );                  // has meaning only if m_isKeepout == true
    SetDoNotAllowFootprints( false );           // has meaning only if m_isKeepout == true
    m_cornerRadius = 0;
    SetLocalFlags( 0 );                         // flags tempoarry used in zone calculations
    m_Poly = new SHAPE_POLY_SET();              // Outlines
    m_FilledPolysUseThickness = true;           // set the "old" way to build filled polygon areas (before 6.0.x)
    m_islandRemovalMode       = ISLAND_REMOVAL_MODE::ALWAYS;
    aParent->GetZoneSettings().ExportSetting( *this );

    m_needRefill = false;   // True only after some edition.
}


ZONE_CONTAINER::ZONE_CONTAINER( const ZONE_CONTAINER& aZone )
        : BOARD_CONNECTED_ITEM( aZone.GetParent(), PCB_ZONE_AREA_T )
{
    initDataFromSrcInCopyCtor( aZone );
}


ZONE_CONTAINER& ZONE_CONTAINER::operator=( const ZONE_CONTAINER& aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    // Replace the outlines for aOther outlines.
    delete m_Poly;
    m_Poly = new SHAPE_POLY_SET( *aOther.m_Poly );

    m_isKeepout = aOther.m_isKeepout;
    m_CornerSelection  = nullptr; // for corner moving, corner index to (null if no selection)
    m_ZoneClearance    = aOther.m_ZoneClearance;            // clearance value
    m_ZoneMinThickness = aOther.m_ZoneMinThickness;
    m_FilledPolysUseThickness = aOther.m_FilledPolysUseThickness;
    m_FillMode = aOther.m_FillMode;                         // filling mode (segments/polygons)
    m_PadConnection = aOther.m_PadConnection;
    m_ThermalReliefGap = aOther.m_ThermalReliefGap;
    m_ThermalReliefCopperBridge = aOther.m_ThermalReliefCopperBridge;
    SetHatchStyle( aOther.GetHatchStyle() );
    SetHatchPitch( aOther.GetHatchPitch() );
    m_HatchLines = aOther.m_HatchLines;     // copy vector <SEG>

    m_FilledPolysList = aOther.m_FilledPolysList;
    m_RawPolysList = aOther.m_RawPolysList;
    m_filledPolysHash = aOther.m_filledPolysHash;
    m_FillSegmList = aOther.m_FillSegmList;      // vector <> copy
    m_insulatedIslands = aOther.m_insulatedIslands;

    m_HatchFillTypeThickness = aOther.m_HatchFillTypeThickness;
    m_HatchFillTypeGap = aOther.m_HatchFillTypeGap;
    m_HatchFillTypeOrientation = aOther.m_HatchFillTypeOrientation;
    m_HatchFillTypeSmoothingLevel = aOther.m_HatchFillTypeSmoothingLevel;
    m_HatchFillTypeSmoothingValue = aOther.m_HatchFillTypeSmoothingValue;

    SetLayerSet( aOther.GetLayerSet() );

    return *this;
}


ZONE_CONTAINER::~ZONE_CONTAINER()
{
    delete m_Poly;
    delete m_CornerSelection;
}


void ZONE_CONTAINER::initDataFromSrcInCopyCtor( const ZONE_CONTAINER& aZone )
{
    // members are expected non initialize in this.
    // initDataFromSrcInCopyCtor() is expected to be called
    // only from a copy constructor.

    // Copy only useful EDA_ITEM flags:
    m_Flags = aZone.m_Flags;
    m_forceVisible = aZone.m_forceVisible;

    m_isKeepout = aZone.m_isKeepout;
    SetLayerSet( aZone.GetLayerSet() );

    m_Poly = new SHAPE_POLY_SET( *aZone.m_Poly );

    // For corner moving, corner index to drag, or nullptr if no selection
    m_CornerSelection = nullptr;
    m_IsFilled = aZone.m_IsFilled;
    m_ZoneClearance = aZone.m_ZoneClearance;     // clearance value
    m_ZoneMinThickness = aZone.m_ZoneMinThickness;
    m_FilledPolysUseThickness = aZone.m_FilledPolysUseThickness;
    m_FillMode = aZone.m_FillMode;               // Filling mode (segments/polygons)
    m_hv45 = aZone.m_hv45;
    m_priority = aZone.m_priority;
    m_PadConnection = aZone.m_PadConnection;
    m_ThermalReliefGap = aZone.m_ThermalReliefGap;
    m_ThermalReliefCopperBridge = aZone.m_ThermalReliefCopperBridge;
    m_FilledPolysList = aZone.m_FilledPolysList;
    m_RawPolysList = aZone.m_RawPolysList;
    m_filledPolysHash = aZone.m_filledPolysHash;
    m_FillSegmList = aZone.m_FillSegmList;      // vector <> copy
    m_insulatedIslands = aZone.m_insulatedIslands;

    m_doNotAllowCopperPour = aZone.m_doNotAllowCopperPour;
    m_doNotAllowVias = aZone.m_doNotAllowVias;
    m_doNotAllowTracks = aZone.m_doNotAllowTracks;
    m_doNotAllowPads = aZone.m_doNotAllowPads;
    m_doNotAllowFootprints = aZone.m_doNotAllowFootprints;

    m_cornerSmoothingType = aZone.m_cornerSmoothingType;
    m_cornerRadius = aZone.m_cornerRadius;

    m_hatchStyle = aZone.m_hatchStyle;
    m_hatchPitch = aZone.m_hatchPitch;
    m_HatchLines = aZone.m_HatchLines;

    m_HatchFillTypeThickness = aZone.m_HatchFillTypeThickness;
    m_HatchFillTypeGap = aZone.m_HatchFillTypeGap;
    m_HatchFillTypeOrientation = aZone.m_HatchFillTypeOrientation;
    m_HatchFillTypeSmoothingLevel = aZone.m_HatchFillTypeSmoothingLevel;
    m_HatchFillTypeSmoothingValue = aZone.m_HatchFillTypeSmoothingValue;

    SetLocalFlags( aZone.GetLocalFlags() );

    // Now zone type and layer are set, transfer net info
    // (has meaning only for copper zones)
    m_netinfo = aZone.m_netinfo;

    m_area = aZone.m_area;

    SetNeedRefill( aZone.NeedRefill() );
}


EDA_ITEM* ZONE_CONTAINER::Clone() const
{
    return new ZONE_CONTAINER( *this );
}


bool ZONE_CONTAINER::UnFill()
{
    bool change = false;

    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
    {
        change |= !pair.second.IsEmpty();
        pair.second.RemoveAllContours();
    }

    for( std::pair<PCB_LAYER_ID, ZONE_SEGMENT_FILL> pair : m_FillSegmList )
    {
        change |= !pair.second.empty();
        pair.second.clear();
    }

    m_IsFilled = false;

    return change;
}


const wxPoint ZONE_CONTAINER::GetPosition() const
{
    return (wxPoint) GetCornerPosition( 0 );
}


PCB_LAYER_ID ZONE_CONTAINER::GetLayer() const
{
    return BOARD_ITEM::GetLayer();
}


bool ZONE_CONTAINER::IsOnCopperLayer() const
{
    return ( m_layerSet & LSET::AllCuMask() ).count() > 0;
}


bool ZONE_CONTAINER::CommonLayerExists( const LSET aLayerSet ) const
{
    LSET common = GetLayerSet() & aLayerSet;

    return common.count() > 0;
}


void ZONE_CONTAINER::SetLayer( PCB_LAYER_ID aLayer )
{
    SetLayerSet( LSET( aLayer ) );

    m_Layer = aLayer;
}


void ZONE_CONTAINER::SetLayerSet( LSET aLayerSet )
{
    if( GetIsKeepout() )
    {
        // Keepouts can only exist on copper layers
        aLayerSet &= LSET::AllCuMask();
    }

    if( aLayerSet.count() == 0 )
        return;

    if( m_layerSet != aLayerSet )
    {
        SetNeedRefill( true );

        UnFill();

        for( PCB_LAYER_ID layer : aLayerSet.Seq() )
        {
            m_FillSegmList[layer]     = {};
            m_FilledPolysList[layer]  = {};
            m_RawPolysList[layer]     = {};
            m_filledPolysHash[layer]  = {};
            m_insulatedIslands[layer] = {};
        }
    }

    m_layerSet = aLayerSet;

    // Set the single layer parameter.
    // For zones that can be on many layers, this parameter does not have
    // really meaning and is a bit arbitrary if more than one layer is set.
    // But many functions are using it.
    // So we need to initialize it to a reasonable value.
    // Priority is F_Cu then B_Cu then to the first selected layer
    m_Layer = aLayerSet.Seq()[0];

    if( m_Layer != F_Cu && aLayerSet[B_Cu] )
        m_Layer = B_Cu;
}


LSET ZONE_CONTAINER::GetLayerSet() const
{
    return m_layerSet;
}


void ZONE_CONTAINER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    LSEQ layers = m_layerSet.Seq();

    for( unsigned int idx = 0; idx < layers.size(); idx++ )
        aLayers[idx] = layers[idx];

    aCount = layers.size();
}


bool ZONE_CONTAINER::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    return m_layerSet.test( aLayer );
}


const EDA_RECT ZONE_CONTAINER::GetBoundingBox() const
{
    auto bb = m_Poly->BBox();

    EDA_RECT ret( (wxPoint) bb.GetOrigin(), wxSize( bb.GetWidth(), bb.GetHeight() ) );

    return ret;
}


int ZONE_CONTAINER::GetThermalReliefGap( D_PAD* aPad ) const
{
    if( aPad == NULL || aPad->GetThermalGap() == 0 )
        return m_ThermalReliefGap;
    else
        return aPad->GetThermalGap();
}


int ZONE_CONTAINER::GetThermalReliefCopperBridge( D_PAD* aPad ) const
{
    if( aPad == NULL || aPad->GetThermalWidth() == 0 )
        return m_ThermalReliefCopperBridge;
    else
        return aPad->GetThermalWidth();
}


int ZONE_CONTAINER::GetKeepouts( std::map<int, wxString>* aSources ) const
{
    wxString source;
    int      keepouts = 0;

    auto     setFlag = [&]( int aFlag )
                       {
                           keepouts |= aFlag;

                           if( aSources )
                               (*aSources)[ aFlag ] = source;
                       };

    if( m_isKeepout )
    {
        if( aSources )
            source = _( "zone properties" );

        if( m_doNotAllowTracks )
            setFlag( DISALLOW_TRACKS );

        if( m_doNotAllowVias )
            setFlag( DISALLOW_VIAS );

        if( m_doNotAllowPads )
            setFlag( DISALLOW_PADS );

        if( m_doNotAllowFootprints )
            setFlag( DISALLOW_FOOTPRINTS );

        if( m_doNotAllowCopperPour )
            setFlag( DISALLOW_ZONES );
    }

    DRC_RULE* rule = GetRule( this, nullptr, DISALLOW_CONSTRAINT );

    if( rule )
    {
        if( aSources )
            source = wxString::Format( _( "'%s' rule" ), rule->m_Name );

        if( ( rule->m_DisallowFlags & DISALLOW_VIAS ) > 0 )
            setFlag( DISALLOW_VIAS );

        if( ( rule->m_DisallowFlags & DISALLOW_MICRO_VIAS ) > 0 )
            setFlag( DISALLOW_MICRO_VIAS );

        if( ( rule->m_DisallowFlags & DISALLOW_BB_VIAS ) > 0 )
            setFlag( DISALLOW_BB_VIAS );

        if( ( rule->m_DisallowFlags & DISALLOW_TRACKS ) > 0 )
            setFlag( DISALLOW_TRACKS );

        if( ( rule->m_DisallowFlags & DISALLOW_PADS ) > 0 )
            setFlag( DISALLOW_PADS );

        if( ( rule->m_DisallowFlags & DISALLOW_ZONES ) > 0 )
            setFlag( DISALLOW_ZONES );

        if( ( rule->m_DisallowFlags & DISALLOW_TEXTS ) > 0 )
            setFlag( DISALLOW_TEXTS );

        if( ( rule->m_DisallowFlags & DISALLOW_GRAPHICS ) > 0 )
            setFlag( DISALLOW_GRAPHICS );

        if( ( rule->m_DisallowFlags & DISALLOW_HOLES ) > 0 )
            setFlag( DISALLOW_HOLES );

        if( ( rule->m_DisallowFlags & DISALLOW_FOOTPRINTS ) > 0 )
            setFlag( DISALLOW_FOOTPRINTS );
    }

    return keepouts;
}


void ZONE_CONTAINER::SetCornerRadius( unsigned int aRadius )
{
    if( m_cornerRadius != aRadius )
        SetNeedRefill( true );

    m_cornerRadius = aRadius;
}


bool ZONE_CONTAINER::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Normally accuracy is zoom-relative, but for the generic HitTest we just use
    // a fixed (small) value.
    int accuracy = std::max( aAccuracy, Millimeter2iu( 0.1 ) );

    return HitTestForCorner( aPosition, accuracy * 2 ) || HitTestForEdge( aPosition, accuracy );
}


void ZONE_CONTAINER::SetSelectedCorner( const wxPoint& aPosition, int aAccuracy )
{
    SHAPE_POLY_SET::VERTEX_INDEX corner;

    // If there is some corner to be selected, assign it to m_CornerSelection
    if( HitTestForCorner( aPosition, aAccuracy * 2, corner )
        || HitTestForEdge( aPosition, aAccuracy, corner ) )
    {
        if( m_CornerSelection == nullptr )
            m_CornerSelection = new SHAPE_POLY_SET::VERTEX_INDEX;

        *m_CornerSelection = corner;
    }
}

bool ZONE_CONTAINER::HitTestForCorner( const wxPoint& refPos, int aAccuracy,
                                       SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const
{
    return m_Poly->CollideVertex( VECTOR2I( refPos ), aCornerHit, aAccuracy );
}


bool ZONE_CONTAINER::HitTestForCorner( const wxPoint& refPos, int aAccuracy ) const
{
    SHAPE_POLY_SET::VERTEX_INDEX dummy;
    return HitTestForCorner( refPos, aAccuracy, dummy );
}


bool ZONE_CONTAINER::HitTestForEdge( const wxPoint& refPos, int aAccuracy,
                                     SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const
{
    return m_Poly->CollideEdge( VECTOR2I( refPos ), aCornerHit, aAccuracy );
}


bool ZONE_CONTAINER::HitTestForEdge( const wxPoint& refPos, int aAccuracy ) const
{
    SHAPE_POLY_SET::VERTEX_INDEX dummy;
    return HitTestForEdge( refPos, aAccuracy, dummy );
}


bool ZONE_CONTAINER::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    // Calculate bounding box for zone
    EDA_RECT bbox = GetBoundingBox();
    bbox.Normalize();

    EDA_RECT arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    if( aContained )
    {
         return arect.Contains( bbox );
    }
    else    // Test for intersection between aBox and the polygon
            // For a polygon, using its bounding box has no sense here
    {
        // Fast test: if aBox is outside the polygon bounding box, rectangles cannot intersect
        if( !arect.Intersects( bbox ) )
            return false;

        int count = m_Poly->TotalVertices();

        for( int ii = 0; ii < count; ii++ )
        {
            auto vertex = m_Poly->CVertex( ii );
            auto vertexNext = m_Poly->CVertex( ( ii + 1 ) % count );

            // Test if the point is within the rect
            if( arect.Contains( ( wxPoint ) vertex ) )
            {
                return true;
            }

            // Test if this edge intersects the rect
            if( arect.Intersects( ( wxPoint ) vertex, ( wxPoint ) vertexNext ) )
            {
                return true;
            }
        }

        return false;
    }
}


int ZONE_CONTAINER::GetLocalClearance( wxString* aSource ) const
{
    if( m_isKeepout )
        return 0;

    if( aSource )
        *aSource = _( "zone" );

    return m_ZoneClearance;
}


bool ZONE_CONTAINER::HitTestFilledArea( PCB_LAYER_ID aLayer, const wxPoint& aRefPos ) const
{
    if( !m_FilledPolysList.count( aLayer ) )
        return false;

    return m_FilledPolysList.at( aLayer ).Contains( VECTOR2I( aRefPos.x, aRefPos.y ) );
}


bool ZONE_CONTAINER::HitTestCutout( const VECTOR2I& aRefPos, int* aOutlineIdx, int* aHoleIdx ) const
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


void ZONE_CONTAINER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();
    wxString  msg, msg2;

    if( GetIsKeepout() )
        msg = _( "Keepout Area" );
    else if( IsOnCopperLayer() )
        msg = _( "Copper Zone" );
    else
        msg = _( "Non-copper Zone" );

    // Display Cutout instead of Outline for holes inside a zone
    // i.e. when num contour !=0
    // Check whether the selected corner is in a hole; i.e., in any contour but the first one.
    if( m_CornerSelection != nullptr && m_CornerSelection->m_contour > 0 )
        msg << wxT( " " ) << _( "Cutout" );

    aList.emplace_back( _( "Type" ), msg, DARKCYAN );

    if( GetIsKeepout() )
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

        aList.emplace_back( MSG_PANEL_ITEM( _( "Keepout" ), msg, RED ) );
    }
    else if( IsOnCopperLayer() )
    {
        if( GetNetCode() >= 0 )
        {
            NETINFO_ITEM* net = GetNet();
            NETCLASS*     netclass = nullptr;

            if( net )
            {
                if( net->GetNet() )
                    netclass = GetNetClass();
                else
                    netclass = GetBoard()->GetDesignSettings().GetDefault();

                msg = UnescapeString( net->GetNetname() );
            }
            else
            {
                msg = wxT( "<no name>" );
            }

            aList.emplace_back( _( "Net" ), msg, RED );

            if( netclass )
                aList.emplace_back( _( "NetClass" ), netclass->GetName(), DARKMAGENTA );
        }

        // Display priority level
        msg.Printf( wxT( "%d" ), GetPriority() );
        aList.emplace_back( _( "Priority" ), msg, BLUE );
    }

    wxString layerDesc;
    int count = 0;

    for( PCB_LAYER_ID layer : m_layerSet.Seq() )
    {
        if( count == 0 )
            layerDesc = GetBoard()->GetLayerName( layer );

        count++;
    }

    if( count > 1 )
        layerDesc.Printf( _( "%s and %d more" ), layerDesc, count - 1 );

    aList.emplace_back( _( "Layer" ), layerDesc, DARKGREEN );

    if( !m_zoneName.empty() )
        aList.emplace_back( _( "Name" ), m_zoneName, DARKMAGENTA );

    switch( m_FillMode )
    {
    case ZONE_FILL_MODE::POLYGONS:      msg = _( "Solid" ); break;
    case ZONE_FILL_MODE::HATCH_PATTERN: msg = _( "Hatched" ); break;
    default:                            msg = _( "Unknown" ); break;
    }

    aList.emplace_back( _( "Fill Mode" ), msg, BROWN );

    msg = MessageTextFromValue( units, m_area, false, EDA_DATA_TYPE::AREA );
    aList.emplace_back( _( "Filled Area" ), msg, BLUE );

    wxString source;
    int      clearance = GetClearance( nullptr, &source );

    msg.Printf( _( "Min Clearance: %s" ), MessageTextFromValue( units, clearance, true ) );
    msg2.Printf( _( "(from %s)" ), source );
    aList.emplace_back( msg, msg2, BLACK );

    // Useful for statistics, especially when zones are complex the number of hatches
    // and filled polygons can explain the display and DRC calculation time:
    msg.Printf( wxT( "%d" ), (int) m_HatchLines.size() );
    aList.emplace_back( MSG_PANEL_ITEM( _( "Hatch Lines" ), msg, BLUE ) );

    PCB_LAYER_ID layer = m_Layer;

    // NOTE: This brings in dependence on PCB_EDIT_FRAME to the qa tests, which isn't ideal.
    // TODO: Figure out a way for items to know the active layer without the whole edit frame?
#if 0
    if( PCB_EDIT_FRAME* pcbframe = dynamic_cast<PCB_EDIT_FRAME*>( aFrame ) )
        if( m_FilledPolysList.count( pcbframe->GetActiveLayer() ) )
            layer = pcbframe->GetActiveLayer();
#endif

    if( !m_FilledPolysList.at( layer ).IsEmpty() )
    {
        msg.Printf( wxT( "%d" ), m_FilledPolysList.at( layer ).TotalVertices() );
        aList.emplace_back( MSG_PANEL_ITEM( _( "Corner Count" ), msg, BLUE ) );
    }
}


/* Geometric transforms: */

void ZONE_CONTAINER::Move( const wxPoint& offset )
{
    /* move outlines */
    m_Poly->Move( offset );

    Hatch();

    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
        pair.second.Move( offset );

    for( std::pair<PCB_LAYER_ID, ZONE_SEGMENT_FILL> pair : m_FillSegmList )
    {
        for( SEG& seg : pair.second )
        {
            seg.A += VECTOR2I( offset );
            seg.B += VECTOR2I( offset );
        }
    }
}


void ZONE_CONTAINER::MoveEdge( const wxPoint& offset, int aEdge )
{
    int next_corner;

    if( m_Poly->GetNeighbourIndexes( aEdge, nullptr, &next_corner ) )
    {
        m_Poly->SetVertex( aEdge, m_Poly->CVertex( aEdge ) + VECTOR2I( offset ) );
        m_Poly->SetVertex( next_corner, m_Poly->CVertex( next_corner ) + VECTOR2I( offset ) );
        Hatch();

        SetNeedRefill( true );
    }
}


void ZONE_CONTAINER::Rotate( const wxPoint& centre, double angle )
{
    wxPoint pos;

    angle = -DECIDEG2RAD( angle );

    m_Poly->Rotate( angle, VECTOR2I( centre ) );
    Hatch();

    /* rotate filled areas: */
    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
        pair.second.Rotate( angle, VECTOR2I( centre ) );

    for( std::pair<PCB_LAYER_ID, ZONE_SEGMENT_FILL> pair : m_FillSegmList )
    {
        for( SEG& seg : pair.second )
        {
            wxPoint a( seg.A );
            RotatePoint( &a, centre, angle );
            seg.A = a;
            wxPoint b( seg.B );
            RotatePoint( &b, centre, angle );
            seg.B = a;
        }
    }
}


void ZONE_CONTAINER::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    Mirror( aCentre, aFlipLeftRight );
    int copperLayerCount = GetBoard()->GetCopperLayerCount();

    if( GetIsKeepout() )
        SetLayerSet( FlipLayerMask( GetLayerSet(), copperLayerCount ) );
    else
        SetLayer( FlipLayer( GetLayer(), copperLayerCount ) );
}


void ZONE_CONTAINER::Mirror( const wxPoint& aMirrorRef, bool aMirrorLeftRight )
{
    // ZONE_CONTAINERs mirror about the x-axis (why?!?)
    m_Poly->Mirror( aMirrorLeftRight, !aMirrorLeftRight, VECTOR2I( aMirrorRef ) );

    Hatch();

    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
        pair.second.Mirror( aMirrorLeftRight, !aMirrorLeftRight, VECTOR2I( aMirrorRef ) );

    for( std::pair<PCB_LAYER_ID, ZONE_SEGMENT_FILL> pair : m_FillSegmList )
    {
        for( SEG& seg : pair.second )
        {
            if( aMirrorLeftRight )
            {
                MIRROR( seg.A.x, aMirrorRef.x );
                MIRROR( seg.B.x, aMirrorRef.x );
            }
            else
            {
                MIRROR( seg.A.y, aMirrorRef.y );
                MIRROR( seg.B.y, aMirrorRef.y );
            }
        }
    }
}


ZONE_CONNECTION ZONE_CONTAINER::GetPadConnection( D_PAD* aPad ) const
{
    if( aPad == NULL || aPad->GetEffectiveZoneConnection() == ZONE_CONNECTION::INHERITED )
        return m_PadConnection;
    else
        return aPad->GetEffectiveZoneConnection();
}


void ZONE_CONTAINER::RemoveCutout( int aOutlineIdx, int aHoleIdx )
{
    // Ensure the requested cutout is valid
    if( m_Poly->OutlineCount() < aOutlineIdx || m_Poly->HoleCount( aOutlineIdx ) < aHoleIdx )
        return;

    SHAPE_POLY_SET cutPoly( m_Poly->Hole( aOutlineIdx, aHoleIdx ) );

    // Add the cutout back to the zone
    m_Poly->BooleanAdd( cutPoly, SHAPE_POLY_SET::PM_FAST );

    SetNeedRefill( true );
}


void ZONE_CONTAINER::AddPolygon( const SHAPE_LINE_CHAIN& aPolygon )
{
    wxASSERT( aPolygon.IsClosed() );

    // Add the outline as a new polygon in the polygon set
    if( m_Poly->OutlineCount() == 0 )
        m_Poly->AddOutline( aPolygon );
    else
        m_Poly->AddHole( aPolygon );

    SetNeedRefill( true );
}


void ZONE_CONTAINER::AddPolygon( std::vector< wxPoint >& aPolygon )
{
    if( aPolygon.empty() )
        return;

    SHAPE_LINE_CHAIN outline;

    // Create an outline and populate it with the points of aPolygon
    for( const wxPoint& pt : aPolygon)
        outline.Append( pt );

    outline.SetClosed( true );

    AddPolygon( outline );
}


bool ZONE_CONTAINER::AppendCorner( wxPoint aPosition, int aHoleIdx, bool aAllowDuplication )
{
    // Ensure the main outline exists:
    if( m_Poly->OutlineCount() == 0 )
        m_Poly->NewOutline();

    // If aHoleIdx >= 0, the corner musty be added to the hole, index aHoleIdx.
    // (remember: the index of the first hole is 0)
    // Return error if if does dot exist.
    if( aHoleIdx >= m_Poly->HoleCount( 0 ) )
        return false;

    m_Poly->Append( aPosition.x, aPosition.y, -1, aHoleIdx, aAllowDuplication );

    SetNeedRefill( true );

    return true;
}


wxString ZONE_CONTAINER::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    wxString text;

    // Check whether the selected contour is a hole (contour index > 0)
    if( m_CornerSelection != nullptr &&  m_CornerSelection->m_contour > 0 )
        text << wxT( " " ) << _( "(Cutout)" );

    if( GetIsKeepout() )
        text << wxT( " " ) << _( "(Keepout)" );
    else
        text << GetNetnameMsg();

    wxString layerDesc;
    int count = 0;

    for( PCB_LAYER_ID layer : m_layerSet.Seq() )
    {
        if( count == 0 )
            layerDesc = GetBoard()->GetLayerName( layer );

        count++;
    }

    if( count > 1 )
        layerDesc.Printf( _( "%s and %d more" ), layerDesc, count - 1 );

    return wxString::Format( _( "Zone Outline %s on %s" ), text, layerDesc );
}


int ZONE_CONTAINER::GetHatchPitch() const
{
    return m_hatchPitch;
}


void ZONE_CONTAINER::SetHatch( ZONE_HATCH_STYLE aHatchStyle, int aHatchPitch, bool aRebuildHatch )
{
    SetHatchPitch( aHatchPitch );
    m_hatchStyle = aHatchStyle;

    if( aRebuildHatch )
        Hatch();
}


void ZONE_CONTAINER::SetHatchPitch( int aPitch )
{
    m_hatchPitch = aPitch;
}


void ZONE_CONTAINER::UnHatch()
{
    m_HatchLines.clear();
}


// Creates hatch lines inside the outline of the complex polygon
// sort function used in ::Hatch to sort points by descending wxPoint.x values
bool sortEndsByDescendingX( const VECTOR2I& ref, const VECTOR2I& tst )
{
    return tst.x < ref.x;
}


void ZONE_CONTAINER::Hatch()
{
    UnHatch();

    if( m_hatchStyle == ZONE_HATCH_STYLE::NO_HATCH || m_hatchPitch == 0 || m_Poly->IsEmpty() )
        return;

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

    if( m_hatchStyle == ZONE_HATCH_STYLE::DIAGONAL_EDGE )
        spacing = m_hatchPitch;
    else
        spacing = m_hatchPitch * 2;

    // set the "length" of hatch lines (the length on horizontal axis)
    int  hatch_line_len = m_hatchPitch;

    // To have a better look, give a slope depending on the layer
    LAYER_NUM layer = GetLayer();
    int     slope_flag = (layer & 1) ? 1 : -1;  // 1 or -1
    double  slope = 0.707106 * slope_flag;      // 45 degrees slope
    int     max_a, min_a;

    if( slope_flag == 1 )
    {
        max_a   = KiROUND( max_y - slope * min_x );
        min_a   = KiROUND( min_y - slope * max_x );
    }
    else
    {
        max_a   = KiROUND( max_y - slope * max_x );
        min_a   = KiROUND( min_y - slope * min_x );
    }

    min_a = (min_a / spacing) * spacing;

    // calculate an offset depending on layer number,
    // for a better look of hatches on a multilayer board
    int offset = (layer * 7) / 8;
    min_a += offset;

    // loop through hatch lines
    #define MAXPTS 200      // Usually we store only few values per one hatch line
                            // depending on the complexity of the zone outline

    static std::vector<VECTOR2I> pointbuffer;
    pointbuffer.clear();
    pointbuffer.reserve( MAXPTS + 2 );

    for( int a = min_a; a < max_a; a += spacing )
    {
        // get intersection points for this hatch line

        // Note: because we should have an even number of intersections with the
        // current hatch line and the zone outline (a closed polygon,
        // or a set of closed polygons), if an odd count is found
        // we skip this line (should not occur)
        pointbuffer.clear();

        // Iterate through all vertices
        for( auto iterator = m_Poly->IterateSegmentsWithHoles(); iterator; iterator++ )
        {
            double  x, y, x2, y2;
            int     ok;

            SEG segment = *iterator;

            ok = FindLineSegmentIntersection( a, slope,
                                              segment.A.x, segment.A.y,
                                              segment.B.x, segment.B.y,
                                              &x, &y, &x2, &y2 );

              if( ok )
              {
                  VECTOR2I point( KiROUND( x ), KiROUND( y ) );
                  pointbuffer.push_back( point );
              }

              if( ok == 2 )
              {
                  VECTOR2I point( KiROUND( x2 ), KiROUND( y2 ) );
                  pointbuffer.push_back( point );
              }

              if( pointbuffer.size() >= MAXPTS )    // overflow
              {
                  wxASSERT( 0 );
                  break;
              }
        }

        // ensure we have found an even intersection points count
        // because intersections are the ends of segments
        // inside the polygon(s) and a segment has 2 ends.
        // if not, this is a strange case (a bug ?) so skip this hatch
        if( pointbuffer.size() % 2 != 0 )
            continue;

        // sort points in order of descending x (if more than 2) to
        // ensure the starting point and the ending point of the same segment
        // are stored one just after the other.
        if( pointbuffer.size() > 2 )
            sort( pointbuffer.begin(), pointbuffer.end(), sortEndsByDescendingX );

        // creates lines or short segments inside the complex polygon
        for( unsigned ip = 0; ip < pointbuffer.size(); ip += 2 )
        {
            int dx = pointbuffer[ip + 1].x - pointbuffer[ip].x;

            // Push only one line for diagonal hatch,
            // or for small lines < twice the line length
            // else push 2 small lines
            if( m_hatchStyle == ZONE_HATCH_STYLE::DIAGONAL_FULL
                    || std::abs( dx ) < 2 * hatch_line_len )
            {
                m_HatchLines.emplace_back( SEG( pointbuffer[ip], pointbuffer[ip + 1] ) );
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

                m_HatchLines.emplace_back( SEG( pointbuffer[ip].x, pointbuffer[ip].y, x1, y1 ) );

                m_HatchLines.emplace_back( SEG( pointbuffer[ip+1].x, pointbuffer[ip+1].y, x2, y2 ) );
            }
        }
    }
}


int ZONE_CONTAINER::GetDefaultHatchPitch()
{
    return Mils2iu( 20 );
}


BITMAP_DEF ZONE_CONTAINER::GetMenuImage() const
{
    return add_zone_xpm;
}


void ZONE_CONTAINER::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_ZONE_AREA_T );

    std::swap( *((ZONE_CONTAINER*) this), *((ZONE_CONTAINER*) aImage) );
}


void ZONE_CONTAINER::CacheTriangulation()
{
    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
        pair.second.CacheTriangulation();
}


bool ZONE_CONTAINER::IsIsland( PCB_LAYER_ID aLayer, int aPolyIdx )
{
    if( GetNetCode() < 1 )
        return true;

    if( !m_insulatedIslands.count( aLayer ) )
        return false;

    return m_insulatedIslands.at( aLayer ).count( aPolyIdx );
}


/*
 * Some intersecting zones, despite being on the same layer with the same net, cannot be
 * merged due to other parameters such as fillet radius.  The copper pour will end up
 * effectively merged though, so we want to keep the corners of such intersections sharp.
 */
void ZONE_CONTAINER::GetColinearCorners( BOARD* aBoard, std::set<VECTOR2I>& aCorners )
{
    int epsilon = Millimeter2iu( 0.001 );

    // Things get messy when zone of different nets intersect.  To do it right we'd need to
    // run our colinear test with the final filled regions rather than the outline regions.
    // However, since there's no order dependance the only way to do that is to iterate
    // through successive zone fills until the results are no longer changing -- and that's
    // not going to happen.  So we punt and ignore any "messy" corners.
    std::set<VECTOR2I> colinearCorners;
    std::set<VECTOR2I> messyCorners;

    for( ZONE_CONTAINER* candidate : aBoard->Zones() )
    {
        if( candidate == this )
            continue;

        if( candidate->GetLayerSet() != GetLayerSet() )
            continue;

        if( candidate->GetIsKeepout() != GetIsKeepout() )
            continue;

        for( auto iter = m_Poly->CIterate(); iter; iter++ )
        {
            if( candidate->m_Poly->Collide( iter.Get(), epsilon ) )
            {
                if( candidate->GetNetCode() == GetNetCode() )
                    colinearCorners.insert( VECTOR2I( iter.Get() ) );
                else
                    messyCorners.insert( VECTOR2I( iter.Get() ) );
            }
        }
    }

    for( VECTOR2I corner : colinearCorners )
    {
        if( messyCorners.count( corner ) == 0 )
            aCorners.insert( corner );
    }
}


bool ZONE_CONTAINER::BuildSmoothedPoly( SHAPE_POLY_SET& aSmoothedPoly,
                                        std::set<VECTOR2I>* aPreserveCorners ) const
{
    if( GetNumCorners() <= 2 )  // malformed zone. polygon calculations do not like it ...
        return false;

    // Make a smoothed polygon out of the user-drawn polygon if required
    switch( m_cornerSmoothingType )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        aSmoothedPoly = m_Poly->Chamfer( m_cornerRadius, aPreserveCorners );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
    {
        auto board = GetBoard();
        int maxError = ARC_HIGH_DEF;

        if( board )
            maxError = board->GetDesignSettings().m_MaxError;

        aSmoothedPoly = m_Poly->Fillet( m_cornerRadius, maxError, aPreserveCorners );
        break;
    }
    default:
        // Acute angles between adjacent edges can create issues in calculations,
        // in inflate/deflate outlines transforms, especially when the angle is very small.
        // We can avoid issues by creating a very small chamfer which remove acute angles,
        // or left it without chamfer and use only CPOLYGONS_LIST::InflateOutline to create
        // clearance areas
        aSmoothedPoly = m_Poly->Chamfer( Millimeter2iu( 0.0 ), aPreserveCorners );
        break;
    }

    return true;
};


double ZONE_CONTAINER::CalculateFilledArea()
{
    m_area = 0.0;

    // Iterate over each outline polygon in the zone and then iterate over
    // each hole it has to compute the total area.
    for( std::pair<PCB_LAYER_ID, SHAPE_POLY_SET> pair : m_FilledPolysList )
    {
        SHAPE_POLY_SET& poly = pair.second;

        for( int i = 0; i < poly.OutlineCount(); i++ )
        {
            m_area += poly.Outline( i ).Area();

            for( int j = 0; j < poly.HoleCount( i ); j++ )
            {
                m_area -= poly.Hole( i, j ).Area();
            }
        }
    }

    return m_area;
}


/* Function TransformOutlinesShapeWithClearanceToPolygon
 * Convert the zone filled areas polygons to polygons
 * inflated (optional) by max( aClearanceValue, the zone clearance)
 * and copy them in aCornerBuffer
 * @param aClearance       the clearance around outlines
 * @param aPreserveCorners an optional set of corners which should not be chamfered/filleted
 */
void ZONE_CONTAINER::TransformOutlinesShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
        int aClearance, std::set<VECTOR2I>* aPreserveCorners ) const
{
    // Creates the zone outline polygon (with holes if any)
    SHAPE_POLY_SET polybuffer;
    BuildSmoothedPoly( polybuffer, aPreserveCorners );

    // Calculate the polygon with clearance
    // holes are linked to the main outline, so only one polygon is created.
    if( aClearance )
    {
        BOARD* board = GetBoard();
        int maxError = ARC_HIGH_DEF;

        if( board )
            maxError = board->GetDesignSettings().m_MaxError;

        int segCount = std::max( GetArcToSegmentCount( aClearance, maxError, 360.0 ), 3 );
        polybuffer.Inflate( aClearance, segCount );
    }

    polybuffer.Fracture( SHAPE_POLY_SET::PM_FAST );
    aCornerBuffer.Append( polybuffer );
}

//
/********* MODULE_ZONE_CONTAINER **************/
//
MODULE_ZONE_CONTAINER::MODULE_ZONE_CONTAINER( BOARD_ITEM_CONTAINER* aParent ) :
                        ZONE_CONTAINER( aParent, true )
{
    // in a footprint, net classes are not managed.
    // so set the net to NETINFO_LIST::ORPHANED_ITEM
    SetNetCode( -1, true );
}


MODULE_ZONE_CONTAINER::MODULE_ZONE_CONTAINER( const MODULE_ZONE_CONTAINER& aZone )
        : ZONE_CONTAINER( aZone.GetParent(), true )
{
    initDataFromSrcInCopyCtor( aZone );
}


MODULE_ZONE_CONTAINER& MODULE_ZONE_CONTAINER::operator=( const MODULE_ZONE_CONTAINER& aOther )
{
    ZONE_CONTAINER::operator=( aOther );
    return *this;
}


EDA_ITEM* MODULE_ZONE_CONTAINER::Clone() const
{
    return new MODULE_ZONE_CONTAINER( *this );
}


unsigned int MODULE_ZONE_CONTAINER::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    const int HIDE = std::numeric_limits<unsigned int>::max();

    if( !aView )
        return 0;

    bool flipped = GetParent() && GetParent()->GetLayer() == B_Cu;

    // Handle Render tab switches
    if( !flipped && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( flipped && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0;
}
