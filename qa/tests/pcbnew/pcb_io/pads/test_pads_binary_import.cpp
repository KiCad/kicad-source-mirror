/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcb_io/pads/pcb_io_pads_binary.h>
#include <pcb_io/pads/pcb_io_pads.h>
#include <pcb_io/pads/pads_binary_parser.h>
#include <pcb_io/pads/pads_parser.h>
#include <pcb_io/pads/pads_sdb.h>
#include <io/pads/pads_binary_utils.h>
#include <io/pads/pads_common.h>
#include <layer_ids.h>
#include <padstack.h>
#include <board.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_field.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <pcb_dimension.h>
#include <footprint.h>
#include <netinfo.h>
#include <zone.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <netclass.h>
#include <project/net_settings.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/filename.h>
#include <wx/ffile.h>


struct PADS_BINARY_BOARD_INFO
{
    std::string dir;
    std::string binaryFile;
    std::string ascFile;
    bool        differentRevision;
};


static const PADS_BINARY_BOARD_INFO PADS_BINARY_BOARDS[] = {
    { "TMS1mmX19", "TMS1mmX19.pcb", "TMS1mmX19.asc", false },
    { "MC4_PLUS_CSHAPE", "MC4_PLUS_CSHAPE.pcb", "MC4_PLUS_CSHAPE.asc", false },
    { "MC2_PLUS_REV1", "MC2_PLUS_REV1.pcb", "MC2_PLUS_REV1.asc", true },
    { "Ems4_Rev2", "Ems4_Rev2.pcb", "Ems4_Rev2.asc", false },
    { "LCORE_4", "LCORE_4.pcb", "LCORE_4.asc", false },
    { "LCORE_2", "LCORE_2.pcb", "LCORE_2.asc", false },
    { "Dexter_MotorCtrl", "Dexter_MotorCtrl.pcb", "Dexter_MotorCtrl.asc", true },
    { "MAIS_FC", "MAIS_FC.pcb", "MAIS_FC.asc", true },
};


static wxString GetBinaryPath( const PADS_BINARY_BOARD_INFO& aBoard )
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + aBoard.dir + "/" + aBoard.binaryFile;
}


static wxString GetAscPath( const PADS_BINARY_BOARD_INFO& aBoard )
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + aBoard.dir + "/" + aBoard.ascFile;
}


/**
 * Load a binary .pcb file. Returns nullptr and issues a warning if the load
 * throws, since the parser is under active development. Callers must null-check.
 */
// Boards are loaded by name from a handful of files, and several tests reload the same 37 MB
// design. Share one parse per file and stage binary inputs because importing saved design rules
// beside the source file would otherwise modify corpus fixtures.
static std::shared_ptr<BOARD> CachedLoad( const wxString& aFilename, bool aBinary )
{
    static std::map<std::string, std::shared_ptr<BOARD>> cache;

    std::string key = ( aBinary ? "B:" : "A:" ) + aFilename.ToStdString();
    auto        it = cache.find( key );

    if( it != cache.end() )
        return it->second;

    std::shared_ptr<BOARD> board;

    try
    {
        if( aBinary )
        {
            static const std::filesystem::path stagedRoot =
                    std::filesystem::temp_directory_path()
                    / ( "kicad_pads_binary_cached_"
                        + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
            const std::filesystem::path source( aFilename.ToStdString() );
            const std::filesystem::path stagedDirectory =
                    stagedRoot / std::to_string( std::hash<std::string>{}( key ) );
            const std::filesystem::path stagedPath = stagedDirectory / source.filename();

            std::filesystem::create_directories( stagedDirectory );
            std::filesystem::copy_file( source, stagedPath, std::filesystem::copy_options::overwrite_existing );
            std::filesystem::permissions( stagedPath, std::filesystem::perms::owner_write,
                                          std::filesystem::perm_options::add );

            PCB_IO_PADS_BINARY plugin;
            board.reset( plugin.LoadBoard( wxString::FromUTF8( stagedPath.string() ), nullptr, nullptr, nullptr ) );
        }
        else
        {
            PCB_IO_PADS plugin;
            board.reset( plugin.LoadBoard( aFilename, nullptr, nullptr, nullptr ) );
        }
    }
    catch( const std::exception& )
    {
        board.reset();
        cache[key] = board;
        throw;
    }

    cache[key] = board;
    return board;
}


static std::shared_ptr<BOARD> LoadBinary( const PADS_BINARY_BOARD_INFO& aBoard )
{
    PCB_IO_PADS_BINARY plugin;
    wxString           filename = GetBinaryPath( aBoard );

    BOOST_CHECK_MESSAGE( plugin.CanReadBoard( filename ),
                         aBoard.dir << " binary should be readable by PCB_IO_PADS_BINARY" );

    std::shared_ptr<BOARD> board;

    try
    {
        board = CachedLoad( filename, true );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( aBoard.dir << " binary threw exception during load: " << e.what() );
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aBoard.dir << " binary failed to load" );
    return board;
}


static std::shared_ptr<BOARD> LoadBinaryPath( const wxString& aFilename, const std::string& aLabel )
{
    PCB_IO_PADS_BINARY plugin;

    BOOST_CHECK_MESSAGE( plugin.CanReadBoard( aFilename ),
                         aLabel << " binary should be readable by PCB_IO_PADS_BINARY" );

    std::shared_ptr<BOARD> board;

    try
    {
        board = CachedLoad( aFilename, true );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( aLabel << " binary threw exception during load: " << e.what() );
        return nullptr;
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aLabel << " binary failed to load" );
    return board;
}


static std::shared_ptr<BOARD> LoadAsc( const PADS_BINARY_BOARD_INFO& aBoard )
{
    PCB_IO_PADS plugin;
    wxString    filename = GetAscPath( aBoard );

    std::shared_ptr<BOARD> board;

    try
    {
        board = CachedLoad( filename, false );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( aBoard.dir << " ASC threw exception during load: " << e.what() );
        return nullptr;
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aBoard.dir << " ASC failed to load" );
    return board;
}


static std::shared_ptr<BOARD> LoadAscPath( const wxString& aFilename, const std::string& aLabel )
{
    std::shared_ptr<BOARD> board;

    try
    {
        board = CachedLoad( aFilename, false );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( aLabel << " ASC threw exception during load: " << e.what() );
        return nullptr;
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aLabel << " ASC failed to load" );
    return board;
}


static std::pair<size_t, size_t> ExactLayeredTrackMatches( const wxString& aBinaryPath, const wxString& aAsciiPath,
                                                           const std::string& aLabel )
{
    auto binary = LoadBinaryPath( aBinaryPath, aLabel );
    auto ascii = LoadAscPath( aAsciiPath, aLabel );

    auto viaAnchor = []( const BOARD* aBoard ) -> std::optional<VECTOR2I>
    {
        std::optional<VECTOR2I> anchor;

        for( PCB_TRACK* item : aBoard->Tracks() )
        {
            if( item->Type() != PCB_VIA_T )
                continue;

            VECTOR2I position = item->GetPosition();

            if( !anchor || std::tie( position.x, position.y ) < std::tie( anchor->x, anchor->y ) )
            {
                anchor = position;
            }
        }

        return anchor;
    };

    std::optional<VECTOR2I> binaryAnchor = viaAnchor( binary.get() );
    std::optional<VECTOR2I> asciiAnchor = viaAnchor( ascii.get() );

    if( binaryAnchor && asciiAnchor )
        binary->Move( *asciiAnchor - *binaryAnchor );

    using TRACK_GEOMETRY = std::tuple<int, int, int, int, int, int>;

    auto trackGeometry = []( const BOARD* aBoard )
    {
        std::set<TRACK_GEOMETRY> geometry;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( track->Type() != PCB_TRACE_T )
                continue;

            VECTOR2I start = track->GetStart();
            VECTOR2I end = track->GetEnd();

            if( std::tie( end.x, end.y ) < std::tie( start.x, start.y ) )
                std::swap( start, end );

            geometry.emplace( static_cast<int>( track->GetLayer() ), start.x, start.y, end.x, end.y,
                              track->GetWidth() );
        }

        return geometry;
    };

    const std::set<TRACK_GEOMETRY> binaryTracks = trackGeometry( binary.get() );
    const std::set<TRACK_GEOMETRY> asciiTracks = trackGeometry( ascii.get() );
    std::vector<TRACK_GEOMETRY>    matched;

    std::set_intersection( binaryTracks.begin(), binaryTracks.end(), asciiTracks.begin(), asciiTracks.end(),
                           std::back_inserter( matched ) );

    return { matched.size(), asciiTracks.size() };
}


static std::pair<size_t, size_t> ExactNettedTrackMatches( const wxString& aBinaryPath, const wxString& aAsciiPath,
                                                          const std::string& aLabel )
{
    auto binary = LoadBinaryPath( aBinaryPath, aLabel );
    auto ascii = LoadAscPath( aAsciiPath, aLabel );

    auto viaAnchor = []( const BOARD* aBoard ) -> std::optional<VECTOR2I>
    {
        std::optional<VECTOR2I> anchor;

        for( PCB_TRACK* item : aBoard->Tracks() )
        {
            if( item->Type() != PCB_VIA_T )
                continue;

            VECTOR2I position = item->GetPosition();

            if( !anchor || std::tie( position.x, position.y ) < std::tie( anchor->x, anchor->y ) )
                anchor = position;
        }

        return anchor;
    };

    std::optional<VECTOR2I> binaryAnchor = viaAnchor( binary.get() );
    std::optional<VECTOR2I> asciiAnchor = viaAnchor( ascii.get() );

    if( binaryAnchor && asciiAnchor )
        binary->Move( *asciiAnchor - *binaryAnchor );

    using NETTED_TRACK = std::tuple<int, int, int, int, int, int, wxString>;

    auto tracks = []( const BOARD* aBoard )
    {
        std::set<NETTED_TRACK> rows;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( track->Type() != PCB_TRACE_T )
                continue;

            VECTOR2I start = track->GetStart();
            VECTOR2I end = track->GetEnd();

            if( std::tie( end.x, end.y ) < std::tie( start.x, start.y ) )
                std::swap( start, end );

            rows.emplace( static_cast<int>( track->GetLayer() ), start.x, start.y, end.x, end.y, track->GetWidth(),
                          track->GetNetname() );
        }

        return rows;
    };

    const std::set<NETTED_TRACK> binaryTracks = tracks( binary.get() );
    const std::set<NETTED_TRACK> asciiTracks = tracks( ascii.get() );
    std::vector<NETTED_TRACK>    matched;

    std::set_intersection( binaryTracks.begin(), binaryTracks.end(), asciiTracks.begin(), asciiTracks.end(),
                           std::back_inserter( matched ) );

    return { matched.size(), asciiTracks.size() };
}


static std::pair<size_t, size_t> ExactViaMatches( const wxString& aBinaryPath, const wxString& aAsciiPath,
                                                  const std::string& aLabel )
{
    auto binary = LoadBinaryPath( aBinaryPath, aLabel );
    auto ascii = LoadAscPath( aAsciiPath, aLabel );

    auto viaAnchor = []( const BOARD* aBoard ) -> std::optional<VECTOR2I>
    {
        std::optional<VECTOR2I> anchor;

        for( PCB_TRACK* item : aBoard->Tracks() )
        {
            if( item->Type() != PCB_VIA_T )
                continue;

            VECTOR2I position = item->GetPosition();

            if( !anchor || std::tie( position.x, position.y ) < std::tie( anchor->x, anchor->y ) )
                anchor = position;
        }

        return anchor;
    };

    std::optional<VECTOR2I> binaryAnchor = viaAnchor( binary.get() );
    std::optional<VECTOR2I> asciiAnchor = viaAnchor( ascii.get() );

    if( binaryAnchor && asciiAnchor )
        binary->Move( *asciiAnchor - *binaryAnchor );

    using VIA_GEOMETRY = std::tuple<wxString, int, int, int, int, int, int, int>;

    auto vias = []( const BOARD* aBoard )
    {
        std::set<VIA_GEOMETRY> rows;

        for( PCB_TRACK* item : aBoard->Tracks() )
        {
            PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );

            if( !via )
                continue;

            rows.emplace( via->GetNetname(), via->GetPosition().x, via->GetPosition().y,
                          via->GetWidth( via->TopLayer() ), via->GetDrillValue(), static_cast<int>( via->GetViaType() ),
                          static_cast<int>( via->TopLayer() ), static_cast<int>( via->BottomLayer() ) );
        }

        return rows;
    };

    const std::set<VIA_GEOMETRY> binaryVias = vias( binary.get() );
    const std::set<VIA_GEOMETRY> asciiVias = vias( ascii.get() );
    std::vector<VIA_GEOMETRY>    matched;

    std::set_intersection( binaryVias.begin(), binaryVias.end(), asciiVias.begin(), asciiVias.end(),
                           std::back_inserter( matched ) );

    return { matched.size(), asciiVias.size() };
}


static int CountEdgeCutsShapes( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts )
                count++;
        }
    }

    return count;
}


static size_t CountVias( const std::shared_ptr<BOARD>& aBoard )
{
    size_t vias = 0;

    for( PCB_TRACK* item : aBoard->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
            ++vias;
    }

    return vias;
}


static size_t CountTraces( const std::shared_ptr<BOARD>& aBoard )
{
    size_t traces = 0;

    for( PCB_TRACK* item : aBoard->Tracks() )
    {
        if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
            ++traces;
    }

    return traces;
}


static FOOTPRINT* FindFootprintByReference( const BOARD* aBoard, const wxString& aReference )
{
    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetReference() == aReference )
            return fp;
    }

    return nullptr;
}


static std::set<wxString> BoardNetNames( const BOARD* aBoard )
{
    std::set<wxString> names;

    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        if( net && !net->GetNetname().IsEmpty() )
            names.insert( net->GetNetname() );
    }

    return names;
}


static wxString JoinNetSet( const std::set<wxString>& aNames )
{
    wxString joined;

    for( const wxString& name : aNames )
    {
        if( !joined.IsEmpty() )
            joined += wxString( "," );

        joined += name;
    }

    return joined;
}


static std::vector<std::string> CanonicalGeometryRows( const BOARD* aBoard )
{
    std::vector<std::string> rows;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            const VECTOR2I     drill = pad->GetDrillSize();
            std::ostringstream row;

            row << "PAD " << std::quoted( fp->GetReference().ToStdString() ) << " "
                << std::quoted( pad->GetNumber().ToStdString() ) << " " << pad->GetPosition().x << " "
                << pad->GetPosition().y << " " << pad->GetOrientation().AsTenthsOfADegree() << " "
                << static_cast<int>( pad->GetAttribute() ) << " " << drill.x << " " << drill.y << " "
                << static_cast<int>( pad->GetDrillShape() ) << " " << std::quoted( pad->GetLayerSet().FmtHex() ) << " "
                << std::quoted( pad->GetNetname().ToStdString() );

            for( PCB_LAYER_ID layer : pad->GetLayerSet().Seq() )
            {
                const VECTOR2I size = pad->GetSize( layer );
                const VECTOR2I offset = pad->GetOffset( layer );

                row << " " << static_cast<int>( layer ) << ":" << static_cast<int>( pad->GetShape( layer ) ) << ":"
                    << size.x << ":" << size.y << ":" << offset.x << ":" << offset.y << ":"
                    << pad->GetRoundRectCornerRadius( layer ) << ":" << pad->GetChamferPositions( layer );
            }

            rows.push_back( row.str() );
        }
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        std::ostringstream row;

        if( PCB_VIA* via = dynamic_cast<PCB_VIA*>( track ) )
        {
            row << "VIA " << std::quoted( via->GetNetname().ToStdString() ) << " " << via->GetPosition().x << " "
                << via->GetPosition().y << " " << via->GetWidth( via->TopLayer() ) << " " << via->GetDrillValue() << " "
                << static_cast<int>( via->GetViaType() ) << " " << static_cast<int>( via->TopLayer() ) << " "
                << static_cast<int>( via->BottomLayer() );
        }
        else if( PCB_ARC* arc = dynamic_cast<PCB_ARC*>( track ) )
        {
            row << "ARC " << std::quoted( arc->GetNetname().ToStdString() ) << " "
                << static_cast<int>( arc->GetLayer() ) << " " << arc->GetStart().x << " " << arc->GetStart().y << " "
                << arc->GetMid().x << " " << arc->GetMid().y << " " << arc->GetEnd().x << " " << arc->GetEnd().y << " "
                << arc->GetWidth();
        }
        else
        {
            VECTOR2I start = track->GetStart();
            VECTOR2I end = track->GetEnd();

            if( std::tie( end.x, end.y ) < std::tie( start.x, start.y ) )
                std::swap( start, end );

            row << "TRACK " << std::quoted( track->GetNetname().ToStdString() ) << " "
                << static_cast<int>( track->GetLayer() ) << " " << start.x << " " << start.y << " " << end.x << " "
                << end.y << " " << track->GetWidth();
        }

        rows.push_back( row.str() );
    }

    std::sort( rows.begin(), rows.end() );
    return rows;
}


