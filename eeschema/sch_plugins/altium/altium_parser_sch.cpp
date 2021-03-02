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


int PropertiesReadKiCadUnitFrac1(
        const std::map<wxString, wxString>& aProperties, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    // Dunno why Altium invents different units for the same purpose
    int key     = ALTIUM_PARSER::PropertiesReadInt( aProperties, aKey, 0 );
    int keyFrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, aKey + "_FRAC1", 0 );
    return Altium2KiCadUnit( key * 10, keyFrac );
}


template <typename T>
T PropertiesReadEnum( const std::map<wxString, wxString>& aProperties, const wxString& aKey,
        int aLower, int aUpper, T aDefault )
{
    int value = ALTIUM_PARSER::PropertiesReadInt( aProperties, aKey, static_cast<int>( aDefault ) );
    if( value < aLower || value > aUpper )
        return aDefault;
    else
        return static_cast<T>( value );
}


ASCH_COMPONENT::ASCH_COMPONENT( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::COMPONENT );

    currentpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "CURRENTPARTID", ALTIUM_COMPONENT_NONE );
    libreference = ALTIUM_PARSER::PropertiesReadString( aProperties, "LIBREFERENCE", "" );
    sourcelibraryname = ALTIUM_PARSER::PropertiesReadString( aProperties, "SOURCELIBRARYNAME", "" );
    componentdescription =
            ALTIUM_PARSER::PropertiesReadString( aProperties, "COMPONENTDESCRIPTION", "" );

    orientation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );
    isMirrored  = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISMIRRORED", false );
    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    partcount        = ALTIUM_PARSER::PropertiesReadInt( aProperties, "PARTCOUNT", 0 );
    displaymodecount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "DISPLAYMODECOUNT", 0 );
    displaymode      = ALTIUM_PARSER::PropertiesReadInt( aProperties, "DISPLAYMODE", 0 );
}


ASCH_PIN::ASCH_PIN( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::PIN );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
                              ASCH_PIN_SYMBOL_OUTEREDGE::NO_SYMBOL;

    int symbolInnerEdgeInt = ALTIUM_PARSER::PropertiesReadInt( aProperties, "SYMBOL_INNEREDGE", 0 );
    symbolInnerEdge        = ( symbolInnerEdgeInt == 0 || symbolInnerEdgeInt == 3 ) ?
                              static_cast<ASCH_PIN_SYMBOL_INNEREDGE>( symbolInnerEdgeInt ) :
                              ASCH_PIN_SYMBOL_INNEREDGE::NO_SYMBOL;
    electrical = PropertiesReadEnum<ASCH_PIN_ELECTRICAL>(
            aProperties, "ELECTRICAL", 0, 7, ASCH_PIN_ELECTRICAL::INPUT );

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


ASCH_LABEL::ASCH_LABEL( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::LABEL );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    fontId     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "FONTID", 0 );
    isMirrored = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISMIRRORED", false );

    justification = PropertiesReadEnum<ASCH_LABEL_JUSTIFICATION>(
            aProperties, "JUSTIFICATION", 0, 8, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );
}


ASCH_BEZIER::ASCH_BEZIER( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::BEZIER );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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
    ownerpartdisplaymode =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTDISPLAYMODE", 0 );

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


ASCH_SHEET_SYMBOL::ASCH_SHEET_SYMBOL( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::SHEET_SYMBOL );


    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    size     = wxSize( PropertiesReadKiCadUnitFrac( aProperties, "XSIZE" ),
            PropertiesReadKiCadUnitFrac( aProperties, "YSIZE" ) );

    isSolid = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISSOLID", false );

    color     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::PropertiesReadInt( aProperties, "AREACOLOR", 0 );
}


ASCH_SHEET_ENTRY::ASCH_SHEET_ENTRY( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::SHEET_ENTRY );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    // some magic, because it stores those infos in a different unit??
    distanceFromTop = PropertiesReadKiCadUnitFrac1( aProperties, "DISTANCEFROMTOP" );

    side = PropertiesReadEnum<ASCH_SHEET_ENTRY_SIDE>(
            aProperties, "SIDE", 0, 3, ASCH_SHEET_ENTRY_SIDE::LEFT );

    name = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );

    iotype = PropertiesReadEnum<ASCH_PORT_IOTYPE>(
            aProperties, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    style = PropertiesReadEnum<ASCH_PORT_STYLE>(
            aProperties, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );
}


ASCH_POWER_PORT::ASCH_POWER_PORT( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::POWER_PORT );

    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    location    = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    text        = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );
    showNetName = ALTIUM_PARSER::PropertiesReadBool( aProperties, "SHOWNETNAME", true );

    style = PropertiesReadEnum<ASCH_POWER_PORT_STYLE>(
            aProperties, "STYLE", 0, 10, ASCH_POWER_PORT_STYLE::CIRCLE );
}

