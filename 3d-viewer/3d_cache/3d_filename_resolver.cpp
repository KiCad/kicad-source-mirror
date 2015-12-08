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
#include <wx/filename.h>
#include <wx/utils.h>

#include "3d_filename_resolver.h"

#define S3D_RESOLVER_CONFIG wxT( "3Dresolver.cfg" )


bool S3D_FILENAME_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    if( aConfigDir.empty() )
        return false;

    wxFileName cfgdir( aConfigDir, "" );
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

    wxFileName projdir( aProjDir, "" );
    projdir.Normalize();

    if( false == projdir.DirExists() )
        return false;

    wxString path = projdir.GetPath();

    if( flgChanged )
        *flgChanged = false;

    if( m_Paths.empty() )
    {
        m_Paths.push_back( path );

        if( flgChanged )
            *flgChanged = true;

    }
    else
    {
        if( m_Paths.front().Cmp( path ) )
        {
            m_Paths.pop_front();
            m_Paths.push_front( path );
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
    std::cout << " * [INFO] changed project dir to " << m_Paths.front().ToUTF8() << "\n";
#endif

    return true;
}


wxString S3D_FILENAME_RESOLVER::GetProjectDir( void )
{
    if( m_Paths.empty() )
        return wxEmptyString;

    return m_Paths.front();
}


bool S3D_FILENAME_RESOLVER::createPathList( void )
{
    if( !m_Paths.empty() )
        return true;

    wxString kmod;

    // add the current working directory as the first entry by
    // default; since CWD is not necessarily what we really want,
    // the user may change this later with a call to SetProjectDir()

    if( !addPath( wxFileName::GetCwd() ) )
        m_Paths.push_back( wxEmptyString );

    if( wxGetEnv( wxT( "KISYS3DMOD" ), &kmod ) )
        addPath( kmod );

    if( !m_ConfigDir.empty() )
        readPathList();

    if( m_Paths.empty() )
        return false;

#ifdef DEBUG
    std::cout << " * [3D model] search paths:\n";
    std::list< wxString >::const_iterator sPL = m_Paths.begin();
    std::list< wxString >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        std::cout << "   + '" << (*sPL).ToUTF8() << "'\n";
        ++sPL;
    }
#endif

    return true;
}


bool S3D_FILENAME_RESOLVER::UpdatePathList( std::vector< wxString >& aPathList )
{
    while( m_Paths.size() > 1 )
        m_Paths.pop_back();

    size_t nI = aPathList.size();

    for( size_t i = 0; i < nI; ++i )
        addPath( aPathList[i] );

#ifdef DEBUG
    std::cerr << "* S3D_FILENAME_RESOLVER::UpdatePathList()\n";
    std::cerr << "NItems: " << aPathList.size() << "\n";

    for( size_t i = 0; i < aPathList.size(); ++i )
        std::cerr << "Item #" << i << ": " << aPathList[i].ToUTF8() << "\n";
#endif

    return writePathList();
}


wxString S3D_FILENAME_RESOLVER::ResolvePath( const wxString& aFileName )
{
    if( aFileName.empty() )
        return wxEmptyString;

    if( m_Paths.empty() )
        createPathList();

    // first attempt to use the name as specified:
    wxString aResolvedName;
    wxString fname = aFileName;

#ifdef _WIN32
    // translate from KiCad's internal UNIX-like path to MSWin paths
    fname.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    if( checkRealPath( fname, aResolvedName ) )
        return aResolvedName;

    // look up the filename in the internal filename map
    std::map< wxString, wxString, S3D::rsort_wxString >::iterator mi;
    mi = m_NameMap.find( fname );

    if( mi != m_NameMap.end() )
        return mi->second;

    std::list< wxString >::const_iterator sPL = m_Paths.begin();
    std::list< wxString >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        wxFileName fpath( wxFileName::DirName( *sPL ) );
        wxFileName filename( fname );

        // we can only attempt a search if the filename is incomplete
        if( filename.IsRelative() )
        {
            wxString fullPath = fpath.GetPathWithSep() + fname;

            if( checkRealPath( fullPath, aResolvedName ) )
                return aResolvedName;
        }

        ++sPL;
    }

    std::cerr << " * [3D Model] filename could not be resolved: '";
    std::cerr << aFileName.ToUTF8() << "'\n";

    return wxEmptyString;
}


