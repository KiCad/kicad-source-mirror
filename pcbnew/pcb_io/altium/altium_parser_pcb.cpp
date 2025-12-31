/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <map>
#include <unordered_map>

#include <ki_exception.h>
#include <math/util.h>

#include <wx/log.h>

#include "altium_parser_pcb.h"
#include "io/altium/altium_binary_parser.h"
#include "io/altium/altium_props_utils.h"


/**
 * Flag to enable Altium importer logging.
 *
 * Use "KICAD_ALTIUM_IMPORT" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* traceAltiumImport = wxT( "KICAD_ALTIUM_IMPORT" );


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

void altium_parse_polygons( std::map<wxString, wxString>& aProps,
                            std::vector<ALTIUM_VERTICE>& aVertices )
{
    for( size_t i = 0; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const wxString si = std::to_string( i );

        const wxString vxi = wxT( "VX" ) + si;
        const wxString vyi = wxT( "VY" ) + si;

        if( aProps.find( vxi ) == aProps.end() || aProps.find( vyi ) == aProps.end() )
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon

        const bool     isRound = ALTIUM_PROPS_UTILS::ReadInt( aProps, wxT( "KIND" ) + si, 0 ) != 0;
        const int32_t  radius = ALTIUM_PROPS_UTILS::ReadKicadUnit( aProps, wxT( "R" ) + si, wxT( "0mil" ) );
        const double   sa = ALTIUM_PROPS_UTILS::ReadDouble( aProps, wxT( "SA" ) + si, 0. );
        const double   ea = ALTIUM_PROPS_UTILS::ReadDouble( aProps, wxT( "EA" ) + si, 0. );
        const VECTOR2I vp = VECTOR2I( ALTIUM_PROPS_UTILS::ReadKicadUnit( aProps, vxi, wxT( "0mil" ) ),
                                     -ALTIUM_PROPS_UTILS::ReadKicadUnit( aProps, vyi, wxT( "0mil" ) ) );
        const VECTOR2I cp = VECTOR2I( ALTIUM_PROPS_UTILS::ReadKicadUnit( aProps, wxT( "CX" ) + si, wxT( "0mil" ) ),
                                     -ALTIUM_PROPS_UTILS::ReadKicadUnit( aProps, wxT( "CY" ) + si, wxT( "0mil" ) ) );

        aVertices.emplace_back( isRound, radius, sa, ea, vp, cp );
    }
}


static ALTIUM_MODE ReadAltiumModeFromProperties( const std::map<wxString, wxString>& aProps,
                                                 wxString                            aKey )
{
    wxString mode = ALTIUM_PROPS_UTILS::ReadString( aProps, aKey, wxT( "" ) );

    if( mode == wxT( "None" ) )
        return ALTIUM_MODE::NONE;
    else if( mode == wxT( "Rule" ) )
        return ALTIUM_MODE::RULE;
    else if( mode == wxT( "Manual" ) )
        return ALTIUM_MODE::MANUAL;

    wxLogError( _( "Unknown Mode string: '%s'." ), mode );
    return ALTIUM_MODE::UNKNOWN;
}


static ALTIUM_RECORD ReadAltiumRecordFromProperties( const std::map<wxString, wxString>& aProps,
                                                     wxString                            aKey )
{
    wxString record = ALTIUM_PROPS_UTILS::ReadString( aProps, aKey, wxT( "" ) );

    if( record == wxT( "Arc" ) )
        return ALTIUM_RECORD::ARC;
    else if( record == wxT( "Pad" ) )
        return ALTIUM_RECORD::PAD;
    else if( record == wxT( "Via" ) )
        return ALTIUM_RECORD::VIA;
    else if( record == wxT( "Track" ) )
        return ALTIUM_RECORD::TRACK;
    else if( record == wxT( "Text" ) )
        return ALTIUM_RECORD::TEXT;
    else if( record == wxT( "Fill" ) )
        return ALTIUM_RECORD::FILL;
    else if( record == wxT( "Region" ) ) // correct?
        return ALTIUM_RECORD::REGION;
    else if( record == wxT( "Model" ) )
        return ALTIUM_RECORD::MODEL;

    wxLogError( _( "Unknown Record name string: '%s'." ), record );
    return ALTIUM_RECORD::UNKNOWN;
}


static AEXTENDED_PRIMITIVE_INFORMATION_TYPE
ReadAltiumExtendedPrimitiveInformationTypeFromProperties(
        const std::map<wxString, wxString>& aProps, wxString aKey )
{
    wxString parsedType = ALTIUM_PROPS_UTILS::ReadString( aProps, aKey, wxT( "" ) );

    if( parsedType == wxT( "Mask" ) )
        return AEXTENDED_PRIMITIVE_INFORMATION_TYPE::MASK;

    wxLogError( _( "Unknown Extended Primitive Information type: '%s'." ), parsedType );
    return AEXTENDED_PRIMITIVE_INFORMATION_TYPE::UNKNOWN;
}


/**
 *  Throw an IO_ERROR if the actual length is less than the expected length.
 *
 * @param aStreamType the current stream type (e.g. 'Pads6')
 * @param aSubrecordName the current subrecord name (e.g. 'subrecord5')
 * @param aExpectedLength the expected length needed to parse the stream
 * @param aActualLength the actual length of the subrecord encountered
 */
