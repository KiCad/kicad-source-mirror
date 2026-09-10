/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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

// Cache and KiCad library coordinates both use Y down. The file writer applies the axis flip.

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/string.h>
#include <wx/translation.h>

#include <base_units.h>
#include <eda_text.h>
#include <embedded_files.h>
#include <font/font.h>
#include <geometry/shape_compound.h>
#include <layer_ids.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <math/util.h>
#include <pin_type.h>
#include <paths.h>
#include <sch_field.h>
#include <sch_no_connect.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <stroke_params.h>
#include <string_utils.h>
#include <symbol.h>
#include <template_fieldnames.h>

#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_stream.h>


static std::string normalizedPath( std::string aPath )
{
    std::transform( aPath.begin(), aPath.end(), aPath.begin(),
                    []( unsigned char c )
                    {
                        return c == '\\' ? '/' : static_cast<char>( std::tolower( c ) );
                    } );
    return aPath;
}


static bool propertyNamesEqual( const std::string& aLeft, const std::string& aRight )
{
    return aLeft.size() == aRight.size()
           && std::equal( aLeft.begin(), aLeft.end(), aRight.begin(),
                          []( unsigned char a, unsigned char b )
                          {
                              return std::tolower( a ) == std::tolower( b );
                          } );
}


static bool isInstalledPropertyName( const std::string& aName )
{
    std::string normalized;

    for( unsigned char character : aName )
    {
        if( std::isalnum( character ) )
            normalized.push_back( static_cast<char>( std::tolower( character ) ) );
    }

    return normalized == "installedcomponent" || normalized == "installedcomponents"
           || normalized == "installedproperty";
}


int OrcadDisplayType( const ORCAD_DISPLAY_PROP& aProp )
{
    return ( aProp.dispMode >> 8 ) & 0xFF;
}


bool OrcadDisplayPropVisible( const ORCAD_DISPLAY_PROP& aProp )
{
    // Capture renders simulation results through mutually exclusive UI layers.
    // Their display mode describes text within that layer, not persistent visibility.
    if( propertyNamesEqual( aProp.name, "BiasValue Power" )
        || propertyNamesEqual( aProp.name, "BiasValue Current" )
        || propertyNamesEqual( aProp.name, "BiasValue Voltage" ) )
    {
        return false;
    }

    int type = OrcadDisplayType( aProp );
    return type >= 1 && type <= 4;
}


bool OrcadDisplayPropShowsName( const ORCAD_DISPLAY_PROP& aProp )
{
    int type = OrcadDisplayType( aProp );
    return type == 2 || type == 3;
}


bool OrcadDisplayPropShowsValue( const ORCAD_DISPLAY_PROP& aProp )
{
    int type = OrcadDisplayType( aProp );
    return type == 1 || type == 2 || type == 4;
}


int OrcadDisplayFontId( const ORCAD_DISPLAY_PROP& aProp )
{
    if( aProp.fontIdx > 0 )
        return aProp.fontIdx;

    if( aProp.name == "Part Reference" || aProp.name == "Reference" )
        return 2;

    if( aProp.name == "Value" )
        return 9;

    return 10;
}


wxString OrcadPinNameMarkup( const wxString& aName )
{
    wxString result;
    wxString overbar;
    bool     marked = false;

    auto flushOverbar = [&]()
    {
        if( !overbar.IsEmpty() )
        {
            result += wxS( "~{" ) + overbar + wxS( "}" );
            overbar.clear();
        }
    };

    for( wxUniChar character : aName )
    {
        if( character == '\\' )
        {
            marked = true;
            continue;
        }

        if( marked && character != ' ' && character != '\t' && character != '\r' && character != '\n' )
        {
            overbar += character;
        }
        else
        {
            flushOverbar();
            result += character;
        }

        marked = false;
    }

    flushOverbar();
    return result;
}


LINE_STYLE OrcadLineStyle( int aStyle )
{
    switch( aStyle )
    {
    case 0: return LINE_STYLE::SOLID;
    case 1: return LINE_STYLE::DASH;
    case 2: return LINE_STYLE::DOT;
    case 3: return LINE_STYLE::DASHDOT;
    case 4: return LINE_STYLE::DASHDOTDOT;
    case 5: return LINE_STYLE::DEFAULT;
    default: return LINE_STYLE::DEFAULT;
    }
}


std::pair<double, double> OrcadDashRatios( int aFormatVersionMajor )
{
    if( aFormatVersionMajor < 3 )
        return { 67.0, 21.0 };

    return { 3.0, 1.0 };
}


int OrcadLineWidthIu( int aWidth )
{
    switch( aWidth )
    {
    case 0: return schIUScale.MilsToIU( 10 );
    case 1: return schIUScale.MilsToIU( 30 );
    case 2: return schIUScale.MilsToIU( 50 );
    case 3: return schIUScale.MilsToIU( 10 );
    default: return 0;
    }
}


int OrcadPageGraphicLineWidthIu( int aWidth )
{
    return aWidth == 0 ? schIUScale.MilsToIU( 5 ) : OrcadLineWidthIu( aWidth );
}


FILL_T OrcadFillType( int aFillStyle, int aHatchStyle )
{
    if( aFillStyle == 0 )
        return FILL_T::FILLED_SHAPE;

    if( aFillStyle == 2 )
    {
        switch( aHatchStyle )
        {
        case 3: return FILL_T::REVERSE_HATCH;
        case 4:
        case 5: return FILL_T::CROSS_HATCH;
        default: return FILL_T::HATCH;
        }
    }

    return FILL_T::NO_FILL;
}


int OrcadHatchPitchIu( uint32_t aModifyTimestamp )
{
    constexpr uint32_t currentHatchEpoch = 1577836800;
    return schIUScale.MilsToIU( aModifyTimestamp >= currentHatchEpoch ? 80 : 25 );
}


int OrcadHatchLineWidthIu( uint32_t aModifyTimestamp )
{
    constexpr uint32_t currentHatchEpoch = 1577836800;
    return schIUScale.MilsToIU( aModifyTimestamp >= currentHatchEpoch ? 10 : 3 );
}


std::vector<SEG> OrcadHatchLines( const EDA_SHAPE& aShape, int aHatchStyle, int aPitch )
{
    EDA_SHAPE shape( aShape );
    shape.SetFillMode( FILL_T::FILLED_SHAPE );

    SHAPE_POLY_SET polygon;
    shape.TransformShapeToPolygon( polygon, 0, schIUScale.MilsToIU( 1 ), ERROR_INSIDE, true );

    std::vector<SEG> lines;

    auto append = [&]( const std::vector<double>& aSlopes, bool aVertical )
    {
        SHAPE_POLY_SET hatchArea = polygon.CloneDropTriangulation();

        if( aVertical )
            hatchArea.Rotate( ANGLE_90 );

        std::vector<SEG> generated = hatchArea.GenerateHatchLines( aSlopes, aPitch, -1 );

        if( aVertical )
        {
            for( SEG& line : generated )
            {
                RotatePoint( line.A, -ANGLE_90 );
                RotatePoint( line.B, -ANGLE_90 );
            }
        }

        lines.insert( lines.end(), generated.begin(), generated.end() );
    };

    switch( aHatchStyle )
    {
    case 0: append( { 0.0 }, false ); break;
    case 1: append( { 0.0 }, true ); break;
    case 2: append( { 1.0 }, false ); break;
    case 3: append( { -1.0 }, false ); break;
    case 4:
        append( { 0.0 }, false );
        append( { 0.0 }, true );
        break;
    case 5: append( { -1.0, 1.0 }, false ); break;
    default: append( { -1.0 }, false ); break;
    }

    return lines;
}


int OrcadTextBaselineOffset( int aTextSize )
{
    return KiROUND( aTextSize * 8.0 / 21.0 );
}


namespace
{

int schMm( double aMm )
{
    return schIUScale.mmToIU( aMm );
}


int dbuIu( double aDbu )
{
    return KiROUND( aDbu * ORCAD_IU_PER_DBU );
}


STROKE_PARAMS strokeFor( const ORCAD_PRIMITIVE& aPrimitive, int aColor )
{
    return STROKE_PARAMS( OrcadLineWidthIu( aPrimitive.lineWidth ), OrcadLineStyle( aPrimitive.lineStyle ),
                          OrcadColor( aColor ) );
}


bool laterRectangleOccludesTextUnderscores( const std::vector<ORCAD_PRIMITIVE>& aPrimitives,
                                            size_t aTextIndex )
{
    const ORCAD_PRIMITIVE& text = aPrimitives[aTextIndex];

    if( text.kind != ORCAD_PRIM_KIND::TEXT || text.text.find( '_' ) == std::string::npos )
        return false;

    int textLeft = std::min( text.x1, text.x2 );
    int textRight = std::max( text.x1, text.x2 );
    int textTop = std::min( text.y1, text.y2 );
    int textBottom = std::max( text.y1, text.y2 );

    for( size_t i = aTextIndex + 1; i < aPrimitives.size(); ++i )
    {
        const ORCAD_PRIMITIVE& rectangle = aPrimitives[i];

        if( rectangle.kind != ORCAD_PRIM_KIND::RECTANGLE )
            continue;

        int rectangleLeft = std::min( rectangle.x1, rectangle.x2 );
        int rectangleRight = std::max( rectangle.x1, rectangle.x2 );
        int rectangleTop = std::min( rectangle.y1, rectangle.y2 );
        int rectangleBottom = std::max( rectangle.y1, rectangle.y2 );
        int halfStroke = KiROUND( static_cast<double>( OrcadLineWidthIu( rectangle.lineWidth ) )
                                  / ORCAD_IU_PER_DBU / 2.0 );

        if( textLeft >= rectangleLeft && textRight <= rectangleRight && textTop >= rectangleTop
            && textBottom <= rectangleBottom && rectangleBottom - textBottom <= halfStroke )
        {
            return true;
        }
    }

    return false;
}


ELECTRICAL_PINTYPE pinTypeFor( ORCAD_PORT_TYPE aType )
{
    switch( aType )
    {
    case ORCAD_PORT_TYPE::INPUT_TYPE: return ELECTRICAL_PINTYPE::PT_INPUT;
    case ORCAD_PORT_TYPE::BIDIRECTIONAL: return ELECTRICAL_PINTYPE::PT_BIDI;
    case ORCAD_PORT_TYPE::OUTPUT: return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case ORCAD_PORT_TYPE::OPEN_COLLECTOR: return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case ORCAD_PORT_TYPE::PASSIVE: return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case ORCAD_PORT_TYPE::TRI_STATE: return ELECTRICAL_PINTYPE::PT_TRISTATE;
    case ORCAD_PORT_TYPE::OPEN_EMITTER: return ELECTRICAL_PINTYPE::PT_OPENEMITTER;
    case ORCAD_PORT_TYPE::POWER_IN: return ELECTRICAL_PINTYPE::PT_POWER_IN;
    }

    return ELECTRICAL_PINTYPE::PT_PASSIVE;
}


// OrCAD bookkeeping props kept out of user fields; useful ones map to dedicated
// KiCad fields, rest describe library linkage import re-establishes itself.
bool isBookkeepingProp( const std::string& aName )
{
    static const char* const skipped[] = {
        "Part Reference",      "Reference",           "Name",           "Graphic",        "Implementation",
        "Implementation Type", "Source Library",      "Source Package", "Source Part",
    };

    for( const char* name : skipped )
    {
        if( propertyNamesEqual( aName, name ) )
            return true;
    }

    return false;
}


size_t symbolPinIndex( const ORCAD_SYMBOL_DEF& aSymbol, const ORCAD_PIN_INST& aPin, size_t aFallback )
{
    if( aPin.pinIndex == 0 )
        return aFallback;

    int position = std::abs( static_cast<int>( aPin.pinIndex ) ) - 1;

    for( size_t i = 0; i < aSymbol.pins.size(); ++i )
    {
        if( aSymbol.pins[i].position == position )
            return i;
    }

    return static_cast<size_t>( position );
}


bool symbolPinsMatch( const ORCAD_SYMBOL_DEF& aSymbol, const ORCAD_PLACED_INSTANCE& aInstance )
{
    if( aInstance.pins.size() > aSymbol.pins.size() )
        return false;

    ORCAD_BBOX box = aSymbol.bbox.value_or( ORCAD_BBOX() );
    int        orient = OrcadOrientOf( aInstance.rotation, aInstance.mirror );

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        size_t symbolPin = symbolPinIndex( aSymbol, aInstance.pins[i], i );

        if( symbolPin >= aSymbol.pins.size() )
            return false;

        VECTOR2I point = OrcadTransformPoint( orient, box.x2 - box.x1, box.y2 - box.y1, aInstance.x, aInstance.y,
                                              aSymbol.pins[symbolPin].hotptX, aSymbol.pins[symbolPin].hotptY );

        if( point.x != aInstance.pins[i].x || point.y != aInstance.pins[i].y )
            return false;
    }

    return true;
}


bool sequentialSymbolPinsMatch( const ORCAD_SYMBOL_DEF& aSymbol, const ORCAD_PLACED_INSTANCE& aInstance )
{
    if( aInstance.pins.size() != aSymbol.pins.size() )
        return false;

    ORCAD_BBOX box = aSymbol.bbox.value_or( ORCAD_BBOX() );
    int        orient = OrcadOrientOf( aInstance.rotation, aInstance.mirror );

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        VECTOR2I point = OrcadTransformPoint( orient, box.x2 - box.x1, box.y2 - box.y1, aInstance.x, aInstance.y,
                                              aSymbol.pins[i].hotptX, aSymbol.pins[i].hotptY );

        if( point.x != aInstance.pins[i].x || point.y != aInstance.pins[i].y )
            return false;
    }

    return true;
}


bool symbolGeometryMatches( const ORCAD_SYMBOL_DEF& aLeft, const ORCAD_SYMBOL_DEF& aRight )
{
    if( aLeft.bbox.has_value() != aRight.bbox.has_value() || aLeft.pins.size() != aRight.pins.size() )
        return false;

    if( aLeft.bbox
        && ( aLeft.bbox->x1 != aRight.bbox->x1 || aLeft.bbox->y1 != aRight.bbox->y1 || aLeft.bbox->x2 != aRight.bbox->x2
             || aLeft.bbox->y2 != aRight.bbox->y2 ) )
    {
        return false;
    }

    for( size_t i = 0; i < aLeft.pins.size(); ++i )
    {
        const ORCAD_SYMBOL_PIN& left = aLeft.pins[i];
        const ORCAD_SYMBOL_PIN& right = aRight.pins[i];

        if( left.startX != right.startX || left.startY != right.startY || left.hotptX != right.hotptX
            || left.hotptY != right.hotptY )
        {
            return false;
        }
    }

    return true;
}


std::optional<int64_t> symbolPinDisplacement( const ORCAD_SYMBOL_DEF& aSymbol, const ORCAD_PLACED_INSTANCE& aInstance )
{
    if( aInstance.pins.size() > aSymbol.pins.size() )
        return std::nullopt;

    ORCAD_BBOX box = aSymbol.bbox.value_or( ORCAD_BBOX() );
    int        orient = OrcadOrientOf( aInstance.rotation, aInstance.mirror );
    int64_t    displacement = 0;

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        size_t symbolPin = symbolPinIndex( aSymbol, aInstance.pins[i], i );

        if( symbolPin >= aSymbol.pins.size() )
            return std::nullopt;

        VECTOR2I point = OrcadTransformPoint( orient, box.x2 - box.x1, box.y2 - box.y1, aInstance.x, aInstance.y,
                                              aSymbol.pins[symbolPin].hotptX, aSymbol.pins[symbolPin].hotptY );
        displacement += std::abs( static_cast<int64_t>( point.x ) - aInstance.pins[i].x );
        displacement += std::abs( static_cast<int64_t>( point.y ) - aInstance.pins[i].y );
    }

    return displacement;
}


