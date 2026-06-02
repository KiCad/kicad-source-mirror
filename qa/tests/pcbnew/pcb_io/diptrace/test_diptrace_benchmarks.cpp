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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_diptrace_benchmarks.cpp
 * Benchmark tests for DipTrace pad import accuracy.
 *
 * These tests encode what correct pad import should look like:
 * reasonable dimensions, correct pin spacing for known packages,
 * and pad-to-net assignment. Tests that currently fail represent
 * known gaps to be closed.
 */

#include "test_diptrace_benchmarks_fixture.h"


namespace
{
static constexpr int CONNECT_TOL_NM = 1000; // 1 um


bool PointsNear( const VECTOR2I& aA, const VECTOR2I& aB )
{
    return ( aA - aB ).EuclideanNorm() <= CONNECT_TOL_NM;
}


bool IsHeuristicParserWarning( const wxString& aMessage )
{
    return aMessage.Contains( wxS( "design rule set" ) )
           || aMessage.Contains( wxS( "inter-ruleset transition marker" ) )
           || aMessage.Contains( wxS( "no validated component boundaries found" ) )
           || aMessage.Contains( wxS( "parse error" ) )
           || aMessage.Contains( wxS( "parsing failed" ) )
           || aMessage.Contains( wxS( "outline traversal aborted" ) );
}


class DIPTRACE_WARNING_CAPTURE : public wxLog
{
public:
    std::vector<wxString> m_warnings;

protected:
    void DoLogRecord( wxLogLevel aLevel, const wxString& aMessage,
                      const wxLogRecordInfo& ) override
    {
        if( aLevel == wxLOG_Warning )
            m_warnings.push_back( aMessage );
    }
};


int CountDisconnectedEdgeCutsEndpoints( const BOARD& aBoard, int& aTotalEndpoints )
{
    std::vector<VECTOR2I> endpoints;

    for( const BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != Edge_Cuts )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( shape->GetShape() != SHAPE_T::SEGMENT && shape->GetShape() != SHAPE_T::ARC )
            continue;

        endpoints.push_back( shape->GetStart() );
        endpoints.push_back( shape->GetEnd() );
    }

    aTotalEndpoints = static_cast<int>( endpoints.size() );

    int disconnected = 0;

    for( size_t i = 0; i < endpoints.size(); i++ )
    {
        bool connected = false;

        for( size_t j = 0; j < endpoints.size(); j++ )
        {
            if( i == j )
                continue;

            if( PointsNear( endpoints[i], endpoints[j] ) )
            {
                connected = true;
                break;
            }
        }

        if( !connected )
            disconnected++;
    }

    return disconnected;
}


int CountDisconnectedTraceEndpoints( const BOARD& aBoard, int& aTotalEndpoints )
{
    std::map<int, std::vector<VECTOR2I>>                                netAnchors;
    std::unordered_map<int, std::unordered_multimap<int64_t, VECTOR2I>> netAnchorBuckets;

    auto cellCoord = []( int aValue ) -> int
    {
        if( aValue >= 0 )
            return aValue / CONNECT_TOL_NM;

        return -( ( -aValue + CONNECT_TOL_NM - 1 ) / CONNECT_TOL_NM );
    };

    auto cellKey = []( int aCellX, int aCellY ) -> int64_t
    {
        return ( static_cast<int64_t>( aCellX ) << 32 ) ^ static_cast<uint32_t>( aCellY );
    };

    auto addAnchor = [&]( int aNetCode, const VECTOR2I& aPos )
    {
        netAnchors[aNetCode].push_back( aPos );

        int cx = cellCoord( aPos.x );
        int cy = cellCoord( aPos.y );
        netAnchorBuckets[aNetCode].emplace( cellKey( cx, cy ), aPos );
    };

    for( const PCB_TRACK* track : aBoard.Tracks() )
    {
        int netCode = track->GetNetCode();

        if( netCode <= 0 )
            continue;

        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
        {
            addAnchor( netCode, track->GetStart() );
            addAnchor( netCode, track->GetEnd() );
        }
        else if( track->Type() == PCB_VIA_T )
        {
            addAnchor( netCode, track->GetPosition() );
        }
    }

    for( const FOOTPRINT* fp : aBoard.Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            int netCode = pad->GetNetCode();

            if( netCode > 0 )
                addAnchor( netCode, pad->GetPosition() );
        }
    }

    aTotalEndpoints = 0;
    int disconnected = 0;

    for( const PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() != PCB_TRACE_T && track->Type() != PCB_ARC_T )
            continue;

        int netCode = track->GetNetCode();

        if( netCode <= 0 )
            continue;

        auto bucketIt = netAnchorBuckets.find( netCode );

        if( bucketIt == netAnchorBuckets.end() )
            continue;

        const auto&    buckets = bucketIt->second;
        const VECTOR2I endpoints[2] = { track->GetStart(), track->GetEnd() };

        for( const VECTOR2I& endpoint : endpoints )
        {
            aTotalEndpoints++;

            int nearbyCount = 0;
            int cx = cellCoord( endpoint.x );
            int cy = cellCoord( endpoint.y );

            for( int dx = -1; dx <= 1 && nearbyCount < 2; dx++ )
            {
                for( int dy = -1; dy <= 1 && nearbyCount < 2; dy++ )
                {
                    auto range = buckets.equal_range( cellKey( cx + dx, cy + dy ) );

                    for( auto it = range.first; it != range.second; ++it )
                    {
                        if( PointsNear( endpoint, it->second ) )
                        {
                            nearbyCount++;

                            if( nearbyCount >= 2 )
                                break;
                        }
                    }
                }
            }

            if( nearbyCount < 2 )
                disconnected++;
        }
    }

    return disconnected;
}


int CountViaNetShorts( const BOARD& aBoard, int& aCheckedVias, std::vector<std::string>* aReports = nullptr )
{
    struct NET_ANCHOR
    {
        VECTOR2I pos;
        int      netCode = 0;
        LSET     layers;
    };

    std::vector<NET_ANCHOR> anchors;

    for( const PCB_TRACK* track : aBoard.Tracks() )
    {
        int netCode = track->GetNetCode();

        if( netCode <= 0 )
            continue;

        LSET copperLayers = track->GetLayerSet() & LSET::AllCuMask();

        if( copperLayers.none() )
            continue;

        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
        {
            anchors.push_back( { track->GetStart(), netCode, copperLayers } );
            anchors.push_back( { track->GetEnd(), netCode, copperLayers } );
        }
        else if( track->Type() == PCB_VIA_T )
        {
            anchors.push_back( { track->GetPosition(), netCode, copperLayers } );
        }
    }

    for( const FOOTPRINT* fp : aBoard.Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            int  netCode = pad->GetNetCode();
            LSET padLayers = pad->GetLayerSet() & LSET::AllCuMask();

            if( netCode > 0 && !padLayers.none() )
                anchors.push_back( { pad->GetPosition(), netCode, padLayers } );
        }
    }

    int shortedVias = 0;
    aCheckedVias = 0;

    for( const PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );
        int            viaNet = via->GetNetCode();
        LSET           viaLayers = via->GetLayerSet() & LSET::AllCuMask();

        if( viaNet <= 0 || viaLayers.none() )
            continue;

        aCheckedVias++;
        bool shortFound = false;

        for( const NET_ANCHOR& anchor : anchors )
        {
            if( anchor.netCode == viaNet )
                continue;

            if( ( viaLayers & anchor.layers ).none() )
                continue;

            if( PointsNear( via->GetPosition(), anchor.pos ) )
            {
                shortFound = true;

                if( aReports && aReports->size() < 20 )
                {
                    const NETINFO_ITEM* viaNetInfo = aBoard.FindNet( viaNet );
                    const NETINFO_ITEM* otherNetInfo = aBoard.FindNet( anchor.netCode );
                    std::string         viaName =
                            viaNetInfo ? std::string( viaNetInfo->GetNetname().utf8_str() ) : std::to_string( viaNet );
                    std::string otherName = otherNetInfo ? std::string( otherNetInfo->GetNetname().utf8_str() )
                                                         : std::to_string( anchor.netCode );

                    aReports->push_back( "via(" + viaName + ") at (" + std::to_string( via->GetPosition().x ) + ","
                                         + std::to_string( via->GetPosition().y ) + ") overlaps net " + otherName );
                }

                break;
            }
        }

        if( shortFound )
            shortedVias++;
    }

    return shortedVias;
}


int CountPadsOutsideBoardOutline( BOARD& aBoard, int& aTotalPads, bool& aHasOutline )
{
    SHAPE_POLY_SET boardOutline;
    aHasOutline = aBoard.GetBoardPolygonOutlines( boardOutline, true ) && boardOutline.OutlineCount() > 0;

    aTotalPads = 0;

    if( !aHasOutline )
        return 0;

    int outsidePads = 0;

    for( const FOOTPRINT* fp : aBoard.Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            aTotalPads++;

            if( !boardOutline.Contains( pad->GetPosition(), -1, CONNECT_TOL_NM ) )
                outsidePads++;
        }
    }

    return outsidePads;
}


bool IsRectLikeSmdPadShape( PAD_SHAPE aShape )
{
    return aShape == PAD_SHAPE::RECTANGLE || aShape == PAD_SHAPE::ROUNDRECT || aShape == PAD_SHAPE::OVAL;
}


bool HasDipExtension( const std::filesystem::path& aPath )
{
    std::string ext = aPath.extension().string();
    std::transform( ext.begin(), ext.end(), ext.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );
    return ext == ".dip";
}


const FOOTPRINT* FindFootprintByRef( const BOARD& aBoard, const wxString& aRef )
{
    for( const FOOTPRINT* fp : aBoard.Footprints() )
    {
        if( fp->GetReference() == aRef )
            return fp;
    }

    return nullptr;
}


int CardinalDeg( double aDegrees )
{
    int deg = static_cast<int>( std::lround( aDegrees ) );
    deg = ( ( deg % 360 ) + 360 ) % 360;

    int cardinal = static_cast<int>( std::lround( deg / 90.0 ) ) * 90;
    return ( ( cardinal % 360 ) + 360 ) % 360;
}


int NormalizeDeg( double aDegrees )
{
    int deg = static_cast<int>( std::lround( aDegrees ) );
    return ( ( deg % 360 ) + 360 ) % 360;
}


int DipXmlSpokeMode( std::string aSpoke )
{
    aSpoke.erase( std::remove_if( aSpoke.begin(), aSpoke.end(),
                                  []( unsigned char c ) { return std::isspace( c ) != 0; } ),
                  aSpoke.end() );
    std::transform( aSpoke.begin(), aSpoke.end(), aSpoke.begin(),
                    []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

    if( aSpoke == "direct" )
        return 0;

    if( aSpoke == "2spoke90" )
        return 1;

    if( aSpoke == "2spoke" )
        return 2;

    if( aSpoke == "4spoke45" )
        return 3;

    if( aSpoke == "4spoke" )
        return 4;

    return -1;
}


ISLAND_REMOVAL_MODE DipXmlIslandModeToKiCad( bool aIslandRegion, bool aIslandInternal,
                                             bool aIslandConnection )
{
    if( aIslandInternal || aIslandConnection )
        return ISLAND_REMOVAL_MODE::ALWAYS;

    if( aIslandRegion )
        return ISLAND_REMOVAL_MODE::AREA;

    return ISLAND_REMOVAL_MODE::NEVER;
}


long long DipXmlMinimumAreaToKiCadIu2( double aMinimumAreaMm )
{
    // Match importer conversion path:
    // DipXML MinimumArea (mm scalar) -> DipTrace units -> KiCad IU -> IU^2.
    long long dipUnits = static_cast<long long>( std::llround( aMinimumAreaMm * 30000.0 ) );
    long long linearIu = dipUnits * 100 / 3;
    return linearIu * linearIu;
}


struct DIPXML_PAD_STYLE
{
    bool   isSurface = false;
    bool   isThrough = false;
    bool   isRoundHole = false;
    double holeMm = 0.0;
};


struct DIPXML_PATTERN_PAD
{
    std::string styleName;
    int         angleCardinalDeg = 0;
};


struct DIPXML_BOARD_MODEL
{
    std::unordered_map<std::string, DIPXML_PAD_STYLE>                                    styles;
    std::unordered_map<std::string, std::unordered_map<std::string, DIPXML_PATTERN_PAD>> patterns;
    std::vector<std::tuple<std::string, std::string, int>>                               components;
    std::unordered_map<int, std::string>                                                 netNames;

    struct DIPXML_COPPER_POUR
    {
        int         netId = -1;
        int         layer = -1;
        int         priority = 0;
        double      clearanceMm = 0.0;
        double      lineWidthMm = 0.0;
        double      minimumAreaMm = 0.0;
        std::string spoke;
        double      spokeWidthMm = 0.0;
        bool        islandRegion = false;
        bool        islandInternal = false;
        bool        islandConnection = false;
    };

    std::vector<DIPXML_COPPER_POUR> copperPours;
    int                              traceViaPointsRaw = 0;
    int                              traceViaPointsStyleZeroRaw = 0;
    int                              traceViaPointsUniqueNetPos = 0;
    int                              viaComponentCount = 0;
};


std::string ToUtf8( const wxString& aText )
{
    return std::string( aText.utf8_str() );
}


wxString ChildTextByName( const wxXmlNode* aParent, const wxString& aName )
{
    if( !aParent )
        return wxString();

    for( const wxXmlNode* child = aParent->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == aName )
        {
            wxString out;

            for( const wxXmlNode* text = child->GetChildren(); text; text = text->GetNext() )
            {
                if( text->GetType() == wxXML_TEXT_NODE || text->GetType() == wxXML_CDATA_SECTION_NODE )
                    out += text->GetContent();
            }

            out.Trim( true );
            out.Trim( false );
            return out;
        }
    }

    return wxString();
}


