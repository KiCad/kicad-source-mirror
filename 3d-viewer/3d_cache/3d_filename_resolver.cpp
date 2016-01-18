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
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>

#include "3d_filename_resolver.h"

#define S3D_RESOLVER_CONFIG wxT( "3Dresolver.cfg" )

// flag bits used to track different one-off messages to users
#define ERRFLG_NODIR    (1)
#define ERRFLG_RELPATH  (2)


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult );


S3D_FILENAME_RESOLVER::S3D_FILENAME_RESOLVER()
{
    m_errflags = 0;
}


bool S3D_FILENAME_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    if( aConfigDir.empty() )
        return false;

    wxFileName cfgdir( aConfigDir, wxT( "" ) );
    cfgdir.Normalize();

    if( false == cfgdir.DirExists() )
        return false;

    m_ConfigDir = cfgdir.GetPath();
    createPathList();

    return true;
}


bool S3D_FILENAME_RESOLVER::SetProjectDir( const wxString& aProjDir, bool* flgChanged )
{
    if( aProjDir.empty() )
        return false;

    wxFileName projdir( aProjDir, wxT( "" ) );
    projdir.Normalize();

    if( false == projdir.DirExists() )
        return false;

    wxString path = projdir.GetPath();

    if( flgChanged )
        *flgChanged = false;

    if( m_Paths.empty() )
    {
        S3D_ALIAS al;
        al.m_alias = _( "(DEFAULT)" );
        al.m_pathvar = _( "${PROJDIR}" );
        al.m_pathexp = path;
        al.m_description = _( "Current project directory" );
        m_Paths.push_back( al );
        m_NameMap.clear();

        if( flgChanged )
            *flgChanged = true;

    }
    else
    {
        if( m_Paths.front().m_pathexp.Cmp( path ) )
        {
            m_Paths.front().m_pathexp = path;
            m_NameMap.clear();

            if( flgChanged )
                *flgChanged = true;

        }
        else
        {
            return true;
        }
    }

#ifdef DEBUG
    std::cout << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cout << " * [INFO] changed project dir to ";
    std::cout << m_Paths.front().m_pathexp.ToUTF8() << "\n";
#endif

    return true;
}


wxString S3D_FILENAME_RESOLVER::GetProjectDir( void )
{
    if( m_Paths.empty() )
        return wxEmptyString;

    return m_Paths.front().m_pathexp;
}


bool S3D_FILENAME_RESOLVER::createPathList( void )
{
    if( !m_Paths.empty() )
        return true;

    wxString kmod;

    // add the current working directory as the first entry by
    // default; since CWD is not necessarily what we really want,
    // the user may change this later with a call to SetProjectDir()

    S3D_ALIAS lpath;
    lpath.m_alias = _( "(DEFAULT)" );
    lpath.m_pathvar = _( "(PROJECT DIR)" );
    lpath.m_pathexp = wxFileName::GetCwd();
    lpath.m_description = _( "Current project directory" );
    m_Paths.push_back( lpath );

    lpath.m_alias = wxT( "KISYS3DMOD" );
    lpath.m_pathvar = wxT( "${KISYS3DMOD}" );
    lpath.m_description = _( "Legacy 3D environment path" );
    addPath( lpath );

    if( !m_ConfigDir.empty() )
        readPathList();

    if( m_Paths.empty() )
        return false;

#ifdef DEBUG
    std::cout << " * [3D model] search paths:\n";
    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        std::cout << "   + '" << (*sPL).m_pathexp.ToUTF8() << "'\n";
        ++sPL;
    }
#endif

    return true;
}


bool S3D_FILENAME_RESOLVER::UpdatePathList( std::vector< S3D_ALIAS >& aPathList )
{
    while( m_Paths.size() > 1 )
        m_Paths.pop_back();

    size_t nI = aPathList.size();

    for( size_t i = 0; i < nI; ++i )
        addPath( aPathList[i] );

    return writePathList();
}