bool S3D_FILENAME_RESOLVER::checkRealPath( const wxString& aFileName,
    wxString& aResolvedName )
{
    aResolvedName.clear();
    wxFileName fname( aFileName );
    fname.Normalize();

    if( !fname.FileExists() )
        return false;

    aResolvedName = fname.GetFullPath();
    m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, aResolvedName ) );

    return true;
}


bool S3D_FILENAME_RESOLVER::addPath( const wxString& aPath )
{
    if( aPath.empty() )
        return false;

    wxFileName path( aPath, "" );
    path.Normalize();

    if( !path.DirExists() )
    {
        std::cerr << " * [3D Model] invalid path: '" << path.GetPath().ToUTF8() << "'\n";
        return false;
    }

    wxString pname = path.GetPath();
    std::list< wxString >::const_iterator sPL = m_Paths.begin();
    std::list< wxString >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( !pname.Cmp( *sPL ) )
            return true;

        ++sPL;
    }

    m_Paths.push_back( pname );
    return true;
}


bool S3D_FILENAME_RESOLVER::readPathList( void )
{
    if( m_ConfigDir.empty() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * 3D configuration directory is unknown\n";
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
        std::cerr << " * no 3D configuration file: '";
        std::cerr << cfgname.ToUTF8() << "'\n";
        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * could not open configuration file '" << cfgname.ToUTF8() << "'\n";
        return false;
    }

    int lineno = 0;
    bool mod = false;   // set to true if there are non-existent paths in the file

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

        std::string::size_type spos = cfgLine.find_first_of( '"', 0 );

        if( std::string::npos == spos )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * bad config entry in config file '" << cfgname.ToUTF8() << "'\n";
            std::cerr << "   line " << lineno << " [missing opening quote mark]\n";
        }

        cfgLine.erase( 0, spos + 1 );

        spos = cfgLine.find_last_of( '"' );

        if( std::string::npos == spos )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * bad config entry in config file '" << cfgname.ToUTF8() << "'\n";
            std::cerr << "   line " << lineno << " [missing closing quote mark]\n";
        }

        cfgLine.erase( spos );

        if( !addPath( cfgLine ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * bad config entry in config file '" << cfgname.ToUTF8() << "'\n";
            std::cerr << "   line " << lineno << " [not a valid path]: '";
            std::cerr << cfgLine << "'\n";
            mod = true;
        }
    }

    cfgFile.close();

    if( mod )
        writePathList();

    if( m_Paths.size() != nitems )
        return true;

    return false;
}


bool S3D_FILENAME_RESOLVER::writePathList( void )
{
    if( m_ConfigDir.empty() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * 3D configuration directory is unknown\n";
        return false;
    }

    if( m_Paths.empty() || 1 == m_Paths.size() )
        return false;

    wxString cfgname = m_ConfigDir + S3D_RESOLVER_CONFIG;
    std::ofstream cfgFile;

    cfgFile.open( cfgname.ToUTF8(), std::ios_base::trunc );

    if( !cfgFile.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * could not open configuration file '" << cfgname.ToUTF8() << "'\n";
        return false;
    }

    std::list< wxString >::const_iterator sPL = m_Paths.begin();
    std::list< wxString >::const_iterator ePL = m_Paths.end();

    // the first entry is the current project dir; we never add a project dir
    // to the path list in the configuration file
    ++sPL;

    while( sPL != ePL )
    {
        cfgFile << "\"" << (*sPL).ToUTF8() << "\"\n";
        ++sPL;
    }

    bool bad = cfgFile.bad();
    cfgFile.close();

    if( bad )
        return false;

    return true;
}


wxString S3D_FILENAME_RESOLVER::ShortenPath( const wxString& aFullPathName )
{
    wxString fname = aFullPathName;

    if( m_Paths.empty() )
        createPathList();

    std::list< wxString >::const_iterator sL = m_Paths.begin();
    std::list< wxString >::const_iterator eL = m_Paths.end();

    while( sL != eL )
    {
        wxFileName fpath( *sL, "" );
        wxString fps = fpath.GetPathWithSep();

        if( std::string::npos != fname.find( fps ) )
        {
            fname = fname.substr( fps.size() );

#ifdef _WIN32
            fname.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

            return fname;
        }

        ++sL;
    }

    return fname;
}



const std::list< wxString >* S3D_FILENAME_RESOLVER::GetPaths( void )
{
    return &m_Paths;
}