bool ParseDoubleAttr( const wxString& aRaw, double& aOut )
{
    wxString tmp = aRaw;
    tmp.Trim( true );
    tmp.Trim( false );

    return !tmp.IsEmpty() && tmp.ToDouble( &aOut );
}


int CardinalDegFromRadians( const wxString& aRadiansRaw )
{
    double radians = 0.0;

    if( !ParseDoubleAttr( aRadiansRaw, radians ) )
        return 0;

    return CardinalDeg( radians * 180.0 / M_PI );
}


int DipLayerIndexFromKiCadLayer( const BOARD& aBoard, PCB_LAYER_ID aLayer )
{
    int copperCount = static_cast<int>( aBoard.GetCopperLayerCount() );

    if( copperCount < 2 )
        return -1;

    if( aLayer == F_Cu )
        return 0;

    if( aLayer == B_Cu )
        return copperCount - 1;

    if( aLayer >= In1_Cu && aLayer <= In30_Cu )
    {
        int innerDelta = static_cast<int>( aLayer - In1_Cu );

        if( innerDelta % 2 != 0 )
            return -1;

        int idx = 1 + innerDelta / 2;
        int maxInnerIdx = copperCount - 2;

        if( idx >= 1 && idx <= maxInnerIdx )
            return idx;
    }

    return -1;
}


bool LoadDipXmlModel( const std::string& aPath, DIPXML_BOARD_MODEL& aOut )
{
    wxXmlDocument doc;

    if( !doc.Load( wxString::FromUTF8( aPath ) ) )
        return false;

    wxXmlNode* root = doc.GetRoot();

    if( !root )
        return false;

    std::set<std::string> traceViaNetPointKeys;

    std::function<void( wxXmlNode* )> walk = [&]( wxXmlNode* node )
    {
        for( ; node; node = node->GetNext() )
        {
            if( node->GetType() == wxXML_ELEMENT_NODE )
            {
                if( node->GetName() == wxT( "PadStyle" ) )
                {
                    std::string styleName = ToUtf8( node->GetAttribute( wxT( "Name" ), wxString() ) );

                    if( !styleName.empty() )
                    {
                        DIPXML_PAD_STYLE style;
                        wxString         type = node->GetAttribute( wxT( "Type" ), wxString() );
                        wxString         holeType = node->GetAttribute( wxT( "HoleType" ), wxString() );
                        style.isSurface = ( type.CmpNoCase( wxT( "Surface" ) ) == 0 );
                        style.isThrough = ( type.CmpNoCase( wxT( "Through" ) ) == 0 );
                        style.isRoundHole = ( holeType.CmpNoCase( wxT( "Round" ) ) == 0 );
                        ParseDoubleAttr( node->GetAttribute( wxT( "Hole" ), wxT( "0" ) ), style.holeMm );
                        aOut.styles[styleName] = style;
                    }
                }
                else if( node->GetName() == wxT( "Pattern" ) )
                {
                    std::string patternStyle = ToUtf8( node->GetAttribute( wxT( "PatternStyle" ), wxString() ) );

                    if( !patternStyle.empty() )
                    {
                        auto&      padMap = aOut.patterns[patternStyle];
                        wxXmlNode* padsNode = nullptr;

                        for( wxXmlNode* child = node->GetChildren(); child; child = child->GetNext() )
                        {
                            if( child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == wxT( "Pads" ) )
                            {
                                padsNode = child;
                                break;
                            }
                        }

                        if( padsNode )
                        {
                            for( wxXmlNode* padNode = padsNode->GetChildren(); padNode; padNode = padNode->GetNext() )
                            {
                                if( padNode->GetType() != wxXML_ELEMENT_NODE || padNode->GetName() != wxT( "Pad" ) )
                                {
                                    continue;
                                }

                                wxString padKey = ChildTextByName( padNode, wxT( "Number" ) );

                                if( padKey.IsEmpty() )
                                    padKey = padNode->GetAttribute( wxT( "Id" ), wxString() );

                                std::string key = ToUtf8( padKey );

                                if( key.empty() )
                                    continue;

                                DIPXML_PATTERN_PAD pad;
                                pad.styleName = ToUtf8( padNode->GetAttribute( wxT( "Style" ), wxString() ) );
                                pad.angleCardinalDeg =
                                        CardinalDegFromRadians( padNode->GetAttribute( wxT( "Angle" ), wxT( "0" ) ) );
                                padMap[key] = pad;
                            }
                        }
                    }
                }
                else if( node->GetName() == wxT( "Component" ) )
                {
                    if( node->GetAttribute( wxT( "Type" ), wxString() ).CmpNoCase( wxT( "Via" ) ) == 0 )
                        aOut.viaComponentCount++;

                    wxString ref = ChildTextByName( node, wxT( "RefDes" ) );

                    if( !ref.IsEmpty() )
                    {
                        std::string refUtf8 = ToUtf8( ref );
                        std::string patternStyle = ToUtf8( node->GetAttribute( wxT( "PatternStyle" ), wxString() ) );
                        int angleCardinal = CardinalDegFromRadians( node->GetAttribute( wxT( "Angle" ), wxT( "0" ) ) );
                        aOut.components.emplace_back( std::move( refUtf8 ), std::move( patternStyle ), angleCardinal );
                    }
                }
                else if( node->GetName() == wxT( "Net" ) )
                {
                    long netId = -1;

                    if( node->GetAttribute( wxT( "Id" ), wxString() ).ToLong( &netId ) )
                    {
                        wxString netName = ChildTextByName( node, wxT( "Name" ) );
                        aOut.netNames[static_cast<int>( netId )] = ToUtf8( netName );

                        for( wxXmlNode* child = node->GetChildren(); child; child = child->GetNext() )
                        {
                            if( child->GetType() != wxXML_ELEMENT_NODE || child->GetName() != wxT( "Traces" ) )
                                continue;

                            for( wxXmlNode* traceNode = child->GetChildren(); traceNode;
                                 traceNode = traceNode->GetNext() )
                            {
                                if( traceNode->GetType() != wxXML_ELEMENT_NODE
                                    || traceNode->GetName() != wxT( "Trace" ) )
                                {
                                    continue;
                                }

                                for( wxXmlNode* traceChild = traceNode->GetChildren(); traceChild;
                                     traceChild = traceChild->GetNext() )
                                {
                                    if( traceChild->GetType() != wxXML_ELEMENT_NODE
                                        || traceChild->GetName() != wxT( "Points" ) )
                                    {
                                        continue;
                                    }

                                    for( wxXmlNode* pointNode = traceChild->GetChildren(); pointNode;
                                         pointNode = pointNode->GetNext() )
                                    {
                                        if( pointNode->GetType() != wxXML_ELEMENT_NODE
                                            || pointNode->GetName() != wxT( "Point" ) )
                                        {
                                            continue;
                                        }

                                        wxString viaStyle = pointNode->GetAttribute( wxT( "ViaStyle" ), wxString() );

                                        if( viaStyle.IsEmpty() )
                                            continue;

                                        aOut.traceViaPointsRaw++;

                                        long viaStyleId = -1;

                                        if( viaStyle.ToLong( &viaStyleId ) && viaStyleId == 0 )
                                            aOut.traceViaPointsStyleZeroRaw++;

                                        std::string key = std::to_string( netId ) + "|"
                                                          + ToUtf8( pointNode->GetAttribute( wxT( "X" ), wxString() ) )
                                                          + "|"
                                                          + ToUtf8( pointNode->GetAttribute( wxT( "Y" ), wxString() ) );
                                        traceViaNetPointKeys.insert( std::move( key ) );
                                    }
                                }
                            }
                        }
                    }
                }
                else if( node->GetName() == wxT( "CopperPour" ) )
                {
                    long   netId = -1;
                    long   lay = -1;
                    double clearance = 0.0;
                    double lineWidth = 0.0;
                    double minimumArea = 0.0;
                    double spokeWidth = 0.0;

                    if( node->GetAttribute( wxT( "NetId" ), wxString() ).ToLong( &netId )
                        && node->GetAttribute( wxT( "Lay" ), wxString() ).ToLong( &lay ) )
                    {
                        ParseDoubleAttr( node->GetAttribute( wxT( "Clearance" ), wxT( "0" ) ), clearance );
                        ParseDoubleAttr( node->GetAttribute( wxT( "LineWidth" ), wxT( "0" ) ), lineWidth );
                        ParseDoubleAttr( node->GetAttribute( wxT( "MinimumArea" ), wxT( "0" ) ), minimumArea );
                        ParseDoubleAttr( node->GetAttribute( wxT( "SpokeWidth" ), wxT( "0" ) ), spokeWidth );

                        long priority = 0;
                        node->GetAttribute( wxT( "Priority" ), wxT( "0" ) ).ToLong( &priority );

                        DIPXML_BOARD_MODEL::DIPXML_COPPER_POUR pour;
                        pour.netId = static_cast<int>( netId );
                        pour.layer = static_cast<int>( lay );
                        pour.priority = static_cast<int>( priority );
                        pour.clearanceMm = clearance;
                        pour.lineWidthMm = lineWidth;
                        pour.minimumAreaMm = minimumArea;
                        pour.spoke = ToUtf8( node->GetAttribute( wxT( "Spoke" ), wxString() ) );
                        pour.spokeWidthMm = spokeWidth;
                        pour.islandRegion =
                                node->GetAttribute( wxT( "IslandRegion" ), wxT( "N" ) ).CmpNoCase( wxT( "Y" ) ) == 0;
                        pour.islandInternal =
                                node->GetAttribute( wxT( "IslandInternal" ), wxT( "N" ) ).CmpNoCase( wxT( "Y" ) ) == 0;
                        pour.islandConnection = node->GetAttribute( wxT( "IslandConnection" ), wxT( "N" ) )
                                                        .CmpNoCase( wxT( "Y" ) )
                                                == 0;
                        aOut.copperPours.push_back( pour );
                    }
                }
            }

            if( node->GetChildren() )
                walk( node->GetChildren() );
        }
    };

    walk( root );
    aOut.traceViaPointsUniqueNetPos = static_cast<int>( traceViaNetPointKeys.size() );
    return true;
}
} // namespace


BOOST_FIXTURE_TEST_SUITE( DipTraceBenchmarks, DIPTRACE_BENCHMARK_FIXTURE )


/**
 * Verify that the Z80 board produces a substantial pad count.
 * The footprints (ICs averaging 14-40 pins and discretes at 2 pins) yield well over 400 pads.
 * Standalone Static Via components are imported as vias rather than single-pad footprints, so
 * they no longer contribute to this count (observed 480 footprint pads after that fix).
 */
BOOST_AUTO_TEST_CASE( TotalPadCount )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
        totalPads += static_cast<int>( fp->Pads().size() );

    BOOST_CHECK_MESSAGE( totalPads > 400, "Z80 board should have >400 total pads, got " + std::to_string( totalPads ) );
}


/**
 * Verify that multi-pin IC footprints have the expected number of pads.
 * The Z80 board should have at least one footprint with >= 14 pads (DIP-14)
 * and ideally one with ~40 pads (the Z80 CPU itself).
 */
