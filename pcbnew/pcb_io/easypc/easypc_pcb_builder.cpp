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

#include <easypc/easypc_pcb_builder.h>
#include <easypc/easypc_pcb_items.h>

#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_classes_root.h>
#include <io/easypc/easypc_design_settings.h>
#include <io/easypc/easypc_document.h>
#include <io/easypc/easypc_text_metrics.h>

#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <footprint.h>
#include <ki_exception.h>
#include <lib_id.h>
#include <netclass.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <project/net_settings.h>
#include <reporter.h>
#include <string_utils.h>
#include <title_block.h>
#include <zone.h>

#include <limits>
#include <optional>
#include <set>


using namespace EASYPC;


namespace EASYPC_PCB
{

namespace
{

    /// Polygon error for power planes, below the plot's resolution
    constexpr int PLANE_MAX_ERROR = 1000;

    /// Plane fills keep every sliver of the plotted copper
    constexpr int PLANE_MIN_THICKNESS = 10000;

    /// The copper item kinds of the spacing table that KiCad checks with a clearance constraint
    struct COPPER_KIND
    {
        int         Kind;
        const char* Name;
        const char* Type; ///< the KiCad item type it becomes; free and poured copper become graphics and zones
    };

    constexpr COPPER_KIND COPPER_KINDS[] = { { SPACING_TRACK, "track", "'Track'" },
                                             { SPACING_PAD, "pad", "'Pad'" },
                                             { SPACING_VIA, "via", "'Via'" },
                                             { SPACING_SHAPE, "shape", "'Graphic'" },
                                             { SPACING_TEXT, "text", "'Text'" } };


    wxString kindTest( const COPPER_KIND& aKind, const wxString& aItem )
    {
        wxString test = aItem + wxS( ".Type == " ) + aKind.Type;

        if( aKind.Kind == SPACING_SHAPE )
            return wxS( "(" ) + test + wxS( " || " ) + aItem + wxS( ".Type == 'Zone')" );

        return test;
    }


    /// One design unit is 0.0001 mm, so four decimals are exact and integer formatting avoids the user's locale
    wxString mm( int64_t aDsu )
    {
        uint64_t magnitude = aDsu < 0 ? 0 - static_cast<uint64_t>( aDsu ) : static_cast<uint64_t>( aDsu );

        return wxString::Format( wxS( "%s%llu.%04llumm" ), aDsu < 0 ? wxS( "-" ) : wxS( "" ),
                                 static_cast<unsigned long long>( magnitude / 10000 ),
                                 static_cast<unsigned long long>( magnitude % 10000 ) );
    }


    /// An expression string literal, whose only escape is \' so a final backslash cannot be held
    wxString exprLiteral( const wxString& aText )
    {
        wxString text = aText;
        text.Replace( wxS( "'" ), wxS( "\\'" ) );
        return wxS( "'" ) + text + wxS( "'" );
    }


    /// A quoted rules file string, DSNLEXER decoding \\ and \"
    wxString quoteSexpr( const wxString& aText )
    {
        wxString text = aText;
        text.Replace( wxS( "\\" ), wxS( "\\\\" ) );
        text.Replace( wxS( "\"" ), wxS( "\\\"" ) );
        return wxS( "\"" ) + text + wxS( "\"" );
    }


    /// One table's value, keyed (a << 16) | b with a bidirectional table storing the larger kind first, -1 when absent
    int32_t spacingValue( const SOURCE_SPACINGS& aSpacings, int aTypeA, int aTypeB )
    {
        if( aSpacings.Bidirectional && aTypeB > aTypeA )
            std::swap( aTypeA, aTypeB );

        auto it = aSpacings.Values.find(
                static_cast<int32_t>( ( static_cast<uint32_t>( aTypeA ) << 16 ) | ( aTypeB & 0xffff ) ) );
        return it == aSpacings.Values.end() ? -1 : it->second;
    }


    class PCB_BUILDER
    {
    public:
        PCB_BUILDER( const DESIGN_DOCUMENT& aDocument, BOARD& aBoard, REPORTER* aReporter );

        void Build();

        std::unique_ptr<LAYER_MAPPER> m_layers;
        wxString                      m_customRules;

    private:
        void buildLayers();
        void buildRules();
        void buildNetClasses();
        void buildFootprints();
        void buildFreeItems();
        void buildTracksAndVias();
        void buildAreas();
        void buildPlanes();
        void buildGroups();

        /// What the negative plot of the plane on aLayer clears
        SHAPE_POLY_SET planeClearances( const SOURCE_LAYER& aLayer, PCB_LAYER_ID aKiCadLayer,
                                        NETINFO_ITEM* aNet ) const;

        /// Whether buildAreas() turns aArea into a filled zone with its stored pour
        bool becomesZone( const SOURCE_AREA& aArea ) const;

        void placeValueTexts( FOOTPRINT& aFootprint, const SOURCE_COMPONENT_INSTANCE& aInstance,
                              const SOURCE_SYMBOL_INSTANCE& aSymbolInstance );

        /// The design table value for a pair, -1 when the pair is not checked
        int32_t designValue( int aTypeA, int aTypeB ) const;

        int32_t shapeSpacing( int aKind ) const
        {
            return m_spacings ? spacingValue( *m_spacings, SPACING_SHAPE, aKind ) : -1;
        }

        void addRule( const wxString& aName, const wxString& aConstraint, const wxString& aCondition );

        NETINFO_ITEM* netOfNode( const OBJECT* aNode ) const;

