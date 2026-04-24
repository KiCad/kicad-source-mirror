/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <geometry/eda_angle.h>
#include <stroke_params.h>

#include <pcbnew_utils/board_file_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <filesystem>
#include <memory>


BOOST_AUTO_TEST_SUITE( EllipseRoundTrip )


namespace
{

/**
 * Temporary directory holder that cleans up on destruction. Keeps the file on disk
 * for the lifetime of the test so save-then-load is possible.
 */
class TEMP_FILE_HOLDER
{
public:
    TEMP_FILE_HOLDER( const std::string& aPrefix, const std::string& aSuffix )
    {
        std::filesystem::path dir = std::filesystem::temp_directory_path() / aPrefix;
        std::filesystem::create_directories( dir );
        m_path = dir / ( "board" + aSuffix );
    }

    ~TEMP_FILE_HOLDER()
    {
        std::error_code ec;
        std::filesystem::remove( m_path, ec );
        std::filesystem::remove( m_path.parent_path(), ec );
    }

    const std::filesystem::path& Path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace


BOOST_AUTO_TEST_CASE( ClosedEllipseRoundTrip )
{
    // Build a minimal board in memory with a single closed ellipse.
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 100000, 200000 ) ); // 100 mm, 200 mm
    shape->SetEllipseMajorRadius( 50000 );                 // 50 mm
    shape->SetEllipseMinorRadius( 30000 );                 // 30 mm
    shape->SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 ); // 0.15 mm
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    // Save to a temp file.
    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_closed", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    // Reload the file.
    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    // Find the ellipse in the reloaded board and verify parameters.
    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );

    BOOST_CHECK_EQUAL( found->GetEllipseCenter().x, 100000 );
    BOOST_CHECK_EQUAL( found->GetEllipseCenter().y, 200000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 50000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 30000 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseArcRoundTrip )
{
    // Same as above but with elliptical arc.
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE_ARC );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 40000 ); // 40 mm
    shape->SetEllipseMinorRadius( 25000 ); // 25 mm
    shape->SetEllipseRotation( EDA_ANGLE( 45.0, DEGREES_T ) );
    shape->SetEllipseStartAngle( EDA_ANGLE( 20.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 160.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 200 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_arc", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Elliptical arc not found in reloaded board" );

    BOOST_CHECK_EQUAL( found->GetEllipseCenter().x, 0 );
    BOOST_CHECK_EQUAL( found->GetEllipseCenter().y, 0 );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 40000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 25000 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 45.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 20.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 160.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseStrokeAndFillRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 50000, 50000 ) );
    shape->SetEllipseMajorRadius( 30000 );
    shape->SetEllipseMinorRadius( 20000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetLayer( F_SilkS );
    shape->SetStroke( STROKE_PARAMS( 250, LINE_STYLE::DASH ) );
    shape->SetFillMode( FILL_T::FILLED_SHAPE );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_stroke_fill", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK_EQUAL( found->GetStroke().GetWidth(), 250 );
    BOOST_CHECK( found->GetStroke().GetLineStyle() == LINE_STYLE::DASH );
    BOOST_CHECK( found->GetFillMode() == FILL_T::FILLED_SHAPE );
}


BOOST_AUTO_TEST_CASE( EllipseLockedRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 10000 );
    shape->SetEllipseMinorRadius( 5000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    shape->SetLocked( true );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_locked", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK( found->IsLocked() );
}


