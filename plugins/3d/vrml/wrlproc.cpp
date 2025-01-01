/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include <iostream>
#include <sstream>
#include <wx/string.h>
#include <wx/log.h>
#include "wrlproc.h"
#include <wx_filename.h>

#define GETLINE                                                                                \
    do                                                                                         \
    {                                                                                          \
        try                                                                                    \
        {                                                                                      \
            char* cp = m_file->ReadLine();                                                     \
                                                                                               \
            if( nullptr == cp )                                                                \
            {                                                                                  \
                m_eof = true;                                                                  \
                m_buf.clear();                                                                 \
            }                                                                                  \
            else                                                                               \
            {                                                                                  \
                m_buf = cp;                                                                    \
                m_bufpos = 0;                                                                  \
            }                                                                                  \
                                                                                               \
            m_fileline = m_file->LineNumber();                                                 \
        }                                                                                      \
        catch( ... )                                                                           \
        {                                                                                      \
            m_error = " * [INFO] input line too long";                                         \
            m_eof = true;                                                                      \
            m_buf.clear();                                                                     \
        }                                                                                      \
    } while( 0 )


WRLPROC::WRLPROC( LINE_READER* aLineReader )
{
    m_fileVersion = WRLVERSION::VRML_INVALID;
    m_eof = false;
    m_fileline = 0;
    m_bufpos = 0;
    m_file = aLineReader;

    if( nullptr == aLineReader )
    {
        m_eof = true;
        return;
    }

    m_error.clear();
    wxString tname = m_file->GetSource();
    m_filename = tname.ToUTF8();
    wxFileName fn( tname );

    if( fn.IsRelative() )
        fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    m_filedir = fn.GetPathWithSep().ToUTF8();

    m_buf.clear();
    GETLINE;

    if( m_eof )
        return;

    if( m_buf.compare( 0, 16, "#VRML V1.0 ascii"  ) == 0 )
    {
        m_fileVersion = WRLVERSION::VRML_V1;
        // nothing < 0x20, and no:
        // single or double quote
        // backslash
        // curly brace
        // plus
        // period
        m_badchars = "\"'\\{}+.";
        return;
    }

    if( m_buf.compare( 0, 15, "#VRML V2.0 utf8" ) == 0 )
    {
        m_fileVersion = WRLVERSION::VRML_V2;
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
        // NOTE: badchars should include '-' but due to my bad model naming scheme
        // in the VRML model generator, I have allowed '-'. Other VRML parsers seem to
        // accept '-'. FreeCAD produces names with '+' in them so '+' has been allowed
        // as well; '+' is not even valid for VRML1.
        //m_badchars = "'\"#,.+-[]\\{}";
        m_badchars = "'\"#,.[]\\{}";
        return;
    }

    m_buf.clear();
    m_fileVersion = WRLVERSION::VRML_INVALID;
    m_eof = true;

    m_error = "not a valid VRML file: '";
    m_error.append( m_filename );
    m_error.append( 1, '\'' );
    m_badchars.clear();
}


WRLPROC::~WRLPROC()
{
}