BOOST_AUTO_TEST_CASE( MultiPinFootprints )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int maxPads = 0;
    int icCount = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        int padCount = static_cast<int>( fp->Pads().size() );

        if( padCount > maxPads )
            maxPads = padCount;

        if( padCount >= 14 )
            icCount++;
    }

    BOOST_CHECK_MESSAGE( maxPads >= 28, "Z80 board should have a footprint with >=28 pads (Z80 DIP-40), "
                                        "max found: "
                                                + std::to_string( maxPads ) );

    BOOST_CHECK_MESSAGE( icCount >= 5, "Z80 board should have >=5 footprints with >=14 pads (ICs), "
                                       "found: "
                                               + std::to_string( icCount ) );
}


/**
 * Verify that pad dimensions are within reasonable physical bounds.
 * Through-hole pads are typically 1.0-3.0mm diameter.
 * SMD pads range from 0.2mm to ~5mm.
 * A board full of through-hole ICs should have most pads > 0.8mm.
 */
BOOST_AUTO_TEST_CASE( PadDimensionsReasonable )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int totalPads = 0;
    int reasonablePads = 0;
    int tinyPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            totalPads++;

            VECTOR2I size = pad->GetSize( PADSTACK::ALL_LAYERS );
            double   widthMm = pcbIUScale.IUTomm( size.x );
            double   heightMm = pcbIUScale.IUTomm( size.y );
            double   maxDim = std::max( widthMm, heightMm );

            if( maxDim >= 0.8 && maxDim <= 5.0 )
                reasonablePads++;

            if( maxDim < 0.5 )
                tinyPads++;
        }
    }

    BOOST_REQUIRE_GT( totalPads, 0 );

    double reasonablePercent = 100.0 * reasonablePads / totalPads;
    double tinyPercent = 100.0 * tinyPads / totalPads;

    BOOST_CHECK_MESSAGE( reasonablePercent > 50.0,
                         "At least 50% of pads should be 0.8-5.0mm, got " + std::to_string( reasonablePercent ) + "% ("
                                 + std::to_string( reasonablePads ) + "/" + std::to_string( totalPads ) + ")" );

    BOOST_CHECK_MESSAGE( tinyPercent < 20.0,
                         "Less than 20% of pads should be <0.5mm, got " + std::to_string( tinyPercent ) + "% ("
                                 + std::to_string( tinyPads ) + "/" + std::to_string( totalPads ) + ")" );
}


/**
 * Verify DIP pin spacing for multi-pin IC footprints.
 * In a DIP package, adjacent pins on the same side are spaced 2.54mm (100 mil).
 * Find footprints with many pads, sort by position, and check adjacent spacing.
 */
BOOST_AUTO_TEST_CASE( DipPinSpacing )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    bool   foundGoodSpacing = false;
    double toleranceMm = 0.3;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->Pads().size() < 14 )
            continue;

        // Collect pad local positions (relative to footprint origin)
        std::vector<VECTOR2I> positions;

        for( const PAD* pad : fp->Pads() )
        {
            VECTOR2I local = pad->GetPosition() - fp->GetPosition();
            positions.push_back( local );
        }

        // Sort by Y then X to get column ordering
        std::sort( positions.begin(), positions.end(),
                   []( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       if( std::abs( a.x - b.x ) < 100000 )
                           return a.y < b.y;

                       return a.x < b.x;
                   } );

        // Check adjacent spacing within the same column
        for( size_t i = 1; i < positions.size(); i++ )
        {
            if( std::abs( positions[i].x - positions[i - 1].x ) > 100000 )
                continue;

            double spacingMm = pcbIUScale.IUTomm( std::abs( positions[i].y - positions[i - 1].y ) );

            if( std::abs( spacingMm - 2.54 ) < toleranceMm )
            {
                foundGoodSpacing = true;
                break;
            }
        }

        if( foundGoodSpacing )
            break;
    }

    BOOST_CHECK_MESSAGE( foundGoodSpacing, "At least one multi-pin IC should have ~2.54mm DIP pin spacing" );
}


/**
 * Verify that at least some pads have non-zero net assignments.
 * After proper import, pads connected to nets should carry their net code.
 */
BOOST_AUTO_TEST_CASE( PadNetAssignment )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int totalPads = 0;
    int padsWithNets = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            totalPads++;

            if( pad->GetNetCode() > 0 )
                padsWithNets++;
        }
    }

    BOOST_REQUIRE_GT( totalPads, 0 );

    double netPercent = 100.0 * padsWithNets / totalPads;

    BOOST_CHECK_MESSAGE( padsWithNets > 0, "At least some pads should have net assignments, got 0 out of "
                                                   + std::to_string( totalPads ) );

    BOOST_CHECK_MESSAGE( netPercent > 30.0,
                         "At least 30% of pads should have nets, got " + std::to_string( netPercent ) + "%" );
}


/**
 * Verify that specific well-known nets are assigned to pads.
 * GND, VCC/+5V, and bus signals like A0 and D0 should appear on pad net lists.
 */
BOOST_AUTO_TEST_CASE( KnownNetsOnPads )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    std::set<wxString> padNetNames;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            if( pad->GetNetCode() > 0 )
                padNetNames.insert( pad->GetNet()->GetNetname() );
        }
    }

    BOOST_CHECK_MESSAGE( padNetNames.count( wxT( "GND" ) ) > 0, "GND should appear on at least one pad" );

    BOOST_CHECK_MESSAGE( padNetNames.count( wxT( "A0" ) ) > 0, "A0 should appear on at least one pad" );

    BOOST_CHECK_MESSAGE( padNetNames.count( wxT( "D0" ) ) > 0, "D0 should appear on at least one pad" );
}


/**
 * Cross-file consistency: verify that pad parsing produces consistent results
 * across different DipTrace format versions.
 */
BOOST_AUTO_TEST_CASE( CrossVersionPadConsistency )
{
    struct TestCase
    {
        std::string file;
        int         minPads;
        int         componentCount;
    };

    std::vector<TestCase> cases = {
        { "project4.dip", 30, 27 },   { "z80_board.dip", 200, 104 },   { "logic_probe.dip", 100, 113 },
        { "keyboard.dip", 200, 123 }, { "156bus_narrow.dip", 20, 17 },
    };

    for( const TestCase& tc : cases )
    {
        auto board = LoadBoard( tc.file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + tc.file );

        int totalPads = 0;

        for( const FOOTPRINT* fp : board->Footprints() )
            totalPads += static_cast<int>( fp->Pads().size() );

        BOOST_CHECK_MESSAGE( totalPads >= tc.minPads, tc.file + ": expected >=" + std::to_string( tc.minPads )
                                                              + " pads, got " + std::to_string( totalPads ) );
    }
}


/**
 * Verify that track segments are imported with expected counts.
 * The Z80 board is a heavily routed 2-layer design and should have
 * over 1000 track segments.
 */
BOOST_AUTO_TEST_CASE( TrackSegmentCounts )
{
    struct TestCase
    {
        std::string file;
        int         minTracks;
    };

    std::vector<TestCase> cases = {
        { "project4.dip", 100 },    { "156bus_narrow.dip", 30 }, { "z80_board.dip", 1500 },
        { "logic_probe.dip", 400 }, { "keyboard.dip", 400 },
    };

    for( const TestCase& tc : cases )
    {
        auto board = LoadBoard( tc.file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + tc.file );

        int trackCount = 0;

        for( const PCB_TRACK* trk : board->Tracks() )
        {
            if( trk->Type() == PCB_TRACE_T )
                trackCount++;
        }

        BOOST_CHECK_MESSAGE( trackCount >= tc.minTracks, tc.file + ": expected >=" + std::to_string( tc.minTracks )
                                                                 + " tracks, got " + std::to_string( trackCount ) );
    }
}


/**
 * Verify that vias are imported on boards that should have them.
 * The Z80 board uses many vias for layer transitions; simpler boards
 * may have none.
 */
BOOST_AUTO_TEST_CASE( ViaCounts )
{
    struct TestCase
    {
        std::string file;
        int         minVias;
        int         maxVias;
    };

    // After the standalone-via classification fix, single-pad Pad/Fiducial components become
    // footprints instead of bare vias, and Static Via components remain vias. Observed counts
    // stay inside these ranges (project4=63, z80=463, logic_probe=90, 156bus=0, keyboard=20).
    std::vector<TestCase> cases = {
        { "project4.dip", 50, 200 },   { "z80_board.dip", 400, 1000 }, { "logic_probe.dip", 10, 100 },
        { "156bus_narrow.dip", 0, 5 }, { "keyboard.dip", 10, 40 },
    };

    for( const TestCase& tc : cases )
    {
        auto board = LoadBoard( tc.file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + tc.file );

        int viaCount = 0;

        for( const PCB_TRACK* trk : board->Tracks() )
        {
            if( trk->Type() == PCB_VIA_T )
                viaCount++;
        }

        BOOST_CHECK_MESSAGE( viaCount >= tc.minVias, tc.file + ": expected >=" + std::to_string( tc.minVias )
                                                             + " vias, got " + std::to_string( viaCount ) );

        BOOST_CHECK_MESSAGE( viaCount <= tc.maxVias, tc.file + ": expected <=" + std::to_string( tc.maxVias )
                                                             + " vias, got " + std::to_string( viaCount ) );
    }
}


/**
 * Verify track widths and via dimensions are physically reasonable.
 * Track widths should be 0.1-3.0mm for typical PCBs.
 * Via diameters should be 0.3-2.0mm.
 */
BOOST_AUTO_TEST_CASE( TrackAndViaDimensionsReasonable )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int totalTracks = 0;
    int reasonableTracks = 0;
    int totalVias = 0;
    int reasonableVias = 0;

    for( const PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T )
        {
            totalTracks++;
            double widthMm = pcbIUScale.IUTomm( trk->GetWidth() );

            if( widthMm >= 0.1 && widthMm <= 3.0 )
                reasonableTracks++;
        }
        else if( trk->Type() == PCB_VIA_T )
        {
            totalVias++;
            const PCB_VIA* via = static_cast<const PCB_VIA*>( trk );
            double         diamMm = pcbIUScale.IUTomm( via->GetWidth( F_Cu ) );

            if( diamMm >= 0.3 && diamMm <= 2.0 )
                reasonableVias++;
        }
    }

    if( totalTracks > 0 )
    {
        double pct = 100.0 * reasonableTracks / totalTracks;

        BOOST_CHECK_MESSAGE( pct > 90.0, "At least 90% of tracks should have reasonable widths (0.1-3.0mm), got "
                                                 + std::to_string( pct ) + "%" );
    }

    if( totalVias > 0 )
    {
        double pct = 100.0 * reasonableVias / totalVias;

        BOOST_CHECK_MESSAGE( pct > 90.0, "At least 90% of vias should have reasonable diameters (0.3-2.0mm), got "
                                                 + std::to_string( pct ) + "%" );
    }
}


/**
 * Verify that tracks carry valid net assignments.
 * All tracks in a properly imported board should have net codes > 0.
 */
BOOST_AUTO_TEST_CASE( TrackNetAssignment )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int                totalTracks = 0;
    int                tracksWithNets = 0;
    std::set<wxString> trackNetNames;

    for( const PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T )
        {
            totalTracks++;

            if( trk->GetNetCode() > 0 )
            {
                tracksWithNets++;
                trackNetNames.insert( trk->GetNet()->GetNetname() );
            }
        }
    }

    BOOST_REQUIRE_GT( totalTracks, 0 );

    double netPct = 100.0 * tracksWithNets / totalTracks;

    BOOST_CHECK_MESSAGE( netPct > 90.0,
                         "At least 90% of tracks should have net assignments, got " + std::to_string( netPct ) + "%" );

    BOOST_CHECK_MESSAGE( trackNetNames.count( wxT( "GND" ) ) > 0, "GND net should appear on tracks" );

    BOOST_CHECK_MESSAGE( trackNetNames.count( wxT( "A0" ) ) > 0, "A0 net should appear on tracks" );
}


/**
 * Verify that copper zones are imported with expected counts per board.
 */
