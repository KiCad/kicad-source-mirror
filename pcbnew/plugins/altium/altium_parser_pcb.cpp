/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <map>
#include <unordered_map>

#include <ki_exception.h>
#include <math/util.h>

#include <wx/log.h>

#include "altium_parser_pcb.h"
#include "plugins/altium/altium_parser.h"


ALTIUM_LAYER altium_layer_from_name( const wxString& aName )
{
    static const std::unordered_map<std::string, ALTIUM_LAYER> hash_map = {
        { "TOP", ALTIUM_LAYER::TOP_LAYER },
        { "MID1", ALTIUM_LAYER::MID_LAYER_1 },
        { "MID2", ALTIUM_LAYER::MID_LAYER_2 },
        { "MID3", ALTIUM_LAYER::MID_LAYER_3 },
        { "MID4", ALTIUM_LAYER::MID_LAYER_4 },
        { "MID5", ALTIUM_LAYER::MID_LAYER_5 },
        { "MID6", ALTIUM_LAYER::MID_LAYER_6 },
        { "MID7", ALTIUM_LAYER::MID_LAYER_7 },
        { "MID8", ALTIUM_LAYER::MID_LAYER_8 },
        { "MID9", ALTIUM_LAYER::MID_LAYER_9 },
        { "MID10", ALTIUM_LAYER::MID_LAYER_10 },
        { "MID11", ALTIUM_LAYER::MID_LAYER_11 },
        { "MID12", ALTIUM_LAYER::MID_LAYER_12 },
        { "MID13", ALTIUM_LAYER::MID_LAYER_13 },
        { "MID14", ALTIUM_LAYER::MID_LAYER_14 },
        { "MID15", ALTIUM_LAYER::MID_LAYER_15 },
        { "MID16", ALTIUM_LAYER::MID_LAYER_16 },
        { "MID17", ALTIUM_LAYER::MID_LAYER_17 },
        { "MID18", ALTIUM_LAYER::MID_LAYER_18 },
        { "MID19", ALTIUM_LAYER::MID_LAYER_19 },
        { "MID20", ALTIUM_LAYER::MID_LAYER_20 },
        { "MID21", ALTIUM_LAYER::MID_LAYER_21 },
        { "MID22", ALTIUM_LAYER::MID_LAYER_22 },
        { "MID23", ALTIUM_LAYER::MID_LAYER_23 },
        { "MID24", ALTIUM_LAYER::MID_LAYER_24 },
        { "MID25", ALTIUM_LAYER::MID_LAYER_25 },
        { "MID26", ALTIUM_LAYER::MID_LAYER_26 },
        { "MID27", ALTIUM_LAYER::MID_LAYER_27 },
        { "MID28", ALTIUM_LAYER::MID_LAYER_28 },
        { "MID29", ALTIUM_LAYER::MID_LAYER_29 },
        { "MID30", ALTIUM_LAYER::MID_LAYER_30 },
        { "BOTTOM", ALTIUM_LAYER::BOTTOM_LAYER },

        { "TOPOVERLAY", ALTIUM_LAYER::TOP_OVERLAY },
        { "BOTTOMOVERLAY", ALTIUM_LAYER::BOTTOM_OVERLAY },
        { "TOPPASTE", ALTIUM_LAYER::TOP_PASTE },
        { "BOTTOMPASTE", ALTIUM_LAYER::BOTTOM_PASTE },
        { "TOPSOLDER", ALTIUM_LAYER::TOP_SOLDER },
        { "BOTTOMSOLDER", ALTIUM_LAYER::BOTTOM_SOLDER },

        { "PLANE1", ALTIUM_LAYER::INTERNAL_PLANE_1 },
        { "PLANE2", ALTIUM_LAYER::INTERNAL_PLANE_2 },
        { "PLANE3", ALTIUM_LAYER::INTERNAL_PLANE_3 },
        { "PLANE4", ALTIUM_LAYER::INTERNAL_PLANE_4 },
        { "PLANE5", ALTIUM_LAYER::INTERNAL_PLANE_5 },
        { "PLANE6", ALTIUM_LAYER::INTERNAL_PLANE_6 },
        { "PLANE7", ALTIUM_LAYER::INTERNAL_PLANE_7 },
        { "PLANE8", ALTIUM_LAYER::INTERNAL_PLANE_8 },
        { "PLANE9", ALTIUM_LAYER::INTERNAL_PLANE_9 },
        { "PLANE10", ALTIUM_LAYER::INTERNAL_PLANE_10 },
        { "PLANE11", ALTIUM_LAYER::INTERNAL_PLANE_11 },
        { "PLANE12", ALTIUM_LAYER::INTERNAL_PLANE_12 },
        { "PLANE13", ALTIUM_LAYER::INTERNAL_PLANE_13 },
        { "PLANE14", ALTIUM_LAYER::INTERNAL_PLANE_14 },
        { "PLANE15", ALTIUM_LAYER::INTERNAL_PLANE_15 },
        { "PLANE16", ALTIUM_LAYER::INTERNAL_PLANE_16 },

        { "DRILLGUIDE", ALTIUM_LAYER::DRILL_GUIDE },
        { "KEEPOUT", ALTIUM_LAYER::KEEP_OUT_LAYER },

        { "MECHANICAL1", ALTIUM_LAYER::MECHANICAL_1 },
        { "MECHANICAL2", ALTIUM_LAYER::MECHANICAL_2 },
        { "MECHANICAL3", ALTIUM_LAYER::MECHANICAL_3 },
        { "MECHANICAL4", ALTIUM_LAYER::MECHANICAL_4 },
        { "MECHANICAL5", ALTIUM_LAYER::MECHANICAL_5 },
        { "MECHANICAL6", ALTIUM_LAYER::MECHANICAL_6 },
        { "MECHANICAL7", ALTIUM_LAYER::MECHANICAL_7 },
        { "MECHANICAL8", ALTIUM_LAYER::MECHANICAL_8 },
        { "MECHANICAL9", ALTIUM_LAYER::MECHANICAL_9 },
        { "MECHANICAL10", ALTIUM_LAYER::MECHANICAL_10 },
        { "MECHANICAL11", ALTIUM_LAYER::MECHANICAL_11 },
        { "MECHANICAL12", ALTIUM_LAYER::MECHANICAL_12 },
        { "MECHANICAL13", ALTIUM_LAYER::MECHANICAL_13 },
        { "MECHANICAL14", ALTIUM_LAYER::MECHANICAL_14 },
        { "MECHANICAL15", ALTIUM_LAYER::MECHANICAL_15 },
        { "MECHANICAL16", ALTIUM_LAYER::MECHANICAL_16 },

        { "DRILLDRAWING", ALTIUM_LAYER::DRILL_DRAWING },
        { "MULTILAYER", ALTIUM_LAYER::MULTI_LAYER },

        // FIXME: the following mapping is just a guess
        { "CONNECTIONS", ALTIUM_LAYER::CONNECTIONS },
        { "BACKGROUND", ALTIUM_LAYER::BACKGROUND },
        { "DRCERRORMARKERS", ALTIUM_LAYER::DRC_ERROR_MARKERS },
        { "SELECTIONS", ALTIUM_LAYER::SELECTIONS },
        { "VISIBLEGRID1", ALTIUM_LAYER::VISIBLE_GRID_1 },
        { "VISIBLEGRID2", ALTIUM_LAYER::VISIBLE_GRID_2 },
        { "PADHOLES", ALTIUM_LAYER::PAD_HOLES },
        { "VIAHOLES", ALTIUM_LAYER::VIA_HOLES },
    };

    auto it = hash_map.find( std::string( aName.c_str() ) );

    if( it == hash_map.end() )
    {
        wxLogError( _( "Unknown mapping of the Altium layer '%s'." ), aName );
        return ALTIUM_LAYER::UNKNOWN;
    }
    else
    {
        return it->second;
    }
}