static void ExpectSubrecordLengthAtLeast( const std::string& aStreamType,
                                          const std::string& aSubrecordName, size_t aExpectedLength,
                                          size_t aActualLength )
{
    if( aActualLength < aExpectedLength )
    {
        THROW_IO_ERROR( wxString::Format( "%s stream %s has length %d, "
                                          "which is unexpected (expected at least %d)",
                                          aStreamType, aSubrecordName, aActualLength,
                                          aExpectedLength ) );
    }
}


AEXTENDED_PRIMITIVE_INFORMATION::AEXTENDED_PRIMITIVE_INFORMATION( ALTIUM_BINARY_PARSER& aReader )
{
    const std::map<wxString, wxString> props = aReader.ReadProperties();

    if( props.empty() )
        THROW_IO_ERROR( wxT( "ExtendedPrimitiveInformation stream has no properties!" ) );

    primitiveIndex = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "PRIMITIVEINDEX" ), -1 );
    primitiveObjectId = ReadAltiumRecordFromProperties( props, wxT( "PRIMITIVEOBJECTID" ) );
    type = ReadAltiumExtendedPrimitiveInformationTypeFromProperties( props, wxT( "TYPE" ) );

    pastemaskexpansionmode = ReadAltiumModeFromProperties( props, wxT( "PASTEMASKEXPANSIONMODE" ) );
    pastemaskexpansionmanual = ALTIUM_PROPS_UTILS::ReadKicadUnit(
            props, wxT( "PASTEMASKEXPANSION_MANUAL" ), wxT( "0mil" ) );
    soldermaskexpansionmode =
            ReadAltiumModeFromProperties( props, wxT( "SOLDERMASKEXPANSIONMODE" ) );
    soldermaskexpansionmanual = ALTIUM_PROPS_UTILS::ReadKicadUnit(
            props, wxT( "SOLDERMASKEXPANSION_MANUAL" ), wxT( "0mil" ) );
}


ABOARD6::ABOARD6( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> props = aReader.ReadProperties();

    if( props.empty() )
        THROW_IO_ERROR( wxT( "Board6 stream has no properties!" ) );

    sheetpos  = VECTOR2I( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "SHEETX" ), wxT( "0mil" ) ),
                          -ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "SHEETY" ), wxT( "0mil" ) ) );
    sheetsize = wxSize( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "SHEETWIDTH" ), wxT( "0mil" ) ),
                        ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "SHEETHEIGHT" ), wxT( "0mil" ) ) );

    layercount = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "LAYERSETSCOUNT" ), 1 ) + 1;

    for( size_t i = 1; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const wxString layeri    = wxT( "LAYER" ) + wxString( std::to_string( i ) );
        const wxString layername = layeri + wxT( "NAME" );

        auto layernameit = props.find( layername );

        if( layernameit == props.end() )
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon

        ABOARD6_LAYER_STACKUP l;

        l.name = ALTIUM_PROPS_UTILS::ReadString( props, layername, wxT( "" ) );
        wxString originalName = l.name;
        int ii = 2;

        // Ensure that layer names are unique in KiCad
        while( !layerNames.insert( l.name ).second )
            l.name = wxString::Format( wxT( "%s %d" ), originalName, ii++ );

        l.nextId = ALTIUM_PROPS_UTILS::ReadInt( props, layeri + wxT( "NEXT" ), 0 );
        l.prevId = ALTIUM_PROPS_UTILS::ReadInt( props, layeri + wxT( "PREV" ), 0 );
        l.copperthick = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, layeri + wxT( "COPTHICK" ), wxT( "1.4mil" ) );

        l.dielectricconst = ALTIUM_PROPS_UTILS::ReadDouble( props, layeri + wxT( "DIELCONST" ), 0. );
        l.dielectricthick = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, layeri + wxT( "DIELHEIGHT" ),  wxT( "60mil" ) );
        l.dielectricmaterial = ALTIUM_PROPS_UTILS::ReadString( props, layeri + wxT( "DIELMATERIAL" ), wxT( "FR-4" ) );

        stackup.push_back( l );
    }

    altium_parse_polygons( props, board_vertices );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Board6 stream was not parsed correctly!" ) );
}

ACLASS6::ACLASS6( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "Classes6 stream has no properties!" ) );

    name     = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "NAME" ), wxT( "" ) );
    uniqueid = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "UNIQUEID" ), wxT( "" ) );
    kind     = static_cast<ALTIUM_CLASS_KIND>( ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "KIND" ), -1 ) );

    for( size_t i = 0; i < std::numeric_limits<size_t>::max(); i++ )
    {
        auto mit = properties.find( wxT( "M" ) + wxString( std::to_string( i ) ) );

        if( mit == properties.end() )
            break; // it doesn't seem like we know beforehand how many components are in the netclass

        names.push_back( mit->second );
    }

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Classes6 stream was not parsed correctly" ) );
}

