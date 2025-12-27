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

#include <sch_label.h>
#include <sch_text.h>
#include <font/font.h>
#include <font/outline_font.h>
#include <font/stroke_font.h>
#include <settings/settings_manager.h>


/**
 * Test fixture for label bounding box tests.
 */
struct LABEL_BBOX_FIXTURE
{
    LABEL_BBOX_FIXTURE()
    {
    }

    KIFONT::FONT* GetFirstExistingOutlineFont()
    {
        const std::vector<wxString> fontNames = {
            wxS( "Arial" ),
            wxS( "Segoe UI" ),
            wxS( "Tahoma" ),
            wxS( "Verdana" ),
            wxS( "DejaVu Sans" ),
            wxS( "Liberation Sans" ),
            wxS( "FreeSans" ),
            wxS( "Ubuntu" ),
            wxS( "Helvetica" ),
            wxS( "Helvetica Neue" )
        };

        for( const wxString& name : fontNames )
        {
            KIFONT::FONT* font = KIFONT::FONT::GetFont( name, false, false );

            if( font && font->IsOutline() )
                return font;
        }

        return nullptr;
    }

    /**
     * Helper function to check that a text bounding box is reasonable.
     * The bounding box should contain the text without excessive empty space.
     * For a left-justified text, the width should be approximately equal to the text width,
     * not double it.
     */
    void CheckTextBoundingBoxWidth( const EDA_TEXT& aText, const wxString& aTestName,
                                     double aMaxWidthMultiplier = 1.3 )
    {
        KIFONT::FONT* font = aText.GetDrawFont( nullptr );
        BOOST_REQUIRE_MESSAGE( font, aTestName + ": Font should not be null" );

        VECTOR2I textSize = aText.GetTextSize();
        int      thickness = aText.GetEffectiveTextPenWidth();
        wxString text = aText.GetShownText( true );

        // Get the text width from the font using empty metrics (defaults)
        KIFONT::METRICS metrics;
        VECTOR2I extents = font->StringBoundaryLimits( text, textSize, thickness,
                                                        aText.IsBold(), aText.IsItalic(),
                                                        metrics );

        // Get the actual bounding box
        BOX2I bbox = aText.GetTextBox( nullptr );

        // The bounding box width should be close to the text extents width
        // Allow some margin for font metrics, pen width, etc.
        int expectedMaxWidth = extents.x * aMaxWidthMultiplier;

        BOOST_TEST_MESSAGE( aTestName + ": Font: " + font->GetName() );
        BOOST_TEST_MESSAGE( "  Text: '" + text + "'" );
        BOOST_TEST_MESSAGE( "  Text extents width: " + std::to_string( extents.x ) );
        BOOST_TEST_MESSAGE( "  BBox width: " + std::to_string( bbox.GetWidth() ) );
        BOOST_TEST_MESSAGE( "  Expected max width: " + std::to_string( expectedMaxWidth ) );

        // Check that bbox is not excessively wide (like double the text width)
        BOOST_CHECK_MESSAGE( bbox.GetWidth() <= expectedMaxWidth,
                             aTestName + ": Bounding box width (" + std::to_string( bbox.GetWidth() )
                             + ") exceeds expected max (" + std::to_string( expectedMaxWidth )
                             + "). Text may have been measured twice." );

        // Also check that the bbox is not too small (sanity check)
        int expectedMinWidth = extents.x * 0.7;
        BOOST_CHECK_MESSAGE( bbox.GetWidth() >= expectedMinWidth,
                             aTestName + ": Bounding box width (" + std::to_string( bbox.GetWidth() )
                             + ") is smaller than expected min (" + std::to_string( expectedMinWidth ) + ")" );
    }

