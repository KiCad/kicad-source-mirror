/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <sstream>
#include "wrlproc.h"

#define GETLINE do {\
    m_filepos = m_file.tellg(); \
    std::getline( m_file, m_buf ); \
    ++m_fileline; \
    } while( 0 )


WRLPROC::WRLPROC()
{
    m_filepos = 0;
    m_fileline = 0;
    m_linepos = 0;
    m_fileVersion = VRML_INVALID;

    return;
}


WRLPROC::~WRLPROC()
{
    Close();
    return;
}


bool WRLPROC::Open( const std::string& aFileName )
{
    if( m_file.is_open() )
    {
        m_error = "attempting to open a new file while one is still open";
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] " << m_error << "\n";
        #endif
        return false;
    }

    // initialize some internal variables; this is necessary since
    // the values may be set differently if there have been previous
    // failed calls to Open().
    m_error.clear();
    m_filename = aFileName;
    m_filepos = 0;
    m_fileline = 0;
    m_linepos = 0;
    m_file.open( aFileName.c_str() );

    if( !m_file.is_open() )
    {
        m_error = "could not open file '";
        m_error.append( aFileName );
        m_error.append( 1, '\'' );
        return false;
    }

    m_buf.clear();
    GETLINE;
    m_linepos = 0;
    m_buf = m_buf.substr( 0, 16 );

    if( m_buf.find( "#VRML V1.0 ascii"  ) == 0 )
    {
        m_fileVersion = VRML_V1;
        // nothing < 0x20, and no:
        // single or double quote
        // backslash
        // curly brace
        // plus
        // period
        m_badchars = "\"'\\{}+.";
        return true;
    }

    if( m_buf.find( "#VRML V2.0 utf8" ) == 0 )
    {
        m_fileVersion = VRML_V2;
        // nothing < 0x20, and no:
        // single or double quotes
        // sharp (#)
        // plus
        // minus
        // comma
        // period
        // square brackets []
        // curly braces {}
        // backslash
        m_badchars = "'\"#,.+-[]\\{}";
        return true;
    }

    m_file.close();
    m_buf.clear();
    m_fileVersion = VRML_INVALID;
    m_error = "not a valid VRML file: '";
    m_error.append( aFileName );
    m_error.append( 1, '\'' );
    m_badchars.clear();

    return false;
}


void WRLPROC::Close()
{
    m_file.close();
    m_buf.clear();
    m_filepos = 0;
    m_fileline = 0;
    m_linepos = 0;
    m_fileVersion = VRML_INVALID;
    m_filename.clear();
    m_error.clear();
    m_badchars.clear();

    return;
}


bool WRLPROC::getRawLine( void )
{
    m_error.clear();

    if( !m_file.is_open() )
    {
        m_error = "no open file";
        return false;
    }

    if( m_linepos >= m_buf.size() )
        m_buf.clear();

    if( !m_buf.empty() )
        return true;

    m_linepos = 0;

    if( m_file.bad() )
    {
        m_error = "bad stream";
        return false;
    }

    if( m_file.eof() )
        return false;

    GETLINE;

    if( m_file.bad() && m_buf.empty() )
        return false;

    if( VRML_V1 == m_fileVersion && !m_buf.empty() )
    {
        std::string::iterator sS = m_buf.begin();
        std::string::iterator eS = m_buf.end();

        while( sS != eS )
        {
            if( '\xff' == ((*sS) & 0x80) )
            {
                m_error = " non-ASCII character sequence in VRML1 file";
                return false;
            }

            ++sS;
        }
    }

    return true;
}


bool WRLPROC::EatSpace( void )
{
    if( m_linepos >= m_buf.size() )
        m_buf.clear();

RETRY:
    while( m_buf.empty() && !m_file.bad() && !m_file.eof() )
        getRawLine();

    // buffer may be empty if we have reached EOF or encountered IO errors
    if( m_buf.empty() )
        return false;

    // eliminate leading white space (including control characters)
    while( m_linepos < m_buf.size() )
    {
        if( m_buf[m_linepos] > 0x20 )
            break;

        ++m_linepos;
    }

    if( m_linepos == m_buf.size() )
    {
        // lines consisting entirely of white space are not unusual
        m_buf.clear();
        goto RETRY;
    }

    return true;
}