void altium_parse_polygons(
        std::map<wxString, wxString>& aProperties, std::vector<ALTIUM_VERTICE>& aVertices )
{
    for( size_t i = 0; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const wxString si = std::to_string( i );

        const wxString vxi = "VX" + si;
        const wxString vyi = "VY" + si;

        if( aProperties.find( vxi ) == aProperties.end()
                || aProperties.find( vyi ) == aProperties.end() )
        {
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon
        }

        const bool isRound = ALTIUM_PARSER::PropertiesReadInt( aProperties, "KIND" + si, 0 ) != 0;
        const int32_t radius =
                ALTIUM_PARSER::PropertiesReadKicadUnit( aProperties, "R" + si, "0mil" );
        const double  sa = ALTIUM_PARSER::PropertiesReadDouble( aProperties, "SA" + si, 0. );
        const double  ea = ALTIUM_PARSER::PropertiesReadDouble( aProperties, "EA" + si, 0. );
        const wxPoint vp =
                wxPoint( ALTIUM_PARSER::PropertiesReadKicadUnit( aProperties, vxi, "0mil" ),
                        -ALTIUM_PARSER::PropertiesReadKicadUnit( aProperties, vyi, "0mil" ) );
        const wxPoint cp =
                wxPoint( ALTIUM_PARSER::PropertiesReadKicadUnit( aProperties, "CX" + si, "0mil" ),
                        -ALTIUM_PARSER::PropertiesReadKicadUnit( aProperties, "CY" + si, "0mil" ) );

        aVertices.emplace_back( isRound, radius, sa, ea, vp, cp );
    }
}

