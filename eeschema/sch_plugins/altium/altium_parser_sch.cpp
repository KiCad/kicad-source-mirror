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


ASCH_COMPONENT::ASCH_COMPONENT( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::COMPONENT );

    currentpartid =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "CURRENTPARTID", ALTIUM_COMPONENT_NONE );
    libreference = ALTIUM_PARSER::PropertiesReadString( aProperties, "LIBREFERENCE", "" );

    orientation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );

    int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
}


ASCH_PIN::ASCH_PIN( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::PIN );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

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

    orientation    = static_cast<ASCH_PIN_ORIENTATION>( pinconglomerate & 0x03 );
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
}


ASCH_RECTANGLE::ASCH_RECTANGLE( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::RECTANGLE );

    ownerindex =
            ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", 0 );

    int x      = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac  = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y      = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac  = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    bottomLeft = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );

    x        = ALTIUM_PARSER::PropertiesReadInt( aProperties, "CORNER.X", 0 );
    xfrac    = ALTIUM_PARSER::PropertiesReadInt( aProperties, "CORNER.X_FRAC", 0 );
    y        = ALTIUM_PARSER::PropertiesReadInt( aProperties, "CORNER.Y", 0 );
    yfrac    = ALTIUM_PARSER::PropertiesReadInt( aProperties, "CORNER.Y_FRAC", 0 );
    topRight = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );

    lineWidth     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LINEWIDTH", 0 );
    isSolid       = ALTIUM_PARSER::PropertiesReadBool( aProperties, "ISSOLID", false );
    isTransparent = ALTIUM_PARSER::PropertiesReadBool( aProperties, "TRANSPARENT", false );
}


ASCH_NET_LABEL::ASCH_NET_LABEL( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::NET_LABEL );

    text        = ALTIUM_PARSER::PropertiesReadString( aProperties, "TEXT", "" );
    orientation = ALTIUM_PARSER::PropertiesReadInt( aProperties, "ORIENTATION", 0 );

    int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
}


ASCH_BUS::ASCH_BUS( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::BUS );

    indexinsheet = ALTIUM_PARSER::PropertiesReadInt( aProperties, "INDEXINSHEET", 0 );
    linewidth    = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LINEWIDTH", 0 );

    int locationcount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 0; i < locationcount; i++ )
    {
        const wxString si = std::to_string( i + 1 );

        int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "X" + si, 0 );
        int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "X" + si + "_FRAC", 0 );
        int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "Y" + si, 0 );
        int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "Y" + si + "_FRAC", 0 );
        points.emplace_back( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
    }
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
    linewidth    = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LINEWIDTH", 0 );

    int locationcount = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATIONCOUNT", 0 );
    for( int i = 0; i < locationcount; i++ )
    {
        const wxString si = std::to_string( i + 1 );

        int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "X" + si, 0 );
        int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "X" + si + "_FRAC", 0 );
        int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "Y" + si, 0 );
        int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "Y" + si + "_FRAC", 0 );
        points.emplace_back( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
    }
}


ASCH_JUNCTION::ASCH_JUNCTION( const std::map<wxString, wxString>& aProperties )
{
    wxASSERT( PropertiesReadRecord( aProperties ) == ALTIUM_SCH_RECORD::JUNCTION );

    ownerpartid = ALTIUM_PARSER::PropertiesReadInt( aProperties, "OWNERPARTID", -1 );

    int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
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

    int x     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PARSER::PropertiesReadInt( aProperties, "LOCATION.Y_FRAC", 0 );
    location  = wxPoint( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );
}