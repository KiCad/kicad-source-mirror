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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Schematic writer for the KiCad 10.0 file format, extracted from the 10.0.0 release. It can
// only emit what that release knew, so its output always opens there.

#include <sch_io/kicad_sexpr/writers/sch_writer_v10.h>

#include <algorithm>
#include <fmt/format.h>
#include <wx/mstream.h>
#include <base_units.h>
#include <bitmap_base.h>
#include <build_version.h>
#include <schematic_lexer.h>

using namespace TSCHEMATIC_T;
#include <io/kicad/kicad_io_utils.h>
#include <io/kicad/legacy_format.h>
#include <schematic.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h> // SYMBOL_ORIENTATION_T
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <string_utils.h>
#include <locale_io.h>
#include <richio.h>


// Era copies of the shared fill and shape helpers. The current ones emit newer tokens.
static const char* emptyString = "";

static void formatArc( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aArc, bool aIsPrivate, const STROKE_PARAMS& aStroke,
                       FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY, const KIID& aUuid = niluuid );
static void formatCircle( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aCircle, bool aIsPrivate,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                          const KIID& aUuid = niluuid );
static void formatRect( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aRect, bool aIsPrivate, const STROKE_PARAMS& aStroke,
                        FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY, const KIID& aUuid = niluuid );
static void formatBezier( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aBezier, bool aIsPrivate,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                          const KIID& aUuid = niluuid );
static void formatPoly( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aPolyLine, bool aIsPrivate,
                        const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                        const KIID& aUuid = niluuid );

static void formatFill( OUTPUTFORMATTER* aFormatter, FILL_T aFillMode, const COLOR4D& aFillColor )
{
    const char* fillType;

    switch( aFillMode )
    {
    default:
    case FILL_T::NO_FILL: fillType = "none"; break;
    case FILL_T::FILLED_SHAPE: fillType = "outline"; break;
    case FILL_T::FILLED_WITH_BG_BODYCOLOR: fillType = "background"; break;
    case FILL_T::FILLED_WITH_COLOR: fillType = "color"; break;
    case FILL_T::HATCH: fillType = "hatch"; break;
    case FILL_T::REVERSE_HATCH: fillType = "reverse_hatch"; break;
    case FILL_T::CROSS_HATCH: fillType = "cross_hatch"; break;
    }

    if( aFillMode == FILL_T::FILLED_WITH_COLOR || aFillMode == FILL_T::HATCH || aFillMode == FILL_T::REVERSE_HATCH
        || aFillMode == FILL_T::CROSS_HATCH )
    {
        aFormatter->Print( "(fill (type %s) (color %d %d %d %s))", fillType, KiROUND( aFillColor.r * 255.0 ),
                           KiROUND( aFillColor.g * 255.0 ), KiROUND( aFillColor.b * 255.0 ),
                           FormatDouble2Str( aFillColor.a ).c_str() );
    }
    else
    {
        aFormatter->Print( "(fill (type %s))", fillType );
    }
}


static const char* getPinElectricalTypeToken( ELECTRICAL_PINTYPE aType )
{
    switch( aType )
    {
    case ELECTRICAL_PINTYPE::PT_INPUT: return SCHEMATIC_LEXER::TokenName( T_input );

    case ELECTRICAL_PINTYPE::PT_OUTPUT: return SCHEMATIC_LEXER::TokenName( T_output );

    case ELECTRICAL_PINTYPE::PT_BIDI: return SCHEMATIC_LEXER::TokenName( T_bidirectional );

    case ELECTRICAL_PINTYPE::PT_TRISTATE: return SCHEMATIC_LEXER::TokenName( T_tri_state );

    case ELECTRICAL_PINTYPE::PT_PASSIVE: return SCHEMATIC_LEXER::TokenName( T_passive );

    case ELECTRICAL_PINTYPE::PT_NIC: return SCHEMATIC_LEXER::TokenName( T_free );

    case ELECTRICAL_PINTYPE::PT_UNSPECIFIED: return SCHEMATIC_LEXER::TokenName( T_unspecified );

    case ELECTRICAL_PINTYPE::PT_POWER_IN: return SCHEMATIC_LEXER::TokenName( T_power_in );

    case ELECTRICAL_PINTYPE::PT_POWER_OUT: return SCHEMATIC_LEXER::TokenName( T_power_out );

    case ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR: return SCHEMATIC_LEXER::TokenName( T_open_collector );

    case ELECTRICAL_PINTYPE::PT_OPENEMITTER: return SCHEMATIC_LEXER::TokenName( T_open_emitter );

    case ELECTRICAL_PINTYPE::PT_NC: return SCHEMATIC_LEXER::TokenName( T_no_connect );

    default: wxFAIL_MSG( "Missing symbol library pin connection type" );
    }

    return emptyString;
}


static const char* getPinShapeToken( GRAPHIC_PINSHAPE aShape )
{
    switch( aShape )
    {
    case GRAPHIC_PINSHAPE::LINE: return SCHEMATIC_LEXER::TokenName( T_line );

    case GRAPHIC_PINSHAPE::INVERTED: return SCHEMATIC_LEXER::TokenName( T_inverted );

    case GRAPHIC_PINSHAPE::CLOCK: return SCHEMATIC_LEXER::TokenName( T_clock );

    case GRAPHIC_PINSHAPE::INVERTED_CLOCK: return SCHEMATIC_LEXER::TokenName( T_inverted_clock );

    case GRAPHIC_PINSHAPE::INPUT_LOW: return SCHEMATIC_LEXER::TokenName( T_input_low );

    case GRAPHIC_PINSHAPE::CLOCK_LOW: return SCHEMATIC_LEXER::TokenName( T_clock_low );

    case GRAPHIC_PINSHAPE::OUTPUT_LOW: return SCHEMATIC_LEXER::TokenName( T_output_low );

    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK: return SCHEMATIC_LEXER::TokenName( T_edge_clock_high );

    case GRAPHIC_PINSHAPE::NONLOGIC: return SCHEMATIC_LEXER::TokenName( T_non_logic );

    default: wxFAIL_MSG( "Missing symbol library pin shape type" );
    }

    return emptyString;
}


static EDA_ANGLE getPinAngle( PIN_ORIENTATION aOrientation )
{
    switch( aOrientation )
    {
    case PIN_ORIENTATION::PIN_RIGHT: return ANGLE_0;
    case PIN_ORIENTATION::PIN_LEFT: return ANGLE_180;
    case PIN_ORIENTATION::PIN_UP: return ANGLE_90;
    case PIN_ORIENTATION::PIN_DOWN: return ANGLE_270;
    default: wxFAIL_MSG( "Missing symbol library pin orientation type" ); return ANGLE_0;
    }
}


static const char* getSheetPinShapeToken( LABEL_FLAG_SHAPE aShape )
{
    switch( aShape )
    {
    case LABEL_FLAG_SHAPE::L_INPUT: return SCHEMATIC_LEXER::TokenName( T_input );
    case LABEL_FLAG_SHAPE::L_OUTPUT: return SCHEMATIC_LEXER::TokenName( T_output );
    case LABEL_FLAG_SHAPE::L_BIDI: return SCHEMATIC_LEXER::TokenName( T_bidirectional );
    case LABEL_FLAG_SHAPE::L_TRISTATE: return SCHEMATIC_LEXER::TokenName( T_tri_state );
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED: return SCHEMATIC_LEXER::TokenName( T_passive );
    case LABEL_FLAG_SHAPE::F_DOT: return SCHEMATIC_LEXER::TokenName( T_dot );
    case LABEL_FLAG_SHAPE::F_ROUND: return SCHEMATIC_LEXER::TokenName( T_round );
    case LABEL_FLAG_SHAPE::F_DIAMOND: return SCHEMATIC_LEXER::TokenName( T_diamond );
    case LABEL_FLAG_SHAPE::F_RECTANGLE: return SCHEMATIC_LEXER::TokenName( T_rectangle );
    default: wxFAIL; return SCHEMATIC_LEXER::TokenName( T_passive );
    }
}