BOOST_AUTO_TEST_CASE( EllipseNegativeRotationRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 40000 );
    shape->SetEllipseMinorRadius( 20000 );
    shape->SetEllipseRotation( EDA_ANGLE( -60.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_negrot", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), -60.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseArcStartEqualsEndRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE_ARC );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 30000 );
    shape->SetEllipseMinorRadius( 15000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetEllipseStartAngle( EDA_ANGLE( 45.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 45.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_start_eq_end", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Elliptical arc not found in reloaded board" );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 45.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 45.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseMinimumRadiiRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 1 );
    shape->SetEllipseMinorRadius( 1 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_minradii", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK( found->GetEllipseMajorRadius() >= 1 );
    BOOST_CHECK( found->GetEllipseMinorRadius() >= 1 );
}


BOOST_AUTO_TEST_CASE( EllipseLargeRadiiRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 500000000 ); // 500 mm
    shape->SetEllipseMinorRadius( 250000000 ); // 250 mm
    shape->SetEllipseRotation( EDA_ANGLE( 15.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_largeradii", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 500000000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 250000000 );
}


BOOST_AUTO_TEST_CASE( FootprintEllipseRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U1" );
    fp->SetPosition( VECTOR2I( 100000, 100000 ) );

    auto shape = std::make_unique<PCB_SHAPE>( fp.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 20000 );
    shape->SetEllipseMinorRadius( 10000 );
    shape->SetEllipseRotation( EDA_ANGLE( 45.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    fp->Add( shape.release(), ADD_MODE::APPEND, true );

    board->Add( fp.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_fp", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fpFound = reloaded->Footprints().empty() ? nullptr : reloaded->Footprints().front();
    BOOST_REQUIRE_MESSAGE( fpFound, "Footprint not found in reloaded board" );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : fpFound->GraphicalItems() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "fp_ellipse not found in reloaded footprint" );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 20000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 10000 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 45.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( FootprintEllipseArcRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto fp = std::make_unique<FOOTPRINT>( board.get() );
    fp->SetReference( "U2" );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    auto shape = std::make_unique<PCB_SHAPE>( fp.get() );
    shape->SetShape( SHAPE_T::ELLIPSE_ARC );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 15000 );
    shape->SetEllipseMinorRadius( 8000 );
    shape->SetEllipseRotation( EDA_ANGLE( -30.0, DEGREES_T ) );
    shape->SetEllipseStartAngle( EDA_ANGLE( 10.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 270.0, DEGREES_T ) );
    shape->SetLayer( F_Fab );
    shape->SetWidth( 100 );
    fp->Add( shape.release(), ADD_MODE::APPEND, true );

    board->Add( fp.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_fp_arc", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fpFound = reloaded->Footprints().empty() ? nullptr : reloaded->Footprints().front();
    BOOST_REQUIRE_MESSAGE( fpFound, "Footprint not found in reloaded board" );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : fpFound->GraphicalItems() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "fp_ellipse_arc not found in reloaded footprint" );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 15000 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 8000 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), -30.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 10.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 270.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseArcWrapAroundAnglesRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE_ARC );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 30000 );
    shape->SetEllipseMinorRadius( 15000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetEllipseStartAngle( EDA_ANGLE( 350.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 370.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_wraparound", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Elliptical arc not found in reloaded board" );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 350.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 370.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseArcNegativeStartAngleRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE_ARC );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 20000 );
    shape->SetEllipseMinorRadius( 10000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetEllipseStartAngle( EDA_ANGLE( -45.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 90.0, DEGREES_T ) );
    shape->SetLayer( F_SilkS );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_negstart", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Elliptical arc not found in reloaded board" );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), -45.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 90.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseOnEdgeCutsRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 100000, 100000 ) );
    shape->SetEllipseMajorRadius( 50000 );
    shape->SetEllipseMinorRadius( 30000 );
    shape->SetEllipseRotation( EDA_ANGLE( 10.0, DEGREES_T ) );
    shape->SetLayer( Edge_Cuts );
    shape->SetWidth( 150 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_edgecuts", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK( found->GetLayer() == Edge_Cuts );
}


BOOST_AUTO_TEST_CASE( EllipseOnBackCopperRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    auto shape = std::make_unique<PCB_SHAPE>( board.get() );
    shape->SetShape( SHAPE_T::ELLIPSE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 25000 );
    shape->SetEllipseMinorRadius( 15000 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetLayer( B_Cu );
    shape->SetWidth( 200 );
    board->Add( shape.release(), ADD_MODE::APPEND, true );

    TEMP_FILE_HOLDER tmp( "kicad_qa_ellipse_roundtrip_bcu", ".kicad_pcb" );
    KI_TEST::DumpBoardToFile( *board, tmp.Path().string() );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( tmp.Path().string() );
    BOOST_REQUIRE( reloaded );

    PCB_SHAPE* found = nullptr;

    for( BOARD_ITEM* item : reloaded->Drawings() )
    {
        if( PCB_SHAPE* ps = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in reloaded board" );
    BOOST_CHECK( found->GetLayer() == B_Cu );
}


BOOST_AUTO_TEST_SUITE_END()