WRLVERSION WRLPROC::GetVRMLType( void )
{
    return m_fileVersion;
}


bool WRLPROC::ReadGlob( std::string& aGlob, bool* hasComma )
{
    aGlob.clear();

    if( NULL != hasComma )
        *hasComma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    size_t ssize = m_buf.size();

    while( m_buf[m_linepos] > 0x20 && m_linepos < ssize )
    {
        if( ',' == m_buf[m_linepos] )
        {
            // the comma is a special instance of blank space
            if( NULL != hasComma )
                *hasComma = true;

            ++m_linepos;

            return true;
        }

        if( '{' == m_buf[m_linepos] || '}' == m_buf[m_linepos]
            || '[' == m_buf[m_linepos] || ']' == m_buf[m_linepos] )
            return true;

        aGlob.append( 1, m_buf[m_linepos++] );
    }

    return true;
}


bool WRLPROC::ReadName( std::string& aName )
{
    aName.clear();

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    size_t ssize = m_buf.size();

    while( m_buf[m_linepos] > 0x20 && m_linepos < ssize )
    {
        if( '[' == m_buf[m_linepos] || '{' == m_buf[m_linepos]
            || '.' == m_buf[m_linepos] || '#' == m_buf[m_linepos] )
        {
            if( !aName.empty() )
            {
                return true;
            }
            else
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << m_fileline << ", column " << m_linepos;
                ostr << " -- invalid name";
                m_error = ostr.str();

                return false;
            }
        }

        if( m_badchars.find( m_buf[m_linepos] ) != std::string::npos )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << m_fileline << ", column " << m_linepos;
            ostr << " -- invalid character in name";
            m_error = ostr.str();

            return false;
        }

        aName.append( 1, m_buf[m_linepos++] );
    }

    return true;
}


bool WRLPROC::DiscardNode( void )
{
    if( !EatSpace() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( '{' != m_buf[m_linepos] )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] expecting character '{' at line " << m_linepos;
        ostr << ", column " << m_linepos;
        m_error = ostr.str();

        std::cerr << m_error << "\n";
        return false;
    }

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    ++m_linepos;
    size_t lvl = 1;
    std::string tmp;

    while( lvl > 0 )
    {
        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        // comments must be skipped
        if( '#' == m_buf[m_linepos] )
        {
            m_linepos = 0;
            m_buf.clear();
            continue;
        }

        if( '{' == m_buf[m_linepos] )
        {
            ++m_linepos;
            ++lvl;
            continue;
        }

        if( '}' == m_buf[m_linepos] )
        {
            ++m_linepos;
            --lvl;
            continue;
        }

        // note: if we have a ']' we must skip it and test the next non-blank character;
        // this ensures that we don't miss a '}' in cases where the braces are not
        // separated by space. if we had proceeded to ReadGlob() we could have had a problem.
        if( ']' == m_buf[m_linepos] || '[' == m_buf[m_linepos] )
        {
            ++m_linepos;
            continue;
        }

        // strings are handled as a special case since they may contain
        // control characters and braces
        if( '"' == m_buf[m_linepos] )
        {
            if( !ReadString( tmp ) )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] " << m_error;
                m_error = ostr.str();

                return false;
            }
        }

        // everything at this point can be read and discarded via ReadGlob()
        if( !ReadGlob( tmp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }
    }

    return true;
}