ORCAD_SYMBOL_DEF symbolVariantForPlacedPins( const ORCAD_SYMBOL_DEF& aSymbol, const ORCAD_PLACED_INSTANCE& aInstance )
{
    ORCAD_SYMBOL_DEF fitted = aSymbol;
    fitted.variants.clear();
    fitted.synthesized = true;

    if( aInstance.pins.size() > fitted.pins.size() )
        return fitted;

    ORCAD_BBOX                            box = fitted.bbox.value_or( ORCAD_BBOX() );
    int                                   orient = OrcadOrientOf( aInstance.rotation, aInstance.mirror );
    const ORCAD_ORIENT_ENTRY&             transform = ORCAD_ORIENT_TABLE[orient];
    VECTOR2I                              offset = OrcadOrientOffset( orient, box.x2 - box.x1, box.y2 - box.y1 );
    int                                   determinant = transform.a * transform.d - transform.b * transform.c;
    std::map<std::pair<int, int>, size_t> targetCounts;
    std::map<std::pair<int, int>, std::set<size_t>> targetPins;

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        size_t symbolPin = symbolPinIndex( fitted, aInstance.pins[i], i );

        if( symbolPin >= fitted.pins.size() )
            continue;

        int x = aInstance.pins[i].x - aInstance.x - offset.x;
        int y = aInstance.pins[i].y - aInstance.y - offset.y;
        int hotptX = ( transform.d * x - transform.b * y ) / determinant;
        int hotptY = ( -transform.c * x + transform.a * y ) / determinant;
        ++targetCounts[{ hotptX, hotptY }];
        targetPins[{ hotptX, hotptY }].insert( symbolPin );
    }

    std::set<std::pair<int, int>> handledStackedTargets;

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        size_t symbolPin = symbolPinIndex( fitted, aInstance.pins[i], i );

        if( symbolPin >= fitted.pins.size() )
            continue;

        int                     x = aInstance.pins[i].x - aInstance.x - offset.x;
        int                     y = aInstance.pins[i].y - aInstance.y - offset.y;
        int                     hotptX = ( transform.d * x - transform.b * y ) / determinant;
        int                     hotptY = ( -transform.c * x + transform.a * y ) / determinant;
        auto                    target = std::pair{ hotptX, hotptY };
        const std::set<size_t>& stackedPins = targetPins.at( target );
        bool                    stackedTarget = targetCounts.at( target ) > 1;

        auto occupant = std::find_if( fitted.pins.begin(), fitted.pins.end(),
                                      [&]( const ORCAD_SYMBOL_PIN& aPin )
                                      {
                                          return aPin.hotptX == hotptX && aPin.hotptY == hotptY;
                                      } );

        if( stackedTarget && !handledStackedTargets.count( target ) )
        {
            occupant = std::find_if( fitted.pins.begin(), fitted.pins.end(),
                                     [&]( const ORCAD_SYMBOL_PIN& aPin )
                                     {
                                         size_t index = static_cast<size_t>( &aPin - fitted.pins.data() );
                                         return aPin.hotptX == hotptX && aPin.hotptY == hotptY
                                                && !stackedPins.count( index );
                                     } );
            handledStackedTargets.insert( target );
        }
        else if( stackedTarget )
        {
            occupant = fitted.pins.end();
        }

        if( occupant != fitted.pins.end() && occupant != fitted.pins.begin() + symbolPin )
        {
            std::swap( occupant->hotptX, fitted.pins[symbolPin].hotptX );
            std::swap( occupant->hotptY, fitted.pins[symbolPin].hotptY );
            std::swap( occupant->startX, fitted.pins[symbolPin].startX );
            std::swap( occupant->startY, fitted.pins[symbolPin].startY );
            continue;
        }

        int deltaX = hotptX - fitted.pins[symbolPin].hotptX;
        int deltaY = hotptY - fitted.pins[symbolPin].hotptY;

        fitted.pins[symbolPin].hotptX = hotptX;
        fitted.pins[symbolPin].hotptY = hotptY;
        fitted.pins[symbolPin].startX += deltaX;
        fitted.pins[symbolPin].startY += deltaY;
    }

    return fitted;
}

} // namespace


void ORCAD_CONVERTER::prepareSymbols()
{
    // Page symbols absent from design cache get synthesized placeholder so T0x10
    // connection points stay electrically intact.
    std::vector<std::string>                                         missingOrder;
    std::map<std::string, std::vector<const ORCAD_PLACED_INSTANCE*>> missing;

    // Scan root and child-folder pages; child-folder parts live in childFolderPages,
    // not root page list.
    std::vector<const ORCAD_RAW_PAGE*> allPages;

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
        allPages.push_back( &page );

    for( const auto& [folder, pages] : m_design.childFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            allPages.push_back( &page );
    }

    for( const auto& [folder, pages] : m_design.unreferencedFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            allPages.push_back( &page );
    }

    for( const ORCAD_RAW_PAGE* page : allPages )
    {
        for( const ORCAD_PLACED_INSTANCE& inst : page->instances )
        {
            if( inst.pkgName.empty() || m_design.symbols.count( inst.pkgName ) )
                continue;

            auto [it, isNew] = missing.try_emplace( inst.pkgName );

            if( isNew )
                missingOrder.push_back( inst.pkgName );

            it->second.push_back( &inst );
        }
    }

    for( const std::string& name : missingOrder )
    {
        const std::vector<const ORCAD_PLACED_INSTANCE*>& insts = missing[name];

        note( wxString::Format( _( "Symbol '%s' is absent from the design cache; synthesized "
                                   "placeholder from %d instance(s)." ),
                                FromOrcadString( name ), (int) insts.size() ) );

        m_design.symbols[name] = synthesizeSymbol( name, insts );
    }

    for( const ORCAD_RAW_PAGE* page : allPages )
    {
        for( const ORCAD_PLACED_INSTANCE& inst : page->instances )
        {
            auto symbol = m_design.symbols.find( inst.pkgName );

            if( symbol == m_design.symbols.end() || inst.pins.empty() )
                continue;

            std::vector<const ORCAD_SYMBOL_DEF*> candidates = { &symbol->second };

            for( const ORCAD_SYMBOL_DEF& variant : symbol->second.variants )
            {
                if( !variant.synthesized )
                    candidates.push_back( &variant );
            }

            for( const ORCAD_SYMBOL_DEF& variant : symbol->second.variants )
            {
                if( variant.synthesized )
                    candidates.push_back( &variant );
            }

            const ORCAD_SYMBOL_DEF* matching = nullptr;

            for( const ORCAD_SYMBOL_DEF* candidate : candidates )
            {
                if( symbolPinsMatch( *candidate, inst ) )
                {
                    matching = candidate;
                    break;
                }
            }

            if( !matching )
            {
                for( const ORCAD_SYMBOL_DEF* candidate : candidates )
                {
                    if( !sequentialSymbolPinsMatch( *candidate, inst ) )
                        continue;

                    ORCAD_SYMBOL_DEF fitted = *candidate;
                    fitted.variants.clear();
                    fitted.synthesized = true;

                    for( size_t i = 0; i < inst.pins.size(); ++i )
                    {
                        if( inst.pins[i].pinIndex )
                            fitted.pins[i].position = std::abs( static_cast<int>( inst.pins[i].pinIndex ) ) - 1;
                    }

                    if( symbolPinsMatch( fitted, inst ) )
                    {
                        symbol->second.variants.insert( symbol->second.variants.begin(), std::move( fitted ) );
                        matching = &symbol->second.variants.front();
                    }

                    break;
                }
            }

            if( matching )
            {
                auto sourceProperty = inst.props.find( "Source Library" );
                std::string sourceLibrary = sourceProperty != inst.props.end() ? sourceProperty->second
                                                                              : inst.sourceLibrary;

                bool sourceMatchedGraphics = !sourceLibrary.empty() && !matching->sourceLib.empty()
                                             && normalizedPath( sourceLibrary ) == normalizedPath( matching->sourceLib );

                if( matching != &symbol->second && !sourceMatchedGraphics
                    && std::none_of( symbol->second.variants.begin(), symbol->second.variants.end(),
                                     [&]( const ORCAD_SYMBOL_DEF& aVariant )
                                     {
                                         return aVariant.synthesized && symbolGeometryMatches( aVariant, *matching );
                                     } ) )
                {
                    ORCAD_SYMBOL_DEF fitted = *matching;
                    fitted.primitives = symbol->second.primitives;
                    fitted.color = symbol->second.color;
                    fitted.props = symbol->second.props;
                    fitted.generalFlags = symbol->second.generalFlags;
                    fitted.variants.clear();
                    fitted.synthesized = true;

                    if( symbolPinsMatch( fitted, inst ) )
                        symbol->second.variants.insert( symbol->second.variants.begin(), std::move( fitted ) );
                }

                continue;
            }

            const ORCAD_SYMBOL_DEF* best = nullptr;
            int64_t                 bestDisplacement = std::numeric_limits<int64_t>::max();

            for( const ORCAD_SYMBOL_DEF* candidate : candidates )
            {
                std::optional<int64_t> displacement = symbolPinDisplacement( *candidate, inst );

                if( displacement && *displacement < bestDisplacement )
                {
                    best = candidate;
                    bestDisplacement = *displacement;
                }
            }

            if( best )
            {
                ORCAD_SYMBOL_DEF fitted = symbolVariantForPlacedPins( *best, inst );

                if( symbolPinsMatch( fitted, inst ) )
                    symbol->second.variants.push_back( std::move( fitted ) );
            }
        }
    }

    for( const ORCAD_RAW_PAGE* page : allPages )
    {
        for( const ORCAD_PLACED_INSTANCE& inst : page->instances )
            libForInstance( inst );
    }

    m_preparedPkgToLib = m_pkgToLib;

    for( const auto& [name, entry] : m_libSymbols )
        m_preparedLibUnits.emplace( name, entry.units );

    computeFontBaseline();
}


std::vector<LIB_SYMBOL*> ORCAD_CONVERTER::BuildSymbolLibrary()
{
    // .OLB has same Library/Cache/Packages streams but no pages, so lib entries built
    // directly from cache defs (grouped by base name, Normal view) w/ package data.
    auto stripView = []( const std::string& aName ) -> std::string
    {
        size_t dot = aName.rfind( '.' );

        if( dot != std::string::npos )
        {
            std::string view = aName.substr( dot + 1 );

            if( view == "Normal" || view == "Convert" )
                return aName.substr( 0, dot );
        }

        return aName;
    };

    for( auto& [cacheName, def] : m_design.symbols )
    {
        // Only drawable library parts and power symbols become library items
        if( def.typeId != ORCAD_ST_LIBRARY_PART && def.typeId != ORCAD_ST_GLOBAL_SYMBOL )
            continue;

        bool        convertView = cacheName.size() > 8 && cacheName.compare( cacheName.size() - 8, 8, ".Convert" ) == 0;
        std::string base = stripView( cacheName );
        std::string libname = SymbolId( base );

        LIB_ENTRY& ls = m_libSymbols[libname];

        // First (Normal) view wins; .Convert is DeMorgan alternate sharing pin map,
        // not separate item.
        if( !ls.name.empty() )
            continue;

        if( convertView && m_design.symbols.count( base + ".Normal" ) )
            continue;

        ls.name = libname;

        std::string          bare = base.substr( 0, base.find( '.' ) );
        const ORCAD_PACKAGE* pkg = nullptr;

        for( const std::string& key : { base, bare } )
        {
            auto pkgIt = m_design.packages.find( key );

            if( pkgIt != m_design.packages.end() )
            {
                pkg = &pkgIt->second;
                break;
            }
        }

        ls.isPower = def.typeId == ORCAD_ST_GLOBAL_SYMBOL;
        ls.refPrefix = ( pkg && !pkg->refDes.empty() ) ? pkg->refDes : ( ls.isPower ? "#PWR" : "U" );
        ls.footprint = pkg ? pkg->pcbFootprint : "";

        if( ls.isPower )
            ls.powerNet = base; // power value = net name (symbol base)

        std::vector<std::string> pinNumbers;
        std::vector<bool>        pinNumberVisible;
        std::vector<bool>        pinIgnore;

        if( pkg && !pkg->devices.empty() )
        {
            pinNumbers = pkg->devices.front().pinNumbers;
            pinNumberVisible.reserve( pinNumbers.size() );

            for( const std::string& number : pinNumbers )
                pinNumberVisible.push_back( !number.empty() );

            pinIgnore = pkg->devices.front().pinIgnore;
        }

        ls.units.push_back( { "A", &def, pinNumbers, pinNumberVisible, pinIgnore, {}, {} } );
    }

    computeFontBaseline();

    std::vector<LIB_SYMBOL*> out;

    for( auto& [libname, entry] : m_libSymbols )
    {
        // Single malformed cache symbol must not sink whole library, but dropping one without
        // saying so leaves a library that looks complete and is not.
        try
        {
            if( LIB_SYMBOL* symbol = kicadSymbolFor( libname ) )
                out.push_back( static_cast<LIB_SYMBOL*>( symbol->Duplicate() ) );
        }
        catch( const std::exception& e )
        {
            warn( wxString::Format( _( "The symbol '%s' could not be converted and was left out of the "
                                       "library (%s)." ),
                                    FromOrcadString( libname ), wxString::FromUTF8( e.what() ) ) );
        }
    }

    return out;
}


ORCAD_SYMBOL_DEF
ORCAD_CONVERTER::synthesizeSymbol( const std::string&                               aPkgName,
                                   const std::vector<const ORCAD_PLACED_INSTANCE*>& aInstances ) const
{
    // Prefer reference-orientation instance: t-term in T = I + t(w,h) + M*p cancels
    // there, making reconstructed connection points exact.
    const ORCAD_PLACED_INSTANCE* ref = aInstances.front();

    for( const ORCAD_PLACED_INSTANCE* inst : aInstances )
    {
        if( OrcadOrientOf( inst->rotation, inst->mirror ) == 0 )
        {
            ref = inst;
            break;
        }
    }

    int                       ori = OrcadOrientOf( ref->rotation, ref->mirror );
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[ori];
    int                       det = e.a * e.d - e.b * e.c; // always +/-1

    auto inv = [&]( int aX, int aY ) -> ORCAD_POINT
    {
        return { ( e.d * aX - e.b * aY ) / det, ( -e.c * aX + e.a * aY ) / det };
    };

    // Instance-local pin positions from T0x10 records.
    std::vector<ORCAD_POINT> r;

    for( const ORCAD_PIN_INST& t : ref->pins )
        r.push_back( inv( t.x - ref->x, t.y - ref->y ) );

    std::optional<int> inferredWidth;
    std::optional<int> inferredHeight;

    if( r.size() == 1 && ori == 0 )
    {
        std::map<int, int> widthCounts;
        std::map<int, int> heightCounts;

        for( const ORCAD_PLACED_INSTANCE* inst : aInstances )
        {
            if( inst->pins.size() != 1 )
                continue;

            int                       instOrient = OrcadOrientOf( inst->rotation, inst->mirror );
            const ORCAD_ORIENT_ENTRY& transform = ORCAD_ORIENT_TABLE[instOrient];
            int localX = transform.a * r.front().x + transform.b * r.front().y;
            int localY = transform.c * r.front().x + transform.d * r.front().y;
            int offsetX = inst->pins.front().x - inst->x - localX;
            int offsetY = inst->pins.front().y - inst->y - localY;

            auto tally = [&]( int aSelector, int aOffset )
            {
                if( aOffset <= 0 )
                    return;

                if( aSelector == 1 )
                    ++widthCounts[aOffset];
                else if( aSelector == 2 )
                    ++heightCounts[aOffset];
            };

            tally( transform.txSel, offsetX );
            tally( transform.tySel, offsetY );
        }

        auto mostFrequent = []( const std::map<int, int>& aCounts ) -> std::optional<int>
        {
            auto best = std::max_element( aCounts.begin(), aCounts.end(),
                                          []( const auto& aLeft, const auto& aRight )
                                          {
                                              return aLeft.second < aRight.second;
                                          } );
            return best == aCounts.end() ? std::nullopt : std::optional<int>( best->first );
        };

        inferredWidth = mostFrequent( widthCounts );
        inferredHeight = mostFrequent( heightCounts );
    }

    std::vector<char> sides; // 'L', 'R', 'T', 'B' or 0 per pin
    int               bx1 = 0;
    int               by1 = 0;
    int               bx2 = 0;
    int               by2 = 0;

    if( !r.empty() )
    {
        int x0 = r.front().x;
        int x1 = r.front().x;
        int y0 = r.front().y;
        int y1 = r.front().y;

        for( const ORCAD_POINT& p : r )
        {
            x0 = std::min( x0, p.x );
            x1 = std::max( x1, p.x );
            y0 = std::min( y0, p.y );
            y1 = std::max( y1, p.y );
        }

        bool horizFirst = ( x1 - x0 ) >= ( y1 - y0 );

        for( const ORCAD_POINT& p : r )
        {
            std::vector<char> cand;

            if( p.x == x0 )
                cand.push_back( 'L' );

            if( p.x == x1 )
                cand.push_back( 'R' );

            if( p.y == y0 )
                cand.push_back( 'T' );

            if( p.y == y1 )
                cand.push_back( 'B' );

            if( cand.empty() )
            {
                sides.push_back( 0 );
                continue;
            }

            // Prefer L/R sides when pin field wider than tall, T/B otherwise; ties
            // keep L,R,T,B order.
            char pick = cand.front();

            for( char c : cand )
            {
                bool isTB = c == 'T' || c == 'B';

                if( isTB != horizFirst )
                {
                    pick = c;
                    break;
                }
            }

            sides.push_back( pick );
        }

        if( r.size() == 1 && inferredWidth && inferredHeight )
        {
            bx1 = 0;
            by1 = 0;
            bx2 = *inferredWidth;
            by2 = *inferredHeight;
        }
        else
        {
            auto hasSide = [&]( char aSide )
            {
                return std::find( sides.begin(), sides.end(), aSide ) != sides.end();
            };

            bx1 = hasSide( 'L' ) ? x0 + PIN_LEN_DBU : x0;
            bx2 = hasSide( 'R' ) ? x1 - PIN_LEN_DBU : x1;
            by1 = hasSide( 'T' ) ? y0 + PIN_LEN_DBU : y0;
            by2 = hasSide( 'B' ) ? y1 - PIN_LEN_DBU : y1;

            if( bx2 <= bx1 )
            {
                int m = (int) std::floor( ( bx1 + bx2 ) / 2.0 );
                bx1 = m - PIN_LEN_DBU;
                bx2 = m + PIN_LEN_DBU;
            }

            if( by2 <= by1 )
            {
                int m = (int) std::floor( ( by1 + by2 ) / 2.0 );
                by1 = m - PIN_LEN_DBU;
                by2 = m + PIN_LEN_DBU;
            }
        }
    }
    else
    {
        // No pin records; derive body from inverse-transformed placed box.
        ORCAD_POINT c1 = inv( ref->bbox.x1 - ref->x, ref->bbox.y1 - ref->y );
        ORCAD_POINT c2 = inv( ref->bbox.x2 - ref->x, ref->bbox.y2 - ref->y );

        bx1 = std::min( c1.x, c2.x );
        bx2 = std::max( c1.x, c2.x );
        by1 = std::min( c1.y, c2.y );
        by2 = std::max( c1.y, c2.y );
    }

    // Remove orientation re-anchoring offset so definition anchors like real cache
    // symbol.
    int         w = bx2 - bx1;
    int         h = by2 - by1;
    VECTOR2I    t = OrcadOrientOffset( ori, w, h );
    ORCAD_POINT s = inv( t.x, t.y );

    ORCAD_SYMBOL_DEF sym;
    sym.typeId = ORCAD_ST_LIBRARY_PART;
    sym.name = aPkgName;
    sym.synthesized = true;
    sym.bbox = ORCAD_BBOX{ bx1 - s.x, by1 - s.y, bx2 - s.x, by2 - s.y };

    ORCAD_PRIMITIVE rect;
    rect.kind = ORCAD_PRIM_KIND::RECTANGLE;
    rect.x1 = bx1 - s.x;
    rect.y1 = by1 - s.y;
    rect.x2 = bx2 - s.x;
    rect.y2 = by2 - s.y;
    sym.primitives.push_back( rect );

    auto hasDualRowDescription = []( const std::map<std::string, std::string>& aProps )
    {
        return std::any_of( aProps.begin(), aProps.end(),
                            []( const auto& aProperty )
                            {
                                std::string value = aProperty.second;
                                std::transform( value.begin(), value.end(), value.begin(),
                                                []( unsigned char c )
                                                {
                                                    return static_cast<char>( std::toupper( c ) );
                                                } );
                                return value.find( "DUAL ROW" ) != std::string::npos;
                            } );
    };
    bool dualRowConnector = hasDualRowDescription( ref->props );
    auto package = m_design.packages.find( ref->sourcePackage );

    if( package != m_design.packages.end() )
        dualRowConnector = dualRowConnector || hasDualRowDescription( package->second.props );

    for( size_t i = 0; i < r.size(); ++i )
    {
        int px = r[i].x - s.x;
        int py = r[i].y - s.y;
        int ddx = 0;
        int ddy = 0;

        switch( sides[i] )
        {
        case 'L': ddx = PIN_LEN_DBU; break;
        case 'R': ddx = -PIN_LEN_DBU; break;
        case 'T': ddy = PIN_LEN_DBU; break;
        case 'B': ddy = -PIN_LEN_DBU; break;
        default: break;
        }

        ORCAD_SYMBOL_PIN pin;

        if( dualRowConnector )
        {
            int pinIndex = std::abs( static_cast<int>( ref->pins[i].pinIndex ) );
            pin.position = pinIndex > 0 ? pinIndex - 1 : static_cast<int>( i );
            pin.name = std::to_string( pinIndex > 0 ? pinIndex : i + 1 );
        }

        pin.startX = px + ddx;
        pin.startY = py + ddy;
        pin.hotptX = px;
        pin.hotptY = py;
        pin.portType = ORCAD_PORT_TYPE::PASSIVE;
        pin.shapeBits = 0;
        sym.pins.push_back( pin );
    }

    return sym;
}


