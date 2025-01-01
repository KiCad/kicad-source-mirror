/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file wrlproc.h
 * defines the basic input class for VRML
 */


#ifndef WRLPROC_H
#define WRLPROC_H

#include <fstream>
#include <string>
#include <vector>

#include "richio.h"
#include "wrltypes.h"

class WRLPROC
{
public:
    WRLPROC( LINE_READER* aLineReader );
    ~WRLPROC();

    bool eof( void );

    // return the VRML Version
    WRLVERSION GetVRMLType( void );

    // return the parent directory of the current file
    const char* GetParentDir( void );

    // helper routines
    std::string GetError( void );
    bool GetFilePosData( size_t& line, size_t& column );
    std::string GetFilePosition() const;
    std::string GetFileName( void );

    // eatSpace discards all leading white space from the current m_linepos
    // and continues until a non-empty line is found which contains non-blank
    // characters
    bool EatSpace( void );

    // Peek returns the next non-white char in the file or '\0' on failure
    char Peek( void );

    // Pop removes the current char from the buffer
    void Pop( void );

    // read up to the next whitespace or comma
    bool ReadGlob( std::string& aGlob );

    // read a VRML name; is similar to ReadGlob except that it enforces
    // name checking rules, does not allow a comma at the end, and
    // stops when a left/right brace or bracket is found.
    bool ReadName( std::string& aName );
    bool DiscardNode( void );               // discard node data in a matched '{}' set
    bool DiscardList( void );               // discard list data in a matched '[]' set

    // single variable readers
    bool ReadString( std::string& aSFString );  // read a VRML string
    bool ReadSFBool( bool& aSFBool );
    bool ReadSFColor( WRLVEC3F& aSFColor );
    bool ReadSFFloat( float& aSFFloat );
    bool ReadSFInt( int& aSFInt32 );
    bool ReadSFRotation( WRLROTATION& aSFRotation );
    bool ReadSFVec2f( WRLVEC2F& aSFVec2f );
    bool ReadSFVec3f( WRLVEC3F& aSFVec3f );

    // array readers
    bool ReadMFString( std::vector< std::string >& aMFString );
    bool ReadMFColor( std::vector< WRLVEC3F >& aMFColor );
    bool ReadMFFloat( std::vector< float >& aMFFloat );
    bool ReadMFInt( std::vector< int >& aMFInt32 );
    bool ReadMFRotation( std::vector< WRLROTATION >& aMFRotation );
    bool ReadMFVec2f( std::vector< WRLVEC2F >& aMFVec2f );
    bool ReadMFVec3f( std::vector< WRLVEC3F >& aMFVec3f );

    // getRawLine reads a single non-blank line and in the case of a VRML1 file
    // it checks for invalid characters (bit 8 set). If m_buf is not empty and
    // not completely parsed the function returns 'true'. The file position
    // parameters are updated as appropriate.
    bool getRawLine( void );

private:
    LINE_READER* m_file;
    std::string m_buf;          // string being parsed
    bool m_eof;
    unsigned int m_fileline;
    unsigned int m_bufpos;

    WRLVERSION m_fileVersion;   // VRML file version
    std::string m_error;        // error message
    std::string m_badchars;     // characters forbidden in VRML{1|2} names
    std::string m_filename;     // current file
    std::string m_filedir;      // parent directory of the file
};

#endif  // WRLPROC_H