        const DESIGN_DOCUMENT&                            m_doc;
        const SOURCE_DESIGN*                              m_design;
        const SOURCE_SPACINGS*                            m_spacings;
        BOARD&                                            m_board;
        REPORTER*                                         m_reporter;
        FRAME                                             m_frame;
        std::map<const OBJECT*, NETINFO_ITEM*>            m_nets;                          ///< source to KiCad net
        std::map<const OBJECT*, std::vector<BOARD_ITEM*>> m_created;                       ///< for groups
        bool                                              m_viaMask[2] = { false, false }; ///< front, back
    };

} // namespace


PCB_BUILDER::PCB_BUILDER( const DESIGN_DOCUMENT& aDocument, BOARD& aBoard, REPORTER* aReporter ) :
        m_doc( aDocument ),
        m_design( dynamic_cast<const SOURCE_DESIGN*>( aDocument.Design.get() ) ),
        m_spacings( m_design ? dynamic_cast<const SOURCE_SPACINGS*>( m_design->Spacings ) : nullptr ),
        m_board( aBoard ),
        m_reporter( aReporter )
{
    if( !m_design )
        THROW_IO_ERROR( _( "Easy-PC design has no design root." ) );

    m_layers = std::make_unique<LAYER_MAPPER>( TypedItems<const SOURCE_LAYER>( m_design->Layers ) );

    // Put the board's top left corner one inch in from the KiCad origin
    bool any = false;

    for( const SOURCE_BOARD* board : TypedItems<const SOURCE_BOARD>( m_design->Boards ) )
    {
        const SOURCE_SHAPE* shape = dynamic_cast<const SOURCE_SHAPE*>( board->Shape );

        if( !shape )
            continue;

        for( const OBJECT* obj : shape->Segments )
        {
            if( const SOURCE_SEGMENT* seg = dynamic_cast<const SOURCE_SEGMENT*>( obj ) )
            {
                m_frame.Origin.X = any ? std::min( m_frame.Origin.X, seg->X ) : seg->X;
                m_frame.Origin.Y = any ? std::max( m_frame.Origin.Y, seg->Y ) : seg->Y;
                any = true;
            }
        }
    }

    m_frame.Offset = VECTOR2I( 25400000, 25400000 );
}


void PCB_BUILDER::Build()
{
    buildLayers();

    for( const SOURCE_NET* net : TypedItems<const SOURCE_NET>( m_design->Nets ) )
    {
        // The schematic import names the net the same way, so an update from the schematic keeps it
        NETINFO_ITEM* info = new NETINFO_ITEM( &m_board, ToKiCadMarkup( net->Name ) );
        m_board.Add( info );
        m_nets[net] = info;
    }

    buildRules();

    for( const SOURCE_BOARD* board : TypedItems<const SOURCE_BOARD>( m_design->Boards ) )
        AddShapeItem( m_board, *board, m_frame, *m_layers, Edge_Cuts );

    buildFootprints();
    buildFreeItems();
    buildTracksAndVias();
    buildAreas();
    buildPlanes();
    buildGroups();

    TITLE_BLOCK titleBlock;
    EASYPC::FillTitleBlock( m_doc, titleBlock );
    m_board.SetTitleBlock( titleBlock );

    // Items sit right of and below the page origin by the frame offset, so the page must reach back to it
    BOX2L extents( VECTOR2L( 0, 0 ) );
    extents.Merge( VECTOR2L( m_board.ComputeBoundingBox( false ).GetEnd() ) );
    m_board.SetPageSettings( EASYPC::PageForExtents( extents ) );
}


int32_t PCB_BUILDER::designValue( int aTypeA, int aTypeB ) const
{
    if( !m_spacings || m_spacings->Bidirectional )
        return m_spacings ? spacingValue( *m_spacings, aTypeA, aTypeB ) : -1;

    // A one-way table depends on which item the caller names first; KiCad pairs are unordered
    return std::max( spacingValue( *m_spacings, aTypeA, aTypeB ), spacingValue( *m_spacings, aTypeB, aTypeA ) );
}


void PCB_BUILDER::addRule( const wxString& aName, const wxString& aConstraint, const wxString& aCondition )
{
    if( m_customRules.IsEmpty() )
        m_customRules = wxS( "(version 1)\n" );

    m_customRules += wxS( "\n(rule " ) + quoteSexpr( aName ) + wxS( "\n    " ) + aConstraint + wxS( "\n" );
    m_customRules += wxS( "    (condition " ) + quoteSexpr( aCondition ) + wxS( "))\n" );
}


void PCB_BUILDER::buildRules()
{
    BOARD_DESIGN_SETTINGS& bds = m_board.GetDesignSettings();

    // BOARD::SetProject keeps importer settings and net classes only when they are flagged as loaded
    m_board.m_LegacyDesignSettingsLoaded = true;
    m_board.m_LegacyNetclassesLoaded = true;

    // Easy-PC has no board-wide clearance floor; every pair gets its own rule
    bds.m_MinClearance = 0;
    bds.m_HoleClearance = 0;
    bds.m_HoleToHoleMin = 0;
    bds.m_CopperEdgeClearance = 0;
    bds.m_SolderMaskMinWidth = 0;
    bds.m_SolderMaskToCopperClearance = 0;

    // Design parameters are only stored above engine 8000
    auto param = [&]( int aId )
    {
        return m_design->Version > 8000 ? std::max( 0, m_design->DesignParameters.Param( aId ) ) : 0;
    };

    bds.m_TrackMinWidth = DsuToNm( param( DP_MIN_TRACK_WIDTH ) );
    bds.m_MinThroughDrill = DsuToNm( param( DP_MIN_HOLE_SIZE ) );
    bds.m_ViasMinAnnularWidth = DsuToNm( param( DP_MIN_VIA_ANNULAR_RING ) );
    bds.m_SolderMaskMinWidth = DsuToNm( param( DP_MIN_SOLDER_MASK_WIDTH ) );
    bds.m_SolderMaskToCopperClearance = DsuToNm( param( DP_MIN_SOLDER_MASK_TO_TRACK ) );

    if( param( DP_MIN_ANNULAR_RING ) > 0 )
    {
        addRule( wxS( "Easy-PC minimum pad annular ring" ),
                 wxS( "(constraint annular_width (min " ) + mm( param( DP_MIN_ANNULAR_RING ) ) + wxS( "))" ),
                 wxS( "A.Type == 'Pad'" ) );
    }

    if( param( DP_MIN_TEXT_SIZE ) > 0 )
    {
        addRule( wxS( "Easy-PC minimum text size" ),
                 wxS( "(constraint text_height (min " ) + mm( param( DP_MIN_TEXT_SIZE ) ) + wxS( "))" ),
                 wxS( "A.Type == 'Text'" ) );
    }

    buildNetClasses();

    if( !m_spacings )
        return;

    // A pair missing from the table is not checked (GetSpacing returns -1), which in KiCad needs an explicit 0
    for( size_t i = 0; i < std::size( COPPER_KINDS ); ++i )
    {
        for( size_t j = i; j < std::size( COPPER_KINDS ); ++j )
        {
            const COPPER_KIND& a = COPPER_KINDS[i];
            const COPPER_KIND& b = COPPER_KINDS[j];

            addRule( wxString::Format( wxS( "Easy-PC %s to %s" ), a.Name, b.Name ),
                     wxS( "(constraint clearance (min " ) + mm( std::max( 0, designValue( a.Kind, b.Kind ) ) )
                             + wxS( "))" ),
                     kindTest( a, wxS( "A" ) ) + wxS( " && " ) + kindTest( b, wxS( "B" ) ) );
        }
    }

    for( const COPPER_KIND& kind : COPPER_KINDS )
    {
        addRule( wxString::Format( wxS( "Easy-PC board to %s" ), kind.Name ),
                 wxS( "(constraint edge_clearance (min " )
                         + mm( std::max( 0, designValue( SPACING_BOARD, kind.Kind ) ) ) + wxS( "))" ),
                 kindTest( kind, wxS( "A" ) ) );
    }
}


void PCB_BUILDER::buildNetClasses()
{
    std::shared_ptr<NET_SETTINGS>& netSettings = m_board.GetDesignSettings().m_NetSettings;

    // A track to track spacing is the closest Easy-PC has to a netclass clearance; custom rules refine it
    int32_t trackSpacing = designValue( SPACING_TRACK, SPACING_TRACK );

    auto fill = [&]( NETCLASS& aClass, const SOURCE_NET_CLASS* aSource )
    {
        aClass.SetClearance( static_cast<int>( DsuToNm( std::max( 0, trackSpacing ) ) ) );

        if( !aSource )
            return;

        int32_t nominal = 0;

        for( const SOURCE_TRACK_STYLE* style : { aSource->TrackStyle1, aSource->TrackStyle2 } )
        {
            if( style )
                nominal = std::max( nominal, style->Width );
        }

        if( nominal > 0 )
        {
            aClass.SetTrackWidth( static_cast<int>( DsuToNm( nominal ) ) );
            aClass.SetDiffPairWidth( static_cast<int>( DsuToNm( nominal ) ) );
        }

        if( aSource->ViaStyle )
        {
            aClass.SetViaDiameter( static_cast<int>( DsuToNm( aSource->ViaStyle->Size ) ) );

            if( aSource->ViaStyle->Drill > 0 )
                aClass.SetViaDrill( static_cast<int>( DsuToNm( aSource->ViaStyle->Drill ) ) );
        }

        // The stored gap applies whether or not diff pairs are enabled
        if( aSource->DiffPairGap > 0 )
            aClass.SetDiffPairGap( static_cast<int>( DsuToNm( aSource->DiffPairGap ) ) );
    };

    std::map<const OBJECT*, wxString> classNames;
    const SOURCE_NET_CLASS*           defaultSource = nullptr;
    wxString                          defaultName;

    if( const SOURCE_DEFAULTS* defaults = dynamic_cast<const SOURCE_DEFAULTS*>( m_design->Defaults ) )
        defaultName = defaults->NetClass;

    for( const SOURCE_NET_CLASS* source : TypedItems<const SOURCE_NET_CLASS>( m_design->NetClasses ) )
    {
        if( !defaultName.IsEmpty() && source->Name.CmpNoCase( defaultName ) == 0 && !defaultSource )
            defaultSource = source;

        wxString name =
                source->Name.CmpNoCase( NETCLASS::Default ) == 0 ? wxString( wxS( "EasyPC_Default" ) ) : source->Name;

        std::shared_ptr<NETCLASS> netclass = std::make_shared<NETCLASS>( name );
        fill( *netclass, source );
        netSettings->SetNetclass( name, netclass );
        classNames[source] = name;

        // Only the minimum style is a DRC limit in Easy-PC's own checks, so it becomes a rule
        int32_t minimum = std::numeric_limits<int32_t>::max();

        for( const SOURCE_TRACK_STYLE* style : { source->TrackStyle1, source->TrackStyle2 } )
        {
            if( style )
                minimum = std::min( minimum, style->Width );
        }

        if( minimum != std::numeric_limits<int32_t>::max() && minimum > 0 && !name.EndsWith( wxS( "\\" ) ) )
        {
            addRule( wxS( "Easy-PC net class " ) + source->Name + wxS( " minimum width" ),
                     wxS( "(constraint track_width (min " ) + mm( minimum ) + wxS( "))" ),
                     wxS( "A.NetClass == " ) + exprLiteral( name ) );
        }
    }

    fill( *netSettings->GetDefaultNetclass(), defaultSource );

    for( const auto& [net, kicadNet] : m_nets )
    {
        if( auto it = classNames.find( static_cast<const SOURCE_NET*>( net )->NetClass ); it != classNames.end() )
        {
            netSettings->SetNetclassPatternAssignment( kicadNet->GetNetname(), it->second );
            kicadNet->SetNetClass( netSettings->GetNetClassByName( it->second ) );
        }
    }
}


SHAPE_POLY_SET PCB_BUILDER::planeClearances( const SOURCE_LAYER& aLayer, PCB_LAYER_ID aKiCadLayer,
                                             NETINFO_ITEM* aNet ) const
{
    // The negative plane plot (Ampera_V1, Chademo_VCU_V3_ISA, GwizBluePill, router2 ODB++): every feature it draws
    // is a clearance, and the plane is the board less them
    SHAPE_POLY_SET clear;
    int            iso = m_frame.Length( m_design->IsolationGap );
    int            spoke = m_frame.Length( m_design->ThermalRelief );
    EDA_ANGLE      spokeAngle = m_design->ThermalAngled ? ANGLE_45 : ANGLE_0;

    // A ring from the land out by isolation_gap, cut by four spokes; round for a round land and rectangular, in the
    // land's orientation, for a rectangular one (GwizBluePill T116)
    auto thermal = [&]( const VECTOR2I& aCentre, const VECTOR2I& aLand, bool aRectangular, const EDA_ANGLE& aAngle )
    {
        SHAPE_POLY_SET ring;
        SHAPE_POLY_SET land;
        SHAPE_POLY_SET spokes;
        int            reach = std::max( aLand.x, aLand.y );

        if( aRectangular )
        {
            TransformTrapezoidToPolygon( ring, aCentre, aLand + VECTOR2I( 2 * iso, 2 * iso ), aAngle, 0, 0, 0,
                                         PLANE_MAX_ERROR, ERROR_OUTSIDE );
            TransformTrapezoidToPolygon( land, aCentre, aLand, aAngle, 0, 0, 0, PLANE_MAX_ERROR, ERROR_INSIDE );
        }
        else
        {
            TransformCircleToPolygon( ring, aCentre, aLand.x / 2 + iso, PLANE_MAX_ERROR, ERROR_OUTSIDE );
            TransformCircleToPolygon( land, aCentre, aLand.x / 2, PLANE_MAX_ERROR, ERROR_INSIDE );
        }

        ring.BooleanSubtract( land );

        // The spokes run from the centre past the ring with square ends
        for( int i = 0; i < 4; ++i )
        {
            VECTOR2I         dir( reach + iso + spoke, 0 );
            VECTOR2I         side( 0, spoke / 2 );
            SHAPE_LINE_CHAIN rect;

            RotatePoint( dir, spokeAngle + ANGLE_90 * i );
            RotatePoint( side, spokeAngle + ANGLE_90 * i );
            rect.Append( aCentre + side );
            rect.Append( aCentre + dir + side );
            rect.Append( aCentre + dir - side );
            rect.Append( aCentre - side );
            rect.SetClosed( true );
            spokes.AddOutline( rect );
        }

        ring.BooleanSubtract( spokes );
        clear.Append( ring );
    };

    for( FOOTPRINT* fp : m_board.Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->IsOnLayer( aKiCadLayer ) )
                continue;

            VECTOR2I    size = pad->GetSize( aKiCadLayer );
            ::PAD_SHAPE shape = pad->GetShape( aKiCadLayer );

            if( pad->GetNet() == aNet && pad->GetAttribute() == PAD_ATTRIB::PTH )
            {
                thermal( pad->GetPosition(), size,
                         shape == ::PAD_SHAPE::RECTANGLE || shape == ::PAD_SHAPE::ROUNDRECT
                                 || shape == ::PAD_SHAPE::CHAMFERED_RECT,
                         pad->GetOrientation() );
                continue;
            }

            // A surface pad on an outer plane's own net is part of its copper
            if( pad->GetNet() == aNet && !pad->HasHole() )
                continue;

            if( shape == ::PAD_SHAPE::CUSTOM )
            {
                pad->TransformShapeToPolygon( clear, aKiCadLayer, iso, PLANE_MAX_ERROR, ERROR_OUTSIDE );
                continue;
            }

            // The clearance grows each half size by the isolation, keeping a rectangle's corners square
            PAD grown( *pad );
            grown.SetSize( aKiCadLayer, size + VECTOR2I( 2 * iso, 2 * iso ) );
            grown.TransformShapeToPolygon( clear, aKiCadLayer, 0, PLANE_MAX_ERROR, ERROR_OUTSIDE );
        }
    }

    for( PCB_TRACK* track : m_board.Tracks() )
    {
        if( !track->IsOnLayer( aKiCadLayer ) )
            continue;

        PCB_VIA* via = dynamic_cast<PCB_VIA*>( track );

        if( via && via->GetNet() == aNet )
        {
            int width = via->GetWidth( aKiCadLayer );
            thermal( via->GetPosition(), VECTOR2I( width, width ), false, ANGLE_0 );
        }
        else if( via )
        {
            TransformCircleToPolygon( clear, via->GetPosition(), via->GetWidth( aKiCadLayer ) / 2 + iso,
                                      PLANE_MAX_ERROR, ERROR_OUTSIDE );
        }
        else if( track->GetNet() != aNet )
        {
            track->TransformShapeToPolygon( clear, aKiCadLayer, iso, PLANE_MAX_ERROR, ERROR_OUTSIDE );
        }
    }

    // A pour keepout is cleared as its region and its outline stroke
    for( const SOURCE_AREA* area : TypedItems<const SOURCE_AREA>( m_design->Areas ) )
    {
        const SOURCE_DESIGN_SHAPE* shape = dynamic_cast<const SOURCE_DESIGN_SHAPE*>( area->Shape );

        if( !shape || !( area->Flags & AREA_POUR_KEEPOUT ) || area->Layer != &aLayer )
            continue;

        clear.Append( ShapeItemArea( *area, m_frame, PLANE_MAX_ERROR ) );
        SHAPE_POLY_SET region = ShapeToPolySet( *shape, m_frame );
        region.ClearArcs();
        clear.Append( region );
    }

    clear.Simplify();
    return clear;
}