wxString S3D_FILENAME_RESOLVER::ResolvePath( const wxString& aFileName )
{
    if( aFileName.empty() )
        return wxEmptyString;

    if( m_Paths.empty() )
        createPathList();

    // look up the filename in the internal filename map
    std::map< wxString, wxString, S3D::rsort_wxString >::iterator mi;
    mi = m_NameMap.find( aFileName );

    if( mi != m_NameMap.end() )
        return mi->second;

    // first attempt to use the name as specified:
    wxString tname = aFileName;

    #ifdef _WIN32
    // translate from KiCad's internal UNIX-like path to MSWin paths
    tname.Replace( wxT( "/" ), wxT( "\\" ) );
    #endif

    // this case covers full paths and paths relative to
    // the current working directory (which is not necessarily
    // the current project directory)
    if( wxFileName::FileExists( tname ) )
    {
        wxFileName tmp( tname );

        if( tmp.Normalize() )
            tname = tmp.GetFullPath();

        m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

        return tname;
    }

    // at this point aFileName:
    // a. is a legacy ${} shortened name
    // b. an aliased shortened name
    // c. cannot be determined

    if( aFileName.StartsWith( wxT( "${" ) ) )
    {
        wxFileName tmp( aFileName );

        if( tmp.Normalize() )
        {
            tname = tmp.GetFullPath();
            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

            return tname;
        }

        // XXX - no such path - consider showing the user a pop-up message
        return wxEmptyString;
    }

    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    // check the path relative to the current project directory;
    // note: this is not necessarily the same as the current working
    // directory, which has already been checked
    do
    {
        wxFileName fpath( wxFileName::DirName( sPL->m_pathexp ) );
        wxString fullPath = fpath.GetPathWithSep() + tname;

        if( wxFileName::FileExists( fullPath ) )
        {
            wxFileName tmp( fullPath );

            if( tmp.Normalize() )
                tname = tmp.GetFullPath();

            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

            return tname;
        }
    } while( 0 );

    ++sPL;

    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( aFileName, alias, relpath ) )
    {
        // XXX - no such path - consider showing the user a pop-up message
        return wxEmptyString;
    }

    while( sPL != ePL )
    {
        if( !sPL->m_alias.Cmp( alias ) )
        {
            wxFileName fpath( wxFileName::DirName( sPL->m_pathexp ) );
            wxString fullPath = fpath.GetPathWithSep() + relpath;

            if( wxFileName::FileExists( fullPath ) )
            {
                wxFileName tmp( fullPath );

                if( tmp.Normalize() )
                    tname = tmp.GetFullPath();

                m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

                return tname;
            }
        }

        ++sPL;
    }

    // XXX - no such path - consider showing the user a pop-up message
    wxString errmsg = _( "filename could not be resolved" );
    std::cerr << " * [3D Model] " << errmsg.ToUTF8() << " '";
    std::cerr << aFileName.ToUTF8() << "'\n";

    return wxEmptyString;
}