std::string ORCAD_CONVERTER::unitLetter( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    // Cache names look like "<base><unit>.<view>", e.g. "74LS00A.Normal" (unit A of
    // 74LS00) or "C.Normal" (single-unit capacitor).
    const std::string& v = aInst.pkgName;
    auto               sourceProperty = aInst.props.find( "Source Package" );
    std::string base = sourceProperty != aInst.props.end() && !sourceProperty->second.empty() ? sourceProperty->second
                       : !aInst.sourcePackage.empty()                                         ? aInst.sourcePackage
                                                      : v.substr( 0, v.find( '.' ) );

    std::string tail = ( v.compare( 0, base.size(), base ) == 0 ) ? v.substr( base.size() ) : v;

    size_t      dot = tail.find( '.' );
    std::string unit = tail.substr( 0, dot );
    std::string view = dot == std::string::npos ? std::string() : tail.substr( dot + 1 );

    if( !view.empty() )
    {
        std::string lower = view;

        std::transform( lower.begin(), lower.end(), lower.begin(),
                        []( unsigned char c )
                        {
                            return (char) std::tolower( c );
                        } );

        // Non-Normal views are DeMorgan alternates with own body graphics, so view
        // name stays part of unit discriminator.
        if( lower != "normal" )
            unit += ":" + view;
    }

    return unit;
}


std::pair<const ORCAD_SYMBOL_DEF*, int> ORCAD_CONVERTER::pickVariant( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    auto it = m_design.symbols.find( aInst.pkgName );

    if( it == m_design.symbols.end() )
        return { nullptr, 0 };

    const ORCAD_SYMBOL_DEF* prime = &it->second;

    std::vector<const ORCAD_SYMBOL_DEF*> variants;
    variants.push_back( prime );

    for( const ORCAD_SYMBOL_DEF& v : prime->variants )
        variants.push_back( &v );

    if( aInst.pins.empty() )
        return { prime, 0 };

    auto sourceProperty = aInst.props.find( "Source Library" );
    std::string sourceLibrary = sourceProperty != aInst.props.end() ? sourceProperty->second : aInst.sourceLibrary;

    if( !sourceLibrary.empty() )
    {
        std::string sourceKey = normalizedPath( sourceLibrary );

        for( size_t vi = 0; vi < variants.size(); ++vi )
        {
            if( normalizedPath( variants[vi]->sourceLib ) == sourceKey && symbolPinsMatch( *variants[vi], aInst ) )
                return { variants[vi], static_cast<int>( vi ) };
        }
    }

    for( size_t vi = 0; vi < variants.size(); ++vi )
    {
        const ORCAD_SYMBOL_DEF* sym = variants[vi];

        if( symbolPinsMatch( *sym, aInst ) )
            return { sym, (int) vi };
    }

    return { prime, 0 };
}


const ORCAD_PACKAGE* ORCAD_CONVERTER::packageFor( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    auto        packageProperty = aInst.props.find( "Source Package" );
    std::string base = packageProperty != aInst.props.end() && !packageProperty->second.empty()
                               ? packageProperty->second
                       : !aInst.sourcePackage.empty() ? aInst.sourcePackage
                                                      : aInst.pkgName.substr( 0, aInst.pkgName.find( '.' ) );

    auto it = m_design.packages.find( base );

    if( it == m_design.packages.end() )
        return nullptr;

    auto matchesInstance = [&]( const ORCAD_PACKAGE& aPackage )
    {
        for( const ORCAD_DEVICE& device : aPackage.devices )
        {
            size_t activePins = 0;

            for( size_t i = 0; i < device.pinNumbers.size(); ++i )
            {
                if( i >= device.pinIgnore.size() || !device.pinIgnore[i] )
                    ++activePins;
            }

            if( activePins == aInst.pins.size() )
                return true;
        }

        return false;
    };

    std::vector<const ORCAD_PACKAGE*> candidates = { &it->second };

    for( const ORCAD_PACKAGE& variant : it->second.variants )
        candidates.push_back( &variant );

    const ORCAD_SYMBOL_DEF* selectedSymbol = pickVariant( aInst ).first;

    auto        sourceProperty = aInst.props.find( "Source Library" );
    std::string sourceLibrary = sourceProperty != aInst.props.end() ? sourceProperty->second : std::string();

    if( sourceLibrary.empty() )
        sourceLibrary = aInst.sourceLibrary;

    if( sourceLibrary.empty() && selectedSymbol )
        sourceLibrary = selectedSymbol->sourceLib;

    auto candidateFootprint = []( const ORCAD_PACKAGE& aPackage )
    {
        std::string footprint = aPackage.pcbFootprint;
        auto        property = aPackage.props.find( "PCB Footprint" );

        if( footprint.empty() && property != aPackage.props.end() )
            footprint = property->second;

        return footprint;
    };

    auto hasNumberedPins = []( const ORCAD_PACKAGE& aPackage )
    {
        return std::any_of( aPackage.devices.begin(), aPackage.devices.end(),
                            []( const ORCAD_DEVICE& aDevice )
                            {
                                return std::any_of( aDevice.pinNumbers.begin(), aDevice.pinNumbers.end(),
                                                    []( const std::string& aNumber )
                                                    {
                                                        return !aNumber.empty();
                                                    } );
                            } );
    };

    if( !sourceLibrary.empty() )
    {
        std::string sourceKey = normalizedPath( sourceLibrary );
        bool        designLibrary = sourceKey.ends_with( ".dsn" );
        bool        numberedAlternative =
                std::any_of( candidates.begin(), candidates.end(),
                             [&]( const ORCAD_PACKAGE* aCandidate )
                             {
                                 return matchesInstance( *aCandidate ) && hasNumberedPins( *aCandidate );
                             } );

        for( const ORCAD_PACKAGE* candidate : candidates )
        {
            if( !matchesInstance( *candidate ) || normalizedPath( candidate->sourceLib ) != sourceKey )
                continue;

            if( !designLibrary && numberedAlternative && !hasNumberedPins( *candidate ) )
                continue;

            return candidate;
        }
    }

    auto        footprintProperty = aInst.props.find( "PCB Footprint" );
    std::string footprintKey =
            footprintProperty != aInst.props.end() ? normalizedPath( footprintProperty->second ) : std::string();

    if( !footprintKey.empty() )
    {
        std::vector<const ORCAD_PACKAGE*> footprintMatches;

        for( const ORCAD_PACKAGE* candidate : candidates )
        {
            if( matchesInstance( *candidate ) && normalizedPath( candidateFootprint( *candidate ) ) == footprintKey )
                footprintMatches.push_back( candidate );
        }

        if( footprintMatches.size() == 1 )
            return footprintMatches.front();
    }

    auto logicalPinMatches = [&]( const ORCAD_PACKAGE& aPackage )
    {
        if( !selectedSymbol || aInst.unitIndex >= aPackage.devices.size() )
            return 0;

        const std::vector<std::string>& numbers = aPackage.devices[aInst.unitIndex].pinNumbers;
        int                             matches = 0;

        for( const ORCAD_SYMBOL_PIN& pin : selectedSymbol->pins )
        {
            if( pin.position >= 0 && static_cast<size_t>( pin.position ) < numbers.size() && !pin.name.empty()
                && pin.name == numbers[pin.position] )
            {
                ++matches;
            }
        }

        return matches;
    };

    const ORCAD_PACKAGE* bestNumbered = nullptr;
    int                  bestLogicalMatches = -1;

    for( const ORCAD_PACKAGE* candidate : candidates )
    {
        if( !matchesInstance( *candidate ) || !hasNumberedPins( *candidate ) )
            continue;

        int logicalMatches = logicalPinMatches( *candidate );

        if( logicalMatches > bestLogicalMatches )
        {
            bestNumbered = candidate;
            bestLogicalMatches = logicalMatches;
        }
    }

    if( bestNumbered )
        return bestNumbered;

    for( const ORCAD_PACKAGE* candidate : candidates )
    {
        if( matchesInstance( *candidate ) )
            return candidate;
    }

    return &it->second;
}


std::map<std::string, std::string> ORCAD_CONVERTER::effectiveProps( const ORCAD_PLACED_INSTANCE& aInst,
                                                                    const ORCAD_SYMBOL_DEF&      aDef ) const
{
    std::map<std::string, std::string> props = aDef.props;

    auto overlay = [&]( const std::map<std::string, std::string>& aProperties, bool aAllowEmpty = false )
    {
        for( const auto& [name, value] : aProperties )
        {
            auto existing = std::find_if( props.begin(), props.end(),
                                          [&]( const auto& aProperty )
                                          { return propertyNamesEqual( aProperty.first, name ); } );

            if( existing != props.end() )
            {
                if( aAllowEmpty || !value.empty() || existing->second.empty() )
                    existing->second = value;
            }
            else
                props[name] = value;
        }
    };

    if( const ORCAD_PACKAGE* pkg = packageFor( aInst ) )
        overlay( pkg->props );

    overlay( aInst.props, true );

    if( m_currentOccProps )
    {
        auto occurrence = m_currentOccProps->find( aInst.dbId );

        if( occurrence != m_currentOccProps->end() )
            overlay( occurrence->second, true );
    }

    return props;
}


bool ORCAD_CONVERTER::hasImplicitPowerPinName( const ORCAD_PLACED_INSTANCE& aInstance, size_t aPinIndex,
                                              const std::string& aNetName ) const
{
    const ORCAD_SYMBOL_DEF* definition = pickVariant( aInstance ).first;

    if( !definition || aPinIndex >= aInstance.pins.size() )
        return false;

    size_t index = symbolPinIndex( *definition, aInstance.pins[aPinIndex], aPinIndex );

    if( index >= definition->pins.size() )
        return false;

    const ORCAD_SYMBOL_PIN& pin = definition->pins[index];

    return pin.portType == ORCAD_PORT_TYPE::POWER_IN && ( pin.shapeBits & 0x80 ) != 0
           && pin.hotptX == pin.startX && pin.hotptY == pin.startY && !pin.name.empty()
           && pin.name.compare( 0, 4, "$PIN" ) != 0 && pin.name == aNetName;
}