static EDA_ANGLE getSheetPinAngle( SHEET_SIDE aSide )
{
    switch( aSide )
    {
    case SHEET_SIDE::UNDEFINED:
    case SHEET_SIDE::LEFT: return ANGLE_180;
    case SHEET_SIDE::RIGHT: return ANGLE_0;
    case SHEET_SIDE::TOP: return ANGLE_90;
    case SHEET_SIDE::BOTTOM: return ANGLE_270;
    default: wxFAIL; return ANGLE_0;
    }
}


static const char* getTextTypeToken( KICAD_T aType )
{
    switch( aType )
    {
    case SCH_TEXT_T: return SCHEMATIC_LEXER::TokenName( T_text );
    case SCH_LABEL_T: return SCHEMATIC_LEXER::TokenName( T_label );
    case SCH_GLOBAL_LABEL_T: return SCHEMATIC_LEXER::TokenName( T_global_label );
    case SCH_HIER_LABEL_T: return SCHEMATIC_LEXER::TokenName( T_hierarchical_label );
    case SCH_DIRECTIVE_LABEL_T: return SCHEMATIC_LEXER::TokenName( T_netclass_flag );
    default: wxFAIL; return SCHEMATIC_LEXER::TokenName( T_text );
    }
}


static std::string formatIU( const int& aValue )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aValue );
}


static std::string formatIU( const VECTOR2I& aPt, bool aInvertY )
{
    VECTOR2I pt( aPt.x, aInvertY ? -aPt.y : aPt.y );
    return EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pt );
}


static void formatArc( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aArc, bool aIsPrivate, const STROKE_PARAMS& aStroke,
                       FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(arc %s (start %s) (mid %s) (end %s)", aIsPrivate ? "private" : "",
                       formatIU( aArc->GetStart(), aInvertY ).c_str(), formatIU( aArc->GetArcMid(), aInvertY ).c_str(),
                       formatIU( aArc->GetEnd(), aInvertY ).c_str() );

    KICAD_FORMAT::LEGACY::FormatStroke( aFormatter, aStroke, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


static void formatCircle( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aCircle, bool aIsPrivate,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                          const KIID& aUuid )
{
    aFormatter->Print( "(circle %s (center %s) (radius %s)", aIsPrivate ? "private" : "",
                       formatIU( aCircle->GetStart(), aInvertY ).c_str(), formatIU( aCircle->GetRadius() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatStroke( aFormatter, aStroke, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


static void formatRect( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aRect, bool aIsPrivate, const STROKE_PARAMS& aStroke,
                        FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(rectangle %s (start %s) (end %s)", aIsPrivate ? "private" : "",
                       formatIU( aRect->GetStart(), aInvertY ).c_str(), formatIU( aRect->GetEnd(), aInvertY ).c_str() );
    if( aRect->GetCornerRadius() > 0 )
        aFormatter->Print( "(radius %s)", formatIU( aRect->GetCornerRadius() ).c_str() );
    KICAD_FORMAT::LEGACY::FormatStroke( aFormatter, aStroke, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


static void formatBezier( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aBezier, bool aIsPrivate,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                          const KIID& aUuid )
{
    aFormatter->Print( "(bezier %s (pts ", aIsPrivate ? "private" : "" );

    for( const VECTOR2I& pt :
         { aBezier->GetStart(), aBezier->GetBezierC1(), aBezier->GetBezierC2(), aBezier->GetEnd() } )
    {
        aFormatter->Print( "(xy %s)", formatIU( pt, aInvertY ).c_str() );
    }

    aFormatter->Print( ")" ); // Closes pts token

    KICAD_FORMAT::LEGACY::FormatStroke( aFormatter, aStroke, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


static void formatPoly( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aPolyLine, bool aIsPrivate,
                        const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor, bool aInvertY,
                        const KIID& aUuid )
{
    aFormatter->Print( "(polyline %s (pts ", aIsPrivate ? "private" : "" );

    const SHAPE_POLY_SET& aPolySet = aPolyLine->GetPolyShape();

    if( aPolySet.OutlineCount() == 0 )
    {
        // If we've managed to get a polyline with no points, that's probably a bad thing,
        // but at least don't dereference it and crash.
        wxFAIL_MSG( "Polyline has no outlines" );
    }
    else
    {
        const SHAPE_LINE_CHAIN& outline = aPolySet.Outline( 0 );

        for( const VECTOR2I& pt : outline.CPoints() )
            aFormatter->Print( "(xy %s)", formatIU( pt, aInvertY ).c_str() );

        if( outline.IsClosed() && outline.PointCount() > 1 && outline.CPoint( -1 ) != outline.CPoint( 0 ) )
            aFormatter->Print( "(xy %s)", formatIU( outline.CPoint( 0 ), aInvertY ).c_str() );
    }

    aFormatter->Print( ")" ); // Closes pts token

    KICAD_FORMAT::LEGACY::FormatStroke( aFormatter, aStroke, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


static void libSaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter, const wxString& aLibName = wxEmptyString,
                           bool aIncludeData = true );
static void libSaveSymbolDrawItem( SCH_ITEM* aItem, OUTPUTFORMATTER& aFormatter );
static void libSaveField( SCH_FIELD* aField, OUTPUTFORMATTER& aFormatter );
static void libSavePin( SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter );
static void libSaveText( SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter );
static void libSaveTextBox( SCH_TEXTBOX* aTextBox, OUTPUTFORMATTER& aFormatter );
static void libSaveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter );


static void libSaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter, const wxString& aLibName,
                           bool aIncludeData )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    // If we've requested to embed the fonts in the symbol, do so.
    // Otherwise, clear the embedded fonts from the symbol. Embedded
    // fonts will be used if available
    if( aSymbol->GetAreFontsEmbedded() )
        aSymbol->EmbedFonts();
    else
        aSymbol->GetEmbeddedFiles()->ClearEmbeddedFonts();

    std::vector<SCH_FIELD*> orderedFields;
    std::string             name = aFormatter.Quotew( aSymbol->GetLibId().GetLibItemName().wx_str() );
    std::string             unitName = aSymbol->GetLibId().GetLibItemName();

    if( !aLibName.IsEmpty() )
    {
        name = aFormatter.Quotew( aLibName );

        LIB_ID unitId;

        wxCHECK2( unitId.Parse( aLibName ) < 0, /* do nothing */ );

        unitName = unitId.GetLibItemName();
    }

    if( aSymbol->IsRoot() )
    {
        aFormatter.Print( "(symbol %s", name.c_str() );

        if( aSymbol->IsGlobalPower() )
            aFormatter.Print( "(power global)" );
        else if( aSymbol->IsLocalPower() )
            aFormatter.Print( "(power local)" );

        if( aSymbol->IsMultiBodyStyle() )
        {
            aFormatter.Print( "(body_styles " );

            if( aSymbol->HasDeMorganBodyStyles() )
            {
                aFormatter.Print( "demorgan" );
            }
            else
            {
                for( const wxString& bodyStyle : aSymbol->GetBodyStyleNames() )
                    aFormatter.Print( "%s ", aFormatter.Quotew( bodyStyle ).c_str() );
            }

            aFormatter.Print( ")" );
        }

        if( !aSymbol->GetShowPinNumbers() )
            aFormatter.Print( "(pin_numbers (hide yes))" );

        if( aSymbol->GetPinNameOffset() != schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET )
            || !aSymbol->GetShowPinNames() )
        {
            aFormatter.Print( "(pin_names" );

            if( aSymbol->GetPinNameOffset() != schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET ) )
            {
                aFormatter.Print(
                        "(offset %s)",
                        EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSymbol->GetPinNameOffset() ).c_str() );
            }

            if( !aSymbol->GetShowPinNames() )
                KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "hide", true );

            aFormatter.Print( ")" );
        }

        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "exclude_from_sim", aSymbol->GetExcludedFromSim() );
        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "in_bom", !aSymbol->GetExcludedFromBOM() );
        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "on_board", !aSymbol->GetExcludedFromBoard() );
        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "in_pos_files", !aSymbol->GetExcludedFromPosFiles() );

        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "duplicate_pin_numbers_are_jumpers",
                                          aSymbol->GetDuplicatePinNumbersAreJumpers() );

        const JUMPER_GROUP_SET& jumperGroups = aSymbol->JumperPinGroups();

        if( !jumperGroups.IsEmpty() )
        {
            aFormatter.Print( "(jumper_pin_groups" );

            for( const JUMPER_GROUP& group : jumperGroups.GetAll() )
            {
                aFormatter.Print( "(" );

                for( const wxString& padName : group.GetNames() )
                    aFormatter.Print( "%s ", aFormatter.Quotew( padName ).c_str() );

                aFormatter.Print( ")" );
            }

            aFormatter.Print( ")" );
        }

        aSymbol->GetFields( orderedFields );

        for( SCH_FIELD* field : orderedFields )
            libSaveField( field, aFormatter );

        // @todo At some point in the future the lock status (all units interchangeable) should
        // be set deterministically. For now a custom lock property is used to preserve the
        // locked flag state.
        if( aSymbol->UnitsLocked() )
        {
            SCH_FIELD locked( nullptr, FIELD_T::USER, "ki_locked" );
            libSaveField( &locked, aFormatter );
        }

        libSaveDcmInfoAsFields( aSymbol, aFormatter );

        // Save the draw items grouped by units.
        std::vector<LIB_SYMBOL_UNIT> units = aSymbol->GetUnitDrawItems();
        std::sort( units.begin(), units.end(),
                   []( const LIB_SYMBOL_UNIT& a, const LIB_SYMBOL_UNIT& b )
                   {
                       if( a.m_unit == b.m_unit )
                           return a.m_bodyStyle < b.m_bodyStyle;

                       return a.m_unit < b.m_unit;
                   } );

        for( const LIB_SYMBOL_UNIT& unit : units )
        {
            // Add quotes and escape chars like ") to the UTF8 unitName string
            name = aFormatter.Quotes( unitName );
            name.pop_back(); // Remove last char: the quote ending the string.

            aFormatter.Print( "(symbol %s_%d_%d\"", name.c_str(), unit.m_unit, unit.m_bodyStyle );

            // if the unit has a display name, write that
            if( aSymbol->GetUnitDisplayNames().contains( unit.m_unit ) )
            {
                name = aSymbol->GetUnitDisplayNames().at( unit.m_unit );
                aFormatter.Print( "(unit_name %s)", aFormatter.Quotes( name ).c_str() );
            }

            // Enforce item ordering
            auto cmp = []( const SCH_ITEM* a, const SCH_ITEM* b )
            {
                return *a < *b;
            };

            std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

            for( SCH_ITEM* item : unit.m_items )
                save_map.insert( item );

            for( SCH_ITEM* item : save_map )
                libSaveSymbolDrawItem( item, aFormatter );

            aFormatter.Print( ")" );
        }

        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "embedded_fonts", aSymbol->GetAreFontsEmbedded() );

        if( !aSymbol->EmbeddedFileMap().empty() )
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( aFormatter, *aSymbol, aIncludeData );
    }
    else
    {
        std::shared_ptr<LIB_SYMBOL> parent = aSymbol->GetParent().lock();

        wxASSERT( parent );

        aFormatter.Print( "(symbol %s (extends %s)", name.c_str(), aFormatter.Quotew( parent->GetName() ).c_str() );

        aSymbol->GetFields( orderedFields );

        for( SCH_FIELD* field : orderedFields )
            libSaveField( field, aFormatter );

        libSaveDcmInfoAsFields( aSymbol, aFormatter );

        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "embedded_fonts", aSymbol->GetAreFontsEmbedded() );

        if( !aSymbol->EmbeddedFileMap().empty() )
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( aFormatter, *aSymbol, aIncludeData );
    }

    aFormatter.Print( ")" );
}


