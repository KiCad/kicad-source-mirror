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

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_shape.h>
#include <schematic.h>
#include <lib_symbol.h>
#include <stroke_params.h>
#include <geometry/eda_angle.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct SCH_ELLIPSE_FIXTURE
{
    SCH_ELLIPSE_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_sch_ellipse.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~SCH_ELLIPSE_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString GetTempFileName( const wxString& aPrefix )
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString fileName = wxFileName::CreateTempFileName( tempDir + wxFileName::GetPathSeparator() + aPrefix );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    /**
     * Helper: add a SCH_SHAPE to the first screen, save, reload, and return the
     * matching shape from the reloaded schematic.  Caller checks fields.
     */
    SCH_SHAPE* RoundTrip( SCH_SHAPE* aShape, const wxString& aTag )
    {
        SHAPE_T shapeType = aShape->GetShape();

        m_schematic->CreateDefaultScreens();
        std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
        SCH_SCREEN*             screen = topSheets[0]->GetScreen();
        screen->SetFileName( "test.kicad_sch" );

        screen->Append( aShape );

        wxString fileName = GetTempFileName( aTag );
        fileName += ".kicad_sch";
        m_tempFiles.push_back( fileName );

        SCH_IO_KICAD_SEXPR io;
        io.SaveSchematicFile( fileName, topSheets[0], m_schematic.get() );

        // aShape is now owned by the old screen and will be destroyed by Reset().
        aShape = nullptr;

        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
        SCH_SHEET* loaded = io.LoadSchematicFile( fileName, m_schematic.get() );
        m_schematic->AddTopLevelSheet( loaded );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        SCH_SCREEN* reScreen = loaded->GetScreen();

        for( SCH_ITEM* item : reScreen->Items() )
        {
            if( SCH_SHAPE* ps = dynamic_cast<SCH_SHAPE*>( item ) )
            {
                if( ps->GetShape() == shapeType )
                    return ps;
            }
        }

        return nullptr;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT*                   m_project;
    std::vector<wxString>      m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( SchEllipseRoundTrip, SCH_ELLIPSE_FIXTURE )


BOOST_AUTO_TEST_CASE( SchematicClosedEllipse )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 2540, 5080 ) ); // 100 mil, 200 mil
    shape->SetEllipseMajorRadius( 1270 );              // 50 mil
    shape->SetEllipseMinorRadius( 762 );               // 30 mil
    shape->SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_closed" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic ellipse not found after roundtrip" );

    BOOST_CHECK_EQUAL( found->GetEllipseCenter().x, 2540 );
    BOOST_CHECK_EQUAL( found->GetEllipseCenter().y, 5080 );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 1270 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 762 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SchematicEllipseArc )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 1016 ); // 40 mil
    shape->SetEllipseMinorRadius( 635 );  // 25 mil
    shape->SetEllipseRotation( EDA_ANGLE( 45.0, DEGREES_T ) );
    shape->SetEllipseStartAngle( EDA_ANGLE( 20.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 160.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_arc" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic elliptical arc not found after roundtrip" );

    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 1016 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 635 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 45.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 20.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 160.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SchematicEllipseNegativeRotation )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 2000 );
    shape->SetEllipseMinorRadius( 1000 );
    shape->SetEllipseRotation( EDA_ANGLE( -60.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_negrot" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic ellipse not found after roundtrip" );

    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), -60.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SchematicEllipseStrokeAndFill )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 1000, 1000 ) );
    shape->SetEllipseMajorRadius( 500 );
    shape->SetEllipseMinorRadius( 300 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetStroke( STROKE_PARAMS( 50, LINE_STYLE::DASH ) );
    shape->SetFillMode( FILL_T::FILLED_SHAPE );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_stroke_fill" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic ellipse not found after roundtrip" );

    BOOST_CHECK_EQUAL( found->GetStroke().GetWidth(), 50 );
    BOOST_CHECK( found->GetStroke().GetLineStyle() == LINE_STYLE::DASH );
    BOOST_CHECK( found->GetFillMode() == FILL_T::FILLED_WITH_COLOR );
}


