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

#include <memory>

#include <eeschema_settings.h>
#include <font/font.h>
#include <font/font_metrics.h>
#include <google/protobuf/any.pb.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiface_base.h>
#include <lib_symbol.h>
#include <pin_layout_cache.h>
#include <plotters/plotters_pslike.h>
#include <api/schematic/schematic_types.pb.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_painter.h>
#include <sch_pin.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>

namespace
{
/// Long enough that a bracketed block fits on one line, so the multi-line fallback only fires
/// where a case deliberately shortens the pin.
constexpr int LONG_PIN = 5000000;

constexpr int TEXT_SIZE = 50000;


wxString format( const wxString& aNumber, PIN_NUMBER_DISPLAY aDisplay, int aPinLength = LONG_PIN )
{
    KIFONT::FONT* font = KIFONT::FONT::GetFont( wxEmptyString );

    return FormatStackedPinForDisplay( aNumber, aPinLength, TEXT_SIZE, font,
                                       KIFONT::METRICS::Default(), aDisplay );
}


SCH_PIN* addPin( LIB_SYMBOL& aSymbol, const wxString& aNumber )
{
    SCH_PIN* pin = new SCH_PIN( &aSymbol );
    pin->SetName( wxS( "GND" ) );
    pin->SetNumber( aNumber );
    aSymbol.AddDrawItem( pin );

    return pin;
}


/// A pin laid out well clear of the symbol body, so the indicator geometry is unconstrained.
SCH_PIN* addLaidOutPin( LIB_SYMBOL& aSymbol, const wxString& aNumber )
{
    aSymbol.SetPinNameOffset( 0 );

    SCH_PIN* pin = addPin( aSymbol, aNumber );
    pin->SetPosition( VECTOR2I( 0, 0 ) );
    pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
    pin->SetLength( schIUScale.MilsToIU( 200 ) );

    return pin;
}


/// SCH_PIN reads the symbol editor's pin defaults on construction.
struct PIN_NUMBER_DISPLAY_FIXTURE
{
    PIN_NUMBER_DISPLAY_FIXTURE() : m_previousSettings( Kiface().KifaceSettings() )
    {
        EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
        BOOST_REQUIRE( settings );
        Kiface().InitSettings( settings );
    }

    ~PIN_NUMBER_DISPLAY_FIXTURE()
    {
        Kiface().InitSettings( m_previousSettings );
    }

    SETTINGS_MANAGER   m_settingsManager;
    APP_SETTINGS_BASE* m_previousSettings;
};


/// Records the bars of every icon the painter draws, in sheet coordinates.
class PIN_BAR_GAL : public KIGFX::GAL
{
public:
    PIN_BAR_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aOptions ) : GAL( aOptions ) {}

    void Translate( const VECTOR2D& aTranslation ) override { m_translation = aTranslation; }

    void DrawRectangle( const VECTOR2D& aStart, const VECTOR2D& aEnd ) override
    {
        m_bars.push_back( BOX2I::ByCorners( KiROUND( aStart + m_translation ), KiROUND( aEnd + m_translation ) ) );
    }

    VECTOR2D           m_translation;
    std::vector<BOX2I> m_bars;
};


/// Records the bars of every icon the plotter draws, in sheet coordinates.
class PIN_BAR_PLOTTER : public SVG_PLOTTER
{
public:
    void Rect( const VECTOR2I& aStart, const VECTOR2I& aEnd, FILL_T aFill, int aWidth, int ) override
    {
        BOOST_CHECK( aFill == FILL_T::FILLED_SHAPE );
        BOOST_CHECK_EQUAL( aWidth, 0 );
        m_bars.push_back( BOX2I::ByCorners( aStart, aEnd ) );
    }

    void PlotText( const VECTOR2I& aPos, const KIGFX::COLOR4D&, const wxString& aText, const TEXT_ATTRIBUTES& aAttrs,
                   KIFONT::FONT*, const KIFONT::METRICS&, void* ) override
    {
        m_texts.push_back( { aText, aPos, aAttrs.m_Size.y } );
    }

    struct TEXT
    {
        wxString m_text;
        VECTOR2I m_pos;
        int      m_size;
    };

