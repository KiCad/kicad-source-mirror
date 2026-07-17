/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <diff_merge/kicad_diff_types.h>

#include <core/wx_stl_compat.h>
#include <eda_units.h>
#include <geometry/eda_angle.h>
#include <json_conversions.h>
#include <layer_ids.h>

#include <nlohmann/json.hpp>

#include <wx/string.h>
#include <wx/crt.h>
#include <wx/file.h>

#include <functional>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#ifdef __WINDOWS__
template class KICOMMON_API nlohmann::basic_json<>;
#endif


namespace KICAD_DIFF
{

namespace
{

// Tag strings used in JSON output. Keep stable; changing these is a wire-format break.
constexpr const char* TYPE_NONE     = "none";
constexpr const char* TYPE_BOOL     = "bool";
constexpr const char* TYPE_INT      = "int";
constexpr const char* TYPE_INT64    = "int64";
constexpr const char* TYPE_DOUBLE   = "double";
constexpr const char* TYPE_STRING   = "string";
constexpr const char* TYPE_KIID     = "kiid";
constexpr const char* TYPE_VECTOR2I = "vec2";
constexpr const char* TYPE_BOX2I    = "box2";
constexpr const char* TYPE_COLOR    = "color";
constexpr const char* TYPE_LAYER    = "layer";
constexpr const char* TYPE_ENUM     = "enum";
constexpr const char* TYPE_POLYSET = "polyset";


const char* typeTag( DIFF_VALUE::T aType )
{
    switch( aType )
    {
    case DIFF_VALUE::T::NONE: return TYPE_NONE;
    case DIFF_VALUE::T::BOOL: return TYPE_BOOL;
    case DIFF_VALUE::T::INT: return TYPE_INT;
    case DIFF_VALUE::T::INT64: return TYPE_INT64;
    case DIFF_VALUE::T::DOUBLE: return TYPE_DOUBLE;
    case DIFF_VALUE::T::STRING: return TYPE_STRING;
    case DIFF_VALUE::T::KIID: return TYPE_KIID;
    case DIFF_VALUE::T::VECTOR2I: return TYPE_VECTOR2I;
    case DIFF_VALUE::T::BOX2I: return TYPE_BOX2I;
    case DIFF_VALUE::T::COLOR: return TYPE_COLOR;
    case DIFF_VALUE::T::LAYER: return TYPE_LAYER;
    case DIFF_VALUE::T::ENUM: return TYPE_ENUM;
    case DIFF_VALUE::T::POLYGON_SET: return TYPE_POLYSET;
    }

    return TYPE_NONE;
}


DIFF_VALUE::T typeFromTag( const std::string& aTag )
{
    if( aTag == TYPE_BOOL )     return DIFF_VALUE::T::BOOL;
    if( aTag == TYPE_INT )      return DIFF_VALUE::T::INT;
    if( aTag == TYPE_INT64 )    return DIFF_VALUE::T::INT64;
    if( aTag == TYPE_DOUBLE )   return DIFF_VALUE::T::DOUBLE;
    if( aTag == TYPE_STRING )   return DIFF_VALUE::T::STRING;
    if( aTag == TYPE_KIID )     return DIFF_VALUE::T::KIID;
    if( aTag == TYPE_VECTOR2I ) return DIFF_VALUE::T::VECTOR2I;
    if( aTag == TYPE_BOX2I )    return DIFF_VALUE::T::BOX2I;
    if( aTag == TYPE_COLOR )    return DIFF_VALUE::T::COLOR;
    if( aTag == TYPE_LAYER )    return DIFF_VALUE::T::LAYER;
    if( aTag == TYPE_ENUM )     return DIFF_VALUE::T::ENUM;
    if( aTag == TYPE_POLYSET )
        return DIFF_VALUE::T::POLYGON_SET;

    return DIFF_VALUE::T::NONE;
}

} // namespace


DIFF_VALUE DIFF_VALUE::FromBool( bool aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::BOOL;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromInt( int aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::INT;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromInt64( int64_t aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::INT64;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromDouble( double aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::DOUBLE;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromString( const wxString& aValue )
{
    return FromString( std::string( aValue.ToUTF8() ) );
}


DIFF_VALUE DIFF_VALUE::FromString( const std::string& aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::STRING;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromKiid( const KIID& aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::KIID;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromVector2I( const VECTOR2I& aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::VECTOR2I;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromBox2I( const BOX2I& aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::BOX2I;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromColor( const KIGFX::COLOR4D& aValue )
{
    DIFF_VALUE v;
    v.m_type  = T::COLOR;
    v.m_value = aValue;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromLayer( PCB_LAYER_ID aLayer )
{
    DIFF_VALUE v;
    v.m_type  = T::LAYER;
    v.m_value = aLayer;
    return v;
}


DIFF_VALUE DIFF_VALUE::FromEnum( int aValue, const std::string& aLabel )
{
    DIFF_VALUE v;
    v.m_type  = T::ENUM;
    v.m_value = EnumValue{ aValue, aLabel };
    return v;
}


DIFF_VALUE DIFF_VALUE::FromPolygonSet( PolygonSet aValue )
{
    DIFF_VALUE v;
    v.m_type = T::POLYGON_SET;
    v.m_value = std::move( aValue );
    return v;
}


bool DIFF_VALUE::AsBool() const
{
    return std::get<bool>( m_value );
}


int DIFF_VALUE::AsInt() const
{
    return std::get<int>( m_value );
}


int64_t DIFF_VALUE::AsInt64() const
{
    return std::get<int64_t>( m_value );
}


double DIFF_VALUE::AsDouble() const
{
    return std::get<double>( m_value );
}


wxString DIFF_VALUE::AsString() const
{
    return wxString::FromUTF8( std::get<std::string>( m_value ) );
}


KIID DIFF_VALUE::AsKiid() const
{
    return std::get<KIID>( m_value );
}


VECTOR2I DIFF_VALUE::AsVector2I() const
{
    return std::get<VECTOR2I>( m_value );
}


BOX2I DIFF_VALUE::AsBox2I() const
{
    return std::get<BOX2I>( m_value );
}


KIGFX::COLOR4D DIFF_VALUE::AsColor() const
{
    return std::get<KIGFX::COLOR4D>( m_value );
}


PCB_LAYER_ID DIFF_VALUE::AsLayer() const
{
    return std::get<PCB_LAYER_ID>( m_value );
}


DIFF_VALUE::EnumValue DIFF_VALUE::AsEnum() const
{
    return std::get<EnumValue>( m_value );
}


const DIFF_VALUE::PolygonSet& DIFF_VALUE::AsPolygonSet() const
{
    return std::get<PolygonSet>( m_value );
}


wxString DIFF_VALUE::ToDisplayString() const
{
    switch( m_type )
    {
    case T::NONE:
        return wxS( "<none>" );

    case T::BOOL:
        return AsBool() ? wxS( "true" ) : wxS( "false" );

    case T::INT:
        return wxString::Format( wxS( "%d" ), AsInt() );

    case T::INT64:
        return wxString::Format( wxS( "%lld" ), static_cast<long long>( AsInt64() ) );

    case T::DOUBLE:
        return wxString::Format( wxS( "%g" ), AsDouble() );

    case T::STRING:
        return AsString();

    case T::KIID:
        return AsKiid().AsString();

    case T::VECTOR2I:
    {
        VECTOR2I p = AsVector2I();
        return wxString::Format( wxS( "(%d, %d)" ), p.x, p.y );
    }

    case T::BOX2I:
    {
        BOX2I b = AsBox2I();
        return wxString::Format( wxS( "[(%d, %d) %dx%d]" ),
                                 b.GetX(), b.GetY(), b.GetWidth(), b.GetHeight() );
    }

    case T::COLOR:
        return AsColor().ToCSSString();

    case T::LAYER:
        return LayerName( AsLayer() );

    case T::ENUM:
    {
        EnumValue ev = AsEnum();

        if( !ev.second.empty() )
            return wxString::FromUTF8( ev.second );

        return wxString::Format( wxS( "%d" ), ev.first );
    }

    case T::POLYGON_SET:
    {
        const PolygonSet& ps = AsPolygonSet();
        int               outlineCount = 0;
        int               holeCount = 0;
        int               vertexCount = 0;

        for( const auto& poly : ps )
        {
            if( !poly.empty() )
                ++outlineCount;

            if( poly.size() > 1 )
                holeCount += static_cast<int>( poly.size() ) - 1;

            for( const auto& contour : poly )
                vertexCount += static_cast<int>( contour.size() );
        }

        return wxString::Format( wxS( "%d outline(s), %d hole(s), %d vertex(es)" ), outlineCount, holeCount,
                                 vertexCount );
    }
    }

    return wxEmptyString;
}


wxString DIFF_VALUE::ToDisplayString( EDA_UNITS aUnits, const EDA_IU_SCALE& aScale ) const
{
    if( m_displayHint == DISPLAY_HINT::NONE )
        return ToDisplayString();

    switch( m_type )
    {
    case T::INT:
    case T::INT64:
    {
        double value = ( m_type == T::INT ) ? static_cast<double>( AsInt() )
                                             : static_cast<double>( AsInt64() );

        if( m_displayHint == DISPLAY_HINT::ANGLE )
            return EDA_UNIT_UTILS::UI::MessageTextFromValue( EDA_ANGLE( value, DEGREES_T ) );

        return EDA_UNIT_UTILS::UI::StringFromValue( aScale, aUnits, value, true );
    }

    case T::DOUBLE:
    {
        double value = AsDouble();

        if( m_displayHint == DISPLAY_HINT::ANGLE )
            return EDA_UNIT_UTILS::UI::MessageTextFromValue( EDA_ANGLE( value, DEGREES_T ) );

        return EDA_UNIT_UTILS::UI::StringFromValue( aScale, aUnits, value, true );
    }

    case T::VECTOR2I:
    {
        VECTOR2I p = AsVector2I();
        wxString x = EDA_UNIT_UTILS::UI::StringFromValue( aScale, aUnits, p.x, true );
        wxString y = EDA_UNIT_UTILS::UI::StringFromValue( aScale, aUnits, p.y, true );

        return wxString::Format( wxS( "(%s, %s)" ), x, y );
    }

    default:
        return ToDisplayString();
    }
}


bool DIFF_VALUE::operator==( const DIFF_VALUE& aOther ) const
{
    return m_type == aOther.m_type && m_value == aOther.m_value;
}


nlohmann::json DIFF_VALUE::ToJson() const
{
    nlohmann::json j;
    j["type"] = typeTag( m_type );

    switch( m_type )
    {
    case T::NONE:
        break;

    case T::BOOL:
        j["v"] = AsBool();
        break;

    case T::INT:
        j["v"] = AsInt();
        break;

    case T::INT64:
        j["v"] = AsInt64();
        break;

    case T::DOUBLE:
        j["v"] = AsDouble();
        break;

    case T::STRING:
        j["v"] = std::get<std::string>( m_value );
        break;

    case T::KIID:
        j["v"] = AsKiid().AsStdString();
        break;

    case T::VECTOR2I:
    {
        VECTOR2I p = AsVector2I();
        j["v"] = { p.x, p.y };
        break;
    }

    case T::BOX2I:
    {
        BOX2I b = AsBox2I();
        j["v"] = { b.GetX(), b.GetY(), b.GetWidth(), b.GetHeight() };
        break;
    }

    case T::COLOR:
    {
        KIGFX::COLOR4D c = AsColor();
        j["v"] = { c.r, c.g, c.b, c.a };
        break;
    }

    case T::LAYER:
        j["v"] = static_cast<int>( AsLayer() );
        break;

    case T::ENUM:
    {
        EnumValue ev = AsEnum();
        j["v"]     = ev.first;
        j["label"] = ev.second;
        break;
    }

    case T::POLYGON_SET:
    {
        const PolygonSet& ps = AsPolygonSet();
        nlohmann::json    polygons = nlohmann::json::array();

        for( const auto& poly : ps )
        {
            nlohmann::json contours = nlohmann::json::array();

            for( const auto& contour : poly )
            {
                nlohmann::json points = nlohmann::json::array();

                for( const VECTOR2I& pt : contour )
                    points.push_back( { pt.x, pt.y } );

                contours.push_back( std::move( points ) );
            }

            polygons.push_back( std::move( contours ) );
        }

        j["v"] = std::move( polygons );
        break;
    }
    }

    return j;
}


DIFF_VALUE DIFF_VALUE::FromJson( const nlohmann::json& aJson )
{
    // The display hint is not serialized, so every reconstructed value carries
    // DISPLAY_HINT::NONE and renders in raw internal units under the unit-aware
    // overload; only freshly-converted values get unit formatting.
    if( !aJson.contains( "type" ) )
        return DIFF_VALUE();

    T type = typeFromTag( aJson.at( "type" ).get<std::string>() );

    switch( type )
    {
    case T::NONE:
        return DIFF_VALUE();

    case T::BOOL:
        return FromBool( aJson.at( "v" ).get<bool>() );

    case T::INT:
        return FromInt( aJson.at( "v" ).get<int>() );

    case T::INT64:
        return FromInt64( aJson.at( "v" ).get<int64_t>() );

    case T::DOUBLE:
        return FromDouble( aJson.at( "v" ).get<double>() );

    case T::STRING:
        return FromString( aJson.at( "v" ).get<std::string>() );

    case T::KIID:
        return FromKiid( KIID( aJson.at( "v" ).get<std::string>() ) );

    case T::VECTOR2I:
    {
        const auto& arr = aJson.at( "v" );
        return FromVector2I( VECTOR2I( arr.at( 0 ).get<int>(), arr.at( 1 ).get<int>() ) );
    }

    case T::BOX2I:
    {
        const auto& arr = aJson.at( "v" );
        BOX2I b( VECTOR2I( arr.at( 0 ).get<int>(), arr.at( 1 ).get<int>() ),
                 VECTOR2I( arr.at( 2 ).get<int>(), arr.at( 3 ).get<int>() ) );
        return FromBox2I( b );
    }

    case T::COLOR:
    {
        const auto& arr = aJson.at( "v" );
        return FromColor( KIGFX::COLOR4D( arr.at( 0 ).get<double>(),
                                          arr.at( 1 ).get<double>(),
                                          arr.at( 2 ).get<double>(),
                                          arr.at( 3 ).get<double>() ) );
    }

    case T::LAYER:
        return FromLayer( static_cast<PCB_LAYER_ID>( aJson.at( "v" ).get<int>() ) );

    case T::ENUM:
    {
        std::string label;

        if( aJson.contains( "label" ) )
            label = aJson.at( "label" ).get<std::string>();

        return FromEnum( aJson.at( "v" ).get<int>(), label );
    }

    case T::POLYGON_SET:
    {
        PolygonSet  ps;
        const auto& polygons = aJson.at( "v" );

        for( const auto& poly : polygons )
        {
            std::vector<std::vector<VECTOR2I>> contours;

            for( const auto& contour : poly )
            {
                std::vector<VECTOR2I> points;

                for( const auto& pt : contour )
                    points.emplace_back( pt.at( 0 ).get<int>(), pt.at( 1 ).get<int>() );

                contours.push_back( std::move( points ) );
            }

            ps.push_back( std::move( contours ) );
        }

        return FromPolygonSet( std::move( ps ) );
    }
    }

    return DIFF_VALUE();
}


bool PROPERTY_DELTA::operator==( const PROPERTY_DELTA& aOther ) const
{
    return name == aOther.name && before == aOther.before && after == aOther.after;
}


nlohmann::json PROPERTY_DELTA::ToJson() const
{
    return { { "name",   name },
             { "before", before.ToJson() },
             { "after",  after.ToJson() } };
}


PROPERTY_DELTA PROPERTY_DELTA::FromJson( const nlohmann::json& aJson )
{
    PROPERTY_DELTA d;
    d.name   = aJson.at( "name" ).get<wxString>();
    d.before = DIFF_VALUE::FromJson( aJson.at( "before" ) );
    d.after  = DIFF_VALUE::FromJson( aJson.at( "after" ) );
    return d;
}


bool ITEM_CHANGE::operator==( const ITEM_CHANGE& aOther ) const
{
    return id == aOther.id
        && typeName == aOther.typeName
        && kind == aOther.kind
        && properties == aOther.properties
        && bbox == aOther.bbox
        && refdes == aOther.refdes
        && children == aOther.children;
}


nlohmann::json ITEM_CHANGE::ToJson() const
{
    nlohmann::json j;
    j["id"]       = id.AsString();
    j["typeName"] = typeName;
    j["kind"]     = ChangeKindToString( kind );

    nlohmann::json props = nlohmann::json::array();

    for( const PROPERTY_DELTA& p : properties )
        props.push_back( p.ToJson() );

    j["properties"] = std::move( props );

    j["bbox"] = { bbox.GetX(), bbox.GetY(), bbox.GetWidth(), bbox.GetHeight() };

    if( refdes.has_value() )
        j["refdes"] = *refdes;

    nlohmann::json kids = nlohmann::json::array();

    for( const ITEM_CHANGE& c : children )
        kids.push_back( c.ToJson() );

    j["children"] = std::move( kids );

    return j;
}


ITEM_CHANGE ITEM_CHANGE::FromJson( const nlohmann::json& aJson )
{
    ITEM_CHANGE c;
    c.id       = KIID_PATH( aJson.at( "id" ).get<wxString>() );
    c.typeName = aJson.at( "typeName" ).get<wxString>();
    c.kind     = ChangeKindFromString( aJson.at( "kind" ).get<std::string>() );

    for( const auto& p : aJson.at( "properties" ) )
        c.properties.push_back( PROPERTY_DELTA::FromJson( p ) );

    const auto& b = aJson.at( "bbox" );
    c.bbox = BOX2I( VECTOR2I( b.at( 0 ).get<int>(), b.at( 1 ).get<int>() ),
                    VECTOR2I( b.at( 2 ).get<int>(), b.at( 3 ).get<int>() ) );

    if( aJson.contains( "refdes" ) )
        c.refdes = aJson.at( "refdes" ).get<wxString>();

    for( const auto& kid : aJson.at( "children" ) )
        c.children.push_back( ITEM_CHANGE::FromJson( kid ) );

    return c;
}


nlohmann::json DOCUMENT_DIFF::ToJson() const
{
    nlohmann::json j;
    j["path"]    = path;
    j["docType"] = docType;

    nlohmann::json arr = nlohmann::json::array();

    for( const ITEM_CHANGE& c : changes )
        arr.push_back( c.ToJson() );

    j["changes"] = std::move( arr );
    return j;
}


DOCUMENT_DIFF DOCUMENT_DIFF::FromJson( const nlohmann::json& aJson )
{
    DOCUMENT_DIFF d;
    d.path    = aJson.at( "path" ).get<wxString>();
    d.docType = aJson.at( "docType" ).get<wxString>();

    for( const auto& c : aJson.at( "changes" ) )
        d.changes.push_back( ITEM_CHANGE::FromJson( c ) );

    return d;
}


bool PROJECT_DIFF::Empty() const
{
    for( const DOCUMENT_DIFF& d : documents )
    {
        if( !d.Empty() )
            return false;
    }

    return true;
}


nlohmann::json PROJECT_DIFF::ToJson() const
{
    nlohmann::json arr = nlohmann::json::array();

    for( const DOCUMENT_DIFF& d : documents )
        arr.push_back( d.ToJson() );

    return { { "documents", std::move( arr ) } };
}


PROJECT_DIFF PROJECT_DIFF::FromJson( const nlohmann::json& aJson )
{
    PROJECT_DIFF p;

    for( const auto& d : aJson.at( "documents" ) )
        p.documents.push_back( DOCUMENT_DIFF::FromJson( d ) );

    return p;
}


const char* ChangeKindToString( CHANGE_KIND aKind )
{
    switch( aKind )
    {
    case CHANGE_KIND::ADDED:          return "added";
    case CHANGE_KIND::REMOVED:        return "removed";
    case CHANGE_KIND::MODIFIED:       return "modified";
    case CHANGE_KIND::COLLISION:      return "collision";
    case CHANGE_KIND::DUPLICATE_UUID: return "duplicate_uuid";
    }

    return "unknown";
}


CHANGE_KIND ChangeKindFromString( const std::string& aKind )
{
    if( aKind == "added" )          return CHANGE_KIND::ADDED;
    if( aKind == "removed" )        return CHANGE_KIND::REMOVED;
    if( aKind == "modified" )       return CHANGE_KIND::MODIFIED;
    if( aKind == "collision" )      return CHANGE_KIND::COLLISION;
    if( aKind == "duplicate_uuid" ) return CHANGE_KIND::DUPLICATE_UUID;

    throw std::invalid_argument( "Unknown CHANGE_KIND tag: " + aKind );
}


std::string FormatDiffAsText( const DOCUMENT_DIFF& aDiff, const wxString& aLabelA,
                              const wxString& aLabelB, EDA_UNITS aUnits,
                              const EDA_IU_SCALE& aScale )
{
    std::ostringstream ss;
    ss << "diff " << aLabelA.ToStdString() << " " << aLabelB.ToStdString() << "\n";
    ss << aDiff.changes.size() << " change(s)\n";

    std::function<void( const ITEM_CHANGE&, int )> writeChange =
            [&]( const ITEM_CHANGE& aChange, int aDepth )
            {
                std::string indent( static_cast<std::size_t>( 2 + aDepth * 2 ), ' ' );

                ss << indent << ChangeKindToString( aChange.kind ) << " " << aChange.typeName.ToStdString()
                   << " " << aChange.id.AsString().ToStdString();

                if( aChange.refdes.has_value() )
                    ss << " [" << aChange.refdes->ToStdString() << "]";

                ss << "\n";

                std::string propIndent( static_cast<std::size_t>( 4 + aDepth * 2 ), ' ' );

                for( const PROPERTY_DELTA& p : aChange.properties )
                {
                    ss << propIndent << p.name.ToStdString() << ": "
                       << p.before.ToDisplayString( aUnits, aScale ).ToStdString() << " -> "
                       << p.after.ToDisplayString( aUnits, aScale ).ToStdString() << "\n";
                }

                for( const ITEM_CHANGE& child : aChange.children )
                    writeChange( child, aDepth + 1 );
            };

    for( const ITEM_CHANGE& c : aDiff.changes )
        writeChange( c, 0 );

    return ss.str();
}


bool WriteDiffOutput( const std::string& aContent, const wxString& aOutputPath )
{
    if( aOutputPath.IsEmpty() )
    {
        return fwrite( aContent.data(), 1, aContent.size(), stdout ) == aContent.size()
               && ferror( stdout ) == 0;
    }

    wxFile out( aOutputPath, wxFile::write );

    if( !out.IsOpened() )
        return false;

    out.Write( aContent.data(), aContent.size() );
    return true;
}

} // namespace KICAD_DIFF
