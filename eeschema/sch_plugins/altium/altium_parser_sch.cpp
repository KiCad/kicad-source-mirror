/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/log.h>

#include "plugins/altium/altium_parser.h"
#include "sch_plugins/altium/altium_parser_sch.h"


ALTIUM_SCH_RECORD ReadRecord( const std::map<wxString, wxString>& aProps )
{
    int recordId = ALTIUM_PARSER::ReadInt( aProps, "RECORD", 0 );
    return static_cast<ALTIUM_SCH_RECORD>( recordId );
}


constexpr int Altium2KiCadUnit( const int val, const int frac )
{
    return Mils2iu( val ) * 10 + Mils2iu( frac ) / 10000; // TODO: correct, rounding issues?
}


int ReadKiCadUnitFrac( const std::map<wxString, wxString>& aProps, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    int key     = ALTIUM_PARSER::ReadInt( aProps, aKey, 0 );
    int keyFrac = ALTIUM_PARSER::ReadInt( aProps, aKey + "_FRAC", 0 );
    return Altium2KiCadUnit( key, keyFrac );
}


int ReadKiCadUnitFrac1( const std::map<wxString, wxString>& aProps, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    // Dunno why Altium invents different units for the same purpose
    int key     = ALTIUM_PARSER::ReadInt( aProps, aKey, 0 );
    int keyFrac = ALTIUM_PARSER::ReadInt( aProps, aKey + "_FRAC1", 0 );
    return Altium2KiCadUnit( key * 10, keyFrac );
}


int ReadOwnerIndex( const std::map<wxString, wxString>& aProperties )
{
    return ALTIUM_PARSER::ReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
}


int ReadOwnerPartId( const std::map<wxString, wxString>& aProperties )
{
    return ALTIUM_PARSER::ReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );
}


template <typename T>
T ReadEnum( const std::map<wxString, wxString>& aProps, const wxString& aKey, int aLower,
            int aUpper, T aDefault )
{
    int value = ALTIUM_PARSER::ReadInt( aProps, aKey, static_cast<int>( aDefault ));

    if( value < aLower || value > aUpper )
        return aDefault;
    else
        return static_cast<T>( value );
}


ASCH_STORAGE_FILE::ASCH_STORAGE_FILE( ALTIUM_PARSER& aReader )
{
    aReader.Skip( 5 );
    filename = aReader.ReadWxString();
    uint32_t dataSize = aReader.Read<uint32_t>();
    data = aReader.ReadVector( dataSize );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( "Storage stream was not parsed correctly" );
}


ASCH_SYMBOL::ASCH_SYMBOL( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::COMPONENT );

    currentpartid = ALTIUM_PARSER::ReadInt( aProps, "CURRENTPARTID", ALTIUM_COMPONENT_NONE );
    libreference = ALTIUM_PARSER::ReadString( aProps, "LIBREFERENCE", "" );
    sourcelibraryname = ALTIUM_PARSER::ReadString( aProps, "SOURCELIBRARYNAME", "" );
    componentdescription = ALTIUM_PARSER::ReadString( aProps, "COMPONENTDESCRIPTION", "" );

    orientation = ALTIUM_PARSER::ReadInt( aProps, "ORIENTATION", 0 );
    isMirrored  = ALTIUM_PARSER::ReadBool( aProps, "ISMIRRORED", false );
    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    partcount        = ALTIUM_PARSER::ReadInt( aProps, "PARTCOUNT", 0 );
    displaymodecount = ALTIUM_PARSER::ReadInt( aProps, "DISPLAYMODECOUNT", 0 );
    displaymode      = ALTIUM_PARSER::ReadInt( aProps, "DISPLAYMODE", 0 );
}