    /// The text of the given size, since a head and a tail may read the same
    const TEXT* find( const wxString& aText, int aSize ) const
    {
        for( const TEXT& text : m_texts )
        {
            if( text.m_text == aText && text.m_size == aSize )
                return &text;
        }

        return nullptr;
    }

    std::vector<BOX2I> m_bars;
    std::vector<TEXT>  m_texts;
};


/// The box around a set of bars, which is the icon's box.
BOX2I iconBox( const std::vector<BOX2I>& aBars )
{
    BOOST_REQUIRE_EQUAL( aBars.size(), 3u );

    BOX2I box = aBars.front();

    for( const BOX2I& bar : aBars )
        box.Merge( bar );

    return box;
}
} // namespace


BOOST_FIXTURE_TEST_SUITE( PinNumberDisplay, PIN_NUMBER_DISPLAY_FIXTURE )


BOOST_AUTO_TEST_CASE( ModesRenderTheirShortForm )
{
    // Ordering is by pad number, not by the order the author typed, and a range contributes every
    // pad it covers rather than the one item it is written as
    BOOST_CHECK_EQUAL( format( wxS( "[7,3,5]" ), PIN_NUMBER_DISPLAY::FIRST ), wxS( "3" ) );

    BOOST_CHECK_EQUAL( format( wxS( "[1,2,3]" ), PIN_NUMBER_DISPLAY::ALL ), wxS( "[1,2,3]" ) );

    // A block too wide for the pin breaks onto one line per pad
    BOOST_CHECK_EQUAL( format( wxS( "[1,2,3]" ), PIN_NUMBER_DISPLAY::ALL, 1 ), wxS( "[1\n2\n3]" ) );
}


BOOST_AUTO_TEST_CASE( CollapsedStackSplitsAroundTheIcon )
{
    // The count is the size of the whole stack, drawn as a subscript to the icon
    std::optional<STACKED_PIN_SUMMARY> summary =
            SummarizeStackedPinForDisplay( wxS( "[5-9]" ), PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_REQUIRE( summary.has_value() );
    BOOST_CHECK_EQUAL( summary->m_Head, wxS( "5" ) );
    BOOST_CHECK_EQUAL( summary->m_Tail, wxS( "5" ) );

    summary = SummarizeStackedPinForDisplay( wxS( "[7,3,5]" ), PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_REQUIRE( summary.has_value() );
    BOOST_CHECK_EQUAL( summary->m_Head, wxS( "3" ) );
    BOOST_CHECK_EQUAL( summary->m_Tail, wxS( "3" ) );

    summary = SummarizeStackedPinForDisplay( wxS( "[7,3,5]" ), PIN_NUMBER_DISPLAY::FIRST_AND_ICON );
    BOOST_REQUIRE( summary.has_value() );
    BOOST_CHECK_EQUAL( summary->m_Head, wxS( "3" ) );
    BOOST_CHECK( summary->m_Tail.IsEmpty() );

    summary = SummarizeStackedPinForDisplay( wxS( "[1,2,3]" ), PIN_NUMBER_DISPLAY::ICON_ONLY );
    BOOST_REQUIRE( summary.has_value() );
    BOOST_CHECK( summary->m_Head.IsEmpty() );
    BOOST_CHECK( summary->m_Tail.IsEmpty() );

    // The modes without an inline icon, a lone pad and notation that is not a stack are drawn as written
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "[1,2,3]" ), PIN_NUMBER_DISPLAY::ALL ) );
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "[1,2,3]" ), PIN_NUMBER_DISPLAY::FIRST ) );
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "[4]" ), PIN_NUMBER_DISPLAY::FIRST_AND_COUNT ) );
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "[4]" ), PIN_NUMBER_DISPLAY::FIRST_AND_ICON ) );
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "4" ), PIN_NUMBER_DISPLAY::ICON_ONLY ) );
    BOOST_CHECK( !SummarizeStackedPinForDisplay( wxS( "[1,2" ), PIN_NUMBER_DISPLAY::ICON_ONLY ) );
}


