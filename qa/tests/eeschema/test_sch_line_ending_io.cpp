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
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_line.h>
#include <sch_shape.h>

#include <line_ending.h>
#include <stroke_params.h>

#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( SchLineEndingIo )


namespace
{

class TEMP_SYMBOL_LIBRARY
{
public:
    TEMP_SYMBOL_LIBRARY()
    {
        m_dir = wxFileName::CreateTempFileName( wxS( "kicad_qa_sch_line_ending_io_" ) );
        wxRemoveFile( m_dir );
        wxFileName::Mkdir( m_dir );

        m_path = wxFileName( m_dir, wxS( "line_ending_roundtrip.kicad_sym" ) ).GetFullPath();
    }

    ~TEMP_SYMBOL_LIBRARY()
    {
        if( wxFileName::DirExists( m_dir ) )
            wxFileName::Rmdir( m_dir, wxPATH_RMDIR_RECURSIVE );
    }

    const wxString& Path() const { return m_path; }

private:
    wxString m_dir;
    wxString m_path;
};


void setEndings( SCH_SHAPE* aShape )
{
    aShape->SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    aShape->SetStartEndingLength( 508 );
    aShape->SetStartEndingWidth( 254 );
    aShape->SetStartEndingStrokeWidth( 76 );

    aShape->SetEndEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    aShape->SetEndEndingLength( 762 );
    aShape->SetEndEndingWidth( 508 );
    aShape->SetEndEndingStrokeWidth( 102 );
}


void checkEnding( const LINE_ENDING& aEnding, LINE_ENDING_STYLE aStyle, int aLength, int aWidth,
                  int aStrokeWidth )
{
    BOOST_CHECK_EQUAL( static_cast<int>( aEnding.GetStyle() ), static_cast<int>( aStyle ) );
    BOOST_CHECK_EQUAL( aEnding.GetLength(), aLength );
    BOOST_CHECK_EQUAL( aEnding.GetWidth(), aWidth );
    BOOST_CHECK_EQUAL( aEnding.GetStrokeWidth(), aStrokeWidth );
}


SCH_SHAPE* findSymbolShape( LIB_SYMBOL* aSymbol, SHAPE_T aShape )
{
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( &item );

        if( shape && shape->GetShape() == aShape )
            return shape;
    }

    return nullptr;
}

} // namespace


BOOST_AUTO_TEST_CASE( SymbolLibraryLineEndingsRoundTrip )
{
    TEMP_SYMBOL_LIBRARY tempLib;

    LIB_SYMBOL symbol( wxS( "LineEndingSymbol" ) );
    symbol.GetReferenceField().SetText( wxS( "U" ) );
    symbol.GetValueField().SetText( symbol.GetName() );

    auto* poly = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
    poly->AddPoint( VECTOR2I( 0, 0 ) );
    poly->AddPoint( VECTOR2I( 1000, 0 ) );
    poly->AddPoint( VECTOR2I( 1000, 500 ) );
    poly->SetStroke( STROKE_PARAMS( 50, LINE_STYLE::SOLID ) );
    setEndings( poly );
    symbol.AddDrawItem( poly, false );

    auto* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
    arc->SetArcGeometry( VECTOR2I( 0, 1000 ), VECTOR2I( 500, 1500 ), VECTOR2I( 1000, 1000 ) );
    arc->SetStroke( STROKE_PARAMS( 50, LINE_STYLE::SOLID ) );
    setEndings( arc );
    symbol.AddDrawItem( arc, false );

    auto* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
    bezier->SetStart( VECTOR2I( 0, 2000 ) );
    bezier->SetBezierC1( VECTOR2I( 300, 2500 ) );
    bezier->SetBezierC2( VECTOR2I( 700, 1500 ) );
    bezier->SetEnd( VECTOR2I( 1000, 2000 ) );
    bezier->SetStroke( STROKE_PARAMS( 50, LINE_STYLE::SOLID ) );
    setEndings( bezier );
    symbol.AddDrawItem( bezier, false );

    {
        SCH_IO_KICAD_SEXPR io;

        io.CreateLibrary( tempLib.Path() );
        io.SaveSymbol( tempLib.Path(), new LIB_SYMBOL( symbol ) );
        io.SaveLibrary( tempLib.Path() );
    }

    SCH_IO_KICAD_SEXPR reader;
    LIB_SYMBOL*        loaded = reader.LoadSymbol( tempLib.Path(), symbol.GetName() );

    BOOST_REQUIRE( loaded );

    for( SHAPE_T shapeType : { SHAPE_T::POLY, SHAPE_T::ARC, SHAPE_T::BEZIER } )
    {
        SCH_SHAPE* shape = findSymbolShape( loaded, shapeType );
        BOOST_REQUIRE_MESSAGE( shape, "Expected symbol shape was not reloaded" );

        checkEnding( shape->GetStartEnding(), LINE_ENDING_STYLE::ARROW, 508, 254, 76 );
        checkEnding( shape->GetEndEnding(), LINE_ENDING_STYLE::ARROW_OPEN, 762, 508, 102 );
    }
}