static void libSaveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    if( !aSymbol->GetRawKeyWords().IsEmpty() )
    {
        SCH_FIELD keywords( nullptr, FIELD_T::USER, wxString( "ki_keywords" ) );
        keywords.SetVisible( false );
        keywords.SetText( aSymbol->GetRawKeyWords() );
        libSaveField( &keywords, aFormatter );
    }

    wxArrayString fpFilters = aSymbol->GetFPFilters();

    if( !fpFilters.IsEmpty() )
    {
        wxString tmp;

        for( const wxString& filter : fpFilters )
        {
            // Spaces are not handled in fp filter names so escape spaces if any
            wxString curr_filter = EscapeString( filter, ESCAPE_CONTEXT::CTX_NO_SPACE );

            if( tmp.IsEmpty() )
                tmp = curr_filter;
            else
                tmp += " " + curr_filter;
        }

        SCH_FIELD description( nullptr, FIELD_T::USER, wxString( "ki_fp_filters" ) );
        description.SetVisible( false );
        description.SetText( tmp );
        libSaveField( &description, aFormatter );
    }
}


static void libSaveSymbolDrawItem( SCH_ITEM* aItem, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aItem, "Invalid SCH_ITEM pointer." );

    switch( aItem->Type() )
    {
    case SCH_SHAPE_T:
    {
        SCH_SHAPE*    shape = static_cast<SCH_SHAPE*>( aItem );
        STROKE_PARAMS stroke = shape->GetStroke();
        FILL_T        fillMode = shape->GetFillMode();
        COLOR4D       fillColor = shape->GetFillColor();
        bool          isPrivate = shape->IsPrivate();

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC: formatArc( &aFormatter, shape, isPrivate, stroke, fillMode, fillColor, true ); break;

        case SHAPE_T::CIRCLE: formatCircle( &aFormatter, shape, isPrivate, stroke, fillMode, fillColor, true ); break;

        case SHAPE_T::RECTANGLE: formatRect( &aFormatter, shape, isPrivate, stroke, fillMode, fillColor, true ); break;

        case SHAPE_T::BEZIER: formatBezier( &aFormatter, shape, isPrivate, stroke, fillMode, fillColor, true ); break;

        case SHAPE_T::POLY: formatPoly( &aFormatter, shape, isPrivate, stroke, fillMode, fillColor, true ); break;

        default: UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }

    case SCH_PIN_T: libSavePin( static_cast<SCH_PIN*>( aItem ), aFormatter ); break;

    case SCH_TEXT_T: libSaveText( static_cast<SCH_TEXT*>( aItem ), aFormatter ); break;

    case SCH_TEXTBOX_T: libSaveTextBox( static_cast<SCH_TEXTBOX*>( aItem ), aFormatter ); break;

    default: UNIMPLEMENTED_FOR( aItem->GetClass() );
    }
}


static void libSaveField( SCH_FIELD* aField, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aField && aField->Type() == SCH_FIELD_T, "Invalid SCH_FIELD object." );

    wxString fieldName = aField->GetName();

    if( aField->IsMandatory() )
        fieldName = aField->GetUntranslatedName();

    aFormatter.Print( "(property %s %s %s (at %s %s %s)", aField->IsPrivate() ? "private" : "",
                      aFormatter.Quotew( fieldName ).c_str(), aFormatter.Quotew( aField->GetText() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aField->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -aField->GetPosition().y ).c_str(),
                      fmt::format( "{:g}", aField->GetTextAngle().AsDegrees() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "show_name", aField->IsNameShown() );

    KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "do_not_autoplace", !aField->CanAutoplace() );

    if( !aField->IsVisible() )
        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "hide", true );

    KICAD_FORMAT::LEGACY::FormatTextV10( &aFormatter, *aField, schIUScale, 0 );
    aFormatter.Print( ")" );
}