ACOMPONENT6::ACOMPONENT6( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> props = aReader.ReadProperties();

    if( props.empty() )
        THROW_IO_ERROR( wxT( "Components6 stream has no props" ) );

    layer = altium_layer_from_name( ALTIUM_PROPS_UTILS::ReadString( props, wxT( "LAYER" ), wxT( "" ) ) );
    position         = VECTOR2I( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "X" ), wxT( "0mil" ) ),
                                 -ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "Y" ), wxT( "0mil" ) ) );
    rotation         = ALTIUM_PROPS_UTILS::ReadDouble( props, wxT( "ROTATION" ), 0. );
    locked           = ALTIUM_PROPS_UTILS::ReadBool( props, wxT( "LOCKED" ), false );
    nameon           = ALTIUM_PROPS_UTILS::ReadBool( props, wxT( "NAMEON" ), true );
    commenton        = ALTIUM_PROPS_UTILS::ReadBool( props, wxT( "COMMENTON" ), false );
    sourcedesignator = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SOURCEDESIGNATOR" ), wxT( "" ) );

    sourceUniqueID = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SOURCEUNIQUEID" ), wxT( "" ) );

    // Remove leading backslash from sourceUniqueID to match schematic component unique IDs
    if( sourceUniqueID.starts_with( wxT( "\\" ) ) )
        sourceUniqueID = sourceUniqueID.Mid( 1 );

    sourceHierachicalPath = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SOURCEHIERARCHICALPATH" ), wxT( "" ) );
    sourcefootprintlibrary =
            ALTIUM_PROPS_UTILS::ReadUnicodeString( props, wxT( "SOURCEFOOTPRINTLIBRARY" ), wxT( "" ) );
    pattern = ALTIUM_PROPS_UTILS::ReadUnicodeString( props, wxT( "PATTERN" ), wxT( "" ) );

    sourcecomponentlibrary = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SOURCECOMPONENTLIBRARY" ), wxT( "" ) );
    sourcelibreference = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SOURCELIBREFERENCE" ), wxT( "" ) );

    nameautoposition = static_cast<ALTIUM_TEXT_POSITION>(
                                        ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "NAMEAUTOPOSITION" ), 0 ) );
    commentautoposition = static_cast<ALTIUM_TEXT_POSITION>(
                                        ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "COMMENTAUTOPOSITION" ), 0 ) );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Components6 stream was not parsed correctly" ) );
}

ADIMENSION6::ADIMENSION6( ALTIUM_BINARY_PARSER& aReader )
{
    aReader.Skip( 2 );

    std::map<wxString, wxString> props = aReader.ReadProperties();

    if( props.empty() )
        THROW_IO_ERROR( wxT( "Dimensions6 stream has no props" ) );

    layer = altium_layer_from_name( ALTIUM_PROPS_UTILS::ReadString( props, wxT( "LAYER" ), wxT( "" ) ) );
    kind = static_cast<ALTIUM_DIMENSION_KIND>( ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "DIMENSIONKIND" ), 0 ) );

    textformat = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "TEXTFORMAT" ), wxT( "" ) );
    textprefix = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "TEXTPREFIX" ), wxT( "" ) );
    textsuffix = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "TEXTSUFFIX" ), wxT( "" ) );

    height = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "HEIGHT" ), wxT( "0mil" ) );
    angle  = ALTIUM_PROPS_UTILS::ReadDouble( props, wxT( "ANGLE" ), 0. );

    linewidth      = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "LINEWIDTH" ), wxT( "10mil" ) );
    textheight     = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "TEXTHEIGHT" ), wxT( "10mil" ) );
    textlinewidth  = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "TEXTLINEWIDTH" ), wxT( "6mil" ) );
    textprecision  = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "TEXTPRECISION" ), 2 );
    textbold       = ALTIUM_PROPS_UTILS::ReadBool( props, wxT( "TEXTLINEWIDTH" ), false );
    textitalic     = ALTIUM_PROPS_UTILS::ReadBool( props, wxT( "ITALIC" ), false );
    textgap        = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "TEXTGAP" ), wxT( "10mil" ) );

    arrowsize = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "ARROWSIZE" ), wxT( "60mil" ) );

    wxString text_position_raw = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "TEXTPOSITION" ), wxT( "" ) );

    xy1 = VECTOR2I( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "X1" ), wxT( "0mil" ) ),
                    -ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "Y1" ), wxT( "0mil" ) ) );

    int refcount = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "REFERENCES_COUNT" ), 0 );

    for( int i = 0; i < refcount; i++ )
    {
        const std::string refi = "REFERENCE" + std::to_string( i ) + "POINT";
        const wxString ref( refi );
        referencePoint.emplace_back( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, ref + wxT( "X" ), wxT( "0mil" ) ),
                                     -ALTIUM_PROPS_UTILS::ReadKicadUnit( props, ref + wxT( "Y" ), wxT( "0mil" ) ) );
    }

    for( size_t i = 1; i < std::numeric_limits<size_t>::max(); i++ )
    {
        const std::string texti  = "TEXT" + std::to_string( i );
        const std::string textix = texti + "X";
        const std::string textiy = texti + "Y";

        if( props.find( textix ) == props.end() || props.find( textiy ) == props.end() )
            break; // it doesn't seem like we know beforehand how many vertices are inside a polygon

        textPoint.emplace_back( ALTIUM_PROPS_UTILS::ReadKicadUnit( props, textix, wxT( "0mil" ) ),
                                -ALTIUM_PROPS_UTILS::ReadKicadUnit( props, textiy, wxT( "0mil" ) ) );
    }

    wxString dimensionunit = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "TEXTDIMENSIONUNIT" ), wxT( "Millimeters" ) );

    if(      dimensionunit == wxT( "Inches" ) )      textunit = ALTIUM_UNIT::INCH;
    else if( dimensionunit == wxT( "Mils" ) )        textunit = ALTIUM_UNIT::MILS;
    else if( dimensionunit == wxT( "Millimeters" ) ) textunit = ALTIUM_UNIT::MM;
    else if( dimensionunit == wxT( "Centimeters" ) ) textunit = ALTIUM_UNIT::CM;
    else                                             textunit = ALTIUM_UNIT::UNKNOWN;

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Dimensions6 stream was not parsed correctly" ) );
}

