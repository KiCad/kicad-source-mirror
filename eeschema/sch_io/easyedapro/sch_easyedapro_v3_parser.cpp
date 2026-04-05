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

#include "sch_easyedapro_v3_parser.h"

#include <io/easyedapro/easyedapro_import_utils.h>

#include <core/map_helpers.h>

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_no_connect.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_shape.h>
#include <string_utils.h>
#include <bezier_curves.h>
#include <wx/base64.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <gfx_import_utils.h>
#include <import_gfx/svg_import_plugin.h>
#include <import_gfx/graphics_importer_lib_symbol.h>
#include <import_gfx/graphics_importer_sch.h>

using namespace EASYEDAPRO;


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


static LINE_STYLE ConvertStrokeStyle( const nlohmann::json& aStyle )
{
    int dash = 0;

    if( aStyle.is_number_integer() )
        dash = aStyle.get<int>();
    else if( aStyle.is_string() )
    {
        wxString value = aStyle.get<wxString>();

        if( value == wxS( "DASH" ) )
            dash = 1;
        else if( value == wxS( "DOT" ) )
            dash = 2;
        else if( value == wxS( "DASH_DOT" ) || value == wxS( "DASHDOT" ) )
            dash = 3;
    }

    if( dash == 1 )
        return LINE_STYLE::DASH;
    else if( dash == 2 )
        return LINE_STYLE::DOT;
    else if( dash == 3 )
        return LINE_STYLE::DASHDOT;

    return LINE_STYLE::SOLID;
}


static int AlignToFontV( const wxString& aAlign )
{
    if( aAlign.Contains( wxS( "TOP" ) ) )
        return 0;

    if( aAlign.Contains( wxS( "CENTER" ) ) || aAlign.Contains( wxS( "MIDDLE" ) ) )
        return 1;

    return 2; // BOTTOM
}


static int AlignToFontH( const wxString& aAlign )
{
    if( aAlign.StartsWith( wxS( "LEFT" ) ) )
        return 0;

    if( aAlign.StartsWith( wxS( "CENTER" ) ) )
        return 1;

    return 2; // RIGHT
}


template <typename T>
static VECTOR2<T> V3ScalePos( VECTOR2<T> aValue )
{
    return VECTOR2<T>( SCH_EASYEDAPRO_PARSER::ScaleSize( aValue.x ),
                       SCH_EASYEDAPRO_PARSER::ScaleSize( aValue.y ) );
}


template <typename T>
static VECTOR2<T> V3ScalePosSym( VECTOR2<T> aValue )
{
    return VECTOR2<T>( SCH_EASYEDAPRO_PARSER::ScaleSize( aValue.x ),
                       SCH_EASYEDAPRO_PARSER::ScaleSize( aValue.y ) );
}


static constexpr double c_v3TextSizeScale = 0.62;


template <typename T>
static void ApplyV3TextSizeIfDefined( T& aText, const nlohmann::json& aInner )
{
    if( EASYEDAPRO::V3IsNullOrMissing( aInner, "fontSize" ) )
        return;

    double fontSize = EASYEDAPRO::V3GetDouble( aInner, "fontSize", 0.0 );

    if( fontSize <= 0.0 )
        return;

    double scaledSize = fontSize * c_v3TextSizeScale;

    aText->SetTextSize( VECTOR2I( SCH_EASYEDAPRO_PARSER::ScaleSize( scaledSize ),
                                  SCH_EASYEDAPRO_PARSER::ScaleSize( scaledSize ) ) );
}


static wxString ResolveV3FootprintText( const wxString& aFootprintValue,
                                        const nlohmann::json& aProject,
                                        const wxString& aLibName )
{
    if( aFootprintValue.empty() )
        return wxEmptyString;

    if( aFootprintValue.Contains( wxS( ":" ) ) )
        return aFootprintValue;

    std::string footprintUuid = std::string( aFootprintValue.ToUTF8() );

    if( aProject.contains( "footprints" ) && aProject.at( "footprints" ).is_object()
        && aProject.at( "footprints" ).contains( footprintUuid ) )
    {
        const nlohmann::json& footprint = aProject.at( "footprints" ).at( footprintUuid );
        wxString              footprintTitle;

        if( footprint.is_object() )
            footprintTitle = EASYEDAPRO::V3GetString( footprint, "title" );
        else
            footprintTitle = EASYEDAPRO::V3JsonToString( footprint );

        if( !footprintTitle.empty() )
            return aLibName + wxS( ":" ) + footprintTitle;
    }

    return aFootprintValue;
}


SCH_EASYEDAPRO_V3_PARSER::SCH_EASYEDAPRO_V3_PARSER( SCHEMATIC*         aSchematic,
                                                      PROGRESS_REPORTER* aProgressReporter )
{
    m_schematic = aSchematic;
}


template <typename T>
void SCH_EASYEDAPRO_V3_PARSER::ApplyV3FontStyle( T& text, const nlohmann::json& aInner,
                                                  const char* aAlignField )
{
    wxString color = V3GetString( aInner, "color", wxEmptyString );
    wxString fontFamily = V3GetString( aInner, "fontFamily", wxS( "default" ) );
    wxString align = V3GetString( aInner, aAlignField, wxS( "LEFT_BOTTOM" ) );

    if( !color.empty() && color.StartsWith( wxS( "#" ) ) )
    {
        COLOR4D c( color );
        text->SetTextColor( c );
    }

    if( fontFamily != wxS( "Arial" ) && !fontFamily.IsSameAs( wxS( "default" ), false ) )
        text->SetFont( KIFONT::FONT::GetFont( fontFamily ) );

    ApplyV3TextSizeIfDefined( text, aInner );

    int vAlign = AlignToFontV( align );
    int hAlign = AlignToFontH( align );

    if( !text->GetText().Contains( wxS( "\n" ) ) )
    {
        if( vAlign == 0 )
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( vAlign == 1 )
            text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        else
            text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }
    else
    {
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    }

    if( hAlign == 0 )
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( hAlign == 1 )
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
}


template <typename T>
void SCH_EASYEDAPRO_V3_PARSER::ApplyV3LineStyle( T& shape, const nlohmann::json& aInner )
{
    STROKE_PARAMS stroke = shape->GetStroke();

    wxString colorStr = V3GetString( aInner, "strokeColor", wxEmptyString );

    if( !colorStr.empty() && colorStr.StartsWith( wxS( "#" ) ) )
    {
        COLOR4D color( colorStr );
        stroke.SetColor( color );
    }

    stroke.SetLineStyle( ConvertStrokeStyle( aInner.value( "strokeStyle", nlohmann::json() ) ) );

    double width = V3GetDouble( aInner, "strokeWidth", 0.0 );

    if( width != 0 )
        stroke.SetWidth( SCH_EASYEDAPRO_PARSER::ScaleSize( width ) );

    shape->SetStroke( stroke );
}


static nlohmann::json FlattenPoints( const nlohmann::json& aPoints )
{
    nlohmann::json result = nlohmann::json::array();

    if( !aPoints.is_array() )
        return result;

    for( const nlohmann::json& point : aPoints )
    {
        if( point.is_number() )
        {
            result.push_back( point );
            continue;
        }

        if( point.is_array() && point.size() >= 2 )
        {
            result.push_back( point[0] );
            result.push_back( point[1] );
            continue;
        }

        if( point.is_object() )
        {
            if( point.contains( "x" ) && point.contains( "y" ) )
            {
                result.push_back( point.at( "x" ) );
                result.push_back( point.at( "y" ) );
            }
        }
    }

    return result;
}