void PCB_BUILDER::buildPlanes()
{
    // A copper layer with a net is a power plane, which plots negative: the board, less a clearance
    // of isolation_gap around foreign items and a thermal of isolation_gap with spokes of thermal_relief on its own
    std::vector<ZONE*>            planes;
    std::optional<SHAPE_POLY_SET> outline;

    for( const SOURCE_LAYER* layer : TypedItems<const SOURCE_LAYER>( m_design->Layers ) )
    {
        PCB_LAYER_ID kicadLayer = m_layers->Map( layer );
        auto         netIt = m_nets.find( layer->Net );

        if( layer->Usage != LAYER_USAGE_ELECTRICAL || !layer->Net || kicadLayer == UNDEFINED_LAYER
            || netIt == m_nets.end() )
        {
            continue;
        }

        // The plot clears the board edge by the board to shape spacing beyond the outline's half width
        for( const SOURCE_BOARD* board : TypedItems<const SOURCE_BOARD>( outline ? nullptr : m_design->Boards ) )
        {
            const SOURCE_DESIGN_SHAPE* shape = dynamic_cast<const SOURCE_DESIGN_SHAPE*>( board->Shape );
            const SOURCE_LINE_STYLE*   style = dynamic_cast<const SOURCE_LINE_STYLE*>( board->Style );
            SHAPE_POLY_SET             region;

            if( shape && shape->IsClosed() )
            {
                region = ShapeToPolySet( *shape, m_frame );
                region.ClearArcs();
                region.Deflate( m_frame.Length( std::max( shapeSpacing( SPACING_BOARD ), 0 ) )
                                        + ( style ? m_frame.Length( style->Width ) / 2 : 0 ),
                                CORNER_STRATEGY::ROUND_ALL_CORNERS, PLANE_MAX_ERROR );
            }

            ( outline ? *outline : outline.emplace() ).BooleanAdd( region );
        }

        if( !outline || outline->IsEmpty() )
            continue;

        ZONE* zone = new ZONE( &m_board );
        zone->SetLayer( kicadLayer );
        zone->SetNet( netIt->second );
        zone->SetZoneName( layer->Name );
        *zone->Outline() = *outline;
        zone->SetLocalClearance( m_frame.Length( m_design->IsolationGap ) );
        zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        zone->SetThermalReliefGap( m_frame.Length( m_design->IsolationGap ) );
        zone->SetThermalReliefSpokeWidth( m_frame.Length( m_design->ThermalRelief ) );
        zone->SetMinThickness( PLANE_MIN_THICKNESS );
        zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
        m_board.Add( zone );
        planes.push_back( zone );

        // The fill is drawn as the plane plots; KiCad's filler has no via thermals and clears a
        // dropped pad only around its hole, so its settings above serve a later refill
        SHAPE_POLY_SET fill = *outline;
        fill.BooleanSubtract( planeClearances( *layer, kicadLayer, netIt->second ) );
        fill.Fracture();
        zone->SetFilledPolysList( kicadLayer, fill );
        zone->SetIsFilled( true );
        zone->SetNeedRefill( false );
    }

    if( planes.empty() )
        return;

    wxString names;
    LSET     planeLayers;

    for( ZONE* zone : planes )
    {
        names += ( names.IsEmpty() ? wxString() : wxString( wxS( ", " ) ) ) + zone->GetZoneName();
        planeLayers.set( zone->GetLayer() );
    }

    if( m_reporter )
    {
        m_reporter->Report( wxString::Format( _( "Easy-PC power planes (%s) were imported with the copper the "
                                                 "program plots. KiCad cannot refill them the same way: a refill "
                                                 "makes via thermals solid and clears foreign through-hole lands "
                                                 "only around their holes." ),
                                              names ),
                            RPT_SEVERITY_WARNING );
    }

    // A plane plots a foreign pad or via as its clearance alone; with only planes inside, KiCad drops their
    // unconnected inner copper the same way
    bool onlyPlanesInside = ( m_board.GetEnabledLayers() & LSET::InternalCuMask() ) == planeLayers;

    for( FOOTPRINT* fp : m_board.Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( onlyPlanesInside && pad->GetAttribute() == PAD_ATTRIB::PTH )
                pad->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );

            // KiCad's filler records which lands a fill reaches; the stored fill reaches every land on its own net.
            // The plot's thermal spokes run on the axes unless the design angles them
            for( ZONE* zone : planes )
            {
                if( pad->GetNet() == zone->GetNet() )
                    pad->SetThermalSpokeAngle( m_design->ThermalAngled ? ANGLE_45 : ANGLE_0 );

                if( pad->HasHole() && pad->IsOnLayer( zone->GetLayer() ) && pad->GetNet() == zone->GetNet() )
                    pad->SetZoneLayerOverride( zone->GetLayer(), ZLO_FORCE_FLASHED );
            }
        }
    }

    for( PCB_TRACK* track : m_board.Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( track );

        if( !via )
            continue;

        if( onlyPlanesInside )
            via->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );

        for( ZONE* zone : planes )
        {
            if( via->IsOnLayer( zone->GetLayer() ) && via->GetNet() == zone->GetNet() )
                via->SetZoneLayerOverride( zone->GetLayer(), ZLO_FORCE_FLASHED );
        }
    }
}