AMODEL::AMODEL( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "Model stream has no properties!" ) );

    name       = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "NAME" ), wxT( "" ) );
    id         = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "ID" ), wxT( "" ) );
    isEmbedded = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "EMBED" ), false );

    rotation.x = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "ROTX" ), 0. );
    rotation.y = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "ROTY" ), 0. );
    rotation.z = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "ROTZ" ), 0. );

    z_offset = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "DZ" ), 0. );
    checksum = ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "CHECKSUM" ), 0 );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Model stream was not parsed correctly" ) );
}

ANET6::ANET6( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "Nets6 stream has no properties" ) );

    name = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "NAME" ), wxT( "" ) );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Nets6 stream was not parsed correctly" ) );
}

APOLYGON6::APOLYGON6( ALTIUM_BINARY_PARSER& aReader )
{
    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "Polygons6 stream has no properties" ) );

    layer  = altium_layer_from_name( ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "LAYER" ), wxT( "" ) ) );
    net    = ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "NET" ), ALTIUM_NET_UNCONNECTED );
    locked = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "LOCKED" ), false );

    // TODO: kind

    gridsize      = ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "GRIDSIZE" ), wxT( "0mil" ) );
    trackwidth    = ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "TRACKWIDTH" ), wxT( "0mil" ) );
    minprimlength = ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "MINPRIMLENGTH" ), wxT( "0mil" ) );
    useoctagons   = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "USEOCTAGONS" ), false );

    pourindex = ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "POURINDEX" ), 0 );

    wxString hatchstyleraw = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "HATCHSTYLE" ), wxT( "" ) );

    if(      hatchstyleraw == wxT( "Solid" ) )      hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::SOLID;
    else if( hatchstyleraw == wxT( "45Degree" ) )   hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::DEGREE_45;
    else if( hatchstyleraw == wxT( "90Degree" ) )   hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::DEGREE_90;
    else if( hatchstyleraw == wxT( "Horizontal" ) ) hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::HORIZONTAL;
    else if( hatchstyleraw == wxT( "Vertical" ) )   hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::VERTICAL;
    else if( hatchstyleraw == wxT( "None" ) )       hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::NONE;
    else                                            hatchstyle = ALTIUM_POLYGON_HATCHSTYLE::UNKNOWN;

    altium_parse_polygons( properties, vertices );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Polygons6 stream was not parsed correctly" ) );
}