ABOARD6::ABOARD6( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Board6 stream has no properties!" );
    }

    /*for (auto & property : properties) {
        std::cout << "  * '" << property.first << "' = '" << property.second << "'" << std::endl;
    }*/

    sheetpos  = wxPoint( ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "SHEETX", "0mil" ),
            -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "SHEETY", "0mil" ) );
    sheetsize = wxSize( ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "SHEETWIDTH", "0mil" ),
            ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "SHEETHEIGHT", "0mil" ) );

    layercount = ALTIUM_PARSER::PropertiesReadInt( properties, "LAYERSETSCOUNT", 1 ) + 1;

    for( size_t i = 1; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const wxString layeri    = "LAYER" + std::to_string( i );
        const wxString layername = layeri + "NAME";

        auto layernameit = properties.find( layername );
        if( layernameit == properties.end() )
        {
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon
        }

        ABOARD6_LAYER_STACKUP curlayer;

        curlayer.name   = ALTIUM_PARSER::PropertiesReadString( properties, layername, "" );
        curlayer.nextId = ALTIUM_PARSER::PropertiesReadInt( properties, layeri + "NEXT", 0 );
        curlayer.prevId = ALTIUM_PARSER::PropertiesReadInt( properties, layeri + "PREV", 0 );
        curlayer.copperthick =
                ALTIUM_PARSER::PropertiesReadKicadUnit( properties, layeri + "COPTHICK", "1.4mil" );

        curlayer.dielectricconst =
                ALTIUM_PARSER::PropertiesReadDouble( properties, layeri + "DIELCONST", 0. );
        curlayer.dielectricthick = ALTIUM_PARSER::PropertiesReadKicadUnit(
                properties, layeri + "DIELHEIGHT", "60mil" );
        curlayer.dielectricmaterial =
                ALTIUM_PARSER::PropertiesReadString( properties, layeri + "DIELMATERIAL", "FR-4" );

        stackup.push_back( curlayer );
    }

    altium_parse_polygons( properties, board_vertices );

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Board6 stream was not parsed correctly!" );
    }
}

ACLASS6::ACLASS6( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Classes6 stream has no properties!" );
    }

    name     = ALTIUM_PARSER::PropertiesReadString( properties, "NAME", "" );
    uniqueid = ALTIUM_PARSER::PropertiesReadString( properties, "UNIQUEID", "" );
    kind     = static_cast<ALTIUM_CLASS_KIND>(
            ALTIUM_PARSER::PropertiesReadInt( properties, "KIND", -1 ) );

    for( size_t i = 0; i < std::numeric_limits<size_t>::max(); i++ )
    {
        auto mit = properties.find( "M" + std::to_string( i ) );
        if( mit == properties.end() )
        {
            break; // it doesn't seem like we know beforehand how many components are in the netclass
        }
        names.push_back( mit->second );
    }

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Classes6 stream was not parsed correctly" );
    }
}

ACOMPONENT6::ACOMPONENT6( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Components6 stream has no properties" );
    }

    layer = altium_layer_from_name(
            ALTIUM_PARSER::PropertiesReadString( properties, "LAYER", "" ) );
    position         = wxPoint( ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "X", "0mil" ),
            -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "Y", "0mil" ) );
    rotation         = ALTIUM_PARSER::PropertiesReadDouble( properties, "ROTATION", 0. );
    locked           = ALTIUM_PARSER::PropertiesReadBool( properties, "LOCKED", false );
    nameon           = ALTIUM_PARSER::PropertiesReadBool( properties, "NAMEON", true );
    commenton        = ALTIUM_PARSER::PropertiesReadBool( properties, "COMMENTON", false );
    sourcedesignator = ALTIUM_PARSER::PropertiesReadString( properties, "SOURCEDESIGNATOR", "" );
    sourcefootprintlibrary =
            ALTIUM_PARSER::PropertiesReadString( properties, "SOURCEFOOTPRINTLIBRARY", "" );
    pattern = ALTIUM_PARSER::PropertiesReadString( properties, "PATTERN", "" );

    sourcecomponentlibrary =
            ALTIUM_PARSER::PropertiesReadString( properties, "SOURCECOMPONENTLIBRARY", "" );
    sourcelibreference =
            ALTIUM_PARSER::PropertiesReadString( properties, "SOURCELIBREFERENCE", "" );

    nameautoposition = static_cast<ALTIUM_TEXT_POSITION>(
            ALTIUM_PARSER::PropertiesReadInt( properties, "NAMEAUTOPOSITION", 0 ) );
    commentautoposition = static_cast<ALTIUM_TEXT_POSITION>(
            ALTIUM_PARSER::PropertiesReadInt( properties, "COMMENTAUTOPOSITION", 0 ) );

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Components6 stream was not parsed correctly" );
    }
}