BOOST_AUTO_TEST_CASE( ZoneCounts )
{
    struct TestCase
    {
        std::string file;
        int         expected;
    };

    std::vector<TestCase> cases = {
        { "project4.dip", 0 },    { "156bus_narrow.dip", 1 }, { "z80_board.dip", 2 },
        { "logic_probe.dip", 2 }, { "keyboard.dip", 1 },
    };

    for( const TestCase& tc : cases )
    {
        auto board = LoadBoard( tc.file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + tc.file );

        int zoneCount = static_cast<int>( board->Zones().size() );

        BOOST_CHECK_MESSAGE( zoneCount == tc.expected, tc.file + ": expected " + std::to_string( tc.expected )
                                                               + " zones, got " + std::to_string( zoneCount ) );
    }
}


/**
 * Verify that zones carry valid net assignments.
 * GND pour zones should reference the GND net.
 */
BOOST_AUTO_TEST_CASE( ZoneNetAssignment )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int                zonesWithNets = 0;
    std::set<wxString> zoneNetNames;

    for( const ZONE* zone : board->Zones() )
    {
        if( zone->GetNetCode() > 0 )
        {
            zonesWithNets++;
            zoneNetNames.insert( zone->GetNet()->GetNetname() );
        }
    }

    BOOST_CHECK_MESSAGE( zonesWithNets == static_cast<int>( board->Zones().size() ),
                         "All zones should have net assignments, got " + std::to_string( zonesWithNets ) + "/"
                                 + std::to_string( board->Zones().size() ) );

    BOOST_CHECK_MESSAGE( zoneNetNames.size() >= 1, "At least 1 distinct net should appear on zones" );
}


/**
 * Verify that zone outlines have reasonable vertex counts and physical dimensions.
 * A simple rectangular pour has 4+ vertices; complex board-edge-following pours have more.
 * Zone extent should be at least a few mm in both directions.
 */
BOOST_AUTO_TEST_CASE( ZoneOutlineDimensions )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE_GT( board->Zones().size(), 0u );

    for( const ZONE* zone : board->Zones() )
    {
        const SHAPE_POLY_SET* outline = zone->Outline();
        BOOST_REQUIRE( outline );
        BOOST_REQUIRE_GT( outline->OutlineCount(), 0 );

        int vertexCount = outline->COutline( 0 ).PointCount();

        BOOST_CHECK_MESSAGE( vertexCount >= 3,
                             "Zone outline should have at least 3 vertices, got " + std::to_string( vertexCount ) );

        BOX2I  bbox = outline->BBox();
        double widthMm = pcbIUScale.IUTomm( bbox.GetWidth() );
        double heightMm = pcbIUScale.IUTomm( bbox.GetHeight() );

        BOOST_CHECK_MESSAGE( widthMm > 5.0, "Zone width should be >5mm, got " + std::to_string( widthMm ) + "mm" );

        BOOST_CHECK_MESSAGE( heightMm > 5.0, "Zone height should be >5mm, got " + std::to_string( heightMm ) + "mm" );
    }
}


/**
 * Verify that zones are assigned to valid copper layers.
 */
BOOST_AUTO_TEST_CASE( ZoneLayerAssignment )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    for( const ZONE* zone : board->Zones() )
    {
        PCB_LAYER_ID layer = zone->GetFirstLayer();

        BOOST_CHECK_MESSAGE( IsCopperLayer( layer ), "Zone should be on a copper layer, got layer "
                                                             + std::to_string( static_cast<int>( layer ) ) );
    }
}


/**
 * Verify that footprint graphics (silkscreen outlines) are imported.
 * DIP and axial packages should have outline shapes on their footprints.
 */
BOOST_AUTO_TEST_CASE( FootprintGraphics )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int footprintsWithGraphics = 0;
    int totalGraphics = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        int graphicCount = 0;

        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                graphicCount++;
        }

        if( graphicCount > 0 )
        {
            footprintsWithGraphics++;
            totalGraphics += graphicCount;
        }
    }

    BOOST_CHECK_MESSAGE( footprintsWithGraphics > 0, "At least some footprints should have outline graphics, got 0" );

    BOOST_CHECK_MESSAGE( totalGraphics > 10,
                         "Board should have >10 total footprint graphics, got " + std::to_string( totalGraphics ) );
}


/**
 * Verify that footprint outline dimensions are physically reasonable.
 * The bounding box of footprint graphics should be larger than the pad
 * extent or at least comparable in size.
 */
BOOST_AUTO_TEST_CASE( FootprintGraphicsDimensions )
{
    auto board = LoadBoard( "z80_board.dip" );
    BOOST_REQUIRE( board );

    int reasonableCount = 0;
    int tinyCount = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        bool hasGraphics = false;

        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
            {
                hasGraphics = true;
                break;
            }
        }

        if( !hasGraphics )
            continue;

        BOX2I gfxBbox;
        bool  first = true;

        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
            {
                const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

                if( first )
                {
                    gfxBbox = shape->GetBoundingBox();
                    first = false;
                }
                else
                {
                    gfxBbox.Merge( shape->GetBoundingBox() );
                }
            }
        }

        double widthMm = pcbIUScale.IUTomm( gfxBbox.GetWidth() );
        double heightMm = pcbIUScale.IUTomm( gfxBbox.GetHeight() );
        double maxDim = std::max( widthMm, heightMm );

        if( maxDim >= 2.0 && maxDim <= 80.0 )
            reasonableCount++;
        else if( maxDim < 0.5 )
            tinyCount++;
    }

    BOOST_CHECK_MESSAGE( reasonableCount > 0, "At least some footprints should have reasonably-sized "
                                              "outline graphics (2-80mm)" );

    BOOST_CHECK_MESSAGE( tinyCount == 0,
                         "No footprint graphics should be tiny (<0.5mm), got " + std::to_string( tinyCount ) );
}


/**
 * Every imported Edge.Cuts endpoint should connect to another Edge.Cuts endpoint.
 * This validates that the board outline is contiguous and does not contain open ends.
 */
BOOST_AUTO_TEST_CASE( EdgeCutsOutlineConnectivity )
{
    const std::vector<std::string> files = {
        "project4.dip", "156bus_narrow.dip", "z80_board.dip", "logic_probe.dip", "keyboard.dip",
    };

    for( const std::string& file : files )
    {
        auto board = LoadBoard( file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + file );

        int totalEndpoints = 0;
        int disconnected = CountDisconnectedEdgeCutsEndpoints( *board, totalEndpoints );

        BOOST_CHECK_MESSAGE( totalEndpoints > 0, file + ": expected Edge.Cuts endpoints, got 0" );

        BOOST_CHECK_MESSAGE( disconnected == 0,
                             file + ": expected contiguous outline; found " + std::to_string( disconnected )
                                     + " disconnected endpoints out of " + std::to_string( totalEndpoints ) );
    }
}


/**
 * Trace endpoints should attach to at least one additional anchor on the same net
 * (another trace endpoint, a via, or a pad). This catches broken chain linking.
 */
BOOST_AUTO_TEST_CASE( TraceEndpointConnectivity )
{
    const std::vector<std::string> files = {
        "z80_board.dip",
        "logic_probe.dip",
        "keyboard.dip",
    };

    for( const std::string& file : files )
    {
        auto board = LoadBoard( file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + file );

        int totalEndpoints = 0;
        int disconnected = CountDisconnectedTraceEndpoints( *board, totalEndpoints );
        BOOST_REQUIRE_MESSAGE( totalEndpoints > 0, file + ": expected routed trace endpoints, got 0" );

        double disconnectedPct = 100.0 * disconnected / totalEndpoints;

        BOOST_CHECK_MESSAGE( disconnectedPct <= 15.0,
                             file + ": disconnected trace endpoints = " + std::to_string( disconnected ) + "/"
                                     + std::to_string( totalEndpoints ) + " (" + std::to_string( disconnectedPct )
                                     + "%)" );
    }
}


/**
 * Sanity-check imported 0805 packages: they should be 2-pad SMD footprints with
 * reasonable rectangular/rounded pad shapes and dimensions.
 */
BOOST_AUTO_TEST_CASE( Smd0805PadSanity )
{
    const std::vector<std::string> files = {
        "logic_probe.dip",
        "156bus_narrow.dip",
    };

    int footprints0805 = 0;
    int valid0805 = 0;

    for( const std::string& file : files )
    {
        auto board = LoadBoard( file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + file );

        for( const FOOTPRINT* fp : board->Footprints() )
        {
            wxString fpName = wxString::FromUTF8( fp->GetFPID().GetLibItemName() ).Upper();

            if( !fpName.Contains( "0805" ) )
                continue;

            footprints0805++;

            if( fp->Pads().size() != 2 )
                continue;

            bool                  padsValid = true;
            std::vector<VECTOR2I> padPos;

            for( const PAD* pad : fp->Pads() )
            {
                if( pad->GetAttribute() != PAD_ATTRIB::SMD )
                {
                    padsValid = false;
                    break;
                }

                PAD_SHAPE shape = pad->GetShape( PADSTACK::ALL_LAYERS );

                if( !IsRectLikeSmdPadShape( shape ) )
                {
                    padsValid = false;
                    break;
                }

                VECTOR2I size = pad->GetSize( PADSTACK::ALL_LAYERS );
                double   widthMm = pcbIUScale.IUTomm( size.x );
                double   heightMm = pcbIUScale.IUTomm( size.y );

                if( widthMm < 0.2 || widthMm > 2.5 || heightMm < 0.2 || heightMm > 2.5 )
                {
                    padsValid = false;
                    break;
                }

                padPos.push_back( pad->GetPosition() );
            }

            if( padsValid )
            {
                double pitchMm = pcbIUScale.IUTomm( ( padPos[0] - padPos[1] ).EuclideanNorm() );

                if( pitchMm < 0.5 || pitchMm > 3.5 )
                    padsValid = false;
            }

            if( padsValid )
                valid0805++;
        }
    }

    BOOST_CHECK_MESSAGE( footprints0805 >= 3,
                         "Expected at least 3 imported 0805 footprints, got " + std::to_string( footprints0805 ) );

    BOOST_CHECK_MESSAGE( valid0805 == footprints0805,
                         "All imported 0805 footprints should satisfy SMD/pad-shape/pitch sanity; "
                         "valid=" + std::to_string( valid0805 )
                                 + ", total=" + std::to_string( footprints0805 ) );
}


BOOST_AUTO_TEST_CASE( ImportedViasDoNotShortDifferentNets )
{
    const std::vector<std::string> files = {
        "project4.dip",
        "z80_board.dip",
        "logic_probe.dip",
        "keyboard.dip",
    };

    for( const std::string& file : files )
    {
        auto board = LoadBoard( file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + file );

        int                      checkedVias = 0;
        std::vector<std::string> shortReports;
        int                      shortedVias = CountViaNetShorts( *board, checkedVias, &shortReports );

        for( const std::string& report : shortReports )
            BOOST_TEST_MESSAGE( file + ": " + report );

        BOOST_CHECK_MESSAGE( shortedVias == 0, file + ": vias shorting distinct nets = " + std::to_string( shortedVias )
                                                       + " out of " + std::to_string( checkedVias ) + " vias" );
    }
}