bool S3D_FILENAME_RESOLVER::addPath( const S3D_ALIAS& aPath )
{
    if( aPath.m_alias.empty() || aPath.m_pathvar.empty() )
        return false;

    wxFileName path( aPath.m_pathvar, wxT( "" ) );
    path.Normalize();

    S3D_ALIAS tpath = aPath;

    if( !path.DirExists() )
        tpath.m_pathexp.clear();
    else
        tpath.m_pathexp = path.GetFullPath();

    wxString pname = path.GetPath();
    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( !sPL->m_pathvar.empty() && !tpath.m_pathvar.empty()
            && !tpath.m_pathvar.Cmp( sPL->m_pathvar ) )
        {
            wxString msg = _T( "This alias: " );
            msg.append( tpath.m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _T( "This path: " ) );
            msg.append( tpath.m_pathvar );
            msg.append( wxT( "\n" ) );
            msg.append( _T( "Existing alias: " ) );
            msg.append( sPL->m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _T( "Existing path: " ) );
            msg.append( sPL->m_pathvar );
            wxMessageBox( msg, _T( "Bad alias (duplicate path)" ) );

            return false;
        }

        if( !sPL->m_pathexp.empty() && !tpath.m_pathexp.empty() )
        {
            if( !tpath.m_pathexp.Cmp( sPL->m_pathexp ) )
            {
                wxString msg = _T( "This alias: " );
                msg.append( tpath.m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "Existing alias: " ) );
                msg.append( sPL->m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "This path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "Existing path: " ) );
                msg.append( sPL->m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "This full path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "Existing full path: " ) );
                msg.append( sPL->m_pathexp );
                wxMessageBox( msg, _T( "Bad alias (duplicate path)" ) );

                return false;
            }

            if( tpath.m_pathexp.find( sPL->m_pathexp ) != wxString::npos
                || sPL->m_pathexp.find( tpath.m_pathexp ) != wxString::npos )
            {
                wxString msg = _T( "This alias: " );
                msg.append( tpath.m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "This path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "Existing alias: " ) );
                msg.append( sPL->m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _T( "Existing path: " ) );
                msg.append( sPL->m_pathexp );
                wxMessageBox( msg, _T( "Bad alias (common path)" ) );

                return false;
            }
        }

        if( !tpath.m_alias.Cmp( sPL->m_alias ) )
        {
            wxString msg = _T( "Alias: " );
            msg.append( tpath.m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _T( "This path: " ) );
            msg.append( tpath.m_pathvar );
            msg.append( wxT( "\n" ) );
            msg.append( _T( "Existing path: " ) );
            msg.append( sPL->m_pathvar );
            wxMessageBox( msg, _T( "Bad alias (duplicate name)" ) );

            return false;
        }

        ++sPL;
    }

    // Note: at this point we may still have duplicated paths

    m_Paths.push_back( tpath );
    return true;
}


bool S3D_FILENAME_RESOLVER::readPathList( void )
{
    if( m_ConfigDir.empty() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "3D configuration directory is unknown" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n";
        return false;
    }

    wxFileName cfgpath( m_ConfigDir, S3D_RESOLVER_CONFIG );
    cfgpath.Normalize();
    wxString cfgname = cfgpath.GetFullPath();

    size_t nitems = m_Paths.size();

    std::ifstream cfgFile;
    std::string   cfgLine;

    if( !wxFileName::Exists( cfgname ) )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "no 3D configuration file" );
        std::cerr << " * " << errmsg.ToUTF8() << " '";
        std::cerr << cfgname.ToUTF8() << "'\n";
        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "could not open configuration file" );
        std::cerr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'\n";
        return false;
    }

    int lineno = 0;
    S3D_ALIAS al;
    size_t idx;

    while( cfgFile.good() )
    {
        cfgLine.clear();
        std::getline( cfgFile, cfgLine );
        ++lineno;

        if( cfgLine.empty() )
        {
            if( cfgFile.eof() )
                break;

            continue;
        }

        idx = 0;

        if( !getHollerith( cfgLine, idx, al.m_alias ) )
            continue;

        // never add on KISYS3DMOD from a config file
        if( !al.m_alias.Cmp( wxT( "KISYS3DMOD" ) ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_pathvar ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_description ) )
            continue;

        addPath( al );
    }

    cfgFile.close();

    if( m_Paths.size() != nitems )
        return true;

    return false;
}


bool S3D_FILENAME_RESOLVER::writePathList( void )
{
    if( m_ConfigDir.empty() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "3D configuration directory is unknown" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n";
        wxMessageBox( errmsg, _T( "Write 3D search path list" ) );

        return false;
    }

    wxFileName cfgpath( m_ConfigDir, S3D_RESOLVER_CONFIG );
    wxString cfgname = cfgpath.GetFullPath();
    std::ofstream cfgFile;

    if( m_Paths.empty() || 1 == m_Paths.size() )
    {
        wxMessageDialog md( NULL,
            _T( "3D search path list is empty;\ncontinue to write empty file?" ),
            _T( "Write 3D search path list" ), wxYES_NO );

        if( md.ShowModal() == wxID_YES )
        {
            cfgFile.open( cfgname.ToUTF8(), std::ios_base::trunc );

            if( !cfgFile.is_open() )
            {
                wxMessageBox( _T( "Could not open configuration file" ),
                    _T( "Write 3D search path list" ) );

                return false;
            }

            cfgFile.close();
            return true;
        }

        return false;
    }

    cfgFile.open( cfgname.ToUTF8(), std::ios_base::trunc );

    if( !cfgFile.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "could not open configuration file " );
        std::cerr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'\n";
        wxMessageBox( _T( "Could not open configuration file" ),
                      _T( "Write 3D search path list" ) );

        return false;
    }

    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    // the first entry is the current project dir; we never add the implicit
    // project dir to the path list in the configuration file
    ++sPL;
    std::string tstr;

    while( sPL != ePL )
    {
        // never write the KISYS3DMOD entry
        if( !sPL->m_alias.Cmp( wxT( "KISYS3DMOD") ) )
        {
            ++sPL;
            continue;
        }

        tstr = sPL->m_alias.ToUTF8();
        cfgFile << "\"" << tstr.size() << ":" << tstr << "\",";
        tstr = sPL->m_pathvar.ToUTF8();
        cfgFile << "\"" << tstr.size() << ":" << tstr << "\",";
        tstr = sPL->m_description.ToUTF8();
        cfgFile << "\"" << tstr.size() << ":" << tstr << "\"\n";
        ++sPL;
    }

    bool bad = cfgFile.bad();
    cfgFile.close();

    if( bad )
    {
        wxMessageBox( _T( "Problems writing configuration file" ),
                      _T( "Write 3D search path list" ) );

        return false;
    }

    return true;
}