static void libSavePin( SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPin && aPin->Type() == SCH_PIN_T, "Invalid SCH_PIN object." );

    aPin->ClearFlags( IS_CHANGED );

    aFormatter.Print( "(pin %s %s (at %s %s %s) (length %s)", getPinElectricalTypeToken( aPin->GetType() ),
                      getPinShapeToken( aPin->GetShape() ),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -aPin->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( getPinAngle( aPin->GetOrientation() ) ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetLength() ).c_str() );

    if( !aPin->IsVisible() )
        KICAD_FORMAT::LEGACY::FormatBool( &aFormatter, "hide", true );

    // This follows the EDA_TEXT effects formatting for future expansion.
    aFormatter.Print( "(name %s (effects (font (size %s %s))))", aFormatter.Quotew( aPin->GetName() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetNameTextSize() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetNameTextSize() ).c_str() );

    aFormatter.Print( "(number %s (effects (font (size %s %s))))", aFormatter.Quotew( aPin->GetNumber() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetNumberTextSize() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aPin->GetNumberTextSize() ).c_str() );


    for( const std::pair<const wxString, SCH_PIN::ALT>& alt : aPin->GetAlternates() )
    {
        // There was a bug somewhere in the alternate pin code that allowed pin alternates with no
        // name to be saved in library symbols. This strips any invalid alternates just in case
        // that code resurfaces.
        if( alt.second.m_Name.IsEmpty() )
            continue;

        aFormatter.Print( "(alternate %s %s %s)", aFormatter.Quotew( alt.second.m_Name ).c_str(),
                          getPinElectricalTypeToken( alt.second.m_Type ), getPinShapeToken( alt.second.m_Shape ) );
    }

    aFormatter.Print( ")" );
}


static void libSaveText( SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aText && aText->Type() == SCH_TEXT_T, "Invalid SCH_TEXT object." );

    aFormatter.Print( "(text %s %s (at %s %s %d)", aText->IsPrivate() ? "private" : "",
                      aFormatter.Quotew( aText->GetText() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aText->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -aText->GetPosition().y ).c_str(),
                      aText->GetTextAngle().AsTenthsOfADegree() );

    KICAD_FORMAT::LEGACY::FormatTextV10( &aFormatter, *aText, schIUScale, 0 );
    aFormatter.Print( ")" );
}


static void libSaveTextBox( SCH_TEXTBOX* aTextBox, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aTextBox && aTextBox->Type() == SCH_TEXTBOX_T, "Invalid SCH_TEXTBOX object." );

    aFormatter.Print( "(text_box %s %s", aTextBox->IsPrivate() ? "private" : "",
                      aFormatter.Quotew( aTextBox->GetText() ).c_str() );

    VECTOR2I pos = aTextBox->GetStart();
    VECTOR2I size = aTextBox->GetEnd() - pos;

    aFormatter.Print( "(at %s %s %s) (size %s %s) (margins %s %s %s %s)",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -pos.y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( aTextBox->GetTextAngle() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -size.y ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginLeft() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginTop() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginRight() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginBottom() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatStroke( &aFormatter, aTextBox->GetStroke(), schIUScale );
    formatFill( &aFormatter, aTextBox->GetFillMode(), aTextBox->GetFillColor() );
    KICAD_FORMAT::LEGACY::FormatTextV10( &aFormatter, *aTextBox, schIUScale, 0 );
    aFormatter.Print( ")" );
}


void SCH_WRITER_V10::SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic )
{
    LOCALE_IO toggle;

    m_schematic = aSchematic;

    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( aFileName );

    m_out = &formatter;
    Format( aSheet );
    formatter.Finish();
    m_out = nullptr;
}


void SCH_WRITER_V10::SaveSymbolLibrary( const wxString& aFileName, const std::vector<LIB_SYMBOL*>& aSymbols )
{
    LOCALE_IO toggle;

    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( aFileName );

    formatter.Print( "(kicad_symbol_lib (version %d) (generator \"kicad_symbol_editor\") "
                     "(generator_version \"%s\")",
                     SCH_WRITER_V10::SYMBOL_LIB_VERSION, GetMajorMinorVersion().c_str().AsChar() );

    for( LIB_SYMBOL* symbol : aSymbols )
        libSaveSymbol( symbol, formatter, wxEmptyString );

    formatter.Print( ")" );
    formatter.Finish();
}