BOOST_AUTO_TEST_CASE( FootprintPadsInsideBoardOutline )
{
    const std::vector<std::string> files = {
        "project4.dip",
        "z80_board.dip",
        "logic_probe.dip",
        "keyboard.dip",
    };

    for( const std::string& file : files )
    {
        auto board = LoadBoard( file );
        BOOST_REQUIRE_MESSAGE( board, "Failed to load " + file );

        int  totalPads = 0;
        bool hasOutline = false;
        int  outsidePads = CountPadsOutsideBoardOutline( *board, totalPads, hasOutline );

        BOOST_REQUIRE_MESSAGE( hasOutline, file + ": board outline not available" );
        BOOST_REQUIRE_MESSAGE( totalPads > 0, file + ": no pads found" );

        if( outsidePads > 0 )
        {
            SHAPE_POLY_SET outline;
            board->GetBoardPolygonOutlines( outline, true );
            int reported = 0;

            for( const FOOTPRINT* fp : board->Footprints() )
            {
                for( const PAD* pad : fp->Pads() )
                {
                    if( outline.Contains( pad->GetPosition(), -1, CONNECT_TOL_NM ) )
                        continue;

                    BOOST_TEST_MESSAGE( file + ": outside pad " + std::string( fp->GetReference().utf8_str() ) + ":"
                                        + std::string( pad->GetNumber().utf8_str() ) + " at ("
                                        + std::to_string( pad->GetPosition().x ) + ","
                                        + std::to_string( pad->GetPosition().y ) + ")" );

                    if( ++reported >= 20 )
                        break;
                }

                if( reported >= 20 )
                    break;
            }
        }

        BOOST_CHECK_MESSAGE( outsidePads == 0, file + ": pads outside board outline = " + std::to_string( outsidePads )
                                                       + "/" + std::to_string( totalPads ) );
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesOptional )
{
    const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );
    std::string examplesDir =
            examplesEnv && *examplesEnv ? examplesEnv : "/home/seth/Downloads/DipTrace Viewer/Examples";

    if( !std::filesystem::exists( examplesDir ) )
    {
        BOOST_TEST_MESSAGE( "Viewer examples path not found; skipping ViewerExamplesOptional" );
        return;
    }

    auto pcb2 = LoadBoardFromPath( examplesDir + "/PCB_2.dip" );
    BOOST_REQUIRE( pcb2 );
    BOOST_CHECK_EQUAL( pcb2->GetCopperLayerCount(), 2 );

    int pcb2Tracks = 0;
    int pcb2Vias = 0;

    for( const PCB_TRACK* trk : pcb2->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
            pcb2Tracks++;
        else if( trk->Type() == PCB_VIA_T )
            pcb2Vias++;
    }

    BOOST_CHECK_MESSAGE( pcb2Vias <= pcb2Tracks,
                         "PCB_2: via count should not exceed segment count; tracks=" + std::to_string( pcb2Tracks )
                                 + ", vias=" + std::to_string( pcb2Vias ) );

    int viasWithDrill = 0;
    int viasAt191Mil = 0;

    for( const PCB_TRACK* trk : pcb2->Tracks() )
    {
        if( trk->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( trk );
        int            drillIU = via->GetDrillValue();

        if( drillIU <= 0 )
            continue;

        viasWithDrill++;

        double drillMm = pcbIUScale.IUTomm( drillIU );
        double targetMm = 19.1 * 0.0254;

        if( std::abs( drillMm - targetMm ) <= 0.02 ) // ~0.8 mil tolerance
            viasAt191Mil++;
    }

    BOOST_REQUIRE_MESSAGE( viasWithDrill > 0, "PCB_2: expected vias with non-zero drill" );
    BOOST_CHECK_MESSAGE( viasAt191Mil == viasWithDrill,
                         "PCB_2: vias with 19.1mil drill = " + std::to_string( viasAt191Mil ) + "/"
                                 + std::to_string( viasWithDrill ) );

    int pcb2ShortVias = 0;
    int pcb2CheckedVias = 0;
    pcb2ShortVias = CountViaNetShorts( *pcb2, pcb2CheckedVias );
    BOOST_CHECK_MESSAGE( pcb2ShortVias == 0,
                         "PCB_2: vias shorting distinct nets = " + std::to_string( pcb2ShortVias ) );

    const ZONE* pcb2BottomZone = nullptr;

    for( const ZONE* zone : pcb2->Zones() )
    {
        if( zone->GetLayer() == B_Cu )
        {
            pcb2BottomZone = zone;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( pcb2BottomZone, "PCB_2: expected a B.Cu copper zone" );

    wxString pcb2ZoneNet = pcb2BottomZone->GetNet() ? pcb2BottomZone->GetNet()->GetNetname() : wxString();

    BOOST_CHECK_MESSAGE( pcb2ZoneNet == wxString( "Net 7" ), "PCB_2: B.Cu zone net should be 'Net 7', got '"
                                                                     + std::string( pcb2ZoneNet.utf8_str() ) + "'" );
    BOOST_CHECK( pcb2BottomZone->GetPadConnection() == ZONE_CONNECTION::THERMAL );
    BOOST_CHECK_SMALL( std::abs( pcbIUScale.IUTomm( pcb2BottomZone->GetThermalReliefSpokeWidth() ) - 0.3303 ), 0.03 );

    int pcb2Net7PthPads = 0;
    int pcb2Net7At90 = 0;

    for( const FOOTPRINT* fp : pcb2->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                continue;

            if( !pad->GetNet() || pad->GetNet()->GetNetname() != wxString( "Net 7" ) )
                continue;

            pcb2Net7PthPads++;

            if( ( NormalizeDeg( pad->GetThermalSpokeAngle().AsDegrees() ) % 180 ) == 90 )
                pcb2Net7At90++;
        }
    }

    BOOST_REQUIRE_GT( pcb2Net7PthPads, 0 );
    BOOST_CHECK_EQUAL( pcb2Net7At90, pcb2Net7PthPads );

    int  pcb2Pads = 0;
    bool pcb2HasOutline = false;
    int  pcb2OutsidePads = CountPadsOutsideBoardOutline( *pcb2, pcb2Pads, pcb2HasOutline );
    BOOST_REQUIRE( pcb2HasOutline );

    if( pcb2OutsidePads > 0 )
    {
        SHAPE_POLY_SET outline;
        pcb2->GetBoardPolygonOutlines( outline, true );
        int reported = 0;

        for( const FOOTPRINT* fp : pcb2->Footprints() )
        {
            for( const PAD* pad : fp->Pads() )
            {
                if( !outline.Contains( pad->GetPosition(), -1, CONNECT_TOL_NM ) )
                {
                    BOOST_TEST_MESSAGE( "PCB_2 outside pad: " + std::string( fp->GetReference().utf8_str() ) + ":"
                                        + std::string( pad->GetNumber().utf8_str() )
                                        + " fpLayer=" + std::string( pcb2->GetLayerName( fp->GetLayer() ).utf8_str() )
                                        + " fpOrient=" + std::to_string( fp->GetOrientation().AsDegrees() ) + " pad=("
                                        + std::to_string( pad->GetPosition().x ) + ","
                                        + std::to_string( pad->GetPosition().y ) + ")" );
                    reported++;

                    if( reported >= 20 )
                        break;
                }
            }

            if( reported >= 20 )
                break;
        }
    }

    BOOST_CHECK_MESSAGE( pcb2OutsidePads == 0, "PCB_2: pads outside outline = " + std::to_string( pcb2OutsidePads ) );

    const FOOTPRINT* j1 = FindFootprintByRef( *pcb2, wxT( "J1" ) );
    const FOOTPRINT* j4 = FindFootprintByRef( *pcb2, wxT( "J4" ) );
    const FOOTPRINT* j7 = FindFootprintByRef( *pcb2, wxT( "J7" ) );
    const FOOTPRINT* j8 = FindFootprintByRef( *pcb2, wxT( "J8" ) );
    const FOOTPRINT* j10 = FindFootprintByRef( *pcb2, wxT( "J10" ) );
    const FOOTPRINT* u2 = FindFootprintByRef( *pcb2, wxT( "U2" ) );
    const FOOTPRINT* u3 = FindFootprintByRef( *pcb2, wxT( "U3" ) );

    BOOST_REQUIRE_MESSAGE( j1, "PCB_2: footprint J1 not found" );
    BOOST_REQUIRE_MESSAGE( j4, "PCB_2: footprint J4 not found" );
    BOOST_REQUIRE_MESSAGE( j7, "PCB_2: footprint J7 not found" );
    BOOST_REQUIRE_MESSAGE( j8, "PCB_2: footprint J8 not found" );
    BOOST_REQUIRE_MESSAGE( j10, "PCB_2: footprint J10 not found" );
    BOOST_REQUIRE_MESSAGE( u2, "PCB_2: footprint U2 not found" );
    BOOST_REQUIRE_MESSAGE( u3, "PCB_2: footprint U3 not found" );

    BOOST_CHECK_EQUAL( CardinalDeg( j4->GetOrientation().AsDegrees() ), 90 );
    BOOST_CHECK_EQUAL( CardinalDeg( j7->GetOrientation().AsDegrees() ), 90 );
    BOOST_CHECK_EQUAL( CardinalDeg( j10->GetOrientation().AsDegrees() ), 90 );
    BOOST_CHECK_EQUAL( CardinalDeg( u2->GetOrientation().AsDegrees() ), 0 );
    BOOST_CHECK_EQUAL( CardinalDeg( u3->GetOrientation().AsDegrees() ), 180 );

    int j4SilkSegments = 0;

    for( const BOARD_ITEM* item : j4->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( ( shape->GetLayer() == F_SilkS || shape->GetLayer() == B_SilkS ) && shape->GetShape() == SHAPE_T::SEGMENT )
        {
            j4SilkSegments++;
        }
    }

    BOOST_CHECK_MESSAGE( j4SilkSegments >= 4, "PCB_2: J4 should import as a silkscreen box; silk segments="
                                                      + std::to_string( j4SilkSegments ) );

    int j1Pads = 0;

    for( const PAD* pad : j1->Pads() )
    {
        j1Pads++;
        BOOST_CHECK_MESSAGE( pad->GetAttribute() == PAD_ATTRIB::SMD,
                             "PCB_2: J1 pad should be SMD, got attribute="
                                     + std::to_string( static_cast<int>( pad->GetAttribute() ) ) );
    }

    BOOST_REQUIRE_GT( j1Pads, 0 );

    int j8Pads = 0;

    for( const PAD* pad : j8->Pads() )
    {
        j8Pads++;
        VECTOR2I drill = pad->GetDrillSize();

        BOOST_CHECK_MESSAGE( pad->GetAttribute() == PAD_ATTRIB::PTH, "PCB_2: J8 pad should be through-hole" );
        BOOST_CHECK_MESSAGE( pad->GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE,
                             "PCB_2: J8 pad drill should be circular" );
        BOOST_CHECK_EQUAL( drill.x, drill.y );
    }

    BOOST_REQUIRE_GT( j8Pads, 0 );

    int u2PadsExpectedRotated = 0;

    for( const PAD* pad : u2->Pads() )
    {
        long padNumLong = 0;

        if( !pad->GetNumber().ToLong( &padNumLong ) )
            continue;

        int padNum = static_cast<int>( padNumLong );

        if( ( padNum >= 12 && padNum <= 22 ) || ( padNum >= 34 && padNum <= 44 ) )
        {
            u2PadsExpectedRotated++;

            int padDeg = CardinalDeg( pad->GetOrientation().AsDegrees() );
            BOOST_CHECK_MESSAGE( ( padDeg % 180 ) == 90, "PCB_2: U2 pad " + std::to_string( padNum )
                                                                 + " should be rotated by 90 degrees (got "
                                                                 + std::to_string( padDeg ) + ")" );
        }
    }

    BOOST_CHECK_EQUAL( u2PadsExpectedRotated, 22 );

    auto pcb4 = LoadBoardFromPath( examplesDir + "/PCB_4.dip" );
    BOOST_REQUIRE( pcb4 );
    BOOST_CHECK_GT( pcb4->Footprints().size(), 0u );
    BOOST_CHECK_EQUAL( pcb4->GetCopperLayerCount(), 2 );

    auto pcb6 = LoadBoardFromPath( examplesDir + "/PCB_6.dip" );
    BOOST_REQUIRE( pcb6 );
    BOOST_CHECK_EQUAL( pcb6->GetCopperLayerCount(), 4 );
    BOOST_CHECK_EQUAL( pcb6->GetLayerName( F_Cu ), wxString( "Top" ) );
    BOOST_CHECK_EQUAL( pcb6->GetLayerName( In1_Cu ), wxString( "Gnd" ) );
    BOOST_CHECK_EQUAL( pcb6->GetLayerName( In2_Cu ), wxString( "Pwr" ) );
    BOOST_CHECK_EQUAL( pcb6->GetLayerName( B_Cu ), wxString( "Bottom" ) );

    // 5 stored CopperPours plus 2 synthesized fills for the Gnd (Lay1) and Pwr (Lay2) plane
    // layers, which DipTrace describes at the layer level rather than as pours.
    BOOST_CHECK_EQUAL( pcb6->Zones().size(), 7u );

    int pwrZones = 0;
    int pwr3V3 = 0;
    int pwr5V = 0;
    int pwrVCC = 0;
    int gndInnerZones = 0;

    for( const ZONE* zone : pcb6->Zones() )
    {
        wxString netName = zone->GetNet() ? zone->GetNet()->GetNetname() : wxString();

        if( zone->GetLayer() == In2_Cu )
        {
            pwrZones++;

            if( netName == wxString( "3V3" ) )
                pwr3V3++;
            else if( netName == wxString( "5V" ) )
                pwr5V++;
            else if( netName == wxString( "VCC" ) )
                pwrVCC++;
        }
        else if( zone->GetLayer() == In1_Cu && netName == wxString( "GND" ) )
        {
            gndInnerZones++;
        }
    }

    // In2_Cu (Pwr plane) carries 4 stored pours plus the synthesized Pwr plane fill (net 3V3);
    // In1_Cu (Gnd plane) carries 1 stored GND pour plus the synthesized GND plane fill.
    BOOST_CHECK_EQUAL( pwrZones, 5 );
    BOOST_CHECK_EQUAL( pwr3V3, 2 );
    BOOST_CHECK_EQUAL( pwr5V, 2 );
    BOOST_CHECK_EQUAL( pwrVCC, 1 );
    BOOST_CHECK_EQUAL( gndInnerZones, 2 );

    int pcb6ThermalZones = 0;
    int pcb6SpokeWidthMatches = 0;

    for( const ZONE* zone : pcb6->Zones() )
    {
        if( zone->GetPadConnection() == ZONE_CONNECTION::THERMAL )
            pcb6ThermalZones++;

        if( std::abs( pcbIUScale.IUTomm( zone->GetThermalReliefSpokeWidth() ) - 0.33 ) <= 0.03 )
            pcb6SpokeWidthMatches++;
    }

    // 5 stored pours + 2 synthesized plane fills, all defaulting to thermal pad connection.
    BOOST_CHECK_EQUAL( pcb6ThermalZones, 7 );
    BOOST_CHECK_EQUAL( pcb6SpokeWidthMatches, 5 );

    int pcb6PowerNetPthPads = 0;
    int pcb6PowerNetPadsAt45 = 0;

    for( const FOOTPRINT* fp : pcb6->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD || !pad->GetNet() )
                continue;

            wxString netName = pad->GetNet()->GetNetname();

            if( netName != wxString( "GND" ) && netName != wxString( "3V3" )
                && netName != wxString( "5V" ) && netName != wxString( "VCC" ) )
            {
                continue;
            }

            pcb6PowerNetPthPads++;

            if( ( NormalizeDeg( pad->GetThermalSpokeAngle().AsDegrees() ) % 180 ) == 45 )
                pcb6PowerNetPadsAt45++;
        }
    }

    BOOST_REQUIRE_GT( pcb6PowerNetPthPads, 0 );
    BOOST_CHECK_EQUAL( pcb6PowerNetPadsAt45, pcb6PowerNetPthPads );

    const FOOTPRINT* ic4 = FindFootprintByRef( *pcb6, wxT( "IC4" ) );
    BOOST_REQUIRE_MESSAGE( ic4, "PCB_6: footprint IC4 not found" );
    BOOST_CHECK_EQUAL( CardinalDeg( ic4->GetOrientation().AsDegrees() ), 90 );

    const FOOTPRINT* q4 = FindFootprintByRef( *pcb6, wxT( "Q4" ) );
    BOOST_REQUIRE_MESSAGE( q4, "PCB_6: footprint Q4 not found" );
    BOOST_CHECK_EQUAL( q4->Pads().size(), 3u );

    int q4SilkSegments = 0;
    int q4SilkArcs = 0;

    for( const BOARD_ITEM* item : q4->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

        if( shape->GetLayer() != F_SilkS && shape->GetLayer() != B_SilkS )
            continue;

        if( shape->GetShape() == SHAPE_T::SEGMENT )
            q4SilkSegments++;
        else if( shape->GetShape() == SHAPE_T::ARC )
            q4SilkArcs++;
    }

    BOOST_CHECK_MESSAGE( q4SilkSegments == 4,
                         "PCB_6: Q4 should have 4 silkscreen segments, got " + std::to_string( q4SilkSegments ) );
    BOOST_CHECK_MESSAGE( q4SilkArcs == 1,
                         "PCB_6: Q4 should have 1 silkscreen arc, got " + std::to_string( q4SilkArcs ) );

    const FOOTPRINT* u10 = FindFootprintByRef( *pcb6, wxT( "U10" ) );
    BOOST_REQUIRE_MESSAGE( u10, "PCB_6: footprint U10 not found" );

    int                   u10NpthPads = 0;
    int                   u10Drill508 = 0;
    int                   u10Outer5747 = 0;
    std::vector<VECTOR2I> u10NpthLocals;

    for( const PAD* pad : u10->Pads() )
    {
        if( pad->GetAttribute() != PAD_ATTRIB::NPTH )
            continue;

        u10NpthPads++;
        u10NpthLocals.push_back( pad->GetPosition() - u10->GetPosition() );

        VECTOR2I drill = pad->GetDrillSize();
        VECTOR2I size = pad->GetSize( PADSTACK::ALL_LAYERS );

        BOOST_CHECK_MESSAGE( pad->GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE,
                             "PCB_6: U10 NPTH drill should be circular" );
        BOOST_CHECK_EQUAL( drill.x, drill.y );
        BOOST_CHECK_EQUAL( size.x, size.y );
        BOOST_CHECK_MESSAGE( pad->GetNetCode() <= 0, "PCB_6: U10 NPTH should not have a net assignment" );

        double drillMm = pcbIUScale.IUTomm( drill.x );
        double outerMm = pcbIUScale.IUTomm( size.x );

        if( std::abs( drillMm - 5.08 ) <= 0.02 )
            u10Drill508++;

        if( std::abs( outerMm - 5.7467 ) <= 0.03 )
            u10Outer5747++;
    }

    BOOST_CHECK_EQUAL( u10NpthPads, 2 );
    BOOST_CHECK_EQUAL( u10Drill508, 2 );
    BOOST_CHECK_EQUAL( u10Outer5747, 2 );
    BOOST_REQUIRE_EQUAL( u10NpthLocals.size(), 2u );

    VECTOR2I sym = u10NpthLocals[0] + u10NpthLocals[1];
    int      symTol = pcbIUScale.mmToIU( 0.05 );

    BOOST_CHECK_SMALL( std::abs( sym.x ), symTol );
    BOOST_CHECK_SMALL( std::abs( sym.y ), symTol );

    double holeSpanMm = pcbIUScale.IUTomm( ( u10NpthLocals[0] - u10NpthLocals[1] ).EuclideanNorm() );
    BOOST_CHECK_SMALL( std::abs( holeSpanMm - 24.9934 ), 0.05 );

    auto cnc = LoadBoardFromPath( examplesDir + "/CNC_controller.dip" );
    BOOST_REQUIRE( cnc );
    BOOST_CHECK_EQUAL( cnc->GetCopperLayerCount(), 2 );
    BOOST_CHECK_GT( cnc->Footprints().size(), 120u );
    BOOST_CHECK_LT( cnc->Footprints().size(), 200u );
    BOOST_CHECK_EQUAL( cnc->Zones().size(), 5u );

    int cncViaCount = 0;

    for( const PCB_TRACK* trk : cnc->Tracks() )
    {
        if( trk->Type() == PCB_VIA_T )
            cncViaCount++;
    }

    BOOST_CHECK_GT( cncViaCount, 300 );

    int cncFull = 0;
    int cncThtThermal = 0;

    for( const ZONE* zone : cnc->Zones() )
    {
        if( zone->GetPadConnection() == ZONE_CONNECTION::FULL )
            cncFull++;
        else if( zone->GetPadConnection() == ZONE_CONNECTION::THT_THERMAL )
            cncThtThermal++;
    }

    BOOST_CHECK_EQUAL( cncFull, 3 );
    BOOST_CHECK_GE( cncThtThermal, 1 );
}


