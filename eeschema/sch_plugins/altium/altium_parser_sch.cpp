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

#include <iostream>
#include <unordered_map>

#include <convert_to_biu.h>
#include <ki_exception.h>

#include "plugins/altium/altium_parser.h"
#include "sch_plugins/altium/altium_parser_sch.h"


ALTIUM_SCH_RECORD PropertiesReadRecord( const std::map<wxString, wxString>& aProperties )
{
    int recordId = ALTIUM_PARSER::PropertiesReadInt( aProperties, "RECORD", 0 );
    return static_cast<ALTIUM_SCH_RECORD>( recordId );
}


constexpr int Altium2KiCadUnit( const int val, const int frac )
{
    return Mils2iu( val ) * 10 + Mils2iu( frac ) / 10000; // TODO: correct, rounding issues?
}


int PropertiesReadKiCadUnitFrac(
        const std::map<wxString, wxString>& aProperties, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    int key     = ALTIUM_PARSER::PropertiesReadInt( aProperties, aKey, 0 );
    int keyFrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, aKey + "_FRAC", 0 );
    return Altium2KiCadUnit( key, keyFrac );
}


ASCH_COMPONENT::ASCH_COMPONENT( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::COMPONENT );

    currentpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "CURRENTPARTID", ALTIUM_COMPONENT_NONE );
    libreference = ALTIUM_PARSER::PropertiesReadString( aProperties, "LIBREFERENCE", "" );

    orientation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
}


ASCH_PIN::ASCH_PIN( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::PIN );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    name       = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );
    text       = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );
    designator = ALTIUM_PARSER::PropertiesReadString( aProperties, "DESIGNATOR", "" );

    int symbolOuterInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SYMBOL_OUTER", 0 );
    symbolOuter        = static_cast<ASCH_PIN_SYMBOL_OUTER>( symbolOuterInt );

    int symbolInnerInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SYMBOL_INNER", 0 );
    symbolInner        = static_cast<ASCH_PIN_SYMBOL_INNER>( symbolInnerInt );

    int symbolOuterEdgeInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SYMBOL_OUTEREDGE", 0 );
    symbolOuterEdge        = ( symbolOuterEdgeInt == 0 || symbolOuterEdgeInt == 1
                              || symbolOuterEdgeInt == 4 || symbolOuterEdgeInt == 17 ) ?
                              static_cast<ASCH_PIN_SYMBOL_OUTEREDGE>( symbolOuterEdgeInt ) :
                              ASCH_PIN_SYMBOL_OUTEREDGE::UNKNOWN;

    int symbolInnerEdgeInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SYMBOL_INNEREDGE", 0 );
    symbolInnerEdge        = ( symbolInnerEdgeInt == 0 || symbolInnerEdgeInt == 3 ) ?
                              static_cast<ASCH_PIN_SYMBOL_INNEREDGE>( symbolInnerEdgeInt ) :
                              ASCH_PIN_SYMBOL_INNEREDGE::UNKNOWN;

    int electricalInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ELECTRICAL", 0 );
    electrical        = ( electricalInt >= 0 && electricalInt <= 7 ) ?
                         static_cast<ASCH_PIN_ELECTRICAL>( electricalInt ) :
                         ASCH_PIN_ELECTRICAL::UNKNOWN;

    int pinconglomerate = ALTIUM_PARSER::PropertiesReadInt( aProperties, "PINCONGLOMERATE", 0 );

    orientation    = static_cast<ASCH_RECORD_ORIENTATION>( pinconglomerate & 0x03 );
    showPinName    = ( pinconglomerate & 0x08 ) != 0;
    showDesignator = ( pinconglomerate & 0x10 ) != 0;

    int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );

    int p     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "PINLENGTH", 0 );
    int pfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "PINLENGTH_FRAC", 0 );
    pinlength = Altium2KiCadUnit( p, pfrac );

    // this code calculates the location as required by KiCad without rounding error attached
    int kicadX     = x;
    int kicadXfrac = xfrac;
    int kicadY     = y;
    int kicadYfrac = yfrac;

    switch( orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        kicadX += p;
        kicadXfrac += pfrac;
        break;
    case ASCH_RECORD_ORIENTATION::UPWARDS:
        kicadY += p;
        kicadYfrac += pfrac;
        break;
    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        kicadX -= p;
        kicadXfrac -= pfrac;
        break;
    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        kicadY -= p;
        kicadYfrac -= pfrac;
        break;
    default:
        wxLogWarning( "Pin has unexpected orientation" );
        break;
    }
    kicadLocation = wxPoint(
            Altium2KiCadUnit( kicadX, kicadXfrac ), -Altium2KiCadUnit( kicadY, kicadYfrac ) );
}


ASCH_BEZIER::ASCH_BEZIER( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::BEZIER );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    int locationCount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( PropertiesReadKiCadUnitFrac( aProperties, "X" + si ),
                -PropertiesReadKiCadUnitFrac( aProperties, "Y" + si ) );
    }

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
}