void SCH_WRITER_V10::Format( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET* object." );
    wxCHECK_RET( m_schematic != nullptr, "NULL SCHEMATIC* object." );

    SCH_SHEET_LIST sheets = m_schematic->Hierarchy();
    SCH_SCREEN*    screen = aSheet->GetScreen();

    wxCHECK( screen, /* void */ );

    // If we've requested to embed the fonts in the schematic, do so.
    // Otherwise, clear the embedded fonts from the schematic. Embedded
    // fonts will be used if available
    if( m_schematic->GetAreFontsEmbedded() )
        m_schematic->EmbedFonts();
    else
        m_schematic->GetEmbeddedFiles()->ClearEmbeddedFonts();

    m_out->Print( "(kicad_sch (version %d) (generator \"eeschema\") (generator_version %s)",
                  SCH_WRITER_V10::FORMAT_VERSION, m_out->Quotew( GetMajorMinorVersion() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, screen->GetUuid() );

    KICAD_FORMAT::LEGACY::FormatPageV10( m_out, screen->GetPageSettings() );
    KICAD_FORMAT::LEGACY::FormatTitle( m_out, screen->GetTitleBlock() );

    // Save cache library.
    m_out->Print( "(lib_symbols" );

    for( const auto& [libItemName, libSymbol] : screen->GetLibSymbols() )
        libSaveSymbol( libSymbol, *m_out, libItemName );

    m_out->Print( ")" );

    // Enforce item ordering
    auto cmp = []( const SCH_ITEM* a, const SCH_ITEM* b )
    {
        if( a->Type() != b->Type() )
            return a->Type() < b->Type();

        return a->m_Uuid < b->m_Uuid;
    };

    std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

    for( SCH_ITEM* item : screen->Items() )
    {
        // Markers are not saved, so keep them from being considered below
        if( item->Type() != SCH_MARKER_T )
            save_map.insert( item );
    }

    for( SCH_ITEM* item : save_map )
    {
        switch( item->Type() )
        {
        case SCH_SYMBOL_T: saveSymbol( static_cast<SCH_SYMBOL*>( item ), *m_schematic, sheets, false ); break;

        case SCH_BITMAP_T: saveBitmap( static_cast<SCH_BITMAP&>( *item ) ); break;

        case SCH_SHEET_T: saveSheet( static_cast<SCH_SHEET*>( item ), sheets ); break;

        case SCH_JUNCTION_T: saveJunction( static_cast<SCH_JUNCTION*>( item ) ); break;

        case SCH_NO_CONNECT_T: saveNoConnect( static_cast<SCH_NO_CONNECT*>( item ) ); break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T: saveBusEntry( static_cast<SCH_BUS_ENTRY_BASE*>( item ) ); break;

        case SCH_LINE_T: saveLine( static_cast<SCH_LINE*>( item ) ); break;

        case SCH_SHAPE_T: saveShape( static_cast<SCH_SHAPE*>( item ) ); break;

        case SCH_RULE_AREA_T: saveRuleArea( static_cast<SCH_RULE_AREA*>( item ) ); break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T: saveText( static_cast<SCH_TEXT*>( item ) ); break;

        case SCH_TEXTBOX_T: saveTextBox( static_cast<SCH_TEXTBOX*>( item ) ); break;

        case SCH_TABLE_T: saveTable( static_cast<SCH_TABLE*>( item ) ); break;

        case SCH_GROUP_T: saveGroup( static_cast<SCH_GROUP*>( item ) ); break;

        default: wxASSERT( "Unexpected schematic object type in SCH_WRITER_V10::Format()" );
        }
    }

    if( aSheet->HasRootInstance() )
    {
        std::vector<SCH_SHEET_INSTANCE> instances;

        instances.emplace_back( aSheet->GetRootInstance() );
        saveInstances( instances );
    }

    if( m_schematic->GetTopLevelSheet( 0 ) == aSheet )
    {
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "embedded_fonts", m_schematic->GetAreFontsEmbedded() );

        // Save any embedded files
        if( !m_schematic->GetEmbeddedFiles()->IsEmpty() )
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( *m_out, *m_schematic, true );
    }

    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveSymbol( SCH_SYMBOL* aSymbol, const SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aSheetList,
                                 bool aForClipboard, const SCH_SHEET_PATH* aRelativePath )
{
    wxCHECK_RET( aSymbol != nullptr && m_out != nullptr, "" );

    std::string libName;

    wxString symbol_name = KICAD_FORMAT::LEGACY::FormatLibId( aSymbol->GetLibId() );

    if( symbol_name.size() )
    {
        libName = toUTFTildaText( symbol_name );
    }
    else
    {
        libName = "_NONAME_";
    }

    EDA_ANGLE angle;
    int       orientation = aSymbol->GetOrientation() & ~( SYM_MIRROR_X | SYM_MIRROR_Y );

    if( orientation == SYM_ORIENT_90 )
        angle = ANGLE_90;
    else if( orientation == SYM_ORIENT_180 )
        angle = ANGLE_180;
    else if( orientation == SYM_ORIENT_270 )
        angle = ANGLE_270;
    else
        angle = ANGLE_0;

    m_out->Print( "(symbol" );

    if( !aSymbol->UseLibIdLookup() )
    {
        m_out->Print( "(lib_name %s)", m_out->Quotew( aSymbol->GetSchSymbolLibraryName() ).c_str() );
    }

    m_out->Print( "(lib_id %s) (at %s %s %s)",
                  m_out->Quotew( KICAD_FORMAT::LEGACY::FormatLibId( aSymbol->GetLibId() ).wx_str() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSymbol->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSymbol->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

    bool mirrorX = aSymbol->GetOrientation() & SYM_MIRROR_X;
    bool mirrorY = aSymbol->GetOrientation() & SYM_MIRROR_Y;

    if( mirrorX || mirrorY )
    {
        m_out->Print( "(mirror %s %s)", mirrorX ? "x" : "", mirrorY ? "y" : "" );
    }

    // The symbol unit is always set to the ordianal instance regardless of the current sheet
    // instance to prevent file churn.
    SCH_SYMBOL_INSTANCE ordinalInstance;

    ordinalInstance.m_Reference = aSymbol->GetPrefix();

    const SCH_SCREEN* parentScreen = static_cast<const SCH_SCREEN*>( aSymbol->GetParent() );

    wxASSERT( parentScreen );

    if( parentScreen && m_schematic )
    {
        std::optional<SCH_SHEET_PATH> ordinalPath = m_schematic->Hierarchy().GetOrdinalPath( parentScreen );

        // Design blocks are saved from a temporary sheet & screen which will not be found in
        // the schematic, and will therefore have no ordinal path.
        // wxASSERT( ordinalPath );

        if( ordinalPath )
            aSymbol->GetInstance( ordinalInstance, ordinalPath->Path() );
        else if( aSymbol->GetInstances().size() )
            ordinalInstance = aSymbol->GetInstances()[0];
    }

    int unit = ordinalInstance.m_Unit;

    if( aForClipboard && aRelativePath )
    {
        SCH_SYMBOL_INSTANCE unitInstance;

        if( aSymbol->GetInstance( unitInstance, aRelativePath->Path() ) )
            unit = unitInstance.m_Unit;
    }

    m_out->Print( "(unit %d)", unit );
    m_out->Print( "(body_style %d)", aSymbol->GetBodyStyle() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", aSymbol->GetExcludedFromSim() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_bom", !aSymbol->GetExcludedFromBOM() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "on_board", !aSymbol->GetExcludedFromBoard() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_pos_files", !aSymbol->GetExcludedFromPosFiles() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", aSymbol->GetDNP() );

    AUTOPLACE_ALGO fieldsAutoplaced = aSymbol->GetFieldsAutoplaced();

    if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "fields_autoplaced", true );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aSymbol->m_Uuid );

    std::vector<SCH_FIELD*> orderedFields;
    aSymbol->GetFields( orderedFields, false );

    for( SCH_FIELD* field : orderedFields )
    {
        FIELD_T  id = field->GetId();
        wxString value = field->GetText();

        if( !aForClipboard && aSymbol->GetInstances().size() )
        {
            // The instance fields are always set to the default instance regardless of the
            // sheet instance to prevent file churn.
            if( id == FIELD_T::REFERENCE )
                field->SetText( ordinalInstance.m_Reference );
        }
        else if( aForClipboard && aSymbol->GetInstances().size() && aRelativePath && ( id == FIELD_T::REFERENCE ) )
        {
            SCH_SYMBOL_INSTANCE instance;

            if( aSymbol->GetInstance( instance, aRelativePath->Path() ) )
                field->SetText( instance.m_Reference );
        }

        try
        {
            saveField( field );
        }
        catch( ... )
        {
            // Restore the changed field text on write error.
            if( id == FIELD_T::REFERENCE )
                field->SetText( value );

            throw;
        }

        if( id == FIELD_T::REFERENCE )
            field->SetText( value );
    }

    for( const std::unique_ptr<SCH_PIN>& pin : aSymbol->GetRawPins() )
    {
        // There was a bug introduced somewhere in the original alternated pin code that would
        // set the alternate pin to the default pin name which caused a number of library symbol
        // comparison issues. Clearing the alternate pin resolves this issue.
        if( pin->GetAlt().IsEmpty() || ( pin->GetAlt() == pin->GetBaseName() ) )
        {
            m_out->Print( "(pin %s", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::LEGACY::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( ")" );
        }
        else
        {
            m_out->Print( "(pin %s", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::LEGACY::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( "(alternate %s))", m_out->Quotew( pin->GetAlt() ).c_str() );
        }
    }

    if( !aSymbol->GetInstances().empty() )
    {
        std::map<KIID, std::vector<SCH_SYMBOL_INSTANCE>> projectInstances;
        std::set<KIID>                                   currentProjectKeys;

        m_out->Print( "(instances" );

        wxString projectName;
        KIID     rootSheetUuid = aSchematic.Root().m_Uuid;

        // Collect top-level sheet UUIDs to identify current project instances.
        // When root is virtual (niluuid), Path() skips it, so instance paths
        // start with the real top-level sheet UUID, not niluuid.

        if( rootSheetUuid == niluuid )
        {
            for( const SCH_SHEET* sheet : aSchematic.GetTopLevelSheets() )
                currentProjectKeys.insert( sheet->m_Uuid );
        }
        else
        {
            currentProjectKeys.insert( rootSheetUuid );
        }

        for( const SCH_SYMBOL_INSTANCE& inst : aSymbol->GetInstances() )
        {
            // Zero length KIID_PATH objects are not valid and will cause a crash below.
            wxCHECK2( inst.m_Path.size(), continue );

            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it. This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            //
            // The root sheet UUID can be niluuid for the virtual root. In that case, instance
            // paths may include the virtual root, but SCH_SHEET_PATH::Path() skips it. We need
            // to normalize the path by removing the virtual root before comparison.
            KIID_PATH pathToCheck = inst.m_Path;

            // If root is virtual (niluuid) and path starts with virtual root, strip it
            if( rootSheetUuid == niluuid && !pathToCheck.empty() && pathToCheck[0] == niluuid )
            {
                if( pathToCheck.size() > 1 )
                {
                    pathToCheck.erase( pathToCheck.begin() );
                }
                else
                {
                    // Path only contains virtual root, skip it
                    continue;
                }
            }

            // Check if this instance is orphaned (no matching sheet path)
            // For virtual root, we check if the first real sheet matches one of the top-level sheets
            // For non-virtual root, we check if it matches the root sheet UUID
            bool belongsToThisProject = currentProjectKeys.count( pathToCheck[0] );

            bool isOrphaned = belongsToThisProject && !aSheetList.GetSheetPathByKIIDPath( pathToCheck );

            // Keep all instance data when copying to the clipboard. They may be needed on paste.
            if( !aForClipboard && isOrphaned )
                continue;

            // Group by project - use the first real sheet KIID (after stripping virtual root)
            KIID projectKey = pathToCheck[0];
            auto it = projectInstances.find( projectKey );

            if( it == projectInstances.end() )
                projectInstances[projectKey] = { inst };
            else
                it->second.emplace_back( inst );
        }

        for( auto& [uuid, instances] : projectInstances )
        {
            wxCHECK2( instances.size(), continue );

            // Sort project instances by KIID_PATH.
            std::sort( instances.begin(), instances.end(),
                       []( SCH_SYMBOL_INSTANCE& aLhs, SCH_SYMBOL_INSTANCE& aRhs )
                       {
                           return aLhs.m_Path < aRhs.m_Path;
                       } );

            if( currentProjectKeys.count( uuid ) )
                projectName = m_schematic->Project().GetProjectName();
            else
                projectName = instances[0].m_ProjectName;

            m_out->Print( "(project %s", m_out->Quotew( projectName ).c_str() );

            for( const SCH_SYMBOL_INSTANCE& instance : instances )
            {
                wxString  path;
                KIID_PATH tmp = instance.m_Path;

                if( aForClipboard && aRelativePath )
                    tmp.MakeRelativeTo( aRelativePath->Path() );

                path = tmp.AsString();

                m_out->Print( "(path %s (reference %s) (unit %d)", m_out->Quotew( path ).c_str(),
                              m_out->Quotew( instance.m_Reference ).c_str(), instance.m_Unit );

                if( !instance.m_Variants.empty() )
                {
                    for( const auto& [name, variant] : instance.m_Variants )
                    {
                        m_out->Print( "(variant (name %s)", m_out->Quotew( name ).c_str() );

                        if( variant.m_DNP != aSymbol->GetDNP() )
                            KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", variant.m_DNP );

                        if( variant.m_ExcludedFromSim != aSymbol->GetExcludedFromSim() )
                            KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", variant.m_ExcludedFromSim );

                        if( variant.m_ExcludedFromBOM != aSymbol->GetExcludedFromBOM() )
                            KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_bom", !variant.m_ExcludedFromBOM );

                        if( variant.m_ExcludedFromBoard != aSymbol->GetExcludedFromBoard() )
                            KICAD_FORMAT::LEGACY::FormatBool( m_out, "on_board", !variant.m_ExcludedFromBoard );

                        if( variant.m_ExcludedFromPosFiles != aSymbol->GetExcludedFromPosFiles() )
                            KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_pos_files", !variant.m_ExcludedFromPosFiles );

                        for( const auto& [fname, fvalue] : variant.m_Fields )
                        {
                            m_out->Print( "(field (name %s) (value %s))", m_out->Quotew( fname ).c_str(),
                                          m_out->Quotew( fvalue ).c_str() );
                        }

                        m_out->Print( ")" ); // Closes `variant` token.
                    }
                }

                m_out->Print( ")" ); // Closes `path` token.
            }

            m_out->Print( ")" ); // Closes `project`.
        }

        m_out->Print( ")" ); // Closes `instances`.
    }

    m_out->Print( ")" ); // Closes `symbol`.
}


void SCH_WRITER_V10::saveField( SCH_FIELD* aField )
{
    wxCHECK_RET( aField != nullptr && m_out != nullptr, "" );

    wxString fieldName;

    if( aField->IsMandatory() )
        fieldName = aField->GetUntranslatedName();
    else
        fieldName = aField->GetName();

    m_out->Print( "(property %s %s %s (at %s %s %s)", aField->IsPrivate() ? "private" : "",
                  m_out->Quotew( fieldName ).c_str(), m_out->Quotew( aField->GetText() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aField->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aField->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aField->GetTextAngle() ).c_str() );

    if( !aField->IsVisible() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "hide", true );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "show_name", aField->IsNameShown() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "do_not_autoplace", !aField->CanAutoplace() );

    if( !KICAD_FORMAT::LEGACY::IsDefaultTextV10( *aField )
        || ( aField->GetTextHeight() != schIUScale.MilsToIU( DEFAULT_SIZE_TEXT ) ) )
    {
        KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *aField, schIUScale, 0 );
    }

    m_out->Print( ")" ); // Closes `property` token
}


void SCH_WRITER_V10::saveBitmap( const SCH_BITMAP& aBitmap )
{
    wxCHECK_RET( m_out != nullptr, "" );

    const REFERENCE_IMAGE& refImage = aBitmap.GetReferenceImage();
    const BITMAP_BASE&     bitmapBase = refImage.GetImage();

    const wxImage* image = bitmapBase.GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( "(image (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, refImage.GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, refImage.GetPosition().y ).c_str() );

    double scale = refImage.GetImageScale();

    // 20230121 or older file format versions assumed 300 image PPI at load/save.
    // Let's keep compatibility by changing image scale.
    if( SCH_WRITER_V10::FORMAT_VERSION <= 20230121 )
        scale = scale * 300.0 / bitmapBase.GetPPI();

    if( scale != 1.0 )
        m_out->Print( "%s", fmt::format( "(scale {:g})", refImage.GetImageScale() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aBitmap.m_Uuid );

    wxMemoryOutputStream stream;
    bitmapBase.SaveImageData( stream );

    KICAD_FORMAT::LEGACY::FormatStreamData( *m_out, *stream.GetOutputStreamBuffer() );

    m_out->Print( ")" ); // Closes image token.
}


void SCH_WRITER_V10::saveSheet( SCH_SHEET* aSheet, const SCH_SHEET_LIST& aSheetList )
{
    wxCHECK_RET( aSheet != nullptr && m_out != nullptr, "" );

    m_out->Print( "(sheet (at %s %s) (size %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSheet->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSheet->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSheet->GetSize().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aSheet->GetSize().y ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", aSheet->GetExcludedFromSim() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_bom", !aSheet->GetExcludedFromBOM() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "on_board", !aSheet->GetExcludedFromBoard() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", aSheet->GetDNP() );

    AUTOPLACE_ALGO fieldsAutoplaced = aSheet->GetFieldsAutoplaced();

    if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "fields_autoplaced", true );

    STROKE_PARAMS stroke( aSheet->GetBorderWidth(), LINE_STYLE::SOLID, aSheet->GetBorderColor() );

    stroke.SetWidth( aSheet->GetBorderWidth() );
    KICAD_FORMAT::LEGACY::FormatStroke( m_out, stroke, schIUScale );

    m_out->Print( "(fill (color %d %d %d %s))", KiROUND( aSheet->GetBackgroundColor().r * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().g * 255.0 ), KiROUND( aSheet->GetBackgroundColor().b * 255.0 ),
                  FormatDouble2Str( aSheet->GetBackgroundColor().a ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aSheet->m_Uuid );

    for( SCH_FIELD& field : aSheet->GetFields() )
        saveField( &field );

    for( const SCH_SHEET_PIN* pin : aSheet->GetPins() )
    {
        m_out->Print( "(pin %s %s (at %s %s %s)", EscapedUTF8( pin->GetText() ).c_str(),
                      getSheetPinShapeToken( pin->GetShape() ),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pin->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pin->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( getSheetPinAngle( pin->GetSide() ) ).c_str() );

        KICAD_FORMAT::LEGACY::FormatUuid( m_out, pin->m_Uuid );

        KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *pin, schIUScale, 0 );

        m_out->Print( ")" ); // Closes pin token.
    }

    // Save all sheet instances here except the root sheet instance.
    std::vector<SCH_SHEET_INSTANCE> sheetInstances = aSheet->GetInstances();

    auto it = sheetInstances.begin();

    while( it != sheetInstances.end() )
    {
        if( it->m_Path.size() == 0 )
            it = sheetInstances.erase( it );
        else
            it++;
    }

    if( !sheetInstances.empty() )
    {
        m_out->Print( "(instances" );

        KIID lastProjectUuid;
        KIID rootSheetUuid = m_schematic->Root().m_Uuid;
        bool inProjectClause = false;

        std::set<KIID> currentProjectKeys;

        if( rootSheetUuid == niluuid )
        {
            for( const SCH_SHEET* sheet : m_schematic->GetTopLevelSheets() )
                currentProjectKeys.insert( sheet->m_Uuid );
        }
        else
        {
            currentProjectKeys.insert( rootSheetUuid );
        }

        for( size_t i = 0; i < sheetInstances.size(); i++ )
        {
            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it. This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            //
            // Keep all instance data when copying to the clipboard. It may be needed on paste.
            bool belongsToThisProject =
                    !sheetInstances[i].m_Path.empty() && currentProjectKeys.count( sheetInstances[i].m_Path[0] );

            if( belongsToThisProject && !aSheetList.GetSheetPathByKIIDPath( sheetInstances[i].m_Path, false ) )
            {
                if( inProjectClause
                    && ( ( i + 1 == sheetInstances.size() ) || lastProjectUuid != sheetInstances[i + 1].m_Path[0] ) )
                {
                    m_out->Print( ")" ); // Closes `project` token.
                    inProjectClause = false;
                }

                continue;
            }

            if( lastProjectUuid != sheetInstances[i].m_Path[0] )
            {
                wxString projectName;

                if( belongsToThisProject )
                    projectName = m_schematic->Project().GetProjectName();
                else
                    projectName = sheetInstances[i].m_ProjectName;

                lastProjectUuid = sheetInstances[i].m_Path[0];
                m_out->Print( "(project %s", m_out->Quotew( projectName ).c_str() );
                inProjectClause = true;
            }

            wxString path = sheetInstances[i].m_Path.AsString();

            m_out->Print( "(path %s (page %s)", m_out->Quotew( path ).c_str(),
                          m_out->Quotew( sheetInstances[i].m_PageNumber ).c_str() );

            if( !sheetInstances[i].m_Variants.empty() )
            {
                for( const auto& [name, variant] : sheetInstances[i].m_Variants )
                {
                    m_out->Print( "(variant (name %s)", m_out->Quotew( name ).c_str() );

                    if( variant.m_DNP != aSheet->GetDNP() )
                        KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", variant.m_DNP );

                    if( variant.m_ExcludedFromSim != aSheet->GetExcludedFromSim() )
                        KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", variant.m_ExcludedFromSim );

                    if( variant.m_ExcludedFromBOM != aSheet->GetExcludedFromBOM() )
                        KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_bom", !variant.m_ExcludedFromBOM );

                    for( const auto& [fname, fvalue] : variant.m_Fields )
                    {
                        m_out->Print( "(field (name %s) (value %s))", m_out->Quotew( fname ).c_str(),
                                      m_out->Quotew( fvalue ).c_str() );
                    }

                    m_out->Print( ")" ); // Closes `variant` token.
                }
            }

            m_out->Print( ")" ); // Closes `path` token.

            if( inProjectClause
                && ( ( i + 1 == sheetInstances.size() ) || lastProjectUuid != sheetInstances[i + 1].m_Path[0] ) )
            {
                m_out->Print( ")" ); // Closes `project` token.
                inProjectClause = false;
            }
        }

        m_out->Print( ")" ); // Closes `instances` token.
    }

    m_out->Print( ")" ); // Closes sheet token.
}


void SCH_WRITER_V10::saveJunction( SCH_JUNCTION* aJunction )
{
    wxCHECK_RET( aJunction != nullptr && m_out != nullptr, "" );

    m_out->Print( "(junction (at %s %s) (diameter %s) (color %d %d %d %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aJunction->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aJunction->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aJunction->GetDiameter() ).c_str(),
                  KiROUND( aJunction->GetColor().r * 255.0 ), KiROUND( aJunction->GetColor().g * 255.0 ),
                  KiROUND( aJunction->GetColor().b * 255.0 ), FormatDouble2Str( aJunction->GetColor().a ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aJunction->m_Uuid );
    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveNoConnect( SCH_NO_CONNECT* aNoConnect )
{
    wxCHECK_RET( aNoConnect != nullptr && m_out != nullptr, "" );

    m_out->Print( "(no_connect (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aNoConnect->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aNoConnect->GetPosition().y ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aNoConnect->m_Uuid );
    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry )
{
    wxCHECK_RET( aBusEntry != nullptr && m_out != nullptr, "" );

    // Bus to bus entries are converted to bus line segments.
    if( aBusEntry->GetClass() == "SCH_BUS_BUS_ENTRY" )
    {
        SCH_LINE busEntryLine( aBusEntry->GetPosition(), LAYER_BUS );

        busEntryLine.SetEndPoint( aBusEntry->GetEnd() );
        saveLine( &busEntryLine );
        return;
    }

    m_out->Print( "(bus_entry (at %s %s) (size %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aBusEntry->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aBusEntry->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aBusEntry->GetSize().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aBusEntry->GetSize().y ).c_str() );

    KICAD_FORMAT::LEGACY::FormatStroke( m_out, aBusEntry->GetStroke(), schIUScale );
    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aBusEntry->m_Uuid );
    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveShape( SCH_SHAPE* aShape )
{
    wxCHECK_RET( aShape != nullptr && m_out != nullptr, "" );

    switch( aShape->GetShape() )
    {
    case SHAPE_T::ARC:
        formatArc( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(), aShape->GetFillColor(), false,
                   aShape->m_Uuid );
        break;

    case SHAPE_T::CIRCLE:
        formatCircle( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(), aShape->GetFillColor(), false,
                      aShape->m_Uuid );
        break;

    case SHAPE_T::RECTANGLE:
        formatRect( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(), aShape->GetFillColor(), false,
                    aShape->m_Uuid );
        break;

    case SHAPE_T::BEZIER:
        formatBezier( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(), aShape->GetFillColor(), false,
                      aShape->m_Uuid );
        break;

    case SHAPE_T::POLY:
        formatPoly( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(), aShape->GetFillColor(), false,
                    aShape->m_Uuid );
        break;

    default: UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
    }
}


void SCH_WRITER_V10::saveRuleArea( SCH_RULE_AREA* aRuleArea )
{
    wxCHECK_RET( aRuleArea != nullptr && m_out != nullptr, "" );

    m_out->Print( "(rule_area " );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", aRuleArea->GetExcludedFromSim() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "in_bom", !aRuleArea->GetExcludedFromBOM() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "on_board", !aRuleArea->GetExcludedFromBoard() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", aRuleArea->GetDNP() );

    saveShape( aRuleArea );

    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveLine( SCH_LINE* aLine )
{
    wxCHECK_RET( aLine != nullptr && m_out != nullptr, "" );

    wxString lineType;

    STROKE_PARAMS line_stroke = aLine->GetStroke();

    switch( aLine->GetLayer() )
    {
    case LAYER_BUS: lineType = "bus"; break;
    case LAYER_WIRE: lineType = "wire"; break;
    case LAYER_NOTES: lineType = "polyline"; break;
    default: UNIMPLEMENTED_FOR( LayerName( aLine->GetLayer() ) );
    }

    m_out->Print( "(%s (pts (xy %s %s) (xy %s %s))", TO_UTF8( lineType ),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aLine->GetStartPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aLine->GetStartPoint().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aLine->GetEndPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aLine->GetEndPoint().y ).c_str() );

    KICAD_FORMAT::LEGACY::FormatStroke( m_out, line_stroke, schIUScale );
    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aLine->m_Uuid );
    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveText( SCH_TEXT* aText )
{
    wxCHECK_RET( aText != nullptr && m_out != nullptr, "" );

    // Note: label is nullptr SCH_TEXT, but not for SCH_LABEL_XXX,
    SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aText );

    m_out->Print( "(%s %s", getTextTypeToken( aText->Type() ), m_out->Quotew( aText->GetText() ).c_str() );

    if( aText->Type() == SCH_TEXT_T )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", aText->GetExcludedFromSim() );

    if( aText->Type() == SCH_DIRECTIVE_LABEL_T )
    {
        SCH_DIRECTIVE_LABEL* flag = static_cast<SCH_DIRECTIVE_LABEL*>( aText );

        m_out->Print( "(length %s)", EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, flag->GetPinLength() ).c_str() );
    }

    EDA_ANGLE angle = aText->GetTextAngle();

    if( label )
    {
        if( label->Type() == SCH_GLOBAL_LABEL_T || label->Type() == SCH_HIER_LABEL_T
            || label->Type() == SCH_DIRECTIVE_LABEL_T )
        {
            m_out->Print( "(shape %s)", getSheetPinShapeToken( label->GetShape() ) );
        }

        // The angle of the text is always 0 or 90 degrees for readibility reasons,
        // but the item itself can have more rotation (-90 and 180 deg)
        switch( label->GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT: angle += ANGLE_180; break;
        case SPIN_STYLE::UP: break;
        case SPIN_STYLE::RIGHT: break;
        case SPIN_STYLE::BOTTOM: angle += ANGLE_180; break;
        }
    }

    m_out->Print( "(at %s %s %s)", EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aText->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aText->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

    if( label && !label->GetFields().empty() )
    {
        AUTOPLACE_ALGO fieldsAutoplaced = label->GetFieldsAutoplaced();

        if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "fields_autoplaced", true );
    }

    KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *aText, schIUScale, 0 );
    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aText->m_Uuid );

    if( label )
    {
        for( SCH_FIELD& field : label->GetFields() )
            saveField( &field );
    }

    m_out->Print( ")" ); // Closes text token.
}


void SCH_WRITER_V10::saveTextBox( SCH_TEXTBOX* aTextBox )
{
    wxCHECK_RET( aTextBox != nullptr && m_out != nullptr, "" );

    m_out->Print( "(%s %s", aTextBox->Type() == SCH_TABLECELL_T ? "table_cell" : "text_box",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_sim", aTextBox->GetExcludedFromSim() );

    VECTOR2I pos = aTextBox->GetStart();
    VECTOR2I size = aTextBox->GetEnd() - pos;

    m_out->Print( "(at %s %s %s) (size %s %s) (margins %s %s %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aTextBox->GetTextAngle() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginLeft() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginTop() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginRight() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginBottom() ).c_str() );

    if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( aTextBox ) )
        m_out->Print( "(span %d %d)", cell->GetColSpan(), cell->GetRowSpan() );

    if( aTextBox->Type() != SCH_TABLECELL_T )
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTextBox->GetStroke(), schIUScale );

    formatFill( m_out, aTextBox->GetFillMode(), aTextBox->GetFillColor() );
    KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *aTextBox, schIUScale, 0 );
    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTextBox->m_Uuid );
    m_out->Print( ")" );
}