ARULE6::ARULE6( ALTIUM_BINARY_PARSER& aReader )
{
    aReader.Skip( 2 );

    std::map<wxString, wxString> props = aReader.ReadProperties();

    if( props.empty() )
        THROW_IO_ERROR( wxT( "Rules6 stream has no props" ) );

    name     = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "NAME" ), wxT( "" ) );
    priority = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "PRIORITY" ), 1 );

    scope1expr = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SCOPE1EXPRESSION" ), wxT( "" ) );
    scope2expr = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "SCOPE2EXPRESSION" ), wxT( "" ) );

    wxString rulekind = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "RULEKIND" ), wxT( "" ) );
    if( rulekind == wxT( "Clearance" ) )
    {
        kind         = ALTIUM_RULE_KIND::CLEARANCE;
        clearanceGap = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "GAP" ), wxT( "10mil" ) );
    }
    else if( rulekind == wxT( "DiffPairsRouting" ) )
    {
        kind = ALTIUM_RULE_KIND::DIFF_PAIR_ROUTINGS;
    }
    else if( rulekind == wxT( "Height" ) )
    {
        kind = ALTIUM_RULE_KIND::HEIGHT;
    }
    else if( rulekind == wxT( "HoleSize" ) )
    {
        kind = ALTIUM_RULE_KIND::HOLE_SIZE;
        minLimit = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MINLIMIT" ), wxT( "1mil" ) );
        maxLimit = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MAXLIMIT" ), wxT( "150mil" ) );
    }
    else if( rulekind == wxT( "HoleToHoleClearance" ) )
    {
        kind = ALTIUM_RULE_KIND::HOLE_TO_HOLE_CLEARANCE;
        clearanceGap = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "GAP" ), wxT( "10mil" ) );
    }
    else if( rulekind == wxT( "RoutingVias" ) )
    {
        kind = ALTIUM_RULE_KIND::ROUTING_VIAS;
        width        = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "WIDTH" ), wxT( "20mil" ) );
        minWidth     = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MINWIDTH" ), wxT( "20mil" ) );
        maxWidth     = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MAXWIDTH" ), wxT( "50mil" ) );
        holeWidth    = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "HOLEWIDTH" ), wxT( "10mil" ) );
        minHoleWidth = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MINHOLEWIDTH" ), wxT( "10mil" ) );
        maxHoleWidth = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MAXHOLEWIDTH" ), wxT( "28mil" ) );
    }
    else if( rulekind == wxT( "Width" ) )
    {
        kind = ALTIUM_RULE_KIND::WIDTH;
        minLimit       = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MINLIMIT" ), wxT( "6mil" ) );
        maxLimit       = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "MAXLIMIT" ), wxT( "40mil" ) );
        preferredWidth = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "PREFERREDWIDTH" ), wxT( "6mil" ) );
}
    else if( rulekind == wxT( "PasteMaskExpansion" ) )
    {
        kind = ALTIUM_RULE_KIND::PASTE_MASK_EXPANSION;
        pastemaskExpansion = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "EXPANSION" ), wxT( "0" ) );
    }
    else if( rulekind == wxT( "SolderMaskExpansion" ) )
    {
        kind = ALTIUM_RULE_KIND::SOLDER_MASK_EXPANSION;
        soldermaskExpansion = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "EXPANSION" ), wxT( "4mil" ) );
    }
    else if( rulekind == wxT( "PlaneClearance" ) )
    {
        kind = ALTIUM_RULE_KIND::PLANE_CLEARANCE;
        planeclearanceClearance = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "CLEARANCE" ), wxT( "10mil" ) );
    }
    else if( rulekind == wxT( "PolygonConnect" ) )
    {
        kind = ALTIUM_RULE_KIND::POLYGON_CONNECT;
        polygonconnectAirgapwidth = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "AIRGAPWIDTH" ), wxT( "10mil" ) );
        polygonconnectReliefconductorwidth = ALTIUM_PROPS_UTILS::ReadKicadUnit( props, wxT( "RELIEFCONDUCTORWIDTH" ), wxT( "10mil" ) );
        polygonconnectReliefentries = ALTIUM_PROPS_UTILS::ReadInt( props, wxT( "RELIEFENTRIES" ), 4 );

        wxString style = ALTIUM_PROPS_UTILS::ReadString( props, wxT( "CONNECTSTYLE" ), wxT( "" ) );

        if(      style == wxT( "Direct" ) )    polygonconnectStyle = ALTIUM_CONNECT_STYLE::DIRECT;
        else if( style == wxT( "Relief" ) )    polygonconnectStyle = ALTIUM_CONNECT_STYLE::RELIEF;
        else if( style == wxT( "NoConnect" ) ) polygonconnectStyle = ALTIUM_CONNECT_STYLE::NONE;
        else                                   polygonconnectStyle = ALTIUM_CONNECT_STYLE::UNKNOWN;
    }
    else
    {
        kind = ALTIUM_RULE_KIND::UNKNOWN;
    }

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Rules6 stream was not parsed correctly" ) );
}

AARC6::AARC6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );
    if( recordtype != ALTIUM_RECORD::ARC )
    {
        THROW_IO_ERROR( wxT( "Arcs6 stream has invalid recordtype" ) );
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
    polygon      = aReader.Read<uint16_t>();
    component    = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    center     = aReader.ReadVector2IPos();
    radius     = aReader.ReadKicadUnit();
    startangle = aReader.Read<double>();
    endangle   = aReader.Read<double>();
    width      = aReader.ReadKicadUnit();
    subpolyindex = aReader.Read<uint16_t>();

    if( aReader.GetRemainingSubrecordBytes() >= 10 )
    {
        aReader.Skip( 9 );
        keepoutrestrictions = aReader.Read<uint8_t>();
    }
    else
    {
        keepoutrestrictions = is_keepout ? 0x1F : 0;
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
    {
        THROW_IO_ERROR( wxT( "Arcs6 stream was not parsed correctly" ) );
    }
}

ACOMPONENTBODY6::ACOMPONENTBODY6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::MODEL )
        THROW_IO_ERROR( wxT( "ComponentsBodies6 stream has invalid recordtype" ) );

    aReader.ReadAndSetSubrecordLength();

    aReader.Skip( 7 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 9 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "ComponentsBodies6 stream has no properties" ) );

    modelName       = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "MODEL.NAME" ), wxT( "" ) );
    modelId         = ALTIUM_PROPS_UTILS::ReadString( properties, wxT( "MODELID" ), wxT( "" ) );
    modelIsEmbedded = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "MODEL.EMBED" ), false );

    modelPosition.x = ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "MODEL.2D.X" ), wxT( "0mil" ) );
    modelPosition.y = -ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "MODEL.2D.Y" ), wxT( "0mil" ) );
    modelPosition.z = ALTIUM_PROPS_UTILS::ReadKicadUnit( properties, wxT( "MODEL.3D.DZ" ), wxT( "0mil" ) );

    modelRotation.x = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "MODEL.3D.ROTX" ), 0. );
    modelRotation.y = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "MODEL.3D.ROTY" ), 0. );
    modelRotation.z = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "MODEL.3D.ROTZ" ), 0. );

    rotation = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "MODEL.2D.ROTATION" ), 0. );

    body_opacity_3d = ALTIUM_PROPS_UTILS::ReadDouble( properties, wxT( "BODYOPACITY3D" ), 1. );
    body_projection = ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "BODYPROJECTION" ), 0 );

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Components6 stream was not parsed correctly" ) );
}