    /**
     * Helper function to check label body bounding box.
     */
    void CheckLabelBodyBoundingBox( SCH_LABEL_BASE& aLabel, const wxString& aTestName )
    {
        BOX2I bodyBBox = aLabel.GetBodyBoundingBox( nullptr );
        BOX2I textBBox = aLabel.GetTextBox( nullptr );

        BOOST_TEST_MESSAGE( aTestName + ": Label body bbox width: " + std::to_string( bodyBBox.GetWidth() ) );
        BOOST_TEST_MESSAGE( "  Label text bbox width: " + std::to_string( textBBox.GetWidth() ) );

        // Body bbox should be at least as wide as text bbox (may include margins)
        BOOST_CHECK_MESSAGE( bodyBBox.GetWidth() >= textBBox.GetWidth(),
                             aTestName + ": Body bbox width (" + std::to_string( bodyBBox.GetWidth() )
                             + ") should be >= text bbox width (" + std::to_string( textBBox.GetWidth() ) + ")" );

        // Body bbox should not be excessively larger than text bbox
        // Allow up to 3x for labels with shapes (global labels, etc.)
        int maxBodyWidth = textBBox.GetWidth() * 3;
        BOOST_CHECK_MESSAGE( bodyBBox.GetWidth() <= maxBodyWidth,
                             aTestName + ": Body bbox width (" + std::to_string( bodyBBox.GetWidth() )
                             + ") is excessively large compared to text bbox width ("
                             + std::to_string( textBBox.GetWidth() ) + ")" );
    }
};


BOOST_FIXTURE_TEST_SUITE( LabelBoundingBox, LABEL_BBOX_FIXTURE )


/**
 * Test that SCH_TEXT bounding boxes are correct with the default KiCad font (stroke font).
 */
BOOST_AUTO_TEST_CASE( TextBoundingBoxStrokeFont )
{
    SCH_TEXT text( VECTOR2I( 0, 0 ), wxS( "TEST_TEXT" ) );
    text.SetTextSize( VECTOR2I( 1000, 1000 ) );

    // Use default KiCad font (stroke font)
    text.SetFont( nullptr );

    CheckTextBoundingBoxWidth( text, "SCH_TEXT with stroke font" );
}


/**
 * Test that SCH_TEXT bounding boxes are correct with an outline font.
 */
BOOST_AUTO_TEST_CASE( TextBoundingBoxOutlineFont )
{
    SCH_TEXT text( VECTOR2I( 0, 0 ), wxS( "TEST_TEXT" ) );
    text.SetTextSize( VECTOR2I( 1000, 1000 ) );

    // Try to load an outline font - use a common system font
    // If not available, the test will skip
    KIFONT::FONT* font = GetFirstExistingOutlineFont();

    if( !font )
    {
        BOOST_TEST_MESSAGE( "Outline font not available, skipping test" );
        return;
    }

    text.SetFont( font );

    CheckTextBoundingBoxWidth( text, "SCH_TEXT with outline font (" + font->GetName() + ")" );
}


/**
 * Test various text strings to ensure bounding boxes are correct.
 */
BOOST_AUTO_TEST_CASE( TextBoundingBoxVariousStrings )
{
    std::vector<wxString> testStrings = {
        wxS( "A" ),
        wxS( "ABC" ),
        wxS( "TEST" ),
        wxS( "Hello World" ),
        wxS( "1234567890" ),
        wxS( "WWWWW" ),  // Wide characters
        wxS( "iiiii" ),  // Narrow characters
    };

    // Test with stroke font
    for( const wxString& str : testStrings )
    {
        SCH_TEXT text( VECTOR2I( 0, 0 ), str );
        text.SetTextSize( VECTOR2I( 1000, 1000 ) );
        text.SetFont( nullptr ); // KiCad stroke font

        CheckTextBoundingBoxWidth( text, "Stroke font: '" + str + "'" );
    }

    // Test with outline font if available
    KIFONT::FONT* font = GetFirstExistingOutlineFont();

    if( font )
    {
        for( const wxString& str : testStrings )
        {
            SCH_TEXT text( VECTOR2I( 0, 0 ), str );
            text.SetTextSize( VECTOR2I( 1000, 1000 ) );
            text.SetFont( font );

            CheckTextBoundingBoxWidth( text, "Outline font: '" + str + "'" );
        }
    }
}


/**
 * Test that SCH_LABEL body bounding boxes are correct with stroke font.
 */