ASCH_PIN::ASCH_PIN( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PIN );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    name       = ALTIUM_PARSER::ReadString( aProps, "NAME", "" );
    text       = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );
    designator = ALTIUM_PARSER::ReadString( aProps, "DESIGNATOR", "" );

    int symbolOuterInt = ALTIUM_PARSER::ReadInt( aProps, "SYMBOL_OUTER", 0 );
    symbolOuter        = static_cast<ASCH_PIN_SYMBOL_OUTER>( symbolOuterInt );

    int symbolInnerInt = ALTIUM_PARSER::ReadInt( aProps, "SYMBOL_INNER", 0 );
    symbolInner        = static_cast<ASCH_PIN_SYMBOL_INNER>( symbolInnerInt );

    int symbolOuterEdgeInt = ALTIUM_PARSER::ReadInt( aProps, "SYMBOL_OUTEREDGE", 0 );
    symbolOuterEdge        = ( symbolOuterEdgeInt == 0 || symbolOuterEdgeInt == 1
                              || symbolOuterEdgeInt == 4 || symbolOuterEdgeInt == 17 ) ?
                              static_cast<ASCH_PIN_SYMBOL_OUTEREDGE>( symbolOuterEdgeInt ) :
                              ASCH_PIN_SYMBOL_OUTEREDGE::NO_SYMBOL;

    int symbolInnerEdgeInt = ALTIUM_PARSER::ReadInt( aProps, "SYMBOL_INNEREDGE", 0 );
    symbolInnerEdge        = ( symbolInnerEdgeInt == 0 || symbolInnerEdgeInt == 3 ) ?
                              static_cast<ASCH_PIN_SYMBOL_INNEREDGE>( symbolInnerEdgeInt ) :
                              ASCH_PIN_SYMBOL_INNEREDGE::NO_SYMBOL;
    electrical = ReadEnum<ASCH_PIN_ELECTRICAL>( aProps, "ELECTRICAL", 0, 7,
                                                ASCH_PIN_ELECTRICAL::INPUT );

    int pinconglomerate = ALTIUM_PARSER::ReadInt( aProps, "PINCONGLOMERATE", 0 );

    orientation    = static_cast<ASCH_RECORD_ORIENTATION>( pinconglomerate & 0x03 );
    showPinName    = ( pinconglomerate & 0x08 ) != 0;
    showDesignator = ( pinconglomerate & 0x10 ) != 0;

    int x     = ALTIUM_PARSER::ReadInt( aProps, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::ReadInt( aProps, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::ReadInt( aProps, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::ReadInt( aProps, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );

    int p     = ALTIUM_PARSER::ReadInt( aProps, "PINLENGTH", 0 );
    int pfrac = ALTIUM_PARSER::ReadInt( aProps, "PINLENGTH_FRAC", 0 );
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

    kicadLocation = wxPoint( Altium2KiCadUnit( kicadX, kicadXfrac ),
                             -Altium2KiCadUnit( kicadY, kicadYfrac ) );
}


ASCH_LABEL::ASCH_LABEL( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::LABEL );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    fontId     = ALTIUM_PARSER::ReadInt( aProps, "FONTID", 0 );
    isMirrored = ALTIUM_PARSER::ReadBool( aProps, "ISMIRRORED", false );

    justification = ReadEnum<ASCH_LABEL_JUSTIFICATION>( aProps, "JUSTIFICATION", 0, 8,
                                                        ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );
}


ASCH_TEXT_FRAME::ASCH_TEXT_FRAME( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NOTE
              || ReadRecord( aProps ) == ALTIUM_SCH_RECORD::TEXT_FRAME );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    size = wxSize( ReadKiCadUnitFrac( aProps, "CORNER.X" ) - location.x,
                   -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) - location.y );

    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );
    text.Replace( "~1", "\n", true );

    fontId = ALTIUM_PARSER::ReadInt( aProps, "FONTID", 0 );
    isWordWrapped = ALTIUM_PARSER::ReadBool( aProps, "WORDWRAP", false );
    border = ALTIUM_PARSER::ReadBool( aProps, "SHOWBORDER", false );
    textMargin = ReadKiCadUnitFrac( aProps, "TEXTMARGIN" );
    areaColor = ALTIUM_PARSER::ReadInt( aProps, "AREACOLOR", 0 );

    alignment = ReadEnum<ASCH_TEXT_FRAME_ALIGNMENT>( aProps, "ALIGNMENT", 1, 3,
                                                     ASCH_TEXT_FRAME_ALIGNMENT::LEFT );
}