std::pair<std::string, int> ORCAD_CONVERTER::libForInstance( const ORCAD_PLACED_INSTANCE& aInst,
                                                           const PKG_KEY** aSourceUnit )
{
    auto [sym, vi] = pickVariant( aInst );

    auto        sourceProperty = aInst.props.find( "Source Library" );
    std::string sourceLibrary = sourceProperty != aInst.props.end() ? sourceProperty->second : std::string();

    if( sourceLibrary.empty() )
        sourceLibrary = aInst.sourceLibrary;

    if( sourceLibrary.empty() && sym )
        sourceLibrary = sym->sourceLib;

    auto        packageProperty = aInst.props.find( "Source Package" );
    std::string sourcePackage = packageProperty != aInst.props.end() && !packageProperty->second.empty()
                                        ? packageProperty->second
                                : !aInst.sourcePackage.empty() ? aInst.sourcePackage
                                                               : aInst.pkgName.substr( 0, aInst.pkgName.find( '.' ) );

    std::string srcOrPkg = sourceLibrary + "\n" + sourcePackage;

    auto footprintProperty = aInst.props.find( "PCB Footprint" );

    if( footprintProperty != aInst.props.end() )
        srcOrPkg += "\n" + footprintProperty->second;
    std::string base = sourcePackage;

    std::string letter = unitLetter( aInst );

    if( !sym )
    {
        warn( wxString::Format( _( "No cached symbol for '%s' (%s); placeholder emitted." ),
                                FromOrcadString( aInst.pkgName ), FromOrcadString( aInst.reference ) ) );

        // Register empty definition so unit references stable storage.
        ORCAD_SYMBOL_DEF& slot = m_design.symbols[aInst.pkgName];

        if( slot.name.empty() )
        {
            slot.typeId = 0;
            slot.name = aInst.pkgName;
        }

        sym = &slot;
    }

    const ORCAD_PACKAGE* pkg = packageFor( aInst );

    size_t unitIndex = aInst.unitIndex;

    if( m_currentOccUnitRefs )
    {
        auto unitRefIt = m_currentOccUnitRefs->find( aInst.dbId );

        if( unitRefIt != m_currentOccUnitRefs->end() )
            letter = unitRefIt->second;
    }

    if( letter.empty() && pkg && unitIndex < pkg->devices.size() )
        letter = pkg->devices[unitIndex].unitRef;

    if( letter.empty() && pkg && pkg->devices.size() > 1 )
    {
        std::string index = std::to_string( unitIndex );
        letter = "#" + std::string( 10 - std::min<size_t>( 10, index.size() ), '0' ) + index;
    }

    // The placed instance selects the package device.  Occurrence hierarchy can
    // reassign its displayed unit letter without changing the physical pin map.
    std::vector<std::string> pinNumbers;
    std::vector<bool>        pinIgnore;

    if( pkg )
    {
        bool foundDevice = false;

        if( unitIndex < pkg->devices.size() )
        {
            pinNumbers = pkg->devices[unitIndex].pinNumbers;
            pinIgnore = pkg->devices[unitIndex].pinIgnore;
            foundDevice = true;
        }

        std::string bare = letter.substr( 0, letter.find( ':' ) );

        if( !foundDevice )
        {
            for( const ORCAD_DEVICE& d : pkg->devices )
            {
                if( d.unitRef == bare )
                {
                    pinNumbers = d.pinNumbers;
                    pinIgnore = d.pinIgnore;
                    foundDevice = true;
                    break;
                }
            }
        }

        if( !foundDevice && !pkg->devices.empty() && bare.empty() )
        {
            pinNumbers = pkg->devices.front().pinNumbers;
            pinIgnore = pkg->devices.front().pinIgnore;
        }
    }

    std::string normalizedSourceLibrary = normalizedPath( sourceLibrary );
    std::string normalizedPackageLibrary = pkg ? normalizedPath( pkg->sourceLib ) : std::string();

    bool logicalNumbers = sym
                          && std::all_of( sym->pins.begin(), sym->pins.end(),
                                          []( const ORCAD_SYMBOL_PIN& aPin )
                                          {
                                              return !aPin.name.empty()
                                                     && std::all_of( aPin.name.begin(), aPin.name.end(),
                                                                     []( unsigned char c )
                                                                     {
                                                                         return std::isdigit( c );
                                                                     } );
                                          } );
    bool packagePinCountMatches = pkg
                                  && std::any_of( pkg->devices.begin(), pkg->devices.end(),
                                                  [&]( const ORCAD_DEVICE& aDevice )
                                                  {
                                                      size_t activePins = 0;

                                                      for( size_t i = 0; i < aDevice.pinNumbers.size(); ++i )
                                                      {
                                                          if( i >= aDevice.pinIgnore.size() || !aDevice.pinIgnore[i] )
                                                          {
                                                              ++activePins;
                                                          }
                                                      }

                                                      return activePins == aInst.pins.size();
                                                  } );
    bool stalePackageMap = logicalNumbers && pkg && pkg->devices.size() == 1
                           && !normalizedSourceLibrary.ends_with( ".dsn" )
                           && normalizedPackageLibrary != normalizedSourceLibrary;

    if( logicalNumbers && pkg && !packagePinCountMatches && normalizedPackageLibrary != normalizedSourceLibrary )
    {
        pinNumbers.resize( sym->pins.size() );
        pinIgnore.assign( sym->pins.size(), false );

        for( size_t pi = 0; pi < sym->pins.size(); ++pi )
        {
            int position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );

            if( position >= 0 && static_cast<size_t>( position ) < pinNumbers.size() )
                pinNumbers[position] = sym->pins[pi].name;
        }
    }

    bool packageUsesLogicalPinNames = vi > 0 && sym && sym->pins.size() == 2 && pinNumbers.size() == 2;

    if( packageUsesLogicalPinNames )
    {
        for( size_t pi = 0; pi < sym->pins.size(); ++pi )
        {
            int position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );

            if( position < 0 || static_cast<size_t>( position ) >= pinNumbers.size()
                || wxString::FromUTF8( pinNumbers[position] ).CmpNoCase( wxString::FromUTF8( sym->pins[pi].name ) )
                           != 0 )
            {
                packageUsesLogicalPinNames = false;
                break;
            }
        }
    }

    if( packageUsesLogicalPinNames && normalizedSourceLibrary.ends_with( ".dsn" ) )
    {
        const std::map<std::string, std::string> diodeNumbers = {
            { "a", "2" }, { "anode", "2" }, { "k", "1" }, { "cathode", "1" }
        };
        bool diodePins = true;

        for( const ORCAD_SYMBOL_PIN& pin : sym->pins )
        {
            std::string name = pin.name;
            std::transform( name.begin(), name.end(), name.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            diodePins = diodePins && diodeNumbers.count( name );
        }

        if( diodePins )
        {
            for( size_t pi = 0; pi < sym->pins.size(); ++pi )
            {
                int         position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );
                std::string name = sym->pins[pi].name;
                std::transform( name.begin(), name.end(), name.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );
                pinNumbers[position] = diodeNumbers.at( name );
            }
        }
    }

    auto hasDualRowDescription = []( const std::map<std::string, std::string>& aProps )
    {
        return std::any_of( aProps.begin(), aProps.end(),
                            []( const auto& aProperty )
                            {
                                std::string value = aProperty.second;
                                std::transform( value.begin(), value.end(), value.begin(),
                                                []( unsigned char c )
                                                {
                                                    return static_cast<char>( std::toupper( c ) );
                                                } );
                                return value.find( "DUAL ROW" ) != std::string::npos;
                            } );
    };

    bool blankPackageMap = !pinNumbers.empty()
                           && std::all_of( pinNumbers.begin(), pinNumbers.end(),
                                           []( const std::string& aNumber )
                                           {
                                               return aNumber.empty();
                                           } );
    std::vector<bool> pinNumberVisible;
    pinNumberVisible.reserve( pinNumbers.size() );

    for( const std::string& number : pinNumbers )
        pinNumberVisible.push_back( !number.empty() );

    bool dualRowConnector =
            sym && sym->synthesized && pinNumbers.size() == sym->pins.size() && pinNumbers.size() % 2 == 0
            && ( hasDualRowDescription( aInst.props ) || ( pkg && hasDualRowDescription( pkg->props ) ) );

    if( blankPackageMap && dualRowConnector )
    {
        size_t        half = pinNumbers.size() / 2;
        std::set<int> positions;

        for( size_t pi = 0; pi < sym->pins.size(); ++pi )
        {
            int position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );

            if( position >= 0 && static_cast<size_t>( position ) < pinNumbers.size() )
                positions.insert( position );
        }

        if( positions.size() == pinNumbers.size() )
        {
            for( size_t position = 0; position < pinNumbers.size(); ++position )
            {
                size_t number = position < half ? 2 * position + 1 : 2 * ( position - half + 1 );
                pinNumbers[position] = std::to_string( number );
                pinNumberVisible[position] = true;
            }
        }
    }

    if( sym && !pinNumbers.empty() )
    {
        for( size_t pi = 0; pi < sym->pins.size(); ++pi )
        {
            int position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );

            if( position >= 0 && static_cast<size_t>( position ) < pinNumbers.size() && pinNumbers[position].empty()
                && ( static_cast<size_t>( position ) >= pinIgnore.size() || !pinIgnore[position] ) )
            {
                pinNumbers[position] = sym->pins[pi].name;
            }
        }
    }

    if( !normalizedSourceLibrary.ends_with( ".dsn" ) && sym && sym->pins.size() == 2 && logicalNumbers && pkg
        && pkg->devices.size() == 1 )
    {
        for( size_t pi = 0; pi < sym->pins.size(); ++pi )
        {
            int position = sym->pins[pi].position >= 0 ? sym->pins[pi].position : static_cast<int>( pi );

            if( position >= 0 && static_cast<size_t>( position ) < pinNumbers.size() && stalePackageMap
                && ( static_cast<size_t>( position ) >= pinIgnore.size() || !pinIgnore[position] ) )
            {
                pinNumbers[position] = sym->pins[pi].name;
            }
        }
    }

    std::vector<int> pinOffsets( sym->pins.size() );
    std::vector<bool> explicitPinNets( sym->pins.size() );
    std::vector<int> placedOffsets = placedStackedPinOffsets( aInst );

    for( size_t i = 0; i < aInst.pins.size(); ++i )
    {
        size_t symbolPin = symbolPinIndex( *sym, aInst.pins[i], i );

        if( symbolPin < pinOffsets.size() )
        {
            pinOffsets[symbolPin] = placedOffsets[i];

            if( !aInst.pins[i].IsNoConnect() && ( aInst.pins[i].wordA || aInst.pins[i].wordB )
                && !m_currentImplicitPowerPins.count( &aInst.pins[i] ) )
                explicitPinNets[symbolPin] = true;
        }
    }

    PKG_KEY key{ srcOrPkg, aInst.pkgName, vi, letter };

    if( aSourceUnit )
    {
        *aSourceUnit = nullptr;
        auto prepared = m_preparedPkgToLib.find( key );

        if( prepared != m_preparedPkgToLib.end() )
        {
            *aSourceUnit = &prepared->first;
        }
        else
        {
            // An occurrence can rename its unit letter without changing the source device.
            for( const auto& [sourceKey, selection] : m_preparedPkgToLib )
            {
                if( std::get<0>( sourceKey ) != srcOrPkg || std::get<1>( sourceKey ) != aInst.pkgName
                    || std::get<2>( sourceKey ) != vi )
                    continue;

                const UNIT_INFO& source = m_preparedLibUnits.at( selection.first )[selection.second - 1];

                if( source.symbol == sym && source.pinNumbers == pinNumbers )
                {
                    if( *aSourceUnit )
                    {
                        *aSourceUnit = nullptr;
                        break;
                    }

                    *aSourceUnit = &sourceKey;
                }
            }
        }
    }

    auto    found = m_pkgToLib.find( key );

    if( found != m_pkgToLib.end() )
    {
        auto libIt = m_libSymbols.find( found->second.first );
        int  cachedUnit = found->second.second;

        if( libIt != m_libSymbols.end() && cachedUnit > 0
            && static_cast<size_t>( cachedUnit ) <= libIt->second.units.size() )
        {
            const UNIT_INFO& cached = libIt->second.units[cachedUnit - 1];

            if( cached.pinNumbers == pinNumbers && cached.pinNumberVisible == pinNumberVisible
                && cached.pinIgnore == pinIgnore && cached.pinOffsets == pinOffsets
                && cached.explicitPinNets == explicitPinNets )
                return found->second;
        }
    }

    std::string libnameBase = SymbolId( base );

    if( vi )
        libnameBase += "_v" + std::to_string( vi + 1 );

    std::string libname = libnameBase;

    for( size_t discriminator = 2;; ++discriminator )
    {
        auto lib = m_libSymbols.find( libname );

        if( lib == m_libSymbols.end() )
            break;

        auto unit = std::find_if( lib->second.units.begin(), lib->second.units.end(),
                                  [&]( const UNIT_INFO& aUnit )
                                  {
                                      return aUnit.letter == letter;
                                  } );

            if( unit == lib->second.units.end()
                 || ( unit->pinNumbers == pinNumbers && unit->pinNumberVisible == pinNumberVisible
                      && unit->pinIgnore == pinIgnore && unit->pinOffsets == pinOffsets
                      && unit->explicitPinNets == explicitPinNets ) )
        {
            break;
        }

        libname = libnameBase + "_pins" + std::to_string( discriminator );
    }

    LIB_ENTRY& ls = m_libSymbols[libname];

    if( ls.name.empty() )
    {
        ls.name = libname;
        ls.refPrefix = ( pkg && !pkg->refDes.empty() ) ? pkg->refDes : "U";
        ls.footprint = pkg ? pkg->pcbFootprint : "";
    }

    int unitNo = 0;

    for( size_t i = 0; i < ls.units.size(); ++i )
    {
        if( ls.units[i].letter == letter )
        {
            unitNo = (int) i + 1;
            break;
        }
    }

    if( unitNo == 0 )
    {
        UNIT_INFO unit;
        unit.letter = letter;
        unit.symbol = sym;
        unit.pinNumbers = pinNumbers;
        unit.pinNumberVisible = pinNumberVisible;
        unit.pinIgnore = pinIgnore;
        unit.pinOffsets = pinOffsets;
        unit.explicitPinNets = explicitPinNets;
        ls.units.push_back( std::move( unit ) );
        ls.kicadSymbol.reset();

        std::stable_sort( ls.units.begin(), ls.units.end(),
                          []( const UNIT_INFO& a, const UNIT_INFO& b )
                          {
                              return StrNumCmp( wxString::FromUTF8( a.letter ), wxString::FromUTF8( b.letter ) ) < 0;
                          } );

        for( size_t i = 0; i < ls.units.size(); ++i )
        {
            if( ls.units[i].letter == letter )
            {
                unitNo = (int) i + 1;
                break;
            }
        }

        // Inserting a unit renumbers later-sorting letters, so every map entry for
        // this lib symbol is stale; rebuild them.
        for( auto mapIt = m_pkgToLib.begin(); mapIt != m_pkgToLib.end(); )
        {
            if( mapIt->second.first == libname )
                mapIt = m_pkgToLib.erase( mapIt );
            else
                ++mapIt;
        }

        for( size_t i = 0; i < ls.units.size(); ++i )
        {
            std::string pkgName = sourcePackage + ls.units[i].letter + ".Normal";

            m_pkgToLib[PKG_KEY{ srcOrPkg, pkgName, vi, ls.units[i].letter }] = { libname, (int) i + 1 };
        }
    }

    m_pkgToLib[key] = { libname, unitNo };
    return { libname, unitNo };
}


std::string ORCAD_CONVERTER::powerLibFor( const std::string& aSymbolName, const std::string& aNetName )
{
    std::string libname = "PWR_" + SymbolId( aNetName ) + "_" + SymbolId( aSymbolName );

    if( m_libSymbols.count( libname ) )
        return libname;

    const ORCAD_SYMBOL_DEF* sym = nullptr;
    auto                    it = m_design.symbols.find( aSymbolName );

    if( it != m_design.symbols.end() )
    {
        sym = &it->second;
    }
    else
    {
        // Uncached power symbol; register empty definition (no graphics).
        ORCAD_SYMBOL_DEF& slot = m_design.symbols[aSymbolName];

        if( slot.name.empty() )
        {
            slot.typeId = ORCAD_ST_GLOBAL_SYMBOL;
            slot.name = aSymbolName;
        }

        sym = &slot;
    }

    LIB_ENTRY& ls = m_libSymbols[libname];
    ls.name = libname;
    ls.isPower = true;
    ls.powerNet = aNetName;
    ls.refPrefix = "#PWR";

    UNIT_INFO unit;
    unit.symbol = sym;
    unit.pinNumbers = { "1" };
    ls.units.push_back( std::move( unit ) );

    return libname;
}


LIB_SYMBOL* ORCAD_CONVERTER::kicadSymbolFor( const std::string& aLibName )
{
    auto it = m_libSymbols.find( aLibName );

    if( it == m_libSymbols.end() )
        return nullptr;

    LIB_ENTRY& entry = it->second;

    if( entry.kicadSymbol )
        return entry.kicadSymbol.get();

    // OrCAD cache names can contain LIB_ID-illegal chars (e.g. backslashes from
    // library path prefixes).
    wxString name = LIB_ID::FixIllegalChars( FromOrcadString( entry.name ), false ).wx_str();

    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( name );
    symbol->SetLibId( LIB_ID( wxString::FromUTF8( LIB_NICK ), name ) );
    symbol->SetPinNameOffset( schMm( 0.254 ) );

    // Per-pin text requires the symbol-wide visibility switch. Pin text sizes retain individual visibility.
    bool defaultShowPinNames = symbol->GetShowPinNames();
    bool defaultShowPinNumbers = symbol->GetShowPinNumbers();

    for( const UNIT_INFO& unit : entry.units )
    {
        if( unit.symbol && unit.symbol->generalFlags >= 0 )
        {
            int flags = unit.symbol->generalFlags;
            defaultShowPinNumbers = ( flags & 0x04 ) == 0;
            defaultShowPinNames = ( flags & 0x01 ) != 0;
            break;
        }
    }

    bool showPinNames = defaultShowPinNames;
    bool showPinNumbers = defaultShowPinNumbers;

    for( const UNIT_INFO& unit : entry.units )
    {
        if( !unit.symbol )
            continue;

        for( size_t pinIndex = 0; pinIndex < unit.symbol->pins.size(); ++pinIndex )
        {
            const ORCAD_SYMBOL_PIN& sourcePin = unit.symbol->pins[pinIndex];
            int position = sourcePin.position >= 0 ? sourcePin.position : static_cast<int>( pinIndex );

            for( const ORCAD_DISPLAY_PROP& display : sourcePin.displayProps )
            {
                if( !OrcadDisplayPropVisible( display ) )
                    continue;

                showPinNames |= propertyNamesEqual( display.name, "Name" )
                                || propertyNamesEqual( display.name, "Pin Name" );

                if( position >= static_cast<int>( unit.pinNumberVisible.size() )
                    || unit.pinNumberVisible[position] )
                {
                    showPinNumbers |= propertyNamesEqual( display.name, "Number" )
                                      || propertyNamesEqual( display.name, "Pin Number" );
                }
            }
        }
    }

    symbol->SetShowPinNames( showPinNames );
    symbol->SetShowPinNumbers( showPinNumbers );

    if( entry.isPower )
        symbol->SetGlobalPower();

    if( (int) entry.units.size() > 1 )
    {
        symbol->SetUnitCount( (int) entry.units.size(), false );

        for( size_t unit = 0; unit < entry.units.size(); ++unit )
            symbol->GetUnitDisplayNames()[static_cast<int>( unit ) + 1] = wxEmptyString;
    }

    const ORCAD_SYMBOL_DEF* u0 = entry.units.empty() ? nullptr : entry.units.front().symbol;
    ORCAD_BBOX              bb = ( u0 && u0->bbox ) ? *u0->bbox : ORCAD_BBOX();

    // Default field spot above body; in-memory symbol space Y-down, so above = more
    // negative Y.
    int topY = bb.y1 * ORCAD_IU_PER_DBU - schMm( 2.54 );

    SCH_FIELD& refField = symbol->GetReferenceField();
    SCH_FIELD& valField = symbol->GetValueField();

    if( entry.isPower )
    {
        refField.SetText( wxS( "#PWR" ) );
        refField.SetVisible( false );
        refField.SetTextPos( VECTOR2I( 0, topY ) );

        valField.SetText( FromOrcadString( entry.powerNet ) );
        valField.SetVisible( true );
        valField.SetTextPos( VECTOR2I( 0, topY ) );
    }
    else
    {
        refField.SetText( FromOrcadString( entry.refPrefix ) );
        refField.SetVisible( true );
        refField.SetTextPos( VECTOR2I( 0, topY ) );

        valField.SetText( name );
        valField.SetVisible( true );
        valField.SetTextPos( VECTOR2I( 0, topY - schMm( 2.54 ) ) );
    }

    // An empty Footprint field permits relinking imported board footprints by reference.
    symbol->GetFootprintField().SetVisible( false );
    symbol->GetDatasheetField().SetVisible( false );

    for( size_t ui = 0; ui < entry.units.size(); ++ui )
    {
        const UNIT_INFO& unit = entry.units[ui];
        int              unitNo = (int) ui + 1;

        if( !unit.symbol )
            continue;

        for( size_t primitiveIndex = 0; primitiveIndex < unit.symbol->primitives.size(); ++primitiveIndex )
        {
            const ORCAD_PRIMITIVE& primitive = unit.symbol->primitives[primitiveIndex];

            if( laterRectangleOccludesTextUnderscores( unit.symbol->primitives, primitiveIndex ) )
            {
                ORCAD_PRIMITIVE visible = primitive;
                std::replace( visible.text.begin(), visible.text.end(), '_', ' ' );
                addSymbolPrimitive( symbol.get(), visible, unitNo, unit.symbol->color );
            }
            else
            {
                addSymbolPrimitive( symbol.get(), primitive, unitNo, unit.symbol->color );
            }
        }

        BOX2I bodyBox = symbol->GetBodyBoundingBox( unitNo, 0, false, false );

        for( size_t pi = 0; pi < unit.symbol->pins.size(); ++pi )
        {
            ORCAD_SYMBOL_PIN sourcePin = unit.symbol->pins[pi];
            int                     position =
                    unit.symbol->pins[pi].position >= 0 ? unit.symbol->pins[pi].position : static_cast<int>( pi );

            if( position < static_cast<int>( unit.pinIgnore.size() ) && unit.pinIgnore[position] )
                continue;

            wxString number = position < static_cast<int>( unit.pinNumbers.size() )
                                      ? FromOrcadString( unit.pinNumbers[position] )
                                      : wxString::Format( wxS( "%d" ), (int) pi + 1 );

            int pinOffset = pi < unit.pinOffsets.size() ? unit.pinOffsets[pi] : 0;
            sourcePin.hotptX += pinOffset;
            sourcePin.startX += pinOffset;

            addSymbolPin( symbol.get(), sourcePin, number, unitNo, entry.isPower,
                          entry.isPower ? entry.powerNet : std::string(), defaultShowPinNames,
                          defaultShowPinNumbers,
                          position >= static_cast<int>( unit.pinNumberVisible.size() )
                                  || unit.pinNumberVisible[position],
                          &bodyBox,
                          pinOffset != 0,
                          pi < unit.explicitPinNets.size() && unit.explicitPinNets[pi] );
        }
    }

    entry.kicadSymbol = std::move( symbol );
    return entry.kicadSymbol.get();
}