APAD6::APAD6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::PAD )
        THROW_IO_ERROR( wxT( "Pads6 stream has invalid recordtype" ) );

    // Subrecord 1
    size_t subrecord1 = aReader.ReadAndSetSubrecordLength();

    if( subrecord1 == 0 )
        THROW_IO_ERROR( wxT( "Pads6 stream has no subrecord1 data" ) );

    name = aReader.ReadWxString();

    if( aReader.GetRemainingSubrecordBytes() != 0 )
        THROW_IO_ERROR( wxT( "Pads6 stream has invalid subrecord1 length" ) );

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

    ExpectSubrecordLengthAtLeast( "Pads6", "subrecord5", 110, subrecord5 );

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
    aReader.Skip( 4 ); // to 13

    position = aReader.ReadVector2IPos();
    topsize  = aReader.ReadVector2ISize();
    midsize  = aReader.ReadVector2ISize();
    botsize  = aReader.ReadVector2ISize();
    holesize = aReader.ReadKicadUnit(); // to 49

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
    pastemaskexpansionmode = static_cast<ALTIUM_MODE>( aReader.Read<uint8_t>() );
    soldermaskexpansionmode = static_cast<ALTIUM_MODE>( aReader.Read<uint8_t>() );
    aReader.Skip( 3 ); // to 106

    pad_to_die_length = 0;
    pad_to_die_delay = 0;

    if( subrecord5 == 110 )
    {
        // Don't know exactly what this is, but it's always been 0 in the files with
        // 110-byte subrecord5.
        // e.g. https://gitlab.com/kicad/code/kicad/-/issues/16514
        const uint32_t unknown = aReader.ReadKicadUnit(); // to 110

        if( unknown != 0 )
        {
            THROW_IO_ERROR( wxString::Format( "Pads6 stream subrecord5 + 106 has value %d, "
                                              "which is unexpected",
                                              unknown ) );
        }
        holerotation = 0;
    }
    else
    {
        // More than 110, need at least 114
        ExpectSubrecordLengthAtLeast( "Pads6", "subrecord5", 114, subrecord5 );
        holerotation = aReader.Read<double>(); // to 114
    }

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

    if( subrecord5 >= 202 )
    {
        aReader.Skip( 40 );
        pad_to_die_length = aReader.ReadKicadUnit();
        aReader.Skip( 32 );
        pad_to_die_delay = KiROUND( aReader.Read<double>() * 1e18 );
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

        for( VECTOR2I& pt : sizeAndShape->holeoffset )
            pt.x = aReader.ReadKicadUnitX();

        for( VECTOR2I& pt : sizeAndShape->holeoffset )
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
        THROW_IO_ERROR( wxT( "Pads6 stream was not parsed correctly" ) );
}

AVIA6::AVIA6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::VIA )
        THROW_IO_ERROR( wxT( "Vias6 stream has invalid recordtype" ) );

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
    position = aReader.ReadVector2IPos();
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
        uint8_t temp_byte = aReader.Read<uint8_t>(); // Unknown.

        thermal_relief_airgap = aReader.ReadKicadUnit();
        thermal_relief_conductorcount = aReader.Read<uint8_t>();
        aReader.Skip( 1 ); // Unknown.

        thermal_relief_conductorwidth = aReader.ReadKicadUnit();

        aReader.ReadKicadUnit(); // Unknown.  20mil?
        aReader.ReadKicadUnit(); // Unknown.  20mil?

        aReader.Skip( 4 );
        soldermask_expansion_front = aReader.ReadKicadUnit();
        aReader.Skip( 8 );

        temp_byte = aReader.Read<uint8_t>();
        soldermask_expansion_manual = temp_byte & 0x02;

        aReader.Skip( 7 );

        viamode = static_cast<ALTIUM_PAD_MODE>( aReader.Read<uint8_t>() );

        for( int ii = 0; ii < 32; ++ii )
        {
            diameter_by_layer[ii] = aReader.ReadKicadUnit();
        }
    }

    if( subrecord1 >= 246 )
    {
        aReader.Skip( 38 );
        soldermask_expansion_linked = aReader.Read<uint8_t>() & 0x01;
        soldermask_expansion_back = aReader.ReadKicadUnit();
    }

    if( subrecord1 >= 307 )
    {
        aReader.Skip( 45 );

        pos_tolerance = aReader.ReadKicadUnit();
        neg_tolerance = aReader.ReadKicadUnit();
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Vias6 stream was not parsed correctly" ) );
}

ATRACK6::ATRACK6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::TRACK )
        THROW_IO_ERROR( wxT( "Tracks6 stream has invalid recordtype" ) );

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1    = aReader.Read<uint8_t>();
    is_locked         = ( flags1 & 0x04 ) == 0;
    is_polygonoutline = ( flags1 & 0x02 ) != 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net          = aReader.Read<uint16_t>();
    polygon      = aReader.Read<uint16_t>();
    component    = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    start = aReader.ReadVector2IPos();
    end   = aReader.ReadVector2IPos();
    width = aReader.ReadKicadUnit();
    subpolyindex = aReader.Read<uint16_t>();

    if( aReader.GetRemainingSubrecordBytes() >= 11 )
    {
        aReader.Skip( 10 );
        keepoutrestrictions = aReader.Read<uint8_t>();
    }
    else
    {
        keepoutrestrictions = is_keepout ? 0x1F : 0;
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Tracks6 stream was not parsed correctly" ) );
}