bool WRLPROC::DiscardList( void )
{
    if( !EatSpace() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( '[' != m_buf[m_linepos] )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] expecting character '[' at line " << m_linepos;
        ostr << ", column " << m_linepos;
        m_error = ostr.str();

        return false;
    }

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    ++m_linepos;
    size_t lvl = 1;
    std::string tmp;

    while( lvl > 0 )
    {
        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        // comments must be skipped
        if( '#' == m_buf[m_linepos] )
        {
            m_linepos = 0;
            m_buf.clear();
            continue;
        }

        if( '[' == m_buf[m_linepos] )
        {
            ++m_linepos;
            ++lvl;
            continue;
        }

        if( ']' == m_buf[m_linepos] )
        {
            ++m_linepos;
            --lvl;
            continue;
        }

        // note: if we have a '}' we must skip it and test the next non-blank character;
        // this ensures that we don't miss a ']' in cases where the braces are not
        // separated by space. if we had proceeded to ReadGlob() we could have had a problem.
        if( '}' == m_buf[m_linepos] || '{' == m_buf[m_linepos] )
        {
            ++m_linepos;
            continue;
        }

        // strings are handled as a special case since they may contain
        // control characters and braces
        if( '"' == m_buf[m_linepos] )
        {
            if( !ReadString( tmp ) )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] " << m_error;
                m_error = ostr.str();

                return false;
            }
        }

        // everything at this point can be read and discarded via ReadGlob()
        if( !ReadGlob( tmp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }
    }

    return false;
}


bool WRLPROC::ReadString( std::string& aSFString )
{
    // In VRML1 a string may be unquoted provided there are no space characters
    // In VRML2 all strings must be quoted
    aSFString.clear();

    // remember the line number in case of errors
    size_t ifline = m_fileline;

    while( true )
    {
        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << "invalid VRML file; expecting string at line " << ifline << " but found nothing";
            m_error = ostr.str();

            return false;
        }

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( VRML_V2 == m_fileVersion && '"' != m_buf[m_linepos] )
    {
        m_error = "invalid VRML2 file (string not quoted)";
        return false;
    }

    ifline = m_fileline;

    if( '"' != m_buf[m_linepos] )
    {
        if( !ReadGlob( aSFString ) )
        {
            std::ostringstream ostr;
            ostr << "invalid VRML1 file at lines " << ifline << "--" << m_fileline;

            if( !m_error.empty() )
                ostr << " : " << m_error;

            m_error = ostr.str();

            return false;
        }

        return true;
    }

    bool isesc = false; // true if an escape character was found

    while( true )
    {
        ++m_linepos;

        if( m_linepos >= m_buf.size() )
        {
            aSFString.append( 1, '\n' );

            if( !getRawLine() )
            {
                std::ostringstream ostr;
                ostr << "invalid VRML1 file at lines " << ifline << "--" << m_fileline;
                ostr << "; could not find end of string\n";
                m_error = ostr.str();

                return false;

            }
        }

        if( '\\' == m_buf[m_linepos] )
        {
            if( isesc )
            {
                aSFString.append( 1, '\n' );
                isesc = false;
            }
            else
            {
                isesc = true;
            }

            continue;
        }

        if( '"' ==  m_buf[m_linepos] )
        {
            if( isesc )
                aSFString.append( 1, '"' );
            else
                break;
        }

        // ensure that the backslash escape cannot extend beyond the first character
        isesc = false;
    }

    ++m_linepos;

    return true;
}


bool WRLPROC::ReadSFBool( bool& aSFBool )
{
    if( !EatSpace() )
        return false;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;
    std::string tmp;

    if( !ReadGlob( tmp ) )
        return false;

    if( !tmp.compare( "0" ) )
        aSFBool = false;
    else if( !tmp.compare( "1" ) )
        aSFBool = true;
    else if( !tmp.compare( "TRUE" ) )
        aSFBool = true;
    else if( !tmp.compare( "FALSE" ) )
        aSFBool = false;
    else
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] expected one of 0, 1, TRUE, FALSE but got '" << tmp << "'\n";
        m_error = ostr.str();

        return false;
    }

    return true;
}