ADIMENSION6::ADIMENSION6( ALTIUM_PARSER& aReader )
{
    aReader.Skip( 2 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Dimensions6 stream has no properties" );
    }

    layer = altium_layer_from_name(
            ALTIUM_PARSER::PropertiesReadString( properties, "LAYER", "" ) );
    kind = static_cast<ALTIUM_DIMENSION_KIND>(
            ALTIUM_PARSER::PropertiesReadInt( properties, "DIMENSIONKIND", 0 ) );

    textformat = ALTIUM_PARSER::PropertiesReadString( properties, "TEXTFORMAT", "" );

    height = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "HEIGHT", "0mil" );
    angle  = ALTIUM_PARSER::PropertiesReadDouble( properties, "ANGLE", 0. );

    linewidth      = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "LINEWIDTH", "10mil" );
    textheight     = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "TEXTHEIGHT", "10mil" );
    textlinewidth  = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "TEXTLINEWIDTH", "6mil" );
    textprecision  = ALTIUM_PARSER::PropertiesReadInt( properties, "TEXTPRECISION", 2 );
    textbold       = ALTIUM_PARSER::PropertiesReadBool( properties, "TEXTLINEWIDTH", false );
    textitalic     = ALTIUM_PARSER::PropertiesReadBool( properties, "ITALIC", false );

    arrowsize = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "ARROWSIZE", "60mil" );

    xy1 = wxPoint( ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "X1", "0mil" ),
            -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "Y1", "0mil" ) );

    int refcount = ALTIUM_PARSER::PropertiesReadInt( properties, "REFERENCES_COUNT", 0 );
    for( int i = 0; i < refcount; i++ )
    {
        const std::string refi = "REFERENCE" + std::to_string( i );
        referencePoint.emplace_back(
                ALTIUM_PARSER::PropertiesReadKicadUnit( properties, refi + "POINTX", "0mil" ),
                -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, refi + "POINTY", "0mil" ) );
    }

    for( size_t i = 1; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const std::string texti  = "TEXT" + std::to_string( i );
        const std::string textix = texti + "X";
        const std::string textiy = texti + "Y";

        if( properties.find( textix ) == properties.end()
                || properties.find( textiy ) == properties.end() )
        {
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon
        }

        textPoint.emplace_back(
                ALTIUM_PARSER::PropertiesReadKicadUnit( properties, textix, "0mil" ),
                -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, textiy, "0mil" ) );
    }

    wxString dimensionunit =
            ALTIUM_PARSER::PropertiesReadString( properties, "TEXTDIMENSIONUNIT", "Millimeters" );
    if( dimensionunit == "Inches" )
    {
        textunit = ALTIUM_UNIT::INCHES;
    }
    else if( dimensionunit == "Mils" )
    {
        textunit = ALTIUM_UNIT::MILS;
    }
    else if( dimensionunit == "Millimeters" )
    {
        textunit = ALTIUM_UNIT::MILLIMETERS;
    }
    else if( dimensionunit == "Centimeters" )
    {
        textunit = ALTIUM_UNIT::CENTIMETER;
    }
    else
    {
        textunit = ALTIUM_UNIT::UNKNOWN;
    }

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Dimensions6 stream was not parsed correctly" );
    }
}

AMODEL::AMODEL( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Classes6 stream has no properties!" );
    }

    name       = ALTIUM_PARSER::PropertiesReadString( properties, "NAME", "" );
    id         = ALTIUM_PARSER::PropertiesReadString( properties, "ID", "" );
    isEmbedded = ALTIUM_PARSER::PropertiesReadBool( properties, "EMBED", false );

    rotation.x = ALTIUM_PARSER::PropertiesReadDouble( properties, "ROTX", 0. );
    rotation.y = ALTIUM_PARSER::PropertiesReadDouble( properties, "ROTY", 0. );
    rotation.z = ALTIUM_PARSER::PropertiesReadDouble( properties, "ROTZ", 0. );

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Classes6 stream was not parsed correctly" );
    }
}

ANET6::ANET6( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Nets6 stream has no properties" );
    }

    name = ALTIUM_PARSER::PropertiesReadString( properties, "NAME", "" );

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Nets6 stream was not parsed correctly" );
    }
}

APOLYGON6::APOLYGON6( ALTIUM_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Polygons6 stream has no properties" );
    }

    layer = altium_layer_from_name(
            ALTIUM_PARSER::PropertiesReadString( properties, "LAYER", "" ) );
    net    = ALTIUM_PARSER::PropertiesReadInt( properties, "NET", ALTIUM_NET_UNCONNECTED );
    locked = ALTIUM_PARSER::PropertiesReadBool( properties, "LOCKED", false );

    // TODO: kind

    gridsize      = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "GRIDSIZE", "0mil" );
    trackwidth    = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "TRACKWIDTH", "0mil" );
    minprimlength = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "MINPRIMLENGTH", "0mil" );
    useoctagons   = ALTIUM_PARSER::PropertiesReadBool( properties, "USEOCTAGONS", false );

    pourindex = ALTIUM_PARSER::PropertiesReadInt( properties, "POURINDEX", 0 );

    wxString hatchstyleraw = ALTIUM_PARSER::PropertiesReadString( properties, "HATCHSTYLE", "" );

    if( hatchstyleraw == "Solid" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::SOLID;
    }
    else if( hatchstyleraw == "45Degree" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::DEGREE_45;
    }
    else if( hatchstyleraw == "90Degree" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::DEGREE_90;
    }
    else if( hatchstyleraw == "Horizontal" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::HORIZONTAL;
    }
    else if( hatchstyleraw == "Vertical" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::VERTICAL;
    }
    else if( hatchstyleraw == "None" )
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::NONE;
    }
    else
    {
        hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::UNKNOWN;
    }

    altium_parse_polygons( properties, vertices );

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Polygons6 stream was not parsed correctly" );
    }
}