bool WRLPROC::getRawLine( void )
{
    m_error.clear();

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    if( m_bufpos >= m_buf.size() )
        m_buf.clear();

    if( !m_buf.empty() )
        return true;

    if( m_eof )
        return false;

    GETLINE;

    if( m_eof && m_buf.empty() )
        return false;

    // strip the EOL characters
    while( !m_buf.empty() && ( *m_buf.rbegin() == '\r' || *m_buf.rbegin() == '\n' ) )
        m_buf.erase( --m_buf.end() );

    if( WRLVERSION::VRML_V1 == m_fileVersion && !m_buf.empty() )
    {
        std::string::iterator sS = m_buf.begin();
        std::string::iterator eS = m_buf.end();

        while( sS != eS )
        {
            if( ( ( *sS ) & 0x80 ) )
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
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    if( m_bufpos >= m_buf.size() )
        m_buf.clear();

RETRY:
    while( m_buf.empty() && !m_eof )
        getRawLine();

    // buffer may be empty if we have reached EOF or encountered IO errors
    if( m_buf.empty() )
        return false;

    // eliminate leading white space (including control characters and comments)
    while( m_bufpos < m_buf.size() )
    {
        if( m_buf[m_bufpos] > 0x20 )
            break;

        ++m_bufpos;
    }

    if( m_bufpos == m_buf.size() || '#' == m_buf[m_bufpos] )
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


const char* WRLPROC::GetParentDir( void )
{
    if( m_filedir.empty() )
        return nullptr;

    return m_filedir.c_str();
}


bool WRLPROC::ReadGlob( std::string& aGlob )
{
    aGlob.clear();

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    size_t ssize = m_buf.size();

    while( m_bufpos < ssize && m_buf[m_bufpos] > 0x20 )
    {
        if( ',' == m_buf[m_bufpos] )
        {
            // the comma is a special instance of blank space
            ++m_bufpos;
            return true;
        }

        if( '{' == m_buf[m_bufpos] || '}' == m_buf[m_bufpos]
            || '[' == m_buf[m_bufpos] || ']' == m_buf[m_bufpos] )
            return true;

        aGlob.append( 1, m_buf[m_bufpos++] );
    }

    return true;
}


bool WRLPROC::ReadName( std::string& aName )
{
    aName.clear();

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    size_t ssize = m_buf.size();

    while( m_bufpos < ssize && m_buf[m_bufpos] > 0x20 )
    {
        if( '[' == m_buf[m_bufpos] || '{' == m_buf[m_bufpos]
            || ']' == m_buf[m_bufpos] || '}' == m_buf[m_bufpos]
            || '.' == m_buf[m_bufpos] || '#' == m_buf[m_bufpos]
            || ',' == m_buf[m_bufpos] )
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
                ostr << " * [INFO] line " << m_fileline << ", column " << m_bufpos;
                ostr << " -- invalid name";
                m_error = ostr.str();

                return false;
            }
        }

        if( m_badchars.find( m_buf[m_bufpos] ) != std::string::npos )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << m_fileline << ", column " << m_bufpos;
            ostr << " -- invalid character in name";
            m_error = ostr.str();

            return false;
        }

        if( aName.empty() && m_buf[m_bufpos] >= '0' && m_buf[m_bufpos] <= '9' )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << m_fileline << ", column " << m_bufpos;
            ostr << " -- name must not start with a digit";
            m_error = ostr.str();

            return false;
        }

        aName.append( 1, m_buf[m_bufpos++] );
    }

    return true;
}


bool WRLPROC::DiscardNode( void )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    if( !EatSpace() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( '{' != m_buf[m_bufpos] )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] expecting character '{' at line " << m_fileline;
        ostr << ", column " << m_bufpos;
        m_error = ostr.str();

        wxLogTrace( traceVrmlPlugin, "%s\n", m_error.c_str() );

        return false;
    }

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    ++m_bufpos;
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        // comments must be skipped
        if( '#' == m_buf[m_bufpos] )
        {
            m_bufpos = 0;
            m_buf.clear();
            continue;
        }

        if( '{' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            ++lvl;
            continue;
        }

        if( '}' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            --lvl;
            continue;
        }

        // note: if we have a ']' we must skip it and test the next non-blank character;
        // this ensures that we don't miss a '}' in cases where the braces are not
        // separated by space. if we had proceeded to ReadGlob() we could have had a problem.
        if( ']' == m_buf[m_bufpos] || '[' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            continue;
        }

        // strings are handled as a special case since they may contain
        // control characters and braces
        if( '"' == m_buf[m_bufpos] )
        {
            if( !ReadString( tmp ) )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }
    }

    return true;
}


bool WRLPROC::DiscardList( void )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    if( !EatSpace() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( '[' != m_buf[m_bufpos] )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] expecting character '[' at line " << m_fileline;
        ostr << ", column " << m_bufpos;
        m_error = ostr.str();

        return false;
    }

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    ++m_bufpos;
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        // comments must be skipped
        if( '#' == m_buf[m_bufpos] )
        {
            m_bufpos = 0;
            m_buf.clear();
            continue;
        }

        if( '[' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            ++lvl;
            continue;
        }

        if( ']' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            --lvl;
            continue;
        }

        // note: if we have a '}' we must skip it and test the next non-blank character;
        // this ensures that we don't miss a ']' in cases where the braces are not
        // separated by space. if we had proceeded to ReadGlob() we could have had a problem.
        if( '}' == m_buf[m_bufpos] || '{' == m_buf[m_bufpos] )
        {
            ++m_bufpos;
            continue;
        }

        // strings are handled as a special case since they may contain
        // control characters and braces
        if( '"' == m_buf[m_bufpos] )
        {
            if( !ReadString( tmp ) )
            {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
                ostr << " * [INFO] failed on file '" << m_filename << "'\n";
                ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
                ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    // remember the line number in case of errors
    size_t ifline = m_fileline;

    while( true )
    {
        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << "invalid VRML file; expecting string at line " << ifline <<
                    " but found nothing";
            m_error = ostr.str();

            return false;
        }

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( WRLVERSION::VRML_V2 == m_fileVersion && '"' != m_buf[m_bufpos] )
    {
        m_error = "invalid VRML2 file (string not quoted)";
        return false;
    }

    ifline = m_fileline;

    if( '"' != m_buf[m_bufpos] )
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
        ++m_bufpos;

        if( m_bufpos >= m_buf.size() )
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

        if( '\\' == m_buf[m_bufpos] )
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
        else if( '"' ==  m_buf[m_bufpos] )
        {
            if( isesc )
                aSFString.append( 1, '"' );
            else
                break;
        }
        else
        {
            aSFString.append( 1, m_buf[m_bufpos] );
        }

        // ensure that the backslash escape cannot extend beyond the first character
        isesc = false;
    }

    ++m_bufpos;

    return true;
}