EASYEDAPRO::SYM_INFO
SCH_EASYEDAPRO_V3_PARSER::ParseSymbol( const EASYEDAPRO::V3_DOC_RAW&       aDoc,
                                        const std::map<wxString, wxString>&  aDeviceAttributes,
                                        const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap )
{
    EASYEDAPRO::SYM_INFO symInfo;

    std::unique_ptr<LIB_SYMBOL> ksymbolPtr = std::make_unique<LIB_SYMBOL>( wxEmptyString );
    LIB_SYMBOL*                 ksymbol = ksymbolPtr.get();

    std::map<wxString, int> partUnits;

    std::map<int, std::map<wxString, EASYEDAPRO::SCH_ATTR>>       unitAttributes;
    std::map<int, std::map<wxString, const V3_ROW*>>               unitAttributeRows;
    std::map<int, std::map<wxString, std::vector<const V3_ROW*>>>  unitParentedRows;

    int totalUnits = 0;

    // First pass: collect PART units and HEAD
    for( const V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "PART" ) )
        {
            int unitId = ++totalUnits;

            partUnits[row.id] = unitId;

            wxString partTitle = V3GetString( row.inner, "title" );

            if( !partTitle.empty() )
                partUnits[partTitle] = unitId;
        }
        else if( row.type == wxS( "META" ) )
        {
            if( row.inner.contains( "docType" ) )
                symInfo.head.symbolType = static_cast<EASYEDAPRO::SYMBOL_TYPE>(
                        V3GetInt( row.inner, "docType",
                                  static_cast<int>( EASYEDAPRO::SYMBOL_TYPE::NORMAL ) ) );
        }
    }

    symInfo.partUnits = partUnits;
    ksymbol->SetUnitCount( totalUnits, false );

    int currentUnit = 1;

    auto resolveRowUnit = [&]( const V3_ROW& aRow )
    {
        wxString partId = V3GetString( aRow.inner, "partId" );

        if( !partId.empty() )
        {
            if( auto unitOpt = get_opt( partUnits, partId ) )
                return *unitOpt;
        }

        return currentUnit;
    };

    // Second pass: create shapes and collect pins/attrs
    for( const V3_ROW& row : aDoc.rows )
    {
        int rowUnit = resolveRowUnit( row );

        if( row.type == wxS( "PART" ) )
        {
            currentUnit = partUnits.at( row.id );
        }
        else if( row.type == wxS( "RECT" ) )
        {
            VECTOR2D start( V3GetDouble( row.inner, "dotX1" ),
                            V3GetDouble( row.inner, "dotY1" ) );
            VECTOR2D end( V3GetDouble( row.inner, "dotX2" ),
                          V3GetDouble( row.inner, "dotY2" ) );

            auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );

            rect->SetStart( V3ScalePosSym( start ) );
            rect->SetEnd( V3ScalePosSym( end ) );
            rect->SetUnit( rowUnit );
            ApplyV3LineStyle( rect, row.inner );

            ksymbol->AddDrawItem( rect.release() );
        }
        else if( row.type == wxS( "CIRCLE" ) )
        {
            VECTOR2D center( V3GetDouble( row.inner, "centerX" ),
                             V3GetDouble( row.inner, "centerY" ) );
            double   radius = V3GetDouble( row.inner, "radius" );

            auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );

            circle->SetCenter( V3ScalePosSym( center ) );
            circle->SetEnd( circle->GetCenter()
                            + VECTOR2I( SCH_EASYEDAPRO_PARSER::ScaleSize( radius ), 0 ) );
            circle->SetUnit( rowUnit );
            ApplyV3LineStyle( circle, row.inner );

            ksymbol->AddDrawItem( circle.release() );
        }
        else if( row.type == wxS( "ARC" ) )
        {
            VECTOR2D start( V3GetDouble( row.inner, "startX" ),
                            V3GetDouble( row.inner, "startY" ) );
            VECTOR2D mid( V3GetDouble( row.inner, "referX" ),
                          V3GetDouble( row.inner, "referY" ) );
            VECTOR2D end( V3GetDouble( row.inner, "endX" ),
                          V3GetDouble( row.inner, "endY" ) );

            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );

            shape->SetArcGeometry( V3ScalePosSym( start ),
                                   V3ScalePosSym( mid ),
                                   V3ScalePosSym( end ) );
            shape->SetUnit( rowUnit );
            ApplyV3LineStyle( shape, row.inner );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( row.type == wxS( "BEZIER" ) )
        {
            std::vector<double> points = row.inner.value( "controls", nlohmann::json::array() );

            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER, LAYER_DEVICE );

            for( size_t i = 1; i < points.size(); i += 2 )
            {
                VECTOR2I pt = V3ScalePosSym( VECTOR2D( points[i - 1], points[i] ) );

                switch( i )
                {
                case 1: shape->SetStart( pt );    break;
                case 3: shape->SetBezierC1( pt ); break;
                case 5: shape->SetBezierC2( pt ); break;
                case 7: shape->SetEnd( pt );      break;
                }
            }

            shape->SetUnit( rowUnit );
            ApplyV3LineStyle( shape, row.inner );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( row.type == wxS( "POLY" ) )
        {
            nlohmann::json flatPts = FlattenPoints(
                    row.inner.value( "points", nlohmann::json::array() ) );
            std::vector<double> points = flatPts;

            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

            for( size_t i = 1; i < points.size(); i += 2 )
                shape->AddPoint( V3ScalePosSym( VECTOR2D( points[i - 1], points[i] ) ) );

            shape->SetUnit( rowUnit );
            ApplyV3LineStyle( shape, row.inner );

            ksymbol->AddDrawItem( shape.release() );
        }
        else if( row.type == wxS( "TEXT" ) )
        {
            VECTOR2D pos( V3GetDouble( row.inner, "x" ), V3GetDouble( row.inner, "y" ) );
            double   angle = V3GetDouble( row.inner, "rotation" );
            wxString textStr = V3GetString( row.inner, "value" );

            auto text = std::make_unique<SCH_TEXT>(
                    V3ScalePosSym( pos ), UnescapeHTML( textStr ),
                    LAYER_DEVICE );

            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            text->SetTextAngleDegrees( angle );
            text->SetUnit( rowUnit );
            ApplyV3FontStyle( text, row.inner );

            ksymbol->AddDrawItem( text.release() );
        }
        else if( row.type == wxS( "OBJ" ) )
        {
            VECTOR2D start( V3GetDouble( row.inner, "startX" ),
                            V3GetDouble( row.inner, "startY" ) );
            VECTOR2D size( V3GetDouble( row.inner, "width" ),
                           V3GetDouble( row.inner, "height" ) );
            int      upsideDown = V3GetBool( row.inner, "isMirror" ) ? 1 : 0;

            wxString imageUrl = V3GetString( row.inner, "content" );
            wxString mimeType, data;

            if( imageUrl.BeforeFirst( ':' ) == wxS( "data" ) )
            {
                wxArrayString paramsArr =
                        wxSplit( imageUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                data = imageUrl.AfterFirst( ',' );

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
                        wxArrayString paramsArr =
                                wxSplit( blobUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                        data = blobUrl.AfterFirst( ',' );

                        if( paramsArr.size() > 0 )
                            mimeType = paramsArr[0];
                    }
                }
            }

            if( mimeType.empty() || data.empty() )
                continue;

            wxMemoryBuffer buf = wxBase64Decode( data );

            if( mimeType == wxS( "image/svg+xml" ) )
            {
                VECTOR2D offset = V3ScalePosSym( start );

                SVG_IMPORT_PLUGIN            svgImportPlugin;
                GRAPHICS_IMPORTER_LIB_SYMBOL libsymImporter( ksymbol, 0 );

                svgImportPlugin.SetImporter( &libsymImporter );
                svgImportPlugin.LoadFromMemory( buf );

                VECTOR2D imSize( svgImportPlugin.GetImageWidth(),
                                 svgImportPlugin.GetImageHeight() );

                VECTOR2D pixelScale(
                        schIUScale.IUTomm( SCH_EASYEDAPRO_PARSER::ScaleSize( size.x ) )
                                / imSize.x,
                        schIUScale.IUTomm( SCH_EASYEDAPRO_PARSER::ScaleSize( size.y ) )
                                / imSize.y );

                if( upsideDown )
                    pixelScale.y *= -1;

                libsymImporter.SetScale( pixelScale );

                VECTOR2D offsetMM( schIUScale.IUTomm( offset.x ),
                                   schIUScale.IUTomm( offset.y ) );

                libsymImporter.SetImportOffsetMM( offsetMM );
                svgImportPlugin.Import();

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

                    VECTOR2D pixelScale(
                            SCH_EASYEDAPRO_PARSER::ScaleSize( size.x ) / img.GetWidth(),
                            SCH_EASYEDAPRO_PARSER::ScaleSize( size.y ) / img.GetHeight() );

                    ConvertImageToLibShapes( ksymbol, 0, img, pixelScale,
                                            V3ScalePosSym( start ) );
                }
            }
        }
        else if( row.type == wxS( "PIN" ) )
        {
            unitParentedRows[rowUnit][row.id].push_back( &row );
        }
        else if( row.type == wxS( "ATTR" ) )
        {
            wxString parentId = V3GetString( row.inner, "parentId" );

            if( parentId.empty() )
            {
                EASYEDAPRO::SCH_ATTR attr;
                attr.key = V3GetString( row.inner, "key" );
                attr.value = V3JsonToString( row.inner.value( "value", nlohmann::json() ) );
                attr.keyVisible = V3GetBool( row.inner, "keyVisible" );
                attr.valVisible = V3GetBool( row.inner, "valueVisible" );

                if( !V3IsNullOrMissing( row.inner, "x" ) )
                {
                    attr.position = VECTOR2D( V3GetDouble( row.inner, "x" ),
                                              V3GetDouble( row.inner, "y" ) );
                }

                attr.rotation = V3GetDouble( row.inner, "rotation" );
                attr.fontStyle = wxEmptyString; // v3 has inline styles

                unitAttributes[rowUnit].emplace( attr.key, attr );
                unitAttributeRows[rowUnit].emplace( attr.key, &row );
            }
            else
            {
                unitParentedRows[rowUnit][parentId].push_back( &row );
            }
        }
    }

    // Apply symbol type attributes
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
            wxString powerName = globalNetAttr->value;

            if( powerName.IsEmpty() )
            {
                if( auto nameAttr = get_opt( unitAttributes[1], wxS( "Name" ) ) )
                    powerName = nameAttr->value;
            }

            EDA_TEXT* valueText = static_cast<EDA_TEXT*>( &ksymbol->GetValueField() );
            valueText->SetText( powerName );
            valueText->SetVisible( globalNetAttr->valVisible );

            if( auto globalNetAttrRow = get_opt( unitAttributeRows[1], wxS( "Global Net Name" ) ) )
            {
                const nlohmann::json& attrInner = ( *globalNetAttrRow )->inner;

                wxString color = V3GetString( attrInner, "color", wxEmptyString );
                wxString fontFamily = V3GetString( attrInner, "fontFamily", wxS( "default" ) );
                wxString align = V3GetString( attrInner, "align", wxS( "LEFT_BOTTOM" ) );

                if( !color.empty() && color.StartsWith( wxS( "#" ) ) )
                {
                    COLOR4D c( color );
                    valueText->SetTextColor( c );
                }

                if( fontFamily != wxS( "Arial" ) && !fontFamily.IsSameAs( wxS( "default" ), false ) )
                    valueText->SetFont( KIFONT::FONT::GetFont( fontFamily ) );

                ApplyV3TextSizeIfDefined( valueText, attrInner );

                int vAlign = AlignToFontV( align );
                int hAlign = AlignToFontH( align );

                if( vAlign == 0 )
                    valueText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                else if( vAlign == 1 )
                    valueText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
                else
                    valueText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

                if( hAlign == 0 )
                    valueText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                else if( hAlign == 1 )
                    valueText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                else
                    valueText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            }

            if( globalNetAttr->position )
                ksymbol->GetValueField().SetPosition( V3ScalePosSym( *globalNetAttr->position ) );

            wxString globalNetname = globalNetAttr->value;

            if( globalNetname.IsEmpty() )
                globalNetname = powerName;

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
                value.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true );

                fd->SetText( value );
                fd->SetVisible( false );
            }
        }
    }

    // Process pins
    for( auto& [unitId, parentedRows] : unitParentedRows )
    {
        for( auto& [pinId, rows] : parentedRows )
        {
            const V3_ROW*                            pinRow = nullptr;
            std::map<wxString, EASYEDAPRO::SCH_ATTR> pinAttributes;

            for( const V3_ROW* r : rows )
            {
                if( r->type == wxS( "ATTR" ) )
                {
                    EASYEDAPRO::SCH_ATTR attr;
                    attr.key = V3GetString( r->inner, "key" );
                    attr.value = V3JsonToString( r->inner.value( "value", nlohmann::json() ) );
                    attr.keyVisible = V3GetBool( r->inner, "keyVisible" );
                    attr.valVisible = V3GetBool( r->inner, "valueVisible" );
                    pinAttributes.emplace( attr.key, attr );

                }
                else if( r->type == wxS( "PIN" ) )
                {
                    pinRow = r;
                }
            }

            if( !pinRow )
                continue;

            EASYEDAPRO::PIN_INFO pinInfo;
            pinInfo.pin.position = VECTOR2D( V3GetDouble( pinRow->inner, "x" ),
                                             V3GetDouble( pinRow->inner, "y" ) );
            pinInfo.pin.length = V3GetDouble( pinRow->inner, "length" );
            pinInfo.pin.rotation = V3GetDouble( pinRow->inner, "rotation" );

            wxString pinShape = V3GetString( pinRow->inner, "pinShape" );
            pinInfo.pin.inverted = ( pinShape == wxS( "INVERTED" )
                                     || pinShape == wxS( "INVERTED_CLOCK" ) );

            std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( ksymbol );

            pin->SetUnit( unitId );
            pin->SetLength( SCH_EASYEDAPRO_PARSER::ScaleSize( pinInfo.pin.length ) );
            pin->SetPosition( V3ScalePosSym( pinInfo.pin.position ) );

            PIN_ORIENTATION orient = PIN_ORIENTATION::PIN_RIGHT;

            if( pinInfo.pin.rotation == 0 )
                orient = PIN_ORIENTATION::PIN_RIGHT;
            if( pinInfo.pin.rotation == 90 )
                orient = PIN_ORIENTATION::PIN_UP;
            if( pinInfo.pin.rotation == 180 )
                orient = PIN_ORIENTATION::PIN_LEFT;
            if( pinInfo.pin.rotation == 270 )
                orient = PIN_ORIENTATION::PIN_DOWN;

            pin->SetOrientation( orient );

            if( symInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT )
            {
                pin->SetName( ksymbol->GetName() );
            }
            else
            {
                auto pinNameAttr = get_opt( pinAttributes, wxS( "Pin Name" ) );

                if( !pinNameAttr )
                    pinNameAttr = get_opt( pinAttributes, wxS( "NAME" ) );

                if( pinNameAttr )
                {
                    pin->SetName( pinNameAttr->value );
                    pinInfo.name = pinNameAttr->value;

                    if( !pinNameAttr->valVisible )
                        pin->SetNameTextSize( schIUScale.MilsToIU( 1 ) );
                }
            }

            auto pinNumAttr = get_opt( pinAttributes, wxS( "Pin Number" ) );

            if( !pinNumAttr )
                pinNumAttr = get_opt( pinAttributes, wxS( "NUMBER" ) );

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
            else if( auto pinTypeAttr = get_opt( pinAttributes, wxS( "Pin Type" ) ) )
            {
                if( pinTypeAttr->value == wxS( "IN" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
                if( pinTypeAttr->value == wxS( "OUT" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_OUTPUT );
                if( pinTypeAttr->value == wxS( "BI" ) )
                    pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
            }

            if( get_opt( pinAttributes, wxS( "NO_CONNECT" ) ) )
                pin->SetType( ELECTRICAL_PINTYPE::PT_NC );

            if( pin->GetNumberTextSize() * int( pin->GetNumber().size() ) > pin->GetLength() )
                pin->SetNumberTextSize( pin->GetLength() / pin->GetNumber().size() );

            symInfo.pins.push_back( pinInfo );
            ksymbol->AddDrawItem( pin.release() );
        }
    }

    symInfo.symbolAttr = get_opt( unitAttributes[1], wxS( "Symbol" ) );
    symInfo.libSymbol = std::move( ksymbolPtr );

    return symInfo;
}


static wxString ResolveFieldVariables( const wxString                      aInput,
                                       const std::map<wxString, wxString>& aDeviceAttributes )
{
    wxString inputText = aInput;
    wxString resolvedText;
    int      variableCount = 0;

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
                        get_def( aDeviceAttributes, varName,
                                 wxString::Format( "{%s!}", varName ) );

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


static EASYEDAPRO::SCH_ATTR V3RowToSchAttr( const V3_ROW& aRow )
{
    EASYEDAPRO::SCH_ATTR attr;
    attr.key = V3GetString( aRow.inner, "key" );
    attr.value = V3JsonToString( aRow.inner.value( "value", nlohmann::json() ) );
    attr.keyVisible = V3GetBool( aRow.inner, "keyVisible" );
    attr.valVisible = V3GetBool( aRow.inner, "valueVisible" );

    if( !V3IsNullOrMissing( aRow.inner, "x" ) )
    {
        attr.position = VECTOR2D( V3GetDouble( aRow.inner, "x" ),
                                  V3GetDouble( aRow.inner, "y" ) );
    }

    attr.rotation = V3GetDouble( aRow.inner, "rotation" );
    attr.fontStyle = wxEmptyString;

    return attr;
}


static void ApplyV3AttrToField( SCH_FIELD* aField, const EASYEDAPRO::SCH_ATTR& aAttr,
                                bool aIsSym, bool aToSym,
                                const std::map<wxString, wxString>& aDeviceAttributes,
                                SCH_SYMBOL* aParent, const nlohmann::json& aInner )
{
    EDA_TEXT* text = static_cast<EDA_TEXT*>( aField );

    text->SetText( ResolveFieldVariables( aAttr.value, aDeviceAttributes ) );
    text->SetVisible( aAttr.keyVisible || aAttr.valVisible );

    aField->SetNameShown( aAttr.keyVisible );

    if( aAttr.position )
    {
        aField->SetPosition( !aIsSym ? V3ScalePos( *aAttr.position )
                                     : V3ScalePosSym( *aAttr.position ) );
    }

    // Apply inline font style from v3 JSON
    wxString color = V3GetString( aInner, "color", wxEmptyString );
    wxString fontFamily = V3GetString( aInner, "fontFamily", wxS( "default" ) );
    wxString align = V3GetString( aInner, "align", wxS( "LEFT_BOTTOM" ) );

    if( !color.empty() && color.StartsWith( wxS( "#" ) ) )
    {
        COLOR4D c( color );
        text->SetTextColor( c );
    }

    if( fontFamily != wxS( "Arial" ) && !fontFamily.IsSameAs( wxS( "default" ), false ) )
        text->SetFont( KIFONT::FONT::GetFont( fontFamily ) );

    ApplyV3TextSizeIfDefined( text, aInner );

    int vAlign = AlignToFontV( align );
    int hAlign = AlignToFontH( align );

    if( vAlign == 0 )
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    else if( vAlign == 1 )
        text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    else
        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

    if( hAlign == 0 )
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( hAlign == 1 )
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    auto parent = aParent;
    if( aToSym && parent && parent->Type() == SCH_SYMBOL_T )
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


void SCH_EASYEDAPRO_V3_PARSER::ParseSchematic(
        SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const nlohmann::json& aProject,
        std::map<wxString, EASYEDAPRO::SYM_INFO>&   aSymbolMap,
        const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap,
        const EASYEDAPRO::V3_DOC_RAW&               aDoc, const wxString& aLibName )
{
    std::vector<std::unique_ptr<SCH_ITEM>> createdItems;

    // v3: Collect WIRE and LINE rows by lineGroup for wire geometry
    std::map<wxString, std::vector<const EASYEDAPRO::V3_ROW*>> wireGeometry;
    std::map<wxString, const EASYEDAPRO::V3_ROW*>              wireRows;

    // Collect COMPONENT and ATTR rows by parent ID
    std::map<wxString, std::vector<const EASYEDAPRO::V3_ROW*>> parentedRows;

    // First pass: collect wires, components, and attributes
    for( const EASYEDAPRO::V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "WIRE" ) )
        {
            wireRows[row.id] = &row;
        }
        else if( row.type == wxS( "LINE" ) && !EASYEDAPRO::V3IsNullOrMissing( row.inner, "lineGroup" ) )
        {
            wxString lineGroup = EASYEDAPRO::V3GetString( row.inner, "lineGroup" );
            wireGeometry[lineGroup].push_back( &row );
        }
        else if( row.type == wxS( "COMPONENT" ) )
        {
            parentedRows[row.id].push_back( &row );
        }
        else if( row.type == wxS( "ATTR" ) && !EASYEDAPRO::V3IsNullOrMissing( row.inner, "parentId" ) )
        {
            wxString parentId = EASYEDAPRO::V3GetString( row.inner, "parentId" );
            parentedRows[parentId].push_back( &row );
        }
    }

    // Second pass: create shapes and other non-parented items
    for( const EASYEDAPRO::V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "RECT" ) )
        {
            double x1 = EASYEDAPRO::V3GetDouble( row.inner, "dotX1" );
            double y1 = EASYEDAPRO::V3GetDouble( row.inner, "dotY1" );
            double x2 = EASYEDAPRO::V3GetDouble( row.inner, "dotX2" );
            double y2 = EASYEDAPRO::V3GetDouble( row.inner, "dotY2" );

            std::unique_ptr<SCH_SHAPE> rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );
            rect->SetStart( V3ScalePos( VECTOR2D( x1, y1 ) ) );
            rect->SetEnd( V3ScalePos( VECTOR2D( x2, y2 ) ) );

            SCH_SHAPE* rectPtr = rect.get();
            ApplyV3LineStyle( rectPtr, row.inner );
            createdItems.push_back( std::move( rect ) );
        }
        else if( row.type == wxS( "CIRCLE" ) )
        {
            double centerX = EASYEDAPRO::V3GetDouble( row.inner, "centerX" );
            double centerY = EASYEDAPRO::V3GetDouble( row.inner, "centerY" );
            double radius  = EASYEDAPRO::V3GetDouble( row.inner, "radius" );

            std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );
            circle->SetCenter( V3ScalePos( VECTOR2D( centerX, centerY ) ) );
            circle->SetEnd( circle->GetCenter()
                            + VECTOR2I( SCH_EASYEDAPRO_PARSER::ScaleSize( radius ), 0 ) );

            SCH_SHAPE* circlePtr = circle.get();
            ApplyV3LineStyle( circlePtr, row.inner );
            createdItems.push_back( std::move( circle ) );
        }
        else if( row.type == wxS( "ARC" ) )
        {
            VECTOR2D start( EASYEDAPRO::V3GetDouble( row.inner, "startX" ),
                            EASYEDAPRO::V3GetDouble( row.inner, "startY" ) );
            VECTOR2D mid( EASYEDAPRO::V3GetDouble( row.inner, "referX" ),
                          EASYEDAPRO::V3GetDouble( row.inner, "referY" ) );
            VECTOR2D end( EASYEDAPRO::V3GetDouble( row.inner, "endX" ),
                          EASYEDAPRO::V3GetDouble( row.inner, "endY" ) );

            std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC );
            arc->SetArcGeometry( V3ScalePos( start ),
                                 V3ScalePos( mid ),
                                 V3ScalePos( end ) );

            SCH_SHAPE* arcPtr = arc.get();
            ApplyV3LineStyle( arcPtr, row.inner );
            createdItems.push_back( std::move( arc ) );
        }
        else if( row.type == wxS( "POLY" ) )
        {
            if( EASYEDAPRO::V3IsNullOrMissing( row.inner, "points" ) )
                continue;

            nlohmann::json flatPts = FlattenPoints(
                    row.inner.value( "points", nlohmann::json::array() ) );
            std::vector<double> pointsData = flatPts;

            if( pointsData.size() < 4 )
                continue;

            std::unique_ptr<SCH_SHAPE> poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY );

            for( size_t i = 1; i < pointsData.size(); i += 2 )
                poly->AddPoint( V3ScalePos( VECTOR2D( pointsData[i - 1], pointsData[i] ) ) );

            SCH_SHAPE* polyPtr = poly.get();
            ApplyV3LineStyle( polyPtr, row.inner );
            createdItems.push_back( std::move( poly ) );
        }
        else if( row.type == wxS( "TEXT" ) )
        {
            double x = EASYEDAPRO::V3GetDouble( row.inner, "x" );
            double y = EASYEDAPRO::V3GetDouble( row.inner, "y" );
            double angle = EASYEDAPRO::V3GetDouble( row.inner, "rotation", 0.0 );
            wxString text = EASYEDAPRO::V3GetString( row.inner, "value" );

            std::unique_ptr<SCH_TEXT> schText = std::make_unique<SCH_TEXT>(
                    V3ScalePos( VECTOR2D( x, y ) ),
                    UnescapeHTML( text ) );

            schText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            schText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            schText->SetTextAngleDegrees( angle );

            SCH_TEXT* textPtr = schText.get();
            ApplyV3FontStyle( textPtr, row.inner, "align" );
            createdItems.push_back( std::move( schText ) );
        }
        else if( row.type == wxS( "OBJ" ) )
        {
            if( EASYEDAPRO::V3IsNullOrMissing( row.inner, "content" ) )
                continue;

            wxString content = EASYEDAPRO::V3GetString( row.inner, "content" );

            double x = EASYEDAPRO::V3GetDouble( row.inner, "startX",
                                                 EASYEDAPRO::V3GetDouble( row.inner, "x" ) );
            double y = EASYEDAPRO::V3GetDouble( row.inner, "startY",
                                                 EASYEDAPRO::V3GetDouble( row.inner, "y" ) );
            double width  = EASYEDAPRO::V3GetDouble( row.inner, "width" );
            double height = EASYEDAPRO::V3GetDouble( row.inner, "height" );
            double angle  = EASYEDAPRO::V3GetDouble( row.inner, "rotation", 0.0 );
            bool   flipped = EASYEDAPRO::V3GetBool( row.inner, "isMirror", false );

            VECTOR2D start( x, y );
            VECTOR2D size( width, height );
            VECTOR2D kstart = V3ScalePos( start );
            VECTOR2D ksize  = SCH_EASYEDAPRO_PARSER::ScaleSize( size );

            wxString mimeType;
            wxString base64Data;

            if( content.BeforeFirst( ':' ) == wxS( "data" ) )
            {
                wxArrayString paramsArr =
                        wxSplit( content.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                if( paramsArr.size() > 0 )
                    mimeType = paramsArr[0];

                base64Data = content.AfterFirst( ',' );
            }
            else if( content.BeforeFirst( ':' ) == wxS( "blob" ) )
            {
                wxString objectId = content.AfterLast( ':' );

                if( auto blob = get_opt( aBlobMap, objectId ) )
                {
                    wxString blobUrl = blob->url;

                    if( blobUrl.BeforeFirst( ':' ) == wxS( "data" ) )
                    {
                        wxArrayString paramsArr =
                                wxSplit( blobUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                        if( paramsArr.size() > 0 )
                            mimeType = paramsArr[0];

                        base64Data = blobUrl.AfterFirst( ',' );
                    }
                }
            }

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

                VECTOR2D pixelScale( schIUScale.IUTomm( SCH_EASYEDAPRO_PARSER::ScaleSize( size.x ) )
                                             / imSize.x,
                                     schIUScale.IUTomm( SCH_EASYEDAPRO_PARSER::ScaleSize( size.y ) )
                                             / imSize.y );

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

                    double scaleFactor = SCH_EASYEDAPRO_PARSER::ScaleSize( size.x )
                                         / refImage.GetSize().x;
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
    }

    // Third pass: process wires
    for( const auto& [wireId, wireRow] : wireRows )
    {
        auto geomIt = wireGeometry.find( wireId );
        if( geomIt == wireGeometry.end() )
            continue;

        const std::vector<const EASYEDAPRO::V3_ROW*>& lineRows = geomIt->second;

        bool     firstPointSet = false;
        VECTOR2D firstPoint;

        for( const EASYEDAPRO::V3_ROW* lineRow : lineRows )
        {
            if( EASYEDAPRO::V3IsNullOrMissing( lineRow->inner, "startX" )
                || EASYEDAPRO::V3IsNullOrMissing( lineRow->inner, "startY" )
                || EASYEDAPRO::V3IsNullOrMissing( lineRow->inner, "endX" )
                || EASYEDAPRO::V3IsNullOrMissing( lineRow->inner, "endY" ) )
            {
                continue;
            }

            VECTOR2D start( EASYEDAPRO::V3GetDouble( lineRow->inner, "startX" ),
                            EASYEDAPRO::V3GetDouble( lineRow->inner, "startY" ) );
            VECTOR2D end( EASYEDAPRO::V3GetDouble( lineRow->inner, "endX" ),
                          EASYEDAPRO::V3GetDouble( lineRow->inner, "endY" ) );

            if( !firstPointSet )
            {
                firstPoint = start;
                firstPointSet = true;
            }

            std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>(
                    V3ScalePos( start ), LAYER_WIRE );

            line->SetEndPoint( V3ScalePos( end ) );
            createdItems.push_back( std::move( line ) );
        }

        wxString netName;
        bool     netVisible = false;
        VECTOR2D netPos;
        bool     netPosSet = false;
        wxString netAlign = wxS( "LEFT_BOTTOM" );
        int      netRotation = 0;

        if( auto attrRows = get_opt( parentedRows, wireId ) )
        {
            for( const EASYEDAPRO::V3_ROW* row : *attrRows )
            {
                if( row->type != wxS( "ATTR" ) )
                    continue;

                if( EASYEDAPRO::V3GetString( row->inner, "key" ) != wxS( "NET" ) )
                    continue;

                netName = EASYEDAPRO::V3JsonToString( row->inner.value( "value", nlohmann::json() ) );
                netVisible = EASYEDAPRO::V3GetBool( row->inner, "valueVisible" );
                netAlign = EASYEDAPRO::V3GetString( row->inner, "align", wxS( "LEFT_BOTTOM" ) );
                netRotation = EASYEDAPRO::V3GetInt( row->inner, "rotation", 0 );

                if( !EASYEDAPRO::V3IsNullOrMissing( row->inner, "x" )
                    && !EASYEDAPRO::V3IsNullOrMissing( row->inner, "y" ) )
                {
                    netPos = VECTOR2D( EASYEDAPRO::V3GetDouble( row->inner, "x" ),
                                       EASYEDAPRO::V3GetDouble( row->inner, "y" ) );
                    netPosSet = true;
                }

                break;
            }
        }

        if( netVisible && !netName.IsEmpty() && firstPointSet )
        {
            VECTOR2D labelPos = netPosSet ? netPos : firstPoint;

            std::unique_ptr<SCH_LABEL> label = std::make_unique<SCH_LABEL>(
                    V3ScalePos( labelPos ), netName );

            SPIN_STYLE spinStyle = SPIN_STYLE::RIGHT;

            if( netAlign.StartsWith( wxS( "RIGHT" ) ) )
                spinStyle = SPIN_STYLE::LEFT;
            else if( netAlign.StartsWith( wxS( "LEFT" ) ) )
                spinStyle = SPIN_STYLE::RIGHT;

            int rot = netRotation % 360;

            if( rot < 0 )
                rot += 360;

            // EasyEDA uses CCW rotation, same as KiCad.
            if( rot % 90 == 0 )
            {
                for( int i = 0; i < rot; i += 90 )
                    spinStyle = spinStyle.RotateCCW();
            }

            label->SetSpinStyle( spinStyle );

            createdItems.push_back( std::move( label ) );
        }
    }

    // Fourth pass: process components with their attributes
    for( const auto& [parentId, rows] : parentedRows )
    {
        const EASYEDAPRO::V3_ROW* componentRow = nullptr;
        std::map<wxString, std::pair<EASYEDAPRO::SCH_ATTR, const EASYEDAPRO::V3_ROW*>> attributes;

        for( const EASYEDAPRO::V3_ROW* row : rows )
        {
            if( row->type == wxS( "COMPONENT" ) )
            {
                componentRow = row;
            }
            else if( row->type == wxS( "ATTR" ) )
            {
                EASYEDAPRO::SCH_ATTR attr = V3RowToSchAttr( *row );
                attributes.emplace( attr.key, std::make_pair( attr, row ) );
            }
        }

        if( !componentRow )
            continue;

        auto deviceAttrIt = attributes.find( "Device" );
        auto symbolAttrIt = attributes.find( "Symbol" );

        std::map<wxString, wxString> compAttrs;

        try
        {
            wxString deviceId;

            if( deviceAttrIt != attributes.end() )
                deviceId = deviceAttrIt->second.first.value;

            std::string deviceIdUtf8 = std::string( deviceId.ToUTF8() );

            if( !deviceId.empty()
                && aProject.contains( "devices" )
                && aProject.at( "devices" ).is_object()
                && aProject.at( "devices" ).contains( deviceIdUtf8 ) )
            {
                const nlohmann::json& dev = aProject.at( "devices" ).at( deviceIdUtf8 );

                if( dev.contains( "attributes" ) && dev.at( "attributes" ).is_object() )
                {
                    for( const auto& [key, value] : dev.at( "attributes" ).items() )
                    {
                        compAttrs[wxString::FromUTF8( key )] = EASYEDAPRO::V3JsonToString( value );
                    }
                }
            }
        }
        catch( const std::exception& e )
        {
            wxString deviceId;

            if( deviceAttrIt != attributes.end() )
                deviceId = deviceAttrIt->second.first.value;

            wxLogWarning( wxString::Format(
                    _( "EasyEDA Pro v3 component '%s': failed to read device attributes for '%s': %s" ),
                    parentId, deviceId, e.what() ) );
        }

        wxString symbolId;

        if( symbolAttrIt != attributes.end() && !symbolAttrIt->second.first.value.IsEmpty() )
            symbolId = symbolAttrIt->second.first.value;
        else
            symbolId = get_def( compAttrs, wxS( "Symbol" ), wxEmptyString );

        if( symbolId.IsEmpty() )
        {
            wxLogWarning( wxString::Format(
                    _( "EasyEDA Pro v3 component '%s': missing Symbol attribute, skipping." ),
                    parentId ) );
            continue;
        }

        auto it = aSymbolMap.find( symbolId );
        if( it == aSymbolMap.end() )
        {
            wxString compName = EASYEDAPRO::V3GetString( componentRow->inner, "name", wxS( "?" ) );
            wxLogError( "Symbol of '%s' with uuid '%s' not found.", compName, symbolId );
            continue;
        }

        EASYEDAPRO::SYM_INFO& esymInfo = it->second;
        LIB_SYMBOL            newLibSymbol = *esymInfo.libSymbol.get();

        wxString unitName = EASYEDAPRO::V3GetString( componentRow->inner, "partId" );

        int unitId = 1;
        if( !unitName.IsEmpty() )
        {
            if( auto unitOpt = get_opt( esymInfo.partUnits, unitName ) )
                unitId = *unitOpt;
            else
            {
                wxLogWarning( wxString::Format(
                        _( "EasyEDA Pro v3 component '%s': partId '%s' not found in symbol '%s', using unit 1." ),
                        parentId, unitName, symbolId ) );
            }
        }

        LIB_ID libId = EASYEDAPRO::ToKiCadLibID( aLibName,
                                                 newLibSymbol.GetLibId().GetLibItemName() );

        auto schSym = std::make_unique<SCH_SYMBOL>( newLibSymbol, libId,
                                                     &aSchematic->CurrentSheet(),
                                                     unitId );

        schSym->SetFootprintFieldText( newLibSymbol.GetFootprint() );

        auto footprintAttrIt = attributes.find( wxS( "Footprint" ) );

        wxString resolvedFootprint;

        if( footprintAttrIt != attributes.end() && !footprintAttrIt->second.first.value.IsEmpty() )
            resolvedFootprint = ResolveV3FootprintText( footprintAttrIt->second.first.value,
                                                        aProject, aLibName );
        else
            resolvedFootprint = ResolveV3FootprintText( get_def( compAttrs, wxS( "Footprint" ), wxEmptyString ),
                                                        aProject, aLibName );

        if( !resolvedFootprint.IsEmpty() )
            schSym->SetFootprintFieldText( resolvedFootprint );

        double x        = EASYEDAPRO::V3GetDouble( componentRow->inner, "x" );
        double y        = EASYEDAPRO::V3GetDouble( componentRow->inner, "y" );
        double rotation = EASYEDAPRO::V3GetDouble( componentRow->inner, "rotation", 0.0 );
        bool   mirror   = EASYEDAPRO::V3GetBool( componentRow->inner, "isMirror", false );

        for( double i = rotation; i > 0; i -= 90 )
            schSym->Rotate( VECTOR2I(), true );

        if( mirror )
            schSym->MirrorHorizontally( 0 );

        schSym->SetPosition( V3ScalePos( VECTOR2D( x, y ) ) );

        if( esymInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::NETPORT )
        {
            wxString netName;

            auto nameAttrIt = attributes.find( wxS( "Name" ) );

            if( nameAttrIt != attributes.end() && !nameAttrIt->second.first.value.IsEmpty() )
                netName = nameAttrIt->second.first.value;
            else
                netName = get_def( compAttrs, wxS( "Name" ), wxEmptyString );

            if( netName.IsEmpty() )
                netName = newLibSymbol.GetValueField().GetText();

            netName = ResolveFieldVariables( netName, compAttrs );

            std::vector<SCH_PIN*> pins = schSym->GetPins( &aSchematic->CurrentSheet() );
            SCH_PIN*              pin = pins.empty() ? nullptr : pins.front();

            VECTOR2I labelPos = schSym->GetPosition();
            SPIN_STYLE spinStyle = SPIN_STYLE::RIGHT;
            LABEL_FLAG_SHAPE labelShape = LABEL_FLAG_SHAPE::L_BIDI;

            if( pin )
            {
                labelPos = pin->GetPosition();

                switch( pin->PinDrawOrient( schSym->GetTransform() ) )
                {
                case PIN_ORIENTATION::PIN_LEFT:  spinStyle = SPIN_STYLE::LEFT;   break;
                case PIN_ORIENTATION::PIN_RIGHT: spinStyle = SPIN_STYLE::RIGHT;  break;
                case PIN_ORIENTATION::PIN_UP:    spinStyle = SPIN_STYLE::UP;     break;
                case PIN_ORIENTATION::PIN_DOWN:  spinStyle = SPIN_STYLE::BOTTOM; break;
                default: break;
                }

                switch( pin->GetType() )
                {
                case ELECTRICAL_PINTYPE::PT_INPUT:
                    labelShape = LABEL_FLAG_SHAPE::L_INPUT;
                    break;

                case ELECTRICAL_PINTYPE::PT_OUTPUT:
                    labelShape = LABEL_FLAG_SHAPE::L_OUTPUT;
                    break;

                case ELECTRICAL_PINTYPE::PT_BIDI:
                    labelShape = LABEL_FLAG_SHAPE::L_BIDI;
                    break;

                case ELECTRICAL_PINTYPE::PT_TRISTATE:
                    labelShape = LABEL_FLAG_SHAPE::L_TRISTATE;
                    break;

                default:
                    labelShape = LABEL_FLAG_SHAPE::L_UNSPECIFIED;
                    break;
                }
            }

            std::unique_ptr<SCH_GLOBALLABEL> label =
                    std::make_unique<SCH_GLOBALLABEL>( labelPos, netName );
            label->SetSpinStyle( spinStyle );
            label->SetShape( labelShape );

            bool labelVisible = !netName.IsEmpty();

            if( nameAttrIt != attributes.end() && nameAttrIt->second.second )
            {
                const EASYEDAPRO::V3_ROW* attrRow = nameAttrIt->second.second;

                labelVisible =
                        EASYEDAPRO::V3GetBool( attrRow->inner, "valueVisible", true )
                        && !netName.IsEmpty();

                wxString color = V3GetString( attrRow->inner, "color", wxEmptyString );
                wxString fontFamily =
                        V3GetString( attrRow->inner, "fontFamily", wxS( "default" ) );

                if( !color.empty() && color.StartsWith( wxS( "#" ) ) )
                    label->SetTextColor( COLOR4D( color ) );

                if( fontFamily != wxS( "Arial" )
                    && !fontFamily.IsSameAs( wxS( "default" ), false ) )
                {
                    label->SetFont( KIFONT::FONT::GetFont( fontFamily ) );
                }

                ApplyV3TextSizeIfDefined( label, attrRow->inner );
            }

            label->SetVisible( labelVisible );

            createdItems.push_back( std::move( label ) );
            continue;
        }

        if( esymInfo.head.symbolType == EASYEDAPRO::SYMBOL_TYPE::POWER_PORT )
        {
            SCH_FIELD* valueField = schSym->GetField( FIELD_T::VALUE );

            auto globalNetNameAttrIt = attributes.find( "Global Net Name" );
            auto nameAttrIt = attributes.find( "Name" );
            wxString globalNetNameFromProject =
                    get_def( compAttrs, "Global Net Name", wxEmptyString );
            wxString nameFromProject = get_def( compAttrs, "Name", wxEmptyString );

            const std::pair<EASYEDAPRO::SCH_ATTR, const EASYEDAPRO::V3_ROW*>* valueSource = nullptr;

            if( globalNetNameAttrIt != attributes.end() )
                valueSource = &globalNetNameAttrIt->second;
            else if( nameAttrIt != attributes.end() )
                valueSource = &nameAttrIt->second;

            wxString globalNetName;

            // 1. Pick from schematic attr
            // 2. Pick Name from schematic attr (used by +5V)
            // 3. Pick from project.json
            // 4. Pick Name from project.json
            // 5. Pick from symbol
            if( globalNetNameAttrIt != attributes.end()
                && !globalNetNameAttrIt->second.first.value.IsEmpty() )
            {
                globalNetName = globalNetNameAttrIt->second.first.value;
            }
            else if( nameAttrIt != attributes.end()
                     && !nameAttrIt->second.first.value.IsEmpty() )
            {
                globalNetName = nameAttrIt->second.first.value;
            }
            else if( !globalNetNameFromProject.IsEmpty() )
            {
                globalNetName = globalNetNameFromProject;
            }
            else if( !nameFromProject.IsEmpty() )
            {
                globalNetName = nameFromProject;
            }
            else
            {
                globalNetName = newLibSymbol.GetValueField().GetText();
            }

            globalNetName = ResolveFieldVariables( globalNetName, compAttrs );

            if( valueSource && valueSource->second )
            {
                EASYEDAPRO::SCH_ATTR attr = valueSource->first;
                const EASYEDAPRO::V3_ROW* attrRow = valueSource->second;
                bool valueVisibleMissing = EASYEDAPRO::V3IsNullOrMissing( attrRow->inner,
                                                                          "valueVisible" );
                bool valueFromNameAttr = ( nameAttrIt != attributes.end()
                                           && valueSource == &nameAttrIt->second );

                int attrRotation = EASYEDAPRO::V3GetInt( attrRow->inner, "rotation", 0 ) % 360;

                if( attrRotation < 0 )
                    attrRotation += 360;

                bool hasExplicitStyleOrPosition =
                        !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "x" )
                        || !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "y" )
                        || !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "align" )
                        || !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "fontSize" )
                        || !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "fontFamily" )
                        || !EASYEDAPRO::V3IsNullOrMissing( attrRow->inner, "color" )
                        || attrRotation != 0;

                attr.value = globalNetName;

                if( !globalNetName.IsEmpty() )
                    attr.valVisible = true;
                else if( valueVisibleMissing )
                    attr.valVisible = false;

                if( valueFromNameAttr && !hasExplicitStyleOrPosition )
                {
                    valueField->SetText( ResolveFieldVariables( globalNetName, compAttrs ) );

                    if( !globalNetName.IsEmpty() )
                        valueField->SetVisible( true );
                    else if( valueVisibleMissing )
                        valueField->SetVisible( false );
                }
                else
                {
                    ApplyV3AttrToField( schSym->GetField( FIELD_T::VALUE ), attr, false, true,
                                        compAttrs, schSym.get(), attrRow->inner );

                    if( !globalNetName.IsEmpty() )
                        valueField->SetVisible( true );
                }
            }
            else
            {
                valueField->SetText( ResolveFieldVariables( globalNetName, compAttrs ) );
                valueField->SetVisible( !globalNetName.IsEmpty() );
            }

            for( SCH_PIN* pin : schSym->GetAllLibPins() )
                pin->SetName( globalNetName );

            schSym->SetRef( &aSchematic->CurrentSheet(), wxS( "#PWR?" ) );
            schSym->GetField( FIELD_T::REFERENCE )->SetVisible( false );
        }
        else
        {
            // Regular component
            auto designatorAttrIt = attributes.find( "Designator" );
            SCH_FIELD* refField = schSym->GetField( FIELD_T::REFERENCE );
            bool       hasReferenceValue = false;

            if( designatorAttrIt != attributes.end() )
            {
                const EASYEDAPRO::SCH_ATTR& attr = designatorAttrIt->second.first;
                const EASYEDAPRO::V3_ROW*   attrRow = designatorAttrIt->second.second;

                ApplyV3AttrToField( refField, attr, false, true, compAttrs,
                                    schSym.get(), attrRow->inner );

                if( !attr.value.IsEmpty() )
                {
                    schSym->SetRef( &aSchematic->CurrentSheet(), attr.value );
                    hasReferenceValue = true;
                }
            }

            if( !hasReferenceValue )
                refField->SetVisible( false );

            auto valueAttrIt = attributes.find( "Value" );
            auto nameAttrIt = attributes.find( "Name" );

            wxString fallbackValue = get_def( compAttrs, wxS( "Value" ), wxEmptyString );

            if( fallbackValue.IsEmpty() )
                fallbackValue = get_def( compAttrs, wxS( "Name" ), wxEmptyString );

            const std::pair<EASYEDAPRO::SCH_ATTR, const EASYEDAPRO::V3_ROW*>* valueSource = nullptr;

            if( valueAttrIt != attributes.end() && valueAttrIt->second.first.valVisible )
                valueSource = &valueAttrIt->second;
            else if( nameAttrIt != attributes.end() && nameAttrIt->second.first.valVisible )
                valueSource = &nameAttrIt->second;
            else if( valueAttrIt != attributes.end() )
                valueSource = &valueAttrIt->second;
            else if( nameAttrIt != attributes.end() )
                valueSource = &nameAttrIt->second;

            wxString valueText;

            if( valueAttrIt != attributes.end() && !valueAttrIt->second.first.value.IsEmpty() )
                valueText = valueAttrIt->second.first.value;
            else if( nameAttrIt != attributes.end() && !nameAttrIt->second.first.value.IsEmpty() )
                valueText = nameAttrIt->second.first.value;
            else
                valueText = fallbackValue;

            SCH_FIELD* valueField = schSym->GetField( FIELD_T::VALUE );

            if( !valueText.IsEmpty() )
            {
                if( valueSource && valueSource->second )
                {
                    EASYEDAPRO::SCH_ATTR attr = valueSource->first;
                    attr.value = valueText;

                    ApplyV3AttrToField( valueField, attr, false, true, compAttrs,
                                        schSym.get(), valueSource->second->inner );
                }
                else
                {
                    valueField->SetText( ResolveFieldVariables( valueText, compAttrs ) );
                }
            }
            else
            {
                valueField->SetVisible( false );
            }

            // Apply other visible attributes as fields
            for( const auto& [key, attrPair] : attributes )
            {
                if( key == wxS( "Designator" ) || key == wxS( "Device" ) || key == wxS( "Symbol" )
                    || key == wxS( "Name" ) || key == wxS( "Value" ) )
                {
                    continue;
                }

                const EASYEDAPRO::SCH_ATTR& attr = attrPair.first;
                const EASYEDAPRO::V3_ROW*   attrRow = attrPair.second;

                if( !attr.keyVisible && !attr.valVisible )
                    continue;

                SCH_FIELD* field = nullptr;

                if( key == wxS( "Footprint" ) )
                {
                    field = schSym->GetField( FIELD_T::FOOTPRINT );
                }
                else
                {
                    field = schSym->AddField( SCH_FIELD( schSym.get(), FIELD_T::USER, key ) );
                }

                if( field )
                {
                    if( key == wxS( "Footprint" ) )
                    {
                        EASYEDAPRO::SCH_ATTR footprintAttr = attr;
                        footprintAttr.value = ResolveV3FootprintText( attr.value, aProject,
                                                                      aLibName );

                        ApplyV3AttrToField( field, footprintAttr, false, true,
                                            compAttrs, schSym.get(), attrRow->inner );
                    }
                    else
                    {
                        ApplyV3AttrToField( field, attr, false, true, compAttrs,
                                            schSym.get(), attrRow->inner );
                    }
                }
            }
        }

        createdItems.push_back( std::move( schSym ) );
    }

    BOX2I sheetBBox;
    bool  hasBBox = false;

    for( std::unique_ptr<SCH_ITEM>& item : createdItems )
    {
        BOX2I itemBBox;

        if( item->Type() == SCH_SYMBOL_T )
            itemBBox = static_cast<SCH_SYMBOL*>( item.get() )->GetBodyAndPinsBoundingBox();
        else
            itemBBox = item->GetBoundingBox();

        if( !hasBBox )
        {
            sheetBBox = itemBBox;
            hasBBox = true;
        }
        else
        {
            sheetBBox.Merge( itemBBox );
        }
    }

    SCH_SCREEN* screen = aRootSheet->GetScreen();

    if( hasBBox )
    {
        const int margin = schIUScale.MilsToIU( 200 );

        VECTOR2I offset( -sheetBBox.GetLeft() + margin,
                         -sheetBBox.GetTop() + margin );

        int alignGrid = schIUScale.MilsToIU( 50 );

        offset.x = KiROUND( offset.x / alignGrid ) * alignGrid;
        offset.y = KiROUND( offset.y / alignGrid ) * alignGrid;

        PAGE_INFO pageInfo = screen->GetPageSettings();
        pageInfo.SetWidthMils( schIUScale.IUToMils( sheetBBox.GetWidth() + 2 * margin ) );
        pageInfo.SetHeightMils( schIUScale.IUToMils( sheetBBox.GetHeight() + 2 * margin ) );

        screen->SetPageSettings( pageInfo );

        for( std::unique_ptr<SCH_ITEM>& item : createdItems )
        {
            item->Move( offset );
            screen->Append( item.release() );
        }
    }
    else
    {
        for( std::unique_ptr<SCH_ITEM>& item : createdItems )
            screen->Append( item.release() );
    }
}
