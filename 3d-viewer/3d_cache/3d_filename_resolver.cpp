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
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>

#include "3d_filename_resolver.h"

// configuration file version
#define CFGFILE_VERSION 1
#define S3D_RESOLVER_CONFIG wxT( "3Dresolver.cfg" )

// flag bits used to track different one-off messages to users
#define ERRFLG_ALIAS    (1)
#define ERRFLG_RELPATH  (2)
#define ERRFLG_ENVPATH  (4)

#define MASK_3D_RESOLVER "3D_RESOLVER"


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
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] changed project dir to ";
        ostr << m_Paths.front().m_pathexp.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
    } while( 0 );
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

    // add an entry for the default search path; at this point
    // we cannot set a sensible default so we use an empty string.
    // the user may change this later with a call to SetProjectDir()

    S3D_ALIAS lpath;
    lpath.m_alias = _( "(DEFAULT)" );
    lpath.m_pathvar = _( "${PROJDIR}" );
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
    wxLogTrace( MASK_3D_RESOLVER, " * [3D model] search paths:\n" );
    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        wxLogTrace( MASK_3D_RESOLVER, "   + '%s'\n", (*sPL).m_pathexp.ToUTF8() );
        ++sPL;
    }
#endif

    return true;
}


bool S3D_FILENAME_RESOLVER::UpdatePathList( std::vector< S3D_ALIAS >& aPathList )
{
    while( m_Paths.size() > 2 )
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

        if( !( m_errflags & ERRFLG_ENVPATH ) )
        {
            m_errflags |= ERRFLG_ENVPATH;
            wxString errmsg = _( "[3D File Resolver] No such path; ensure the environment var is defined" );
            errmsg.append( "\n" );
            errmsg.append( tname );
            wxLogMessage( "%s\n", errmsg.ToUTF8() );
        }

        return wxEmptyString;
    }

    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    // check the path relative to the current project directory;
    // note: this is not necessarily the same as the current working
    // directory, which has already been checked
    if( !sPL->m_pathexp.empty() )
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
    }

    ++sPL;  // skip to item 2: KISYS3DMOD

    // check if the path is relative to KISYS3DMOD but lacking
    // the "KISYS3DMOD:" alias tag
    if( !sPL->m_pathexp.empty() )
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
    }

    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( tname, alias, relpath ) )
    {
        if( !( m_errflags & ERRFLG_RELPATH ) )
        {
            m_errflags |= ERRFLG_RELPATH;
            wxString errmsg = _( "[3D File Resolver] No such path; ensure KISYS3DMOD is correctly defined" );
            errmsg.append( "\n" );
            errmsg.append( tname );
            wxLogTrace( MASK_3D_RESOLVER, "%s\n", errmsg.ToUTF8() );
        }

        return wxEmptyString;
    }

    while( sPL != ePL )
    {
        if( !sPL->m_alias.Cmp( alias ) && !sPL->m_pathexp.empty() )
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

    if( !( m_errflags & ERRFLG_ALIAS ) )
    {
        m_errflags |= ERRFLG_ALIAS;
        wxString errmsg = _( "[3D File Resolver] No such path; ensure the path alias is defined" );
        errmsg.append( "\n" );
        errmsg.append( tname.substr( 1 ) );
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", errmsg.ToUTF8() );
    }

    return wxEmptyString;
}