static void addOrcadSymbolHatch( LIB_SYMBOL* aSymbol, const SCH_SHAPE& aOutline,
                                 const ORCAD_PRIMITIVE& aPrimitive, int aUnit, int aColor,
                                 uint32_t aModifyTimestamp )
{
    if( aPrimitive.fillStyle != 2 )
        return;

    STROKE_PARAMS hatchStroke( OrcadHatchLineWidthIu( aModifyTimestamp ), LINE_STYLE::SOLID,
                               OrcadColor( aColor ) );

    for( const SEG& line : OrcadHatchLines( aOutline, aPrimitive.hatchStyle,
                                           OrcadHatchPitchIu( aModifyTimestamp ) ) )
    {
        SCH_SHAPE* hatch = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        hatch->AddPoint( line.A );
        hatch->AddPoint( line.B );
        hatch->SetStroke( hatchStroke );
        hatch->SetFillMode( FILL_T::NO_FILL );
        hatch->SetUnit( aUnit );
        aSymbol->AddDrawItem( hatch, false );
    }
}


void ORCAD_CONVERTER::addSymbolPrimitive( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor,
                                          int aOffsetX, int aOffsetY )
{
    // Cache defs and in-memory symbol space both Y-down; .kicad_sch writer flips to
    // file Y-up itself.
    auto toX = [aOffsetX]( int aV )
    {
        return ( aV + aOffsetX ) * ORCAD_IU_PER_DBU;
    };
    auto toY = [aOffsetY]( int aV )
    {
        return ( aV + aOffsetY ) * ORCAD_IU_PER_DBU;
    };

    switch( aPrim.kind )
    {
    case ORCAD_PRIM_KIND::GROUP_PRIM:
        for( const ORCAD_PRIMITIVE& child : aPrim.children )
            addSymbolPrimitive( aSymbol, child, aUnit, aColor, aOffsetX + aPrim.x1, aOffsetY + aPrim.y1 );

        break;

    case ORCAD_PRIM_KIND::RECTANGLE:
    {
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        shape->SetPosition( VECTOR2I( toX( aPrim.x1 ), toY( aPrim.y1 ) ) );
        shape->SetEnd( VECTOR2I( toX( aPrim.x2 ), toY( aPrim.y2 ) ) );
        shape->SetStroke( strokeFor( aPrim, aColor ) );
        shape->SetFillMode( aPrim.fillStyle == 2 ? FILL_T::NO_FILL
                                                : OrcadFillType( aPrim.fillStyle, aPrim.hatchStyle ) );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );
        addOrcadSymbolHatch( aSymbol, *shape, aPrim, aUnit, aColor, m_design.library.modifyTimestamp );
        break;
    }

    case ORCAD_PRIM_KIND::LINE:
    {
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        shape->AddPoint( VECTOR2I( toX( aPrim.x1 ), toY( aPrim.y1 ) ) );
        shape->AddPoint( VECTOR2I( toX( aPrim.x2 ), toY( aPrim.y2 ) ) );
        shape->SetStroke( strokeFor( aPrim, aColor ) );
        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );
        break;
    }

    case ORCAD_PRIM_KIND::POLYLINE:
    case ORCAD_PRIM_KIND::POLYGON:
    {
        if( aPrim.points.size() < 2 )
            return;

        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        for( const ORCAD_POINT& pt : aPrim.points )
            shape->AddPoint( VECTOR2I( toX( pt.x ), toY( pt.y ) ) );

        bool isPolygon = aPrim.kind == ORCAD_PRIM_KIND::POLYGON;

        if( isPolygon && aPrim.points.front() != aPrim.points.back() )
            shape->AddPoint( VECTOR2I( toX( aPrim.points.front().x ), toY( aPrim.points.front().y ) ) );

        shape->SetStroke( strokeFor( aPrim, aColor ) );
        shape->SetFillMode( isPolygon && aPrim.fillStyle != 2
                                    ? OrcadFillType( aPrim.fillStyle, aPrim.hatchStyle )
                                    : FILL_T::NO_FILL );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );

        if( isPolygon )
            addOrcadSymbolHatch( aSymbol, *shape, aPrim, aUnit, aColor, m_design.library.modifyTimestamp );

        break;
    }

    case ORCAD_PRIM_KIND::BEZIER:
    {
        if( aPrim.points.size() < 4 || ( aPrim.points.size() - 1 ) % 3 != 0 )
        {
            if( aPrim.points.size() >= 2 )
            {
                SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

                for( const ORCAD_POINT& pt : aPrim.points )
                    shape->AddPoint( VECTOR2I( toX( pt.x ), toY( pt.y ) ) );

                shape->SetStroke( strokeFor( aPrim, aColor ) );
                shape->SetFillMode( FILL_T::NO_FILL );
                shape->SetUnit( aUnit );
                aSymbol->AddDrawItem( shape, false );
            }

            return;
        }

        for( size_t i = 0; i + 3 < aPrim.points.size(); i += 3 )
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
            shape->SetPosition( VECTOR2I( toX( aPrim.points[i].x ), toY( aPrim.points[i].y ) ) );
            shape->SetBezierC1( VECTOR2I( toX( aPrim.points[i + 1].x ), toY( aPrim.points[i + 1].y ) ) );
            shape->SetBezierC2( VECTOR2I( toX( aPrim.points[i + 2].x ), toY( aPrim.points[i + 2].y ) ) );
            shape->SetEnd( VECTOR2I( toX( aPrim.points[i + 3].x ), toY( aPrim.points[i + 3].y ) ) );
            shape->SetStroke( strokeFor( aPrim, aColor ) );
            shape->SetFillMode( FILL_T::NO_FILL );
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape, false );
        }

        break;
    }

    case ORCAD_PRIM_KIND::ELLIPSE:
    {
        double cx = ( aPrim.x1 + aPrim.x2 ) / 2.0;
        double cy = ( aPrim.y1 + aPrim.y2 ) / 2.0;
        double rx = std::abs( aPrim.x2 - aPrim.x1 ) / 2.0;
        double ry = std::abs( aPrim.y2 - aPrim.y1 ) / 2.0;

        if( std::abs( rx - ry ) < 0.5 )
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
            VECTOR2I   center( dbuIu( cx ), dbuIu( cy ) );
            shape->SetPosition( center );
            shape->SetEnd( center + VECTOR2I( dbuIu( rx ), 0 ) );
            shape->SetStroke( strokeFor( aPrim, aColor ) );
            shape->SetFillMode( aPrim.fillStyle == 2 ? FILL_T::NO_FILL
                                                    : OrcadFillType( aPrim.fillStyle, aPrim.hatchStyle ) );
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape, false );
            addOrcadSymbolHatch( aSymbol, *shape, aPrim, aUnit, aColor, m_design.library.modifyTimestamp );
        }
        else
        {
            // Unequal radii; approximate ellipse with closed polyline.
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            for( int k = 0; k <= 32; ++k )
            {
                double a = 2.0 * M_PI * k / 32.0;
                shape->AddPoint( VECTOR2I( dbuIu( cx + rx * std::cos( a ) ), dbuIu( cy + ry * std::sin( a ) ) ) );
            }

            shape->SetStroke( strokeFor( aPrim, aColor ) );
            shape->SetFillMode( aPrim.fillStyle == 2 ? FILL_T::NO_FILL
                                                    : OrcadFillType( aPrim.fillStyle, aPrim.hatchStyle ) );
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape, false );
            addOrcadSymbolHatch( aSymbol, *shape, aPrim, aUnit, aColor, m_design.library.modifyTimestamp );
        }

        break;
    }

    case ORCAD_PRIM_KIND::ARC: addSymbolArc( aSymbol, aPrim, aUnit, aColor, aOffsetX, aOffsetY ); break;

    case ORCAD_PRIM_KIND::TEXT:
    {
        bool      multiline = aPrim.text.find( '\n' ) != std::string::npos;
        bool      hasBox = aPrim.x1 != aPrim.x2 || aPrim.y1 != aPrim.y2;
        int       textX = hasBox && !multiline ? ( aPrim.x1 + aPrim.x2 ) / 2 : aPrim.x1;
        int       textY = hasBox ? ( aPrim.y1 + aPrim.y2 ) / 2 : aPrim.y1;
        int       boxedTextYOffset = 0;
        SCH_TEXT* text =
                new SCH_TEXT( VECTOR2I( toX( textX ), toY( textY ) ), FromOrcadString( aPrim.text ), LAYER_DEVICE );
        text->SetTextSize( textSize( aPrim.fontIdx, false ) );
        applyFont( text, aPrim.fontIdx, false );
        applyMultilineSpacing( text, aPrim.fontIdx, false );
        text->SetTextColor( OrcadColor( aColor ) );

        if( aPrim.fontIdx > 0 && aPrim.fontIdx <= static_cast<int>( m_design.library.fonts.size() ) )
        {
            int quarterTurns = KiROUND( m_design.library.fonts[aPrim.fontIdx - 1].escapement / 900.0 );

            if( std::abs( quarterTurns ) % 2 == 1 )
                text->SetTextAngle( ANGLE_VERTICAL );
        }

        text->SetHorizJustify( hasBox && !multiline ? GR_TEXT_H_ALIGN_CENTER : GR_TEXT_H_ALIGN_LEFT );
        text->SetVertJustify( hasBox ? GR_TEXT_V_ALIGN_CENTER : GR_TEXT_V_ALIGN_TOP );

        if( hasBox && aPrim.fontIdx > 0 && aPrim.fontIdx <= static_cast<int>( m_design.library.fonts.size() ) )
        {
            const ORCAD_FONT& font = m_design.library.fonts[aPrim.fontIdx - 1];

            if( ( font.pitchAndFamily & 0x3 ) == 1 && font.width != 0 )
            {
                BOX2I    inkBox = text->GetEffectiveTextShape( false )->BBox();
                int      boxWidth = std::abs( aPrim.x2 - aPrim.x1 ) * ORCAD_IU_PER_DBU;
                int      boxHeight = std::abs( aPrim.y2 - aPrim.y1 ) * ORCAD_IU_PER_DBU;
                int      inkWidth = std::max( ORCAD_IU_PER_DBU,
                                              boxWidth - std::abs( font.width ) * ORCAD_IU_PER_DBU );
                int      inkHeight = KiROUND( boxHeight * 0.7 );
                VECTOR2I size = text->GetTextSize();

                if( inkBox.GetWidth() > 0 && inkBox.GetHeight() > 0 )
                {
                    size.x = KiROUND( static_cast<double>( size.x ) * inkWidth / inkBox.GetWidth() );
                    size.y = KiROUND( static_cast<double>( size.y ) * inkHeight / inkBox.GetHeight() );
                    text->SetTextSize( size );
                    boxedTextYOffset = ( boxHeight - inkHeight ) / 4;
                }
            }
        }

        if( hasBox )
            text->SetPosition( VECTOR2I( toX( textX ), toY( textY ) + boxedTextYOffset ) );

        text->SetUnit( aUnit );
        aSymbol->AddDrawItem( text, false );
        break;
    }

    case ORCAD_PRIM_KIND::IMAGE:
        // Embedded images only in page graphics, not symbol bodies.
        break;
    }
}


void ORCAD_CONVERTER::addSymbolArc( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor,
                                    int aOffsetX, int aOffsetY )
{
    if( !aPrim.start || !aPrim.end )
        return;

    if( aPrim.start == aPrim.end )
        return;

    double cx = ( aPrim.x1 + aPrim.x2 ) / 2.0;
    double cy = ( aPrim.y1 + aPrim.y2 ) / 2.0;
    double rx = std::abs( aPrim.x2 - aPrim.x1 ) / 2.0;
    double ry = std::abs( aPrim.y2 - aPrim.y1 ) / 2.0;

    if( rx == 0.0 )
        rx = 0.01;

    if( ry == 0.0 )
        ry = 0.01;

    double a0 = std::atan2( ( aPrim.start->y - cy ) / ry, ( aPrim.start->x - cx ) / rx );
    double a1 = std::atan2( ( aPrim.end->y - cy ) / ry, ( aPrim.end->x - cx ) / rx );

    // OrCAD draws arcs CCW in screen (Y-down) coords, meaning decreasing parameter
    // angle here.
    if( a1 >= a0 )
        a1 -= 2.0 * M_PI;

    int steps = std::max( 4, (int) ( std::abs( a1 - a0 ) / ( M_PI / 16.0 ) ) );

    // Polyline approximation; robust for elliptical arcs KiCad arcs cannot represent.
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    for( int k = 0; k <= steps; ++k )
    {
        double a = a0 + ( a1 - a0 ) * k / steps;
        shape->AddPoint(
                VECTOR2I( dbuIu( cx + rx * std::cos( a ) + aOffsetX ), dbuIu( cy + ry * std::sin( a ) + aOffsetY ) ) );
    }

    shape->SetStroke( strokeFor( aPrim, aColor ) );
    shape->SetFillMode( FILL_T::NO_FILL );
    shape->SetUnit( aUnit );
    aSymbol->AddDrawItem( shape, false );
}


void ORCAD_CONVERTER::addSymbolPin( LIB_SYMBOL* aSymbol, const ORCAD_SYMBOL_PIN& aPin, const wxString& aNumber,
                                    int aUnit, bool aPower, const std::string& aNameOverride, bool aNameVisible,
                                    bool aShowPinNumbers, bool aNumberVisible, const BOX2I* aBodyBox, bool aHidden,
                                    bool aExplicitNet )
{
    int dx = aPin.startX - aPin.hotptX;
    int dy = aPin.startY - aPin.hotptY;
    int pinLength = KiROUND( std::hypot( (double) dx, (double) dy ) ) * ORCAD_IU_PER_DBU;
    bool hiddenPowerStyle = pinLength == 0 && ( aPin.shapeBits & 0x80 ) != 0;

    if( dx == 0 && dy == 0 && aBodyBox )
    {
        VECTOR2I hotPoint( aPin.hotptX * ORCAD_IU_PER_DBU, aPin.hotptY * ORCAD_IU_PER_DBU );
        std::array<int, 4> distances = {
            std::abs( hotPoint.x - aBodyBox->GetLeft() ), std::abs( hotPoint.x - aBodyBox->GetRight() ),
            std::abs( hotPoint.y - aBodyBox->GetTop() ), std::abs( hotPoint.y - aBodyBox->GetBottom() )
        };

        switch( std::min_element( distances.begin(), distances.end() ) - distances.begin() )
        {
        case 0: dx = 1; break;
        case 1: dx = -1; break;
        case 2: dy = 1; break;
        case 3: dy = -1; break;
        }
    }

    SCH_PIN* pin = new SCH_PIN( aSymbol );

    // Hot point = connection point; start point = body end.
    pin->SetPosition( VECTOR2I( aPin.hotptX * ORCAD_IU_PER_DBU, aPin.hotptY * ORCAD_IU_PER_DBU ) );
    pin->SetLength( pinLength );

    PIN_ORIENTATION orientation = PIN_ORIENTATION::PIN_RIGHT;

    if( dx > 0 )
        orientation = PIN_ORIENTATION::PIN_RIGHT;
    else if( dx < 0 )
        orientation = PIN_ORIENTATION::PIN_LEFT;
    else if( dy > 0 )
        orientation = PIN_ORIENTATION::PIN_DOWN; // start below hot point (Y-down source)
    else if( dy < 0 )
        orientation = PIN_ORIENTATION::PIN_UP;

    pin->SetOrientation( orientation );

    // shapeBits bit1 = clock, bit2 = inverted dot.
    bool isClock = ( aPin.shapeBits & 0x2 ) != 0;
    bool isDot = ( aPin.shapeBits & 0x4 ) != 0;

    if( isClock && isDot )
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
    else if( isClock )
        pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
    else if( isDot )
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
    else
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );

    if( aPower )
    {
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
    }
    else
    {
        pin->SetType( pinTypeFor( aPin.portType ) );
    }

    if( hiddenPowerStyle )
        pin->SetVisible( false );

    if( aHidden )
    {
        pin->SetVisible( false );
        pin->SetLength( 0 );
    }

    if( aExplicitNet && !pin->IsVisible() && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );

    // "$PIN"-prefixed names are OrCAD auto-generated placeholders.
    wxString name;

    if( !aNameOverride.empty() )
        name = FromOrcadString( aNameOverride );
    else if( !aPin.name.empty() && aPin.name.compare( 0, 4, "$PIN" ) != 0 )
        name = FromOrcadString( aPin.name );
    else if( aNameVisible && !aShowPinNumbers )
        name = aNumber;

    auto pinTextSize = [&]( int aFontIdx )
    {
        int resolved = resolveFontIndex( aFontIdx );

        if( resolved > 0 && resolved <= static_cast<int>( m_design.library.fonts.size() ) )
            return textSizeIU( aFontIdx );

        return schMm( 1.78 );
    };

    auto findDisplay = [&]( const char* aName, const char* aAlternate ) -> const ORCAD_DISPLAY_PROP*
    {
        auto it = std::find_if( aPin.displayProps.begin(), aPin.displayProps.end(),
                                [&]( const ORCAD_DISPLAY_PROP& aProp )
                                {
                                    return propertyNamesEqual( aProp.name, aName )
                                           || propertyNamesEqual( aProp.name, aAlternate );
                                } );
        return it != aPin.displayProps.end() ? &*it : nullptr;
    };

    const ORCAD_DISPLAY_PROP* nameDisplay = findDisplay( "Name", "Pin Name" );
    const ORCAD_DISPLAY_PROP* numberDisplay = findDisplay( "Number", "Pin Number" );

    if( nameDisplay )
        aNameVisible = OrcadDisplayPropVisible( *nameDisplay );

    aNumberVisible = aNumberVisible
                     && ( numberDisplay ? OrcadDisplayPropVisible( *numberDisplay ) : aShowPinNumbers );

    int nameFont = nameDisplay && nameDisplay->fontIdx > 0 ? nameDisplay->fontIdx : m_design.library.pinNameFont;
    int numberFont = numberDisplay && numberDisplay->fontIdx > 0 ? numberDisplay->fontIdx
                                                                 : m_design.library.pinNumberFont;

    pin->SetName( OrcadPinNameMarkup( name ) );
    pin->SetNumber( aNumber );
    pin->SetNameTextSize( aNameVisible && !name.IsEmpty() ? pinTextSize( nameFont ) : 0 );
    pin->SetNumberTextSize( aNumberVisible && !aNumber.IsEmpty() ? pinTextSize( numberFont ) : 0 );
    pin->SetUnit( aUnit );

    if( aHidden )
    {
        pin->SetNameTextSize( 0 );
        pin->SetNumberTextSize( 0 );
    }

    aSymbol->AddDrawItem( pin, false );

    if( !aPower && !aHidden && pinLength > 0 && aPin.portType == ORCAD_PORT_TYPE::INPUT_TYPE )
    {
        VECTOR2I direction( ( dx > 0 ) - ( dx < 0 ), ( dy > 0 ) - ( dy < 0 ) );
        VECTOR2I normal( -direction.y, direction.x );
        VECTOR2I apex( aPin.startX * ORCAD_IU_PER_DBU, aPin.startY * ORCAD_IU_PER_DBU );
        VECTOR2I base = apex - direction * dbuIu( 4 );
        VECTOR2I halfWidth = normal * dbuIu( 3 );
        SCH_SHAPE* wedge = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        wedge->AddPoint( apex );
        wedge->AddPoint( base + halfWidth );
        wedge->AddPoint( base - halfWidth );
        wedge->AddPoint( apex );
        wedge->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        wedge->SetFillMode( FILL_T::FILLED_SHAPE );
        wedge->SetUnit( aUnit );
        aSymbol->AddDrawItem( wedge, false );
    }

    if( aPower || aHidden || hiddenPowerStyle )
        return;
}


