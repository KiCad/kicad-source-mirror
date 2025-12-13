/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file gbr_metadata.cpp
 * @brief helper functions to handle the gerber metadata in files,
 * related to the netlist info and aperture attribute.
 */
#include <wx/string.h>
#include <wx/datetime.h>

#include <gbr_metadata.h>
#include <core/utf8.h>


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
        msg.Printf( wxS( "%%TF.CreationDate,%s%s*%%" ), date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_X1:
        msg.Printf( wxS( "G04 #@! TF.CreationDate,%s%s*" ), date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_GBRJOB:
        msg.Printf( wxS( "%s%s" ), date.FormatISOCombined(), timezone_offset );
        break;

    case GBR_NC_STRING_FORMAT_NCDRILL:
        msg.Printf( wxS( "; #@! TF.CreationDate,%s%s" ), date.FormatISOCombined(), timezone_offset );
        break;
    }

    return msg;
}


wxString GbrMakeProjectGUIDfromString( const wxString& aText )
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
     *  N = 1000 or 1001 or 1010 or 1011  : 10xx means Variant 1 (Variant2: 110x and 111x are
     *  reserved)
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
                                                    bool                aUseX1StructuredComment,
                                                    const std::string&  aCustomAttribute )
{
    std::string attribute_string;   // the specific aperture attribute (TA.xxx)
    std::string comment_string;     // a optional G04 comment line to write before the TA. line

    // generate a string to print a Gerber Aperture attribute
    switch( aAttribute )
    {
    // Dummy value (aAttribute must be < GBR_APERTURE_ATTRIB_END).
    case GBR_APERTURE_ATTRIB_END:
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

    case GBR_APERTURE_ATTRIB_EDGECUT:        // print info associated to a board outline
        attribute_string = "TA.AperFunction,Profile";
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

    case GBR_APERTURE_ATTRIB_CONNECTORPAD:
        // print info associated to a flashed edge connector pad (outer layers)
        attribute_string = "TA.AperFunction,ConnectorPad";
        break;

    case GBR_APERTURE_ATTRIB_WASHERPAD:
        // print info associated to flashed mechanical pads (NPTH)
        attribute_string = "TA.AperFunction,WasherPad";
        break;

    case GBR_APERTURE_ATTRIB_HEATSINKPAD:   // print info associated to a flashed heat sink pad
                                            // (typically for SMDs)
        attribute_string = "TA.AperFunction,HeatsinkPad";
        break;

    case GBR_APERTURE_ATTRIB_TESTPOINT:   // print info associated to a flashed test point pad
                                            // (typically for SMDs)
        attribute_string = "TA.AperFunction,TestPad";
        break;

    case GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL: // print info associated to a flashed fiducial pad
                                            // (typically for SMDs)
        attribute_string = "TA.AperFunction,FiducialPad,Global";
        break;

    case GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL:    // print info associated to a flashed fiducial pad
                                                // (typically for SMDs)
        attribute_string = "TA.AperFunction,FiducialPad,Local";
        break;

    case GBR_APERTURE_ATTRIB_CASTELLATEDPAD:
        // print info associated to a flashed castellated pad (typically for SMDs)
        attribute_string = "TA.AperFunction,CastellatedPad";
        break;

    case GBR_APERTURE_ATTRIB_CASTELLATEDDRILL:
        // print info associated to a flashed castellated pad in drill files
        attribute_string = "TA.AperFunction,CastellatedDrill";
        break;

    case GBR_APERTURE_ATTRIB_VIADRILL:      // print info associated to a via hole in drill files
        attribute_string = "TA.AperFunction,ViaDrill";
        break;

    case GBR_APERTURE_ATTRIB_BACKDRILL:
        attribute_string = "TA.AperFunction,BackDrill";
        break;

    case GBR_APERTURE_ATTRIB_CMP_DRILL:     // print info associated to a component
                                            // round pad hole in drill files
        attribute_string = "TA.AperFunction,ComponentDrill";
        break;

    case GBR_APERTURE_ATTRIB_PRESSFITDRILL:
        // print info associated to a flashed component pad with pressfit option in drill files
        attribute_string = "TA.AperFunction,ComponentDrill,PressFit";
        break;

    // print info associated to a component oblong pad hole in drill files
    // Same as a round pad hole, but is a specific aperture in drill file and
    // a G04 comment is added to the aperture function
    case GBR_APERTURE_ATTRIB_CMP_OBLONG_DRILL:
        comment_string = "aperture for slot hole";
        attribute_string = "TA.AperFunction,ComponentDrill";
        break;

    case GBR_APERTURE_ATTRIB_CMP_POSITION:      // print info associated to a component
                                                // flashed shape at the component position
                                                // in placement files
        attribute_string = "TA.AperFunction,ComponentMain";
        break;

    case GBR_APERTURE_ATTRIB_PAD1_POS:     // print info associated to a component
                                                // flashed shape at pad 1 position
                                                // (pad 1 is also pad A1 or pad AA1)
                                                // in placement files
        attribute_string = "TA.AperFunction,ComponentPin";
        break;

    case GBR_APERTURE_ATTRIB_PADOTHER_POS: // print info associated to a component
                                                // flashed shape at pads position (all but pad 1)
                                                // in placement files
                                                // Currently: (could be changed later) same as
                                                // GBR_APERTURE_ATTRIB_PADOTHER_POS
        attribute_string = "TA.AperFunction,ComponentPin";
        break;

    case GBR_APERTURE_ATTRIB_CMP_BODY:          // print info associated to a component
                                                // print the component physical body
                                                // polygon in placement files
        attribute_string = "TA.AperFunction,ComponentOutline,Body";
        break;

    case GBR_APERTURE_ATTRIB_CMP_LEAD2LEAD:     // print info associated to a component
                                                // print the component physical lead to lead
                                                // polygon in placement files
        attribute_string = "TA.AperFunction,ComponentOutline,Lead2Lead";
        break;

    case GBR_APERTURE_ATTRIB_CMP_FOOTPRINT:     // print info associated to a component
                                                // print the component footprint bounding box
                                                // polygon in placement files
        attribute_string = "TA.AperFunction,ComponentOutline,Footprint";
        break;

    case GBR_APERTURE_ATTRIB_CMP_COURTYARD:     // print info associated to a component
                                                // print the component courtyard
                                                // polygon in placement files
        attribute_string = "TA.AperFunction,ComponentOutline,Courtyard";
        break;

    case GBR_APERTURE_ATTRIB_OTHER:
        attribute_string = "TA.AperFunction,Other," + aCustomAttribute;
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


// Helper function to convert a ascii hex char to its integer value
// If the char is not a hexa char, return -1
int char2Hex( unsigned aCode )
{
    if( aCode >= '0' && aCode <= '9' )
        return aCode - '0';

    if( aCode >= 'A' && aCode <= 'F' )
        return aCode - 'A' + 10;

    if( aCode >= 'a' && aCode <= 'f' )
        return aCode - 'a' + 10;

    return -1;
}


wxString FormatStringFromGerber( const wxString& aString )
{
    // make the inverse conversion of FormatStringToGerber()
    // It converts a "normalized" gerber string containing escape sequences
    // and convert it to a 16 bits Unicode char
    // and return a wxString (Unicode 16) from the gerber string
    // Note the initial gerber string can already contain Unicode chars.
    wxString txt;           // The string converted from Gerber string

    unsigned count = aString.Length();

    for( unsigned ii = 0; ii < count; ++ii )
    {
        unsigned code = aString[ii];

        if( code == '\\' && ii < count-5 && aString[ii+1] == 'u' )
        {
            // Note the latest Gerber X2 spec (2019 06) uses \uXXXX to encode
            // the Unicode XXXX hexadecimal value
            // If 4 chars next to 'u' are hexadecimal chars,
            // Convert these 4 hexadecimal digits to a 16 bit Unicode
            // (Gerber allows only 4 hexadecimal digits)
            // If an error occurs, the escape sequence is not translated,
            // and used "as this"
            long value = 0;
            bool error = false;

            for( int jj = 0; jj < 4; jj++ )
            {
                value <<= 4;
                code = aString[ii+jj+2];

                int hexa = char2Hex( code );

                if( hexa >= 0 )
                    value += hexa;
                else
                {
                    error = true;
                    break;
                }
            }

            if( !error )
            {
                if( value >= ' ' )  // Is a valid wxChar ?
                    txt.Append( wxChar( value ) );

                ii += 5;
            }
            else
            {
                txt.Append( aString[ii] );
                continue;
            }
        }
        else
        {
            txt.Append( aString[ii] );
        }
    }

    return txt;
}


wxString ConvertNotAllowedCharsInGerber( const wxString& aString, bool aAllowUtf8Chars,
                                         bool aQuoteString )
{
    /* format string means convert any code > 0x7E and unauthorized codes to a hexadecimal
     * 16 bits sequence Unicode
     * However if aAllowUtf8Chars is true only unauthorized codes will be escaped, because some
     * Gerber files accept UTF8 chars.
     * unauthorized codes are ',' '*' '%' '\' '"' and are used as separators in Gerber files
     */
    wxString txt;

    if( aQuoteString )
        txt << "\"";

    for( unsigned ii = 0; ii < aString.Length(); ++ii )
    {
        wxChar code = aString[ii];
        bool convert = false;

        switch( code )
        {
        case '\\':
        case '%':
        case '*':
        case ',':
            convert = true;
            break;

        case '"':
            if( aQuoteString )
                convert = true;
            break;

        default:
            break;
        }

        if( !aAllowUtf8Chars && code > 0x7F )
            convert = true;

        if( convert )
        {
            // Convert code to 4 hexadecimal digit
            // (Gerber allows only 4 hexadecimal digit) in escape seq:
            // "\uXXXX", XXXX is the Unicode 16 bits hexa value
            char hexa[32];
            std::snprintf( hexa, sizeof( hexa ), "\\u%4.4X", code & 0xFFFF );
            txt += hexa;
        }
        else
        {
            txt += code;
        }
    }

    if( aQuoteString )
        txt << "\"";

    return txt;
}


std::string GBR_DATA_FIELD::GetGerberString() const
{
    wxString converted;

    if( !m_field.IsEmpty() )
        converted = ConvertNotAllowedCharsInGerber( m_field, m_useUTF8, m_escapeString );

    // Convert the char string to std::string. Be careful when converting a wxString to
    // a std::string: using static_cast<const char*> is mandatory
    std::string txt = static_cast<const char*>( converted.utf8_str() );

    return txt;
}


std::string FormatStringToGerber( const wxString& aString )
{
    wxString converted;

    /* format string means convert any code > 0x7E and unauthorized codes to a hexadecimal
     * 16 bits sequence Unicode
     * unauthorized codes are ',' '*' '%' '\'
     * This conversion is not made for quoted strings, because if the string is
     * quoted, the conversion is expected to be already made, and the returned string must use
     * UTF8 encoding
     */
    if( !aString.IsEmpty() && ( aString[0] != '\"' || aString[aString.Len()-1] != '\"' ) )
        converted = ConvertNotAllowedCharsInGerber( aString, false, false );
    else
        converted = aString;

    // Convert the char string to std::string. Be careful when converting a wxString to
    // a std::string: using static_cast<const char*> is mandatory
    std::string txt = static_cast<const char*>( converted.utf8_str() );

    return txt;
}


// Netname and Pan num fields cannot be empty in Gerber files
// Normalized names must be used, if any
#define NO_NET_NAME wxT( "N/C" )    // net name of not connected pads (one pad net) (normalized)
#define NO_PAD_NAME wxT( "" )       // pad name of pads without pad name/number (not normalized)


bool FormatNetAttribute( std::string& aPrintedText, std::string& aLastNetAttributes,
                         const GBR_NETLIST_METADATA* aData, bool& aClearPreviousAttributes,
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
    // it is added to the object attributes dictionary
    // On file, only modified or new attributes are printed.
    if( aData == nullptr )
        return false;

    std::string pad_attribute_string;
    std::string net_attribute_string;
    std::string cmp_attribute_string;

    if( aData->m_NetAttribType == GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED )
        return false;     // idle command: do nothing

    if( ( aData->m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        // print info associated to a flashed pad (cmpref, pad name, and optionally pin function)
        // example1: %TO.P,R5,3*%
        // example2: %TO.P,R5,3,reset*%
        pad_attribute_string = prepend_string + "TO.P,";
        pad_attribute_string += FormatStringToGerber( aData->m_Cmpref ) + ",";

        if( aData->m_Padname.IsEmpty() )
        {
            // Happens for "mechanical" or never connected pads
            pad_attribute_string += FormatStringToGerber( NO_PAD_NAME );
        }
        else
        {
            pad_attribute_string += aData->m_Padname.GetGerberString();

            // In Pcbnew, the pin function comes from the schematic.
            // so it exists only for named pads
            if( !aData->m_PadPinFunction.IsEmpty() )
            {
                pad_attribute_string += ',';
                pad_attribute_string += aData->m_PadPinFunction.GetGerberString();
            }
        }

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
                net_attribute_string += FormatStringToGerber( NO_NET_NAME );
            }
        }
        else
        {
            net_attribute_string += FormatStringToGerber( aData->m_Netname );
        }

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
        cmp_attribute_string += FormatStringToGerber( aData->m_Cmpref ) + eol_string;
    }

    // the full list of requested attributes:
    std::string full_attribute_string = pad_attribute_string + net_attribute_string
                                   + cmp_attribute_string;
    // the short list of requested attributes
    // (only modified or new attributes are stored here):
    std::string short_attribute_string;

    // Attributes have changed: update attribute string, and see if the previous attribute
    // list (dictionary in Gerber language) must be cleared
    if( aLastNetAttributes != full_attribute_string )
    {
        // first, remove no longer existing attributes.
        // Because in KiCad the full attribute list is evaluated for each object,
        // the entire dictionary is cleared
        // If m_TryKeepPreviousAttributes is true, only the no longer existing attribute
        // is cleared.
        // Note: to avoid interaction between clear attributes and set attributes
        // the clear attribute is inserted first.
        bool clearDict = false;

        if( aLastNetAttributes.find( "TO.P," ) != std::string::npos )
        {
            if( pad_attribute_string.empty() )  // No more this attribute
            {
                if( aData->m_TryKeepPreviousAttributes )    // Clear only this attribute
                    short_attribute_string.insert( 0, prepend_string + "TO.P" + eol_string );
                else
                    clearDict = true;
            }
            else if( aLastNetAttributes.find( pad_attribute_string ) == std::string::npos )
            {
                // This attribute has changed
                short_attribute_string += pad_attribute_string;
            }
        }
        else    // New attribute
        {
            short_attribute_string += pad_attribute_string;
        }

        if( aLastNetAttributes.find( "TO.N," ) != std::string::npos )
        {
            if( net_attribute_string.empty() )  // No more this attribute
            {
                if( aData->m_TryKeepPreviousAttributes )    // Clear only this attribute
                    short_attribute_string.insert( 0, prepend_string + "TO.N" + eol_string );
                else
                    clearDict = true;
            }
            else if( aLastNetAttributes.find( net_attribute_string ) == std::string::npos )
            {
                // This attribute has changed.
                short_attribute_string += net_attribute_string;
            }
        }
        else    // New attribute
        {
            short_attribute_string += net_attribute_string;
        }

        if( aLastNetAttributes.find( "TO.C," ) != std::string::npos )
        {
            if( cmp_attribute_string.empty() )  // No more this attribute
            {
                if( aData->m_TryKeepPreviousAttributes )    // Clear only this attribute
                {
                    // Refinement:
                    // the attribute will be cleared only if there is no pad attribute.
                    // If a pad attribute exists, the component name exists so the old
                    // TO.C value will be updated, therefore no need to clear it before updating
                    if( pad_attribute_string.empty() )
                        short_attribute_string.insert( 0, prepend_string + "TO.C" + eol_string );
                }
                else
                {
                    clearDict = true;
                }
            }
            else if( aLastNetAttributes.find( cmp_attribute_string ) == std::string::npos )
            {
                // This attribute has changed.
                short_attribute_string += cmp_attribute_string;
            }
        }
        else    // New attribute
        {
            short_attribute_string += cmp_attribute_string;
        }

        aClearPreviousAttributes = clearDict;

        aLastNetAttributes = full_attribute_string;

        if( clearDict )
            aPrintedText = full_attribute_string;
        else
            aPrintedText = short_attribute_string;
    }

    return true;
}


void GBR_CMP_PNP_METADATA::ClearData()
{
    // Clear all strings
    m_Orientation = 0.0;
    m_Manufacturer.Clear();
    m_MPN.Clear();
    m_Package.Clear();
    m_Value.Clear();
    m_MountType = MOUNT_TYPE_UNSPECIFIED;
}


wxString GBR_CMP_PNP_METADATA::FormatCmpPnPMetadata()
{
    wxString text;
    wxString start_of_line( "%TO." );
    wxString end_of_line( "*%\n" );

    wxString mountTypeStrings[] =
    {
        "Other", "SMD", "TH"
    };

    if( !m_Manufacturer.IsEmpty() )
        text << start_of_line << "CMfr," << m_Manufacturer << end_of_line;

    if( !m_MPN.IsEmpty() )
        text << start_of_line << "CMPN," << m_MPN << end_of_line;

    if( !m_Package.IsEmpty() )
        text << start_of_line << "Cpkg," << m_Package << end_of_line;

    if( !m_Footprint.IsEmpty() )
        text << start_of_line << "CFtp," << m_Footprint << end_of_line;

    if( !m_Value.IsEmpty() )
        text << start_of_line << "CVal," << m_Value << end_of_line;

    if( !m_LibraryName.IsEmpty() )
        text << start_of_line << "CLbN," << m_LibraryName << end_of_line;

    if( !m_LibraryDescr.IsEmpty() )
        text << start_of_line << "CLbD," << m_LibraryDescr << end_of_line;

    text << start_of_line << "CMnt," << mountTypeStrings[m_MountType] << end_of_line;
    text << start_of_line << "CRot," << m_Orientation << end_of_line;

    return text;
}