BOOST_AUTO_TEST_CASE( NetNamesPreserveAuthoringOrder )
{
    const std::vector<std::pair<wxString, wxString>> cases = {
        { "[7,3,5]", "7" }, { "[A12,A2]", "A12" }, { "[9-11,3]", "9" }, { "9", "9" }
    };

    for( const auto& [number, first] : cases )
    {
        for( PIN_NUMBER_DISPLAY display : { PIN_NUMBER_DISPLAY::ALL, PIN_NUMBER_DISPLAY::FIRST,
                                            PIN_NUMBER_DISPLAY::FIRST_AND_ICON, PIN_NUMBER_DISPLAY::FIRST_AND_COUNT,
                                            PIN_NUMBER_DISPLAY::ICON_ONLY } )
        {
            LIB_SYMBOL sym( wxS( "FPGA" ) );
            SCH_PIN* pin = addPin( sym, number );
            pin->SetName( wxEmptyString );
            pin->SetNumberDisplay( display );

            SCH_SHEET_PATH path;
            SCH_SYMBOL instance( sym, sym.GetLibId(), &path, 0, 0 );
            instance.SetRef( &path, "U1" );
            const auto pins = instance.GetPins( &path );
            BOOST_REQUIRE_EQUAL( pins.size(), 1u );
            BOOST_CHECK_EQUAL( pins.front()->GetDefaultNetName( path ), "Net-(U1-Pad" + first + ")" );
        }
    }
}


BOOST_AUTO_TEST_CASE( SchematicPinInheritsFromItsLibraryPin )
{
    SCH_SYMBOL schSymbol;
    schSymbol.SetLibSymbol( new LIB_SYMBOL( wxS( "FPGA" ) ) );

    SCH_PIN* libPin = addPin( *schSymbol.GetLibSymbolRef(), wxS( "[1,2,3]" ) );
    SCH_PIN  schPin( &schSymbol, libPin );

    libPin->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_CHECK( schPin.GetNumberDisplay() == PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );

    auto numberInfo = schPin.GetLayoutCache().GetPinNumberInfo( 0 );
    auto tailInfo = schPin.GetLayoutCache().GetPinNumberTailInfo( 0 );
    BOOST_REQUIRE( numberInfo.has_value() );
    BOOST_REQUIRE( tailInfo.has_value() );
    BOOST_CHECK_EQUAL( numberInfo->m_Text, wxString( "1" ) );
    BOOST_CHECK_EQUAL( tailInfo->m_Text, wxString( "3" ) );

    libPin->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST );
    BOOST_CHECK( schPin.GetNumberDisplay() == PIN_NUMBER_DISPLAY::FIRST );

    numberInfo = schPin.GetLayoutCache().GetPinNumberInfo( 0 );
    BOOST_REQUIRE( numberInfo.has_value() );
    BOOST_CHECK_EQUAL( numberInfo->m_Text, wxString( "1" ) );
    BOOST_CHECK( !schPin.GetLayoutCache().GetPinNumberTailInfo( 0 ).has_value() );

    // The instance reads the name icon setting from the library too
    BOOST_CHECK( !schPin.GetStackIconOnName() );
    libPin->SetStackIconOnName( true );
    BOOST_CHECK( schPin.GetStackIconOnName() );

    libPin->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_CHECK( schPin.GetNumberDisplay() == PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
}