BOOST_AUTO_TEST_CASE( ViewerExamplesDipXmlParityOptional )
{
    const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );
    std::string examplesDir =
            examplesEnv && *examplesEnv ? examplesEnv : "/home/seth/Downloads/DipTrace Viewer/Examples";

    if( !std::filesystem::exists( examplesDir ) )
    {
        BOOST_TEST_MESSAGE( "Viewer examples path not found; skipping ViewerExamplesDipXmlParityOptional" );
        return;
    }

    struct SAMPLE
    {
        const char* dip;
        const char* dipxml;
        int         minComparedFootprints;
        int         minComparedPads;
    };

    static const std::array<SAMPLE, 3> samples = { {
            { "PCB_2.dip", "PCB_2.dipxml", 60, 200 },
            { "PCB_4.dip", "PCB_4.dipxml", 60, 200 },
            { "PCB_6.dip", "PCB_6.dipxml", 100, 700 },
    } };

    for( const SAMPLE& sample : samples )
    {
        std::string dipPath = examplesDir + "/" + sample.dip;
        std::string xmlPath = examplesDir + "/" + sample.dipxml;

        if( !std::filesystem::exists( dipPath ) || !std::filesystem::exists( xmlPath ) )
        {
            BOOST_TEST_MESSAGE( "Skipping " + std::string( sample.dip ) + " parity check; missing .dip or .dipxml" );
            continue;
        }

        DIPXML_BOARD_MODEL model;
        BOOST_REQUIRE_MESSAGE( LoadDipXmlModel( xmlPath, model ), "Failed to load DipXML model: " + xmlPath );

        auto board = LoadBoardFromPath( dipPath );
        BOOST_REQUIRE( board );

        int importedViaCount = 0;

        for( const PCB_TRACK* trk : board->Tracks() )
        {
            if( trk->Type() == PCB_VIA_T )
                importedViaCount++;
        }

        int expectedViaCount = model.traceViaPointsUniqueNetPos + model.viaComponentCount;

        BOOST_CHECK_MESSAGE( importedViaCount == expectedViaCount,
                             std::string( sample.dip ) + ": via parity mismatch; imported="
                                     + std::to_string( importedViaCount )
                                     + " expected(trace-via unique net+xy="
                                     + std::to_string( model.traceViaPointsUniqueNetPos )
                                     + ", standalone via components=" + std::to_string( model.viaComponentCount )
                                     + ", trace-via raw points=" + std::to_string( model.traceViaPointsRaw ) + ")" );

        int                      comparedFootprints = 0;
        int                      footprintAngleMismatches = 0;
        int                      comparedPads = 0;
        int                      padAngleMismatches = 0;
        int                      padTypeMismatches = 0;
        int                      padDrillMismatches = 0;
        int                      missingPatternPads = 0;
        int                      zoneKeyMismatches = 0;
        int                      zoneClearanceMismatches = 0;
        int                      zoneMinWidthMismatches = 0;
        int                      zoneConnectionMismatches = 0;
        int                      zoneSpokeWidthMismatches = 0;
        int                      zoneIslandModeMismatches = 0;
        int                      zoneMinAreaMismatches = 0;
        int                      zonePriorityMismatches = 0;
        std::vector<std::string> missingPadReports;
        std::vector<std::string> zoneMismatchReports;

        for( const auto& [ref, patternStyle, componentAngleCardinal] : model.components )
        {
            const FOOTPRINT* fp = FindFootprintByRef( *board, wxString::FromUTF8( ref ) );

            if( !fp )
                continue;

            comparedFootprints++;

            if( CardinalDeg( fp->GetOrientation().AsDegrees() ) != componentAngleCardinal )
                footprintAngleMismatches++;

            auto patternIt = model.patterns.find( patternStyle );

            if( patternIt == model.patterns.end() )
                continue;

            const auto& patternPads = patternIt->second;

            for( const auto& [padKey, padExpected] : patternPads )
            {
                const PAD* foundPad = nullptr;

                for( const PAD* pad : fp->Pads() )
                {
                    if( ToUtf8( pad->GetNumber() ) == padKey )
                    {
                        foundPad = pad;
                        break;
                    }
                }

                if( !foundPad )
                {
                    missingPatternPads++;

                    if( missingPadReports.size() < 20 )
                    {
                        missingPadReports.push_back( ref + ":" + padKey + " style=" + padExpected.styleName
                                                     + " patt=" + patternStyle );
                    }

                    continue;
                }

                comparedPads++;

                auto styleIt = model.styles.find( padExpected.styleName );

                if( styleIt != model.styles.end() )
                {
                    const DIPXML_PAD_STYLE& style = styleIt->second;

                    if( style.isSurface )
                    {
                        if( foundPad->GetAttribute() != PAD_ATTRIB::SMD )
                            padTypeMismatches++;
                    }
                    else if( style.isThrough )
                    {
                        if( foundPad->GetAttribute() == PAD_ATTRIB::SMD )
                            padTypeMismatches++;

                        if( style.holeMm > 0.0 )
                        {
                            VECTOR2I drill = foundPad->GetDrillSize();

                            if( drill.x <= 0 || drill.y <= 0 )
                            {
                                padDrillMismatches++;
                            }
                            else if( style.isRoundHole
                                     && ( foundPad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE || drill.x != drill.y ) )
                            {
                                padDrillMismatches++;
                            }
                        }
                    }
                }

                VECTOR2I padSize = foundPad->GetSize( PADSTACK::ALL_LAYERS );

                if( padSize.x != padSize.y )
                {
                    int gotParity = CardinalDeg( foundPad->GetFPRelativeOrientation().AsDegrees() ) % 180;
                    int expectedParity = padExpected.angleCardinalDeg % 180;

                    if( gotParity != expectedParity )
                        padAngleMismatches++;
                }
            }
        }

        BOOST_CHECK_MESSAGE( comparedFootprints >= sample.minComparedFootprints,
                             std::string( sample.dip )
                                     + ": compared footprints=" + std::to_string( comparedFootprints ) );
        BOOST_CHECK_MESSAGE( comparedPads >= sample.minComparedPads,
                             std::string( sample.dip ) + ": compared pads=" + std::to_string( comparedPads ) );
        BOOST_CHECK_EQUAL( footprintAngleMismatches, 0 );
        BOOST_CHECK_EQUAL( padAngleMismatches, 0 );
        BOOST_CHECK_EQUAL( padTypeMismatches, 0 );
        BOOST_CHECK_EQUAL( padDrillMismatches, 0 );

        if( !model.copperPours.empty() )
        {
            std::map<std::string, std::vector<int>> expectedZonesByKey;
            std::map<std::string, std::vector<int>> expectedZoneMinWidthsByKey;
            std::map<std::string, std::vector<int>> expectedZoneConnectionsByKey;
            std::map<std::string, std::vector<int>> expectedZoneSpokeWidthsByKey;
            std::map<std::string, std::vector<int>> expectedZoneIslandModesByKey;
            std::map<std::string, std::vector<long long>> expectedZoneMinAreaByKey;
            std::map<std::string, std::vector<int>> expectedZonePrioritiesByKey;
            std::map<std::string, std::vector<int>> importedZonePrioritiesByKey;
            std::map<std::string, std::vector<int>> importedZonesByKey;
            std::map<std::string, std::vector<int>> importedZoneMinWidthsByKey;
            std::map<std::string, std::vector<int>> importedZoneConnectionsByKey;
            std::map<std::string, std::vector<int>> importedZoneSpokeWidthsByKey;
            std::map<std::string, std::vector<int>> importedZoneIslandModesByKey;
            std::map<std::string, std::vector<long long>> importedZoneMinAreaByKey;

            for( const auto& pour : model.copperPours )
            {
                std::string netName;
                auto        netIt = model.netNames.find( pour.netId );

                if( netIt != model.netNames.end() )
                    netName = netIt->second;

                std::string key = std::to_string( pour.layer ) + "|" + netName;
                expectedZonePrioritiesByKey[key].push_back( pour.priority );
                expectedZonesByKey[key].push_back( static_cast<int>( std::lround( pour.clearanceMm * 1000.0 ) ) );
                expectedZoneMinWidthsByKey[key].push_back(
                        static_cast<int>( std::lround( pour.lineWidthMm * 1000.0 ) ) );
                expectedZoneSpokeWidthsByKey[key].push_back(
                        static_cast<int>( std::lround( pour.spokeWidthMm * 1000.0 ) ) );

                int spokeMode = DipXmlSpokeMode( pour.spoke );
                int connection = ( spokeMode == 0 ) ? 0 : 1;
                expectedZoneConnectionsByKey[key].push_back( connection );

                ISLAND_REMOVAL_MODE islandMode = DipXmlIslandModeToKiCad(
                        pour.islandRegion, pour.islandInternal, pour.islandConnection );
                expectedZoneIslandModesByKey[key].push_back( static_cast<int>( islandMode ) );

                if( islandMode == ISLAND_REMOVAL_MODE::AREA )
                {
                    expectedZoneMinAreaByKey[key].push_back(
                            DipXmlMinimumAreaToKiCadIu2( pour.minimumAreaMm ) );
                }
            }

            for( const ZONE* zone : board->Zones() )
            {
                // Synthesized plane fills are layer-level constructs, not stored CopperPours, so
                // they have no XML pour to compare against; skip them in this parity check.
                if( zone->GetZoneName() == wxString( "DipTrace Plane" ) )
                    continue;

                int dipLayer = DipLayerIndexFromKiCadLayer( *board, zone->GetLayer() );

                if( dipLayer < 0 )
                    continue;

                std::string netName = zone->GetNet() ? ToUtf8( zone->GetNet()->GetNetname() ) : std::string();
                std::string key = std::to_string( dipLayer ) + "|" + netName;
                int         clearanceUm = static_cast<int>(
                        std::lround( pcbIUScale.IUTomm( zone->GetLocalClearance().value_or( 0 ) ) * 1000.0 ) );
                int minWidthUm = static_cast<int>( std::lround( pcbIUScale.IUTomm( zone->GetMinThickness() ) * 1000.0 ) );
                int spokeWidthUm = static_cast<int>(
                        std::lround( pcbIUScale.IUTomm( zone->GetThermalReliefSpokeWidth() ) * 1000.0 ) );
                int connection = zone->GetPadConnection() == ZONE_CONNECTION::FULL ? 0 : 1;
                int islandMode = static_cast<int>( zone->GetIslandRemovalMode() );
                importedZonePrioritiesByKey[key].push_back( zone->GetAssignedPriority() );
                importedZonesByKey[key].push_back( clearanceUm );
                importedZoneMinWidthsByKey[key].push_back( minWidthUm );
                importedZoneSpokeWidthsByKey[key].push_back( spokeWidthUm );
                importedZoneConnectionsByKey[key].push_back( connection );
                importedZoneIslandModesByKey[key].push_back( islandMode );

                if( zone->GetIslandRemovalMode() == ISLAND_REMOVAL_MODE::AREA )
                    importedZoneMinAreaByKey[key].push_back( zone->GetMinIslandArea() );
            }

            std::set<std::string> allKeys;

            for( const auto& [key, _] : expectedZonesByKey )
                allKeys.insert( key );

            for( const auto& [key, _] : importedZonesByKey )
                allKeys.insert( key );

            for( const std::string& key : allKeys )
            {
                auto expIt = expectedZonesByKey.find( key );
                auto gotIt = importedZonesByKey.find( key );

                if( expIt == expectedZonesByKey.end() || gotIt == importedZonesByKey.end() )
                {
                    zoneKeyMismatches++;

                    if( zoneMismatchReports.size() < 20 )
                    {
                        zoneMismatchReports.push_back( std::string( "key-missing " ) + key + " exp="
                                                       + std::to_string( expIt != expectedZonesByKey.end() ) + " got="
                                                       + std::to_string( gotIt != importedZonesByKey.end() ) );
                    }

                    continue;
                }

                auto expVals = expIt->second;
                auto gotVals = gotIt->second;
                auto expWidthVals = expectedZoneMinWidthsByKey[key];
                auto gotWidthVals = importedZoneMinWidthsByKey[key];
                auto expConnVals = expectedZoneConnectionsByKey[key];
                auto gotConnVals = importedZoneConnectionsByKey[key];
                auto expSpokeWidthVals = expectedZoneSpokeWidthsByKey[key];
                auto gotSpokeWidthVals = importedZoneSpokeWidthsByKey[key];
                auto expIslandVals = expectedZoneIslandModesByKey[key];
                auto gotIslandVals = importedZoneIslandModesByKey[key];
                auto expMinAreaVals = expectedZoneMinAreaByKey[key];
                auto gotMinAreaVals = importedZoneMinAreaByKey[key];
                auto expPriorityVals = expectedZonePrioritiesByKey[key];
                auto gotPriorityVals = importedZonePrioritiesByKey[key];
                std::sort( expPriorityVals.begin(), expPriorityVals.end() );
                std::sort( gotPriorityVals.begin(), gotPriorityVals.end() );
                std::sort( expVals.begin(), expVals.end() );
                std::sort( gotVals.begin(), gotVals.end() );
                std::sort( expWidthVals.begin(), expWidthVals.end() );
                std::sort( gotWidthVals.begin(), gotWidthVals.end() );
                std::sort( expConnVals.begin(), expConnVals.end() );
                std::sort( gotConnVals.begin(), gotConnVals.end() );
                std::sort( expSpokeWidthVals.begin(), expSpokeWidthVals.end() );
                std::sort( gotSpokeWidthVals.begin(), gotSpokeWidthVals.end() );
                std::sort( expIslandVals.begin(), expIslandVals.end() );
                std::sort( gotIslandVals.begin(), gotIslandVals.end() );
                std::sort( expMinAreaVals.begin(), expMinAreaVals.end() );
                std::sort( gotMinAreaVals.begin(), gotMinAreaVals.end() );

                if( expVals.size() != gotVals.size()
                    || expWidthVals.size() != gotWidthVals.size()
                    || expConnVals.size() != gotConnVals.size()
                    || expSpokeWidthVals.size() != gotSpokeWidthVals.size()
                    || expIslandVals.size() != gotIslandVals.size()
                    || expMinAreaVals.size() != gotMinAreaVals.size()
                    || expPriorityVals.size() != gotPriorityVals.size() )
                {
                    zoneKeyMismatches++;

                    if( zoneMismatchReports.size() < 20 )
                    {
                        zoneMismatchReports.push_back( std::string( "key-count " ) + key
                                                       + " expN=" + std::to_string( expVals.size() )
                                                       + " gotN=" + std::to_string( gotVals.size() ) );
                    }

                    continue;
                }

                for( size_t i = 0; i < expVals.size(); i++ )
                {
                    // 20 um tolerance handles import scale quantization.
                    if( std::abs( expVals[i] - gotVals[i] ) > 20 )
                    {
                        zoneClearanceMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "clearance " ) + key
                                                           + " expUm=" + std::to_string( expVals[i] )
                                                           + " gotUm=" + std::to_string( gotVals[i] ) );
                        }
                    }

                    if( std::abs( expWidthVals[i] - gotWidthVals[i] ) > 20 )
                    {
                        zoneMinWidthMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "min-width " ) + key
                                                           + " expUm=" + std::to_string( expWidthVals[i] )
                                                           + " gotUm=" + std::to_string( gotWidthVals[i] ) );
                        }
                    }

                    if( expPriorityVals[i] != gotPriorityVals[i] )
                    {
                        zonePriorityMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "priority " ) + key
                                                           + " exp=" + std::to_string( expPriorityVals[i] )
                                                           + " got=" + std::to_string( gotPriorityVals[i] ) );
                        }
                    }

                    if( expConnVals[i] != gotConnVals[i] )
                    {
                        zoneConnectionMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "connection " ) + key
                                                           + " exp=" + std::to_string( expConnVals[i] )
                                                           + " got=" + std::to_string( gotConnVals[i] ) );
                        }
                    }

                    if( std::abs( expSpokeWidthVals[i] - gotSpokeWidthVals[i] ) > 20 )
                    {
                        zoneSpokeWidthMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "spoke-width " ) + key
                                                           + " expUm=" + std::to_string( expSpokeWidthVals[i] )
                                                           + " gotUm=" + std::to_string( gotSpokeWidthVals[i] ) );
                        }
                    }

                    if( expIslandVals[i] != gotIslandVals[i] )
                    {
                        zoneIslandModeMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "island-mode " ) + key
                                                           + " exp=" + std::to_string( expIslandVals[i] )
                                                           + " got=" + std::to_string( gotIslandVals[i] ) );
                        }
                    }
                }

                for( size_t i = 0; i < expMinAreaVals.size(); i++ )
                {
                    long long diff = std::llabs( expMinAreaVals[i] - gotMinAreaVals[i] );
                    long long tol = std::max<long long>( 1'000'000'000LL, expMinAreaVals[i] / 100 );

                    if( diff > tol )
                    {
                        zoneMinAreaMismatches++;

                        if( zoneMismatchReports.size() < 20 )
                        {
                            zoneMismatchReports.push_back( std::string( "island-area " ) + key
                                                           + " exp=" + std::to_string( expMinAreaVals[i] )
                                                           + " got=" + std::to_string( gotMinAreaVals[i] ) );
                        }
                    }
                }
            }
        }

        BOOST_CHECK_EQUAL( zoneKeyMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneClearanceMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneMinWidthMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneConnectionMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneSpokeWidthMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneIslandModeMismatches, 0 );
        BOOST_CHECK_EQUAL( zoneMinAreaMismatches, 0 );
        BOOST_CHECK_EQUAL( zonePriorityMismatches, 0 );

        BOOST_TEST_MESSAGE( std::string( sample.dip )
                            + ": dipxml parity footprintComparisons=" + std::to_string( comparedFootprints )
                            + ", padComparisons=" + std::to_string( comparedPads )
                            + ", missingPatternPads=" + std::to_string( missingPatternPads ) );

        if( !missingPadReports.empty() )
        {
            for( const std::string& rep : missingPadReports )
                BOOST_TEST_MESSAGE( std::string( sample.dip ) + ": missing pad " + rep );
        }

        if( !zoneMismatchReports.empty() )
        {
            for( const std::string& rep : zoneMismatchReports )
                BOOST_TEST_MESSAGE( std::string( sample.dip ) + ": zone mismatch " + rep );
        }
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesViaParityOptional )
{
    const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );
    std::string examplesDir =
            examplesEnv && *examplesEnv ? examplesEnv : "/home/seth/Downloads/DipTrace Viewer/Examples";

    if( !std::filesystem::exists( examplesDir ) )
    {
        BOOST_TEST_MESSAGE( "Viewer examples path not found; skipping ViewerExamplesViaParityOptional" );
        return;
    }

    struct SAMPLE
    {
        const char* dip;
        const char* dipxml;
        bool        expectAllTraceViaStyleZero = false;
    };

    static const std::array<SAMPLE, 4> samples = { {
            { "PCB_2.dip", "PCB_2.dipxml", false },
            { "PCB_4.dip", "PCB_4.dipxml", false },
            { "PCB_6.dip", "PCB_6.dipxml", false },
            { "CNC_controller.dip", "CNC_controller.dipxml", true },
    } };

    for( const SAMPLE& sample : samples )
    {
        std::string dipPath = examplesDir + "/" + sample.dip;
        std::string xmlPath = examplesDir + "/" + sample.dipxml;

        if( !std::filesystem::exists( dipPath ) || !std::filesystem::exists( xmlPath ) )
        {
            BOOST_TEST_MESSAGE( "Skipping " + std::string( sample.dip ) + " via parity check; missing .dip or .dipxml" );
            continue;
        }

        DIPXML_BOARD_MODEL model;
        BOOST_REQUIRE_MESSAGE( LoadDipXmlModel( xmlPath, model ), "Failed to load DipXML model: " + xmlPath );

        auto board = LoadBoardFromPath( dipPath );
        BOOST_REQUIRE( board );

        int importedViaCount = 0;

        for( const PCB_TRACK* trk : board->Tracks() )
        {
            if( trk->Type() == PCB_VIA_T )
                importedViaCount++;
        }

        int expectedViaCount = model.traceViaPointsUniqueNetPos + model.viaComponentCount;

        BOOST_CHECK_MESSAGE( importedViaCount == expectedViaCount,
                             std::string( sample.dip ) + ": via parity mismatch; imported="
                                     + std::to_string( importedViaCount )
                                     + " expected(trace-via unique net+xy="
                                     + std::to_string( model.traceViaPointsUniqueNetPos )
                                     + ", standalone via components=" + std::to_string( model.viaComponentCount )
                                     + ", trace-via raw points=" + std::to_string( model.traceViaPointsRaw )
                                     + ", trace-via style0 raw="
                                     + std::to_string( model.traceViaPointsStyleZeroRaw ) + ")" );

        if( sample.expectAllTraceViaStyleZero )
        {
            BOOST_REQUIRE_MESSAGE( model.traceViaPointsRaw > 0,
                                   std::string( sample.dip ) + ": expected routed via points in DipXML" );
            BOOST_CHECK_EQUAL( model.traceViaPointsStyleZeroRaw, model.traceViaPointsRaw );
        }
    }
}