bool WRLPROC::ReadSFBool( bool& aSFBool )
{
    if( !EatSpace() )
        return false;

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;
    std::string tmp;

    if( !ReadGlob( tmp ) )
        return false;

    if( !tmp.compare( "0" ) )
    {
        aSFBool = false;
    }
    else if( !tmp.compare( "1" ) )
    {
        aSFBool = true;
    }
    else if( !tmp.compare( "TRUE" ) )
    {
        aSFBool = true;
    }
    else if( !tmp.compare( "FALSE" ) )
    {
        aSFBool = false;
    }
    else
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] expected one of 0, 1, TRUE, FALSE but got '" << tmp << "'\n";
        m_error = ostr.str();

        return false;
    }

    return true;
}


bool WRLPROC::ReadSFColor( WRLVEC3F& aSFColor )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !ReadSFVec3f( aSFColor ) )
        return false;

    if( aSFColor.x < 0.0 || aSFColor.x > 1.0 || aSFColor.y < 0.0 || aSFColor.y > 1.0
        || aSFColor.z < 0.0 || aSFColor.z > 1.0 )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] invalid RGB value in color triplet";
        m_error = ostr.str();

        return false;
    }

    return true;
}


bool WRLPROC::ReadSFFloat( float& aSFFloat )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    aSFFloat = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string tmp;

    if( !ReadGlob( tmp ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    std::istringstream istr;
    istr.str( tmp );
    istr >> aSFFloat;

    if( istr.fail() || !istr.eof() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] invalid character in SFFloat";
        m_error = ostr.str();

        return false;
    }

    return true;
}


bool WRLPROC::ReadSFInt( int& aSFInt32 )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    aSFInt32 = 0;
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string tmp;

    if( !ReadGlob( tmp ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline<< ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] " << m_error;
        m_error = ostr.str();

        return false;
    }

    if( tmp.find( "0x" ) != std::string::npos )
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

    if( istr.fail() || !istr.eof() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << " * [INFO] failed on file '" << m_filename << "'\n";
        ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
        ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
        ostr << " * [INFO] invalid character in SFInt";
        m_error = ostr.str();

        return false;
    }

    return true;
}


bool WRLPROC::ReadSFRotation( WRLROTATION& aSFRotation )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    aSFRotation.x = 0.0;
    aSFRotation.y = 0.0;
    aSFRotation.z = 1.0;
    aSFRotation.w = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string tmp;
    float trot[4];

    for( int i = 0; i < 4; ++i )
    {
        if( !ReadGlob( tmp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        std::istringstream istr;
        istr.str( tmp );
        istr >> trot[i];

        if( istr.fail() || !istr.eof() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] invalid character in space delimited quartet";
            m_error = ostr.str();

            return false;
        }

    }

    aSFRotation.x = trot[0];
    aSFRotation.y = trot[1];
    aSFRotation.z = trot[2];
    aSFRotation.w = trot[3];

    return true;
}


bool WRLPROC::ReadSFVec2f( WRLVEC2F& aSFVec2f )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    aSFVec2f.x = 0.0;
    aSFVec2f.y = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string tmp;

    float tcol[2];

    for( int i = 0; i < 2; ++i )
    {
        if( !ReadGlob( tmp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        std::istringstream istr;
        istr.str( tmp );
        istr >> tcol[i];

        if( istr.fail() || !istr.eof() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] invalid character in space delimited pair";
            m_error = ostr.str();

            return false;
        }

    }

    aSFVec2f.x = tcol[0];
    aSFVec2f.y = tcol[1];

    return true;
}