void PCB_BUILDER::buildGroups()
{
    // A dimension is stored as a group of its lines, arrows and text, kept as drawn
    for( const SOURCE_GROUP* group : TypedItems<const SOURCE_GROUP>( m_design->Groups ) )
    {
        PCB_GROUP* kgroup = new PCB_GROUP( &m_board );
        kgroup->SetName( group->Name );

        for( const OBJECT* member : group->Members )
        {
            auto it = m_created.find( member );

            if( it == m_created.end() )
                continue;

            for( BOARD_ITEM* item : it->second )
            {
                if( !item->GetParentGroup() )
                    kgroup->AddItem( item );
            }
        }

        if( kgroup->GetItems().empty() )
            delete kgroup;
        else
            m_board.Add( kgroup );
    }
}


void PCB_BUILDER::buildLayers()
{
    int copper = m_layers->CopperCount();
    m_board.SetCopperLayerCount( copper );

    LSET                   enabled = LSET::AllCuMask( copper ) | LSET( { Edge_Cuts } );
    BOARD_DESIGN_SETTINGS& bds = m_board.GetDesignSettings();

    // Mask and paste openings are the pad grown by the layer type's oversize
    for( const SOURCE_LAYER* layer : TypedItems<const SOURCE_LAYER>( m_design->Layers ) )
    {
        PCB_LAYER_ID             id = m_layers->Map( layer );
        const SOURCE_LAYER_TYPE* type = layer->Type;
        bool                     mask = id == F_Mask || id == B_Mask;

        if( id == UNDEFINED_LAYER )
            continue;

        enabled.set( id );

        if( IsCopperLayer( id ) || IsUserLayer( id ) )
            m_board.SetLayerName( id, layer->Name );

        if( !type || ( !mask && id != F_Paste && id != B_Paste ) )
            continue;

        // Vias are plotted on a mask layer only when its type enables them
        if( mask )
            m_viaMask[id == F_Mask ? 0 : 1] = type->ViasEnabled;

        if( type->OversizeType == LAYER_OVERSIZE_TYPE::FIXED && mask )
            bds.m_SolderMaskExpansion = m_frame.Length( type->Oversize );
        else if( type->OversizeType == LAYER_OVERSIZE_TYPE::FIXED )
            bds.m_SolderPasteMargin = m_frame.Length( type->Oversize );
        else if( type->OversizeType == LAYER_OVERSIZE_TYPE::PERCENT && !mask )
            bds.m_SolderPasteMarginRatio = type->Oversize / 100.0;
    }

    m_board.SetEnabledLayers( enabled );
    m_board.SetVisibleLayers( enabled );
}


