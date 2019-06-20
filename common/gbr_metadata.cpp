/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gbr_metadata.h>

wxString GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT aFormat )
{
    // creates the CreationDate attribute:
    // The attribute value must conform to the full version of the ISO 8601
    // date and time format, including time and time zone. Note that this is
    // the date the Gerber file was effectively created,
    // not the time the project of PCB was started
    wxDateTime date( wxDateTime::GetTimeNow() );
    // Date format: see http://www.cplusplus.com/reference/ctime/strftime
    wxString timezone_offset;   // ISO 8601 offset from UTC in timezone
    timezone_offset = date.Format( "%z" );  // Extract the time zone offset
    // The time zone offset format is +mm or +hhmm (or -mm or -hhmm)
    // (mm = number of minutes, hh = number of hours. 1h00mn is returned as +0100)
    // we want +(or -) hh:mm
    if( timezone_offset.Len() > 3 )     // format +hhmm or -hhmm found
        // Add separator between hours and minutes
        timezone_offset.insert( 3, ":", 1 );

    wxString msg;

    switch( aFormat )
    {
    case GBR_NC_STRING_FORMAT_X2:
        msg.Printf( "%%TF.CreationDate,%s%s*%%", date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_X1:
        msg.Printf( "G04 #@! TF.CreationDate,%s%s*", date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_GBRJOB:
        msg.Printf( "\"CreationDate\":  \"%s%s\"", date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_NCDRILL:
        msg.Printf( "; #@! TF.CreationDate,%s%s", date.FormatISOCombined(), timezone_offset );
        break;
    }
    return msg;
}


wxString GbrMakeProjectGUIDfromString( wxString& aText )
{
    /* Gerber GUID format should be RFC4122 Version 1 or 4.
     * See en.wikipedia.org/wiki/Universally_unique_identifier
     * The format is:
     * xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
     * with
     *   x = hexDigit lower/upper case
     * and
     *  M = '1' or '4' (UUID version: 1 (basic) or 4 (random)) (we use 4: UUID random)
     * and
     *  N = '8' or '9' or 'A|a' or 'B|b' : UUID variant 1: 2 MSB bits have meaning) (we use N = 9)
     *  N = 1000 or 1001 or 1010 or 1011  : 10xx means Variant 1 (Variant2: 110x and 111x are reserved)
     */

    wxString guid;

    // Build a 32 digits GUID from the board name:
    // guid has 32 digits, so add chars in name to be sure we can build a 32 digits guid
    // (i.e. from a 16 char string name)
    // In fact only 30 digits are used, and 2 UID id
    wxString bname = aText;
    int cnt = 16 - bname.Len();

    if( cnt > 0 )
        bname.Append( 'X', cnt );

    int chr_idx = 0;

    // Output the 8 first hex digits:
    for( unsigned ii = 0; ii < 4; ii++ )
    {
        int cc = int( bname[chr_idx++] ) & 0xFF;
        guid << wxString::Format( "%2.2x", cc );
    }

    // Output the 4 next hex digits:
    guid << '-';

    for( unsigned ii = 0; ii < 2; ii++ )
    {
        int cc = int( bname[chr_idx++] ) & 0xFF;
        guid << wxString::Format( "%2.2x", cc );
    }

    // Output the 4 next hex digits (UUID version and 3 digits):
    guid << "-4";   // first digit: UUID version 4 (M = 4)
    {
        int cc = int( bname[chr_idx++] ) << 4 & 0xFF0;
        cc += int( bname[chr_idx] ) >> 4 & 0x0F;
        guid << wxString::Format( "%3.3x", cc );
    }

    // Output the 4 next hex digits (UUID variant and 3 digits):
    guid << "-9";  // first digit: UUID variant 1 (N = 9)
    {
        int cc = (int( bname[chr_idx++] ) & 0x0F) << 8;
        cc += int( bname[chr_idx++] ) & 0xFF;
        guid << wxString::Format( "%3.3x", cc );
    }

    // Output the 12 last hex digits:
    guid << '-';

    for( unsigned ii = 0; ii < 6; ii++ )
    {
        int cc = int( bname[chr_idx++] ) & 0xFF;
        guid << wxString::Format( "%2.2x", cc );
    }

    return guid;
}


std::string GBR_APERTURE_METADATA::FormatAttribute( GBR_APERTURE_ATTRIB aAttribute,
                                                    bool aUseX1StructuredComment )
{
    std::string attribute_string;   // the specific aperture attribute (TA.xxx)
    std::string comment_string;     // a optional G04 comment line to write before the TA. line

    // generate a string to print a Gerber Aperture attribute
    switch( aAttribute )
    {
    case GBR_APERTURE_ATTRIB_END:           // Dummy value (aAttribute must be < GBR_APERTURE_ATTRIB_END)
    case GBR_APERTURE_ATTRIB_NONE:          // idle command: do nothing
        break;

    case GBR_APERTURE_ATTRIB_ETCHEDCMP:     // print info associated to an item
                                            // which connects 2 different nets
                                            // (Net tees, microwave component)
        attribute_string = "TA.AperFunction,EtchedComponent";
        break;

    case GBR_APERTURE_ATTRIB_CONDUCTOR:     // print info associated to a track
        attribute_string = "TA.AperFunction,Conductor";
        break;

    case GBR_APERTURE_ATTRIB_CUTOUT:        // print info associated to a outline
        attribute_string = "TA.AperFunction,CutOut";
        break;

    case GBR_APERTURE_ATTRIB_VIAPAD:        // print info associated to a flashed via
        attribute_string = "TA.AperFunction,ViaPad";
        break;

    case GBR_APERTURE_ATTRIB_NONCONDUCTOR:  // print info associated to a item on a copper layer
                                            // which is not a track (for instance a text)
        attribute_string = "TA.AperFunction,NonConductor";
        break;

    case GBR_APERTURE_ATTRIB_COMPONENTPAD:  // print info associated to a flashed
                                            // through hole component on outer layer
        attribute_string = "TA.AperFunction,ComponentPad";
        break;

    case GBR_APERTURE_ATTRIB_SMDPAD_SMDEF:  // print info associated to a flashed for SMD pad.
                                            // with  solder mask defined from the copper shape
                                            // Excluded BGA pads which have their own type
        attribute_string = "TA.AperFunction,SMDPad,SMDef";
        break;

    case GBR_APERTURE_ATTRIB_SMDPAD_CUDEF:  // print info associated to a flashed SMD pad with
                                            // a solder mask defined by the solder mask
        attribute_string = "TA.AperFunction,SMDPad,CuDef";
        break;

    case GBR_APERTURE_ATTRIB_BGAPAD_SMDEF:  // print info associated to flashed BGA pads with
                                            // a solder mask defined by the copper shape
        attribute_string = "TA.AperFunction,BGAPad,SMDef";
        break;

    case GBR_APERTURE_ATTRIB_BGAPAD_CUDEF:  // print info associated to a flashed BGA pad with
                                            // a solder mask defined by the solder mask
        attribute_string = "TA.AperFunction,BGAPad,CuDef";
        break;

    case GBR_APERTURE_ATTRIB_CONNECTORPAD:  // print info associated to a flashed edge connector pad (outer layers)
        attribute_string = "TA.AperFunction,ConnectorPad";
        break;

    case GBR_APERTURE_ATTRIB_WASHERPAD:     // print info associated to flashed mechanical pads (NPTH)
        attribute_string = "TA.AperFunction,WasherPad";
        break;

    case GBR_APERTURE_ATTRIB_HEATSINKPAD:   // print info associated to a flashed heat sink pad
                                            // (typically for SMDs)
        attribute_string = "TA.AperFunction,HeatsinkPad";
        break;

    case GBR_APERTURE_ATTRIB_VIADRILL:   // print info associated to a via hole in drill files
        attribute_string = "TA.AperFunction,ViaDrill";
        break;

    case GBR_APERTURE_ATTRIB_COMPONENTDRILL:    // print info associated to a component
                                                // round pad hole in drill files
        attribute_string = "TA.AperFunction,ComponentDrill";
        break;

    // print info associated to a component oblong pad hole in drill files
    // Same as a round pad hole, but is a specific aperture in drill file and
    // a G04 comment is added to the aperture function
    case GBR_APERTURE_ATTRIB_COMPONENTOBLONGDRILL:
        comment_string = "aperture for slot hole";
        attribute_string = "TA.AperFunction,ComponentDrill";

    break;
    }

    std::string full_attribute_string;
    wxString eol_string;

    if( !attribute_string.empty() )
    {
        if( !comment_string.empty() )
        {
            full_attribute_string = "G04 " + comment_string + "*\n";
        }

        if( aUseX1StructuredComment )
        {
            full_attribute_string += "G04 #@! ";
            eol_string = "*\n";
        }
        else
        {
            full_attribute_string += "%";
            eol_string = "*%\n";
        }
    }

    full_attribute_string += attribute_string + eol_string;

    return full_attribute_string;
}

wxString FormatStringFromGerber( const wxString& aString )
{
    // make the inverse conversion of formatStringToGerber()
    // It converts a "normalized" gerber string and convert it to a 16 bits sequence unicode
    // and return a wxString (unicode 16) from the gerber string
    wxString txt;

    for( unsigned ii = 0; ii < aString.Length(); ++ii )
    {
        unsigned code = aString[ii];

        if( code == '\\' )
        {
            // Convert 4 hexadecimal digits to a 16 bit unicode
            // (Gerber allows only 4 hexadecimal digits)
            long value = 0;

            for( int jj = 0; jj < 4; jj++ )
            {
                value <<= 4;
                code = aString[++ii];
                // Very basic conversion, but it expects a valid gerber file
                int hexa = (code <= '9' ? code - '0' : code - 'A' + 10) & 0xF;
                value += hexa;
            }

            txt.Append( wxChar( value ) );
        }
        else
            txt.Append( aString[ii] );
    }

    return txt;
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
                         GBR_NETLIST_METADATA* aData, bool& aClearPreviousAttributes,
                         bool aUseX1StructuredComment )
{
    aClearPreviousAttributes = false;
    wxString prepend_string;
    wxString eol_string;

    if( aUseX1StructuredComment )
    {
        prepend_string = "G04 #@! ";
        eol_string = "*\n";
    }
    else
    {
        prepend_string = "%";
        eol_string = "*%\n";
    }

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
        pad_attribute_string = prepend_string + "TO.P,";
        pad_attribute_string += formatStringToGerber( aData->m_Cmpref ) + ",";

        if( aData->m_Padname.IsEmpty() )
            // Happens for "mechanical" or never connected pads
            pad_attribute_string += formatStringToGerber( NO_PAD_NAME );
        else
            pad_attribute_string += formatStringToGerber( aData->m_Padname );

        pad_attribute_string += eol_string;
    }

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
    {
        // print info associated to a net
        // example: %TO.N,Clk3*%
        net_attribute_string = prepend_string + "TO.N,";

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

        net_attribute_string += eol_string;
    }

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) &&
        !( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        // print info associated to a footprint
        // example: %TO.C,R2*%
        // Because GBR_NETINFO_PAD option already contains this info, it is not
        // created here for a GBR_NETINFO_PAD attribute
        cmp_attribute_string = prepend_string + "TO.C,";
        cmp_attribute_string += formatStringToGerber( aData->m_Cmpref ) + eol_string;
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

        if( aLastNetAttributes.find( "TO.P," ) != std::string::npos )
        {
            if( pad_attribute_string.empty() )  // No more this attribute
                clearDict = true;
            else if( aLastNetAttributes.find( pad_attribute_string )
                     == std::string::npos )     // This attribute has changed
                short_attribute_string += pad_attribute_string;
        }
        else    // New attribute
            short_attribute_string += pad_attribute_string;

        if( aLastNetAttributes.find( "TO.N," ) != std::string::npos )
        {
            if( net_attribute_string.empty() )  // No more this attribute
                clearDict = true;
            else if( aLastNetAttributes.find( net_attribute_string )
                     == std::string::npos )     // This attribute has changed
                short_attribute_string += net_attribute_string;
        }
        else    // New attribute
            short_attribute_string += net_attribute_string;

        if( aLastNetAttributes.find( "TO.C," ) != std::string::npos )
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