/**
 * Compare counts with tolerance for binary/ASC differences.
 * Different-revision files get BOOST_WARN only. Same-revision files allow
 * up to 5% or off-by-2, with exact matches reported via BOOST_WARN.
 */
static void CheckCountWithTolerance( const std::string& aLabel, size_t aBinaryCount, size_t aAscCount,
                                     bool aDifferentRevision )
{
    if( aDifferentRevision )
    {
        BOOST_WARN_MESSAGE( aBinaryCount == aAscCount,
                            aLabel << " binary=" << aBinaryCount << " asc=" << aAscCount << " (different revision)" );
        return;
    }

    if( aBinaryCount == aAscCount )
    {
        BOOST_CHECK_MESSAGE( true, aLabel << " counts match: " << aBinaryCount );
        return;
    }

    size_t maxCount = std::max( aBinaryCount, aAscCount );
    size_t diff = ( aBinaryCount > aAscCount ) ? aBinaryCount - aAscCount : aAscCount - aBinaryCount;

    bool withinTolerance = ( diff <= 2 ) || ( diff * 100 / maxCount <= 5 );

    BOOST_CHECK_MESSAGE( withinTolerance,
                         aLabel << " counts differ beyond tolerance: binary=" << aBinaryCount << " asc=" << aAscCount );

    BOOST_WARN_MESSAGE( aBinaryCount == aAscCount,
                        aLabel << " exact count mismatch: binary=" << aBinaryCount << " asc=" << aAscCount );
}


/**
 * Structural integrity checks. Uses BOOST_CHECK for invariants
 * and BOOST_WARN for in-progress features.
 */
static void RunStructuralChecks( const PADS_BINARY_BOARD_INFO& aBoard, const BOARD* aBinaryBoard )
{
    BOOST_WARN_MESSAGE( aBinaryBoard->Tracks().size() > 0, aBoard.dir << " binary has no tracks" );

    std::set<std::pair<int, int>> viaPositions;
    bool                          hasDuplicate = false;

    for( PCB_TRACK* trk : aBinaryBoard->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( trk );

        if( !via || via->GetViaType() != VIATYPE::THROUGH )
            continue;

        auto key = std::make_pair( via->GetPosition().x, via->GetPosition().y );

        if( viaPositions.count( key ) )
        {
            hasDuplicate = true;
            break;
        }

        viaPositions.insert( key );
    }

    BOOST_CHECK_MESSAGE( !hasDuplicate, aBoard.dir << " binary should have no duplicate through-hole vias" );

    for( PCB_TRACK* trk : aBinaryBoard->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
        {
            BOOST_CHECK_MESSAGE( IsCopperLayer( trk->GetLayer() ),
                                 aBoard.dir << " binary track on non-copper layer " << trk->GetLayer() );
        }
    }

    for( FOOTPRINT* fp : aBinaryBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            BOOST_WARN_MESSAGE( pad->GetSize( PADSTACK::ALL_LAYERS ).x > 0
                                        && pad->GetSize( PADSTACK::ALL_LAYERS ).y > 0,
                                aBoard.dir << " " << fp->GetReference() << " pad has zero size" );
        }
    }
}


BOOST_AUTO_TEST_SUITE( PadsBinaryImport )


BOOST_AUTO_TEST_CASE( CanonicalGeometryRowsCoverPadsTracksAndVias )
{
    auto board = LoadAsc( PADS_BINARY_BOARDS[3] );

    BOOST_REQUIRE( board );

    const std::vector<std::string> rows = CanonicalGeometryRows( board.get() );
    size_t                         pads = 0;
    size_t                         tracks = 0;
    size_t                         vias = 0;

    for( const std::string& row : rows )
    {
        pads += row.rfind( "PAD ", 0 ) == 0;
        tracks += row.rfind( "TRACK ", 0 ) == 0 || row.rfind( "ARC ", 0 ) == 0;
        vias += row.rfind( "VIA ", 0 ) == 0;
    }

    BOOST_CHECK_GT( pads, 0u );
    BOOST_CHECK_GT( tracks, 0u );
    BOOST_CHECK_GT( vias, 0u );
}


BOOST_AUTO_TEST_CASE( RouteObjectClassDoesNotDependOnCachedBounds )
{
    const wxString source = GetBinaryPath( PADS_BINARY_BOARDS[4] );

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* objects = sdb.Section( 62 );
    BOOST_REQUIRE( objects != nullptr );
    BOOST_REQUIRE_GT( objects->physicalCount, 0u );
    BOOST_REQUIRE( objects->stride == 36 || objects->stride == 48 );

    auto writeRingU32 = [&]( size_t aLogicalOffset, uint32_t aValue )
    {
        for( size_t byte = 0; byte < 4; ++byte )
        {
            size_t physical = objects->physicalOffset + aLogicalOffset % objects->physicalBytes;
            bytes.at( physical ) = static_cast<uint8_t>( aValue >> ( byte * 8 ) );
            ++aLogicalOffset;
        }
    };

    const size_t boundLoOffset = objects->stride == 36 ? 12 : 24;
    const size_t boundHiOffset = objects->stride == 36 ? 16 : 28;

    for( uint32_t index = 0; index < objects->physicalCount; ++index )
    {
        const size_t base = 32 + static_cast<size_t>( index ) * objects->stride;
        writeRingU32( base + boundLoOffset, 0x12345678 );
        writeRingU32( base + boundHiOffset, 0x76543210 );
    }

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_route_bounds_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    using ROUTE_ROW = std::tuple<std::string, int, double, std::vector<std::pair<double, double>>>;
    auto routeRows = []( const PADS_IO::BINARY_PARSER& aParser )
    {
        std::vector<ROUTE_ROW> rows;

        for( const PADS_IO::ROUTE& route : aParser.GetRoutes() )
        {
            for( const PADS_IO::TRACK& track : route.tracks )
            {
                std::vector<std::pair<double, double>> points;

                for( const PADS_IO::ARC_POINT& point : track.points )
                    points.emplace_back( point.x, point.y );

                rows.emplace_back( route.net_name, track.layer, track.width, std::move( points ) );
            }
        }

        return rows;
    };

    PADS_IO::BINARY_PARSER baseline;
    PADS_IO::BINARY_PARSER changedBounds;
    baseline.Parse( source );
    changedBounds.Parse( tempPath );
    wxRemoveFile( tempPath );

    BOOST_CHECK( routeRows( changedBounds ) == routeRows( baseline ) );
}


BOOST_AUTO_TEST_CASE( BinaryFileDetection )
{
    PCB_IO_PADS_BINARY binaryPlugin;
    PCB_IO_PADS        ascPlugin;

    for( const auto& board : PADS_BINARY_BOARDS )
    {
        wxString binaryPath = GetBinaryPath( board );

        BOOST_CHECK_MESSAGE( binaryPlugin.CanReadBoard( binaryPath ),
                             board.dir << " binary should be recognized by PCB_IO_PADS_BINARY" );

        BOOST_CHECK_MESSAGE( !ascPlugin.CanReadBoard( binaryPath ),
                             board.dir << " binary should NOT be recognized by PCB_IO_PADS" );
    }
}


BOOST_AUTO_TEST_CASE( AsciiFileRejection )
{
    PCB_IO_PADS_BINARY binaryPlugin;

    for( const auto& board : PADS_BINARY_BOARDS )
    {
        wxString ascPath = GetAscPath( board );

        BOOST_CHECK_MESSAGE( !binaryPlugin.CanReadBoard( ascPath ),
                             board.dir << " ASCII should NOT be recognized by PCB_IO_PADS_BINARY" );
    }
}


#define BINARY_LOAD_TEST( name, idx )                                                                                  \
    BOOST_AUTO_TEST_CASE( BasicLoad_##name )                                                                           \
    {                                                                                                                  \
        auto board = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                            \
                                                                                                                       \
        BOOST_CHECK( board->Footprints().size() > 0 );                                                                 \
    }

BINARY_LOAD_TEST( TMS1mmX19, 0 )
BINARY_LOAD_TEST( MC4_PLUS_CSHAPE, 1 )
BINARY_LOAD_TEST( MC2_PLUS_REV1, 2 )
BINARY_LOAD_TEST( Ems4_Rev2, 3 )
BINARY_LOAD_TEST( LCORE_4, 4 )
BINARY_LOAD_TEST( LCORE_2, 5 )
BINARY_LOAD_TEST( Dexter_MotorCtrl, 6 )
BINARY_LOAD_TEST( MAIS_FC, 7 )


#define FOOTPRINT_COUNT_TEST( name, idx )                                                                              \
    BOOST_AUTO_TEST_CASE( FootprintCount_##name )                                                                      \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        CheckCountWithTolerance( #name " footprints", bin->Footprints().size(), asc->Footprints().size(),              \
                                 PADS_BINARY_BOARDS[idx].differentRevision );                                          \
    }

FOOTPRINT_COUNT_TEST( TMS1mmX19, 0 )
FOOTPRINT_COUNT_TEST( MC4_PLUS_CSHAPE, 1 )
FOOTPRINT_COUNT_TEST( MC2_PLUS_REV1, 2 )
FOOTPRINT_COUNT_TEST( Ems4_Rev2, 3 )
FOOTPRINT_COUNT_TEST( LCORE_4, 4 )
FOOTPRINT_COUNT_TEST( LCORE_2, 5 )
FOOTPRINT_COUNT_TEST( Dexter_MotorCtrl, 6 )
FOOTPRINT_COUNT_TEST( MAIS_FC, 7 )


#define NET_COUNT_TEST( name, idx )                                                                                    \
    BOOST_AUTO_TEST_CASE( NetCount_##name )                                                                            \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        CheckCountWithTolerance( #name " nets", bin->GetNetCount(), asc->GetNetCount(),                                \
                                 PADS_BINARY_BOARDS[idx].differentRevision );                                          \
    }

NET_COUNT_TEST( TMS1mmX19, 0 )
NET_COUNT_TEST( MC4_PLUS_CSHAPE, 1 )
NET_COUNT_TEST( MC2_PLUS_REV1, 2 )
NET_COUNT_TEST( Ems4_Rev2, 3 )
NET_COUNT_TEST( LCORE_4, 4 )
NET_COUNT_TEST( LCORE_2, 5 )
NET_COUNT_TEST( Dexter_MotorCtrl, 6 )
NET_COUNT_TEST( MAIS_FC, 7 )


#define NET_NAMES_EXACT_TEST( name, idx )                                                                              \
    BOOST_AUTO_TEST_CASE( NetNamesExact_##name )                                                                       \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto binNames = BoardNetNames( bin.get() );                                                                    \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
        auto ascNames = BoardNetNames( asc.get() );                                                                    \
                                                                                                                       \
        std::set<wxString> missing;                                                                                    \
        std::set_difference( ascNames.begin(), ascNames.end(), binNames.begin(), binNames.end(),                       \
                             std::inserter( missing, missing.begin() ) );                                              \
                                                                                                                       \
        std::set<wxString> extra;                                                                                      \
        std::set_difference( binNames.begin(), binNames.end(), ascNames.begin(), ascNames.end(),                       \
                             std::inserter( extra, extra.begin() ) );                                                  \
                                                                                                                       \
        BOOST_CHECK_MESSAGE( missing.empty(), #name " missing binary nets: " << JoinNetSet( missing ) );               \
        BOOST_CHECK_MESSAGE( extra.empty(), #name " extra binary nets: " << JoinNetSet( extra ) );                     \
    }

NET_NAMES_EXACT_TEST( MC4_PLUS_CSHAPE, 1 )
NET_NAMES_EXACT_TEST( Ems4_Rev2, 3 )


// The section-62/64 route stream is present on routed boards and absent on unrouted boards.
// Exact geometry and layer comparisons above guard against fabricated route reconstruction;
// these smoke tests keep the older in-tree corpus exercising both structural cases.
#define ROUTED_TRACKS_TEST( name, idx )                                                                                \
    BOOST_AUTO_TEST_CASE( RoutedTracks_##name )                                                                        \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        BOOST_CHECK_GT( CountTraces( bin ), 0u );                                                                      \
    }

ROUTED_TRACKS_TEST( Ems4_Rev2, 3 )
ROUTED_TRACKS_TEST( LCORE_4, 4 )
ROUTED_TRACKS_TEST( LCORE_2, 5 )
ROUTED_TRACKS_TEST( MAIS_FC, 7 )


BOOST_AUTO_TEST_CASE( UnroutedBoardHasNoTracksOrVias_TMS1mmX19 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[0] );

    BOOST_CHECK_EQUAL( CountTraces( bin ), 0u );
    BOOST_CHECK_EQUAL( CountVias( bin ), 0u );
}


BOOST_AUTO_TEST_CASE( UnroutedBoardHasNoTracksOrVias_MC4_PLUS_CSHAPE )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[1] );

    BOOST_CHECK_EQUAL( CountTraces( bin ), 0u );
    BOOST_CHECK_EQUAL( CountVias( bin ), 0u );
}


// The per-pin padstack assignment (section-15 (pin, ref) pairs sliced by the section-14
// descriptor table, indexed into the extended section-4 pool) gives each decal's pads their
// own geometry. Before it, every pad on the board shared pad stack 0, collapsing the whole
// board to a single distinct pad geometry. Assert the imported board now carries a variety of
// pad geometries that approaches the ASCII reference's variety.
static size_t DistinctPadGeometries( BOARD* aBoard )
{
    std::set<std::tuple<int, int, int>> geoms;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            VECTOR2I sz = pad->GetSize( F_Cu );
            geoms.emplace( static_cast<int>( pad->GetShape( F_Cu ) ), sz.x, sz.y );
        }
    }

    return geoms.size();
}


BOOST_AUTO_TEST_CASE( PerPinPadStackGeometry_MC4_PLUS_CSHAPE )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[1] );

    size_t binGeoms = DistinctPadGeometries( bin.get() );
    BOOST_TEST_MESSAGE( "MC4_PLUS_CSHAPE binary distinct pad geometries: " << binGeoms );

    // A single shared pad stack 0 would yield one geometry; per-pin assignment recovers the
    // real spread.
    BOOST_CHECK_GE( binGeoms, 5u );

    auto asc = LoadAsc( PADS_BINARY_BOARDS[1] );

    size_t ascGeoms = DistinctPadGeometries( asc.get() );
    BOOST_TEST_MESSAGE( "MC4_PLUS_CSHAPE ASCII distinct pad geometries: " << ascGeoms );

    // De-duplicated library passives without a descriptor keep the default geometry, so
    // allow margin, but the binary should reach a substantial fraction of the reference.
    BOOST_CHECK_GE( binGeoms, ascGeoms / 2 );
}