NETINFO_ITEM* PCB_BUILDER::netOfNode( const OBJECT* aNode ) const
{
    const SOURCE_NODE* node = dynamic_cast<const SOURCE_NODE*>( aNode );
    auto               it = node ? m_nets.find( node->Parent ) : m_nets.end();

    return it == m_nets.end() ? nullptr : it->second;
}


void PCB_BUILDER::buildFootprints()
{
    for( const SOURCE_COMPONENT_INSTANCE* ci :
         TypedItems<const SOURCE_COMPONENT_INSTANCE>( m_design->ComponentInstances ) )
    {
        const SOURCE_COMPONENT*     component = dynamic_cast<const SOURCE_COMPONENT*>( ci->Component );
        const SOURCE_PCB_COMPONENT* pcb = component ? component->Board : nullptr;

        for( const SOURCE_SYMBOL_INSTANCE* si : TypedItems<const SOURCE_SYMBOL_INSTANCE>( ci->SymbolInstances ) )
        {
            const SOURCE_SYMBOL* symbol = dynamic_cast<const SOURCE_SYMBOL*>( si->Symbol );

            if( si->Gate != -1 || !si->Placed || !symbol )
                continue;

            std::map<const SOURCE_FREE_PAD*, PAD*> padMap;
            LIB_ID                                 id( wxS( "easypc" ), EscapeString( symbol->Name, CTX_LIBID ) );
            std::unique_ptr<FOOTPRINT>             fp = ConvertFootprint( *symbol, *m_layers, id, pcb, &padMap );

            fp->SetParent( &m_board );

            // A pad instance's style exception replaces the definition's style for that one pad
            FRAME symbolFrame{ POINT32{ symbol->OriginX, symbol->OriginY } };

            for( const SOURCE_PAD_INSTANCE* pi : TypedItems<const SOURCE_PAD_INSTANCE>( si->Pads ) )
            {
                const SOURCE_FREE_PAD*  freePad = dynamic_cast<const SOURCE_FREE_PAD*>( pi->Pad );
                const SOURCE_PAD_STYLE* exception = dynamic_cast<const SOURCE_PAD_STYLE*>( pi->StyleException );
                auto                    it = padMap.find( freePad );

                if( !exception || it == padMap.end() )
                    continue;

                std::unique_ptr<PAD> pad = CreatePad( fp.get(), *freePad, *exception, symbolFrame, *m_layers );
                pad->SetNumber( it->second->GetNumber() );
                fp->Remove( it->second );
                delete it->second;
                it->second = pad.get();
                fp->Add( pad.release() );
            }

            fp->SetReference( ci->Name );

            if( component && component->Schematic )
                fp->SetValue( component->Schematic->Name );

            // Native placement is mirror in X, then rotate anticlockwise, then translate.  A left-right flip
            // records 180 degrees in the orientation, so the rotation is applied relative to it
            if( si->Mirrored )
                fp->Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

            fp->Rotate( VECTOR2I( 0, 0 ), FRAME::Angle( si->Angle ) );
            fp->SetPosition( m_frame.ToKiCad( si->Position ) );

            placeValueTexts( *fp, *ci, *si );

            for( const SOURCE_PAD_INSTANCE* pi : TypedItems<const SOURCE_PAD_INSTANCE>( si->Pads ) )
            {
                auto          it = padMap.find( dynamic_cast<const SOURCE_FREE_PAD*>( pi->Pad ) );
                NETINFO_ITEM* net = netOfNode( pi->Node );

                if( it != padMap.end() && net )
                    it->second->SetNet( net );
            }

            m_board.Add( fp.release() );
        }
    }
}