ASCH_NOTE::ASCH_NOTE( const std::map<wxString, wxString>& aProperties ) :
        ASCH_TEXT_FRAME( aProperties )
{
    wxASSERT( ReadRecord( aProperties ) == ALTIUM_SCH_RECORD::NOTE );

    author = ALTIUM_PARSER::ReadString( aProperties, "AUTHOR", "" );
}


ASCH_BEZIER::ASCH_BEZIER( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BEZIER );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    int locationCount = ALTIUM_PARSER::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_POLYLINE::ASCH_POLYLINE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POLYLINE );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    int locationCount = ALTIUM_PARSER::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );

    int linestyleVar = ALTIUM_PARSER::ReadInt( aProps, "LINESTYLEEXT", 0 );
    linestyleVar     = ALTIUM_PARSER::ReadInt( aProps, "LINESTYLE", linestyleVar ); // overwrite if present
    linestyle        = linestyleVar >= 0 && linestyleVar <= 3 ?
                                static_cast<ASCH_POLYLINE_LINESTYLE>( linestyleVar ) :
                                ASCH_POLYLINE_LINESTYLE::SOLID;
}


ASCH_POLYGON::ASCH_POLYGON( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POLYGON );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    int locationCount = ALTIUM_PARSER::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
    isSolid   = ALTIUM_PARSER::ReadBool( aProps, "ISSOLID", false );

    color     = ALTIUM_PARSER::ReadInt( aProps, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::ReadInt( aProps, "AREACOLOR", 0 );
}


ASCH_ROUND_RECTANGLE::ASCH_ROUND_RECTANGLE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::ROUND_RECTANGLE );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    bottomLeft = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                          -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    topRight   = wxPoint( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                          -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    topRight = wxPoint( ReadKiCadUnitFrac( aProps, "CORNERXRADIUS" ),
                        -ReadKiCadUnitFrac( aProps, "CORNERYRADIUS" ) );

    lineWidth     = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
    isSolid       = ALTIUM_PARSER::ReadBool( aProps, "ISSOLID", false );
    isTransparent = ALTIUM_PARSER::ReadBool( aProps, "TRANSPARENT", false );

    color     = ALTIUM_PARSER::ReadInt( aProps, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::ReadInt( aProps, "AREACOLOR", 0 );
}


ASCH_ARC::ASCH_ARC( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::ARC );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    center = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                      -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    radius = ReadKiCadUnitFrac( aProps, "RADIUS" );

    startAngle = ALTIUM_PARSER::ReadDouble( aProps, "STARTANGLE", 0 );
    endAngle   = ALTIUM_PARSER::ReadDouble( aProps, "ENDANGLE", 0 );

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_LINE::ASCH_LINE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::LINE );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    point1 = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                      -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    point2 = wxPoint( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                      -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_RECTANGLE::ASCH_RECTANGLE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::RECTANGLE );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PARSER::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    bottomLeft = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                          -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    topRight   = wxPoint( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                          -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    lineWidth     = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
    isSolid       = ALTIUM_PARSER::ReadBool( aProps, "ISSOLID", false );
    isTransparent = ALTIUM_PARSER::ReadBool( aProps, "TRANSPARENT", false );

    color     = ALTIUM_PARSER::ReadInt( aProps, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::ReadInt( aProps, "AREACOLOR", 0 );
}


ASCH_SHEET_SYMBOL::ASCH_SHEET_SYMBOL( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_SYMBOL );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    size     = wxSize( ReadKiCadUnitFrac( aProps, "XSIZE" ),
                       ReadKiCadUnitFrac( aProps, "YSIZE" ) );

    isSolid = ALTIUM_PARSER::ReadBool( aProps, "ISSOLID", false );

    color     = ALTIUM_PARSER::ReadInt( aProps, "COLOR", 0 );
    areacolor = ALTIUM_PARSER::ReadInt( aProps, "AREACOLOR", 0 );
}


ASCH_SHEET_ENTRY::ASCH_SHEET_ENTRY( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_ENTRY );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    // some magic, because it stores those infos in a different unit??
    distanceFromTop = ReadKiCadUnitFrac1( aProps, "DISTANCEFROMTOP" );

    side = ReadEnum<ASCH_SHEET_ENTRY_SIDE>( aProps, "SIDE", 0, 3, ASCH_SHEET_ENTRY_SIDE::LEFT );

    name = ALTIUM_PARSER::ReadString( aProps, "NAME", "" );

    iotype = ReadEnum<ASCH_PORT_IOTYPE>( aProps, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    style = ReadEnum<ASCH_PORT_STYLE>( aProps, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );
}