// Vias are exact structural anchors decoded from the section 60 via records. On the routed
// v0x2026 LCORE boards the binary via set matches the ASCII reference exactly.
BOOST_AUTO_TEST_CASE( StructuralRoutesAndVias_LCORE_4 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[4] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[4] );

    BOOST_REQUIRE( bin );
    BOOST_REQUIRE( asc );

    BOOST_CHECK_GT( CountTraces( bin ), 0u );
    BOOST_CHECK_EQUAL( CountVias( bin ), CountVias( asc ) );
}


BOOST_AUTO_TEST_CASE( StructuralRoutesAndVias_LCORE_2 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[5] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[5] );

    BOOST_REQUIRE( bin );
    BOOST_REQUIRE( asc );

    BOOST_CHECK_GT( CountTraces( bin ), 0u );
    BOOST_CHECK_EQUAL( CountVias( bin ), CountVias( asc ) );
}


BOOST_AUTO_TEST_CASE( ViaCountAtLeast_Ems4_Rev2 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[3] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[3] );

    BOOST_REQUIRE( bin );
    BOOST_REQUIRE( asc );

    BOOST_REQUIRE_GE( CountVias( asc ), 443u );
    BOOST_CHECK_GE( CountVias( bin ), 443u );
    BOOST_CHECK_LE( CountVias( bin ), CountVias( asc ) );
}


BOOST_AUTO_TEST_CASE( BoardOutline_LCORE_4 )
{
    auto board = LoadBinary( PADS_BINARY_BOARDS[4] );

    BOOST_CHECK_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0, "LCORE_4 binary should have board outline shapes" );
}


BOOST_AUTO_TEST_CASE( BoardOutline_LCORE_2 )
{
    auto board = LoadBinary( PADS_BINARY_BOARDS[5] );

    BOOST_CHECK_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0, "LCORE_2 binary should have board outline shapes" );
}


BOOST_AUTO_TEST_CASE( BoardOutline_OtherVersions )
{
    int indices[] = { 0, 1, 2, 3, 6, 7 };

    for( int i : indices )
    {
        auto board = LoadBinary( PADS_BINARY_BOARDS[i] );

        BOOST_WARN_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0,
                            PADS_BINARY_BOARDS[i].dir << " binary outline parsing not yet complete" );
    }
}


// Centerline bounding box of all Edge_Cuts drawing shapes, using each shape's own
// geometric bounding box (arcs report their true swept extent, not the chord). The
// line width does not materially affect the comparison since both importers draw the
// outline on the same layer and the tolerance absorbs the half-width.
static BOX2I EdgeCutsBBox( const BOARD* aBoard )
{
    BOX2I bbox;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts )
                bbox.Merge( shape->GetBoundingBox() );
        }
    }

    return bbox;
}


static int CountEdgeCutsArcs( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts && shape->GetShape() == SHAPE_T::ARC )
                count++;
        }
    }

    return count;
}


// Arc-laden board outlines (MC4/LCORE) are decoded from the binary vertex run plus
// the geometric arc-parameter table. Validate that the binary outline reproduces the
// ASCII outline's overall size and arc content (not just "more than zero shapes").
static void CheckArcOutlineMatchesAsc( int aIdx )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[aIdx] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[aIdx] );

    const std::string& name = PADS_BINARY_BOARDS[aIdx].dir;

    int binArcs = CountEdgeCutsArcs( bin.get() );
    int ascArcs = CountEdgeCutsArcs( asc.get() );

    BOOST_CHECK_MESSAGE( binArcs == ascArcs, name << " edge-cut arc count: binary=" << binArcs << " asc=" << ascArcs );

    BOX2I binBox = EdgeCutsBBox( bin.get() );
    BOX2I ascBox = EdgeCutsBBox( asc.get() );

    BOOST_REQUIRE_MESSAGE( binBox.GetWidth() > 0 && ascBox.GetWidth() > 0, name << " edge-cut bounding box is empty" );

    // Size must agree closely. The fixed slack absorbs the outline pen width (the ASC
    // carries the piece width while the binary path leaves it at 0) plus ASC 2-decimal
    // mils rounding; the 1% term scales with the board. This is far tighter than the
    // ~7M nm error a wrong major/minor arc selection produced.
    double tolX = std::max( 400000.0, ascBox.GetWidth() * 0.01 );
    double tolY = std::max( 400000.0, ascBox.GetHeight() * 0.01 );

    BOOST_CHECK_MESSAGE( std::abs( binBox.GetWidth() - ascBox.GetWidth() ) < tolX,
                         name << " edge-cut width: binary=" << binBox.GetWidth() << " asc=" << ascBox.GetWidth() );
    BOOST_CHECK_MESSAGE( std::abs( binBox.GetHeight() - ascBox.GetHeight() ) < tolY,
                         name << " edge-cut height: binary=" << binBox.GetHeight() << " asc=" << ascBox.GetHeight() );
}


BOOST_AUTO_TEST_CASE( ArcBoardOutline_MC4_PLUS_CSHAPE )
{
    CheckArcOutlineMatchesAsc( 1 );
}


BOOST_AUTO_TEST_CASE( ArcBoardOutline_LCORE_4 )
{
    CheckArcOutlineMatchesAsc( 4 );
}


BOOST_AUTO_TEST_CASE( ArcBoardOutline_LCORE_2 )
{
    CheckArcOutlineMatchesAsc( 5 );
}


// A wrong board outline is worse than none. For every corpus board assert the binary
// importer either ships no Edge_Cuts outline at all, or one whose bounding box matches
// the ASCII reference outline within tolerance. This catches the class of bug where a
// decal or concatenated piece is mistaken for the board outline (too large) or a small
// wrong piece is selected (too small), which a bare "more than zero shapes" check missed.
BOOST_AUTO_TEST_CASE( BoardOutlineCorrectOrAbsent )
{
    for( const PADS_BINARY_BOARD_INFO& info : PADS_BINARY_BOARDS )
    {
        if( info.differentRevision )
            continue;

        auto bin = LoadBinary( info );
        auto asc = LoadAsc( info );

        BOX2I binBox = EdgeCutsBBox( bin.get() );
        BOX2I ascBox = EdgeCutsBBox( asc.get() );

        // No binary outline shipped is acceptable; only a present-but-wrong one fails.
        if( binBox.GetWidth() <= 0 && binBox.GetHeight() <= 0 )
            continue;

        BOOST_REQUIRE_MESSAGE( ascBox.GetWidth() > 0 && ascBox.GetHeight() > 0,
                               info.dir << " ships a binary outline but the ASC has none "
                                           "to validate against" );

        double tolX = std::max( 400000.0, ascBox.GetWidth() * 0.02 );
        double tolY = std::max( 400000.0, ascBox.GetHeight() * 0.02 );

        BOOST_CHECK_MESSAGE( std::abs( binBox.GetWidth() - ascBox.GetWidth() ) < tolX,
                             info.dir << " board outline width: binary=" << binBox.GetWidth()
                                      << " asc=" << ascBox.GetWidth() << " (wrong outline shipped)" );
        BOOST_CHECK_MESSAGE( std::abs( binBox.GetHeight() - ascBox.GetHeight() ) < tolY,
                             info.dir << " board outline height: binary=" << binBox.GetHeight()
                                      << " asc=" << ascBox.GetHeight() << " (wrong outline shipped)" );

        // Size alone is translation-invariant, so also pin the binary outline relative
        // to the binary footprints (a shared frame, unlike the ASC importer which uses a
        // different absolute origin). A correctly placed outline contains the parts; a
        // doubly-origin-shifted one keeps the right size but drifts off the parts.
        BOX2I fpBox;

        for( FOOTPRINT* fp : bin->Footprints() )
            fpBox.Merge( fp->GetPosition() );

        if( fpBox.GetWidth() > 0 || fpBox.GetHeight() > 0 )
        {
            BOX2I grown = binBox;
            grown.Inflate( std::max( binBox.GetWidth(), binBox.GetHeight() ) );

            BOOST_CHECK_MESSAGE( grown.Contains( fpBox.GetCenter() ),
                                 info.dir << " board outline at " << binBox.GetCenter()
                                          << " does not enclose the footprint cloud centered at " << fpBox.GetCenter()
                                          << " (outline mispositioned)" );
        }
    }
}


#define STRUCTURAL_INTEGRITY_TEST( name, idx )                                                                         \
    BOOST_AUTO_TEST_CASE( StructuralIntegrity_##name )                                                                 \
    {                                                                                                                  \
        auto board = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                            \
                                                                                                                       \
        RunStructuralChecks( PADS_BINARY_BOARDS[idx], board.get() );                                                   \
    }

#define ZONE_COUNT_TEST( name, idx )                                                                                   \
    BOOST_AUTO_TEST_CASE( ZoneCount_##name )                                                                           \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        size_t binZones = bin->Zones().size();                                                                         \
        size_t ascZones = asc->Zones().size();                                                                         \
                                                                                                                       \
        if( PADS_BINARY_BOARDS[idx].differentRevision )                                                                \
        {                                                                                                              \
            BOOST_WARN_MESSAGE( binZones > 0, #name " binary zone count: " << binZones << " (different revision)" );   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            BOOST_WARN_MESSAGE( binZones == ascZones,                                                                  \
                                #name " zone count: binary=" << binZones << " asc=" << ascZones );                     \
        }                                                                                                              \
    }

ZONE_COUNT_TEST( MC4_PLUS_CSHAPE, 1 )
ZONE_COUNT_TEST( Ems4_Rev2, 3 )
ZONE_COUNT_TEST( LCORE_4, 4 )
ZONE_COUNT_TEST( LCORE_2, 5 )
ZONE_COUNT_TEST( MAIS_FC, 7 )


BOOST_AUTO_TEST_CASE( ZoneCountExact_MC4_PLUS_CSHAPE )
{
    // This board's copper-shape recovery has been verified directly against the ASCII *LINES*
    // COPPER entries by name (not just by count): the binary importer decodes all 15 real
    // COPPER shapes with zero false positives (up from 2 before this session's classifier and
    // circle-closure fixes). asc->Zones() is NOT used as the reference here -- the ASCII pour
    // parser has its own unrelated, pre-existing bug (it reads far more "*POUROUT*"-adjacent
    // lines than there are real pours, most filtered downstream but not audited closely enough
    // to reconstruct a reliable total), so its zone count is not a trustworthy oracle for this
    // comparison. Two shapes are on copper layer 4; the other thirteen retain their serialized
    // documentation layers as filled graphics. 36 = 2 copper shapes + 9 keepouts + 25 pours.
    auto bin = LoadBinary( PADS_BINARY_BOARDS[1] );

    BOOST_REQUIRE( bin );
    BOOST_CHECK_EQUAL( bin->Zones().size(), 36 );
    BOOST_CHECK_EQUAL( std::count_if( bin->Drawings().begin(), bin->Drawings().end(),
                                      []( const BOARD_ITEM* aItem )
                                      {
                                          const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>( aItem );
                                          return shape && shape->GetShape() == SHAPE_T::POLY && shape->IsSolidFill();
                                      } ),
                       13 );
}


BOOST_AUTO_TEST_CASE( ZoneCountExact_Ems4_Rev2 )
{
    // See ZoneCountExact_MC4_PLUS_CSHAPE for why asc->Zones() is not used as the reference.
    // The binary importer recovers all 39 real COPPER owners. Four are on copper layers; the
    // other 35 retain their silkscreen/paste/custom layers as filled graphics.
    auto bin = LoadBinary( PADS_BINARY_BOARDS[3] );

    BOOST_REQUIRE( bin );
    BOOST_CHECK_EQUAL( bin->Zones().size(), 32 );
    BOOST_CHECK_EQUAL( std::count_if( bin->Drawings().begin(), bin->Drawings().end(),
                                      []( const BOARD_ITEM* aItem )
                                      {
                                          const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>( aItem );
                                          return shape && shape->GetShape() == SHAPE_T::POLY && shape->IsSolidFill();
                                      } ),
                       35 );
}


BOOST_AUTO_TEST_CASE( CopperShapeLevelsUseSerializedSuccessorField )
{
    struct EXPECTED_LEVEL
    {
        size_t      board;
        std::string name;
        int         level;
    };

    const std::vector<EXPECTED_LEVEL> expected = {
        { 1, "DRW68014421", 4 },  { 1, "DRW62739568", 26 }, { 3, "DRW55434270", 1 },
        { 3, "DRW28145516", 22 }, { 3, "DRW81324514", 24 },
    };

    std::map<size_t, std::shared_ptr<PADS_IO::BINARY_PARSER>> parsers;

    for( const EXPECTED_LEVEL& item : expected )
    {
        auto& parser = parsers[item.board];

        if( !parser )
        {
            parser = std::make_shared<PADS_IO::BINARY_PARSER>();
            wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + PADS_BINARY_BOARDS[item.board].dir
                                + "/" + PADS_BINARY_BOARDS[item.board].binaryFile;
            parser->Parse( filename );
        }

        auto shape = std::find_if( parser->GetCopperShapes().begin(), parser->GetCopperShapes().end(),
                                   [&]( const PADS_IO::COPPER_SHAPE& aShape )
                                   {
                                       return aShape.name == item.name;
                                   } );

        BOOST_REQUIRE( shape != parser->GetCopperShapes().end() );
        BOOST_CHECK_EQUAL( shape->layer, item.level );
    }
}


BOOST_AUTO_TEST_CASE( ZoneCountExact_LCORE_4 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[4] );

    BOOST_REQUIRE( bin );
    BOOST_CHECK_EQUAL( bin->Zones().size(), 4 ); // 2 KEEPOUT, 2 POUROUT
}


BOOST_AUTO_TEST_CASE( ZoneCountExact_LCORE_2 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[5] );

    BOOST_REQUIRE( bin );
    BOOST_CHECK_EQUAL( bin->Zones().size(), 4 ); // 2 KEEPOUT, 2 POUROUT
}


static size_t CountFreeTexts( const BOARD* aBoard )
{
    size_t count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( dynamic_cast<PCB_TEXT*>( item ) )
            count++;
    }

    return count;
}


static bool HasFreeText( const BOARD* aBoard, const wxString& aText )
{
    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( item );

        if( text && text->GetText() == aText )
            return true;
    }

    return false;
}


#define FREE_TEXT_COUNT_TEST( name, idx )                                                                              \
    BOOST_AUTO_TEST_CASE( FreeTextCount_##name )                                                                       \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        size_t binTexts = CountFreeTexts( bin.get() );                                                                 \
        size_t ascTexts = CountFreeTexts( asc.get() );                                                                 \
                                                                                                                       \
        BOOST_WARN_MESSAGE( binTexts == ascTexts,                                                                      \
                            #name " free text count: binary=" << binTexts << " asc=" << ascTexts                       \
                                                              << " (text extraction incomplete)" );                    \
    }

FREE_TEXT_COUNT_TEST( TMS1mmX19, 0 )
FREE_TEXT_COUNT_TEST( MC4_PLUS_CSHAPE, 1 )
FREE_TEXT_COUNT_TEST( MC2_PLUS_REV1, 2 )
FREE_TEXT_COUNT_TEST( Ems4_Rev2, 3 )
FREE_TEXT_COUNT_TEST( LCORE_4, 4 )
FREE_TEXT_COUNT_TEST( LCORE_2, 5 )
FREE_TEXT_COUNT_TEST( Dexter_MotorCtrl, 6 )
FREE_TEXT_COUNT_TEST( MAIS_FC, 7 )


