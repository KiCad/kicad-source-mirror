/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "sch_easyedapro_parser.h"
#include <io/easyedapro/easyedapro_import_utils.h>

#include <core/map_helpers.h>

#include <sch_io/sch_io_mgr.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_no_connect.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <string_utils.h>
#include <bezier_curves.h>
#include <wx/base64.h>
#include <wx/log.h>
#include <wx/url.h>
#include <wx/mstream.h>
#include <gfx_import_utils.h>
#include <import_gfx/svg_import_plugin.h>
#include <import_gfx/graphics_importer_lib_symbol.h>
#include <import_gfx/graphics_importer_sch.h>


// clang-format off
static const std::vector<wxString> c_attributesWhitelist = { "Value",
                                                             "Datasheet",
                                                             "Manufacturer Part",
                                                             "Manufacturer",
                                                             "BOM_Manufacturer Part",
                                                             "BOM_Manufacturer",
                                                             "Supplier Part",
                                                             "Supplier",
                                                             "BOM_Supplier Part",
                                                             "BOM_Supplier",
                                                             "LCSC Part Name" };
// clang-format on


SCH_EASYEDAPRO_PARSER::SCH_EASYEDAPRO_PARSER( SCHEMATIC*         aSchematic,
                                              PROGRESS_REPORTER* aProgressReporter )
{
    m_schematic = aSchematic;
}


SCH_EASYEDAPRO_PARSER::~SCH_EASYEDAPRO_PARSER()
{
}


double SCH_EASYEDAPRO_PARSER::Convert( wxString aValue )
{
    double value = 0;

    if( !aValue.ToCDouble( &value ) )
        THROW_IO_ERROR( wxString::Format( _( "Failed to parse value: '%s'" ), aValue ) );

    return value;
}


double SCH_EASYEDAPRO_PARSER::SizeToKi( wxString aValue )
{
    return ScaleSize( Convert( aValue ) );
}


static LINE_STYLE ConvertStrokeStyle( int aStyle )
{
    if( aStyle == 0 )
        return LINE_STYLE::SOLID;
    else if( aStyle == 1 )
        return LINE_STYLE::DASH;
    else if( aStyle == 2 )
        return LINE_STYLE::DOT;
    else if( aStyle == 3 )
        return LINE_STYLE::DASHDOT;

    return LINE_STYLE::DEFAULT;
}


template <typename T>
void SCH_EASYEDAPRO_PARSER::ApplyFontStyle( const std::map<wxString, nlohmann::json>& fontStyles,
                                            T& text, const wxString& styleStr )
{
    auto it = fontStyles.find( styleStr );

    if( it == fontStyles.end() )
        return;

    nlohmann::json style = it->second;

    if( !style.is_array() )
        return;

    if( style.size() < 12 )
        return;

    if( style.at( 3 ).is_string() )
    {
        COLOR4D color( style.at( 3 ).get<wxString>() );
        text->SetTextColor( color );
    }

    if( style.at( 4 ).is_string() )
    {
        wxString fontname = ( style.at( 4 ) );

        // JLCEDA Pro V3 export to format version V1 specifies Arial explicitly instead of null for default font
        if( fontname != wxS( "Arial" ) && !fontname.IsSameAs( wxS( "default" ), false ) )
            text->SetFont( KIFONT::FONT::GetFont( fontname ) );
    }

    if( style.at( 5 ).is_number() )
    {
        double size = style.at( 5 ).get<double>() * 0.62;
        text->SetTextSize( VECTOR2I( ScaleSize( size ), ScaleSize( size ) ) );
    }

    if( style.at( 10 ).is_number() )
    {
        int valign = style.at( 10 );

        if( !text->GetText().Contains( wxS( "\n" ) ) )
        {
            if( valign == 0 )
                text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            else if( valign == 1 )
                text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            else if( valign == 2 )
                text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        }
        else
        {
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            // TODO: align by first line
        }
    }
    else
    {
        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }

    if( style.at( 11 ).is_number() )
    {
        int halign = style.at( 11 );

        if( halign == 0 )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( halign == 1 )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else if( halign == 2 )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    }
    else
    {
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
}


template <typename T>
void SCH_EASYEDAPRO_PARSER::ApplyLineStyle( const std::map<wxString, nlohmann::json>& lineStyles,
                                            T& shape, const wxString& styleStr )
{
    auto it = lineStyles.find( styleStr );

    if( it == lineStyles.end() )
        return;

    nlohmann::json style = it->second;

    if( !style.is_array() )
        return;

    if( style.size() < 6 )
        return;

    STROKE_PARAMS stroke = shape->GetStroke();

    if( style.at( 2 ).is_string() )
    {
        wxString colorStr = style.at( 2 ).get<wxString>();

        if( !colorStr.empty() && colorStr.starts_with( wxS( "#" ) ) )
        {
            COLOR4D color( colorStr );
            stroke.SetColor( color );
        }
    }

    if( style.at( 3 ).is_number() )
    {
        int dashStyle = style.at( 3 );
        stroke.SetLineStyle( ConvertStrokeStyle( dashStyle ) );
    }

    if( style.at( 5 ).is_number() )
    {
        double thickness = style.at( 5 );
        stroke.SetWidth( ScaleSize( thickness ) );
    }

    shape->SetStroke( stroke );
}


wxString SCH_EASYEDAPRO_PARSER::ResolveFieldVariables(
        const wxString aInput, const std::map<wxString, wxString>& aDeviceAttributes )
{
    wxString inputText = aInput;
    wxString resolvedText;
    int      variableCount = 0;

    // Resolve variables
    // ={Variable1}text{Variable2}
    do
    {
        if( !inputText.StartsWith( wxS( "={" ) ) )
            return inputText;

        resolvedText.Clear();
        variableCount = 0;

        for( size_t i = 1; i < inputText.size(); )
        {
            wxUniChar c = inputText[i++];

            if( c == '{' )
            {
                wxString varName;
                bool     endFound = false;

                while( i < inputText.size() )
                {
                    c = inputText[i++];

                    if( c == '}' )
                    {
                        endFound = true;
                        break;
                    }

                    varName << c;
                }

                if( !endFound )
                    return inputText;

                wxString varValue =
                        get_def( aDeviceAttributes, varName, wxString::Format( "{%s!}", varName ) );

                resolvedText << varValue;
                variableCount++;
            }
            else
            {
                resolvedText << c;
            }
        }
        inputText = resolvedText;

    } while( variableCount > 0 );

    return resolvedText;
}


template <typename T>
void SCH_EASYEDAPRO_PARSER::ApplyAttrToField( const std::map<wxString, nlohmann::json>& fontStyles,
                                              T* field, const EASYEDAPRO::SCH_ATTR& aAttr,
                                              bool aIsSym, bool aToSym,
                                              const std::map<wxString, wxString>& aDeviceAttributes,
                                              SCH_SYMBOL*                         aParent )
{
    EDA_TEXT* text = static_cast<EDA_TEXT*>( field );

    text->SetText( ResolveFieldVariables( aAttr.value, aDeviceAttributes ) );
    text->SetVisible( aAttr.keyVisible || aAttr.valVisible );

    field->SetNameShown( aAttr.keyVisible );

    if( aAttr.position )
    {
        field->SetPosition( !aIsSym ? ScalePos( *aAttr.position )
                                    : ScalePosSym( *aAttr.position ) );
    }

    ApplyFontStyle( fontStyles, text, aAttr.fontStyle );

    auto parent = aParent;
    if( parent && parent->Type() == SCH_SYMBOL_T )
    {
        int orient = static_cast<SCH_SYMBOL*>( parent )->GetOrientation();

        if( orient == SYM_ORIENT_180 )
        {
            text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
        }
        else if( orient == SYM_MIRROR_X + SYM_ORIENT_0 )
        {
            text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
        }
        else if( orient == SYM_MIRROR_Y + SYM_ORIENT_0 )
        {
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
        }
        else if( orient == SYM_MIRROR_Y + SYM_ORIENT_180 )
        {
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( text->GetHorizJustify() ) );
        }
        else if( orient == SYM_ORIENT_90 )
        {
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
        }
        if( orient == SYM_ORIENT_270 )
        {
            text->SetTextAngle( ANGLE_VERTICAL );
        }
        else if( orient == SYM_MIRROR_X + SYM_ORIENT_90 )
        {
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
        }
        else if( orient == SYM_MIRROR_X + SYM_ORIENT_270 )
        {
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
        }
        else if( orient == SYM_MIRROR_Y + SYM_ORIENT_90 )
        {
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
        }
        else if( orient == SYM_MIRROR_Y + SYM_ORIENT_270 )
        {
            text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( text->GetHorizJustify() ) );
        }

        if( aAttr.rotation == 90 )
        {
            if( text->GetTextAngle() == ANGLE_HORIZONTAL )
                text->SetTextAngle( ANGLE_VERTICAL );
            else
                text->SetTextAngle( ANGLE_HORIZONTAL );

            if( orient == SYM_ORIENT_90 )
            {
                text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
                text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
            }
            if( orient == SYM_ORIENT_270 )
            {
                text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
                text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -text->GetHorizJustify() ) );
            }
            else if( orient == SYM_MIRROR_X + SYM_ORIENT_90 )
            {
                text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -text->GetVertJustify() ) );
            }
        }
    }
}