void PCB_BUILDER::placeValueTexts( FOOTPRINT& aFootprint, const SOURCE_COMPONENT_INSTANCE& aInstance,
                                   const SOURCE_SYMBOL_INSTANCE& aSymbolInstance )
{
    bool referencePlaced = false;
    bool valuePlaced = false;

    // The positions are in board coordinates, so they are applied after the footprint is placed.  The first
    // reference and value positions place the fields; a further reference position is a text of its own
    for( const SOURCE_VALUE_POSITION* vp : TypedItems<const SOURCE_VALUE_POSITION>( aSymbolInstance.ValuePositions ) )
    {
        if( ( vp->Types & VALUE_REFERENCE_NAME ) && !referencePlaced )
        {
            bool placed = ApplyTextPosition( aFootprint.Reference(), *vp, m_frame, *m_layers );
            aFootprint.Reference().SetVisible( placed && vp->Displayed );
            referencePlaced = true;
        }
        else if( ( vp->Types & VALUE_COMPONENT_NAME ) && !valuePlaced )
        {
            bool placed = ApplyTextPosition( aFootprint.Value(), *vp, m_frame, *m_layers );
            aFootprint.Value().SetVisible( placed && vp->Displayed );
            valuePlaced = true;
        }
        else if( ( vp->Types & VALUE_REFERENCE_NAME ) && vp->Displayed )
        {
            std::unique_ptr<PCB_TEXT> item = std::make_unique<PCB_TEXT>( &aFootprint );
            item->SetText( aInstance.Name );

            if( ApplyTextPosition( *item, *vp, m_frame, *m_layers ) )
                aFootprint.Add( item.release() );
        }
    }

    if( !referencePlaced )
        aFootprint.Reference().SetVisible( false );

    if( !valuePlaced )
        aFootprint.Value().SetVisible( false );
}