void SCH_WRITER_V10::saveTable( SCH_TABLE* aTable )
{
    if( aTable->GetFlags() & SKIP_STRUCT )
    {
        aTable = static_cast<SCH_TABLE*>( aTable->Clone() );

        int minCol = aTable->GetColCount();
        int maxCol = -1;
        int minRow = aTable->GetRowCount();
        int maxRow = -1;

        for( int row = 0; row < aTable->GetRowCount(); ++row )
        {
            for( int col = 0; col < aTable->GetColCount(); ++col )
            {
                SCH_TABLECELL* cell = aTable->GetCell( row, col );

                if( cell->IsSelected() )
                {
                    minRow = std::min( minRow, row );
                    maxRow = std::max( maxRow, row );
                    minCol = std::min( minCol, col );
                    maxCol = std::max( maxCol, col );
                }
                else
                {
                    cell->SetFlags( STRUCT_DELETED );
                }
            }
        }

        wxCHECK_MSG( maxCol >= minCol && maxRow >= minRow, /*void*/, wxT( "No selected cells!" ) );

        int destRow = 0;

        for( int row = minRow; row <= maxRow; row++ )
            aTable->SetRowHeight( destRow++, aTable->GetRowHeight( row ) );

        int destCol = 0;

        for( int col = minCol; col <= maxCol; col++ )
            aTable->SetColWidth( destCol++, aTable->GetColWidth( col ) );

        aTable->DeleteMarkedCells();
        aTable->SetColCount( ( maxCol - minCol ) + 1 );
    }

    wxCHECK_RET( aTable != nullptr && m_out != nullptr, "" );

    m_out->Print( "(table (column_count %d)", aTable->GetColCount() );

    m_out->Print( "(border" );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "external", aTable->StrokeExternal() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "header", aTable->StrokeHeaderSeparator() );

    if( aTable->StrokeExternal() || aTable->StrokeHeaderSeparator() )
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTable->GetBorderStroke(), schIUScale );

    m_out->Print( ")" ); // Close `border` token.

    m_out->Print( "(separators" );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "rows", aTable->StrokeRows() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "cols", aTable->StrokeColumns() );

    if( aTable->StrokeRows() || aTable->StrokeColumns() )
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTable->GetSeparatorsStroke(), schIUScale );

    m_out->Print( ")" ); // Close `separators` token.

    m_out->Print( "(column_widths" );

    for( int col = 0; col < aTable->GetColCount(); ++col )
    {
        m_out->Print( " %s", EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTable->GetColWidth( col ) ).c_str() );
    }

    m_out->Print( ")" );

    m_out->Print( "(row_heights" );

    for( int row = 0; row < aTable->GetRowCount(); ++row )
    {
        m_out->Print( " %s", EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTable->GetRowHeight( row ) ).c_str() );
    }

    m_out->Print( ")" );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTable->m_Uuid );

    m_out->Print( "(cells" );

    for( SCH_TABLECELL* cell : aTable->GetCells() )
        saveTextBox( cell );

    m_out->Print( ")" ); // Close `cells` token.
    m_out->Print( ")" ); // Close `table` token.

    if( aTable->GetFlags() & SKIP_STRUCT )
        delete aTable;
}