BOOST_AUTO_TEST_CASE( InlineIconSitsBetweenTheHeadAndTheTail )
{
    LIB_SYMBOL sym( wxS( "FPGA" ) );
    SCH_PIN*   plain = addLaidOutPin( sym, wxS( "4" ) );
    SCH_PIN*   stacked = addLaidOutPin( sym, wxS( "[1,2,3]" ) );

    // A plain pin has no stack to advertise, and the full list and first-only look like they always did
    plain->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_CHECK( !plain->GetLayoutCache().GetStackIconBBox().has_value() );
    plain->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_ICON );
    BOOST_CHECK( !plain->GetLayoutCache().GetStackIconBBox().has_value() );

    for( PIN_NUMBER_DISPLAY display : { PIN_NUMBER_DISPLAY::ALL, PIN_NUMBER_DISPLAY::FIRST } )
    {
        stacked->SetNumberDisplay( display );
        BOOST_CHECK( !stacked->GetLayoutCache().GetStackIconBBox().has_value() );
    }

    stacked->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );

    PIN_LAYOUT_CACHE& cache = stacked->GetLayoutCache();
    OPT_BOX2I         icon = cache.GetStackIconBBox();
    OPT_BOX2I         numBox = cache.GetPinNumberBBox();
    auto              head = cache.GetPinNumberInfo( 0 );
    auto              tail = cache.GetPinNumberTailInfo( 0 );

    BOOST_REQUIRE( icon.has_value() );
    BOOST_REQUIRE( numBox.has_value() );
    BOOST_REQUIRE( head.has_value() );
    BOOST_REQUIRE( tail.has_value() );

    // Both texts start where the line says and the icon fits in the gap between them
    KIFONT::FONT* font = KIFONT::FONT::GetFont( wxEmptyString );
    const int     headWidth = font->StringBoundaryLimits( head->m_Text, VECTOR2D( head->m_TextSize, head->m_TextSize ),
                                                          head->m_Thickness, false, false,
                                                          KIFONT::METRICS::Default() ).x;

    BOOST_CHECK( head->m_HAlign == GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK( tail->m_HAlign == GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_GE( icon->GetLeft(), head->m_TextPosition.x + headWidth );
    BOOST_CHECK_LE( icon->GetRight(), tail->m_TextPosition.x );

    // The count is a subscript, smaller than the number and dropped below its centre line
    BOOST_CHECK_EQUAL( tail->m_TextSize, head->m_TextSize * 3 / 5 );
    BOOST_CHECK_EQUAL( tail->m_TextPosition.y - head->m_TextPosition.y, head->m_TextSize / 5 );
    BOOST_CHECK( numBox->Contains( BOX2I::ByCenter( tail->m_TextPosition, { 1, 1 } ) ) );

    // The head does not move across the line when the count is dropped
    const int headY = head->m_TextPosition.y;

    stacked->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_ICON );
    BOOST_CHECK( !cache.GetPinNumberTailInfo( 0 ).has_value() );
    BOOST_REQUIRE( cache.GetStackIconBBox().has_value() );
    BOOST_REQUIRE( cache.GetPinNumberInfo( 0 ).has_value() );
    BOOST_CHECK_EQUAL( cache.GetPinNumberInfo( 0 )->m_TextPosition.y, headY );
    BOOST_CHECK_GE( cache.GetStackIconBBox()->GetLeft(), cache.GetPinNumberInfo( 0 )->m_TextPosition.x );

    stacked->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );

    // The number's box spans the whole line, so selection and the bounding box cover the icon
    BOOST_CHECK( numBox->Contains( *icon ) );
    BOOST_CHECK_EQUAL( numBox->GetCenter().x, stacked->GetPosition().x + stacked->GetLength() / 2 );

    // Icon only has no text, but the icon keeps the room a number would take
    stacked->SetNumberDisplay( PIN_NUMBER_DISPLAY::ICON_ONLY );
    BOOST_CHECK( !cache.GetPinNumberInfo( 0 ).has_value() );
    BOOST_CHECK( !cache.GetPinNumberTailInfo( 0 ).has_value() );

    icon = cache.GetStackIconBBox();
    numBox = cache.GetPinNumberBBox();
    BOOST_REQUIRE( icon.has_value() );
    BOOST_REQUIRE( numBox.has_value() );
    BOOST_CHECK( icon->Intersects( *numBox ) );
    BOOST_CHECK_GE( icon->GetLeft(), numBox->GetLeft() );
    BOOST_CHECK_LE( icon->GetRight(), numBox->GetRight() );

    // The icon takes the room a number would, so the painter and the plotter step off the pin alike
    BOOST_CHECK_GE( numBox->GetHeight(), stacked->GetNumberTextSize() );

    // Zeroing the number text size is how a symbol hides one pin's number; the icon has to outlive
    // that or the stack disappears with nothing standing in for its pads
    stacked->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    stacked->SetNumberTextSize( 0 );
    BOOST_CHECK_GT( StackedPinIconSize( *stacked ), 0 );

    OPT_BOX2I hidden = stacked->GetLayoutCache().GetStackIconBBox();
    BOOST_REQUIRE( hidden.has_value() );
    BOOST_CHECK_GT( hidden->GetWidth(), 0 );
    BOOST_CHECK_GT( hidden->GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( IconOnNameFollowsTheSettingAndTheName )
{
    LIB_SYMBOL sym( wxS( "FPGA" ) );
    SCH_PIN*   plain = addLaidOutPin( sym, wxS( "4" ) );
    SCH_PIN*   stacked = addLaidOutPin( sym, wxS( "[1,2,3]" ) );
    plain->SetStackIconOnName( true );

    // Off by default, and a plain pin has no stack to advertise
    BOOST_CHECK( !stacked->GetLayoutCache().GetStackNameIconBBox().has_value() );
    BOOST_CHECK( !plain->GetLayoutCache().GetStackNameIconBBox().has_value() );

    stacked->SetStackIconOnName( true );

    // The icon is independent of the notation, and it sits clear of the name
    for( PIN_NUMBER_DISPLAY display : { PIN_NUMBER_DISPLAY::ALL, PIN_NUMBER_DISPLAY::FIRST,
                                        PIN_NUMBER_DISPLAY::FIRST_AND_ICON, PIN_NUMBER_DISPLAY::FIRST_AND_COUNT,
                                        PIN_NUMBER_DISPLAY::ICON_ONLY } )
    {
        stacked->SetNumberDisplay( display );

        OPT_BOX2I icon = stacked->GetLayoutCache().GetStackNameIconBBox();
        OPT_BOX2I nameBox = stacked->GetLayoutCache().GetPinNameBBox();
        BOOST_REQUIRE( icon.has_value() );
        BOOST_REQUIRE( nameBox.has_value() );
        BOOST_CHECK( !icon->Intersects( *nameBox ) );
    }

    // The icon marks the name, so it goes when the name does
    stacked->SetNameTextSize( 0 );
    BOOST_CHECK( !stacked->GetLayoutCache().GetStackNameIconBBox().has_value() );
}


BOOST_AUTO_TEST_CASE( AbbreviatedNumberDoesNotInflateTheBoundingBox )
{
    LIB_SYMBOL sym( wxS( "FPGA" ) );
    SCH_PIN*   pin = addLaidOutPin( sym, wxS( "[1000000,2000000,3000000]" ) );
    SCH_PIN*   small = addLaidOutPin( sym, wxS( "[1,2,3]" ) );

    const BOX2I full = pin->GetLayoutCache().GetPinBoundingBox( false, true, false );

    pin->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST );

    // The undrawn pads must stop reserving room
    BOOST_CHECK_LT( pin->GetLayoutCache().GetPinBoundingBox( false, true, false ).GetWidth(), full.GetWidth() );

    // With only the icon drawn, what the stack names cannot matter at all
    pin->SetNumberDisplay( PIN_NUMBER_DISPLAY::ICON_ONLY );
    small->SetNumberDisplay( PIN_NUMBER_DISPLAY::ICON_ONLY );
    BOOST_CHECK_EQUAL( pin->GetLayoutCache().GetPinBoundingBox( false, true, false ).GetWidth(),
                       small->GetLayoutCache().GetPinBoundingBox( false, true, false ).GetWidth() );
}