ATEXT6::ATEXT6( ALTIUM_BINARY_PARSER& aReader, std::map<uint32_t, wxString>& aStringTable )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::TEXT )
        THROW_IO_ERROR( wxT( "Texts6 stream has invalid recordtype" ) );

    // Subrecord 1 - Properties
    size_t subrecord1 = aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );
    aReader.Skip( 6 );
    component = aReader.Read<uint16_t>();
    aReader.Skip( 4 );
    position = aReader.ReadVector2IPos();
    height   = aReader.ReadKicadUnit();
    strokefonttype = static_cast<STROKE_FONT_TYPE>( aReader.Read<uint16_t>() );
    // TODO: The Serif font type doesn't match well with KiCad, we should replace it with a better match

    rotation     = aReader.Read<double>();
    isMirrored   = aReader.Read<uint8_t>() != 0;
    strokewidth  = aReader.ReadKicadUnit();

    if( subrecord1 < 123 )
    {
        fonttype = ALTIUM_TEXT_TYPE::STROKE;
        aReader.SkipSubrecord();
        return;
    }

    isComment    = aReader.Read<uint8_t>() != 0;
    isDesignator = aReader.Read<uint8_t>() != 0;
    aReader.Skip( 1 );
    fonttype = static_cast<ALTIUM_TEXT_TYPE>( aReader.Read<uint8_t>() );
    isBold   = aReader.Read<uint8_t>() != 0;
    isItalic = aReader.Read<uint8_t>() != 0;

    char fontData[64] = { 0 };
    aReader.ReadBytes( fontData, sizeof( fontData ) );
    fontname = wxString( fontData, wxMBConvUTF16LE(), sizeof( fontData ) ).BeforeFirst( '\0' );

    char tmpbyte = aReader.Read<uint8_t>();
    isInverted = !!tmpbyte;
    margin_border_width = aReader.ReadKicadUnit(); // "Margin Border"
    widestring_index = aReader.Read<uint32_t>();
    aReader.Skip( 4 );

    // An inverted rect in Altium is like a text box with the text inverted.
    isInvertedRect = aReader.Read<uint8_t>() != 0;

    textbox_rect_width = aReader.ReadKicadUnit();
    textbox_rect_height = aReader.ReadKicadUnit();
    textbox_rect_justification = static_cast<ALTIUM_TEXT_POSITION>( aReader.Read<uint8_t>() );
    text_offset_width = aReader.ReadKicadUnit(); // "Text Offset"

    int remaining = aReader.GetRemainingSubrecordBytes();

    if( remaining >= 103 )
    {
        VECTOR2I unk_vec = aReader.ReadVector2ISize();
        wxLogTrace( traceAltiumImport, " Unk vec: %d, %d\n", unk_vec.x, unk_vec.y );

        barcode_margin = aReader.ReadVector2ISize();

        int32_t unk32 = aReader.ReadKicadUnit();
        wxLogTrace( traceAltiumImport, " Unk32: %d\n", unk32 );

        barcode_type = static_cast<ALTIUM_BARCODE_TYPE>( aReader.Read<uint8_t>() );
        uint8_t unk8 = aReader.Read<uint8_t>();
        wxLogTrace( traceAltiumImport, " Unk8: %u\n", unk8 );

        barcode_inverted = aReader.Read<uint8_t>() != 0;
        fonttype = static_cast<ALTIUM_TEXT_TYPE>( aReader.Read<uint8_t>() );

        aReader.ReadBytes( fontData, sizeof( fontData ) );
        barcode_fontname = wxString( fontData, wxMBConvUTF16LE(), sizeof( fontData ) ).BeforeFirst( '\0' );

        for( size_t ii = 0; ii < 5; ++ii )
        {
            uint8_t temp = aReader.Peek<uint8_t>();
            uint32_t temp32 = ii < 1 ? ALTIUM_PROPS_UTILS::ConvertToKicadUnit( aReader.Peek<uint32_t>() ) : 0;
            wxLogTrace( traceAltiumImport, "2ATEXT6 %zu:\t Byte:%u, Kicad:%u\n", ii, temp, temp32 );
            aReader.Skip( 1 );
        }

        // "Frame" text type flag
        isFrame = aReader.Read<uint8_t>() != 0;

        // Use "Offset" border value instead of "Margin"
        isOffsetBorder = aReader.Read<uint8_t>() != 0;

        for( size_t ii = 0; ii < 8; ++ii )
        {
            uint8_t temp = aReader.Peek<uint8_t>();
            uint32_t temp32 = ii < 3 ? ALTIUM_PROPS_UTILS::ConvertToKicadUnit( aReader.Peek<uint32_t>() ) : 0;
            wxLogTrace( traceAltiumImport, "3ATEXT6 %zu:\t Byte:%u, Kicad:%u\n", ii, temp, temp32 );
            aReader.Skip( 1 );
        }
    }
    else
    {
        isFrame = textbox_rect_height != 0 && textbox_rect_width != 0;
        isOffsetBorder = false;
    }

    if( remaining >= 115 )
    {
        // textbox_rect_justification will be wrong (5) when this flag is unset,
        // in that case, we should always use the left bottom justification.
        isJustificationValid = aReader.Read<uint8_t>() != 0;
    }
    else
    {
        isJustificationValid = false;
    }

    aReader.SkipSubrecord();

    // Subrecord 2 - Legacy 8bit string, max 255 chars, unknown codepage
    aReader.ReadAndSetSubrecordLength();

    auto entry = aStringTable.find( widestring_index );

    if( entry != aStringTable.end() )
        text = entry->second;
    else
        text = aReader.ReadWxString();

    // Normalize Windows line endings
    text.Replace( wxT( "\r\n" ), wxT( "\n" ) );

    aReader.SkipSubrecord();

    // Altium only supports inverting truetype fonts
    if( fonttype != ALTIUM_TEXT_TYPE::STROKE )
    {
        isInverted = false;
        isInvertedRect = false;
    }

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Texts6 stream was not parsed correctly" ) );
}