/**
 * Free-text extraction is solved for these boards (sec5/sec8 text-header stream
 * with metadata lagging geometry by one record slot). The binary importer
 * reproduces the ASCII free-text count exactly, so lock it in with a hard check
 * to catch regressions. Boards still under development stay on BOOST_WARN above.
 */
#define FREE_TEXT_EXACT_TEST( name, idx )                                                                              \
    BOOST_AUTO_TEST_CASE( FreeTextExact_##name )                                                                       \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        BOOST_CHECK_EQUAL( CountFreeTexts( bin.get() ), CountFreeTexts( asc.get() ) );                                 \
    }

FREE_TEXT_EXACT_TEST( TMS1mmX19, 0 )
FREE_TEXT_EXACT_TEST( Ems4_Rev2, 3 )
FREE_TEXT_EXACT_TEST( LCORE_4, 4 )
FREE_TEXT_EXACT_TEST( LCORE_2, 5 )


BOOST_AUTO_TEST_CASE( FreeTextRejectsClusterNames_MC4_PLUS_CSHAPE )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[1] );

    BOOST_CHECK_MESSAGE( !HasFreeText( bin.get(), "CLU_DCDC5V" ),
                         "MC4 binary cluster name CLU_DCDC5V should not import as free text" );
    BOOST_CHECK_MESSAGE( !HasFreeText( bin.get(), "CLU_DCDC3V3" ),
                         "MC4 binary cluster name CLU_DCDC3V3 should not import as free text" );
}


BOOST_AUTO_TEST_CASE( FreeTextUsesDeclaredSection8Ring )
{
    wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* texts = sdb.Section( 8 );
    BOOST_REQUIRE( texts != nullptr );
    BOOST_REQUIRE( texts->physicalOffset >= 180 );
    BOOST_REQUIRE( texts->physicalBytes >= 108 );

    const size_t ringBase = texts->physicalOffset - 36;
    const size_t decoyBase = texts->physicalOffset - 180;
    std::copy_n( bytes.begin() + ringBase, 144, bytes.begin() + decoyBase );

    PADS_IO::BINARY_PARSER baseline;
    baseline.Parse( source );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_text_decoy_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    PADS_IO::BINARY_PARSER withDecoy;
    withDecoy.Parse( tempPath );
    wxRemoveFile( tempPath );

    BOOST_CHECK_EQUAL( withDecoy.GetTexts().size(), baseline.GetTexts().size() );
}


#define PAD_COUNT_TEST( name, idx )                                                                                    \
    BOOST_AUTO_TEST_CASE( PadCount_##name )                                                                            \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        size_t binPads = 0;                                                                                            \
        size_t ascPads = 0;                                                                                            \
                                                                                                                       \
        for( FOOTPRINT * fp : bin->Footprints() )                                                                      \
            binPads += fp->Pads().size();                                                                              \
                                                                                                                       \
        for( FOOTPRINT * fp : asc->Footprints() )                                                                      \
            ascPads += fp->Pads().size();                                                                              \
                                                                                                                       \
        BOOST_WARN_MESSAGE( binPads == ascPads,                                                                        \
                            #name " pad count: binary=" << binPads << " asc=" << ascPads                               \
                                                        << " (part-to-decal linking incomplete)" );                    \
    }

/**
 * Per-footprint content correctness over the RESOLVED set. A binary footprint is
 * "resolved" when the placement -> parttype -> decal chain assigned it a decal, i.e.
 * its library name differs from its own reference designator (the graceful fallback
 * used for the small constant set of placements that are absent from section 22).
 *
 * For each resolved footprint that also exists in the ASCII import, the binary decal
 * (footprint library name) must match the ASCII decal, and the binary pad count must
 * equal the ASCII pad count. This proves the chain is content-correct, not merely that
 * aggregate totals happen to line up.
 */
#define PER_FOOTPRINT_CONTENT_TEST( name, idx, maxNameWrong, maxCountWrong )                                           \
    BOOST_AUTO_TEST_CASE( PerFootprintContent_##name )                                                                 \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        std::map<wxString, FOOTPRINT*> ascByRef;                                                                       \
                                                                                                                       \
        for( FOOTPRINT * fp : asc->Footprints() )                                                                      \
            ascByRef[fp->GetReference()] = fp;                                                                         \
                                                                                                                       \
        int      resolved = 0, nameWrong = 0, countWrong = 0;                                                          \
        wxString wrongRefs;                                                                                            \
                                                                                                                       \
        for( FOOTPRINT * fp : bin->Footprints() )                                                                      \
        {                                                                                                              \
            wxString binDecal = fp->GetFPID().GetLibItemName().wx_str();                                               \
                                                                                                                       \
            if( binDecal == fp->GetReference() )                                                                       \
                continue; /* unresolved placement (absent from section 22) */                                          \
                                                                                                                       \
            auto it = ascByRef.find( fp->GetReference() );                                                             \
                                                                                                                       \
            if( it == ascByRef.end() )                                                                                 \
                continue;                                                                                              \
                                                                                                                       \
            resolved++;                                                                                                \
                                                                                                                       \
            if( binDecal != it->second->GetFPID().GetLibItemName().wx_str() )                                          \
            {                                                                                                          \
                nameWrong++;                                                                                           \
                wrongRefs += fp->GetReference()                                                                        \
                             + wxString::Format( ":name %s/%s %zu/%zu ", binDecal,                                     \
                                                 it->second->GetFPID().GetLibItemName().wx_str(), fp->Pads().size(),   \
                                                 it->second->Pads().size() );                                          \
            }                                                                                                          \
            else if( fp->Pads().size() != it->second->Pads().size() )                                                  \
            {                                                                                                          \
                countWrong++;                                                                                          \
                wrongRefs += fp->GetReference()                                                                        \
                             + wxString::Format( ":%zu/%zu ", fp->Pads().size(), it->second->Pads().size() );          \
            }                                                                                                          \
        }                                                                                                              \
                                                                                                                       \
        BOOST_TEST_MESSAGE( #name " per-footprint: resolved=" << resolved << " nameWrong=" << nameWrong                \
                                                              << " countWrong=" << countWrong                          \
                                                              << " wrongRefs=" << wrongRefs );                         \
        BOOST_CHECK_LE( nameWrong, maxNameWrong );                                                                     \
        BOOST_CHECK_LE( countWrong, maxCountWrong );                                                                   \
    }

PER_FOOTPRINT_CONTENT_TEST( TMS1mmX19, 0, 1, 0 )
PER_FOOTPRINT_CONTENT_TEST( MC4_PLUS_CSHAPE, 1, 0, 0 )
PER_FOOTPRINT_CONTENT_TEST( Ems4_Rev2, 3, 0, 0 )
PER_FOOTPRINT_CONTENT_TEST( LCORE_4, 4, 1, 0 )


BOOST_AUTO_TEST_CASE( SectionBoundaryPlacement_TMS1mmX19_SM3 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[0] );

    FOOTPRINT* sm3 = FindFootprintByReference( bin.get(), "SM3" );

    BOOST_REQUIRE_MESSAGE( sm3 != nullptr, "TMS1mmX19 binary should import section-boundary placement SM3" );
    BOOST_CHECK_EQUAL( sm3->GetFPID().GetLibItemName().wx_str(), "MTHOLE-M3-3.2MM" );
    BOOST_CHECK_EQUAL( sm3->Pads().size(), 1u );
}


/**
 * v0x2021 pad import (Dexter_MotorCtrl, MAIS_FC). The v0x2021 dialect carries no
 * parttype-definition layer, so the placement -> decal link is the direct decal index
 * in the NEXT 96-byte placement record's @+56, resolved against the JMPVIA_AAAAA-anchored
 * 100-byte decal-name table (terminal count at record +72). The PCB_IO_PADS ASCII reader
 * loads zero footprints for these revisions, so we score the binary import's own decoded
 * pads against the .asc PARTDECAL/PART GOLD that the pure-binary RE harness verified
 * (MAIS 43 exact; Dexter resolves the bulk of the placed set, with the small constant set
 * of section-22-omitted connectors/LEDs getting footprints with no fabricated pads).
 */
BOOST_AUTO_TEST_CASE( V2021_PadImport_MAIS_FC )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[7] );

    size_t binPads = 0;
    int    resolved = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
    {
        binPads += fp->Pads().size();

        if( fp->GetFPID().GetLibItemName().wx_str() != fp->GetReference() )
            resolved++;
    }

    BOOST_TEST_MESSAGE( "MAIS_FC v0x2021: pads=" << binPads << " resolved=" << resolved );

    // MAIS_FC has 5 placed parts (43 pads) and the chain is fully deterministic here.
    BOOST_CHECK_EQUAL( binPads, 43u );
    BOOST_CHECK_EQUAL( resolved, 5 );

    // Verify the actual decal mapping, not just the aggregate count. These connectors
    // live in section 19 (section 22 carries no placements on this board), so this also
    // exercises the +1-lag leading-block recovery: J7 is the anchor block that has no
    // 0xFEFF marker of its own. Decal -> expected terminal count from the .asc PARTDECAL.
    std::map<wxString, std::pair<wxString, size_t>> expected = {
        { "J7", { "54722-0201", 20 } }, { "J1", { "SOLDERLAND4", 11 } }, { "J2", { "CON3", 3 } },
        { "J3", { "CON3", 3 } },        { "J4", { "CON6", 6 } },
    };

    int verified = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
    {
        auto it = expected.find( fp->GetReference() );

        if( it == expected.end() )
            continue;

        verified++;
        BOOST_CHECK_EQUAL( fp->GetFPID().GetLibItemName().wx_str(), it->second.first );
        BOOST_CHECK_EQUAL( fp->Pads().size(), it->second.second );
    }

    BOOST_CHECK_EQUAL( verified, 5 );
}


BOOST_AUTO_TEST_CASE( V2021_PadImport_Dexter_MotorCtrl )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[6] );

    size_t binPads = 0;
    int    resolved = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
    {
        binPads += fp->Pads().size();

        if( fp->GetFPID().GetLibItemName().wx_str() != fp->GetReference() )
            resolved++;
    }

    BOOST_TEST_MESSAGE( "Dexter_MotorCtrl v0x2021: pads=" << binPads << " resolved=" << resolved );

    // Dexter_MotorCtrl previously imported zero pads (the chain was gated off for v0x2021).
    // The direct decal-index path now resolves the large majority of placements; the binary
    // is a different revision than the .asc (GOLD 918) and carries additional placed parts,
    // so assert a substantial, content-bearing pad count rather than an exact GOLD match.
    BOOST_CHECK_GT( binPads, 800u );
    BOOST_CHECK_GT( resolved, 200 );

    // Verify specific multi-pin decal mappings so a shifted decal index that still yields
    // many pads would be caught. Decal name and terminal count both come from the binary
    // JMPVIA table and match the .asc PARTDECAL for these parts.
    std::map<wxString, std::pair<wxString, size_t>> expected = {
        { "D1", { "DFN-6-9", 9 } },
        { "L1", { "IHLP-2525", 2 } },
    };

    int verified = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
    {
        auto it = expected.find( fp->GetReference() );

        if( it == expected.end() )
            continue;

        verified++;
        BOOST_CHECK_EQUAL( fp->GetFPID().GetLibItemName().wx_str(), it->second.first );
        BOOST_CHECK_EQUAL( fp->Pads().size(), it->second.second );
    }

    BOOST_CHECK_EQUAL( verified, 2 );
}


/**
 * Verify the v0x2021 placement Y coordinate, orientation and side.
 *
 * The v0x2021 dialect frames its 96-byte placement records shifted +20 from the new
 * dialect, putting Y immediately after X (xOff+4), the orientation at xOff+8 and the
 * side flag at nameOff+28. Y was previously emitted as 0 (collapsing every part onto a
 * single line) and the orientation was read from the lagged neighbour record. Ground
 * truth is the MAIS_FC.asc *PART* section (the binary SetOrientation path does not
 * negate the stored angle, so the placed orientation equals the ASC ORI value):
 *   J7  9000000 270 M   J1 10800000 0 N   J2 23663130 180 N
 *   J3  8785902   0 N   J4 21216083 90 N
 */
BOOST_AUTO_TEST_CASE( V2021_PartPlacement_MAIS_FC )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[7] ); // MAIS_FC, v0x2021

    struct Oracle
    {
        wxString ref;
        int      ascY;    // ASC design-space Y, for relative ordering only
        double   ori;     // ASC ORI degrees (top-side parts)
        bool     flipped; // ASC MIRROR flag
    };

    const std::vector<Oracle> oracle = {
        { "J1", 10800000, 0.0, false },  { "J2", 23663130, 180.0, false }, { "J3", 8785902, 0.0, false },
        { "J4", 21216083, 90.0, false }, { "J7", 9000000, 270.0, true },
    };

    std::map<wxString, FOOTPRINT*> binFps;

    for( FOOTPRINT* fp : bin->Footprints() )
        binFps[fp->GetReference()] = fp;

    // Y must not collapse onto a single value (the unsolved-Y regression emitted 0 for all).
    std::set<int> binYs;

    for( const Oracle& o : oracle )
    {
        BOOST_REQUIRE_MESSAGE( binFps.count( o.ref ), "v0x2021 missing footprint " << o.ref );
        binYs.insert( binFps[o.ref]->GetPosition().y );
    }

    BOOST_CHECK_MESSAGE( binYs.size() == oracle.size(), "v0x2021 part Y collapsed to "
                                                                << binYs.size() << " of " << oracle.size()
                                                                << " distinct values (Y decode regression)" );

    // Side must match the MIRROR flag; un-mirrored parts must carry the exact ASC ORI.
    for( const Oracle& o : oracle )
    {
        FOOTPRINT* fp = binFps[o.ref];

        BOOST_CHECK_MESSAGE( fp->IsFlipped() == o.flipped,
                             "v0x2021 " << o.ref << " side " << fp->IsFlipped() << " != " << o.flipped );

        if( o.flipped )
            continue;

        double deg = fp->GetOrientation().Normalize().AsDegrees();
        double diff = std::abs( deg - o.ori );

        BOOST_CHECK_MESSAGE( diff < 0.1 || std::abs( diff - 360.0 ) < 0.1,
                             "v0x2021 " << o.ref << " orientation " << deg << " != " << o.ori );
    }

    // Relative Y ordering must follow the ASC (origin/scale/flip invariant): sorting by ASC
    // Y must yield a strictly monotonic placed-Y sequence.
    std::vector<Oracle> byAscY( oracle );
    std::sort( byAscY.begin(), byAscY.end(),
               []( const Oracle& a, const Oracle& b )
               {
                   return a.ascY < b.ascY;
               } );

    bool incr = true;
    bool decr = true;

    for( size_t i = 1; i < byAscY.size(); ++i )
    {
        int prev = binFps[byAscY[i - 1].ref]->GetPosition().y;
        int cur = binFps[byAscY[i].ref]->GetPosition().y;

        if( cur <= prev )
            incr = false;

        if( cur >= prev )
            decr = false;
    }

    BOOST_CHECK_MESSAGE( incr || decr, "v0x2021 placed-Y ordering does not match ASC oracle" );
}