ASCH_PORT::ASCH_PORT( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::PORT );

    ownerpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    name = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );

    width  = PropertiesReadKiCadUnitFrac( aProperties, "WIDTH" );
    height = PropertiesReadKiCadUnitFrac( aProperties, "HEIGHT" );

    iotype = PropertiesReadEnum<ASCH_PORT_IOTYPE>(
            aProperties, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    style = PropertiesReadEnum<ASCH_PORT_STYLE>(
            aProperties, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );
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

    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );
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

ASCH_SHEET_FONT::ASCH_SHEET_FONT( const std::map<wxString, wxString>& aProperties, int aId )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::SHEET );

    const wxString sid = std::to_string( aId );

    fontname = ALTIUM_PARSER::PropertiesReadString( aProperties, "FONTNAME" + sid, "" );

    size     = PropertiesReadKiCadUnitFrac( aProperties, "SIZE" + sid );
    rotation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ROTATION" + sid, 0 );

    italic    = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ITALIC" + sid, false );
    bold      = ALTIUM_PARSER::PropertiesReadBool( aProperties, "BOLD" + sid, false );
    underline = ALTIUM_PARSER::PropertiesReadBool( aProperties, "UNDERLINE" + sid, false );
}

wxPoint ASchSheetGetSize( ASCH_SHEET_SIZE aSheetSize )
{
    // From: https://github.com/vadmium/python-altium/blob/master/format.md#sheet
    switch( aSheetSize )
    {
    default:
    case ASCH_SHEET_SIZE::A4:
        return { 1150, 760 };
    case ASCH_SHEET_SIZE::A3:
        return { 1550, 1110 };
    case ASCH_SHEET_SIZE::A2:
        return { 2230, 1570 };
    case ASCH_SHEET_SIZE::A1:
        return { 3150, 2230 };
    case ASCH_SHEET_SIZE::A0:
        return { 4460, 3150 };
    case ASCH_SHEET_SIZE::A:
        return { 950, 750 };
    case ASCH_SHEET_SIZE::B:
        return { 1500, 950 };
    case ASCH_SHEET_SIZE::C:
        return { 2000, 1500 };
    case ASCH_SHEET_SIZE::D:
        return { 3200, 2000 };
    case ASCH_SHEET_SIZE::E:
        return { 4200, 3200 };
    case ASCH_SHEET_SIZE::LETTER:
        return { 1100, 850 };
    case ASCH_SHEET_SIZE::LEGAL:
        return { 1400, 850 };
    case ASCH_SHEET_SIZE::TABLOID:
        return { 1700, 1100 };
    case ASCH_SHEET_SIZE::ORCAD_A:
        return { 990, 790 };
    case ASCH_SHEET_SIZE::ORCAD_B:
        return { 1540, 990 };
    case ASCH_SHEET_SIZE::ORCAD_C:
        return { 2060, 1560 };
    case ASCH_SHEET_SIZE::ORCAD_D:
        return { 3260, 2060 };
    case ASCH_SHEET_SIZE::ORCAD_E:
        return { 4280, 3280 };
    }
}


ASCH_SHEET::ASCH_SHEET( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::SHEET );

    int fontidcount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "FONTIDCOUNT", 0 );
    for( int i = 1; i <= fontidcount; i++ )
    {
        fonts.emplace_back( aProperties, i );
    }

    sheetSize = PropertiesReadEnum<ASCH_SHEET_SIZE>(
            aProperties, "SHEETSTYLE", 0, 17, ASCH_SHEET_SIZE::A4 );
    sheetOrientation = PropertiesReadEnum<ASCH_SHEET_WORKSPACEORIENTATION>(
            aProperties, "WORKSPACEORIENTATION", 0, 1, ASCH_SHEET_WORKSPACEORIENTATION::LANDSCAPE );
}


ASCH_SHEET_NAME::ASCH_SHEET_NAME( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::SHEET_NAME );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    isHidden = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISHIDDEN", false );
}


ASCH_FILE_NAME::ASCH_FILE_NAME( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::FILE_NAME );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    isHidden = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISHIDDEN", false );
}


ASCH_DESIGNATOR::ASCH_DESIGNATOR( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::DESIGNATOR );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    name = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );
    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
}


ASCH_BUS_ENTRY::ASCH_BUS_ENTRY( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::BUS_ENTRY );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );
    corner   = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "CORNER.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "CORNER.Y" ) );
}


ASCH_PARAMETER::ASCH_PARAMETER( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::PARAMETER );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    location = wxPoint( PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.X" ),
            -PropertiesReadKiCadUnitFrac( aProperties, "LOCATION.Y" ) );

    orientation = PropertiesReadEnum<ASCH_RECORD_ORIENTATION>(
            aProperties, "ORIENTATION", 0, 3, ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    name = ALTIUM_PARSER::PropertiesReadString( aProperties, "NAME", "" );
    text = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );

    isHidden   = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISHIDDEN", false );
    isMirrored = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISMIRRORED", false );
    isShowName = ALTIUM_PARSER::PropertiesReadBool( aProperties, "SHOWNAME", false );
}