AFILL6::AFILL6( ALTIUM_BINARY_PARSER& aReader )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::FILL )
        THROW_IO_ERROR( wxT( "Fills6 stream has invalid recordtype" ) );

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
    pos1     = aReader.ReadVector2IPos();
    pos2     = aReader.ReadVector2IPos();
    rotation = aReader.Read<double>();

    if( aReader.GetRemainingSubrecordBytes() >= 10 )
    {
        aReader.Skip( 9 );
        keepoutrestrictions = aReader.Read<uint8_t>();
    }
    else
    {
        keepoutrestrictions = is_keepout ? 0x1F : 0;
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Fills6 stream was not parsed correctly" ) );
}

AREGION6::AREGION6( ALTIUM_BINARY_PARSER& aReader, bool aExtendedVertices )
{
    ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( aReader.Read<uint8_t>() );

    if( recordtype != ALTIUM_RECORD::REGION )
        THROW_IO_ERROR( wxT( "Regions6 stream has invalid recordtype" ) );

    // Subrecord 1
    aReader.ReadAndSetSubrecordLength();

    layer = static_cast<ALTIUM_LAYER>( aReader.Read<uint8_t>() );

    uint8_t flags1 = aReader.Read<uint8_t>();
    is_locked      = ( flags1 & 0x04 ) == 0;
    is_teardrop    = ( flags1 & 0x10 ) != 0;

    uint8_t flags2 = aReader.Read<uint8_t>();
    is_keepout     = flags2 == 2;

    net = aReader.Read<uint16_t>();
    polygon = aReader.Read<uint16_t>();
    component = aReader.Read<uint16_t>();
    aReader.Skip( 5 );
    holecount = aReader.Read<uint16_t>();
    aReader.Skip( 2 );

    std::map<wxString, wxString> properties = aReader.ReadProperties();

    if( properties.empty() )
        THROW_IO_ERROR( wxT( "Regions6 stream has empty properties" ) );

    int  pkind     = ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "KIND" ), 0 );
    bool is_cutout = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "ISBOARDCUTOUT" ), false );

    is_shapebased = ALTIUM_PROPS_UTILS::ReadBool( properties, wxT( "ISSHAPEBASED" ), false );
    keepoutrestrictions = static_cast<uint8_t>(
            ALTIUM_PROPS_UTILS::ReadInt( properties, wxT( "KEEPOUTRESTRIC" ), 0x1F ) );

    // TODO: this can differ from the other subpolyindex?!
    // Note: "the other subpolyindex" is "polygon"
    subpolyindex = static_cast<uint16_t>(
            ALTIUM_PROPS_UTILS::ReadInt( properties, "SUBPOLYINDEX", ALTIUM_POLYGON_NONE ) );

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
        kind = ALTIUM_REGION_KIND::DASHED_OUTLINE;
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

    if( aExtendedVertices )
        num_outline_vertices++; // Has a closing vertex

    for( uint32_t i = 0; i < num_outline_vertices; i++ )
    {
        if( aExtendedVertices )
        {
            bool     isRound  = aReader.Read<uint8_t>() != 0;
            VECTOR2I position = aReader.ReadVector2IPos();
            VECTOR2I center   = aReader.ReadVector2IPos();
            int32_t  radius   = aReader.ReadKicadUnit();
            double   angle1   = aReader.Read<double>();
            double   angle2   = aReader.Read<double>();
            outline.emplace_back( isRound, radius, angle1, angle2, position, center );
        }
        else
        {
            // For some regions the coordinates are stored as double and not as int32_t
            int32_t x = ALTIUM_PROPS_UTILS::ConvertToKicadUnit( aReader.Read<double>() );
            int32_t y = ALTIUM_PROPS_UTILS::ConvertToKicadUnit( -aReader.Read<double>() );
            outline.emplace_back( VECTOR2I( x, y ) );
        }
    }

    holes.resize( holecount );
    for( uint16_t k = 0; k < holecount; k++ )
    {
        uint32_t num_hole_vertices = aReader.Read<uint32_t>();
        holes.at( k ).reserve( num_hole_vertices );

        for( uint32_t i = 0; i < num_hole_vertices; i++ )
        {
            int32_t x = ALTIUM_PROPS_UTILS::ConvertToKicadUnit( aReader.Read<double>() );
            int32_t y = ALTIUM_PROPS_UTILS::ConvertToKicadUnit( -aReader.Read<double>() );
            holes.at( k ).emplace_back( VECTOR2I( x, y ) );
        }
    }

    aReader.SkipSubrecord();

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( wxT( "Regions6 stream was not parsed correctly" ) );
}
