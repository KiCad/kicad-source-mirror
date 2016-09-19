/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file gbr_metadata.cpp
 * @brief helper functions to handle the gerber metadata in files,
 * related to the netlist info and aperture attribute.
 */

#include <fctsys.h>
#include <plot_auxiliary_data.h>


std::string GBR_APERTURE_METADATA::FormatAttribute( GBR_APERTURE_ATTRIB aAttribute )
{
    std::string attribute_string;

    // generate a string to print a Gerber Aperture attribute
    switch( aAttribute )
    {
    case GBR_APERTURE_ATTRIB_END:           // Dummy value (aAttribute must be < GBR_APERTURE_ATTRIB_END)
    case GBR_APERTURE_ATTRIB_NONE:          // idle command: do nothing
        break;

    case GBR_APERTURE_ATTRIB_ETCHEDCMP:     // print info associated to an item
                                            // which connects 2 different nets
                                            // (Net tees, microwave component)
        attribute_string = "%TA.AperFunction,EtchedComponent*%\n";
        break;

    case GBR_APERTURE_ATTRIB_CONDUCTOR:     // print info associated to a track
        attribute_string = "%TA.AperFunction,Conductor*%\n";
        break;

    case GBR_APERTURE_ATTRIB_CUTOUT:        // print info associated to a outline
        attribute_string = "%TA.AperFunction,CutOut*%\n";
        break;

    case GBR_APERTURE_ATTRIB_VIAPAD:        // print info associated to a flashed via
        attribute_string = "%TA.AperFunction,ViaPad*%\n";
        break;

    case GBR_APERTURE_ATTRIB_NONCONDUCTOR:  // print info associated to a flashed pad
        attribute_string = "%TA.AperFunction,NonConductor*%\n";
        break;

    case GBR_APERTURE_ATTRIB_COMPONENTPAD:  // print info associated to a flashed
                                            // through hole component on outer layer
        attribute_string = "%TA.AperFunction,ComponentPad*%\n";
        break;

    case GBR_APERTURE_ATTRIB_SMDPAD_SMDEF:  // print info associated to a flashed for SMD pad.
                                            // with  solder mask defined from the copper shape
                                            // Excluded BGA pads which have their own type
        attribute_string = "%TA.AperFunction,SMDPad,SMDef*%\n";
        break;

    case GBR_APERTURE_ATTRIB_SMDPAD_CUDEF:  // print info associated to a flashed SMD pad with
                                            // a solder mask defined by the solder mask
        attribute_string = "%TA.AperFunction,SMDPad,CuDef*%\n";
        break;

    case GBR_APERTURE_ATTRIB_BGAPAD_SMDEF:  // print info associated to flashed BGA pads with
                                            // a solder mask defined by the copper shape
        attribute_string = "%TA.AperFunction,BGAPad,SMDef*%\n";
        break;

    case GBR_APERTURE_ATTRIB_BGAPAD_CUDEF:  // print info associated to a flashed BGA pad with
                                            // a solder mask defined by the solder mask
        attribute_string = "%TA.AperFunction,BGAPad,CuDef*%\n";
        break;

    case GBR_APERTURE_ATTRIB_CONNECTORPAD:  // print info associated to a flashed edge connector pad (outer layers)
        attribute_string = "%TA.AperFunction,ConnectorPad*%\n";
        break;

    case GBR_APERTURE_ATTRIB_WASHERPAD:     // print info associated to flashed mechanical pads (NPTH)
        attribute_string = "%TA.AperFunction,WasherPad*%\n";
        break;

    case GBR_APERTURE_ATTRIB_HEATSINKPAD:   // print info associated to a flashed heat sink pad (typically for SMDs)
        attribute_string = "%TA.AperFunction,HeatsinkPad*%\n";
        break;
    }

    return attribute_string;
}


std::string formatStringToGerber( const wxString& aString )
{
    /* format string means convert any code > 0x7F and unautorized code to a hexadecimal
     * 16 bits sequence unicode
     * unautorized codes are ',' '*' '%' '\'
     */
    std::string txt;

    txt.reserve( aString.Length() );

    for( unsigned ii = 0; ii < aString.Length(); ++ii )
    {
        unsigned code = aString[ii];
        bool convert = false;

        switch( code )
        {
        case '\\':
        case '%':
        case '*':
        case ',':
            convert = true;
            break;

        default:
            break;
        }

        if( convert || code > 0x7F )
        {
            txt += '\\';

            // Convert code to 4 hexadecimal digit
            // (Gerber allows only 4 hexadecimal digit)
            char hexa[32];
            sprintf( hexa,"%4.4X", code & 0xFFFF);
            txt += hexa;
        }
        else
            txt += char( code );
    }

    return txt;
}