/// The painter and the plotter place the icons and the text around them separately, so they have to agree.
BOOST_AUTO_TEST_CASE( PainterAndPlotterDrawTheSameIcons )
{
    struct CASE
    {
        const char*        m_name;
        PIN_NUMBER_DISPLAY m_display;
        bool               m_iconOnName;
        int                m_nameOffsetMils;
        int                m_lengthMils = 200;
        const wchar_t*     m_number = L"[1,2,3]";
    };

    const CASE cases[] = {
        { "inline", PIN_NUMBER_DISPLAY::FIRST_AND_COUNT, false, 0 },
        { "first and icon", PIN_NUMBER_DISPLAY::FIRST_AND_ICON, false, 0 },
        { "narrow pin", PIN_NUMBER_DISPLAY::FIRST_AND_COUNT, false, 40, 100, L"[18-27]" },
        { "inline, name inside", PIN_NUMBER_DISPLAY::FIRST_AND_COUNT, false, 40 },
        { "icon only", PIN_NUMBER_DISPLAY::ICON_ONLY, false, 0 },
        { "name icon", PIN_NUMBER_DISPLAY::FIRST, true, 0 },
        { "name icon, name inside", PIN_NUMBER_DISPLAY::FIRST, true, 40 },
    };

    for( const CASE& testCase : cases )
    {
        LIB_SYMBOL sym( wxS( "FPGA" ) );
        SCH_PIN*   pin = addLaidOutPin( sym, testCase.m_number );
        pin->SetLength( schIUScale.MilsToIU( testCase.m_lengthMils ) );
        sym.SetPinNameOffset( schIUScale.MilsToIU( testCase.m_nameOffsetMils ) );

        // Only one icon at a time, so the bars on screen are known to belong to it
        pin->SetNumberDisplay( testCase.m_display );
        pin->SetStackIconOnName( testCase.m_iconOnName );

        KIGFX::GAL_DISPLAY_OPTIONS options;
        PIN_BAR_GAL                gal( options );
        KIGFX::SCH_PAINTER         painter( &gal );
        PIN_BAR_PLOTTER            plotter;

        plotter.SetRenderSettings( painter.GetSettings() );

        for( PIN_ORIENTATION orientation : { PIN_ORIENTATION::PIN_RIGHT, PIN_ORIENTATION::PIN_LEFT,
                                             PIN_ORIENTATION::PIN_UP, PIN_ORIENTATION::PIN_DOWN } )
        {
            pin->SetOrientation( orientation );
            gal.m_bars.clear();
            painter.Draw( pin, LAYER_PIN );

            plotter.m_bars.clear();
            plotter.m_texts.clear();
            pin->PlotPinTexts( &plotter, pin->GetPosition(), orientation, sym.GetPinNameOffset(), true, true, false );

            BOOST_TEST_CONTEXT( testCase.m_name << " orientation " << static_cast<int>( orientation ) )
            {
                // The pin is horizontal for these two, so along it is X
                const bool horizontal = ( orientation == PIN_ORIENTATION::PIN_RIGHT
                                          || orientation == PIN_ORIENTATION::PIN_LEFT );

                // Distance of a plotted point from the painted one, along the pin and across it
                auto compare = [&]( const VECTOR2I& aDrawn, const VECTOR2I& aPlotted )
                {
                    const int along = horizontal ? aDrawn.x - aPlotted.x : aDrawn.y - aPlotted.y;
                    const int across = horizontal ? aDrawn.y - aPlotted.y : aDrawn.x - aPlotted.x;

                    // Integer rounding of the two placements may differ by a unit
                    BOOST_CHECK_LE( std::abs( along ), 1 );
                    BOOST_CHECK_LE( std::abs( across ), pin->GetNumberTextSize() / 8 );
                };

                const BOX2I drawn = iconBox( gal.m_bars );
                const BOX2I plotted = iconBox( plotter.m_bars );

                BOOST_CHECK_EQUAL( drawn.GetSize(), plotted.GetSize() );
                compare( drawn.GetCenter(), plotted.GetCenter() );

                if( testCase.m_display == PIN_NUMBER_DISPLAY::FIRST_AND_COUNT
                    || testCase.m_display == PIN_NUMBER_DISPLAY::FIRST_AND_ICON )
                {
                    auto head = pin->GetLayoutCache().GetPinNumberInfo( 0 );
                    auto tail = pin->GetLayoutCache().GetPinNumberTailInfo( 0 );
                    BOOST_REQUIRE( head.has_value() );

                    const auto* plottedHead = plotter.find( head->m_Text, head->m_TextSize );
                    BOOST_REQUIRE( plottedHead );
                    compare( head->m_TextPosition, plottedHead->m_pos );

                    BOOST_CHECK_EQUAL( tail.has_value(), testCase.m_display == PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );

                    if( tail )
                    {
                        const auto* plottedTail = plotter.find( tail->m_Text, tail->m_TextSize );
                        BOOST_REQUIRE( plottedTail );
                        compare( tail->m_TextPosition, plottedTail->m_pos );

                        // Both renderers drop the subscript by the same amount, down the text.
                        // Vertical text reads bottom to top, so down is towards +X.
                        const int drawnDrop = horizontal ? tail->m_TextPosition.y - head->m_TextPosition.y
                                                         : tail->m_TextPosition.x - head->m_TextPosition.x;
                        const int plottedDrop = horizontal ? plottedTail->m_pos.y - plottedHead->m_pos.y
                                                           : plottedTail->m_pos.x - plottedHead->m_pos.x;

                        BOOST_CHECK_EQUAL( drawnDrop, pin->GetNumberTextSize() / 5 );
                        BOOST_CHECK_LE( std::abs( plottedDrop - drawnDrop ), 1 );
                        BOOST_CHECK_EQUAL( plottedTail->m_size, pin->GetNumberTextSize() * 3 / 5 );
                    }
                }
            }
        }
    }
}