ASCH_POWER_PORT::ASCH_POWER_PORT( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POWER_PORT );

    ownerpartid = ReadOwnerPartId( aProps );

    location    = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                           -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    text        = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );
    showNetName = ALTIUM_PARSER::ReadBool( aProps, "SHOWNETNAME", true );

    style = ReadEnum<ASCH_POWER_PORT_STYLE>( aProps, "STYLE", 0, 10,
                                             ASCH_POWER_PORT_STYLE::CIRCLE );
}

ASCH_PORT::ASCH_PORT( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PORT );

    ownerpartid = ReadOwnerPartId( aProps );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    name = ALTIUM_PARSER::ReadString( aProps, "NAME", "" );
    harnessType = ALTIUM_PARSER::ReadString( aProps, "HARNESSTYPE", "" );

    width  = ReadKiCadUnitFrac( aProps, "WIDTH" );
    height = ReadKiCadUnitFrac( aProps, "HEIGHT" );

    iotype = ReadEnum<ASCH_PORT_IOTYPE>( aProps, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    style = ReadEnum<ASCH_PORT_STYLE>( aProps, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );
}


ASCH_NO_ERC::ASCH_NO_ERC( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NO_ERC );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isActive   = ALTIUM_PARSER::ReadBool( aProps, "ISACTIVE", true );
    supressAll = ALTIUM_PARSER::ReadInt( aProps, "SUPPRESSALL", true );
}


ASCH_NET_LABEL::ASCH_NET_LABEL( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NET_LABEL );

    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );
}


ASCH_BUS::ASCH_BUS( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BUS );

    indexinsheet = ALTIUM_PARSER::ReadInt( aProps, "INDEXINSHEET", 0 );

    int locationcount = ALTIUM_PARSER::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_WIRE::ASCH_WIRE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::WIRE );

    indexinsheet = ALTIUM_PARSER::ReadInt( aProps, "INDEXINSHEET", 0 );

    int locationcount = ALTIUM_PARSER::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_JUNCTION::ASCH_JUNCTION( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::JUNCTION );

    ownerpartid = ReadOwnerPartId( aProps );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
}


ASCH_IMAGE::ASCH_IMAGE( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMAGE );

    indexinsheet = ALTIUM_PARSER::ReadInt( aProps, "INDEXINSHEET", 0 );
    ownerpartid = ReadOwnerPartId( aProps );

    filename = ALTIUM_PARSER::ReadString( aProps, "FILENAME", "" );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    corner = wxPoint( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                      -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    embedimage = ALTIUM_PARSER::ReadBool( aProps, "EMBEDIMAGE", false );
    keepaspect = ALTIUM_PARSER::ReadBool( aProps, "KEEPASPECT", false );
}


ASCH_SHEET_FONT::ASCH_SHEET_FONT( const std::map<wxString, wxString>& aProps, int aId )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET );

    const wxString sid = std::to_string( aId );

    fontname = ALTIUM_PARSER::ReadString( aProps, "FONTNAME" + sid, "" );

    size     = ReadKiCadUnitFrac( aProps, "SIZE" + sid );
    rotation = ALTIUM_PARSER::ReadInt( aProps, "ROTATION" + sid, 0 );

    italic    = ALTIUM_PARSER::ReadBool( aProps, "ITALIC" + sid, false );
    bold      = ALTIUM_PARSER::ReadBool( aProps, "BOLD" + sid, false );
    underline = ALTIUM_PARSER::ReadBool( aProps, "UNDERLINE" + sid, false );
}