void PCB_BUILDER::buildFreeItems()
{
    // The design list also holds every area's stored pour; buildAreas() makes those zone fills
    std::set<const SOURCE_FREE_COPPER*> poured;

    for( const SOURCE_AREA* area : TypedItems<const SOURCE_AREA>( m_design->Areas ) )
    {
        if( becomesZone( *area ) )
        {
            for( const SOURCE_FREE_COPPER* piece : TypedItems<const SOURCE_FREE_COPPER>( area->Copper ) )
                poured.insert( piece );
        }
    }

    for( const SOURCE_FREE_COPPER* copper : TypedItems<const SOURCE_FREE_COPPER>( m_design->Coppers ) )
    {
        if( poured.count( copper ) )
            continue;

        NETINFO_ITEM* net = nullptr;

        for( const SOURCE_COPPER_TERMINAL* term : TypedItems<const SOURCE_COPPER_TERMINAL>( copper->Terminals ) )
        {
            if( ( net = netOfNode( term->Node ) ) != nullptr )
                break;
        }

        for( PCB_SHAPE* shape : AddShapeItem( m_board, *copper, m_frame, *m_layers ) )
        {
            if( net && IsCopperLayer( shape->GetLayer() ) )
                shape->SetNet( net );

            m_created[copper].push_back( shape );
        }
    }

    for( const SOURCE_FREE_TEXT* text : TypedItems<const SOURCE_FREE_TEXT>( m_design->Texts ) )
    {
        if( PCB_TEXT* ktext = AddText( m_board, *text, text->Text, m_frame, *m_layers ) )
            m_created[text].push_back( ktext );
    }

    for( const SOURCE_FREE_PAD* freePad : TypedItems<const SOURCE_FREE_PAD>( m_design->Pads ) )
    {
        const SOURCE_PAD_STYLE* style = dynamic_cast<const SOURCE_PAD_STYLE*>( freePad->Style );

        if( !style )
            continue;

        // KiCad pads live in footprints, so each free pad gets a footprint of its own at the pad position
        FOOTPRINT* fp = new FOOTPRINT( &m_board );
        fp->SetReference( wxEmptyString );
        fp->Reference().SetVisible( false );
        fp->Value().SetVisible( false );

        std::unique_ptr<PAD> pad = CreatePad( fp, *freePad, *style, FRAME{ freePad->Position }, *m_layers );

        if( NETINFO_ITEM* net = netOfNode( freePad->Node ) )
            pad->SetNet( net );

        fp->Add( pad.release() );
        fp->SetPosition( m_frame.ToKiCad( freePad->Position ) );
        m_board.Add( fp );
        m_created[freePad].push_back( fp );
    }
}


void PCB_BUILDER::buildTracksAndVias()
{
    std::set<const SOURCE_VIA*> vias;

    for( const SOURCE_NET* net : TypedItems<const SOURCE_NET>( m_design->Nets ) )
    {
        NETINFO_ITEM* info = m_nets[net];

        for( const SOURCE_NODE* node : TypedItems<const SOURCE_NODE>( net->Nodes ) )
        {
            const SOURCE_VIA*       via = dynamic_cast<const SOURCE_VIA*>( node->Item );
            const SOURCE_PAD_STYLE* style = via ? dynamic_cast<const SOURCE_PAD_STYLE*>( via->Style ) : nullptr;

            if( !style || !vias.insert( via ).second )
                continue;

            PCB_VIA* kvia = new PCB_VIA( &m_board );
            kvia->SetPosition( m_frame.ToKiCad( via->Position ) );
            kvia->SetWidth( PADSTACK::ALL_LAYERS, m_frame.Length( style->Size ) );
            kvia->SetDrill( m_frame.Length( style->Drill ) );
            kvia->SetNet( info );

            // The net class forces tenting (0 open, 1 tented) or, with 2, defers to the via
            const SOURCE_NET_CLASS* netClass = dynamic_cast<const SOURCE_NET_CLASS*>( net->NetClass );
            int                     rule = netClass ? netClass->TentedVias : 2;
            bool                    tented = rule == 1 || ( rule == 2 && via->Tented );

            kvia->SetFrontTentingMode( !tented && m_viaMask[0] ? TENTING_MODE::NOT_TENTED : TENTING_MODE::TENTED );
            kvia->SetBackTentingMode( !tented && m_viaMask[1] ? TENTING_MODE::NOT_TENTED : TENTING_MODE::TENTED );
            m_board.Add( kvia );
            m_created[via].push_back( kvia );
        }

        for( const SOURCE_CONNECTION* conn : TypedItems<const SOURCE_CONNECTION>( net->Connections ) )
        {
            const SOURCE_TRACK* track = dynamic_cast<const SOURCE_TRACK*>( conn->Track );
            PCB_LAYER_ID        layer = track ? m_layers->Map( track->Layer ) : UNDEFINED_LAYER;

            if( layer == UNDEFINED_LAYER )
                continue;

            std::vector<const SOURCE_TRACK_SEGMENT*> verts;

            for( const OBJECT* obj : track->Segments )
            {
                if( const SOURCE_TRACK_SEGMENT* seg = dynamic_cast<const SOURCE_TRACK_SEGMENT*>( obj ) )
                    verts.push_back( seg );
            }

            auto add = [&]( PCB_TRACK* aItem, int aWidth )
            {
                aItem->SetWidth( aWidth );
                aItem->SetLayer( layer );
                aItem->SetNet( info );
                m_board.Add( aItem );
                m_created[conn].push_back( aItem );
            };

            for( size_t i = 0; i + 1 < verts.size(); ++i )
            {
                const SOURCE_TRACK_SEGMENT* s = verts[i];
                const SOURCE_TRACK_SEGMENT* e = verts[i + 1];
                const SOURCE_TRACK_STYLE*   style = dynamic_cast<const SOURCE_TRACK_STYLE*>( s->Style );

                // A span's width is its own style's, with no fallback to the track's default style
                int width = style ? m_frame.Length( style->Width ) : 0;

                // A zero-length span plots as a dot of the track width, which a degenerate track also does
                int32_t  angle = s->X != e->X || s->Y != e->Y ? s->ArcAngle : 0;
                ARC_FORM form =
                        ArcForm( POINT32{ s->X, s->Y }, POINT32{ e->X, e->Y }, angle, s->Anticlockwise, m_frame );

                if( form.IsArc() )
                {
                    PCB_ARC* arc = new PCB_ARC( &m_board );
                    arc->SetStart( form.Start );
                    arc->SetMid( form.Mid );
                    arc->SetEnd( form.End );
                    add( arc, width );
                }

                for( size_t k = 1; k < form.Points.size(); ++k )
                {
                    PCB_TRACK* ktrack = new PCB_TRACK( &m_board );
                    ktrack->SetStart( form.Points[k - 1] );
                    ktrack->SetEnd( form.Points[k] );
                    add( ktrack, width );
                }
            }
        }
    }
}