ARULE6::ARULE6( ALTIUM_PARSER& aReader )
{
    // Initialize all variables and make Coverity happy
    clearanceGap                       = 0;
    planeclearanceClearance            = 0;
    polygonconnectAirgapwidth          = 0;
    polygonconnectReliefconductorwidth = 0;
    polygonconnectReliefentries        = 0;
    polygonconnectStyle                = ALTIUM_CONNECT_STYLE::UNKNOWN;

    aReader.Skip( 2 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Rules6 stream has no properties" );
    }

    name     = ALTIUM_PARSER::PropertiesReadString( properties, "NAME", "" );
    priority = ALTIUM_PARSER::PropertiesReadInt( properties, "PRIORITY", 1 );

    scope1expr = ALTIUM_PARSER::PropertiesReadString( properties, "SCOPE1EXPRESSION", "" );
    scope2expr = ALTIUM_PARSER::PropertiesReadString( properties, "SCOPE2EXPRESSION", "" );

    wxString rulekind = ALTIUM_PARSER::PropertiesReadString( properties, "RULEKIND", "" );
    if( rulekind == "Clearance" )
    {
        kind         = ALTIUM_RULE_KIND::CLEARANCE;
        clearanceGap = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "GAP", "10mil" );
    }
    else if( rulekind == "DiffPairsRouting" )
    {
        kind = ALTIUM_RULE_KIND::DIFF_PAIR_ROUTINGS;
    }
    else if( rulekind == "Height" )
    {
        kind = ALTIUM_RULE_KIND::HEIGHT;
    }
    else if( rulekind == "HoleSize" )
    {
        kind = ALTIUM_RULE_KIND::HOLE_SIZE;
    }
    else if( rulekind == "HoleToHoleClearance" )
    {
        kind = ALTIUM_RULE_KIND::HOLE_TO_HOLE_CLEARANCE;
    }
    else if( rulekind == "Width" )
    {
        kind = ALTIUM_RULE_KIND::WIDTH;
    }
    else if( rulekind == "PasteMaskExpansion" )
    {
        kind = ALTIUM_RULE_KIND::PASTE_MASK_EXPANSION;
    }
    else if( rulekind == "PlaneClearance" )
    {
        kind = ALTIUM_RULE_KIND::PLANE_CLEARANCE;
        planeclearanceClearance =
                ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "CLEARANCE", "10mil" );
    }
    else if( rulekind == "PolygonConnect" )
    {
        kind = ALTIUM_RULE_KIND::POLYGON_CONNECT;
        polygonconnectAirgapwidth =
                ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "AIRGAPWIDTH", "10mil" );
        polygonconnectReliefconductorwidth = ALTIUM_PARSER::PropertiesReadKicadUnit(
                properties, "RELIEFCONDUCTORWIDTH", "10mil" );
        polygonconnectReliefentries =
                ALTIUM_PARSER::PropertiesReadInt( properties, "RELIEFENTRIES", 4 );

        wxString style = ALTIUM_PARSER::PropertiesReadString( properties, "CONNECTSTYLE", "" );

        if( style == "Direct" )
            polygonconnectStyle = ALTIUM_CONNECT_STYLE::DIRECT;
        else if( style == "Relief" )
            polygonconnectStyle = ALTIUM_CONNECT_STYLE::RELIEF;
        else if( style == "NoConnect" )
            polygonconnectStyle = ALTIUM_CONNECT_STYLE::NONE;
        else
            polygonconnectStyle = ALTIUM_CONNECT_STYLE::UNKNOWN;
    }
    else
    {
        kind = ALTIUM_RULE_KIND::UNKNOWN;
    }

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Rules6 stream was not parsed correctly" );
    }
}

AARC6::AARC6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::ARC )
    {
        THROW_IO_ERROR( "Arcs6 stream has invalid recordtype" );
    }

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1    = aReader.Read<uint8_t>();
    is_locked         = ( flags1 & 0x04 ) == 0;
    is_polygonoutline = ( flags1 & 0x02 ) != 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net          = aReader.Read<uint16_t>();
    subpolyindex = aReader.Read<uint16_t>();
    component    = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    center     = aReader.ReadWxPoint();
    radius     = aReader.ReadKicadUnit();
    startangle = aReader.Read<double>();
    endangle   = aReader.Read<double>();
    width      = aReader.ReadKicadUnit();

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Arcs6 stream was not parsed correctly" );
    }
}