wxString S3D_FILENAME_RESOLVER::ShortenPath( const wxString& aFullPathName )
{
    wxString fname = aFullPathName;

    if( m_Paths.empty() )
        createPathList();

    std::list< S3D_ALIAS >::const_iterator sL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator eL = m_Paths.end();
    size_t idx;

    // test for files within the current project directory
    if( !sL->m_pathexp.empty() )
    {
        wxFileName fpath( sL->m_pathexp, wxT( "" ) );
        wxString fps = fpath.GetPathWithSep();

        idx = fname.find( fps );

        if( std::string::npos != idx && 0 == idx  )
        {
            fname = fname.substr( fps.size() );
            return fname;
        }
    }

    ++sL;

    while( sL != eL )
    {
        if( sL->m_pathexp.empty() )
        {
            ++sL;
            continue;
        }

        wxFileName fpath( sL->m_pathexp, wxT( "" ) );
        wxString fps = fpath.GetPathWithSep();
        wxString tname;

        idx = fname.find( fps );

        if( std::string::npos != idx && 0 == idx  )
        {
            fname = fname.substr( fps.size() );

            #ifdef _WIN32
            // ensure only the '/' separator is used in the internal name
            fname.Replace( wxT( "\\" ), wxT( "/" ) );
            #endif

            tname = ":";
            tname.append( sL->m_alias );
            tname.append( ":" );
            tname.append( fname );

            return tname;
        }

        ++sL;
    }

#ifdef _WIN32
    // it is strange to convert an MSWin full path to use the
    // UNIX separator but this is done for consistency and can
    // be helpful even when transferring project files from
    // MSWin to *NIX.
    fname.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    return fname;
}



const std::list< S3D_ALIAS >* S3D_FILENAME_RESOLVER::GetPaths( void )
{
    return &m_Paths;
}


bool S3D_FILENAME_RESOLVER::SplitAlias( const wxString& aFileName,
    wxString& anAlias, wxString& aRelPath )
{
    anAlias.clear();
    aRelPath.clear();

    if( !aFileName.StartsWith( wxT( ":" ) ) )
        return false;

    size_t tagpos = aFileName.find( wxT( ":" ), 1 );

    if( wxString::npos ==  tagpos || 1 == tagpos )
        return false;

    if( tagpos + 1 >= aFileName.length() )
        return false;

    anAlias = aFileName.substr( 1, tagpos - 1 );
    aRelPath = aFileName.substr( tagpos + 1 );

    return true;
}


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult )
{
    aResult.clear();

    if( aIndex < 0 || aIndex >= aString.size() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "bad Hollerith string on line" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "missing opening quote mark in config file" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "invalid entry (unexpected end of line)" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    std::string tnum;

    while( aString[i2] >= '0' && aString[i2] <= '9' )
        tnum.append( 1, aString[i2++] );

    if( tnum.empty() || aString[i2++] != ':' )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "bad Hollerith string on line" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    std::istringstream istr;
    istr.str( tnum );
    size_t nchars;
    istr >> nchars;

    if( (i2 + nchars) >= aString.size() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "invalid entry (unexpected end of line)" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    if( nchars > 0 )
    {
        aResult = aString.substr( i2, nchars );
        i2 += nchars;
    }

    if( aString[i2] != '"' )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "missing closing quote mark in config file" );
        std::cerr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'\n";

        return false;
    }

    aIndex = i2 + 1;
    return true;
}