BOOST_AUTO_TEST_CASE( SymbolEllipse )
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestEllipse" ), nullptr );

    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_DEVICE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 508 ); // 20 mil
    shape->SetEllipseMinorRadius( 254 ); // 10 mil
    shape->SetEllipseRotation( EDA_ANGLE( 15.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );
    symbol->AddDrawItem( shape );

    // Verify the shape was added and fields are accessible.
    SCH_SHAPE* found = nullptr;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( SCH_SHAPE* ps = dynamic_cast<SCH_SHAPE*>( &item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in symbol draw items" );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 508 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 254 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 15.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SymbolEllipseArc )
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestEllipseArc" ), nullptr );

    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC, LAYER_DEVICE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 762 ); // 30 mil
    shape->SetEllipseMinorRadius( 381 ); // 15 mil
    shape->SetEllipseRotation( EDA_ANGLE( -45.0, DEGREES_T ) );
    shape->SetEllipseStartAngle( EDA_ANGLE( 30.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 300.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );
    symbol->AddDrawItem( shape );

    SCH_SHAPE* found = nullptr;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( SCH_SHAPE* ps = dynamic_cast<SCH_SHAPE*>( &item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE_ARC )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Elliptical arc not found in symbol draw items" );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 762 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 381 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), -45.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 30.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 300.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SchematicEllipseArcStartEqualsEnd )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 500 );
    shape->SetEllipseMinorRadius( 300 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetEllipseStartAngle( EDA_ANGLE( 90.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 90.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_arc_eq" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic elliptical arc not found after roundtrip" );

    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 90.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 90.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( SymbolEllipseWithFill )
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestEllipseFill" ), nullptr );

    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_DEVICE );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 762 );
    shape->SetEllipseMinorRadius( 381 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );
    shape->SetFillMode( FILL_T::FILLED_SHAPE );
    symbol->AddDrawItem( shape );

    SCH_SHAPE* found = nullptr;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( SCH_SHAPE* ps = dynamic_cast<SCH_SHAPE*>( &item ) )
        {
            if( ps->GetShape() == SHAPE_T::ELLIPSE )
            {
                found = ps;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( found, "Ellipse not found in symbol draw items" );
    BOOST_CHECK( found->GetFillMode() == FILL_T::FILLED_SHAPE );
}


BOOST_AUTO_TEST_CASE( SchematicEllipseMinimumRadii )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    shape->SetEllipseMajorRadius( 1 );
    shape->SetEllipseMinorRadius( 1 );
    shape->SetEllipseRotation( ANGLE_0 );
    shape->SetStroke( STROKE_PARAMS( 25, LINE_STYLE::SOLID ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_minradii" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic ellipse not found after roundtrip" );

    BOOST_CHECK( found->GetEllipseMajorRadius() >= 1 );
    BOOST_CHECK( found->GetEllipseMinorRadius() >= 1 );
}


/**
 * Verify that every ellipse field survives a save/load roundtrip.
 * Covers the parseEllipseBody defaults documentation requirement:
 * center, major_radius, minor_radius, rotation_angle, stroke, fill, uuid.
 */
BOOST_AUTO_TEST_CASE( SchematicEllipseAllFieldsPreserved )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 3810, 7620 ) );
    shape->SetEllipseMajorRadius( 2540 );
    shape->SetEllipseMinorRadius( 1270 );
    shape->SetEllipseRotation( EDA_ANGLE( 77.5, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 38, LINE_STYLE::DOT ) );
    shape->SetFillMode( FILL_T::NO_FILL );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_all_fields" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic ellipse not found after roundtrip" );

    BOOST_CHECK_EQUAL( found->GetEllipseCenter().x, 3810 );
    BOOST_CHECK_EQUAL( found->GetEllipseCenter().y, 7620 );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 2540 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 1270 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), 77.5, 1e-6 );
    BOOST_CHECK_EQUAL( found->GetStroke().GetWidth(), 38 );
    BOOST_CHECK( found->GetStroke().GetLineStyle() == LINE_STYLE::DOT );
    BOOST_CHECK( found->GetFillMode() == FILL_T::NO_FILL );
}


/**
 * Verify that every ellipse arc field survives a save/load roundtrip.
 * Covers: center, major_radius, minor_radius, rotation_angle,
 *         start_angle, end_angle, stroke.
 */
BOOST_AUTO_TEST_CASE( SchematicEllipseArcAllFieldsPreserved )
{
    auto* shape = new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC, LAYER_NOTES );
    shape->SetEllipseCenter( VECTOR2I( 1270, 2540 ) );
    shape->SetEllipseMajorRadius( 5080 );
    shape->SetEllipseMinorRadius( 2540 );
    shape->SetEllipseRotation( EDA_ANGLE( -22.5, DEGREES_T ) );
    shape->SetEllipseStartAngle( EDA_ANGLE( 15.0, DEGREES_T ) );
    shape->SetEllipseEndAngle( EDA_ANGLE( 315.0, DEGREES_T ) );
    shape->SetStroke( STROKE_PARAMS( 50, LINE_STYLE::DASHDOT ) );

    SCH_SHAPE* found = RoundTrip( shape, "sch_ellipse_arc_all_fields" );
    BOOST_REQUIRE_MESSAGE( found, "Schematic elliptical arc not found after roundtrip" );

    BOOST_CHECK_EQUAL( found->GetEllipseCenter().x, 1270 );
    BOOST_CHECK_EQUAL( found->GetEllipseCenter().y, 2540 );
    BOOST_CHECK_EQUAL( found->GetEllipseMajorRadius(), 5080 );
    BOOST_CHECK_EQUAL( found->GetEllipseMinorRadius(), 2540 );
    BOOST_CHECK_CLOSE( found->GetEllipseRotation().AsDegrees(), -22.5, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseStartAngle().AsDegrees(), 15.0, 1e-6 );
    BOOST_CHECK_CLOSE( found->GetEllipseEndAngle().AsDegrees(), 315.0, 1e-6 );
    BOOST_CHECK_EQUAL( found->GetStroke().GetWidth(), 50 );
    BOOST_CHECK( found->GetStroke().GetLineStyle() == LINE_STYLE::DASHDOT );
}


BOOST_AUTO_TEST_SUITE_END()
