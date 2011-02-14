/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef SCH_SWEET_PARSER_H_
#define SCH_SWEET_PARSER_H_

#include <utf8.h>
#include <sweet_lexer.h>


namespace SCH {

class LIB_TABLE;
class PART;
class POLY_LINE;


/**
 * Class SWEET_PARSER
 * scans a Sweet string as input and stuffs a PART.
 * <p>
 * Most functions in this class throw IO_ERROR and PARSE_ERROR.  The IO_ERROR can
 * happen if there is difficulty reading the input stream.
 */
class SWEET_PARSER : public SWEET_LEXER
{
    LIB_TABLE*      libs;
    int             contains;       // separate from PART::contains until done
                                    // so we can see what we inherited from base PART

    // all these private functions rely on libs having been set via the public API, Parse( PART*)

    void parseExtends( PART* me );

    void parsePolyLine( POLY_LINE* me );


public:

    /**
     * Constructor SWEET_PARSER
     * takes aSweet string and gets ready to parse it.
     * @param aSweet is the full description of a PART.
     * @param aSource is used in error reporting and describes where the Sweet
     *  string came from in any appropriate way.
     */
    SWEET_PARSER( const STRING& aSweet, const wxString& aSource = wxEmptyString ) :
        SWEET_LEXER( aSweet, aSource ),
        libs( 0 ),
        contains( 0 )
    {
    }

    SWEET_PARSER( LINE_READER* aLineReader ) :
        SWEET_LEXER( aLineReader ),
        libs( 0 ),
        contains( 0 )
    {
    }

    /**
     * Function Parse
     * stuffs @a aPart with data from this SWEET_LEXER, which has its own
     * sweet string source.
     * @param aPart is what is to be stuffed.
     * @param aTable is the view of the LIBs in play.
     */
    void Parse( PART* aPart, LIB_TABLE* aTable ) throw( IO_ERROR, PARSE_ERROR );
};

} // namespace SCH

#endif  // SCH_SWEET_PARSER_H_