PAD_COUNT_TEST( TMS1mmX19, 0 )
PAD_COUNT_TEST( MC4_PLUS_CSHAPE, 1 )
PAD_COUNT_TEST( MC2_PLUS_REV1, 2 )
PAD_COUNT_TEST( Ems4_Rev2, 3 )
PAD_COUNT_TEST( LCORE_4, 4 )
PAD_COUNT_TEST( LCORE_2, 5 )
PAD_COUNT_TEST( Dexter_MotorCtrl, 6 )
PAD_COUNT_TEST( MAIS_FC, 7 )

BOOST_AUTO_TEST_CASE( PadCountExact_MC4_PLUS_CSHAPE )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[1] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[1] );

    size_t binPads = 0;
    size_t ascPads = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
        binPads += fp->Pads().size();

    for( FOOTPRINT* fp : asc->Footprints() )
        ascPads += fp->Pads().size();

    BOOST_CHECK_EQUAL( binPads, ascPads );
}

BOOST_AUTO_TEST_CASE( PadCountExact_Ems4_Rev2 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[3] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[3] );

    size_t binPads = 0;
    size_t ascPads = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
        binPads += fp->Pads().size();

    for( FOOTPRINT* fp : asc->Footprints() )
        ascPads += fp->Pads().size();

    BOOST_CHECK_EQUAL( binPads, ascPads );
}

BOOST_AUTO_TEST_CASE( PadCountExact_LCORE_4 )
{
    auto bin = LoadBinary( PADS_BINARY_BOARDS[4] );
    auto asc = LoadAsc( PADS_BINARY_BOARDS[4] );

    size_t binPads = 0;
    size_t ascPads = 0;

    for( FOOTPRINT* fp : bin->Footprints() )
        binPads += fp->Pads().size();

    for( FOOTPRINT* fp : asc->Footprints() )
        ascPads += fp->Pads().size();

    BOOST_CHECK_EQUAL( binPads, ascPads );
}


STRUCTURAL_INTEGRITY_TEST( TMS1mmX19, 0 )
STRUCTURAL_INTEGRITY_TEST( MC4_PLUS_CSHAPE, 1 )
STRUCTURAL_INTEGRITY_TEST( MC2_PLUS_REV1, 2 )
STRUCTURAL_INTEGRITY_TEST( Ems4_Rev2, 3 )
STRUCTURAL_INTEGRITY_TEST( LCORE_4, 4 )
STRUCTURAL_INTEGRITY_TEST( LCORE_2, 5 )
STRUCTURAL_INTEGRITY_TEST( Dexter_MotorCtrl, 6 )
STRUCTURAL_INTEGRITY_TEST( MAIS_FC, 7 )


/**
 * Verify that the binary import produces multiple distinct track widths when the
 * ASCII reference file has multiple widths. This validates per-segment width
 * resolution from sec62 rather than defaulting everything to one width.
 */
#define WIDTH_VARIETY_TEST( name, idx )                                                                                \
    BOOST_AUTO_TEST_CASE( TrackWidthVariety_##name )                                                                   \
    {                                                                                                                  \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                                              \
                                                                                                                       \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                                                 \
                                                                                                                       \
        std::set<int> binWidths;                                                                                       \
        std::set<int> ascWidths;                                                                                       \
                                                                                                                       \
        for( PCB_TRACK * trk : bin->Tracks() )                                                                         \
        {                                                                                                              \
            if( trk->Type() == PCB_TRACE_T )                                                                           \
                binWidths.insert( trk->GetWidth() );                                                                   \
        }                                                                                                              \
                                                                                                                       \
        for( PCB_TRACK * trk : asc->Tracks() )                                                                         \
        {                                                                                                              \
            if( trk->Type() == PCB_TRACE_T )                                                                           \
                ascWidths.insert( trk->GetWidth() );                                                                   \
        }                                                                                                              \
                                                                                                                       \
        if( ascWidths.size() > 1 )                                                                                     \
        {                                                                                                              \
            BOOST_WARN_MESSAGE( binWidths.size() > 1, #name " binary should have multiple distinct track widths "      \
                                                            "(found "                                                  \
                                                              << binWidths.size() << ", ASCII has "                    \
                                                              << ascWidths.size() << ")" );                            \
        }                                                                                                              \
    }

WIDTH_VARIETY_TEST( TMS1mmX19, 0 )
WIDTH_VARIETY_TEST( Ems4_Rev2, 3 )
WIDTH_VARIETY_TEST( LCORE_4, 4 )
WIDTH_VARIETY_TEST( LCORE_2, 5 )
WIDTH_VARIETY_TEST( Dexter_MotorCtrl, 6 )
WIDTH_VARIETY_TEST( MAIS_FC, 7 )


/**
 * Geometric regression guard for the structural copper/keepout vertex link.
 *
 * Copper and keepout zone polygons are sliced out of the sec12 vertex pool by each owner's
 * structural vertexStart cursor read from the directly framed circular owner array, not by
 * bounding-box matching. A wrong section-12 origin or a one-record owner-lag
 * error leaves the zone COUNT intact while shifting every structural vertex, so the
 * ZoneCountExact tests cannot catch it.
 *
 * This test therefore probes the parser directly: it asserts the structural owner loop for
 * named owners equals the design-coordinate vertices the validated shape-vertex-link
 * harness produced (100% geometric ASC match across the v0x2025/26/27 corpus). It covers
 * one owner per new-format version: MC4_PLUS_CSHAPE (v0x2027, a copper fill and an
 * arc-ordinal keepout), Ems4_Rev2 (v0x2025, two copper fills) and LCORE_4 (v0x2026, a
 * copper fill). No BR board (v0x2027) ships in the in-tree corpus; MC4 is the v0x2027
 * representative.
 */
BOOST_AUTO_TEST_CASE( StructuralZoneVertices )
{
    struct EXPECTED_OWNER
    {
        std::string                      boardDir;
        std::string                      binaryFile;
        std::string                      owner;
        std::vector<std::pair<int, int>> firstVerts; // design coords, sec12 units
    };

    const std::vector<EXPECTED_OWNER> cases = {
        { "MC4_PLUS_CSHAPE",
          "MC4_PLUS_CSHAPE.pcb",
          "DRW68014421",
          { { 0, 0 }, { 0, 5486400 }, { 9982200, 5486400 }, { 10210800, 5257800 } } },
        { "MC4_PLUS_CSHAPE",
          "MC4_PLUS_CSHAPE.pcb",
          "DRW16024650",
          { { 0, 0 }, { 0, 9784905 }, { -452970, 8882805 }, { -1905015, -2943150 } } },
        { "Ems4_Rev2",
          "Ems4_Rev2.pcb",
          "DRW55434270",
          { { 0, -571500 }, { 0, 1333500 }, { 1714500, 1333500 }, { 1714500, 6096000 } } },
        { "Ems4_Rev2",
          "Ems4_Rev2.pcb",
          "DRW1329638",
          { { 0, 0 }, { 0, -2095500 }, { 2857500, -2095500 }, { 3048000, -2286000 } } },
        { "LCORE_4",
          "LCORE_4.pcb",
          "DRW47981509",
          { { 803078, -1462546 }, { -1259549, -2233786 }, { -295519, -4678038 }, { 1767106, -3906796 } } },
    };

    std::map<std::string, std::shared_ptr<PADS_IO::BINARY_PARSER>> parsers;

    for( const EXPECTED_OWNER& ec : cases )
    {
        BOOST_TEST_CONTEXT( ec.boardDir << " " << ec.owner )
        {
            std::shared_ptr<PADS_IO::BINARY_PARSER>& parser = parsers[ec.boardDir];

            if( !parser )
            {
                parser = std::make_shared<PADS_IO::BINARY_PARSER>();
                wxString filename =
                        KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + ec.boardDir + "/" + ec.binaryFile;
                parser->Parse( filename );
            }

            std::vector<VECTOR2I> loop;

            BOOST_REQUIRE_MESSAGE( parser->GetOwnerLoopForTest( ec.owner, loop ),
                                   ec.owner << " has no structural sec12 loop" );

            BOOST_REQUIRE_GE( loop.size(), ec.firstVerts.size() );

            for( size_t i = 0; i < ec.firstVerts.size(); ++i )
            {
                BOOST_CHECK_EQUAL( loop[i].x, ec.firstVerts[i].first );
                BOOST_CHECK_EQUAL( loop[i].y, ec.firstVerts[i].second );
            }
        }
    }
}


// PADS name fields use a legacy 8-bit code page, so a high byte must survive the fixed-string read
// and decode through ConvertText. Rejecting it blanks the whole field, and the same board then
// imports with names from .asc and without them from .pcb.
BOOST_AUTO_TEST_CASE( FixedStringKeepsHighBytes )
{
    // "Res" with a CP1252 e-acute in the middle
    std::vector<uint8_t> field = { 'R', 0xE9, 's', 0 };

    std::string raw = PADS_IO::readFixedString( field, 0, 4 );

    BOOST_REQUIRE_EQUAL( raw.size(), 3u );
    BOOST_CHECK_EQUAL( static_cast<unsigned>( static_cast<uint8_t>( raw[1] ) ), 0xE9u );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertText( raw ), wxString::FromUTF8( "R\xC3\xA9s" ) );

    // Net names reach the board through the inverted-name wrapper, so it has to decode as well
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( raw ), wxString::FromUTF8( "R\xC3\xA9s" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "/" + raw ),
                       wxString::FromUTF8( "~{R\xC3\xA9s}" ) );
}


// A control byte still invalidates the field, and only trailing spaces are trimmed. StrPurge also
// stripped leading whitespace, which the documented contract does not allow.
BOOST_AUTO_TEST_CASE( FixedStringRejectsControlAndKeepsLeadingSpace )
{
    std::vector<uint8_t> control = { 'A', 0x01, 'B', 0 };

    BOOST_CHECK( PADS_IO::readFixedString( control, 0, 4 ).empty() );

    std::vector<uint8_t> padded = { ' ', ' ', 'N', 'E', 'T', ' ', ' ', 0 };

    BOOST_CHECK_EQUAL( PADS_IO::readFixedString( padded, 0, 8 ), std::string( "  NET" ) );
}


// A section-14 record count that overruns the file must be rejected before it sizes the decal-name
// table. Without the extent guard the reserve raises std::bad_alloc, which is not an IO_ERROR and
// so escapes the plugin's catch.
BOOST_AUTO_TEST_CASE( DecalNameTableRejectsOversizedCount )
{
    // The directory entry count is a u32 at HEADER_SIZE + DecalHeader * DIR_ENTRY_SIZE
    const size_t sec14CountOffset = 10 + 14 * 16;

    std::ifstream in( GetBinaryPath( PADS_BINARY_BOARDS[4] ).ToStdString(), std::ios::binary );

    BOOST_REQUIRE( in.good() );

    std::vector<char> bytes( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

    in.close();

    BOOST_REQUIRE_GT( bytes.size(), sec14CountOffset + 4 );

    bytes[sec14CountOffset + 0] = static_cast<char>( 0xFF );
    bytes[sec14CountOffset + 1] = static_cast<char>( 0xFF );
    bytes[sec14CountOffset + 2] = static_cast<char>( 0xFF );
    bytes[sec14CountOffset + 3] = static_cast<char>( 0x0F );

    std::filesystem::path mutated = std::filesystem::temp_directory_path()
                                    / "kicad_pads_sec14_oversized_count.pcb";

    {
        std::ofstream out( mutated, std::ios::binary );

        out.write( bytes.data(), static_cast<std::streamsize>( bytes.size() ) );
    }

    PADS_IO::BINARY_PARSER parser;

    BOOST_CHECK_EXCEPTION( parser.Parse( wxString::FromUTF8( mutated.string() ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxT( "decal-name table extent" ) );
                           } );

    std::filesystem::remove( mutated );
}


/**
 * Section-4 pad-shape enum decode.
 *
 * The shape code is an enum {0:OF, 1:RF, 2:R, 3:S}; the earlier map left code 0
 * unmapped so every oblong (OF) padstack was imported as round. TMS1mmX19 and
 * MC4_PLUS_CSHAPE both carry code-0 oblong padstacks, so the decoded shape set must
 * contain "OF". Asserted at the parser level because the importer currently assigns a
 * single default padstack per decal, which would mask the per-padstack shape.
 */
BOOST_AUTO_TEST_CASE( PadStackShapeEnum_OblongDecoded )
{
    const std::vector<std::string> boardsWithOblong = { "TMS1mmX19", "MC4_PLUS_CSHAPE" };

    for( const std::string& dir : boardsWithOblong )
    {
        BOOST_TEST_CONTEXT( dir )
        {
            PADS_IO::BINARY_PARSER parser;
            wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + dir + "/" + dir + ".pcb";

            parser.Parse( filename );

            std::set<std::string> shapes = parser.GetPadStackShapesForTest();

            BOOST_CHECK_MESSAGE( shapes.count( "OF" ),
                                 dir << " decoded no OF padstack (shape-enum off-by-one regression)" );

            // The same boards carry round and rectangular-finger padstacks, so a decode
            // that collapsed everything to one shape would also be caught.
            BOOST_CHECK_MESSAGE( shapes.count( "R" ), dir << " decoded no round padstack" );
            BOOST_CHECK_MESSAGE( shapes.count( "RF" ), dir << " decoded no RF padstack" );
        }
    }
}


/**
 * De-duplicated passive terminal positions (the sec14 trailer pool + sec15 unified stream,
 * addressed by the per-decal +68 cursor in the decal-name header table).
 *
 * The high-volume 2-pin passives (0402/0603/...) are de-duplicated out of section 15 and
 * previously imported with their pads stacked at the origin (the synthesizePlaceholderTerminals
 * fallback). The +68 cursor recovers each decal's exact decal-local terminal window. Values
 * here are the binary-decoded positions, independently confirmed against the MC4 ASC
 * *PARTDECAL* terminals.
 */
BOOST_AUTO_TEST_CASE( DedupePoolTerminalPositions_MC4 )
{
    PADS_IO::BINARY_PARSER parser;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/MC4_PLUS_CSHAPE/MC4_PLUS_CSHAPE.pcb";

    parser.Parse( filename );

    const std::map<std::string, PADS_IO::PART_DECAL>& decals = parser.GetPartDecals();

    struct Expected
    {
        std::string                            name;
        std::vector<std::pair<double, double>> terms;
    };

    const std::vector<Expected> cases = {
        { "0402", { { -750000, 0 }, { 750000, 0 } } },          { "0603", { { -945000, 0 }, { 945000, 0 } } },
        { "0805", { { -1200000, 0 }, { 1200000, 0 } } },        { "1206", { { -1800000, 0 }, { 1800000, 0 } } },
        { "CC3", { { 0, -571500 }, { 0, 0 }, { 0, 571500 } } },
    };

    for( const Expected& e : cases )
    {
        BOOST_TEST_CONTEXT( e.name )
        {
            auto it = decals.find( e.name );
            BOOST_REQUIRE_MESSAGE( it != decals.end(), "decal " << e.name << " missing" );

            const std::vector<PADS_IO::TERMINAL>& terms = it->second.terminals;
            BOOST_REQUIRE_EQUAL( terms.size(), e.terms.size() );

            for( size_t t = 0; t < e.terms.size(); ++t )
            {
                BOOST_CHECK_EQUAL( terms[t].x, e.terms[t].first );
                BOOST_CHECK_EQUAL( terms[t].y, e.terms[t].second );
            }
        }
    }
}


//---------------------------------------------------------------------------------------
// WAVE 3 round-trip oracle tests: part clusters (groups), dimensions, stackup, diff pairs.
//---------------------------------------------------------------------------------------

/**
 * PADS *CLUSTER* part groups become KiCad PCB_GROUPs.
 *
 * MC4_PLUS_CSHAPE (v0x2027) carries two clusters in its .asc *CLUSTER* section:
 * CLU_DCDC5V (CLSTID 1, 16 parts) and CLU_DCDC3V3 (CLSTID 2, 16 parts). The binary
 * decode (sec22 +108 CLSTID -> 60-byte cluster-table ordinal -> name) must reproduce
 * both named groups with their exact 16-member footprint sets.
 */
BOOST_AUTO_TEST_CASE( ClusterGroups_MC4_PLUS_CSHAPE )
{
    const PADS_BINARY_BOARD_INFO board{ "MC4_PLUS_CSHAPE", "MC4_PLUS_CSHAPE.pcb", "MC4_PLUS_CSHAPE.asc", false };

    std::shared_ptr<BOARD> brd = LoadBinary( board );
    BOOST_REQUIRE( brd != nullptr );

    std::map<wxString, std::set<wxString>> groupMembers;

    for( PCB_GROUP* group : brd->Groups() )
    {
        std::set<wxString>& refs = groupMembers[group->GetName()];

        for( EDA_ITEM* item : group->GetItems() )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
                refs.insert( static_cast<FOOTPRINT*>( item )->GetReference() );
        }
    }

    BOOST_REQUIRE_MESSAGE( groupMembers.count( "CLU_DCDC5V" ), "cluster CLU_DCDC5V missing from PCB_GROUPs" );
    BOOST_REQUIRE_MESSAGE( groupMembers.count( "CLU_DCDC3V3" ), "cluster CLU_DCDC3V3 missing from PCB_GROUPs" );

    const std::set<wxString> clu5v = { "C79", "C80", "C81", "C82", "C83", "C84", "C85", "C86",
                                       "D4",  "L2",  "R84", "R85", "R86", "R87", "R89", "U10" };
    const std::set<wxString> clu3v3 = { "C72", "C73", "C74", "C75", "C76", "C77", "C78", "D3",
                                        "L1",  "R77", "R78", "R79", "R80", "R81", "R83", "U9" };

    BOOST_CHECK( groupMembers["CLU_DCDC5V"] == clu5v );
    BOOST_CHECK( groupMembers["CLU_DCDC3V3"] == clu3v3 );
}