ACOMPONENTBODY6::ACOMPONENTBODY6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::MODEL )
    {
        THROW_IO_ERROR( "ComponentsBodies6 stream has invalid recordtype" );
    }

    aReader.ReadAndSetSubrecordLength();

    aReader.Skip( 7 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 9 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "ComponentsBodies6 stream has no properties" );
    }

    modelName       = ALTIUM_PARSER::PropertiesReadString( properties, "MODEL.NAME", "" );
    modelId         = ALTIUM_PARSER::PropertiesReadString( properties, "MODELID", "" );
    modelIsEmbedded = ALTIUM_PARSER::PropertiesReadBool( properties, "MODEL.EMBED", false );

    modelPosition.x = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "MODEL.2D.X", "0mil" );
    modelPosition.y = -ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "MODEL.2D.Y", "0mil" );
    modelPosition.z = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "MODEL.3D.DZ", "0mil" );

    modelRotation.x = ALTIUM_PARSER::PropertiesReadDouble( properties, "MODEL.3D.ROTX", 0. );
    modelRotation.y = ALTIUM_PARSER::PropertiesReadDouble( properties, "MODEL.3D.ROTY", 0. );
    modelRotation.z = ALTIUM_PARSER::PropertiesReadDouble( properties, "MODEL.3D.ROTZ", 0. );

    rotation = ALTIUM_PARSER::PropertiesReadDouble( properties, "MODEL.2D.ROTATION", 0. );

    bodyOpacity = ALTIUM_PARSER::PropertiesReadDouble( properties, "BODYOPACITY3D", 1. );

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Components6 stream was not parsed correctly" );
    }
}

APAD6::APAD6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::PAD )
        THROW_IO_ERROR( "Pads6 stream has invalid recordtype" );

    // Subrecord 1
    size_t subrecord1 = aReader.ReadAndSetSubrecordLength();

    if( subrecord1 == 0 )
        THROW_IO_ERROR( "Pads6 stream has no subrecord1 data" );

    name = aReader.ReadWxString();

    if( aReader.GetRemainingSubrecordBytes() != 0 )
        THROW_IO_ERROR( "Pads6 stream has invalid subrecord1 length" );

    aReader.SkipSubrecord();

    // Subrecord 2
    aReader.ReadAndSetSubrecordLength();
    aReader.SkipSubrecord();

    // Subrecord 3
    aReader.ReadAndSetSubrecordLength();
    aReader.SkipSubrecord();

    // Subrecord 4
    aReader.ReadAndSetSubrecordLength();
    aReader.SkipSubrecord();

    // Subrecord 5
    size_t subrecord5 = aReader.ReadAndSetSubrecordLength();

    if( subrecord5 < 114 )
        THROW_IO_ERROR( "Pads6 stream subrecord has length < 114, which is unexpected" );

    layer     = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
    tolayer   = ALTIUM_LAYER::UNKNOWN;
    fromlayer = ALTIUM_LAYER::UNKNOWN;

    uint8_t flags1  = aReader.Read<uint8_t>();
    is_test_fab_top = ( flags1 & 0x80 ) != 0;
    is_tent_bottom  = ( flags1 & 0x40 ) != 0;
    is_tent_top     = ( flags1 & 0x20 ) != 0;
    is_locked       = ( flags1 & 0x04 ) == 0;

    uint8_t flags2     = aReader.Read<uint8_t>();
    is_test_fab_bottom = ( flags2 & 0x01 ) != 0;

    net = aReader.Read<uint16_t>();
    aReader.Skip( 2 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 4 );

    position = aReader.ReadWxPoint();
    topsize  = aReader.ReadWxSize();
    midsize  = aReader.ReadWxSize();
    botsize  = aReader.ReadWxSize();
    holesize = aReader.ReadKicadUnit();

    topshape = static_cast<ALTIUM_PAD_SHAPE>( aReader.Read<uint8_t>() );
    midshape = static_cast<ALTIUM_PAD_SHAPE>( aReader.Read<uint8_t>() );
    botshape = static_cast<ALTIUM_PAD_SHAPE>( aReader.Read<uint8_t>() );

    direction = aReader.Read<double>();
    plated    = aReader.Read<uint8_t>() != 0;
    aReader.Skip( 1 );
    padmode = static_cast<ALTIUM_PAD_MODE>( aReader.Read<uint8_t>() );
    aReader.Skip( 23 );
    pastemaskexpansionmanual  = aReader.ReadKicadUnit();
    soldermaskexpansionmanual = aReader.ReadKicadUnit();
    aReader.Skip( 7 );
    pastemaskexpansionmode  = static_cast<ALTIUM_PAD_RULE>( aReader.Read<uint8_t>() );
    soldermaskexpansionmode = static_cast<ALTIUM_PAD_RULE>( aReader.Read<uint8_t>() );
    aReader.Skip( 3 );
    holerotation = aReader.Read<double>();

    if( subrecord5 >= 120 )
    {
        tolayer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
        aReader.Skip( 2 );
        fromlayer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
        //aReader.skip( 2 );
    }
    else if( subrecord5 == 171 )
    {
    }

    aReader.SkipSubrecord();

    // Subrecord 6
    size_t subrecord6 = aReader.ReadAndSetSubrecordLength();
    // Known lengths: 596, 628, 651
    // 596 is the number of bytes read in this code-block
    if( subrecord6 >= 596 )
    { // TODO: detect type from something else than the size?
        sizeAndShape = std::make_unique<APAD6_SIZE_AND_SHAPE>();

        for( wxSize& size : sizeAndShape->inner_size )
            size.x = aReader.ReadKicadUnitX();

        for( wxSize& size : sizeAndShape->inner_size )
            size.y = aReader.ReadKicadUnitY();

        for( ALTIUM_PAD_SHAPE& shape : sizeAndShape->inner_shape )
            shape = static_cast<ALTIUM_PAD_SHAPE>( aReader.Read<uint8_t>() );

        aReader.Skip( 1 );

        sizeAndShape->holeshape    = static_cast<ALTIUM_PAD_HOLE_SHAPE>( aReader.Read<uint8_t>() );
        sizeAndShape->slotsize     = aReader.ReadKicadUnit();
        sizeAndShape->slotrotation = aReader.Read<double>();

        for( wxPoint& pt : sizeAndShape->holeoffset )
            pt.x = aReader.ReadKicadUnitX();

        for( wxPoint& pt : sizeAndShape->holeoffset )
            pt.y = aReader.ReadKicadUnitY();

        aReader.Skip( 1 );

        for( ALTIUM_PAD_SHAPE_ALT& shape : sizeAndShape->alt_shape )
            shape = static_cast<ALTIUM_PAD_SHAPE_ALT>( aReader.Read<uint8_t>() );

        for( uint8_t& radius : sizeAndShape->cornerradius )
            radius = aReader.Read<uint8_t>();
    }
    else if( subrecord6 != 0 )
    {
        wxLogError( _( "Pads6 stream has unexpected length for subrecord 6: %d." ), subrecord6 );
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( "Pads6 stream was not parsed correctly" );
}