void SCH_WRITER_V10::saveGroup( SCH_GROUP* aGroup )
{
    // Don't write empty groups
    if( aGroup->GetItems().empty() )
        return;

    m_out->Print( "(group %s", m_out->Quotew( aGroup->GetName() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aGroup->m_Uuid );

    if( aGroup->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    if( aGroup->HasDesignBlockLink() )
        m_out->Print( "(lib_id \"%s\")", KICAD_FORMAT::LEGACY::FormatLibId( aGroup->GetDesignBlockLibId() ).c_str() );

    wxArrayString memberIds;

    for( EDA_ITEM* member : aGroup->GetItems() )
        memberIds.Add( member->m_Uuid.AsString() );

    memberIds.Sort();

    m_out->Print( "(members" );

    for( const wxString& memberId : memberIds )
        m_out->Print( " %s", m_out->Quotew( memberId ).c_str() );

    m_out->Print( ")" ); // Close `members` token.
    m_out->Print( ")" ); // Close `group` token.
}


void SCH_WRITER_V10::saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aInstances )
{
    if( aInstances.size() )
    {
        m_out->Print( "(sheet_instances" );

        for( const SCH_SHEET_INSTANCE& instance : aInstances )
        {
            wxString path = instance.m_Path.AsString();

            if( path.IsEmpty() )
                path = wxT( "/" ); // Root path

            m_out->Print( "(path %s (page %s))", m_out->Quotew( path ).c_str(),
                          m_out->Quotew( instance.m_PageNumber ).c_str() );
        }

        m_out->Print( ")" ); // Close sheet instances token.
    }
}
