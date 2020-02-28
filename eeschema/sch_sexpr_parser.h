/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file sch_sexpr_parser.h
 * @brief Schematic and symbol library s-expression file format parser definitions.
 */

#ifndef __SCH_SEXPR_PARSER_H__
#define __SCH_SEXPR_PARSER_H__

#include <convert_to_biu.h>                      // IU_PER_MM
#include <math/util.h>                           // KiROUND, Clamp

#include <class_library.h>
#include <symbol_lib_lexer.h>


class LIB_ARC;
class LIB_BEZIER;
class LIB_CIRCLE;
class LIB_ITEM;
class LIB_PIN;
class LIB_POLYLINE;
class LIB_TEXT;


class SCH_SEXPR_PARSER : public SYMBOL_LIB_LEXER
{
    int m_requiredVersion;  ///< Set to the symbol library file version required.
    int m_fieldId;          ///< The current field ID.
    int m_unit;             ///< The current unit being parsed.
    int m_convert;          ///< The current body style being parsed.
    wxString m_symbolName;  ///< The current symbol name.

    void parseHeader();

    inline int parseInt()
    {
        return (int)strtol( CurText(), NULL, 10 );
    }

    inline int parseInt( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseInt();
    }

    /**
     * Parse the current token as an ASCII numeric string with possible leading
     * whitespace into a double precision floating point number.
     *
     * @throw IO_ERROR if an error occurs attempting to convert the current token.
     * @return The result of the parsed token.
     */
    double parseDouble();

    inline double parseDouble( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseDouble();
    }

    inline double parseDouble( TSYMBOL_LIB_T::T aToken )
    {
        return parseDouble( GetTokenText( aToken ) );
    }

    inline int parseInternalUnits()
    {
        auto retval = parseDouble() * IU_PER_MM;

        // Schematic internal units are represented as integers.  Any values that are
        // larger or smaller than the schematic units represent undefined behavior for
        // the system.  Limit values to the largest that can be displayed on the screen.
        double int_limit = std::numeric_limits<int>::max() * 0.7071; // 0.7071 = roughly 1/sqrt(2)

        return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
    }

    inline int parseInternalUnits( const char* aExpected )
    {
        auto retval = parseDouble( aExpected ) * IU_PER_MM;

        double int_limit = std::numeric_limits<int>::max() * 0.7071;

        return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
    }

    inline int parseInternalUnits( TSYMBOL_LIB_T::T aToken )
    {
        return parseInternalUnits( GetTokenText( aToken ) );
    }

    inline wxPoint parseXY()
    {
        wxPoint xy;

        xy.x = parseInternalUnits( "X coordinate" );
        xy.y = parseInternalUnits( "Y coordinate" );

        return xy;
    }

    FILL_T parseFillMode();

    void parseEDA_TEXT( EDA_TEXT* aText );
    void parsePinNames( std::unique_ptr<LIB_PART>& aSymbol );

    void parseProperty( std::unique_ptr<LIB_PART>& aSymbol );

    LIB_ARC* parseArc();
    LIB_BEZIER* parseBezier();
    LIB_CIRCLE* parseCircle();
    LIB_PIN* parsePin();
    LIB_POLYLINE* parsePolyLine();
    LIB_RECTANGLE* parseRectangle();
    LIB_TEXT* parseText();

public:
    SCH_SEXPR_PARSER( LINE_READER* aLineReader = nullptr );

    void ParseLib( LIB_PART_MAP& aSymbolLibMap );

    LIB_PART* ParseSymbol( LIB_PART_MAP& aSymbolLibMap );

    LIB_ITEM* ParseDrawItem();

    /**
     * Return whether a version number, if any was parsed, was too recent
     */
    bool IsTooRecent() const;
};

#endif    // __SCH_SEXPR_PARSER_H__