/// A line wider than its pin overhangs the free end, never the symbol body
BOOST_AUTO_TEST_CASE( WideLineOverhangsTheFreeEndOfTheirPin )
{
    for( PIN_ORIENTATION orientation : { PIN_ORIENTATION::PIN_RIGHT, PIN_ORIENTATION::PIN_LEFT,
                                         PIN_ORIENTATION::PIN_UP, PIN_ORIENTATION::PIN_DOWN } )
    {
        LIB_SYMBOL sym( wxS( "FPGA" ) );
        SCH_PIN*   pin = addLaidOutPin( sym, wxS( "[18-27]" ) );
        pin->SetLength( schIUScale.MilsToIU( 100 ) );
        pin->SetOrientation( orientation );
        pin->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );

        OPT_BOX2I numBox = pin->GetLayoutCache().GetPinNumberBBox();
        OPT_BOX2I icon = pin->GetLayoutCache().GetStackIconBBox();
        BOOST_REQUIRE( numBox.has_value() );
        BOOST_REQUIRE( icon.has_value() );

        const int length = pin->GetLength();

        BOOST_TEST_CONTEXT( "orientation " << static_cast<int>( orientation ) )
        {
            // The line is wider than the pin, or the case proves nothing
            const bool horizontal = orientation == PIN_ORIENTATION::PIN_RIGHT
                                    || orientation == PIN_ORIENTATION::PIN_LEFT;
            BOOST_REQUIRE_GT( horizontal ? numBox->GetWidth() : numBox->GetHeight(), length );
            BOOST_CHECK( numBox->Contains( *icon ) );

            switch( orientation )
            {
            case PIN_ORIENTATION::PIN_RIGHT:
                BOOST_CHECK_LE( numBox->GetRight(), length + 1 );
                BOOST_CHECK_LT( numBox->GetLeft(), 0 );
                break;
            case PIN_ORIENTATION::PIN_LEFT:
                BOOST_CHECK_GE( numBox->GetLeft(), -length - 1 );
                BOOST_CHECK_GT( numBox->GetRight(), 0 );
                break;
            case PIN_ORIENTATION::PIN_UP:
                BOOST_CHECK_GE( numBox->GetTop(), -length - 1 );
                BOOST_CHECK_GT( numBox->GetBottom(), 0 );
                break;
            default:
                BOOST_CHECK_LE( numBox->GetBottom(), length + 1 );
                BOOST_CHECK_LT( numBox->GetTop(), 0 );
                break;
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( IconBarsFollowTheReadingDirection )
{
    // Three bars along the reading direction of the text they sit in, whichever way that points
    for( bool vertical : { false, true } )
    {
        std::vector<BOX2I> bars;
        BuildStackedPinIconBars( 1000, vertical, [&]( const BOX2I& aBar ) { bars.push_back( aBar ); } );

        BOOST_REQUIRE_EQUAL( bars.size(), 3u );

        for( const BOX2I& bar : bars )
        {
            BOOST_CHECK_EQUAL( vertical ? bar.GetHeight() : bar.GetWidth(), StackedPinIconExtents( 1000 ).x );
            BOOST_CHECK_LT( vertical ? bar.GetWidth() : bar.GetHeight(), StackedPinIconExtents( 1000 ).x );
        }

        const BOX2I all = iconBox( bars );
        BOOST_CHECK_EQUAL( vertical ? all.GetWidth() : all.GetHeight(), StackedPinIconExtents( 1000 ).y );
    }
}


BOOST_AUTO_TEST_CASE( DisplayModeSurvivesTheFileFormat )
{
    LIB_SYMBOL sym( wxS( "FPGA" ) );
    addPin( sym, wxS( "[1,2,3]" ) )->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    addPin( sym, wxS( "[6,7]" ) )->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST );
    addPin( sym, wxS( "[9,10]" ) )->SetNumberDisplay( PIN_NUMBER_DISPLAY::ICON_ONLY );
    addPin( sym, wxS( "[13,14]" ) )->SetNumberDisplay( PIN_NUMBER_DISPLAY::FIRST_AND_ICON );
    addPin( sym, wxS( "8" ) );

    SCH_PIN* named = addPin( sym, wxS( "[11,12]" ) );
    named->SetStackIconOnName( true );

    STRING_FORMATTER formatter;
    SCH_IO_KICAD_SEXPR::FormatLibSymbol( &sym, formatter );

    // Every symbol in the wild is at the default; emitting the tokens unconditionally would rewrite
    // whole libraries on the next save
    std::string text = formatter.GetString();
    BOOST_CHECK( text.find( "number_display all" ) == std::string::npos );
    BOOST_CHECK( text.find( "stack_icon_on_name no" ) == std::string::npos );

    std::vector<LIB_SYMBOL*> loaded = SCH_IO_KICAD_SEXPR::ParseLibSymbols( text, "pin display test" );
    BOOST_REQUIRE_EQUAL( loaded.size(), 1u );

    std::unique_ptr<LIB_SYMBOL>            reloaded( loaded.front() );
    std::map<wxString, PIN_NUMBER_DISPLAY> byNumber;
    std::map<wxString, bool>               iconOnName;

    for( SCH_PIN* pin : reloaded->GetGraphicalPins() )
    {
        byNumber[pin->GetNumber()] = pin->GetNumberDisplay();
        iconOnName[pin->GetNumber()] = pin->GetStackIconOnName();
    }

    BOOST_CHECK( byNumber[wxS( "[1,2,3]" )] == PIN_NUMBER_DISPLAY::FIRST_AND_COUNT );
    BOOST_CHECK( byNumber[wxS( "[6,7]" )] == PIN_NUMBER_DISPLAY::FIRST );
    BOOST_CHECK( byNumber[wxS( "[9,10]" )] == PIN_NUMBER_DISPLAY::ICON_ONLY );
    BOOST_CHECK( byNumber[wxS( "[13,14]" )] == PIN_NUMBER_DISPLAY::FIRST_AND_ICON );
    BOOST_CHECK( byNumber[wxS( "8" )] == PIN_NUMBER_DISPLAY::ALL );

    BOOST_CHECK( iconOnName[wxS( "[11,12]" )] );
    BOOST_CHECK( !iconOnName[wxS( "[1,2,3]" )] );
}


BOOST_AUTO_TEST_CASE( ApiRoundTripKeepsTheDisplayMode )
{
    LIB_SYMBOL sym( wxS( "FPGA" ) );

    for( PIN_NUMBER_DISPLAY display : { PIN_NUMBER_DISPLAY::ALL, PIN_NUMBER_DISPLAY::FIRST,
                                        PIN_NUMBER_DISPLAY::FIRST_AND_ICON, PIN_NUMBER_DISPLAY::FIRST_AND_COUNT,
                                        PIN_NUMBER_DISPLAY::ICON_ONLY } )
    {
        for( bool iconOnName : { false, true } )
        {
            SCH_PIN* pin = addPin( sym, wxS( "[1,2,3]" ) );
            pin->SetNumberDisplay( display );
            pin->SetStackIconOnName( iconOnName );

            google::protobuf::Any packed;
            pin->Serialize( packed );

            SCH_PIN restored( &sym );
            BOOST_REQUIRE( restored.Deserialize( packed ) );

            BOOST_CHECK( restored.GetNumberDisplay() == display );
            BOOST_CHECK_EQUAL( restored.GetStackIconOnName(), iconOnName );
        }
    }

    // A payload from before the modes existed carries neither field
    SCH_PIN* legacy = addPin( sym, wxS( "[1,2,3]" ) );
    google::protobuf::Any packed;
    legacy->Serialize( packed );

    kiapi::schematic::types::SchematicPin message;
    BOOST_REQUIRE( packed.UnpackTo( &message ) );
    message.clear_number_display();
    message.clear_stack_icon_on_name();
    packed.PackFrom( message );

    SCH_PIN restored( &sym );
    BOOST_REQUIRE( restored.Deserialize( packed ) );
    BOOST_CHECK( restored.GetNumberDisplay() == PIN_NUMBER_DISPLAY::ALL );
    BOOST_CHECK( !restored.GetStackIconOnName() );
}


BOOST_AUTO_TEST_SUITE_END()
