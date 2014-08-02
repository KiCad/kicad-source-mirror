/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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

#ifndef IDF_HELPERS_H
#define IDF_HELPERS_H

#include <wx/wx.h>
#include <fstream>
#include <string>
#include <idf_common.h>

/**
 * Macro TO_UTF8
 * converts a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 * NOTE: Taken from KiCad include/macros.h
 */
#define TO_UTF8( wxstring )  ( (const char*) (wxstring).utf8_str() )

/**
 * function FROM_UTF8
 * converts a UTF8 encoded C string to a wxString for all wxWidgets build modes.
 * NOTE: Taken from KiCad include/macros.h
 */
static inline wxString FROM_UTF8( const char* cstring )
{
    wxString line = wxString::FromUTF8( cstring );

    if( line.IsEmpty() )  // happens when cstring is not a valid UTF8 sequence
        line = wxConvCurrent->cMB2WC( cstring );    // try to use locale conversion

        return line;
}


#define ERROR_IDF std::cerr << "* " << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): "

// minimum drill diameters / slot widths to be represented in the IDF output
#define IDF_MIN_DIA_MM ( 0.001 )
#define IDF_MIN_DIA_THOU ( 0.00039 )
#define IDF_MIN_DIA_TNM ( 100 )

// conversion from thou to mm
#define IDF_THOU_TO_MM 0.0254
// conversion from TNM to mm
#define IDF_TNM_TO_MM 0.00001

namespace IDF3
{

/**
 *  Function FetchIDFLine
 *  retrieves a single line from an IDF file and performs minimal processing. If a comment symbol
 *  is encountered then it is removed and a single leading space is removed if present; all trailing
 *  spaces are removed. If the line is not a comment then all leading and trailing spaces are stripped.
 *
 * @param aModel is an open IDFv3 file
 * @param aLine (output) is the line retrieved from the file
 * @param isComment (output) is set to true if the line is a comment
 * @param aFilePos (output) is set to the beginning of the line in case the file needs to be rewound
 *
 * @return bool: true if a line was read and was not empty; otherwise false
 */
bool FetchIDFLine( std::ifstream& aModel, std::string& aLine, bool& isComment, std::streampos& aFilePos );


/**
 * Function GetIDFString
 * parses a line retrieved via FetchIDFLine() and returns the first IDF string found from the starting
 * point aIndex
 *
 * @param aLine is the line to parse
 * @param aIDFString (output) is the IDF string retrieved
 * @param hasQuotes (output) is true if the string was in quotation marks
 * @param aIndex (input/output) is the index into the input line
 *
 * @return bool: true if a string was retrieved, otherwise false
 */
bool GetIDFString( const std::string& aLine, std::string& aIDFString,
                   bool& hasQuotes, int& aIndex );

/**
 * Function CompareToken
 * performs a case-insensitive comparison of a token string and an input string
 *
 * @param aToken is an IDF token such as ".HEADER"
 * @param aInputString is a string typically retrieved via GetIDFString
 *
 * @return bool: true if the token and input string match
 */
bool CompareToken( const char* aTokenString, const std::string& aInputString );


/**
 * Function ParseOwner
 * parses the input string for a valid IDF Owner type
 *
 * @param aToken is the string to be parsed
 * @param aOwner (output) is the IDF Owner class
 *
 * @return bool: true if a valid OWNER was found, otherwise false
 */
bool ParseOwner( const std::string& aToken, IDF3::KEY_OWNER& aOwner );


/**
 * Function ParseIDFLayer
 * parses an input string for a valid IDF layer specification
 *
 * @param aToken is the string to be parsed
 * @param aLayer (output) is the IDF Layer type or group
 *
 * @return bool: true if a valid IDF Layer type was found, otherwise false
 */
bool ParseIDFLayer( const std::string& aToken, IDF3::IDF_LAYER& aLayer );


/**
 * Function WriteLayersText
 * writes out text corresponding to the given IDF Layer type
 *
 * @param aBoardFile is an IDFv3 file open for output
 * @param aLayer is the IDF Layer type
 *
 * @return bool: true if the data was successfully written, otherwise false
 */
bool WriteLayersText( std::ofstream& aBoardFile, IDF3::IDF_LAYER aLayer );


/**
 * Function GetPlacementString
 * returns a string representing the given IDF Placement type
 *
 * @param aPlacement is the IDF placement type to encode as a string
 *
 * @return string: the string representation of aPlacement
 */
std::string GetPlacementString( IDF3::IDF_PLACEMENT aPlacement );


/**
 * Function GetLayerString
 * returns a string representing the given IDF Layer type
 *
 * @param aLayer is the IDF layer type to encode as a string
 *
 * @return string: the string representation of aLayer
 */
std::string GetLayerString( IDF3::IDF_LAYER aLayer );

std::string GetOwnerString( IDF3::KEY_OWNER aOwner );
}

#endif // IDF_HELPERS_H