bool PCB_BUILDER::becomesZone( const SOURCE_AREA& aArea ) const
{
    return aArea.Type == AREA_COPPER && !( aArea.Flags & AREA_POUR_KEEPOUT )
           && dynamic_cast<const SOURCE_DESIGN_SHAPE*>( aArea.Shape )
           && m_layers->Map( aArea.Layer ) != UNDEFINED_LAYER;
}


void PCB_BUILDER::buildAreas()
{
    int routingAreas = 0;
    int ruleAreas = 0;

    for( const SOURCE_AREA* area : TypedItems<const SOURCE_AREA>( m_design->Areas ) )
    {
        const SOURCE_DESIGN_SHAPE* shape = dynamic_cast<const SOURCE_DESIGN_SHAPE*>( area->Shape );

        if( !shape )
            continue;

        // A pour keepout keeps every zone fill out; KiCad has no way to limit that to one net
        if( area->Flags & AREA_POUR_KEEPOUT )
        {
            LSET copper = m_layers->PadLayers( area->Layer, false ) & LSET::AllCuMask();

            if( copper.none() )
                continue;

            SHAPE_POLY_SET outline = ShapeToPolySet( *shape, m_frame );
            outline.ClearArcs();

            ZONE* zone = new ZONE( &m_board );
            ++ruleAreas;
            zone->SetIsRuleArea( true );
            zone->SetZoneName( area->Name.IsEmpty() ? wxString::Format( wxS( "Easy-PC area %d" ), ruleAreas )
                                                    : area->Name );
            zone->SetLayerSet( copper );
            *zone->Outline() = outline;
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            m_board.Add( zone );
            continue;
        }

        // The routing area check is off in every known design, so these restrict nothing
        if( area->Type == AREA_ROUTING )
        {
            PCB_GROUP* group = new PCB_GROUP( &m_board );
            group->SetName( area->Name.IsEmpty() ? wxString( wxS( "Easy-PC routing area" ) ) : area->Name );

            for( PCB_SHAPE* outline : AddShapeItem( m_board, *area, m_frame, *m_layers, Cmts_User ) )
                group->AddItem( outline );

            ++routingAreas;

            if( group->GetItems().empty() )
            {
                delete group;
                continue;
            }

            m_board.Add( group );
            m_created[area].push_back( group );
            continue;
        }

        if( !becomesZone( *area ) )
            continue;

        PCB_LAYER_ID layer = m_layers->Map( area->Layer );
        ZONE*        zone = new ZONE( &m_board );
        zone->SetLayer( layer );
        *zone->Outline() = ShapeToPolySet( *shape, m_frame );

        if( auto netIt = m_nets.find( area->Net ); netIt != m_nets.end() )
            zone->SetNet( netIt->second );

        // Pours clear each item by the shape spacing plus the pour net's guard; the
        // smallest over the item kinds is the zone's own clearance, larger kinds are left to the custom rules
        const SOURCE_NET* net = dynamic_cast<const SOURCE_NET*>( area->Net );
        int32_t           clearance = -1;

        for( int kind : { SPACING_TRACK, SPACING_PAD, SPACING_VIA, SPACING_SHAPE } )
        {
            int32_t spacing = shapeSpacing( kind );

            if( spacing >= 0 && ( clearance < 0 || spacing < clearance ) )
                clearance = spacing;
        }

        if( clearance >= 0 )
            zone->SetLocalClearance( m_frame.Length( int64_t( clearance ) + ( net ? net->GuardSpacing : 0 ) ) );

        // Keep the imported pour within a micron of its stored outline.
        SHAPE_POLY_SET fill;
        int            maxError = std::min( m_board.GetDesignSettings().m_MaxError, 1000 );

        // One union of all pieces; adding them one by one is quadratic on large pours
        for( const SOURCE_FREE_COPPER* poured : TypedItems<const SOURCE_FREE_COPPER>( area->Copper ) )
        {
            SHAPE_POLY_SET piece = ShapeItemArea( *poured, m_frame, maxError );

            for( int i = 0; i < piece.OutlineCount(); ++i )
                fill.AddPolygon( piece.Polygon( i ) );
        }

        if( !fill.IsEmpty() )
        {
            fill.Simplify();
            fill.Fracture();
            zone->SetFilledPolysList( layer, fill );
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );
        }

        m_board.Add( zone );
    }

    if( routingAreas && m_reporter )
    {
        m_reporter->Report( wxString::Format( _( "%d Easy-PC routing areas were imported as outlines on Cmts.User; "
                                                 "the design does not run its routing area check, so they restrict "
                                                 "nothing." ),
                                              routingAreas ),
                            RPT_SEVERITY_WARNING );
    }
}


wxString BuildBoard( const DESIGN_DOCUMENT& aDocument, BOARD& aBoard, REPORTER* aReporter,
                     const LAYER_MAPPING_HANDLER& aLayerMapping )
{
    PCB_BUILDER builder( aDocument, aBoard, aReporter );

    if( aLayerMapping )
        builder.m_layers->ApplyUserMapping( aLayerMapping( builder.m_layers->Describe() ) );

    builder.Build();
    return builder.m_customRules;
}

} // namespace EASYPC_PCB
