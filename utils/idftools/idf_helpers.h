/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 Cirilo Bernardo
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

#ifndef IDF_HELPERS_H
#define IDF_HELPERS_H

#include <wx/string.h>
#include <iostream>
#include <string>
#include <idf_common.h>

/**
 * Convert a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 *
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 *
 * NOTE: Taken from KiCad include/macros.h
 */
#define TO_UTF8( wxstring )  ( (const char*) (wxstring).utf8_str() )

/**
 * Convert a UTF8 encoded C string to a wxString for all wxWidgets build modes.
 *
 * NOTE: Taken from KiCad include/macros.h
 */
static inline wxString From_UTF8( const char* cstring )
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
 *  Retrieve a single line from an IDF file and performs minimal processing.
 *
 * If a comment symbol is encountered then it is removed and a single leading space is removed
 * if present; all trailing spaces are removed. If the line is not a comment then all leading
 * and trailing spaces are stripped.
 *
 * @param[in] aModel is an open IDFv3 file.
 * @param[out] aLine is the line retrieved from the file.
 * @param[out] isComment is set to true if the line is a comment.
 * @param[out] aFilePosis set to the beginning of the line in case the file needs to be rewound.
 * @return true if a line was read and was not empty; otherwise false.
 */
bool FetchIDFLine( std::istream& aModel, std::string& aLine, bool& isComment,
                   std::streampos& aFilePos );


/**
 * Parse a line retrieved via FetchIDFLine() and returns the first IDF string found from the
 * starting point aIndex.
 *
 * @param[in] aLine is the line to parse.
 * @param[out] aIDFString is the IDF string retrieved.
 * @param[out] hasQuotes is true if the string was in quotation marks.
 * @param[in,out] aIndex is the index into the input line.
 * @return true if a string was retrieved, otherwise false.
 */
bool GetIDFString( const std::string& aLine, std::string& aIDFString, bool& hasQuotes,
                   int& aIndex );

/**
 * Perform a case-insensitive comparison of a token string and an input string.
 *
 * @param aToken is an IDF token such as ".HEADER".
 * @param aInputString is a string typically retrieved via GetIDFString.
 * @return true if the token and input string match.
 */
bool CompareToken( const char* aTokenString, const std::string& aInputString );


/**
 * Parse the input string for a valid IDF Owner type.
 *
 * @param[in] aToken is the string to be parsed.
 * @param[out] aOwner is the IDF Owner class.
 * @return true if a valid OWNER was found, otherwise false.
 */
bool ParseOwner( const std::string& aToken, IDF3::KEY_OWNER& aOwner );


/**
 * Parse an input string for a valid IDF layer specification.
 *
 * @param[in] aToken is the string to be parsed.
 * @param[out] aLayer is the IDF Layer type or group.
 * @return true if a valid IDF Layer type was found, otherwise false.
 */
bool ParseIDFLayer( const std::string& aToken, IDF3::IDF_LAYER& aLayer );


/**
 * Write out text corresponding to the given IDF Layer type.
 *
 * @param[in] aBoardFile is an IDFv3 file open for output.
 * @param aLayer is the IDF Layer type.
 * @return true if the data was successfully written, otherwise false
 */
bool WriteLayersText( std::ostream& aBoardFile, IDF3::IDF_LAYER aLayer );


/**
 * Return a string representing the given IDF Placement type.
 *
 * @param aPlacement is the IDF placement type to encode as a string.
 * @return the string representation of aPlacement.
 */
std::string GetPlacementString( IDF3::IDF_PLACEMENT aPlacement );


/**
 * Return a string representing the given IDF Layer type.
 *
 * @param aLayer is the IDF layer type to encode as a string.
 * @return the string representation of aLayer.
 */
std::string GetLayerString( IDF3::IDF_LAYER aLayer );

std::string GetOwnerString( IDF3::KEY_OWNER aOwner );
}

#endif // IDF_HELPERS_H