BOOST_AUTO_TEST_CASE( ModernConnectionsUseDeclaredSection24Ring )
{
    const wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* text = sdb.Section( 8 );
    const PADS_IO::SDB_SECTION* connections = sdb.Section( 24 );
    BOOST_REQUIRE( text != nullptr );
    BOOST_REQUIRE( connections != nullptr );
    BOOST_REQUIRE_GT( connections->count, 2u );
    BOOST_REQUIRE_GE( text->physicalBytes, 68u );
    BOOST_REQUIRE_GE( connections->physicalOffset, 36u );

    const size_t declaredRecord = connections->physicalOffset - 36 + 2 * 68;
    BOOST_REQUIRE_LE( declaredRecord + 68, bytes.size() );

    std::copy_n( bytes.begin() + declaredRecord, 68, bytes.begin() + text->physicalOffset );
    bytes[declaredRecord + 23] = 0x42;

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_connection_decoy_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    bool rejectedDeclaredRing = false;

    try
    {
        PADS_IO::BINARY_PARSER parser;
        parser.Parse( tempPath );
    }
    catch( const std::exception& e )
    {
        rejectedDeclaredRing = std::string( e.what() ).find( "net-connection ring framing" ) != std::string::npos;
    }

    wxRemoveFile( tempPath );
    BOOST_CHECK( rejectedDeclaredRing );
}


BOOST_AUTO_TEST_CASE( ModernConnectionsRequireSerializedFinalEdgeHead )
{
    const wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* connections = sdb.Section( 24 );
    BOOST_REQUIRE( connections != nullptr );
    BOOST_REQUIRE_GE( connections->physicalOffset, 36u );

    const size_t finalHead = connections->physicalOffset - 36 + static_cast<size_t>( connections->count ) * 68;
    BOOST_REQUIRE_LE( finalHead + 36, bytes.size() );

    bytes[finalHead + 23] = 0x42;

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_connection_final_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    bool rejectedFinalHead = false;

    try
    {
        PADS_IO::BINARY_PARSER parser;
        parser.Parse( tempPath );
    }
    catch( const std::exception& e )
    {
        rejectedFinalHead = std::string( e.what() ).find( "net-connection ring framing" ) != std::string::npos;
    }

    wxRemoveFile( tempPath );
    BOOST_CHECK( rejectedFinalHead );
}


BOOST_AUTO_TEST_CASE( LegacyConnectionsRequireDeclaredRingMarker )
{
    const wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/Dexter_MotorCtrl/Dexter_MotorCtrl.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* connections = sdb.Section( 24 );
    BOOST_REQUIRE( connections != nullptr );
    BOOST_REQUIRE_GT( connections->count, 0u );

    const size_t firstRecord = static_cast<size_t>( connections->physicalOffset ) + 16;
    BOOST_REQUIRE_LE( firstRecord + 68, bytes.size() );
    bytes[firstRecord + 39] = 0x42;

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_connection_legacy_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    bool rejectedLegacyRing = false;

    try
    {
        PADS_IO::BINARY_PARSER parser;
        parser.Parse( tempPath );
    }
    catch( const std::exception& e )
    {
        rejectedLegacyRing = std::string( e.what() ).find( "legacy net-connection ring framing" ) != std::string::npos;
    }

    wxRemoveFile( tempPath );
    BOOST_CHECK( rejectedLegacyRing );
}


/**
 * Direct coverage of the SDB database container: the header, the section directory and
 * the coordinate origin decoded from a known board, independent of the board-level
 * import. Locks the database-centric foundation the section readers sit on.
 */
BOOST_AUTO_TEST_CASE( SdbContainerDecode )
{
    wxString path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE_MESSAGE( PADS_IO::ReadFileToBuffer( path, bytes ), "LCORE_2.pcb test data should be readable" );

    PADS_IO::PADS_SDB sdb;
    BOOST_REQUIRE_NO_THROW( sdb.Load( std::move( bytes ) ) );

    // LCORE_2 is a v0x2026 board: the modern 75-entry directory and a per-axis origin.
    BOOST_CHECK_EQUAL( sdb.Version(), 0x2026 );
    BOOST_CHECK( !sdb.IsOldFormat() );
    BOOST_CHECK_EQUAL( sdb.SectionCount(), 75u );

    BOOST_REQUIRE( sdb.Coords().Found() );
    BOOST_CHECK_EQUAL( sdb.Coords().OriginX(), -2290000 );
    BOOST_CHECK_EQUAL( sdb.Coords().OriginY(), -213230500 );

    // design = raw - origin, so the origin itself maps to design (0, 0).
    BOOST_CHECK_EQUAL( sdb.Coords().DesignX( sdb.Coords().OriginX() ), 0 );
    BOOST_CHECK_EQUAL( sdb.Coords().DesignY( sdb.Coords().OriginY() ), 0 );

    // The physical net controller is present after the rotated board-setup view.
    const PADS_IO::SDB_SECTION* nets = sdb.Section( 23 );
    BOOST_REQUIRE( nets != nullptr );
    BOOST_CHECK( sdb.Coords().HeaderBase() > 0 );
    BOOST_CHECK( nets->physicalOffset > sdb.Coords().HeaderBase() );
    BOOST_CHECK_EQUAL( nets->physicalBytes, nets->totalBytes );

    // Out-of-range section indices return null rather than indexing past the directory.
    BOOST_CHECK( sdb.Section( -1 ) == nullptr );
    BOOST_CHECK( sdb.Section( 10000 ) == nullptr );
}


BOOST_AUTO_TEST_CASE( SdbPagedControllersUseSerializedPageCounts )
{
    wxString path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/TMS1mmX19/TMS1mmX19.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE_MESSAGE( PADS_IO::ReadFileToBuffer( path, bytes ), "TMS1mmX19.pcb test data should be readable" );

    PADS_IO::PADS_SDB sdb;
    BOOST_REQUIRE_NO_THROW( sdb.Load( std::move( bytes ) ) );

    BOOST_CHECK_EQUAL( sdb.SectionCount(), 75u );

    const PADS_IO::SDB_SECTION* pageDirectory = sdb.Section( 26 );
    const PADS_IO::SDB_SECTION* clearance = sdb.Section( 41 );
    const PADS_IO::SDB_SECTION* highSpeed = sdb.Section( 42 );
    const PADS_IO::SDB_SECTION* route = sdb.Section( 46 );
    const PADS_IO::SDB_SECTION* diffPair = sdb.Section( 48 );
    const PADS_IO::SDB_SECTION* netRelationships = sdb.Section( 49 );
    const PADS_IO::SDB_SECTION* stringPool = sdb.Section( 57 );
    const PADS_IO::SDB_SECTION* layers = sdb.Section( 69 );
    const PADS_IO::SDB_SECTION* layerState = sdb.Section( 70 );
    const PADS_IO::SDB_SECTION* displayPreferences = sdb.Section( 71 );
    const PADS_IO::SDB_SECTION* fonts = sdb.Section( 73 );

    BOOST_REQUIRE( pageDirectory != nullptr );
    BOOST_REQUIRE( clearance != nullptr );
    BOOST_REQUIRE( highSpeed != nullptr );
    BOOST_REQUIRE( route != nullptr );
    BOOST_REQUIRE( diffPair != nullptr );
    BOOST_REQUIRE( netRelationships != nullptr );
    BOOST_REQUIRE( stringPool != nullptr );
    BOOST_REQUIRE( layers != nullptr );
    BOOST_REQUIRE( layerState != nullptr );
    BOOST_REQUIRE( displayPreferences != nullptr );
    BOOST_REQUIRE( fonts != nullptr );

    BOOST_CHECK_EQUAL( pageDirectory->physicalOffset, 474496u );
    BOOST_CHECK_EQUAL( pageDirectory->physicalBytes, 120u );

    BOOST_CHECK_EQUAL( clearance->physicalOffset, 474660u );
    BOOST_CHECK_EQUAL( clearance->physicalCount, 1u );
    BOOST_CHECK_EQUAL( clearance->physicalBytes, 188u );

    BOOST_CHECK_EQUAL( highSpeed->physicalOffset, 474848u );
    BOOST_CHECK_EQUAL( highSpeed->physicalCount, 1u );
    BOOST_CHECK_EQUAL( highSpeed->physicalBytes, 80u );

    BOOST_CHECK_EQUAL( route->physicalOffset, 474928u );
    BOOST_CHECK_EQUAL( route->physicalCount, 1u );
    BOOST_CHECK_EQUAL( route->physicalLiveCount, 1u );
    BOOST_CHECK_EQUAL( route->physicalBytes, 40u );

    BOOST_CHECK_EQUAL( diffPair->physicalOffset, 474968u );
    BOOST_CHECK_EQUAL( diffPair->physicalCount, 10u );
    BOOST_CHECK_EQUAL( diffPair->physicalBytes, 8640u );

    BOOST_CHECK_EQUAL( netRelationships->physicalOffset, 483608u );
    BOOST_CHECK_EQUAL( netRelationships->physicalBytes, 44000u );

    BOOST_CHECK_EQUAL( stringPool->physicalOffset, 549792u );
    BOOST_CHECK_EQUAL( stringPool->physicalBytes, 36703u );

    BOOST_CHECK_EQUAL( layers->physicalOffset, 738695u );
    BOOST_CHECK_EQUAL( layers->physicalBytes, 4724u );
    BOOST_CHECK_EQUAL( layerState->physicalOffset, 743419u );
    BOOST_CHECK_EQUAL( layerState->physicalBytes, 4u );
    BOOST_CHECK_EQUAL( displayPreferences->physicalOffset, 743423u );
    BOOST_CHECK_EQUAL( displayPreferences->physicalBytes, 1144u );
    BOOST_CHECK_EQUAL( fonts->physicalOffset, 744567u );
    BOOST_CHECK_EQUAL( fonts->physicalBytes, 52u );
}


BOOST_AUTO_TEST_CASE( SdbRouteRuleSlotLivenessUsesSerializedSlotState )
{
    wxString path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/TMS1mmX19/TMS1mmX19.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( path, bytes ) );

    PADS_IO::PADS_SDB baseline;
    baseline.Load( bytes );

    const PADS_IO::SDB_SECTION* route = baseline.Section( 46 );
    const PADS_IO::SDB_SECTION* relationships = baseline.Section( 49 );
    const PADS_IO::SDB_SECTION* stringPool = baseline.Section( 57 );
    const PADS_IO::SDB_SECTION* layers = baseline.Section( 69 );

    BOOST_REQUIRE( route != nullptr );
    BOOST_REQUIRE( relationships != nullptr );
    BOOST_REQUIRE( stringPool != nullptr );
    BOOST_REQUIRE( layers != nullptr );
    BOOST_REQUIRE_EQUAL( route->physicalLiveCount, 1u );

    const size_t stateOffset = relationships->physicalOffset + relationships->physicalBytes;
    BOOST_REQUIRE( stateOffset + 4 <= bytes.size() - 42 );

    auto removeState = [stateOffset]( std::vector<uint8_t>& aBytes )
    {
        aBytes.erase( aBytes.begin() + stateOffset, aBytes.begin() + stateOffset + 4 );

        uint32_t containerOffset = static_cast<uint32_t>( aBytes[aBytes.size() - 4] )
                                   | static_cast<uint32_t>( aBytes[aBytes.size() - 3] ) << 8
                                   | static_cast<uint32_t>( aBytes[aBytes.size() - 2] ) << 16
                                   | static_cast<uint32_t>( aBytes[aBytes.size() - 1] ) << 24;
        containerOffset -= 4;
        aBytes[aBytes.size() - 4] = static_cast<uint8_t>( containerOffset );
        aBytes[aBytes.size() - 3] = static_cast<uint8_t>( containerOffset >> 8 );
        aBytes[aBytes.size() - 2] = static_cast<uint8_t>( containerOffset >> 16 );
        aBytes[aBytes.size() - 1] = static_cast<uint8_t>( containerOffset >> 24 );
    };

    std::vector<uint8_t> zeroHandle = bytes;
    std::fill_n( zeroHandle.begin() + route->physicalOffset + 4, 4, uint8_t{ 0 } );
    removeState( zeroHandle );

    PADS_IO::PADS_SDB zeroHandleSlot;
    BOOST_REQUIRE_NO_THROW( zeroHandleSlot.Load( std::move( zeroHandle ) ) );
    BOOST_CHECK_EQUAL( zeroHandleSlot.Section( 46 )->physicalLiveCount, 0u );
    BOOST_CHECK_EQUAL( zeroHandleSlot.Section( 57 )->physicalOffset, stringPool->physicalOffset - 4 );
    BOOST_CHECK_EQUAL( zeroHandleSlot.Section( 69 )->physicalOffset, layers->physicalOffset - 4 );

    std::vector<uint8_t> negativeHandle = bytes;
    negativeHandle[route->physicalOffset + 3] |= 0x80;
    removeState( negativeHandle );

    PADS_IO::PADS_SDB negativeHandleSlot;
    BOOST_REQUIRE_NO_THROW( negativeHandleSlot.Load( std::move( negativeHandle ) ) );
    BOOST_CHECK_EQUAL( negativeHandleSlot.Section( 46 )->physicalLiveCount, 0u );
    BOOST_CHECK_EQUAL( negativeHandleSlot.Section( 57 )->physicalOffset, stringPool->physicalOffset - 4 );
    BOOST_CHECK_EQUAL( negativeHandleSlot.Section( 69 )->physicalOffset, layers->physicalOffset - 4 );
}