bool WRLPROC::ReadSFVec3f( WRLVEC3F& aSFVec3f )
{
    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    aSFVec3f.x = 0.0;
    aSFVec3f.y = 0.0;
    aSFVec3f.z = 0.0;

    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string tmp;

    float tcol[3];

    for( int i = 0; i < 3; ++i )
    {
        if( !ReadGlob( tmp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        // ignore any commas
        if( !EatSpace() )
            return false;

        if( ',' == m_buf[m_bufpos] )
            Pop();

        std::istringstream istr;
        istr.str( tmp );
        istr >> tcol[i];

        if( istr.fail() || !istr.eof() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] invalid character in space delimited triplet";
            m_error = ostr.str();

            return false;
        }

    }

    aSFVec3f.x = tcol[0];
    aSFVec3f.y = tcol[1];
    aSFVec3f.z = tcol[2];

    return true;
}


bool WRLPROC::ReadMFString( std::vector< std::string >& aMFString )
{
    aMFString.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    std::string lstr;

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadString( lstr ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        if( m_bufpos >= m_buf.size() && !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFString.push_back( lstr );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !ReadString( lstr ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFString.push_back( lstr );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] problems encountered while reading list";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFColor( std::vector< WRLVEC3F >& aMFColor )
{
    aMFColor.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    WRLVEC3F lcolor;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFColor( lcolor ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFColor.push_back( lcolor );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFColor( lcolor ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFColor.push_back( lcolor );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();

    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFFloat( std::vector< float >& aMFFloat )
{
    aMFFloat.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    float temp;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFFloat( temp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFFloat.push_back( temp );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFFloat( temp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFFloat.push_back( temp );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();
    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFInt( std::vector< int >& aMFInt32 )
{
    aMFInt32.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    int temp;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFInt( temp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFInt32.push_back( temp );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFInt( temp ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFInt32.push_back( temp );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();

    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFRotation( std::vector< WRLROTATION >& aMFRotation )
{
    aMFRotation.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    WRLROTATION lrot;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFRotation( lrot ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFRotation.push_back( lrot );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFRotation( lrot ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFRotation.push_back( lrot );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();

    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFVec2f( std::vector< WRLVEC2F >& aMFVec2f )
{
    aMFVec2f.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    WRLVEC2F lvec2f;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFVec2f( lvec2f ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFVec2f.push_back( lvec2f );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFVec2f( lvec2f ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFVec2f.push_back( lvec2f );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();
    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::ReadMFVec3f( std::vector< WRLVEC3F >& aMFVec3f )
{
    aMFVec3f.clear();
    size_t fileline = m_fileline;
    size_t linepos = m_bufpos;

    if( !m_file )
    {
        m_error = "no open file";
        return false;
    }

    WRLVEC3F lvec3f;

    while( true )
    {
        if( !EatSpace() )
            return false;

        // if the text is the start of a comment block, clear the buffer and loop
        if( '#' == m_buf[m_bufpos] )
            m_buf.clear();
        else
            break;
    }

    if( m_buf[m_bufpos] != '[' )
    {
        if( !ReadSFVec3f( lvec3f ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
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
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( ',' == m_buf[m_bufpos] )
            Pop();

        aMFVec3f.push_back( lvec3f );
        return true;
    }

    ++m_bufpos;

    while( true )
    {
        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( !ReadSFVec3f( lvec3f ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] " << m_error;
            m_error = ostr.str();

            return false;
        }

        aMFVec3f.push_back( lvec3f );

        if( !EatSpace() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
            ostr << " * [INFO] failed on file '" << m_filename << "'\n";
            ostr << " * [INFO] line " << fileline << ", char " << linepos << " -- ";
            ostr << "line " << m_fileline << ", char " << m_bufpos << "\n";
            ostr << " * [INFO] could not check characters after the string";
            m_error = ostr.str();

            return false;
        }

        if( !EatSpace() )
            return false;

        if( ']' == m_buf[m_bufpos] )
            break;

        if( ',' == m_buf[m_bufpos] )
            Pop();

    }

    ++m_bufpos;
    return true;
}


bool WRLPROC::eof( void )
{
    return m_eof;
}


std::string WRLPROC::GetError( void )
{
    return m_error;
}


bool WRLPROC::GetFilePosData( size_t& line, size_t& column )
{
    if( !m_file )
    {
        line = 0;
        column = 0;
        return false;
    }

    line = m_fileline;
    column = m_bufpos;

    return true;
}


std::string WRLPROC::GetFilePosition() const
{
    std::ostringstream retv;

    if( !m_file )
        retv << "no file loaded to provide file position information";
    else
        retv << "at line " << m_fileline << ", column " << m_bufpos;

    return retv.str();
}


std::string WRLPROC::GetFileName( void )
{
    if( !m_file )
    {
        m_error = "no open file";
        return "";
    }

    return std::string( m_file->GetSource().ToUTF8() );
}


char WRLPROC::Peek( void )
{
    if( !m_file )
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

        return '\0';
    }

    return m_buf[m_bufpos];
}


void WRLPROC::Pop( void )
{
    if( m_bufpos < m_buf.size() )
        ++m_bufpos;
}