EASYEDAPRO::SYM_INFO
SCH_EASYEDAPRO_PARSER::ParseSymbol( const std::vector<nlohmann::json>&  aLines,
                                    const std::map<wxString, wxString>& aDeviceAttributes )
{
    EASYEDAPRO::SYM_INFO symInfo;

    std::unique_ptr<LIB_SYMBOL> ksymbolPtr = std::make_unique<LIB_SYMBOL>( wxEmptyString );
    LIB_SYMBOL*                 ksymbol = ksymbolPtr.get();

    std::map<wxString, nlohmann::json> lineStyles;
    std::map<wxString, nlohmann::json> fontStyles;
    std::map<wxString, int>            partUnits;

    std::map<int, std::map<wxString, EASYEDAPRO::SCH_ATTR>>        unitAttributes;
    std::map<int, std::map<wxString, std::vector<nlohmann::json>>> unitParentedLines;

    int totalUnits = 0;

    for( const nlohmann::json& line : aLines )
    {
        wxString type = line.at( 0 );

        if( type == wxS( "LINESTYLE" ) )
            lineStyles[line.at( 1 )] = line;
        else if( type == wxS( "FONTSTYLE" ) )
            fontStyles[line.at( 1 )] = line;
        else if( type == wxS( "PART" ) )
            partUnits[line.at( 1 )] = ++totalUnits;
    }

    symInfo.partUnits = partUnits;
    ksymbol->SetUnitCount( totalUnits, false );

    int currentUnit = 1;

    for( const nlohmann::json& line : aLines )
    {
        wxString type = line.at( 0 );

        if( type == wxS( "PART" ) )
        {
            currentUnit = partUnits.at( line.at( 1 ) );
        }
        else if( type == wxS( "RECT" ) )
        {
            VECTOR2D start( line.at( 2 ), line.at( 3 ) );
            VECTOR2D end( line.at( 4 ), line.at( 5 ) );
            wxString styleStr = line.at( 9 );

            auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );

            rect->SetStart( ScalePosSym( start ) );
            rect->SetEnd( ScalePosSym( end ) );

            rect->SetUnit( currentUnit );
            ApplyLineStyle( lineStyles, rect, styleStr );

            ksymbol->AddDrawItem( rect.release() );
        }
        else if( type == wxS( "CIRCLE" ) )
        {
            VECTOR2D center( line.at( 2 ), line.at( 3 ) );
            double   radius = line.at( 4 );
            wxString styleStr = line.at( 5 );

            auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );

            circle->SetCenter( ScalePosSym( center ) );
            circle->SetEnd( circle->GetCenter() + VECTOR2I( ScaleSize( radius ), 0 ) );

            circle->SetUnit( currentUnit );
            ApplyLineStyle( lineStyles, circle, styleStr );

            ksymbol->AddDrawItem( circle.release() );
        }
        else if( type == wxS( "ARC" ) )
        {
            VECTOR2D start( line.at( 2 ), line.at( 3 ) );
            VECTOR2D mid( line.at( 4 ), line.at( 5 ) );
            VECTOR2D end( line.at( 6 ), line.at( 7 ) );
            wxString styleStr = line.at( 8 );

            VECTOR2D kstart = ScalePosSym( start );
            VECTOR2D kmid = ScalePosSym( mid );
            VECTOR2D kend = ScalePosSym( end );

            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );

            shape->SetArcGeometry( kstart, kmid, kend );

            shape->SetUnit( currentUnit );
            ApplyLineStyle( lineStyles, shape, styleStr );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( type == wxS( "BEZIER" ) )
        {
            std::vector<double> points = line.at( 2 );
            wxString            styleStr = line.at( 3 );

            std::unique_ptr<SCH_SHAPE> shape =
                    std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER, LAYER_DEVICE );

            for( size_t i = 1; i < points.size(); i += 2 )
            {
                VECTOR2I pt = ScalePosSym( VECTOR2D( points[i - 1], points[i] ) );

                switch( i )
                {
                case 1: shape->SetStart( pt );    break;
                case 3: shape->SetBezierC1( pt ); break;
                case 5: shape->SetBezierC2( pt ); break;
                case 7: shape->SetEnd( pt );      break;
                }
            }

            shape->SetUnit( currentUnit );
            ApplyLineStyle( lineStyles, shape, styleStr );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( type == wxS( "POLY" ) )
        {
            std::vector<double> points = line.at( 2 );
            wxString            styleStr = line.at( 4 );

            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

            for( size_t i = 1; i < points.size(); i += 2 )
                shape->AddPoint( ScalePosSym( VECTOR2D( points[i - 1], points[i] ) ) );

            shape->SetUnit( currentUnit );
            ApplyLineStyle( lineStyles, shape, styleStr );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( type == wxS( "TEXT" ) )
        {
            VECTOR2D pos( line.at( 2 ), line.at( 3 ) );
            double   angle = line.at( 4 ).is_number() ? line.at( 4 ).get<double>() : 0.0;
            wxString textStr = line.at( 5 );
            wxString fontStyleStr = line.at( 6 );

            auto text = std::make_unique<SCH_TEXT>( ScalePosSym( pos ), UnescapeHTML( textStr ),
                                                    LAYER_DEVICE );

            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            text->SetTextAngleDegrees( angle );

            text->SetUnit( currentUnit );
            ApplyFontStyle( fontStyles, text, fontStyleStr );

            ksymbol->AddDrawItem( text.release() );
        }
        else if( type == wxS( "OBJ" ) )
        {
            VECTOR2D start, size;
            wxString mimeType, data;
            //double   angle = 0;
            int      upsideDown = 0;

            if( line.at( 3 ).is_number() )
            {
                start = VECTOR2D( line.at( 3 ), line.at( 4 ) );
                size = VECTOR2D( line.at( 5 ), line.at( 6 ) );
                //angle = line.at( 7 );
                upsideDown = line.at( 8 );

                wxString imageUrl = line.at( 9 );

                if( imageUrl.BeforeFirst( ':' ) == wxS( "data" ) )
                {
                    wxArrayString paramsArr =
                            wxSplit( imageUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                    data = imageUrl.AfterFirst( ',' );

                    if( paramsArr.size() > 0 )
                    {
                        mimeType = paramsArr[0];
                    }
                }
            }
            else if( line.at( 3 ).is_string() )
            {
                mimeType = line.at( 3 ).get<wxString>().BeforeFirst( ';' );

                start = VECTOR2D( line.at( 4 ), line.at( 5 ) );
                size = VECTOR2D( line.at( 6 ), line.at( 7 ) );
                //angle = line.at( 8 );
                data = line.at( 9 ).get<wxString>();
            }

            if( mimeType.empty() || data.empty() )
                continue;

            wxMemoryBuffer buf = wxBase64Decode( data );

            if( mimeType == wxS( "image/svg+xml" ) )
            {
                VECTOR2D offset = ScalePosSym( start );

                SVG_IMPORT_PLUGIN            svgImportPlugin;
                GRAPHICS_IMPORTER_LIB_SYMBOL libsymImporter( ksymbol, 0 );

                svgImportPlugin.SetImporter( &libsymImporter );
                svgImportPlugin.LoadFromMemory( buf );

                VECTOR2D imSize( svgImportPlugin.GetImageWidth(),
                                 svgImportPlugin.GetImageHeight() );

                VECTOR2D pixelScale( schIUScale.IUTomm( ScaleSize( size.x ) ) / imSize.x,
                                     schIUScale.IUTomm( ScaleSize( size.y ) ) / imSize.y );

                if( upsideDown )
                    pixelScale.y *= -1;

                libsymImporter.SetScale( pixelScale );

                VECTOR2D offsetMM( schIUScale.IUTomm( offset.x ), schIUScale.IUTomm( offset.y ) );

                libsymImporter.SetImportOffsetMM( offsetMM );

                svgImportPlugin.Import();

                // TODO: rotation
                for( std::unique_ptr<EDA_ITEM>& item : libsymImporter.GetItems() )
                    ksymbol->AddDrawItem( static_cast<SCH_ITEM*>( item.release() ) );
            }
            else
            {
                wxMemoryInputStream memis( buf.GetData(), buf.GetDataLen() );

                wxImage::SetDefaultLoadFlags( wxImage::GetDefaultLoadFlags()
                                              & ~wxImage::Load_Verbose );
                wxImage img;
                if( img.LoadFile( memis, mimeType ) )
                {
                    int    dimMul = img.GetWidth() * img.GetHeight();
                    double maxPixels = 30000;

                    if( dimMul > maxPixels )
                    {
                        double scale = sqrt( maxPixels / dimMul );
                        img.Rescale( img.GetWidth() * scale, img.GetHeight() * scale );
                    }

                    VECTOR2D pixelScale( ScaleSize( size.x ) / img.GetWidth(),
                                         ScaleSize( size.y ) / img.GetHeight() );

                    // TODO: rotation
                    ConvertImageToLibShapes( ksymbol, 0, img, pixelScale, ScalePosSym( start ) );
                }
            }
        }
        else if( type == wxS( "HEAD" ) )
        {
            symInfo.head = line;
        }
        else if( type == wxS( "PIN" ) )
        {
            wxString pinId = line.at( 1 );
            unitParentedLines[currentUnit][pinId].push_back( line );
        }
        else if( type == wxS( "ATTR" ) )
        {
            wxString parentId = line.at( 2 );

            if( parentId.empty() )
            {
                EASYEDAPRO::SCH_ATTR attr = line;
                unitAttributes[currentUnit].emplace( attr.key, attr );
            }
            else
            {
                unitParentedLines[currentUnit][parentId].push_back( line );
            }
        }
    }

    if( symInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT
        || symInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::NETPORT )
    {
        ksymbol->SetGlobalPower();
        ksymbol->GetReferenceField().SetText( wxS( "#PWR" ) );
        ksymbol->GetReferenceField().SetVisible( false );
        ksymbol->SetKeyWords( wxS( "power-flag" ) );
        ksymbol->SetShowPinNames( false );
        ksymbol->SetShowPinNumbers( false );

        if( auto globalNetAttr = get_opt( unitAttributes[1], wxS( "Global Net Name" ) ) )
        {
            ApplyAttrToField( fontStyles, &ksymbol->GetValueField(), *globalNetAttr, true, true );

            wxString globalNetname = globalNetAttr->value;

            if( !globalNetname.empty() )
            {
                ksymbol->SetDescription( wxString::Format(
                        _( "Power symbol creates a global label with name '%s'" ),
                        globalNetname ) );
            }
        }
    }
    else
    {
        auto designatorAttr = get_opt( unitAttributes[1], wxS( "Designator" ) );

        if( designatorAttr && !designatorAttr->value.empty() )
        {
            wxString symbolPrefix = designatorAttr->value;

            if( symbolPrefix.EndsWith( wxS( "?" ) ) )
                symbolPrefix.RemoveLast();

            ksymbol->GetReferenceField().SetText( symbolPrefix );
        }

        for( const wxString& attrName : c_attributesWhitelist )
        {
            if( auto valOpt = get_opt( aDeviceAttributes, attrName ) )
            {
                if( valOpt->empty() )
                    continue;

                SCH_FIELD* fd = ksymbol->FindFieldCaseInsensitive( attrName );

                if( !fd )
                {
                    fd = new SCH_FIELD( ksymbol, FIELD_T::USER, attrName );
                    ksymbol->AddField( fd );
                }

                wxString value = *valOpt;

                value.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true ); // ℃ -> °C

                fd->SetText( value );
                fd->SetVisible( false );
            }
        }
    }

    for( auto& [unitId, parentedLines] : unitParentedLines )
    {
        for( auto& [pinId, lines] : parentedLines )
        {
            std::optional<EASYEDAPRO::SYM_PIN>       epin;
            std::map<wxString, EASYEDAPRO::SCH_ATTR> pinAttributes;

            for( const nlohmann::json& line : lines )
            {
                wxString type = line.at( 0 );

                if( type == wxS( "ATTR" ) )
                {
                    EASYEDAPRO::SCH_ATTR attr = line;
                    pinAttributes.emplace( attr.key, attr );
                }
                else if( type == wxS( "PIN" ) )
                {
                    epin = line;
                }
            }

            if( !epin )
                continue;

            EASYEDAPRO::PIN_INFO pinInfo;
            pinInfo.pin = *epin;

            std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( ksymbol );

            pin->SetUnit( unitId );

            pin->SetLength( ScaleSize( epin->length ) );
            pin->SetPosition( ScalePosSym( epin->position ) );

            PIN_ORIENTATION orient = PIN_ORIENTATION::PIN_RIGHT;

            if( epin->rotation == 0 )
                orient = PIN_ORIENTATION::PIN_RIGHT;
            if( epin->rotation == 90 )
                orient = PIN_ORIENTATION::PIN_UP;
            if( epin->rotation == 180 )
                orient = PIN_ORIENTATION::PIN_LEFT;
            if( epin->rotation == 270 )
                orient = PIN_ORIENTATION::PIN_DOWN;

            pin->SetOrientation( orient );

            if( symInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT )
            {
                pin->SetName( ksymbol->GetName() );
                //pin->SetVisible( false );
            }
            else
            {
                auto pinNameAttr = get_opt( pinAttributes, "Pin Name" ); // JLCEDA V3

                if( !pinNameAttr )
                    pinNameAttr = get_opt( pinAttributes, "NAME" ); // EasyEDA V2

                if( pinNameAttr )
                {
                    pin->SetName( pinNameAttr->value );
                    pinInfo.name = pinNameAttr->value;

                    if( !pinNameAttr->valVisible )
                        pin->SetNameTextSize( schIUScale.MilsToIU( 1 ) );
                }
            }

            auto pinNumAttr = get_opt( pinAttributes, "Pin Number" ); // JLCEDA V3

            if( !pinNumAttr )
                pinNumAttr = get_opt( pinAttributes, "NUMBER" ); // EasyEDA V2

            if( pinNumAttr )
            {
                pin->SetNumber( pinNumAttr->value );
                pinInfo.number = pinNumAttr->value;

                if( !pinNumAttr->valVisible )
                    pin->SetNumberTextSize( schIUScale.MilsToIU( 1 ) );
            }

            if( symInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT )
            {
                pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            }
            else if( auto pinTypeAttr = get_opt( pinAttributes, "Pin Type" ) )
            {
                if( pinTypeAttr->value == wxS( "IN" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
                if( pinTypeAttr->value == wxS( "OUT" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_OUTPUT );
                if( pinTypeAttr->value == wxS( "BI" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
            }

            if( get_opt( pinAttributes, "NO_CONNECT" ) )
                pin->SetType( ELECTRICAL_PINTYPE::PT_NC );

            if( pin->GetNumberTextSize() * int( pin->GetNumber().size() ) > pin->GetLength() )
                pin->SetNumberTextSize( pin->GetLength() / pin->GetNumber().size() );

            symInfo.pins.push_back( pinInfo );
            ksymbol->AddDrawItem( pin.release() );
        }
    }

    symInfo.symbolAttr = get_opt( unitAttributes[1], "Symbol" ); // TODO: per-unit

    /*BOX2I bbox = ksymbol->GetBodyBoundingBox( 0, 0, true, true );
    bbox.Inflate( schIUScale.MilsToIU( 10 ) );*/

    /*ksymbol->GetReferenceField().SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    ksymbol->GetReferenceField().SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    ksymbol->GetReferenceField().SetPosition( VECTOR2I( bbox.GetCenter().x, -bbox.GetTop() ) );

    ksymbol->GetValueField().SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    ksymbol->GetValueField().SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    ksymbol->GetValueField().SetPosition( VECTOR2I( bbox.GetCenter().x, -bbox.GetBottom() ) );*/

    symInfo.libSymbol = std::move( ksymbolPtr );

    return symInfo;
}


void SCH_EASYEDAPRO_PARSER::ParseSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                                            const nlohmann::json&                       aProject,
                                            std::map<wxString, EASYEDAPRO::SYM_INFO>&   aSymbolMap,
                                            const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap,
                                            const std::vector<nlohmann::json>&          aLines,
                                            const wxString&                             aLibName )
{
    std::vector<std::unique_ptr<SCH_ITEM>> createdItems;

    std::map<wxString, std::vector<nlohmann::json>> parentedLines;
    std::map<wxString, std::vector<nlohmann::json>> ruleLines;

    std::map<wxString, nlohmann::json> lineStyles;
    std::map<wxString, nlohmann::json> fontStyles;

    for( const nlohmann::json& line : aLines )
    {
        wxString type = line.at( 0 );

        if( type == wxS( "LINESTYLE" ) )
            lineStyles[line.at( 1 )] = line;
        else if( type == wxS( "FONTSTYLE" ) )
            fontStyles[line.at( 1 )] = line;
    }

    for( const nlohmann::json& line : aLines )
    {
        wxString type = line.at( 0 );

        if( type == wxS( "RECT" ) )
        {
            VECTOR2D start( line.at( 2 ), line.at( 3 ) );
            VECTOR2D end( line.at( 4 ), line.at( 5 ) );
            wxString styleStr = line.at( 9 );

            std::unique_ptr<SCH_SHAPE> rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );

            rect->SetStart( ScalePos( start ) );
            rect->SetEnd( ScalePos( end ) );

            ApplyLineStyle( lineStyles, rect, styleStr );

            createdItems.push_back( std::move( rect ) );
        }
        else if( type == wxS( "CIRCLE" ) )
        {
            VECTOR2D center( line.at( 2 ), line.at( 3 ) );
            double   radius = line.at( 4 );
            wxString styleStr = line.at( 5 );

            std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );

            circle->SetCenter( ScalePos( center ) );
            circle->SetEnd( circle->GetCenter() + VECTOR2I( ScaleSize( radius ), 0 ) );

            ApplyLineStyle( lineStyles, circle, styleStr );

            createdItems.push_back( std::move( circle ) );
        }
        else if( type == wxS( "POLY" ) )
        {
            std::vector<double> points = line.at( 2 );
            wxString            styleStr = line.at( 4 );

            SHAPE_LINE_CHAIN chain;

            for( size_t i = 1; i < points.size(); i += 2 )
                chain.Append( ScalePos( VECTOR2D( points[i - 1], points[i] ) ) );

            for( int segId = 0; segId < chain.SegmentCount(); segId++ )
            {
                const SEG& seg = chain.CSegment( segId );

                std::unique_ptr<SCH_LINE> schLine =
                        std::make_unique<SCH_LINE>( seg.A, LAYER_NOTES );
                schLine->SetEndPoint( seg.B );

                ApplyLineStyle( lineStyles, schLine, styleStr );

                createdItems.push_back( std::move( schLine ) );
            }
        }
        else if( type == wxS( "TEXT" ) )
        {
            VECTOR2D pos( line.at( 2 ), line.at( 3 ) );
            double   angle = line.at( 4 ).is_number() ? line.at( 4 ).get<double>() : 0.0;
            wxString textStr = line.at( 5 );
            wxString fontStyleStr = line.at( 6 );

            std::unique_ptr<SCH_TEXT> text =
                    std::make_unique<SCH_TEXT>( ScalePos( pos ), UnescapeHTML( textStr ) );

            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

            text->SetTextAngleDegrees( angle );

            ApplyFontStyle( fontStyles, text, fontStyleStr );

            createdItems.push_back( std::move( text ) );
        }
        else if( type == wxS( "OBJ" ) )
        {
            VECTOR2D start, size;
            wxString mimeType, base64Data;
            double   angle = 0;
            int      flipped = 0;

            if( line.at( 3 ).is_number() )
            {
                start = VECTOR2D( line.at( 3 ), line.at( 4 ) );
                size = VECTOR2D( line.at( 5 ), line.at( 6 ) );
                angle = line.at( 7 );
                flipped = line.at( 8 );

                wxString imageUrl = line.at( 9 );

                if( imageUrl.BeforeFirst( ':' ) == wxS( "data" ) )
                {
                    wxArrayString paramsArr =
                            wxSplit( imageUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                    base64Data = imageUrl.AfterFirst( ',' );

                    if( paramsArr.size() > 0 )
                        mimeType = paramsArr[0];
                }
                else if( imageUrl.BeforeFirst( ':' ) == wxS( "blob" ) )
                {
                    wxString objectId = imageUrl.AfterLast( ':' );

                    if( auto blob = get_opt( aBlobMap, objectId ) )
                    {
                        wxString blobUrl = blob->url;

                        if( blobUrl.BeforeFirst( ':' ) == wxS( "data" ) )
                        {
                            wxArrayString paramsArr = wxSplit(
                                    blobUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                            base64Data = blobUrl.AfterFirst( ',' );

                            if( paramsArr.size() > 0 )
                                mimeType = paramsArr[0];
                        }
                    }
                }
            }
            else if( line.at( 3 ).is_string() )
            {
                mimeType = line.at( 3 ).get<wxString>().BeforeFirst( ';' );

                start = VECTOR2D( line.at( 4 ), line.at( 5 ) );
                size = VECTOR2D( line.at( 6 ), line.at( 7 ) );
                angle = line.at( 8 );
                base64Data = line.at( 9 ).get<wxString>();
            }

            VECTOR2D kstart = ScalePos( start );
            VECTOR2D ksize = ScaleSize( size );

            if( mimeType.empty() || base64Data.empty() )
                continue;

            wxMemoryBuffer buf = wxBase64Decode( base64Data );

            if( mimeType == wxS( "image/svg+xml" ) )
            {
                SVG_IMPORT_PLUGIN     svgImportPlugin;
                GRAPHICS_IMPORTER_SCH schImporter;

                svgImportPlugin.SetImporter( &schImporter );
                svgImportPlugin.LoadFromMemory( buf );

                VECTOR2D imSize( svgImportPlugin.GetImageWidth(),
                                 svgImportPlugin.GetImageHeight() );

                VECTOR2D pixelScale( schIUScale.IUTomm( ScaleSize( size.x ) ) / imSize.x,
                                     schIUScale.IUTomm( ScaleSize( size.y ) ) / imSize.y );

                schImporter.SetScale( pixelScale );

                VECTOR2D offsetMM( schIUScale.IUTomm( kstart.x ), schIUScale.IUTomm( kstart.y ) );

                schImporter.SetImportOffsetMM( offsetMM );

                svgImportPlugin.Import();

                for( std::unique_ptr<EDA_ITEM>& item : schImporter.GetItems() )
                {
                    SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item.release() );

                    for( double i = angle; i > 0; i -= 90 )
                    {
                        if( schItem->Type() == SCH_LINE_T )
                        {
                            // Lines need special handling for some reason
                            schItem->SetFlags( STARTPOINT );
                            schItem->Rotate( kstart, false );
                            schItem->ClearFlags( STARTPOINT );

                            schItem->SetFlags( ENDPOINT );
                            schItem->Rotate( kstart, false );
                            schItem->ClearFlags( ENDPOINT );
                        }
                        else
                        {
                            schItem->Rotate( kstart, false );
                        }
                    }

                    if( flipped )
                    {
                        // Lines need special handling for some reason
                        if( schItem->Type() == SCH_LINE_T )
                            schItem->SetFlags( STARTPOINT | ENDPOINT );

                        schItem->MirrorHorizontally( kstart.x );

                        if( schItem->Type() == SCH_LINE_T )
                            schItem->ClearFlags( STARTPOINT | ENDPOINT );
                    }

                    createdItems.emplace_back( schItem );
                }
            }
            else
            {
                std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();
                REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

                wxImage::SetDefaultLoadFlags( wxImage::GetDefaultLoadFlags()
                                              & ~wxImage::Load_Verbose );

                if( refImage.ReadImageFile( buf ) )
                {
                    VECTOR2D kcenter = kstart + ksize / 2;

                    double scaleFactor = ScaleSize( size.x ) / refImage.GetSize().x;
                    refImage.SetImageScale( scaleFactor );
                    bitmap->SetPosition( kcenter );

                    for( double i = angle; i > 0; i -= 90 )
                        bitmap->Rotate( kstart, false );

                    if( flipped )
                        bitmap->MirrorHorizontally( kstart.x );

                    createdItems.push_back( std::move( bitmap ) );
                }
            }
        }
        if( type == wxS( "WIRE" ) )
        {
            wxString wireId = line.at( 1 );
            parentedLines[wireId].push_back( line );
        }
        else if( type == wxS( "COMPONENT" ) )
        {
            wxString compId = line.at( 1 );
            parentedLines[compId].push_back( line );
        }
        else if( type == wxS( "ATTR" ) )
        {
            wxString compId = line.at( 2 );
            parentedLines[compId].push_back( line );
        }
    }

    for( auto& [parentId, lines] : parentedLines )
    {
        std::optional<EASYEDAPRO::SCH_COMPONENT> component;
        std::optional<EASYEDAPRO::SCH_WIRE>      wire;
        std::map<wxString, EASYEDAPRO::SCH_ATTR> attributes;

        for( const nlohmann::json& line : lines )
        {
            if( line.at( 0 ) == "COMPONENT" )
            {
                component = line;
            }
            else if( line.at( 0 ) == "WIRE" )
            {
                wire = line;
            }
            else if( line.at( 0 ) == "ATTR" )
            {
                EASYEDAPRO::SCH_ATTR attr = line;
                attributes.emplace( attr.key, attr );
            }
        }

        if( component )
        {
            auto deviceAttr = get_opt( attributes, "Device" );
            auto symbolAttr = get_opt( attributes, "Symbol" );

            if( !deviceAttr )
                continue;

            std::map<wxString, wxString> compAttrs = EASYEDAPRO::AnyMapToStringMap(
                    aProject.at( "devices" ).at( deviceAttr->value ).at( "attributes" ) );

            wxString symbolId;

            if( symbolAttr && !symbolAttr->value.IsEmpty() )
                symbolId = symbolAttr->value;
            else
                symbolId = compAttrs.at( "Symbol" );

            auto it = aSymbolMap.find( symbolId );
            if( it == aSymbolMap.end() )
            {
                wxLogError( "Symbol of '%s' with uuid '%s' not found.", component->name, symbolId );
                continue;
            }

            EASYEDAPRO::SYM_INFO& esymInfo = it->second;
            LIB_SYMBOL            newLibSymbol = *esymInfo.libSymbol.get();

            wxString unitName = component->name;

            LIB_ID libId = EASYEDAPRO::ToKiCadLibID( aLibName,
                                                     newLibSymbol.GetLibId().GetLibItemName() );

            auto schSym = std::make_unique<SCH_SYMBOL>( newLibSymbol, libId,
                                                        &aSchematic->CurrentSheet(),
                                                        esymInfo.partUnits[unitName] );

            schSym->SetFootprintFieldText( newLibSymbol.GetFootprint() );

            for( double i = component->rotation; i > 0; i -= 90 )
                schSym->Rotate( VECTOR2I(), true );

            if( component->mirror )
                schSym->MirrorHorizontally( 0 );

            schSym->SetPosition( ScalePos( component->position ) );

            if( esymInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT )
            {
                SCH_FIELD* valueField = schSym->GetField( FIELD_T::VALUE );

                auto     globalNetNameAttr = get_opt( attributes, "Global Net Name" );
                wxString globalNetNameFromProject = get_def( compAttrs, "Global Net Name", wxEmptyString );
                wxString globalNetName;

                // 1. Pick from schematic attr
                // 2. Pick from project.json
                // 3. Pick from symbol
                if( globalNetNameAttr && !globalNetNameAttr->value.IsEmpty() )
                {
                    globalNetName = globalNetNameAttr->value;

                    ApplyAttrToField( fontStyles, schSym->GetField( FIELD_T::VALUE ), *globalNetNameAttr, false, true,
                                      compAttrs, schSym.get() );
                }
                else if( !globalNetNameFromProject.IsEmpty() )
                {
                    globalNetName = globalNetNameFromProject;

                    valueField->SetText( ResolveFieldVariables( globalNetName, compAttrs ) );
                }
                else
                {
                    valueField->SetText( newLibSymbol.GetValueField().GetText() );
                }

                for( SCH_PIN* pin : schSym->GetAllLibPins() )
                    pin->SetName( globalNetName );

                schSym->SetRef( &aSchematic->CurrentSheet(), wxS( "#PWR?" ) );
                schSym->GetField( FIELD_T::REFERENCE )->SetVisible( false );
            }
            else if( esymInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::NETPORT )
            {
                auto nameAttr = get_opt( attributes, "Name" );

                wxString netName;

                if( nameAttr && !nameAttr->value.IsEmpty() )
                    netName = nameAttr->value;
                else
                    netName = compAttrs.at( "Name" );

                std::unique_ptr<SCH_GLOBALLABEL> label = std::make_unique<SCH_GLOBALLABEL>(
                        ScalePos( component->position ), netName );

                std::vector<SCH_PIN*> pins = schSym->GetPins( &aSchematic->CurrentSheet() );

                if( pins.size() > 0 )
                {
                    switch( pins[0]->GetType() )
                    {
                    case ELECTRICAL_PINTYPE::PT_INPUT:
                        label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
                        break;
                    case ELECTRICAL_PINTYPE::PT_OUTPUT:
                        label->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
                        break;
                    case ELECTRICAL_PINTYPE::PT_BIDI:
                        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
                        break;
                    default:
                        break;
                    }
                }

                BOX2I bbox = schSym->GetBodyAndPinsBoundingBox();
                bbox.Offset( -schSym->GetPosition() );
                VECTOR2I bboxCenter = bbox.GetCenter();

                SPIN_STYLE spin = SPIN_STYLE::LEFT;

                if( std::abs( bboxCenter.x ) >= std::abs( bboxCenter.y ) )
                {
                    if( bboxCenter.x >= 0 )
                        spin = SPIN_STYLE::RIGHT;
                    else
                        spin = SPIN_STYLE::LEFT;
                }
                else
                {
                    if( bboxCenter.y >= 0 )
                        spin = SPIN_STYLE::BOTTOM;
                    else
                        spin = SPIN_STYLE::UP;
                }

                label->SetSpinStyle( spin );

                if( nameAttr )
                {
                    nlohmann::json style = fontStyles[nameAttr->fontStyle];

                    if( !style.is_null() && style.at( 5 ).is_number() )
                    {
                        double size = style.at( 5 ).get<double>() * 0.62;
                        label->SetTextSize( VECTOR2I( ScaleSize( size ), ScaleSize( size ) ) );
                    }
                }

                createdItems.push_back( std::move( label ) );

                continue;
            }
            else
            {
                for( const wxString& attrKey : c_attributesWhitelist )
                {
                    if( auto valOpt = get_opt( compAttrs, attrKey ) )
                    {
                        if( valOpt->empty() )
                            continue;

                        SCH_FIELD* text = schSym->FindFieldCaseInsensitive( attrKey );

                        if( !text )
                        {
                            text = new SCH_FIELD( schSym.get(), FIELD_T::USER, attrKey );
                            schSym->AddField( text );
                        }

                        wxString value = *valOpt;

                        value.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true ); // ℃ -> °C

                        text->SetText( value );
                        text->SetVisible( false );
                    }
                }

                auto nameAttr = get_opt( attributes, "Name" );
                auto valueAttr = get_opt( attributes, "Value" );

                if( valueAttr && valueAttr->value.empty() )
                    valueAttr->value = get_def( compAttrs, "Value", wxString() );

                if( nameAttr && nameAttr->value.empty() )
                    nameAttr->value = get_def( compAttrs, "Name", wxString() );

                std::optional<EASYEDAPRO::SCH_ATTR> targetValueAttr;

                if( valueAttr && !valueAttr->value.empty() && valueAttr->valVisible )
                    targetValueAttr = valueAttr;
                else if( nameAttr && !nameAttr->value.empty() && nameAttr->valVisible )
                    targetValueAttr = nameAttr;
                else if( valueAttr && !valueAttr->value.empty() )
                    targetValueAttr = valueAttr;
                else if( nameAttr && !nameAttr->value.empty() )
                    targetValueAttr = nameAttr;

                if( targetValueAttr )
                {
                    ApplyAttrToField( fontStyles, schSym->GetField( FIELD_T::VALUE ),
                                      *targetValueAttr, false, true, compAttrs, schSym.get() );
                }

                if( auto descrAttr = get_opt( attributes, "Description" ) )
                {
                    ApplyAttrToField( fontStyles, schSym->GetField( FIELD_T::DESCRIPTION ),
                                      *descrAttr, false, true, compAttrs, schSym.get() );
                }

                if( auto designatorAttr = get_opt( attributes, "Designator" ) )
                {
                    ApplyAttrToField( fontStyles, schSym->GetField( FIELD_T::REFERENCE ),
                                      *designatorAttr, false, true, compAttrs, schSym.get() );

                    schSym->SetRef( &aSchematic->CurrentSheet(), designatorAttr->value );
                }

                for( auto& [attrKey, attr] : attributes )
                {
                    if( attrKey == wxS( "Name" ) || attrKey == wxS( "Value" )
                        || attrKey == wxS( "Global Net Name" ) || attrKey == wxS( "Designator" )
                        || attrKey == wxS( "Description" ) || attrKey == wxS( "Device" )
                        || attrKey == wxS( "Footprint" ) || attrKey == wxS( "Symbol" )
                        || attrKey == wxS( "Unique ID" ) )
                    {
                        continue;
                    }

                    if( attr.value.IsEmpty() )
                        continue;

                    SCH_FIELD* text = schSym->FindFieldCaseInsensitive( attrKey );

                    if( !text )
                    {
                        text = new SCH_FIELD( schSym.get(), FIELD_T::USER, attrKey );
                        schSym->AddField( text );
                    }

                    text->SetPosition( schSym->GetPosition() );

                    ApplyAttrToField( fontStyles, text, attr, false, true, compAttrs,
                                      schSym.get() );
                }
            }

            for( const EASYEDAPRO::PIN_INFO& pinInfo : esymInfo.pins )
            {
                wxString pinKey = parentId + pinInfo.pin.id;
                auto     pinLines = get_opt( parentedLines, pinKey );

                if( !pinLines )
                    continue;

                for( const nlohmann::json& pinLine : *pinLines )
                {
                    if( pinLine.at( 0 ) != "ATTR" )
                        continue;

                    EASYEDAPRO::SCH_ATTR attr = pinLine;

                    if( attr.key != wxS( "NO_CONNECT" ) )
                        continue;

                    if( SCH_PIN* schPin = schSym->GetPin( pinInfo.number ) )
                    {
                        VECTOR2I pos = schSym->GetPinPhysicalPosition( schPin->GetLibPin() );

                        std::unique_ptr<SCH_NO_CONNECT> noConn =
                                std::make_unique<SCH_NO_CONNECT>( pos );

                        createdItems.push_back( std::move( noConn ) );
                    }
                }
            }

            createdItems.push_back( std::move( schSym ) );
        }
        else // Not component
        {
            std::vector<SHAPE_LINE_CHAIN> wireLines;

            if( wire )
            {
                for( const std::vector<double>& ptArr : wire->geometry )
                {
                    SHAPE_LINE_CHAIN chain;

                    for( size_t i = 1; i < ptArr.size(); i += 2 )
                        chain.Append( ScalePos( VECTOR2D( ptArr[i - 1], ptArr[i] ) ) );

                    if( chain.PointCount() < 2 )
                        continue;

                    wireLines.push_back( chain );

                    for( int segId = 0; segId < chain.SegmentCount(); segId++ )
                    {
                        const SEG& seg = chain.CSegment( segId );

                        std::unique_ptr<SCH_LINE> schLine =
                                std::make_unique<SCH_LINE>( seg.A, LAYER_WIRE );
                        schLine->SetEndPoint( seg.B );

                        createdItems.push_back( std::move( schLine ) );
                    }
                }
            }

            auto netAttr = get_opt( attributes, "NET" );

            if( netAttr )
            {
                if( !netAttr->valVisible || netAttr->value.IsEmpty() )
                    continue;

                VECTOR2I    kpos = ScalePos( *netAttr->position );
                VECTOR2I    nearestPos = kpos;
                SEG::ecoord min_dist_sq = VECTOR2I::ECOORD_MAX;

                for( const SHAPE_LINE_CHAIN& chain : wireLines )
                {
                    VECTOR2I    nearestPt = chain.NearestPoint( kpos, false );
                    SEG::ecoord dist_sq = ( nearestPt - kpos ).SquaredEuclideanNorm();

                    if( dist_sq < min_dist_sq )
                    {
                        min_dist_sq = dist_sq;
                        nearestPos = nearestPt;
                    }
                }

                std::unique_ptr<SCH_LABEL> label = std::make_unique<SCH_LABEL>();

                label->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                label->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

                for( double i = netAttr->rotation; i > 0; i -= 90 )
                    label->Rotate90( true );

                label->SetPosition( nearestPos );
                label->SetText( netAttr->value );

                ApplyFontStyle( fontStyles, label, netAttr->fontStyle );

                createdItems.push_back( std::move( label ) );
            }
        }
    }

    // Adjust page to content
    BOX2I sheetBBox;

    for( std::unique_ptr<SCH_ITEM>& ptr : createdItems )
    {
        if( ptr->Type() == SCH_SYMBOL_T )
            sheetBBox.Merge( static_cast<SCH_SYMBOL*>( ptr.get() )->GetBodyAndPinsBoundingBox() );
        else
            sheetBBox.Merge( ptr->GetBoundingBox() );
    }

    SCH_SCREEN* screen = aRootSheet->GetScreen();
    PAGE_INFO   pageInfo = screen->GetPageSettings();

    int alignGrid = schIUScale.MilsToIU( 50 );

    VECTOR2D offset( -sheetBBox.GetLeft(), -sheetBBox.GetTop() );
    offset.x = KiROUND( offset.x / alignGrid ) * alignGrid;
    offset.y = KiROUND( offset.y / alignGrid ) * alignGrid;

    pageInfo.SetWidthMils( schIUScale.IUToMils( sheetBBox.GetWidth() ) );
    pageInfo.SetHeightMils( schIUScale.IUToMils( sheetBBox.GetHeight() ) );

    screen->SetPageSettings( pageInfo );

    for( std::unique_ptr<SCH_ITEM>& ptr : createdItems )
    {
        ptr->Move( offset );
        screen->Append( ptr.release() );
    }
}