ASCH_POLYLINE::ASCH_POLYLINE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::POLYLINE );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    int locationCount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( PropertiesReadKiCadUnitFrac( aProperties, "X" + si ),
                -PropertiesReadKiCadUnitFrac( aProperties, "Y" + si ) );
    }

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );

    int linestyleVar = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LINESTYLEEXT", 0 );
    linestyleVar     = ALTIUM_PARSER::PropertiesReadInt(
            aProperties, "LINESTYLE", linestyleVar ); // overwrite if present
    linestyle = linestyleVar >= 0 && linestyleVar <= 3 ?
                        static_cast<ASCH_POLYLINE_LINESTYLE>( linestyleVar ) :
                        ASCH_POLYLINE_LINESTYLE::SOLID;
}


ASCH_POLYGON::ASCH_POLYGON( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::POLYGON );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    int locationCount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( PropertiesReadKiCadUnitFrac( aProperties, "X" + si ),
                -PropertiesReadKiCadUnitFrac( aProperties, "Y" + si ) );
    }

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
    isSolid   = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISSOLID", false );

    color     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::PropertiesReadInt( aProperties, "AREACOLOR", 0 );
}


ASCH_ROUND_RECTANGLE::ASCH_ROUND_RECTANGLE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::ROUND_RECTANGLE );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    bottomLeft = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    topRight   = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "CORNER.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "CORNER.Y" ) );

    topRight = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "CORNERXRADIUS" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "CORNERYRADIUS" ) );

    lineWidth     = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
    isSolid       = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISSOLID", false );
    isTransparent = ALTIUM_PARSER::PropertiesReadBool( aProperties, "TRANSPARENT", false );

    color     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::PropertiesReadInt( aProperties, "AREACOLOR", 0 );
}


ASCH_ARC::ASCH_ARC( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::ARC );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    center = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    radius = PropertiesReadKiCadUnitFrac( aProperties, "RADIUS" );

    startAngle = ALTIUM_PARSER::PropertiesReadDouble( aProperties, "STARTANGLE", 0 );
    endAngle   = ALTIUM_PARSER::PropertiesReadDouble( aProperties, "ENDANGLE", 0 );

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
}


ASCH_LINE::ASCH_LINE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::LINE );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    point1 = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    point2 = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "CORNER.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "CORNER.Y" ) );

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
}


ASCH_RECTANGLE::ASCH_RECTANGLE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::RECTANGLE );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    bottomLeft = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    topRight   = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "CORNER.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "CORNER.Y" ) );

    lineWidth     = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
    isSolid       = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISSOLID", false );
    isTransparent = ALTIUM_PARSER::PropertiesReadBool( aProperties, "TRANSPARENT", false );

    color     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::PropertiesReadInt( aProperties, "AREACOLOR", 0 );
}


ASCH_NO_ERC::ASCH_NO_ERC( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::NO_ERC );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    isActive   = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISACTIVE", true );
    supressAll = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SUPPRESSALL", true );
}


ASCH_NET_LABEL::ASCH_NET_LABEL( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::NET_LABEL );

    text        = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    int orientationRaw = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );
    orientation        = orientationRaw >= 0 && orientationRaw <= 3 ?
                          static_cast<ASCH_RECORD_ORIENTATION>( orientationRaw ) :
                          ASCH_RECORD_ORIENTATION::RIGHTWARDS;
}


ASCH_BUS::ASCH_BUS( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::BUS );

    indexinsheet = ALTIUM_PARSER::PropertiesReadInt( aProperties, "INDEXINSHEET", 0 );

    int locationcount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( PropertiesReadKiCadUnitFrac( aProperties, "X" + si ),
                -PropertiesReadKiCadUnitFrac( aProperties, "Y" + si ) );
    }

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
}


ASCH_WIRE::ASCH_WIRE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::WIRE );

    /*std::cout << "-----------------------------------" << std::endl;
    // debug
    for( auto& property : aProperties )
    {
        std::cout << "  * '" << property.first << "' = '" << property.second << "'"
                  << std::endl;
    }*/

    indexinsheet = ALTIUM_PARSER::PropertiesReadInt( aProperties, "INDEXINSHEET", 0 );

    int locationcount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( PropertiesReadKiCadUnitFrac( aProperties, "X" + si ),
                -PropertiesReadKiCadUnitFrac( aProperties, "Y" + si ) );
    }

    lineWidth = PropertiesReadKiCadUnitFrac( aProperties, "LINEWIDTH" );
}


ASCH_JUNCTION::ASCH_JUNCTION( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::JUNCTION );

    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", -1 );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
}


ASCH_DESIGNATOR::ASCH_DESIGNATOR( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::DESIGNATOR );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    name = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );
    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    orientation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
}