int ORCAD_CONVERTER::toKicadOrientation( int aOrient )
{
    switch( aOrient & 7 )
    {
    default:
    case 0: return SYM_ORIENT_0;
    case 1: return SYM_ORIENT_90;
    case 2: return SYM_ORIENT_180;
    case 3: return SYM_ORIENT_270;
    case 4: return SYM_ORIENT_0 + SYM_MIRROR_Y;
    case 5: return SYM_ORIENT_90 + SYM_MIRROR_X;
    case 6: return SYM_ORIENT_0 + SYM_MIRROR_X;
    case 7: return SYM_ORIENT_270 + SYM_MIRROR_X;
    }
}


void ORCAD_CONVERTER::finalizeNativePowerPackages()
{
    using PART_KEY = std::pair<const void*, wxString>;
    std::map<PART_KEY, size_t>                            indices;
    std::vector<std::vector<const PLACED_PACKAGE_UNIT*>> parts;

    for( const PLACED_PACKAGE_UNIT& unit : m_placedPackageUnits )
    {
        auto [entry, inserted] = indices.emplace( PART_KEY{ unit.scope, unit.reference }, parts.size() );

        if( inserted )
            parts.emplace_back();

        parts[entry->second].push_back( &unit );
    }

    for( const auto& part : parts )
    {
        if( !m_nativePowerFamilies.count( std::get<0>( *part.front()->sourceUnit ) ) )
            continue;

        const PLACED_PACKAGE_UNIT& first = *part.front();
        const std::string& templateName = m_preparedPkgToLib.at( *first.sourceUnit ).first;
        std::vector<UNIT_INFO> units = m_preparedLibUnits.at( templateName );
        std::set<size_t> placedUnits;
        std::vector<size_t> unitIndices;
        bool compatible = !first.reference.empty() && !first.reference.EndsWith( wxS( "?" ) );

        for( const PLACED_PACKAGE_UNIT* placed : part )
        {
            const auto& source = m_preparedPkgToLib.at( *placed->sourceUnit );
            size_t index = static_cast<size_t>( source.second - 1 );

            if( source.first != templateName || std::get<0>( *placed->sourceUnit ) != std::get<0>( *first.sourceUnit )
                || index >= units.size() || !placedUnits.insert( index ).second )
            {
                compatible = false;
                break;
            }

            units[index] = placed->unit;
            unitIndices.push_back( index );
        }

        std::set<std::string> letters;

        for( const UNIT_INFO& unit : units )
            compatible &= letters.insert( unit.letter ).second;

        if( !compatible )
        {
            warn( wxString::Format( _( "Native power units for '%s' have ambiguous package identity; "
                                      "their symbol definitions were kept separate." ), first.reference ) );
            continue;
        }

        std::string libname = templateName;

        for( size_t discriminator = 2;; ++discriminator )
        {
            auto candidate = m_libSymbols.find( libname );

            if( candidate == m_libSymbols.end() || candidate->second.units == units )
                break;

            libname = templateName + "_pins" + std::to_string( discriminator );
        }

        auto [entry, inserted] = m_libSymbols.try_emplace( libname );

        if( inserted )
        {
            const LIB_ENTRY& source = m_libSymbols.at( templateName );
            entry->second.name = libname;
            entry->second.units = std::move( units );
            entry->second.refPrefix = source.refPrefix;
            entry->second.footprint = source.footprint;
        }

        LIB_SYMBOL* definition = kicadSymbolFor( libname );

        if( !definition )
            continue;

        LIB_SYMBOL complete( *definition );

        // Placement adjusts source text and graphics for the instance transform.  Retain each
        // placed unit's adjusted drawings when assembling the complete package definition.
        for( size_t i = 0; i < part.size(); ++i )
        {
            SCH_SYMBOL* symbol = part[i]->symbol;
            int targetUnit = static_cast<int>( unitIndices[i] + 1 );
            std::vector<SCH_ITEM*> replaced;

            for( SCH_ITEM& item : complete.GetDrawItems() )
            {
                if( item.GetUnit() == targetUnit && item.Type() != SCH_FIELD_T )
                    replaced.push_back( &item );
            }

            for( SCH_ITEM* item : replaced )
                complete.RemoveDrawItem( item );

            for( const SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
            {
                if( item.GetUnit() == symbol->GetUnit() && item.Type() != SCH_FIELD_T )
                {
                    SCH_ITEM* copy = static_cast<SCH_ITEM*>( item.Clone() );
                    copy->SetUnit( targetUnit );
                    complete.AddDrawItem( copy, false );
                }
            }
        }

        complete.GetDrawItems().sort();
        LIB_ID id( wxString::FromUTF8( LIB_NICK ),
                   LIB_ID::FixIllegalChars( FromOrcadString( libname ), false ).wx_str() );

        // All units of one physical part must carry the same complete library definition on save.
        for( size_t i = 0; i < part.size(); ++i )
        {
            SCH_SYMBOL* symbol = part[i]->symbol;
            SCH_SCREEN* screen = static_cast<SCH_SCREEN*>( symbol->GetParent() );
            int unit = static_cast<int>( unitIndices[i] + 1 );
            screen->Remove( symbol );
            symbol->SetSchSymbolLibraryName( wxEmptyString );
            symbol->SetUnit( unit );
            symbol->SetUnitSelection( unit );
            symbol->SetLibId( id );
            symbol->SetLibSymbol( new LIB_SYMBOL( complete ) );
            screen->Append( symbol );
        }
    }

    m_placedPackageUnits.clear();
}


void ORCAD_CONVERTER::placeInstance( ORCAD_RAW_PAGE& aPage, const ORCAD_PLACED_INSTANCE& aInst, SCH_SCREEN* aScreen,
                                     const SCH_SHEET_PATH& aSheetPath )
{
    const PKG_KEY* sourceUnit = nullptr;
    auto [libname, unit] = libForInstance( aInst, &sourceUnit );

    LIB_ENTRY&              entry = m_libSymbols.at( libname );
    const UNIT_INFO&        uinfo = entry.units[unit - 1];
    const ORCAD_SYMBOL_DEF& def = *uinfo.symbol;

    ORCAD_BBOX bb = def.bbox.value_or( ORCAD_BBOX() );
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;
    int        ori = OrcadOrientOf( aInst.rotation, aInst.mirror );
    VECTOR2I   offset = OrcadOrientOffset( ori, w, h );
    VECTOR2I   pos = OrcadDbuToIu( aInst.x + offset.x, aInst.y + offset.y );

    // Transformed def hot points must land exactly on T0x10 absolute pin positions;
    // mismatch means wrong variant/orientation data.
    if( !aInst.pins.empty() && !symbolPinsMatch( def, aInst ) )
    {
        if( def.synthesized )
        {
            note( wxString::Format( _( "Page %s: %s uses a synthesized placeholder at a "
                                       "different orientation; pin layout approximate." ),
                                    FromOrcadString( aPage.name ), FromOrcadString( aInst.reference ) ) );
        }
        else
        {
            warn( wxString::Format( _( "Page %s: %s pin positions mismatch (orientation %d); "
                                       "geometry may be off." ),
                                    FromOrcadString( aPage.name ), FromOrcadString( aInst.reference ), ori ) );
        }
    }

    LIB_SYMBOL* libSymbol = kicadSymbolFor( libname );

    if( !libSymbol )
        return;

    LIB_ID      libId( wxString::FromUTF8( LIB_NICK ),
                       LIB_ID::FixIllegalChars( FromOrcadString( libname ), false ).wx_str() );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aSheetPath, unit, 0, pos );

    symbol->SetOrientation( toKicadOrientation( ori ) );

    TRANSFORM                                     inverseTransform = symbol->GetTransform().InverseTransform();
    std::vector<std::pair<SCH_TEXT*, SCH_SHAPE*>> degreeMarks;

    auto circleTouchesStroke = [&]( const SCH_SHAPE& aCircle )
    {
        int64_t radiusSquared = static_cast<int64_t>( aCircle.GetRadius() ) * aCircle.GetRadius();

        for( const SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
        {
            if( item.Type() != SCH_SHAPE_T )
                continue;

            const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( item );

            if( &shape == &aCircle || shape.GetShape() != SHAPE_T::POLY )
                continue;

            for( const VECTOR2I& point : shape.GetPolyShape().Outline( 0 ).CPoints() )
            {
                if( ( point - aCircle.GetPosition() ).SquaredEuclideanNorm() <= radiusSquared )
                    return true;
            }
        }

        return false;
    };

    for( SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() != SCH_TEXT_T )
            continue;

        SCH_TEXT* text = static_cast<SCH_TEXT*>( &item );

        if( text->GetLayer() == LAYER_PINNAM || text->GetLayer() == LAYER_PINNUM )
            continue;

        for( SCH_ITEM& candidate : symbol->GetLibSymbolRef()->GetDrawItems() )
        {
            if( candidate.Type() != SCH_SHAPE_T )
                continue;

            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( &candidate );

            if( shape->GetShape() != SHAPE_T::CIRCLE || shape->GetRadius() > 2 * ORCAD_IU_PER_DBU
                || circleTouchesStroke( *shape ) )
            {
                continue;
            }

            VECTOR2I delta = shape->GetPosition() - text->GetPosition();

            if( std::abs( delta.x ) <= text->GetTextSize().x && std::abs( delta.y ) <= text->GetTextSize().y )
                degreeMarks.emplace_back( text, shape );
        }
    }

    for( SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() != SCH_TEXT_T )
            continue;

        SCH_TEXT& text = static_cast<SCH_TEXT&>( item );

        if( text.GetLayer() == LAYER_PINNAM || text.GetLayer() == LAYER_PINNUM )
            continue;

        bool      degreeCue = std::any_of( degreeMarks.begin(), degreeMarks.end(),
                                           [&]( const auto& aPair )
                                           {
                                               return aPair.first == &text;
                                           } );
        bool centeredBox = text.GetVertJustify() == GR_TEXT_V_ALIGN_CENTER && !degreeCue;

        if( centeredBox )
        {
            VECTOR2I target = text.GetPosition();
            BOX2I glyphBox = text.GetEffectiveTextShape( false )->BBox();
            VECTOR2I correction = target - glyphBox.Centre();

            if( text.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
                correction.x = 0;

            text.SetPosition( text.GetPosition() + correction );
            continue;
        }

        int       baseline = OrcadTextBaselineOffset( text.GetTextSize().y );
        bool      sourceVertical = text.GetTextAngle().IsVertical();
        VECTOR2I  sourceCanvas( text.GetPosition().x, -text.GetPosition().y );
        VECTOR2I  canvasOffset( offset.x * ORCAD_IU_PER_DBU, offset.y * ORCAD_IU_PER_DBU );
        VECTOR2I  canvasPosition = sourceCanvas - canvasOffset + VECTOR2I( 0, baseline );
        VECTOR2I target = inverseTransform.TransformCoordinate( canvasPosition );
        text.SetPosition( target );
        bool symbolQuarterTurn = symbol->GetTransform().y1 != 0;
        text.SetTextAngle( sourceVertical != symbolQuarterTurn ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    }

    for( const auto& [text, degree] : degreeMarks )
    {
        BOX2I    textBox = symbol->GetTransform().TransformCoordinate( text->GetBoundingBox() );
        int      radius = degree->GetRadius();
        VECTOR2I degreePage( textBox.GetRight() + radius + ORCAD_IU_PER_DBU, textBox.Centre().y );
        VECTOR2I degreeLocal = inverseTransform.TransformCoordinate( degreePage );
        degree->SetPosition( degreeLocal );
        degree->SetEnd( degreeLocal + VECTOR2I( radius, 0 ) );
    }

    std::vector<SCH_ITEM*> pinTexts;

    for( SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T
            && ( item.GetLayer() == LAYER_PINNAM || item.GetLayer() == LAYER_PINNUM ) )
        {
            pinTexts.push_back( &item );
        }
    }

    for( SCH_ITEM* item : pinTexts )
        item->SetLayer( LAYER_DEVICE );

    symbol->GetLibSymbolRef()->GetDrawItems().sort();

    std::map<std::string, std::string> props = effectiveProps( aInst, def );

    auto occurrenceProperty = [&]( const char* aName ) -> const std::string*
    {
        if( !m_currentOccProps )
            return nullptr;

        auto occurrence = m_currentOccProps->find( aInst.dbId );

        if( occurrence == m_currentOccProps->end() )
            return nullptr;

        auto property = std::find_if( occurrence->second.begin(), occurrence->second.end(),
                                      [&]( const auto& aProperty )
                                      {
                                          return propertyNamesEqual( aProperty.first, aName );
                                      } );
        return property != occurrence->second.end() ? &property->second : nullptr;
    };

    const std::string* occurrenceValue = occurrenceProperty( "Value" );
    std::string        value = occurrenceValue ? *occurrenceValue : aInst.value;

    if( !occurrenceValue && value.empty() )
    {
        auto vIt = props.find( "Value" );

        if( vIt != props.end() )
            value = vIt->second;
    }

    if( !occurrenceValue && value.empty() )
        value = entry.name;

    auto installed = std::find_if( props.begin(), props.end(),
                                   []( const auto& aProperty )
                                   {
                                       return isInstalledPropertyName( aProperty.first );
                                   } );
    bool notInstalled =
            installed != props.end() && wxString::FromUTF8( installed->second ).CmpNoCase( wxS( "NI" ) ) == 0;

    const std::string* occurrenceFootprint = occurrenceProperty( "PCB Footprint" );
    std::string        footprint = occurrenceFootprint ? *occurrenceFootprint : std::string();
    auto               fIt = props.find( "PCB Footprint" );

    if( !occurrenceFootprint && fIt != props.end() )
        footprint = fIt->second;

    if( !occurrenceFootprint && footprint.empty() )
        footprint = entry.footprint;

    wxString reference = resolveReference( aInst );

    placeSymbolFields( symbol, aInst, def, ori, value, footprint );

    if( !notInstalled )
    {
        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            if( isInstalledPropertyName( field.GetName().ToStdString() )
                && field.GetText().CmpNoCase( wxS( "NI" ) ) == 0 )
            {
                notInstalled = true;
                break;
            }
        }
    }

    if( notInstalled )
    {
        symbol->SetDNP( true );

        SCH_FIELD* valueField = symbol->GetField( FIELD_T::VALUE );

        if( valueField && valueField->IsVisible() )
            valueField->SetText( wxS( "NI" ) );
    }

    symbol->SetRef( &aSheetPath, reference );
    symbol->SetUnitSelection( &aSheetPath, unit );

    if( sourceUnit && m_preparedLibUnits.at( m_preparedPkgToLib.at( *sourceUnit ).first ).size() > 1 )
    {
        bool nativePower = std::any_of( aInst.pins.begin(), aInst.pins.end(),
                                        [&]( const ORCAD_PIN_INST& aPin )
                                        {
                                            return m_currentImplicitPowerPins.count( &aPin );
                                        } );
        const void* scope = m_currentOccRefs ? static_cast<const void*>( m_currentOccRefs ) : aScreen;
        m_placedPackageUnits.push_back( { symbol, scope, reference, sourceUnit, uinfo } );

        if( nativePower )
            m_nativePowerFamilies.insert( std::get<0>( *sourceUnit ) );
    }

    // Parts without pins remain in the BOM but must not request footprints during board updates.
    if( def.pins.empty() && aInst.pins.empty() )
        symbol->SetExcludedFromBoard( true );

    m_sourceInstances[symbol] = &aInst;
    auto& sourcePins = m_sourcePinIdentities[symbol];

    for( size_t index = 0; index < aInst.pins.size(); ++index )
    {
        SOURCE_PIN_IDENTITY identity;
        size_t pi = symbolPinIndex( def, aInst.pins[index], index );
        VECTOR2I rawPosition = placedPinElectricalPosition( aInst, index );
        identity.position = OrcadDbuToIu( rawPosition.x, rawPosition.y );

        if( pi < def.pins.size() )
        {
            size_t position = def.pins[pi].position >= 0 ? def.pins[pi].position : pi;
            identity.ignored = position < uinfo.pinIgnore.size() && uinfo.pinIgnore[position];
            identity.number = position < uinfo.pinNumbers.size() ? FromOrcadString( uinfo.pinNumbers[position] )
                                                                 : wxString::Format( wxS( "%zu" ), pi + 1 );
            std::vector<SCH_PIN*> candidates;

            for( SCH_PIN* pin : symbol->GetPins( &aSheetPath ) )
            {
                if( pin->GetNumber() == identity.number && pin->GetPosition() == identity.position )
                    candidates.push_back( pin );
            }

            if( candidates.size() == 1 && candidates.front()->GetLibPin() )
                identity.libraryPin = candidates.front()->GetLibPin()->m_Uuid;
        }

        sourcePins.push_back( std::move( identity ) );
    }

    appendPageItem( aScreen, symbol );
    placeDefinitionImages( def, aInst.x, aInst.y, ori, aScreen );

    for( const ORCAD_PIN_INST& pin : aInst.pins )
    {
        if( pin.IsNoConnect() && !pin.wordA && !pin.wordB )
            appendPageItem( aScreen, new SCH_NO_CONNECT( OrcadDbuToIu( pin.x, pin.y ) ) );
    }
}