AVIA6::AVIA6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::VIA )
    {
        THROW_IO_ERROR( "Vias6 stream has invalid recordtype" );
    }

    // Subrecord 1
    size_t subrecord1 = aReader.ReadAndSetSubrecordLength();

    aReader.Skip( 1 );

    uint8_t flags1  = aReader.Read<uint8_t>();
    is_test_fab_top = ( flags1 & 0x80 ) != 0;
    is_tent_bottom  = ( flags1 & 0x40 ) != 0;
    is_tent_top     = ( flags1 & 0x20 ) != 0;
    is_locked       = ( flags1 & 0x04 ) == 0;

    uint8_t flags2     = aReader.Read<uint8_t>();
    is_test_fab_bottom = ( flags2 & 0x01 ) != 0;

    net = aReader.Read<uint16_t>();
    aReader.Skip( 8 );
    position = aReader.ReadWxPoint();
    diameter = aReader.ReadKicadUnit();
    holesize = aReader.ReadKicadUnit();

    layer_start = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
    layer_end   = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    if( subrecord1 <= 74 )
    {
        viamode = ALTIUM_PAD_MODE::SIMPLE;
    }
    else
    {
        aReader.Skip( 43 );
        viamode = static_cast<ALTIUM_PAD_MODE>( aReader.Read<uint8_t>() );
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Vias6 stream was not parsed correctly" );
    }
}

ATRACK6::ATRACK6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::TRACK )
    {
        THROW_IO_ERROR( "Tracks6 stream has invalid recordtype" );
    }

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1    = aReader.Read<uint8_t>();
    is_locked         = ( flags1 & 0x04 ) == 0;
    is_polygonoutline = ( flags1 & 0x02 ) != 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net          = aReader.Read<uint16_t>();
    subpolyindex = aReader.Read<uint16_t>();
    component    = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    start = aReader.ReadWxPoint();
    end   = aReader.ReadWxPoint();
    width = aReader.ReadKicadUnit();

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Tracks6 stream was not parsed correctly" );
    }
}

ATEXT6::ATEXT6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::TEXT )
    {
        THROW_IO_ERROR( "Texts6 stream has invalid recordtype" );
    }

    // Subrecord 1 - Properties
    size_t subrecord1 = aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
    aReader.Skip( 6 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    position = aReader.ReadWxPoint();
    height   = aReader.ReadKicadUnit();
    aReader.Skip( 2 );
    rotation     = aReader.Read<double>();
    isMirrored   = aReader.Read<uint8_t>() != 0;
    strokewidth  = aReader.ReadKicadUnit();
    isComment    = aReader.Read<uint8_t>() != 0;
    isDesignator = aReader.Read<uint8_t>() != 0;
    aReader.Skip( 2 );
    isBold   = aReader.Read<uint8_t>() != 0;
    isItalic = aReader.Read<uint8_t>() != 0;
    aReader.Skip( 64 ); // font_name
    isInverted = aReader.Read<uint8_t>() != 0;
    aReader.Skip( 21 );
    textposition = static_cast<ALTIUM_TEXT_POSITION>( aReader.Read<uint8_t>() );
    /**
     * In Altium 14 (subrecord1 == 230) only left bottom is valid? I think there is a bit missing.
     * https://gitlab.com/kicad/code/kicad/merge_requests/60#note_274913397
     */
    if( subrecord1 <= 230 )
    {
        textposition = ALTIUM_TEXT_POSITION::LEFT_BOTTOM;
    }
    aReader.Skip( 27 );
    fonttype = static_cast<ALTIUM_TEXT_TYPE>( aReader.Read<uint8_t>() );

    aReader.SkipSubrecord();

    // Subrecord 2 - String
    aReader.ReadAndSetSubrecordLength();

    text = aReader.ReadWxString(); // TODO: what about strings with length > 255?

    // Normalize Windows line endings
    text.Replace( "\r\n", "\n" );

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Texts6 stream was not parsed correctly" );
    }
}

