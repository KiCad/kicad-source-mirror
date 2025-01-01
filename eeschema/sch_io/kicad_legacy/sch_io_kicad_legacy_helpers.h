/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_KICAD_LEGACY_HELPERS_H_
#define SCH_IO_KICAD_LEGACY_HELPERS_H_

#include <cstdint>


#define SCH_PARSE_ERROR( text, reader, pos )                         \
    THROW_PARSE_ERROR( text, reader.GetSource(), reader.Line(),      \
                       reader.LineNumber(), pos - reader.Line() )


class LINE_READER;
class wxString;


extern bool is_eol( char c );

/**
 * Compare \a aString to the string starting at \a aLine and advances the character point to
 * the end of \a String and returns the new pointer position in \a aOutput if it is not NULL.
 *
 * @param aString - A pointer to the string to compare.
 * @param aLine - A pointer to string to begin the comparison.
 * @param aOutput - A pointer to a string pointer to the end of the comparison if not NULL.
 * @return true if \a aString was found starting at \a aLine.  Otherwise false.
 */
extern bool strCompare( const char* aString, const char* aLine, const char** aOutput = nullptr );

/**
 * Parse an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtol()".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid integer value.
 * @throw An #IO_ERROR on an unexpected end of line.
 * @throw A #PARSE_ERROR if the parsed token is not a valid integer.
 */
extern int parseInt( LINE_READER& aReader, const char* aLine, const char** aOutput = nullptr );

/**
 * Parse an ASCII hex integer string with possible leading whitespace into
 * a long integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtoll".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid uint32_t value.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a valid integer.
 */
extern uint32_t parseHex( LINE_READER& aReader, const char* aLine, const char** aOutput = nullptr );

/**
 * Parses an ASCII point string with possible leading whitespace into a double precision
 * floating point number and  updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtod".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid double value.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a valid integer.
 */
extern double parseDouble( LINE_READER& aReader, const char* aLine,
                           const char** aOutput = nullptr );

/**
 * Parse a single ASCII character and updates the pointer at \a aOutput if it is not NULL.
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @return A valid ASCII character.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a single character token.
 */
extern char parseChar( LINE_READER& aReader, const char* aCurrentToken,
                       const char** aNextToken = nullptr );

/**
 * Parse an unquoted utf8 string and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be a continuous string with no white space.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
extern void parseUnquotedString( wxString& aString, LINE_READER& aReader, const char* aCurrentToken,
                                 const char** aNextToken = nullptr, bool aCanBeEmpty = false );

/**
 * Parse an quoted ASCII utf8 and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be contained within a single line.  There are no multi-line
 * quoted strings in the legacy schematic file format.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
extern void parseQuotedString( wxString& aString, LINE_READER& aReader, const char* aCurrentToken,
                               const char** aNextToken = nullptr, bool aCanBeEmpty = false );



#endif    // SCH_IO_KICAD_LEGACY_HELPERS_H_