wxPoint ASchSheetGetSize( ASCH_SHEET_SIZE aSheetSize )
{
    // From: https://github.com/vadmium/python-altium/blob/master/format.md#sheet
    switch( aSheetSize )
    {
    default:
    case ASCH_SHEET_SIZE::A4:      return { 1150,  760 };
    case ASCH_SHEET_SIZE::A3:      return { 1550, 1110 };
    case ASCH_SHEET_SIZE::A2:      return { 2230, 1570 };
    case ASCH_SHEET_SIZE::A1:      return { 3150, 2230 };
    case ASCH_SHEET_SIZE::A0:      return { 4460, 3150 };
    case ASCH_SHEET_SIZE::A:       return {  950,  750 };
    case ASCH_SHEET_SIZE::B:       return { 1500,  950 };
    case ASCH_SHEET_SIZE::C:       return { 2000, 1500 };
    case ASCH_SHEET_SIZE::D:       return { 3200, 2000 };
    case ASCH_SHEET_SIZE::E:       return { 4200, 3200 };
    case ASCH_SHEET_SIZE::LETTER:  return { 1100,  850 };
    case ASCH_SHEET_SIZE::LEGAL:   return { 1400,  850 };
    case ASCH_SHEET_SIZE::TABLOID: return { 1700, 1100 };
    case ASCH_SHEET_SIZE::ORCAD_A: return {  990,  790 };
    case ASCH_SHEET_SIZE::ORCAD_B: return { 1540,  990 };
    case ASCH_SHEET_SIZE::ORCAD_C: return { 2060, 1560 };
    case ASCH_SHEET_SIZE::ORCAD_D: return { 3260, 2060 };
    case ASCH_SHEET_SIZE::ORCAD_E: return { 4280, 3280 };
    }
}


ASCH_SHEET::ASCH_SHEET( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET );

    int fontidcount = ALTIUM_PARSER::ReadInt( aProps, "FONTIDCOUNT", 0 );

    for( int i = 1; i <= fontidcount; i++ )
        fonts.emplace_back( aProps, i );

    sheetSize = ReadEnum<ASCH_SHEET_SIZE>( aProps, "SHEETSTYLE", 0, 17, ASCH_SHEET_SIZE::A4 );
    sheetOrientation = ReadEnum<ASCH_SHEET_WORKSPACEORIENTATION>(
            aProps, "WORKSPACEORIENTATION", 0, 1, ASCH_SHEET_WORKSPACEORIENTATION::LANDSCAPE );
}


ASCH_SHEET_NAME::ASCH_SHEET_NAME( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_NAME );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isHidden = ALTIUM_PARSER::ReadBool( aProps, "ISHIDDEN", false );
}


ASCH_FILE_NAME::ASCH_FILE_NAME( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::FILE_NAME );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isHidden = ALTIUM_PARSER::ReadBool( aProps, "ISHIDDEN", false );
}


ASCH_DESIGNATOR::ASCH_DESIGNATOR( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::DESIGNATOR );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    name = ALTIUM_PARSER::ReadString( aProps, "NAME", "" );
    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
}


ASCH_IMPLEMENTATION::ASCH_IMPLEMENTATION( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMPLEMENTATION );

    ownerindex = ALTIUM_PARSER::ReadInt( aProps, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    name = ALTIUM_PARSER::ReadString( aProps, "MODELNAME", "" );
    type = ALTIUM_PARSER::ReadString( aProps, "MODELTYPE", "" );
    libname = ALTIUM_PARSER::ReadString( aProps, "MODELDATAFILE0", "" );
    isCurrent = ALTIUM_PARSER::ReadBool( aProps, "ISCURRENT", false );
}


ASCH_IMPLEMENTATION_LIST::ASCH_IMPLEMENTATION_LIST( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMPLEMENTATION_LIST );

    ownerindex = ReadOwnerIndex( aProps );
}

ASCH_BUS_ENTRY::ASCH_BUS_ENTRY( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BUS_ENTRY );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    corner   = wxPoint( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                        -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );
}


ASCH_PARAMETER::ASCH_PARAMETER( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PARAMETER );

    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );

    location = wxPoint( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                        -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    name = ALTIUM_PARSER::ReadString( aProps, "NAME", "" );
    text = ALTIUM_PARSER::ReadString( aProps, "TEXT", "" );

    isHidden   = ALTIUM_PARSER::ReadBool( aProps, "ISHIDDEN", false );
    isMirrored = ALTIUM_PARSER::ReadBool( aProps, "ISMIRRORED", false );
    isShowName = ALTIUM_PARSER::ReadBool( aProps, "SHOWNAME", false );
}