/**
 * Optional local-corpus sweep.
 * Enable by setting DIPTRACE_EXTERNAL_CORPUS_DIR to a directory containing .dip files.
 */
BOOST_AUTO_TEST_CASE( ExternalCorpusImportOptional )
{
    const char* corpusEnv = std::getenv( "DIPTRACE_EXTERNAL_CORPUS_DIR" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "DIPTRACE_EXTERNAL_CORPUS_DIR not set; skipping external corpus sweep" );
        return;
    }

    std::filesystem::path corpusRoot( corpusEnv );

    if( !std::filesystem::exists( corpusRoot ) )
    {
        BOOST_TEST_MESSAGE( "External corpus path does not exist; skipping external corpus sweep" );
        return;
    }

    std::vector<std::filesystem::path> dipFiles;

    for( const auto& entry : std::filesystem::recursive_directory_iterator( corpusRoot ) )
    {
        if( entry.is_regular_file() && HasDipExtension( entry.path() ) )
            dipFiles.push_back( entry.path() );
    }

    std::sort( dipFiles.begin(), dipFiles.end() );

    BOOST_REQUIRE_MESSAGE( !dipFiles.empty(), "No .dip files found under: " + corpusRoot.string() );

    int loaded = 0;
    int skippedUnreadable = 0;

    for( const std::filesystem::path& path : dipFiles )
    {
        if( !m_plugin.CanReadBoard( path.string() ) )
        {
            skippedUnreadable++;
            continue;
        }

        std::unique_ptr<BOARD> board;
        auto*                  capture = new DIPTRACE_WARNING_CAPTURE();
        wxLog*                 oldLog = wxLog::SetActiveTarget( capture );

        struct LOG_GUARD
        {
            wxLog* old;
            ~LOG_GUARD() { wxLog::SetActiveTarget( old ); }
        } logGuard{ oldLog };

        try
        {
            board = LoadBoardFromPath( path.string() );
        }
        catch( const IO_ERROR& e )
        {
            BOOST_ERROR( path.string() + ": IO_ERROR: " + std::string( e.What().utf8_str() ) );
            continue;
        }
        catch( const std::exception& e )
        {
            BOOST_ERROR( path.string() + ": exception: " + std::string( e.what() ) );
            continue;
        }

        BOOST_REQUIRE_MESSAGE( board, "Failed to load: " + path.string() );
        loaded++;

        for( const wxString& warning : capture->m_warnings )
        {
            BOOST_CHECK_MESSAGE( !IsHeuristicParserWarning( warning ),
                                 path.string() + ": unexpected heuristic parser warning: "
                                         + std::string( warning.utf8_str() ) );
        }

        int outlineEndpoints = 0;
        int outlineDisconnected = CountDisconnectedEdgeCutsEndpoints( *board, outlineEndpoints );

        if( outlineEndpoints > 0 )
        {
            BOOST_CHECK_MESSAGE( outlineDisconnected == 0, path.string() + ": disconnected Edge.Cuts endpoints = "
                                                                   + std::to_string( outlineDisconnected ) + "/"
                                                                   + std::to_string( outlineEndpoints ) );
        }

    }

    BOOST_CHECK_MESSAGE( loaded > 0, "External corpus sweep loaded zero boards" );

    if( skippedUnreadable > 0 )
        BOOST_TEST_MESSAGE( "Skipped " << skippedUnreadable << " .dip files without DTBOARD magic" );
}