AFILL6::AFILL6( ALTIUM_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::FILL )
    {
        THROW_IO_ERROR( "Fills6 stream has invalid recordtype" );
    }

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1 = aReader.Read<uint8_t>();
    is_locked      = ( flags1 & 0x04 ) == 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net = aReader.Read<uint16_t>();
    aReader.Skip( 2 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    pos1     = aReader.ReadWxPoint();
    pos2     = aReader.ReadWxPoint();
    rotation = aReader.Read<double>();

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Fills6 stream was not parsed correctly" );
    }
}

AREGION6::AREGION6( ALTIUM_PARSER& aReader, bool aExtendedVertices )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::REGION )
    {
        THROW_IO_ERROR( "Regions6 stream has invalid recordtype" );
    }

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1 = aReader.Read<uint8_t>();
    is_locked      = ( flags1 & 0x04 ) == 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net = aReader.Read<uint16_t>();
    subpolyindex = aReader.Read<uint16_t>();
    component = aReader.Read<uint16_t>();
    aReader.Skip( 5 );
    holecount = aReader.Read<uint16_t>();
    aReader.Skip( 2 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();
    if( properties.empty() )
    {
        THROW_IO_ERROR( "Regions6 stream has empty properties" );
    }

    int  pkind     = ALTIUM_PARSER::PropertiesReadInt( properties, "KIND", 0 );
    bool is_cutout = ALTIUM_PARSER::PropertiesReadBool( properties, "ISBOARDCUTOUT", false );

    is_shapebased = ALTIUM_PARSER::PropertiesReadBool( properties, "ISSHAPEBASED", false );

    // TODO: this can differ from the other subpolyindex?!
    //subpolyindex = static_cast<uint16_t>(
    //        ALTIUM_PARSER::PropertiesReadInt( properties, "SUBPOLYINDEX", ALTIUM_POLYGON_NONE ) );

    switch( pkind )
    {
    case 0:
        if( is_cutout )
        {
            kind = ALTIUM_REGION_KIND::BOARD_CUTOUT;
        }
        else
        {
            kind = ALTIUM_REGION_KIND::COPPER;
        }
        break;
    case 1:
        kind = ALTIUM_REGION_KIND::POLYGON_CUTOUT;
        break;
    case 2:
        kind = ALTIUM_REGION_KIND::UNKNOWN_2; // TODO: what kind is this?
        break;
    case 3:
        kind = ALTIUM_REGION_KIND::UNKNOWN_3; // TODO: what kind is this?
        break;
    case 4:
        kind = ALTIUM_REGION_KIND::CAVITY_DEFINITION;
        break;
    default:
        kind = ALTIUM_REGION_KIND::UNKNOWN;
        break;
    }

    uint32_t num_outline_vertices = aReader.Read<uint32_t>();

    for( uint32_t i = 0; i < num_outline_vertices; i++ )
    {
        if( aExtendedVertices )
        {
            bool    isRound  = aReader.Read<uint8_t>() != 0;
            wxPoint position = aReader.ReadWxPoint();
            wxPoint center   = aReader.ReadWxPoint();
            int32_t radius   = aReader.ReadKicadUnit();
            double  angle1   = aReader.Read<double>();
            double  angle2   = aReader.Read<double>();
            outline.emplace_back( isRound, radius, angle1, angle2, position, center );
        }
        else
        {
            // For some regions the coordinates are stored as double and not as int32_t
            int32_t x = ALTIUM_PARSER::ConvertToKicadUnit( aReader.Read<double>() );
            int32_t y = ALTIUM_PARSER::ConvertToKicadUnit( -aReader.Read<double>() );
            outline.emplace_back( wxPoint( x, y ) );
        }
    }

    // TODO: for now we only support holes in regions where there are stored as double
    if( !aExtendedVertices )
    {
        holes.resize( holecount );
        for( uint16_t k = 0; k < holecount; k++ )
        {
            uint32_t num_hole_vertices = aReader.Read<uint32_t>();
            holes.at( k ).reserve( num_hole_vertices );

            for( uint32_t i = 0; i < num_hole_vertices; i++ )
            {
                int32_t x = ALTIUM_PARSER::ConvertToKicadUnit( aReader.Read<double>() );
                int32_t y = ALTIUM_PARSER::ConvertToKicadUnit( -aReader.Read<double>() );
                holes.at( k ).emplace_back( wxPoint( x, y ) );
            }
        }
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( "Regions6 stream was not parsed correctly" );
    }
}