BOOST_AUTO_TEST_CASE( LabelBodyBoundingBoxStrokeFont )
{
    SCH_LABEL label( VECTOR2I( 0, 0 ), wxS( "TEST_LABEL" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( nullptr ); // KiCad stroke font

    CheckTextBoundingBoxWidth( label, "SCH_LABEL text with stroke font" );
    CheckLabelBodyBoundingBox( label, "SCH_LABEL body with stroke font" );
}


/**
 * Test that SCH_LABEL body bounding boxes are correct with outline font.
 */
BOOST_AUTO_TEST_CASE( LabelBodyBoundingBoxOutlineFont )
{
    KIFONT::FONT* font = GetFirstExistingOutlineFont();

    if( !font )
    {
        BOOST_TEST_MESSAGE( "Outline font not available, skipping test" );
        return;
    }

    SCH_LABEL label( VECTOR2I( 0, 0 ), wxS( "TEST_LABEL" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( font );

    CheckTextBoundingBoxWidth( label, "SCH_LABEL text with outline font" );
    CheckLabelBodyBoundingBox( label, "SCH_LABEL body with outline font" );
}


/**
 * Test that SCH_GLOBALLABEL body bounding boxes are correct with stroke font.
 */
BOOST_AUTO_TEST_CASE( GlobalLabelBodyBoundingBoxStrokeFont )
{
    SCH_GLOBALLABEL label( VECTOR2I( 0, 0 ), wxS( "GLOBAL_TEST" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( nullptr ); // KiCad stroke font

    CheckTextBoundingBoxWidth( label, "SCH_GLOBALLABEL text with stroke font" );
    CheckLabelBodyBoundingBox( label, "SCH_GLOBALLABEL body with stroke font" );
}


/**
 * Test that SCH_GLOBALLABEL body bounding boxes are correct with outline font.
 * This is the most visible case of the bug where the bounding box was double-width.
 */
BOOST_AUTO_TEST_CASE( GlobalLabelBodyBoundingBoxOutlineFont )
{
    KIFONT::FONT* font = GetFirstExistingOutlineFont();

    if( !font )
    {
        BOOST_TEST_MESSAGE( "Outline font not available, skipping test" );
        return;
    }

    SCH_GLOBALLABEL label( VECTOR2I( 0, 0 ), wxS( "GLOBAL_TEST" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( font );

    CheckTextBoundingBoxWidth( label, "SCH_GLOBALLABEL text with outline font" );
    CheckLabelBodyBoundingBox( label, "SCH_GLOBALLABEL body with outline font" );
}


/**
 * Test that SCH_HIERLABEL body bounding boxes are correct with stroke font.
 */
BOOST_AUTO_TEST_CASE( HierLabelBodyBoundingBoxStrokeFont )
{
    SCH_HIERLABEL label( VECTOR2I( 0, 0 ), wxS( "HIER_TEST" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( nullptr ); // KiCad stroke font

    CheckTextBoundingBoxWidth( label, "SCH_HIERLABEL text with stroke font" );
    CheckLabelBodyBoundingBox( label, "SCH_HIERLABEL body with stroke font" );
}


/**
 * Test that SCH_HIERLABEL body bounding boxes are correct with outline font.
 */
BOOST_AUTO_TEST_CASE( HierLabelBodyBoundingBoxOutlineFont )
{
    KIFONT::FONT* font = GetFirstExistingOutlineFont();

    if( !font )
    {
        BOOST_TEST_MESSAGE( "Outline font not available, skipping test" );
        return;
    }

    SCH_HIERLABEL label( VECTOR2I( 0, 0 ), wxS( "HIER_TEST" ) );
    label.SetTextSize( VECTOR2I( 1000, 1000 ) );
    label.SetFont( font );

    CheckTextBoundingBoxWidth( label, "SCH_HIERLABEL text with outline font" );
    CheckLabelBodyBoundingBox( label, "SCH_HIERLABEL body with outline font" );
}


BOOST_AUTO_TEST_SUITE_END()