BOOST_AUTO_TEST_CASE( SdbStringPoolUsesDeclaredSection57Extent )
{
    wxString path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/TMS1mmX19/TMS1mmX19.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( path, bytes ) );

    PADS_IO::PADS_SDB baseline;
    baseline.Load( bytes );

    const PADS_IO::SDB_SECTION* text = baseline.Section( 8 );
    const PADS_IO::SDB_SECTION* strings = baseline.Section( 57 );
    BOOST_REQUIRE( text != nullptr );
    BOOST_REQUIRE( strings != nullptr );
    BOOST_REQUIRE_GE( text->physicalBytes, 64u );
    BOOST_REQUIRE_GE( strings->physicalBytes, 64u );

    std::vector<uint8_t> decoy = bytes;
    std::copy_n( decoy.begin() + strings->physicalOffset, 64, decoy.begin() + text->physicalOffset );

    PADS_IO::PADS_SDB withDecoy;
    BOOST_REQUIRE_NO_THROW( withDecoy.Load( std::move( decoy ) ) );
    BOOST_CHECK_EQUAL( withDecoy.Section( 57 )->physicalOffset, strings->physicalOffset );
    BOOST_CHECK_EQUAL( withDecoy.Section( 57 )->physicalBytes, strings->physicalBytes );

    std::vector<uint8_t> corrupt = bytes;
    const size_t         extentOffset = 10 + 57 * 16 + 4;
    uint32_t             extent = strings->totalBytes + 1;
    corrupt[extentOffset] = static_cast<uint8_t>( extent );
    corrupt[extentOffset + 1] = static_cast<uint8_t>( extent >> 8 );
    corrupt[extentOffset + 2] = static_cast<uint8_t>( extent >> 16 );
    corrupt[extentOffset + 3] = static_cast<uint8_t>( extent >> 24 );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_string_extent_corrupt_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( corrupt.data(), corrupt.size() ), corrupt.size() );
    }

    PADS_IO::BINARY_PARSER invalid;
    BOOST_CHECK_THROW( invalid.Parse( tempPath ), std::exception );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( SdbGraphicControllersUseDeclaredCircularExtents )
{
    wxString path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( path, bytes ) );

    PADS_IO::PADS_SDB baseline;
    baseline.Load( bytes );

    const PADS_IO::SDB_SECTION* text = baseline.Section( 8 );
    const PADS_IO::SDB_SECTION* owners = baseline.Section( 10 );
    const PADS_IO::SDB_SECTION* pieces = baseline.Section( 11 );
    const PADS_IO::SDB_SECTION* vertices = baseline.Section( 12 );
    BOOST_REQUIRE( text != nullptr );
    BOOST_REQUIRE( owners != nullptr );
    BOOST_REQUIRE( pieces != nullptr );
    BOOST_REQUIRE( vertices != nullptr );
    BOOST_REQUIRE( text->physicalBytes >= 112 );

    BOOST_CHECK_EQUAL( owners->physicalBytes, owners->count * 112u );
    BOOST_CHECK_EQUAL( pieces->physicalBytes, pieces->count * 20u );
    BOOST_CHECK_EQUAL( vertices->physicalBytes, vertices->count * 12u );
    BOOST_CHECK_EQUAL( pieces->physicalOffset, owners->physicalOffset + owners->physicalBytes );
    BOOST_CHECK_EQUAL( vertices->physicalOffset, pieces->physicalOffset + pieces->physicalBytes );

    std::vector<uint8_t> decoy = bytes;
    std::copy_n( decoy.begin() + owners->physicalOffset, 112, decoy.begin() + text->physicalOffset );

    PADS_IO::PADS_SDB withDecoy;
    BOOST_REQUIRE_NO_THROW( withDecoy.Load( std::move( decoy ) ) );
    BOOST_CHECK_EQUAL( withDecoy.Section( 10 )->physicalOffset, owners->physicalOffset );
    BOOST_CHECK_EQUAL( withDecoy.Section( 11 )->physicalOffset, pieces->physicalOffset );
    BOOST_CHECK_EQUAL( withDecoy.Section( 12 )->physicalOffset, vertices->physicalOffset );

    std::vector<uint8_t> corrupt = bytes;
    const size_t         countOffset = 10 + 10 * 16;
    uint32_t             count = owners->count + 1;
    corrupt[countOffset] = static_cast<uint8_t>( count );
    corrupt[countOffset + 1] = static_cast<uint8_t>( count >> 8 );
    corrupt[countOffset + 2] = static_cast<uint8_t>( count >> 16 );
    corrupt[countOffset + 3] = static_cast<uint8_t>( count >> 24 );

    PADS_IO::PADS_SDB invalid;
    BOOST_CHECK_THROW( invalid.Load( std::move( corrupt ) ), std::exception );
}


BOOST_AUTO_TEST_CASE( PourOwnersUseDeclaredSection52Array )
{
    wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_4/LCORE_4.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB sdb;
    sdb.Load( bytes );

    const PADS_IO::SDB_SECTION* text = sdb.Section( 8 );
    const PADS_IO::SDB_SECTION* owners = sdb.Section( 52 );
    BOOST_REQUIRE( text != nullptr );
    BOOST_REQUIRE( owners != nullptr );
    BOOST_REQUIRE( text->physicalBytes >= 88 );
    BOOST_REQUIRE_EQUAL( owners->physicalBytes, owners->count * 88u );

    std::copy_n( bytes.begin() + owners->physicalOffset, 88, bytes.begin() + text->physicalOffset );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_pour_decoy_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    auto board = LoadBinaryPath( tempPath, "LCORE_4 pour decoy" );
    wxRemoveFile( tempPath );

    BOOST_REQUIRE( board );
    BOOST_CHECK_EQUAL( board->Zones().size(), 4 );
}


BOOST_AUTO_TEST_CASE( SdbCorpusVersionsAreRecognized )
{
    BOOST_CHECK( PADS_IO::PADS_SDB::IsSupportedVersion( 0x2017 ) );
    BOOST_CHECK( PADS_IO::PADS_SDB::IsSupportedVersion( 0x2019 ) );
}


BOOST_AUTO_TEST_CASE( SdbRejectsUnclaimedBodyByte )
{
    wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    const size_t totalBytesOffset = 10 + 69 * 16 + 4;
    BOOST_REQUIRE( totalBytesOffset + sizeof( uint32_t ) <= bytes.size() );

    uint32_t totalBytes = static_cast<uint32_t>( bytes[totalBytesOffset] )
                          | static_cast<uint32_t>( bytes[totalBytesOffset + 1] ) << 8
                          | static_cast<uint32_t>( bytes[totalBytesOffset + 2] ) << 16
                          | static_cast<uint32_t>( bytes[totalBytesOffset + 3] ) << 24;
    BOOST_REQUIRE_GT( totalBytes, 0u );
    --totalBytes;

    for( size_t byte = 0; byte < sizeof( totalBytes ); ++byte )
        bytes[totalBytesOffset + byte] = static_cast<uint8_t>( totalBytes >> ( byte * 8 ) );

    PADS_IO::PADS_SDB sdb;
    BOOST_CHECK_THROW( sdb.Load( std::move( bytes ) ), std::exception );
}


BOOST_AUTO_TEST_CASE( RouteObjectsRejectZeroCountWithSerializedBytes )
{
    const wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    const size_t countOffset = 10 + 62 * 16;
    BOOST_REQUIRE( countOffset + sizeof( uint32_t ) <= bytes.size() );
    std::fill_n( bytes.begin() + countOffset, sizeof( uint32_t ), 0 );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_route_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    PADS_IO::BINARY_PARSER parser;
    BOOST_CHECK_THROW( parser.Parse( tempPath ), std::exception );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( VersionedLayerTablesUseSerializedNamesAndCounts )
{
    struct CASE
    {
        const char*              dir;
        const char*              file;
        std::vector<std::string> copperNames;
    };

    const std::vector<CASE> cases = {
        { "Dexter_MotorCtrl",
          "Dexter_MotorCtrl.pcb",
          { "Top", "Signal Routing 1", "Power P3_3v", "Signal Routing 2", "Signal Routing 3", "Power P33v",
            "Signal Routing 4", "Bottom" } },
        { "LCORE_2", "LCORE_2.pcb", { "Top", "Bottom" } },
        { "MC2_PLUS_REV1",
          "MC2_PLUS_REV1.pcb",
          { "Top", "Inner Layer 1 GND", "Inner Layer 2", "Inner Layer 3", "Inner Layer 4 VCC", "Bottom" } },
    };

    for( const CASE& test : cases )
    {
        PADS_IO::BINARY_PARSER parser;
        parser.Parse( KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + test.dir + "/" + test.file );

        std::vector<std::string> actual;

        for( const PADS_IO::LAYER_INFO& layer : parser.GetLayerInfos() )
        {
            if( layer.is_copper )
                actual.push_back( layer.name );
        }

        BOOST_CHECK_EQUAL_COLLECTIONS( actual.begin(), actual.end(), test.copperNames.begin(), test.copperNames.end() );
    }
}


BOOST_AUTO_TEST_CASE( DirectPlacementArraysUseSerializedCountsAndOrder )
{
    struct CASE
    {
        const char* dir;
        const char* file;
        size_t      count;
        const char* first;
        const char* last;
    };

    const std::vector<CASE> cases = {
        { "Dexter_MotorCtrl", "Dexter_MotorCtrl.pcb", 284, "J1", "D5" },
        { "LCORE_2", "LCORE_2.pcb", 30, "C4", "FL1" },
        { "MC2_PLUS_REV1", "MC2_PLUS_REV1.pcb", 402, "C1", "UC128" },
    };

    for( const CASE& test : cases )
    {
        PADS_IO::BINARY_PARSER parser;
        parser.Parse( KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + test.dir + "/" + test.file );

        const std::vector<PADS_IO::PART>& parts = parser.GetParts();
        BOOST_REQUIRE_EQUAL( parts.size(), test.count );
        BOOST_CHECK_EQUAL( parts.front().name, test.first );
        BOOST_CHECK_EQUAL( parts.back().name, test.last );
    }
}


BOOST_AUTO_TEST_CASE( SdbFooterStoresContainerItemBackPointer )
{
    static constexpr char   GUID[] = "{2FE18320-6448-11d1-A412-000000000000}";
    static constexpr size_t GUID_LENGTH = sizeof( GUID ) - 1;

    std::vector<uint8_t> bytes( 128, 0 );
    const size_t         footerStart = bytes.size() - GUID_LENGTH - sizeof( uint32_t );
    const uint32_t       containerItemsOffset = 24;

    std::copy_n( reinterpret_cast<const uint8_t*>( GUID ), GUID_LENGTH,
                 bytes.begin() + static_cast<std::ptrdiff_t>( footerStart ) );

    for( size_t i = 0; i < sizeof( containerItemsOffset ); ++i )
        bytes[footerStart + GUID_LENGTH + i] = static_cast<uint8_t>( containerItemsOffset >> ( i * 8 ) );

    BOOST_REQUIRE_NO_THROW( PADS_IO::ValidateSdbFooter( bytes, footerStart, GUID, GUID_LENGTH ) );

    // The offset is a size_t on a public entry point, so a near-SIZE_MAX value must be rejected
    // rather than wrap the length sum back inside the buffer and reach the memcmp
    BOOST_CHECK_THROW( PADS_IO::ValidateSdbFooter( bytes, SIZE_MAX - 8, GUID, GUID_LENGTH ),
                       IO_ERROR );
    BOOST_CHECK_THROW( PADS_IO::ValidateSdbFooter( bytes, SIZE_MAX, GUID, GUID_LENGTH ), IO_ERROR );

    // A buffer shorter than the footer itself must not underflow the remaining-bytes arithmetic
    std::vector<uint8_t> tiny( 2, 0 );

    BOOST_CHECK_THROW( PADS_IO::ValidateSdbFooter( tiny, 0, GUID, GUID_LENGTH ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( SdbRejectsContainerBackPointerInsideFooter )
{
    const wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );
    BOOST_REQUIRE_GE( bytes.size(), 42u );

    const uint32_t invalidOffset = static_cast<uint32_t>( bytes.size() - 41 );

    for( size_t byte = 0; byte < sizeof( invalidOffset ); ++byte )
        bytes[bytes.size() - sizeof( invalidOffset ) + byte] = static_cast<uint8_t>( invalidOffset >> ( byte * 8 ) );

    PADS_IO::PADS_SDB sdb;
    BOOST_CHECK_THROW( sdb.Load( std::move( bytes ) ), std::exception );
}


/**
 * The board-setup block is reached by arithmetic on two declared counts, not by sweeping
 * section 1 for a field-range signature. The origin lives at that base +60/+64 and shifts
 * every absolute coordinate in the file, so this pins the most load-bearing offset in the
 * container.
 *
 * Recomputing the base here from the raw bytes keeps the test independent of the reader: a
 * change that reintroduces a search would still have to land on the declared offset.
 */
BOOST_AUTO_TEST_CASE( OriginBaseIsDeclaredNotSearched )
{
    const std::vector<std::string> boards = {
        "Dexter_MotorCtrl/Dexter_MotorCtrl.pcb",
        "Ems4_Rev2/Ems4_Rev2.pcb",
        "LCORE_2/LCORE_2.pcb",
        "LCORE_4/LCORE_4.pcb",
        "MAIS_FC/MAIS_FC.pcb",
        "MC2_PLUS_REV1/MC2_PLUS_REV1.pcb",
        "TMS1mmX19/TMS1mmX19.pcb",
    };

    for( const std::string& board : boards )
    {
        wxString             path = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + board;
        std::vector<uint8_t> bytes;

        BOOST_REQUIRE_MESSAGE( PADS_IO::ReadFileToBuffer( path, bytes ), board << " should be readable" );

        auto u32At = [&bytes]( size_t aOffset )
        {
            return static_cast<uint32_t>( bytes[aOffset] ) | ( static_cast<uint32_t>( bytes[aOffset + 1] ) << 8 )
                   | ( static_cast<uint32_t>( bytes[aOffset + 2] ) << 16 )
                   | ( static_cast<uint32_t>( bytes[aOffset + 3] ) << 24 );
        };

        const uint32_t slots = u32At( 10 + 16 );          // directory[1].count
        const uint32_t dirBytes = u32At( 10 + 16 + 4 );   // directory[1].total_bytes
        const uint32_t viewStates = u32At( 10 + 2 * 16 ); // directory[2].count

        BOOST_TEST_CONTEXT( board )
        {
            // The directory declares its own byte size, which is what makes the slot count
            // trustworthy without a version branch.
            BOOST_CHECK_EQUAL( dirBytes, slots * 16 );

            const uint32_t expected = 10 + ( slots - 1 ) * 16 + viewStates * 48;

            PADS_IO::PADS_SDB sdb;
            BOOST_REQUIRE_NO_THROW( sdb.Load( std::move( bytes ) ) );

            BOOST_REQUIRE( sdb.Coords().Found() );
            BOOST_CHECK_EQUAL( sdb.Coords().HeaderBase(), expected );
        }
    }
}


BOOST_AUTO_TEST_CASE( CorruptDeclaredOriginDoesNotRelocateToDftMarker )
{
    wxString source = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/LCORE_2/LCORE_2.pcb";

    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_IO::ReadFileToBuffer( source, bytes ) );

    PADS_IO::PADS_SDB baseline;
    baseline.Load( bytes );
    BOOST_REQUIRE( baseline.Coords().Found() );

    const size_t scaleOffset = baseline.Coords().HeaderBase() + 56;
    BOOST_REQUIRE( scaleOffset + sizeof( float ) <= bytes.size() );
    std::fill_n( bytes.begin() + scaleOffset, sizeof( float ), uint8_t{ 0 } );

    const std::string marker = "DFT_CONFIGURATION";
    auto              markerIt = std::search( bytes.begin(), bytes.end(), marker.begin(), marker.end() );
    BOOST_REQUIRE( markerIt != bytes.end() );

    const char config[] = "X\0"
                          "12345\0"
                          "Y\0"
                          "67890\0\0";
    auto       configIt = markerIt + marker.size() + 1;
    BOOST_REQUIRE( configIt + sizeof( config ) <= bytes.end() );
    std::copy_n( reinterpret_cast<const uint8_t*>( config ), sizeof( config ), configIt );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_pads_origin_corrupt_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".pcb" );

    {
        wxFFile file( tempPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( bytes.data(), bytes.size() ), bytes.size() );
    }

    PADS_IO::BINARY_PARSER parser;
    BOOST_CHECK_THROW( parser.Parse( tempPath ), std::exception );
    wxRemoveFile( tempPath );
}


// Matches the "__"-joined, space-to-underscore flattening used for the ground-truth
// ASCII exports, so dumps and exports pair up by filename.
static std::string FlattenPath( const std::filesystem::path& aRoot, const std::filesystem::path& aFile )
{
    std::filesystem::path relPath = std::filesystem::relative( aFile, aRoot );
    std::string           out;
    bool                  first = true;

    for( const auto& part : relPath )
    {
        if( !first )
            out += "__";

        std::string p = part.string();

        for( char& c : p )
        {
            if( c == ' ' )
                c = '_';
        }

        out += p;
        first = false;
    }

    return out;
}


// The compared tuple carries the via type and both span layers, so a via whose decoded span is
// overwritten by SanitizeLayers stops matching its ASCII reference. ExactViaMatches was written
// with this commit but never called, which is why the span regression was invisible.
BOOST_AUTO_TEST_CASE( ExactViaGeometryMatchesAscii )
{
    size_t nonThroughSeen = 0;

    for( const PADS_BINARY_BOARD_INFO& board : PADS_BINARY_BOARDS )
    {
        if( board.differentRevision )
            continue;

        BOOST_TEST_CONTEXT( board.dir )
        {
            std::pair<size_t, size_t> counts =
                    ExactViaMatches( GetBinaryPath( board ), GetAscPath( board ), board.dir );

            BOOST_CHECK_EQUAL( counts.first, counts.second );

            std::shared_ptr<BOARD> ascii = LoadAscPath( GetAscPath( board ), board.dir );

            for( PCB_TRACK* item : ascii->Tracks() )
            {
                PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );

                if( via && via->GetViaType() != VIATYPE::THROUGH )
                    nonThroughSeen++;
            }
        }
    }

    // Every via in this corpus is full-stack, so the check above guards width, drill, net and
    // position but cannot expose the span ordering. ViaSpanSurvivesOnlyWhenTypeSetFirst covers
    // that; raise this to a real assertion once a blind-via board joins the corpus
    BOOST_TEST_MESSAGE( "non-through vias in the ASCII reference: " << nonThroughSeen );
}