wxString ORCAD_CONVERTER::resolveReference( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    wxString reference = FromOrcadString( aInst.reference );

    // Hierarchy stream carries the authoritative per-occurrence designator; the
    // placed record remains the reusable page template's reference.
    if( m_currentOccRefs )
    {
        auto it = m_currentOccRefs->find( aInst.dbId );

        if( it != m_currentOccRefs->end() )
        {
            wxString occurrence = FromOrcadString( it->second );

            if( !occurrence.IsEmpty() && !occurrence.EndsWith( wxS( "?" ) ) )
                reference = occurrence;
        }
    }

    if( reference.IsEmpty() )
        reference = wxS( "?" );

    return reference;
}


void ORCAD_CONVERTER::placePowerSymbol( ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst, const std::string& aNet,
                                        SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath )
{
    std::string net = canonicalGlobalNetName( aNet );

    // Capture power names ignore case; occurrence-specific names must still remain distinct.
    if( FromOrcadString( net ).CmpNoCase( FromOrcadString( aNet ) ) != 0 )
        net = aNet;

    std::string libname = powerLibFor( aInst.name, net );

    LIB_ENTRY&              entry = m_libSymbols.at( libname );
    const ORCAD_SYMBOL_DEF* def = entry.units.front().symbol;

    ORCAD_BBOX bb = ( def && def->bbox ) ? *def->bbox : ORCAD_BBOX{ 0, 0, 20, 10 };
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;
    int        ori = OrcadOrientOf( aInst.rotation, aInst.mirror );
    VECTOR2I   offset = OrcadOrientOffset( ori, w, h );

    bool hasBbox = aInst.bbox.x1 || aInst.bbox.y1 || aInst.bbox.x2 || aInst.bbox.y2;
    int  bx = hasBbox ? std::min( aInst.bbox.x1, aInst.bbox.x2 ) : aInst.x;
    int  by = hasBbox ? std::min( aInst.bbox.y1, aInst.bbox.y2 ) : aInst.y;

    VECTOR2I pos = OrcadDbuToIu( bx + offset.x, by + offset.y );

    m_powerCount++;
    wxString reference = wxString::Format( wxS( "#PWR%04d" ), m_powerCount );

    LIB_SYMBOL* libSymbol = kicadSymbolFor( libname );

    if( !libSymbol )
        return;

    LIB_ID      libId( wxString::FromUTF8( LIB_NICK ),
                       LIB_ID::FixIllegalChars( FromOrcadString( libname ), false ).wx_str() );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aSheetPath, 1, 0, pos );

    symbol->SetOrientation( toKicadOrientation( ori ) );

    std::vector<SCH_PIN*> pins = symbol->GetPins();
    VECTOR2I              sourcePinDbu = powerPinPos( aPage, aInst );

    if( !pins.empty() )
    {
        VECTOR2I sourcePin = OrcadDbuToIu( sourcePinDbu.x, sourcePinDbu.y );
        symbol->SetPosition( symbol->GetPosition() + sourcePin - pins.front()->GetPosition() );
        pos = symbol->GetPosition();
    }

    SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
    refField->SetPosition( pos + VECTOR2I( 0, schMm( 2.54 ) ) );
    refField->SetTextAngle( ANGLE_HORIZONTAL );
    refField->SetVisible( false );

    auto        nameProperty = aInst.props.find( "Name" );
    std::string displayName = nameProperty != aInst.props.end() ? nameProperty->second : std::string();
    std::string logicalName = aInst.logicalName;
    symbol->SetValueFieldText( FromOrcadString( net ) );

    SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );
    valField->SetPosition( pos );
    valField->SetVisible( false );

    auto       displayedName = std::find_if( aInst.displayProps.begin(), aInst.displayProps.end(),
                                             []( const ORCAD_DISPLAY_PROP& aProp )
                                             {
                                           return propertyNamesEqual( aProp.name, "Name" )
                                                  || propertyNamesEqual( aProp.name, "NODENAME" );
                                       } );

    if( displayedName != aInst.displayProps.end() )
    {
        SCH_FIELD displayField( symbol, FIELD_T::USER, FromOrcadString( displayedName->name ) );
        displayField.SetText( FromOrcadString( !displayName.empty()   ? displayName
                                               : !logicalName.empty() ? logicalName
                                                                      : aNet ) );
        int  fontId = displayFontId( *displayedName );
        bool templateFont = displayUsesTemplateFont( *displayedName );
        int  size = textSizeIU( fontId, templateFont );
        int  baseline = textBaselineOffset( size, fontId, templateFont );
        bool textVertical = ( displayedName->rotation & 1 ) != 0;
        bool symbolFlips = symbol->GetTransform().y1 != 0;

        displayField.SetPosition( OrcadDbuToIu( bx + displayedName->x, by + displayedName->y )
                                  + ( textVertical ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline ) ) );
        displayField.SetTextAngle( textVertical != symbolFlips ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
        displayField.SetTextSize( textSize( fontId, templateFont ) );
        applyFont( &displayField, fontId, templateFont );
        displayField.SetTextColor( OrcadColor( displayedName->color ) );
        displayField.SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        displayField.SetEffectiveVertJustify( GR_TEXT_V_ALIGN_TOP );
        displayField.SetVisible( OrcadDisplayPropVisible( *displayedName ) );
        displayField.SetNameShown( OrcadDisplayPropShowsName( *displayedName ) );
        symbol->AddField( displayField );
    }

    SCH_FIELD* fpField = symbol->GetField( FIELD_T::FOOTPRINT );
    fpField->SetPosition( pos );
    fpField->SetVisible( false );

    SCH_FIELD* dsField = symbol->GetField( FIELD_T::DATASHEET );
    dsField->SetPosition( pos );
    dsField->SetVisible( false );

    symbol->SetRef( &aSheetPath, reference );
    symbol->SetUnitSelection( &aSheetPath, 1 );

    appendPageItem( aScreen, symbol );
}


void ORCAD_CONVERTER::placeSymbolFields( SCH_SYMBOL* aSymbol, const ORCAD_PLACED_INSTANCE& aInst,
                                         const ORCAD_SYMBOL_DEF& aDef, int aOrient, const std::string& aValue,
                                         const std::string& aFootprint )
{
    // Placed bbox includes OrCAD displayed text; true body box = orientation
    // transform of cache def bbox.
    ORCAD_BBOX bb = aDef.bbox.value_or( ORCAD_BBOX() );
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;

    VECTOR2I c1 = OrcadTransformPoint( aOrient, w, h, aInst.x, aInst.y, bb.x1, bb.y1 );
    VECTOR2I c2 = OrcadTransformPoint( aOrient, w, h, aInst.x, aInst.y, bb.x2, bb.y2 );

    int bxmin = std::min( c1.x, c2.x );
    int bxmax = std::max( c1.x, c2.x );
    int bymin = std::min( c1.y, c2.y );
    int bymax = std::max( c1.y, c2.y );

    int cxm = ( bxmin + bxmax ) * ORCAD_IU_PER_DBU / 2;
    int cym = ( bymin + bymax ) * ORCAD_IU_PER_DBU / 2;

    // Fields always read horizontally; KiCad rotates field text w/ symbol, so
    // 90/270 placement gets compensating field angle.
    int       angle = ORCAD_ORIENT_TABLE[aOrient & 7].angle;
    bool      vertical = angle == 90 || angle == 270;
    EDA_ANGLE fieldAngle = vertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL;

    // Visibility follows source display mode; bit 0x100 is field visible flag.
    bool showRef = false;
    bool showVal = false;
    bool showRefName = false;
    bool showValName = false;

    auto findReferenceDisplay = [&]( const char* aName, bool aVisibleOnly ) -> const ORCAD_DISPLAY_PROP*
    {
        auto it = std::find_if( aInst.displayProps.begin(), aInst.displayProps.end(),
                                [&]( const ORCAD_DISPLAY_PROP& aDisplay )
                                {
                                    return propertyNamesEqual( aDisplay.name, aName )
                                           && ( !aVisibleOnly || OrcadDisplayPropVisible( aDisplay ) );
                                } );

        return it != aInst.displayProps.end() ? &*it : nullptr;
    };

    const ORCAD_DISPLAY_PROP* referenceDisplay = findReferenceDisplay( "Part Reference", true );

    if( !referenceDisplay )
        referenceDisplay = findReferenceDisplay( "Reference", true );

    if( !referenceDisplay )
        referenceDisplay = findReferenceDisplay( "Part Reference", false );

    if( !referenceDisplay )
        referenceDisplay = findReferenceDisplay( "Reference", false );

    if( referenceDisplay )
    {
        showRef = OrcadDisplayPropVisible( *referenceDisplay );
        showRefName = OrcadDisplayPropShowsName( *referenceDisplay );
    }

    for( const ORCAD_DISPLAY_PROP& dp : aInst.displayProps )
    {
        bool visible = OrcadDisplayPropVisible( dp );

        if( propertyNamesEqual( dp.name, "Value" ) )
        {
            showVal = visible;
            showValName = OrcadDisplayPropShowsName( dp );
        }
    }

    aSymbol->SetValueFieldText( FromOrcadString( aValue ) );

    SCH_FIELD* refField = aSymbol->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* valField = aSymbol->GetField( FIELD_T::VALUE );

    // Vertical two-pin passives stack Reference/Value right of body, centered;
    // everything else gets Reference above, Value below.
    bool side = !vertical && aDef.pins.size() == 2 && aInst.pins.size() == 2
                && std::abs( aInst.pins[0].y - aInst.pins[1].y ) >= std::abs( aInst.pins[0].x - aInst.pins[1].x );

    if( side )
    {
        int sx = bxmax * ORCAD_IU_PER_DBU + schMm( 1.0 );
        int ry = showVal ? cym - schMm( 0.9 ) : cym;

        refField->SetPosition( VECTOR2I( sx, ry ) );
        refField->SetTextAngle( ANGLE_HORIZONTAL );
        refField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        refField->SetVisible( showRef );
        refField->SetNameShown( showRefName );

        valField->SetPosition( VECTOR2I( sx, cym + schMm( 0.9 ) ) );
        valField->SetTextAngle( ANGLE_HORIZONTAL );
        valField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        valField->SetVisible( showVal );
        valField->SetNameShown( showValName );
    }
    else
    {
        refField->SetPosition( VECTOR2I( cxm, bymin * ORCAD_IU_PER_DBU - schMm( 1.4 ) ) );
        refField->SetTextAngle( fieldAngle );
        refField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        refField->SetVisible( showRef );
        refField->SetNameShown( showRefName );

        valField->SetPosition( VECTOR2I( cxm, bymax * ORCAD_IU_PER_DBU + schMm( 1.4 ) ) );
        valField->SetTextAngle( fieldAngle );
        valField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        valField->SetVisible( showVal );
        valField->SetNameShown( showValName );
    }

    // Display-prop positions are canvas-space offsets from instance anchor; field
    // text does not rotate w/ symbol, so not run through body orientation transform.
    std::map<std::string, std::pair<VECTOR2I, const ORCAD_DISPLAY_PROP*>> shown;

    for( const ORCAD_DISPLAY_PROP& dp : aInst.displayProps )
    {
        auto value = std::make_pair( OrcadDbuToIu( aInst.x + dp.x, aInst.y + dp.y ), &dp );

        if( &dp == referenceDisplay )
            shown["Part Reference"] = value;
        else if( propertyNamesEqual( dp.name, "Part Reference" ) || propertyNamesEqual( dp.name, "Reference" ) )
            continue;
        else
            shown[dp.name] = value;
    }

    auto findShown = [&]( const std::string& aName )
    {
        auto exact = shown.find( aName );

        if( exact != shown.end() )
            return exact;

        return std::find_if( shown.begin(), shown.end(),
                             [&]( const auto& aItem ) { return propertyNamesEqual( aItem.first, aName ); } );
    };

    // KiCad re-rotates field text when parent transform flips X/Y (GetDrawRotation),
    // so store angle that renders property's own text angle after flip.
    bool symbolFlips = aSymbol->GetTransform().y1 != 0;

    auto storedFieldAngle = [&]( const ORCAD_DISPLAY_PROP* aDp ) -> EDA_ANGLE
    {
        bool textVertical = ( aDp->rotation & 1 ) != 0;
        return ( textVertical != symbolFlips ) ? ANGLE_VERTICAL : ANGLE_HORIZONTAL;
    };

    auto displayBaselineShift = [&]( const ORCAD_DISPLAY_PROP* aDp, int aSize ) -> VECTOR2I
    {
        int baseline = textBaselineOffset( aSize, displayFontId( *aDp ), displayUsesTemplateFont( *aDp ) );
        return ( aDp->rotation & 1 ) ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline );
    };

    auto drawDisplayedField = [&]( SCH_FIELD* aField, const ORCAD_DISPLAY_PROP& aDisplay )
    {
        if( !aField->IsVisible() || !OrcadDisplayPropShowsName( aDisplay ) )
            return;

        wxString content = FromOrcadString( aDisplay.name );

        if( OrcadDisplayPropShowsValue( aDisplay ) )
            content += wxS( " = " ) + aField->GetText();

        SCH_TEXT* text = new SCH_TEXT( aField->GetPosition(), content, LAYER_NOTES );
        int       fontId = displayFontId( aDisplay );
        bool      templateFont = displayUsesTemplateFont( aDisplay );
        text->SetTextSize( aField->GetTextSize() );
        applyFont( text, fontId, templateFont );
        text->SetTextColor( OrcadColor( aDisplay.color ) );
        text->SetTextAngle( ( aDisplay.rotation & 1 ) ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        appendPageItem( m_pageItemScreen, text );
        aField->SetVisible( false );
        aField->SetNameShown( false );
    };

    // Keep the source footprint as metadata. An empty Footprint field permits relinking by reference.
    aSymbol->SetFootprintFieldText( wxEmptyString );

    wxString fpName = FromOrcadString( aFootprint );

    if( !fpName.IsEmpty() )
    {
        SCH_FIELD fpMeta( aSymbol, FIELD_T::USER, wxS( "OrCAD Footprint" ) );
        fpMeta.SetText( fpName );

        auto shownFootprint = findShown( "PCB Footprint" );

        if( shownFootprint != shown.end() )
        {
            const ORCAD_DISPLAY_PROP* dp = shownFootprint->second.second;
            int                       fontId = displayFontId( *dp );
            bool                      templateFont = displayUsesTemplateFont( *dp );
            int                       size = textSizeIU( fontId, templateFont );

            fpMeta.SetPosition( shownFootprint->second.first + displayBaselineShift( dp, size ) );
            fpMeta.SetTextAngle( storedFieldAngle( dp ) );
            fpMeta.SetTextSize( textSize( fontId, templateFont ) );
            applyFont( &fpMeta, fontId, templateFont );
            fpMeta.SetTextColor( OrcadColor( dp->color ) );
            fpMeta.SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            fpMeta.SetEffectiveVertJustify( GR_TEXT_V_ALIGN_TOP );
            fpMeta.SetVisible( OrcadDisplayPropVisible( *dp ) );
            fpMeta.SetNameShown( OrcadDisplayPropShowsName( *dp ) );
            drawDisplayedField( &fpMeta, *dp );
        }
        else
        {
            fpMeta.SetPosition( aSymbol->GetPosition() );
            fpMeta.SetVisible( false );
        }

        aSymbol->AddField( fpMeta );
    }

    SCH_FIELD* fpField = aSymbol->GetField( FIELD_T::FOOTPRINT );
    fpField->SetPosition( aSymbol->GetPosition() );
    fpField->SetVisible( false );

    SCH_FIELD* dsField = aSymbol->GetField( FIELD_T::DATASHEET );
    dsField->SetPosition( aSymbol->GetPosition() );
    dsField->SetVisible( false );

    // OrCAD carries explicit reference/value field positions; honor them so fields
    // land where source placed them, not computed fallback.
    auto applyDisplayPos = [&]( SCH_FIELD* aField, const char* aName )
    {
        auto it = findShown( aName );

        if( it == shown.end() )
            return;

        const ORCAD_DISPLAY_PROP* dp = it->second.second;
        int                       fontId = displayFontId( *dp );
        bool                      templateFont = displayUsesTemplateFont( *dp );
        int                       size = textSizeIU( fontId, templateFont );

        // Capture's display-property origin uses the field type's font baseline.
        aField->SetPosition( it->second.first + displayBaselineShift( dp, size ) );
        aField->SetTextSize( textSize( fontId, templateFont ) );
        applyFont( aField, fontId, templateFont );
        aField->SetTextColor( OrcadColor( dp->color ) );
        aField->SetTextAngle( storedFieldAngle( dp ) );
        aField->SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aField->SetEffectiveVertJustify( GR_TEXT_V_ALIGN_TOP );
    };

    applyDisplayPos( refField, "Part Reference" );
    applyDisplayPos( valField, "Value" );

    if( auto shownValue = findShown( "Value" ); shownValue != shown.end() )
        drawDisplayedField( valField, *shownValue->second.second );

    std::map<std::string, std::string> properties = effectiveProps( aInst, aDef );

    auto partReference = std::find_if( properties.begin(), properties.end(),
                                       []( const auto& aProperty )
                                       { return propertyNamesEqual( aProperty.first, "Part Reference" ); } );

    const wxString resolvedReference = resolveReference( aInst );
    const bool occurrenceOverridesReference = resolvedReference != FromOrcadString( aInst.reference );
    wxString displayedText;

    if( partReference != properties.end() && !occurrenceOverridesReference )
    {
        displayedText = FromOrcadString( partReference->second );
    }
    else
    {
        std::string unitDesignator;

        if( m_currentOccUnitRefs )
        {
            auto unitRef = m_currentOccUnitRefs->find( aInst.dbId );

            if( unitRef != m_currentOccUnitRefs->end() )
                unitDesignator = unitRef->second;
        }

        const ORCAD_PACKAGE* package = packageFor( aInst );

        if( unitDesignator.empty() && package && aInst.unitIndex < package->devices.size() )
            unitDesignator = package->devices[aInst.unitIndex].unitRef;

        if( unitDesignator.empty() && !package )
            unitDesignator = unitLetter( aInst );

        if( unitDesignator.empty() && package && package->devices.size() > 1 )
            unitDesignator = unitLetter( aInst );

        unitDesignator = unitDesignator.substr( 0, unitDesignator.find( ':' ) );

        if( !unitDesignator.empty() && unitDesignator.front() != '#' )
            displayedText = resolvedReference + FromOrcadString( unitDesignator );
    }

    if( showRef )
    {
        if( !displayedText.IsEmpty() && displayedText != resolvedReference )
        {
            refField->SetVisible( false );

            SCH_FIELD displayedReference( aSymbol, FIELD_T::USER, wxS( "Part Reference" ) );
            displayedReference.SetText( displayedText );
            displayedReference.SetVisible( true );
            displayedReference.SetNameShown( showRefName );
            applyDisplayPos( &displayedReference, "Part Reference" );
            aSymbol->AddField( displayedReference );
        }
    }

    if( findShown( "Implementation" ) != shown.end() && !aInst.value.empty() )
    {
        auto implementation = std::find_if( properties.begin(), properties.end(),
                                            []( const auto& aProperty )
                                            { return propertyNamesEqual( aProperty.first, "Implementation" ); } );

        if( implementation == properties.end() )
            properties["Implementation"] = aInst.value;
        else if( implementation->second.empty() )
            implementation->second = aInst.value;
    }

    for( const auto& [propName, propValue] : properties )
    {
        if( propertyNamesEqual( propName, "Value" ) || propertyNamesEqual( propName, "PCB Footprint" )
            || propertyNamesEqual( propName, "Part Reference" ) || propertyNamesEqual( propName, "Reference" ) )
            continue;

        if( isBookkeepingProp( propName ) && findShown( propName ) == shown.end() )
            continue;

        bool unset = propValue.empty() || propValue == "<" + propName + ">";

        if( unset )
        {
            if( propertyNamesEqual( propName, "Description" ) )
                aSymbol->GetField( FIELD_T::DESCRIPTION )->SetText( wxString() );
            else if( propertyNamesEqual( propName, "Datasheet" ) )
                aSymbol->GetField( FIELD_T::DATASHEET )->SetText( wxString() );
            else
            {
                std::vector<SCH_FIELD*> fields;
                aSymbol->GetFields( fields, false );

                for( SCH_FIELD* field : fields )
                {
                    if( field->GetId() >= FIELD_T::USER
                        && propertyNamesEqual( field->GetName().ToStdString(), propName ) )
                    {
                        aSymbol->RemoveField( field );
                        break;
                    }
                }
            }

            continue;
        }

        if( propertyNamesEqual( propName, "Description" ) )
        {
            SCH_FIELD* descriptionField = aSymbol->GetField( FIELD_T::DESCRIPTION );
            descriptionField->SetText( FromOrcadString( propValue ) );
            applyDisplayPos( descriptionField, "Description" );

            if( auto shownDescription = findShown( "Description" ); shownDescription != shown.end() )
            {
                const ORCAD_DISPLAY_PROP& dp = *shownDescription->second.second;
                descriptionField->SetVisible( OrcadDisplayPropVisible( dp ) );
                descriptionField->SetNameShown( OrcadDisplayPropShowsName( dp ) );
                drawDisplayedField( descriptionField, dp );
            }

            continue;
        }

        if( propertyNamesEqual( propName, "Datasheet" ) )
        {
            SCH_FIELD* datasheetField = aSymbol->GetField( FIELD_T::DATASHEET );
            datasheetField->SetText( FromOrcadString( propValue ) );
            applyDisplayPos( datasheetField, "Datasheet" );

            if( auto shownDatasheet = findShown( "Datasheet" ); shownDatasheet != shown.end() )
            {
                const ORCAD_DISPLAY_PROP& dp = *shownDatasheet->second.second;
                datasheetField->SetVisible( OrcadDisplayPropVisible( dp ) );
                datasheetField->SetNameShown( OrcadDisplayPropShowsName( dp ) );
                drawDisplayedField( datasheetField, dp );
            }

            continue;
        }

        wxString  fieldName = propertyNamesEqual( propName, "Footprint" ) ? wxString( "OrCAD Footprint Property" )
                                                                          : FromOrcadString( propName );
        SCH_FIELD field( aSymbol, FIELD_T::USER, fieldName );
        field.SetText( FromOrcadString( propValue ) );

        auto sIt = findShown( propName );

        if( sIt != shown.end() )
        {
            const ORCAD_DISPLAY_PROP* dp = sIt->second.second;
            int                       fontId = displayFontId( *dp );
            bool                      templateFont = displayUsesTemplateFont( *dp );
            int                       size = textSizeIU( fontId, templateFont );

            field.SetPosition( sIt->second.first + displayBaselineShift( dp, size ) );
            field.SetTextAngle( storedFieldAngle( dp ) );
            field.SetTextSize( textSize( fontId, templateFont ) );
            applyFont( &field, fontId, templateFont );
            field.SetTextColor( OrcadColor( dp->color ) );
            field.SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            field.SetEffectiveVertJustify( GR_TEXT_V_ALIGN_TOP );
            field.SetVisible( OrcadDisplayPropVisible( *dp ) );
            field.SetNameShown( OrcadDisplayPropShowsName( *dp ) );
            drawDisplayedField( &field, *dp );
        }
        else
        {
            field.SetPosition( aSymbol->GetPosition() );
            field.SetTextSize( VECTOR2I( schMm( 1.27 ), schMm( 1.27 ) ) );
            field.SetVisible( false );
        }

        aSymbol->AddField( field );
    }
}