bool WRLPROC::ReadSFColor( WRLVEC3F& aSFColor, bool* hasComma )
{
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    if( NULL != hasComma )
        *hasComma = false;

    bool lComma = false;

    if( !ReadSFVec3f( aSFColor, &lComma ) )
        return false;

    if( aSFColor.x < 0.0 || aSFColor.x > 1.0 || aSFColor.y < 0.0 || aSFColor.y > 1.0
        || aSFColor.z < 0.0 || aSFColor.z > 1.0 )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] invalid RGB value in color triplet";
        m_error = ostr.str();

        return false;
    }

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadSFFloat( float& aSFFloat, bool* hasComma )
{
    if( NULL != hasComma )
        *hasComma = false;

    aSFFloat = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    bool lComma = false;
    std::string tmp;

    if( !ReadGlob( tmp, &lComma ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    std::istringstream istr;
    istr.str( tmp );

    istr >> aSFFloat;
    tmp.clear();
    istr >> tmp;

    if( !tmp.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] invalid character in SFFloat";
        m_error = ostr.str();

        return false;
    }

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadSFInt( int& aSFInt32, bool* hasComma )
{
    if( NULL != hasComma )
        *hasComma = false;

    aSFInt32 = 0;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    bool lComma = false;
    std::string tmp;

    if( !ReadGlob( tmp, &lComma ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline<< ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( std::string::npos != tmp.find( "0x" ) )
    {
        // Rules: "0x" + "0-9, A-F" - VRML is case sensitive but in
        // this instance we do no enforce case.
        std::stringstream sstr;
        sstr << std::hex << tmp;
        sstr >> aSFInt32;
        return true;
    }

    std::istringstream istr;
    istr.str( tmp );

    istr >> aSFInt32;
    tmp.clear();
    istr >> tmp;

    if( !tmp.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
        ostr << " * [INFO] invalid character in SFInt";
        m_error = ostr.str();

        return false;
    }

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadSFRotation( WRLROTATION& aSFRotation, bool* hasComma )
{
    if( NULL != hasComma )
        *hasComma = false;

    aSFRotation.x = 0.0;
    aSFRotation.y = 0.0;
    aSFRotation.z = 1.0;
    aSFRotation.w = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    bool lComma = false;
    std::string tmp;

    float trot[4];

    for( int i = 0; i < 4; ++i )
    {
        if( !ReadGlob( tmp, &lComma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( lComma && i != 3 )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in space delimited quartet";
            m_error = ostr.str();

            return false;
        }

        std::istringstream istr;
        istr.str( tmp );

        istr >> trot[i];
        tmp.clear();
        istr >> tmp;

        if( !tmp.empty() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] invalid character in space delimited quartet";
            m_error = ostr.str();

            return false;
        }

    }

    aSFRotation.x = trot[0];
    aSFRotation.y = trot[1];
    aSFRotation.z = trot[2];
    aSFRotation.w = trot[3];

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadSFVec2f( WRLVEC2F& aSFVec2f, bool* hasComma )
{
    if( NULL != hasComma )
        *hasComma = false;

    aSFVec2f.x = 0.0;
    aSFVec2f.y = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    bool lComma = false;
    std::string tmp;

    float tcol[2];

    for( int i = 0; i < 2; ++i )
    {
        if( !ReadGlob( tmp, &lComma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( lComma && i != 1 )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in space delimited pair";
            m_error = ostr.str();

            return false;
        }

        std::istringstream istr;
        istr.str( tmp );

        istr >> tcol[i];
        tmp.clear();
        istr >> tmp;

        if( !tmp.empty() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] invalid character in space delimited pair";
            m_error = ostr.str();

            return false;
        }

    }

    aSFVec2f.x = tcol[0];
    aSFVec2f.y = tcol[1];

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadSFVec3f( WRLVEC3F& aSFVec3f, bool* hasComma )
{
    if( NULL != hasComma )
        *hasComma = false;

    aSFVec3f.x = 0.0;
    aSFVec3f.y = 0.0;
    aSFVec3f.z = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    bool lComma = false;
    std::string tmp;

    float tcol[3];

    for( int i = 0; i < 3; ++i )
    {
        if( !ReadGlob( tmp, &lComma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( lComma && i != 2 )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in space delimited triplet";
            m_error = ostr.str();

            return false;
        }

        std::istringstream istr;
        istr.str( tmp );

        istr >> tcol[i];
        tmp.clear();
        istr >> tmp;

        if( !tmp.empty() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] invalid character in space delimited triplet";
            m_error = ostr.str();

            return false;
        }

    }

    aSFVec3f.x = tcol[0];
    aSFVec3f.y = tcol[1];
    aSFVec3f.z = tcol[2];

    if( NULL != hasComma )
        *hasComma = lComma;

    return true;
}


bool WRLPROC::ReadMFString( std::vector< std::string >& aMFString )
{
    aMFString.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    std::string lstr;

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadString( lstr ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( m_linepos >= m_buf.size() && !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_linepos] )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFString.push_back( lstr );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !ReadString( lstr ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_linepos] )
            ++m_linepos;

        aMFString.push_back( lstr );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] problems encountered while reading list";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFColor( std::vector< WRLVEC3F >& aMFColor )
{
    aMFColor.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    WRLVEC3F lcolor;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFColor( lcolor, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFColor.push_back( lcolor );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFColor( lcolor, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFColor.push_back( lcolor );

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFFloat( std::vector< float >& aMFFloat )
{
    aMFFloat.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    float temp;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFFloat( temp, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFFloat.push_back( temp );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFFloat( temp, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFFloat.push_back( temp );

        if( !lcomma )
        {
            if( !EatSpace() )
                return false;

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFInt( std::vector< int >& aMFInt32 )
{
    aMFInt32.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    int temp;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFInt( temp, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFInt32.push_back( temp );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFInt( temp, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFInt32.push_back( temp );

        if( !lcomma )
        {
            if( !EatSpace() )
                return false;

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFRotation( std::vector< WRLROTATION >& aMFRotation )
{
    aMFRotation.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    WRLROTATION lrot;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFRotation( lrot, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFRotation.push_back( lrot );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFRotation( lrot, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFRotation.push_back( lrot );

        if( !lcomma )
        {
            if( !EatSpace() )
                return false;

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFVec2f( std::vector< WRLVEC2F >& aMFVec2f )
{
    aMFVec2f.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    WRLVEC2F lvec2f;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFVec2f( lvec2f, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFVec2f.push_back( lvec2f );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFVec2f( lvec2f, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFVec2f.push_back( lvec2f );

        if( !lcomma )
        {
            if( !EatSpace() )
                return false;

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::ReadMFVec3f( std::vector< WRLVEC3F >& aMFVec3f )
{
    aMFVec3f.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_linepos;

    WRLVEC3F lvec3f;
    bool lcomma = false;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_linepos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_linepos] != '[' )
    {
        if( !ReadSFVec3f( lvec3f, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( !lcomma )
        {
            if( !EatSpace() )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
                ostr << " * [INFO] could not check characters after the string";
                m_error = ostr.str();

                return false;
            }

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma encountered in undelimited list";
            m_error = ostr.str();

            return false;
        }

        aMFVec3f.push_back( lvec3f );
        return true;
    }

    ++m_linepos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_linepos] )
            break;

        if( !ReadSFVec3f( lvec3f, &lcomma ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFVec3f.push_back( lvec3f );

        if( !lcomma )
        {
            if( !EatSpace() )
                return false;

            if( ']' == m_buf[m_linepos] )
                break;

            if( ',' == m_buf[m_linepos] )
                lcomma = true;
        }

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_linepos] )
            break;

        if( !lcomma )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_linepos << "\n";
            ostr << " * [INFO] comma missing in delimited list";
            m_error = ostr.str();

            return false;
        }
    }

    ++m_linepos;
    return true;
}


bool WRLPROC::eof( void )
{
    if( !m_file.is_open() )
        return false;

    return m_file.eof();
}


std::string WRLPROC::GetError( void )
{
    return m_error;
}


bool WRLPROC::GetFilePosData( size_t& line, size_t& column )
{
    if( !m_file.is_open() )
    {
        line = 0;
        column = 0;
        return false;
    }

    line = m_fileline;
    column = m_linepos;

    return true;
}


std::string WRLPROC::GetFileName( void )
{
    return m_filename;
}


char WRLPROC::Peek( void )
{
    if( !m_file.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [BUG] no open file";
        m_error = ostr.str();

        return '\0';
    }

    if( !EatSpace() )
    {
        if( m_error.empty() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed to read data from file\n";
            m_error = ostr.str();
        }

        return false;
    }

    return m_buf[m_linepos];
}


void WRLPROC::Pop( void )
{
    if( m_linepos < m_buf.size() )
        ++m_linepos;

    return;
}