bool S3D_FILENAME_RESOLVER::addPath( const S3D_ALIAS& aPath )
{
    if( aPath.m_alias.empty() || aPath.m_pathvar.empty() )
        return false;

    S3D_ALIAS tpath = aPath;
    tpath.m_duplicate = false;

    #ifdef _WIN32
    while( tpath.m_pathvar.EndsWith( wxT( "\\" ) ) )
        tpath.m_pathvar.erase( tpath.m_pathvar.length() - 1 );
    #else
    while( tpath.m_pathvar.EndsWith( wxT( "/" ) ) &&  tpath.m_pathvar.length() > 1 )
        tpath.m_pathvar.erase( tpath.m_pathvar.length() - 1 );
    #endif

    wxFileName path( tpath.m_pathvar, wxT( "" ) );
    path.Normalize();

    if( !path.DirExists() )
    {
        // suppress the message if the missing pathvar is the
        // legacy KISYS3DMOD variable
        if( aPath.m_pathvar.compare( wxT( "${KISYS3DMOD}" ) ) )
        {
            wxString msg = _T( "The given path does not exist" );
            msg.append( wxT( "\n" ) );
            msg.append( tpath.m_pathvar );
            wxMessageBox( msg, _T( "3D model search path" ) );
        }

        tpath.m_pathexp.clear();
    }
    else
    {
        tpath.m_pathexp = path.GetFullPath();

        #ifdef _WIN32
        while( tpath.m_pathexp.EndsWith( wxT( "\\" ) ) )
        tpath.m_pathexp.erase( tpath.m_pathexp.length() - 1 );
        #else
        while( tpath.m_pathexp.EndsWith( wxT( "/" ) ) &&  tpath.m_pathexp.length() > 1 )
            tpath.m_pathexp.erase( tpath.m_pathexp.length() - 1 );
        #endif
    }

    wxString pname = path.GetPath();
    std::list< S3D_ALIAS >::iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        // aliases with the same m_pathvar are forbidden and the
        // user must be forced to fix the problem in order to
        // obtain good filename resolution
        if( !sPL->m_pathvar.empty() && !tpath.m_pathvar.empty()
            && !tpath.m_pathvar.Cmp( sPL->m_pathvar ) )
        {
            wxString msg = _( "This alias: " );
            msg.append( tpath.m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _( "This path: " ) );
            msg.append( tpath.m_pathvar );
            msg.append( wxT( "\n" ) );
            msg.append( _( "Existing alias: " ) );
            msg.append( sPL->m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _( "Existing path: " ) );
            msg.append( sPL->m_pathvar );
            wxMessageBox( msg, _( "Bad alias (duplicate path)" ) );

            return false;
        }

        // aliases with the same m_pathexp are acceptable (one or both
        // aliases being tested may be expanded variables) but when shortening
        // names the preference is for (a) a fully specified path in m_pathvar
        // then (b) the more senior alias in the list
        if( !sPL->m_pathexp.empty() && !tpath.m_pathexp.empty() )
        {
            if( !tpath.m_pathexp.Cmp( sPL->m_pathexp ) )
            {
                wxString msg = _( "This alias: " );
                msg.append( tpath.m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _( "Existing alias: " ) );
                msg.append( sPL->m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _( "This path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _( "Existing path: " ) );
                msg.append( sPL->m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _( "This full path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _( "Existing full path: " ) );
                msg.append( sPL->m_pathexp );
                wxMessageBox( msg, _( "Bad alias (duplicate path)" ) );

                if( tpath.m_pathvar.StartsWith( wxT( "${" ) ) )
                    tpath.m_duplicate = true;
                else if( sPL->m_pathvar.StartsWith( wxT( "${" ) ) )
                    sPL->m_duplicate = true;

            }

            if( ( tpath.m_pathexp.find( sPL->m_pathexp ) != wxString::npos
                || sPL->m_pathexp.find( tpath.m_pathexp ) != wxString::npos )
                && tpath.m_pathexp.Cmp( sPL->m_pathexp ) )
            {
                wxString msg = _( "This alias: " );
                msg.append( tpath.m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _( "This path: " ) );
                msg.append( tpath.m_pathexp );
                msg.append( wxT( "\n" ) );
                msg.append( _( "Existing alias: " ) );
                msg.append( sPL->m_alias );
                msg.append( wxT( "\n" ) );
                msg.append( _( "Existing path: " ) );
                msg.append( sPL->m_pathexp );
                wxMessageBox( msg, _( "Bad alias (common path)" ) );

                return false;
            }
        }

        if( !tpath.m_alias.Cmp( sPL->m_alias ) )
        {
            wxString msg = _( "Alias: " );
            msg.append( tpath.m_alias );
            msg.append( wxT( "\n" ) );
            msg.append( _( "This path: " ) );
            msg.append( tpath.m_pathvar );
            msg.append( wxT( "\n" ) );
            msg.append( _( "Existing path: " ) );
            msg.append( sPL->m_pathvar );
            wxMessageBox( msg, _( "Bad alias (duplicate name)" ) );

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
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "3D configuration directory is unknown" );
        ostr << " * " << errmsg.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
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
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "no 3D configuration file" );
        ostr << " * " << errmsg.ToUTF8() << " '";
        ostr << cfgname.ToUTF8() << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "Could not open configuration file" );
        ostr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        return false;
    }

    int lineno = 0;
    S3D_ALIAS al;
    al.m_duplicate = false;
    size_t idx;
    int vnum = 0;           // version number

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

        if( 1 == lineno && cfgLine.compare( 0, 2, "#V" ) == 0 )
        {
            // extract the version number and parse accordingly
            if( cfgLine.size() > 2 )
            {
                std::istringstream istr;
                istr.str( cfgLine.substr( 2 ) );
                istr >> vnum;
            }

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

    if( vnum < CFGFILE_VERSION )
        writePathList();

    if( m_Paths.size() != nitems )
        return true;

    return false;
}


bool S3D_FILENAME_RESOLVER::writePathList( void )
{
    if( m_ConfigDir.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "3D configuration directory is unknown" );
        ostr << " * " << errmsg.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        wxMessageBox( errmsg, _( "Write 3D search path list" ) );

        return false;
    }

    wxFileName cfgpath( m_ConfigDir, S3D_RESOLVER_CONFIG );
    wxString cfgname = cfgpath.GetFullPath();
    std::ofstream cfgFile;

    if( m_Paths.empty() || 1 == m_Paths.size() )
    {
        wxMessageDialog md( NULL,
            _( "3D search path list is empty;\ncontinue to write empty file?" ),
            _( "Write 3D search path list" ), wxYES_NO );

        if( md.ShowModal() == wxID_YES )
        {
            cfgFile.open( cfgname.ToUTF8(), std::ios_base::trunc );

            if( !cfgFile.is_open() )
            {
                wxMessageBox( _( "Could not open configuration file" ),
                    _( "Write 3D search path list" ) );

                return false;
            }

            cfgFile << "#V" << CFGFILE_VERSION << "\n";
            cfgFile.close();
            return true;
        }

        return false;
    }

    cfgFile.open( cfgname.ToUTF8(), std::ios_base::trunc );

    if( !cfgFile.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "Could not open configuration file" );
        ostr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        wxMessageBox( errmsg, _( "Write 3D search path list" ) );

        return false;
    }

    cfgFile << "#V" << CFGFILE_VERSION << "\n";
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
        wxMessageBox( _( "Problems writing configuration file" ),
                      _( "Write 3D search path list" ) );

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
    // and KISYS3DMOD directory
    for( int i = 0; i < 2 && sL != eL; ++i )
    {
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
    }

    while( sL != eL )
    {
        // undefined paths and duplicates do not participate
        // in the file name shortening procedure
        if( sL->m_pathexp.empty() || sL->m_duplicate )
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

    if( aIndex >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "bad Hollerith string on line" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "missing opening quote mark in config file" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "invalid entry (unexpected end of line)" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    std::string tnum;

    while( aString[i2] >= '0' && aString[i2] <= '9' )
        tnum.append( 1, aString[i2++] );

    if( tnum.empty() || aString[i2++] != ':' )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "bad Hollerith string on line" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    std::istringstream istr;
    istr.str( tnum );
    size_t nchars;
    istr >> nchars;

    if( (i2 + nchars) >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "invalid entry (unexpected end of line)" );
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    if( nchars > 0 )
    {
        aResult = aString.substr( i2, nchars );
        i2 += nchars;
    }

    if( aString[i2] != '"' )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "missing closing quote mark in config file";
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    aIndex = i2 + 1;
    return true;
}


bool S3D_FILENAME_RESOLVER::ValidateFileName( const wxString& aFileName, bool& hasAlias )
{
    // Rules:
    // 1. The generic form of an aliased 3D relative path is:
    //    ALIAS:relative/path
    // 2. ALIAS is a UTF string excluding wxT( "{}[]()%~<>\"='`;:.,&?/\\|$" )
    // 3. The relative path must be a valid relative path for the platform
    hasAlias = false;

    if( aFileName.empty() )
        return false;

    wxString filename = aFileName;
    wxString lpath;
    size_t pos0 = aFileName.find( ':' );

    // ensure that the file separators suit the current platform
    #ifdef __WINDOWS__
    filename.Replace( wxT( "/" ), wxT( "\\" ) );

    // if we see the :\ pattern then it must be a drive designator
    if( pos0 != wxString::npos )
    {
        size_t pos1 = aFileName.find( wxT( ":\\" ) );

        if( pos1 != wxString::npos && ( pos1 != pos0 || pos1 != 1 ) )
            return false;

        // if we have a drive designator then we have no alias
        if( pos1 != wxString::npos )
            pos0 = wxString::npos;
    }
    #else
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
    #endif

    // names may not end with ':'
    if( pos0 == aFileName.length() -1 )
        return false;

    if( pos0 != wxString::npos )
    {
        // ensure the alias component is not empty
        if( pos0 == 0 )
            return false;

        lpath = filename.substr( 0, pos0 );

        if( wxString::npos != lpath.find_first_of( wxT( "{}[]()%~<>\"='`;:.,&?/\\|$" ) ) )
            return false;

        hasAlias = true;

        lpath = aFileName.substr( pos0 + 1 );
    }
    else
    {
        lpath = aFileName;
    }

    if( wxString::npos != lpath.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}