// PCB_VIA is constructed THROUGH and SetLayerPair sanitizes the span against the current type, so
// the loader has to set the type first. Nothing in the corpus has a blind or buried via, so this
// pins the API trap the loader depends on
BOOST_AUTO_TEST_CASE( ViaSpanSurvivesOnlyWhenTypeSetFirst )
{
    BOARD board;

    board.SetCopperLayerCount( 4 );

    PCB_VIA typeFirst( &board );

    typeFirst.SetViaType( VIATYPE::BURIED );
    typeFirst.SetLayerPair( In1_Cu, In2_Cu );

    BOOST_CHECK_EQUAL( static_cast<int>( typeFirst.TopLayer() ), static_cast<int>( In1_Cu ) );
    BOOST_CHECK_EQUAL( static_cast<int>( typeFirst.BottomLayer() ), static_cast<int>( In2_Cu ) );

    // The order the loader used before the fix, kept here so the trap stays documented
    PCB_VIA spanFirst( &board );

    spanFirst.SetLayerPair( In1_Cu, In2_Cu );
    spanFirst.SetViaType( VIATYPE::BURIED );

    BOOST_CHECK_EQUAL( static_cast<int>( spanFirst.TopLayer() ), static_cast<int>( F_Cu ) );
    BOOST_CHECK_EQUAL( static_cast<int>( spanFirst.BottomLayer() ), static_cast<int>( B_Cu ) );
}


// LoadBoard must never own the board it is appending to. PCB_CONTROL::AppendBoard passes the live
// editor board, catches IO_ERROR and keeps using it, so a plugin that deletes it on the throw path
// leaves a dangling pointer. The flag observes the destructor directly rather than relying on the
// undefined behaviour a double free would produce.
namespace
{
struct TRACKED_BOARD : public BOARD
{
    explicit TRACKED_BOARD( bool& aDestroyed ) : m_destroyed( aDestroyed ) {}
    ~TRACKED_BOARD() override { m_destroyed = true; }

    bool& m_destroyed;
};
}


BOOST_AUTO_TEST_CASE( FailedAppendDoesNotDestroyCallerBoard )
{
    bool           destroyed = false;
    TRACKED_BOARD* caller = new TRACKED_BOARD( destroyed );

    caller->SetCopperLayerCount( 6 );

    PCB_IO_PADS_BINARY plugin;

    // An ASCII PADS export is not a binary container, so the parse throws
    wxString notBinary = GetAscPath( PADS_BINARY_BOARDS[0] );

    BOOST_CHECK_THROW( plugin.LoadBoard( notBinary, caller, nullptr, nullptr ), IO_ERROR );
    BOOST_CHECK( !destroyed );

    if( !destroyed )
    {
        BOOST_CHECK_EQUAL( caller->GetCopperLayerCount(), 6 );
        delete caller;
    }
}


BOOST_AUTO_TEST_SUITE_END()


/**
 * A PADS pour connects solid and an individual pad asks for plane relief through an RT/ST
 * padstack row, so relief is a per-pad property. Both dialects describe the same design and
 * must agree on which pads carry it, and on how each pour connects.
 *
 * Before the binary reader decoded RT/ST it dropped those rows and compensated by making
 * every pour THERMAL, forcing relief onto pads that PADS connects solid.
 */
BOOST_AUTO_TEST_CASE( BinaryPadConnectionMatchesAsc )
{
    auto thermalPadKeys = []( const std::shared_ptr<BOARD>& aBoard )
    {
        std::set<std::string> keys;

        for( FOOTPRINT* fp : aBoard->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetLocalZoneConnection() == ZONE_CONNECTION::THERMAL )
                {
                    keys.insert( fp->GetReference().ToStdString() + "." + pad->GetNumber().ToStdString() );
                }
            }
        }

        return keys;
    };

    auto everyPourIsSolid = []( const std::shared_ptr<BOARD>& aBoard )
    {
        for( ZONE* zone : aBoard->Zones() )
        {
            if( !zone->GetIsRuleArea() && zone->GetNetCode() > 0 && zone->GetPadConnection() != ZONE_CONNECTION::FULL )
            {
                return false;
            }
        }

        return true;
    };

    for( const PADS_BINARY_BOARD_INFO& board : PADS_BINARY_BOARDS )
    {
        BOOST_TEST_CONTEXT( board.dir )
        {
            std::shared_ptr<BOARD> binary = LoadBinary( board );
            std::shared_ptr<BOARD> ascii = LoadAsc( board );

            BOOST_REQUIRE( binary != nullptr );
            BOOST_REQUIRE( ascii != nullptr );

            std::set<std::string>    binKeys = thermalPadKeys( binary );
            std::set<std::string>    ascKeys = thermalPadKeys( ascii );
            std::vector<std::string> extra;

            // The binary reader must not invent relief the design does not ask for. It can
            // still miss some, because the RT/ST decode does not yet cover every way a v0x2027
            // file names a plane-relief padstack.
            std::set_difference( binKeys.begin(), binKeys.end(), ascKeys.begin(), ascKeys.end(),
                                 std::back_inserter( extra ) );

            BOOST_CHECK_MESSAGE( extra.empty(), "binary import invented a thermal override on "
                                                        << extra.size() << " pad(s), first "
                                                        << ( extra.empty() ? std::string( "-" ) : extra.front() ) );

            BOOST_CHECK_MESSAGE( everyPourIsSolid( binary ),
                                 "binary netted pours must connect solid, leaving relief to the "
                                 "per-pad overrides" );
            BOOST_CHECK_MESSAGE( everyPourIsSolid( ascii ),
                                 "ASCII netted pours must connect solid, leaving relief to the "
                                 "per-pad overrides" );
        }
    }
}


BOOST_AUTO_TEST_CASE( PadstackRowsMatchAsciiAcross2022And2027 )
{
    const wxString root = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/padstack_rows/";
    const wxString asciiPath = root + "padstack-v2022.asc";
    const wxString v2022Path = root + "padstack-v2022.pcb";
    const wxString v2027Path = root + "padstack-v2027.pcb";

    PADS_IO::PARSER        ascii;
    PADS_IO::BINARY_PARSER v2022;
    PADS_IO::BINARY_PARSER v2027;
    ascii.Parse( asciiPath );
    v2022.Parse( v2022Path );
    v2027.Parse( v2027Path );

    BOOST_REQUIRE_EQUAL( v2022.GetVersion(), 0x2022 );
    BOOST_REQUIRE_EQUAL( v2027.GetVersion(), 0x2027 );

    auto tuples = []( const auto& aParser, const std::set<std::string>& aDecalNames )
    {
        std::vector<std::string> result;

        for( const auto& [decalName, decal] : aParser.GetPartDecals() )
        {
            if( !aDecalNames.contains( decalName ) )
                continue;

            for( const auto& [pin, stack] : decal.pad_stacks )
            {
                for( const PADS_IO::PAD_STACK_LAYER& layer : stack )
                {
                    std::ostringstream tuple;
                    tuple << decalName << ' ' << pin << ' ' << layer.layer << ' ' << layer.shape << ' ' << std::fixed
                          << std::setprecision( 0 ) << layer.sizeA << ' ' << layer.sizeB << ' '
                          << layer.thermal_outer_diameter;
                    result.push_back( tuple.str() );
                }
            }
        }

        return result;
    };

    std::set<std::string> decalNames;

    for( const auto& [name, unused] : ascii.GetPartDecals() )
        decalNames.insert( name );

    const std::vector<std::string> asciiTuples = tuples( ascii, decalNames );
    const std::vector<std::string> v2022Tuples = tuples( v2022, decalNames );
    const std::vector<std::string> v2027Tuples = tuples( v2027, decalNames );

    for( const auto& [label, binaryTuples] : std::array<std::pair<const char*, const std::vector<std::string>*>, 2>{
                 { { "0x2022", &v2022Tuples }, { "0x2027", &v2027Tuples } } } )
    {
        BOOST_TEST_CONTEXT( label )
        {
            BOOST_REQUIRE_EQUAL( binaryTuples->size(), asciiTuples.size() );
            BOOST_CHECK_EQUAL_COLLECTIONS( binaryTuples->begin(), binaryTuples->end(), asciiTuples.begin(),
                                           asciiTuples.end() );
        }
    }

    const wxString thermalAsciiPath = root + "padstack-thermal-v2027.asc";
    const wxString thermalBinaryPath = root + "padstack-thermal-v2027.pcb";

    PADS_IO::PARSER        thermalAscii;
    PADS_IO::BINARY_PARSER thermalBinary;
    thermalAscii.Parse( thermalAsciiPath );
    thermalBinary.Parse( thermalBinaryPath );
    BOOST_REQUIRE_EQUAL( thermalBinary.GetVersion(), 0x2027 );
    decalNames.clear();

    for( const auto& [name, unused] : thermalAscii.GetPartDecals() )
        decalNames.insert( name );

    const std::vector<std::string> thermalAsciiTuples = tuples( thermalAscii, decalNames );
    const std::vector<std::string> thermalBinaryTuples = tuples( thermalBinary, decalNames );

    BOOST_REQUIRE_EQUAL( thermalBinaryTuples.size(), thermalAsciiTuples.size() );
    BOOST_CHECK_EQUAL_COLLECTIONS( thermalBinaryTuples.begin(), thermalBinaryTuples.end(), thermalAsciiTuples.begin(),
                                   thermalAsciiTuples.end() );
}


/**
 * A PADS through-hole pad opens the soldermask on both outer layers. Importing it with a copper-only
 * layer set tents every hole, so the fabrication output is wrong until the user repairs each
 * footprint by hand. An unplated hole is mechanical: it carries no inner copper and no net.
 */
BOOST_AUTO_TEST_CASE( ThroughHolePadsAreNotTented )
{
    size_t plated = 0;
    size_t unplated = 0;

    auto check = [&]( const std::shared_ptr<BOARD>& aBoard, const std::string& aLabel )
    {
        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                const std::string key =
                        aLabel + " " + footprint->GetReference().ToStdString() + "." + pad->GetNumber().ToStdString();

                if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                {
                    ++plated;
                    BOOST_CHECK_MESSAGE( pad->GetLayerSet().test( F_Mask ) && pad->GetLayerSet().test( B_Mask ),
                                         "plated hole imported tented: " << key );
                }
                else if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                {
                    ++unplated;
                    BOOST_CHECK_MESSAGE( ( pad->GetLayerSet() & LSET::InternalCuMask() ).none(),
                                         "unplated hole given inner copper: " << key );
                    BOOST_CHECK_MESSAGE( pad->GetNetCode() == 0, "unplated hole joined a net: " << key );
                }
            }
        }
    };

    for( const PADS_BINARY_BOARD_INFO& board : PADS_BINARY_BOARDS )
    {
        BOOST_TEST_CONTEXT( board.dir )
        {
            check( LoadBinary( board ), board.dir + " binary" );
            check( LoadAsc( board ), board.dir + " ASC" );
        }
    }

    BOOST_CHECK_GT( plated, 0u );
    BOOST_CHECK_GT( unplated, 0u );
}


/**
 * RT/ST rows say the pad connects to a plane through relief spokes and RA/SA rows are anti-pad
 * clearances. The ASCII reader drops both from the copper geometry; the binary reader must reach
 * the same conclusion on the same design.
 */
BOOST_AUTO_TEST_CASE( ThermalReliefRowsReachBinaryPads )
{
    const wxString root = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/padstack_rows/";

    std::shared_ptr<BOARD> binary = LoadBinaryPath( root + "padstack-thermal-v2027.pcb", "padstack-thermal-v2027" );
    std::shared_ptr<BOARD> ascii = LoadAscPath( root + "padstack-thermal-v2027.asc", "padstack-thermal-v2027" );

    BOOST_REQUIRE( binary != nullptr );
    BOOST_REQUIRE( ascii != nullptr );

    auto thermalPadKeys = []( const std::shared_ptr<BOARD>& aBoard )
    {
        std::set<std::string> keys;

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetLocalZoneConnection() == ZONE_CONNECTION::THERMAL )
                    keys.insert( footprint->GetReference().ToStdString() + "." + pad->GetNumber().ToStdString() );
            }
        }

        return keys;
    };

    const std::set<std::string> binKeys = thermalPadKeys( binary );
    const std::set<std::string> ascKeys = thermalPadKeys( ascii );

    BOOST_REQUIRE_GT( ascKeys.size(), 0u );

    std::vector<std::string> missing;
    std::set_difference( ascKeys.begin(), ascKeys.end(), binKeys.begin(), binKeys.end(),
                         std::back_inserter( missing ) );

    BOOST_CHECK_MESSAGE( missing.empty(), "binary import lost the plane relief on "
                                                  << missing.size() << " pad(s), first "
                                                  << ( missing.empty() ? std::string( "-" ) : missing.front() ) );
}