void ORCAD_CONVERTER::computeFontBaseline()
{
    std::map<int, int> counts;
    std::vector<int>   order;

    auto tally = [&]( int aFontIdx, bool aTemplateFont = true )
    {
        int height = fontHeightDbu( aFontIdx, aTemplateFont );

        if( !height )
            return;

        auto [it, isNew] = counts.try_emplace( height, 0 );

        if( isNew )
            order.push_back( height );

        it->second++;
    };

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
    {
        for( const ORCAD_WIRE& wire : page.wires )
        {
            for( const ORCAD_ALIAS& alias : wire.aliases )
                tally( wireAliasFontId( alias ) );
        }

        for( const ORCAD_PLACED_INSTANCE& inst : page.instances )
        {
            for( const ORCAD_DISPLAY_PROP& dp : inst.displayProps )
                tally( displayFontId( dp ), displayUsesTemplateFont( dp ) );
        }
    }

    int best = 0;
    int bestCount = 0;

    for( int height : order )
    {
        if( counts[height] > bestCount )
        {
            best = height;
            bestCount = counts[height];
        }
    }

    m_fontBaselineDbu = best;
}


int ORCAD_CONVERTER::displayFontId( const ORCAD_DISPLAY_PROP& aProp ) const
{
    return m_design.library.templateFonts.empty() ? resolveFontIndex( aProp.fontIdx ) : OrcadDisplayFontId( aProp );
}


bool ORCAD_CONVERTER::displayUsesTemplateFont( const ORCAD_DISPLAY_PROP& aProp ) const
{
    return aProp.fontIdx <= 0 && !m_design.library.templateFonts.empty();
}


int ORCAD_CONVERTER::wireAliasFontId( const ORCAD_ALIAS& aAlias ) const
{
    if( !m_design.library.templateFonts.empty() )
    {
        if( aAlias.fontIdx <= 0 || aAlias.fontIdx >= static_cast<int>( m_design.library.templateFonts.size() ) )
            return 5;
    }

    return aAlias.fontIdx;
}


int ORCAD_CONVERTER::textBaselineOffset( int aTextSize, int aFontIdx, bool aTemplateFont ) const
{
    int resolved = aTemplateFont ? resolveFontIndex( aFontIdx ) : aFontIdx;

    if( resolved > 0 && resolved <= static_cast<int>( m_design.library.fonts.size() ) )
    {
        std::string face = m_design.library.fonts[resolved - 1].face;
        std::transform( face.begin(), face.end(), face.begin(),
                        []( unsigned char aChar ) { return std::tolower( aChar ); } );

        if( face == "arial narrow" )
            return KiROUND( aTextSize * ( m_design.library.fonts[resolved - 1].bold ? 0.62 : 0.47 ) );

        if( face == "verdana" )
            return KiROUND( aTextSize * 0.675 );
    }

    return OrcadTextBaselineOffset( aTextSize );
}


int ORCAD_CONVERTER::resolveFontIndex( int aFontIdx ) const
{
    const std::vector<int>& templateFonts = m_design.library.templateFonts;

    if( !templateFonts.empty() && aFontIdx >= 0 && aFontIdx < static_cast<int>( templateFonts.size() ) )
    {
        int mapped = templateFonts[aFontIdx];

        if( mapped > 0 )
            return mapped;

        if( templateFonts.front() > 0 )
            return templateFonts.front();
    }

    if( aFontIdx == 0 && !m_design.library.fonts.empty() )
        return 1;

    return aFontIdx;
}


int ORCAD_CONVERTER::fontHeightDbu( int aFontIdx, bool aTemplateFont ) const
{
    if( aTemplateFont )
        aFontIdx = resolveFontIndex( aFontIdx );

    if( aFontIdx > 0 && aFontIdx <= (int) m_design.library.fonts.size() )
        return std::abs( m_design.library.fonts[aFontIdx - 1].height );

    return 0;
}


int ORCAD_CONVERTER::textSizeIU( int aFontIdx, bool aTemplateFont ) const
{
    int height = fontHeightDbu( aFontIdx, aTemplateFont );

    if( !height )
        return schMm( 1.27 );

    double compensation = 1.4;
    int    resolved = aTemplateFont ? resolveFontIndex( aFontIdx ) : aFontIdx;

    if( resolved > 0 && resolved <= static_cast<int>( m_design.library.fonts.size() ) )
    {
        std::string face = m_design.library.fonts[resolved - 1].face;
        std::transform( face.begin(), face.end(), face.begin(),
                        []( unsigned char aChar ) { return std::tolower( aChar ); } );

        if( face == "arial narrow" )
            compensation = m_design.library.fonts[resolved - 1].bold ? 1.6 : 1.46;
        else if( face == "verdana" )
            compensation = 1.46;
    }

    double mm = std::round( height * 25.4 / ( 96.0 * compensation ) * 100.0 ) / 100.0;

    return schMm( mm );
}


VECTOR2I ORCAD_CONVERTER::textSize( int aFontIdx, bool aTemplateFont ) const
{
    int height = textSizeIU( aFontIdx, aTemplateFont );

    if( aTemplateFont )
        aFontIdx = resolveFontIndex( aFontIdx );

    if( aFontIdx <= 0 || aFontIdx > static_cast<int>( m_design.library.fonts.size() ) )
        return VECTOR2I( height, height );

    const ORCAD_FONT& font = m_design.library.fonts[aFontIdx - 1];

    constexpr uint8_t c_FIXED_PITCH = 1;

    if( font.width == 0 )
        return VECTOR2I( height, height );

    if( ( font.pitchAndFamily & 0x3 ) != c_FIXED_PITCH )
    {
        std::string face = font.face;
        std::transform( face.begin(), face.end(), face.begin(),
                        []( unsigned char aChar ) { return std::tolower( aChar ); } );
        // lfWidth is an average character width; KiCad's X size is an em scale.
        double averageEmRatio = 0.5;

        if( face == "arial narrow" )
        {
            // Capture's GDI metrics use different average advances for regular and bold faces.
            averageEmRatio = font.bold ? 803.0 / 2048.0 : 0.358;
        }

        double aspect = std::abs( font.width ) / ( std::abs( font.height ) * averageEmRatio );
        double mm = std::round( schIUScale.IUTomm( height ) * aspect * 100.0 ) / 100.0;
        return VECTOR2I( schMm( mm ), height );
    }

    double mm = std::round( std::abs( font.width ) * 25.4 * 0.9 / 96.0 * 100.0 ) / 100.0;

    return VECTOR2I( schMm( mm ), height );
}


void ORCAD_CONVERTER::applyFont( EDA_TEXT* aText, int aFontIdx, bool aTemplateFont ) const
{
    if( aTemplateFont )
        aFontIdx = resolveFontIndex( aFontIdx );

    if( !aText || aFontIdx <= 0 || aFontIdx > static_cast<int>( m_design.library.fonts.size() ) )
        return;

    const ORCAD_FONT& font = m_design.library.fonts[aFontIdx - 1];
    aText->SetBold( font.bold );
    aText->SetItalic( font.italic );

    if( !font.face.empty() )
    {
        wxString                     face = FromOrcadString( font.face );
        const std::vector<wxString>* embeddedFonts = nullptr;

        if( face.CmpNoCase( wxS( "Elephant" ) ) == 0 )
        {
            face = wxS( "KiCad OrCAD Elephant" );
            wxFileName fontFile( PATHS::GetStockDataPath()
                                 + wxS( "/libwmf/fonts/KiCadOrCADElephant-Black.ttf" ) );
            EMBEDDED_FILES* files = m_schematic->GetEmbeddedFiles();

            if( files->AddFile( fontFile, false ) )
            {
                files->SetAreFontsEmbedded( true );
                embeddedFonts = files->UpdateFontFiles();
            }
        }

        aText->SetFont( KIFONT::FONT::GetFont( face, font.bold, font.italic, embeddedFonts ) );
    }
}


void ORCAD_CONVERTER::applyMultilineSpacing( SCH_TEXT* aText, int aFontIdx, bool aTemplateFont ) const
{
    if( !aText || aText->GetText().Find( '\n' ) == wxNOT_FOUND )
        return;

    int sourceHeight = fontHeightDbu( aFontIdx, aTemplateFont );

    if( sourceHeight <= 0 )
        return;

    KIFONT::FONT* font = aText->GetDrawFont( nullptr );
    int sourceInterline = schMm( sourceHeight * 25.4 / 96.0 );
    int kicadInterline = KiROUND( font->GetInterline( aText->GetTextSize().y, aText->GetFontMetrics() ) );

    if( kicadInterline > 0 )
        aText->SetLineSpacing( static_cast<double>( sourceInterline ) / kicadInterline );
}


std::string ORCAD_CONVERTER::SymbolId( const std::string& aName )
{
    std::string out;
    bool        inRun = false;

    for( unsigned char c : aName )
    {
        // 0xA0 = CP-1252 non-breaking space.
        bool separator = c == ':' || c == '"' || c == '/' || c == ' ' || c == '\t' || c == '\n' || c == '\r'
                         || c == '\f' || c == '\v' || c == 0xA0;

        if( separator )
        {
            if( !inRun )
                out += '_';

            inRun = true;
        }
        else
        {
            out += (char) c;
            inRun = false;
        }
    }

    size_t first = out.find_first_not_of( '_' );

    if( first == std::string::npos )
        return "SYM";

    size_t last = out.find_last_not_of( '_' );
    out = out.substr( first, last - first + 1 );

    return out.empty() ? "SYM" : out;
}