BOOST_AUTO_TEST_CASE( GraphicLineBoundingBoxIncludesLineEndings )
{
    SCH_LINE line( VECTOR2I( 0, 0 ), LAYER_NOTES );
    line.SetEndPoint( VECTOR2I( 1000, 0 ) );
    line.SetLineWidth( 20 );
    line.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    line.SetEndEndingLength( 400 );
    line.SetEndEndingWidth( 400 );

    BOX2I bbox = line.GetBoundingBox();

    BOOST_CHECK( bbox.GetRight() >= 1200 );
    BOOST_CHECK( bbox.GetTop() <= -200 );
    BOOST_CHECK( bbox.GetBottom() >= 200 );
}


BOOST_AUTO_TEST_CASE( WireBoundingBoxIgnoresLineEndingState )
{
    SCH_LINE wire( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire.SetEndPoint( VECTOR2I( 1000, 0 ) );
    wire.SetLineWidth( 20 );
    wire.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    wire.SetEndEndingLength( 400 );
    wire.SetEndEndingWidth( 400 );

    BOX2I bbox = wire.GetBoundingBox();

    BOOST_CHECK( bbox.GetRight() < 1200 );
}


BOOST_AUTO_TEST_CASE( GraphicLineHitTestIncludesLineEnding )
{
    SCH_LINE line( VECTOR2I( 0, 0 ), LAYER_NOTES );
    line.SetEndPoint( VECTOR2I( 1000, 0 ) );
    line.SetLineWidth( 20 );
    line.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    line.SetEndEndingLength( 400 );
    line.SetEndEndingWidth( 400 );

    BOOST_CHECK( line.HitTest( VECTOR2I( 1100, 150 ), 0 ) );

    BOX2I selection( VECTOR2I( 1100, 100 ), VECTOR2I( 50, 50 ) );
    BOOST_CHECK( line.HitTest( selection, false, 0 ) );
}


BOOST_AUTO_TEST_CASE( WireHitTestIgnoresLineEndingState )
{
    SCH_LINE wire( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire.SetEndPoint( VECTOR2I( 1000, 0 ) );
    wire.SetLineWidth( 20 );
    wire.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    wire.SetEndEndingLength( 400 );
    wire.SetEndEndingWidth( 400 );

    BOOST_CHECK( !wire.HitTest( VECTOR2I( 1100, 150 ), 0 ) );
}


BOOST_AUTO_TEST_CASE( ShortGraphicLineHitTestIncludesConsumedBodyEnding )
{
    SCH_LINE line( VECTOR2I( 0, 0 ), LAYER_NOTES );
    line.SetEndPoint( VECTOR2I( 100, 0 ) );
    line.SetLineWidth( 20 );
    line.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    line.SetEndEndingLength( 400 );
    line.SetEndEndingWidth( 400 );

    BOOST_CHECK( line.HitTest( VECTOR2I( -200, 100 ), 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