// Netname and Pan num fields cannot be empty in Gerber files
// Normalized names must be used, if any
#define NO_NET_NAME wxT( "N/C" )    // net name of not connected pads (one pad net) (normalized)
#define NO_PAD_NAME wxT( "" )       // pad name of pads without pad name/number (not normalized)

bool FormatNetAttribute( std::string& aPrintedText, std::string& aLastNetAttributes,
                         GBR_NETLIST_METADATA* aData, bool& aClearPreviousAttributes )
{
    aClearPreviousAttributes = false;

    // print a Gerber net attribute record.
    // it is added to the object attributes dictionnary
    // On file, only modified or new attributes are printed.
    if( aData == NULL )
        return false;

    std::string pad_attribute_string;
    std::string net_attribute_string;
    std::string cmp_attribute_string;

    if( aData->m_NetAttribType == GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED )
        return false;     // idle command: do nothing

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        // print info associated to a flashed pad (cmpref, pad name)
        // example: %TO.P,R5,3*%
        pad_attribute_string = "%TO.P,";
        pad_attribute_string += formatStringToGerber( aData->m_Cmpref ) + ",";

        if( aData->m_Padname.IsEmpty() )
            // Happens for "mechanical" or never connected pads
            pad_attribute_string += formatStringToGerber( NO_PAD_NAME );
        else
            pad_attribute_string += formatStringToGerber( aData->m_Padname );

        pad_attribute_string += "*%\n";
    }

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
    {
        // print info associated to a net
        // example: %TO.N,Clk3*%
        net_attribute_string = "%TO.N,";

        if( aData->m_Netname.IsEmpty() )
        {
            if( aData->m_NotInNet )
            {
                // Happens for not connectable pads: mechanical pads
                // and pads with no padname/num
                // In this case the net name must be left empty
            }
            else
            {
                // Happens for not connected pads: use a normalized
                // dummy name
                net_attribute_string += formatStringToGerber( NO_NET_NAME );
            }
        }
        else
            net_attribute_string += formatStringToGerber( aData->m_Netname );

        net_attribute_string += "*%\n";
    }

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) &&
        !( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        // print info associated to a footprint
        // example: %TO.C,R2*%
        // Because GBR_NETINFO_PAD option already contains this info, it is not
        // created here for a GBR_NETINFO_PAD attribute
        cmp_attribute_string = "%TO.C,";
        cmp_attribute_string += formatStringToGerber( aData->m_Cmpref ) + "*%\n";
    }

    // the full list of requested attributes:
    std::string full_attribute_string = pad_attribute_string + net_attribute_string
                                   + cmp_attribute_string;
    // the short list of requested attributes
    // (only modified or new attributes are stored here):
    std::string short_attribute_string;

    if( aLastNetAttributes != full_attribute_string )
    {
        // first, remove no more existing attributes.
        // Because in Kicad the full attribute list is evaluated for each object,
        // the entire dictionnary is cleared
        bool clearDict = false;

        if( aLastNetAttributes.find( "%TO.P," ) != std::string::npos )
        {
            if( pad_attribute_string.empty() )  // No more this attribute
                clearDict = true;
            else if( aLastNetAttributes.find( pad_attribute_string )
                     == std::string::npos )     // This attribute has changed
                short_attribute_string += pad_attribute_string;
        }
        else    // New attribute
            short_attribute_string += pad_attribute_string;

        if( aLastNetAttributes.find( "%TO.N," ) != std::string::npos )
        {
            if( net_attribute_string.empty() )  // No more this attribute
                clearDict = true;
            else if( aLastNetAttributes.find( net_attribute_string )
                     == std::string::npos )     // This attribute has changed
                short_attribute_string += net_attribute_string;
        }
        else    // New attribute
            short_attribute_string += net_attribute_string;

        if( aLastNetAttributes.find( "%TO.C," ) != std::string::npos )
        {
            if( cmp_attribute_string.empty() )  // No more this attribute
                clearDict = true;
            else if( aLastNetAttributes.find( cmp_attribute_string )
                     == std::string::npos )     // This attribute has changed
                short_attribute_string += cmp_attribute_string;
        }
        else    // New attribute
            short_attribute_string += cmp_attribute_string;

        aClearPreviousAttributes = clearDict;

        aLastNetAttributes = full_attribute_string;

        if( clearDict )
            aPrintedText = full_attribute_string;
        else
            aPrintedText = short_attribute_string;
    }

    return true;
}