/**
 * Generated custom DRC rules for per-zone DipTrace properties must always parse
 * cleanly through KiCad's own rules parser. Run against a committed board so the
 * syntax is validated on every build.
 */
BOOST_AUTO_TEST_CASE( ZoneDesignRulesValid )
{
    static const std::array<const char*, 5> boards = {
        "z80_board.dip", "keyboard.dip", "logic_probe.dip", "project4.dip", "156bus_narrow.dip"
    };

    int boardsWithRules = 0;

    for( const char* name : boards )
    {
        BOARD                board;
        DIPTRACE::PCB_PARSER parser( wxString::FromUTF8( GetTestDataDir() + name ), &board );
        parser.Parse();

        wxString dru = parser.GenerateDesignRules();

        if( dru.IsEmpty() )
            continue;

        boardsWithRules++;

        std::vector<std::shared_ptr<DRC_RULE>> rules;
        WX_STRING_REPORTER                     reporter;
        DRC_RULES_PARSER                       rulesParser( dru, wxString::FromUTF8( name ) );

        BOOST_REQUIRE_NO_THROW( rulesParser.Parse( rules, &reporter ) );
        BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ),
                             std::string( name ) + " rules should parse without error:\n"
                                     + reporter.GetMessages().ToStdString() + "\n--- rules ---\n"
                                     + dru.ToStdString() );
        BOOST_CHECK_GT( rules.size(), 0u );
    }

    BOOST_CHECK_MESSAGE( boardsWithRules > 0,
                         "Expected at least one committed board to generate zone DRC rules" );
}


/**
 * Parity check for the generated zone DRC rules against known DipTrace viewer
 * data. CNC_controller pours all carry BoardClearance=0.66mm and ViaDirect=Y, so
 * the generated file must contain a 0.66mm edge_clearance rule and solid
 * via zone_connection rules.
 */
BOOST_AUTO_TEST_CASE( ZoneDesignRulesParityOptional )
{
    const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );
    std::string examplesDir =
            examplesEnv && *examplesEnv ? examplesEnv : "/home/seth/Downloads/DipTrace Viewer/Examples";
    std::string cncPath = examplesDir + "/CNC_controller.dip";

    if( !std::filesystem::exists( cncPath ) )
    {
        BOOST_TEST_MESSAGE( "Viewer examples path not found; skipping ZoneDesignRulesParityOptional" );
        return;
    }

    BOARD                board;
    DIPTRACE::PCB_PARSER parser( wxString::FromUTF8( cncPath ), &board );
    parser.Parse();

    wxString dru = parser.GenerateDesignRules();

    std::vector<std::shared_ptr<DRC_RULE>> rules;
    WX_STRING_REPORTER                     reporter;
    DRC_RULES_PARSER                       rulesParser( dru, wxT( "DipTrace CNC rules" ) );

    BOOST_REQUIRE_NO_THROW( rulesParser.Parse( rules, &reporter ) );
    BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ),
                         "CNC rules should parse without error:\n" + reporter.GetMessages().ToStdString() );

    int  edgeClearanceIU = 0;
    int  solidViaRules = 0;

    for( const std::shared_ptr<DRC_RULE>& rule : rules )
    {
        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
        {
            if( constraint.m_Type == EDGE_CLEARANCE_CONSTRAINT )
                edgeClearanceIU = constraint.GetValue().Min();
            else if( constraint.m_Type == ZONE_CONNECTION_CONSTRAINT
                     && constraint.m_ZoneConnection == ZONE_CONNECTION::FULL )
                solidViaRules++;
        }
    }

    BOOST_CHECK_SMALL( std::abs( pcbIUScale.IUTomm( edgeClearanceIU ) - 0.66 ), 0.01 );
    BOOST_CHECK_GT( solidViaRules, 0 );
}


/**
 * The exact placement rotation of every component lives in a dedicated placement section,
 * stored as a biased int4 of radians x 1e4. These two fixtures each hold four identical
 * capacitors rotated to known angles, so the importer must reproduce each cardinal AND each
 * arbitrary angle exactly (not snapped to 90 degrees). rotate.dip is a v60 file and rotate4.dip
 * a v54 file, proving the encoding is version-independent.
 */
BOOST_AUTO_TEST_CASE( PlacementRotationAngles )
{
    struct CASE
    {
        std::string file;
        std::map<std::string, double> expected; // refdes -> degrees
    };

    const std::vector<CASE> cases = {
        { "rotate.dip",  { { "C1", 0.0 }, { "C2", 45.0 }, { "C3", 90.0 }, { "C4", 309.33 } } },
        { "rotate4.dip", { { "C1", 0.0 }, { "C2", 90.0 }, { "C3", 45.0 }, { "C4", 327.96 } } },
    };

    for( const CASE& tc : cases )
    {
        auto board = LoadBoard( tc.file );
        BOOST_REQUIRE( board );

        std::map<std::string, double> seen;

        for( FOOTPRINT* fp : board->Footprints() )
            seen[fp->GetReference().ToStdString()] = fp->GetOrientation().Normalize().AsDegrees();

        for( const auto& [ref, deg] : tc.expected )
        {
            BOOST_REQUIRE_MESSAGE( seen.count( ref ), tc.file + ": missing " + ref );

            double got = seen[ref];
            double gap = std::abs( got - deg );
            gap = std::min( gap, 360.0 - gap );

            BOOST_CHECK_MESSAGE( gap < 0.1, tc.file + ": " + ref + " orientation " + std::to_string( got )
                                                    + " deg, expected " + std::to_string( deg ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
