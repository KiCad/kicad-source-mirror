/*
 * This program source code file is part kicad2mcad
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include "3d_resolver.h"

// configuration file version
#define CFGFILE_VERSION 1
#define S3D_RESOLVER_CONFIG "ExportPaths.cfg"

// flag bits used to track different one-off messages to users
#define ERRFLG_ALIAS    (1)
#define ERRFLG_RELPATH  (2)
#define ERRFLG_ENVPATH  (4)


/**
 * Flag to enable plugin loader trace output.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const trace3dResolver = wxT( "KICAD_3D_RESOLVER" );


static std::mutex mutex3D_resolver;


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult );


S3D_RESOLVER::S3D_RESOLVER()
{
    m_errflags = 0;
}


bool S3D_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    createPathList();

    return true;
}


bool S3D_RESOLVER::createPathList( void )
{
    if( !m_Paths.empty() )
        return true;

    readPathList();

    if( m_Paths.empty() )
        return false;

#ifdef DEBUG
    wxLogTrace( trace3dResolver, " * [3D model] search paths:\n" );

    for( const SEARCH_PATH& searchPath : m_Paths )
        wxLogTrace( trace3dResolver, "   + '%s'\n", searchPath.m_Pathexp );
#endif

    return true;
}


wxString S3D_RESOLVER::ResolvePath( const wxString& aFileName,
                                    std::vector<wxString>& aSearchedPaths )
{
    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    if( aFileName.empty() )
        return wxEmptyString;

    if( m_Paths.empty() )
        createPathList();

    // look up the filename in the internal filename map
    std::map<wxString, wxString, S3D::rsort_wxString>::iterator mi;
    mi = m_NameMap.find( aFileName );

    if( mi != m_NameMap.end() )
        return mi->second;

    // first attempt to use the name as specified:
    wxString tname = aFileName;

#ifdef _WIN32
    // translate from KiCad's internal UNIX-like path to MSWin paths
    tname.Replace( "/", "\\" );
#endif

    // Note: variable expansion must preferably be performed via a threadsafe wrapper for the
    // getenv() system call. If we allow the wxFileName::Normalize() routine to perform expansion
    // then we will have a race condition since wxWidgets does not assure a threadsafe wrapper
    // for getenv().
    if( tname.StartsWith( "${" ) || tname.StartsWith( "$(" ) )
        tname = expandVars( tname );

    wxFileName tmpFN( tname );

    // in the case of absolute filenames we don't store a map item
    if( !aFileName.StartsWith( "${" ) && !aFileName.StartsWith( "$(" ) && tmpFN.IsAbsolute() )
    {
        if( tmpFN.FileExists() )
        {
            tmpFN.Normalize();
            return tmpFN.GetFullPath();
        }
        else
        {
            aSearchedPaths.push_back( tmpFN.GetFullPath() );
        }
    }

    // this case covers full paths, leading expanded vars, and paths relative to the current
    // working directory (which is not necessarily the current project directory)
    tmpFN.Normalize();

    if( tmpFN.FileExists() )
    {
        tname = tmpFN.GetFullPath();
        m_NameMap[ aFileName ] = tname;

        // special case: if a path begins with ${ENV_VAR} but is not in the resolver's path list
        // then add it
        if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
            checkEnvVarPath( aFileName );

        return tname;
    }
    else if( tmpFN.GetFullPath() != aFileName )
    {
        aSearchedPaths.push_back( tmpFN.GetFullPath() );
    }

    // if a path begins with ${ENV_VAR}/$(ENV_VAR) and is not resolved then the file either does
    // not exist or the ENV_VAR is not defined
    if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
    {
        m_errflags |= ERRFLG_ENVPATH;
        return aFileName;
    }

    // at this point aFileName is:
    // a. an aliased shortened name or
    // b. cannot be determined

    // check the path relative to the current project directory;
    // NB: this is not necessarily the same as the current working directory, which has already
    // been checked. This case accounts for partial paths which do not contain ${KIPRJMOD}.
    // This check is performed before checking the path relative to ${KICAD6_3DMODEL_DIR} so that
    // users can potentially override a model within ${KICAD6_3DMODEL_DIR}.
    if( !m_Paths.begin()->m_Pathexp.empty() && !tname.StartsWith( ":" ) )
    {
        tmpFN.Assign( m_Paths.begin()->m_Pathexp, "" );
        wxString fullPath = tmpFN.GetPathWithSep() + tname;

        if( fullPath.StartsWith( "${" ) || fullPath.StartsWith( "$(" ) )
            fullPath = expandVars( fullPath );

        tmpFN.Assign( fullPath );
        tmpFN.Normalize();

        if( tmpFN.FileExists() )
        {
            tname = tmpFN.GetFullPath();
            m_NameMap[ aFileName ] = tname;
            return tname;
        }
        else if( tmpFN.GetFullPath() != aFileName )
        {
            aSearchedPaths.push_back( tmpFN.GetFullPath() );
        }
    }

    // check the partial path relative to ${KICAD6_3DMODEL_DIR} (legacy behavior)
    if( !tname.Contains( ":" ) )
    {
        wxFileName fpath;
        wxString fullPath( "${KICAD6_3DMODEL_DIR}" );
        fullPath.Append( fpath.GetPathSeparator() );
        fullPath.Append( tname );
        fullPath = expandVars( fullPath );
        fpath.Assign( fullPath );
        fpath.Normalize();

        if( fpath.FileExists() )
        {
            tname = fpath.GetFullPath();
            m_NameMap[ aFileName ] = tname;
            return tname;
        }
        else
        {
            aSearchedPaths.push_back( fpath.GetFullPath() );
        }
    }

    // at this point the filename must contain an alias or else it is invalid
    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( tname, alias, relpath ) )
    {
        // this can happen if the file was intended to be relative to ${KICAD6_3DMODEL_DIR}
        // but ${KICAD6_3DMODEL_DIR} is not set or is incorrect.
        m_errflags |= ERRFLG_RELPATH;
        return aFileName;
    }

    for( const SEARCH_PATH& path : m_Paths )
    {
        // ${ENV_VAR} paths have already been checked; skip them
        if( path.m_Alias.StartsWith( "${" ) || path.m_Alias.StartsWith( "$(" ) )
            continue;

        if( path.m_Alias == alias && !path.m_Pathexp.empty() )
        {
            wxFileName fpath( wxFileName::DirName( path.m_Pathexp ) );
            wxString fullPath = fpath.GetPathWithSep() + relpath;

            if( fullPath.StartsWith( "${") || fullPath.StartsWith( "$(" ) )
                fullPath = expandVars( fullPath );

            wxFileName tmp( fullPath );
            tmp.Normalize();

            if( tmp.FileExists() )
            {
                tname = tmp.GetFullPath();
                m_NameMap[ aFileName ] = tname;
                return tname;
            }
            else
            {
                aSearchedPaths.push_back( tmp.GetFullPath() );
            }
        }
    }

    m_errflags |= ERRFLG_ALIAS;
    return aFileName;
}


bool S3D_RESOLVER::addPath( const SEARCH_PATH& aPath )
{
    if( aPath.m_Alias.empty() || aPath.m_Pathvar.empty() )
        return false;

    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    SEARCH_PATH tpath = aPath;

#ifdef _WIN32
    while( tpath.m_Pathvar.EndsWith( "\\" ) )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
#else
    while( tpath.m_Pathvar.EndsWith( "/" ) && tpath.m_Pathvar.length() > 1 )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
#endif

    wxFileName path( tpath.m_Pathvar, "" );
    path.Normalize();

    if( !path.DirExists() )
    {
        // Show a message only in debug mode
#ifdef DEBUG
        if( aPath.m_Pathvar == "${KICAD6_3DMODEL_DIR}"
                || aPath.m_Pathvar == "${KIPRJMOD}"
                || aPath.m_Pathvar == "${KISYS3DMOD}" )
        {
            // suppress the message if the missing pathvar is a system variable
        }
        else
        {
            wxString msg = _( "The given path does not exist" );
            msg.append( "\n" );
            msg.append( tpath.m_Pathvar );
            wxLogMessage( "%s\n", msg.ToUTF8() );
        }
#endif

        tpath.m_Pathexp.clear();
    }
    else
    {
        tpath.m_Pathexp = path.GetFullPath();

#ifdef _WIN32
        while( tpath.m_Pathexp.EndsWith( "\\" ) )
        tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#else
        while( tpath.m_Pathexp.EndsWith( "/" ) && tpath.m_Pathexp.length() > 1 )
            tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#endif
    }

    wxString pname = path.GetPath();
    std::list< SEARCH_PATH >::iterator sPL = m_Paths.begin();
    std::list< SEARCH_PATH >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( tpath.m_Alias == sPL->m_Alias )
        {
            wxString msg = _( "Alias:" ) + wxS( " " );
            msg.append( tpath.m_Alias );
            msg.append( "\n" );
            msg.append( _( "This path:" ) + wxS( " " ) );
            msg.append( tpath.m_Pathvar );
            msg.append( "\n" );
            msg.append( _( "Existing path:" ) + wxS( " " ) );
            msg.append( sPL->m_Pathvar );
            wxMessageBox( msg, _( "Bad alias (duplicate name)" ) );

            return false;
        }

        ++sPL;
    }

    m_Paths.push_back( tpath );
    return true;
}


bool S3D_RESOLVER::readPathList( void )
{
    wxFileName cfgpath( wxStandardPaths::Get().GetTempDir(), S3D_RESOLVER_CONFIG );
    cfgpath.Normalize();
    wxString cfgname = cfgpath.GetFullPath();

    size_t nitems = m_Paths.size();

    std::ifstream cfgFile;
    std::string   cfgLine;

    if( !wxFileName::Exists( cfgname ) )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:d\n * no 3D configuration file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cfgname );

        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * Could not open configuration file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cfgname );

        return false;
    }

    int lineno = 0;
    SEARCH_PATH al;
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

        if( !getHollerith( cfgLine, idx, al.m_Alias ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Pathvar ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Description ) )
            continue;

        addPath( al );
    }

    cfgFile.close();

    if( m_Paths.size() != nitems )
        return true;

    return false;
}


void S3D_RESOLVER::checkEnvVarPath( const wxString& aPath )
{
    bool useParen = false;

    if( aPath.StartsWith( "$(" ) )
        useParen = true;
    else if( !aPath.StartsWith( "${" ) )
        return;

    size_t pEnd;

    if( useParen )
        pEnd = aPath.find( ")" );
    else
        pEnd = aPath.find( "}" );

    if( pEnd == wxString::npos )
        return;

    wxString envar = aPath.substr( 0, pEnd + 1 );

    // check if the alias exists; if not then add it to the end of the
    // env var section of the path list
    std::list< SEARCH_PATH >::iterator sPL = m_Paths.begin();
    std::list< SEARCH_PATH >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( sPL->m_Alias == envar )
            return;

        if( !sPL->m_Alias.StartsWith( "${" ) )
            break;

        ++sPL;
    }

    SEARCH_PATH lpath;
    lpath.m_Alias = envar;
    lpath.m_Pathvar = lpath.m_Alias;
    wxFileName tmpFN( lpath.m_Alias, "" );
    wxUniChar psep = tmpFN.GetPathSeparator();
    tmpFN.Normalize();

    if( !tmpFN.DirExists() )
        return;

    lpath.m_Pathexp = tmpFN.GetFullPath();

    if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
        lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

    if( lpath.m_Pathexp.empty() )
        return;

    m_Paths.insert( sPL, lpath );
    return;
}


wxString S3D_RESOLVER::expandVars( const wxString& aPath )
{
    if( aPath.empty() )
        return wxEmptyString;

    wxString result;

    for( const std::pair<const wxString, wxString>& i : m_EnvVars )
    {
        if( !aPath.compare( 2, i.first.length(), i.first ) )
        {
            result = i.second;
            result.append( aPath.substr( 3 + i.first.length() ) );

            if( result.StartsWith( "${" ) || result.StartsWith( "$(" ) )
                result = expandVars( result );

            return result;
        }
    }

    result = wxExpandEnvVars( aPath );

    if( result == aPath )
        return wxEmptyString;

    if( result.StartsWith( "${" ) || result.StartsWith( "$(" ) )
        result = expandVars( result );

    return result;
}


wxString S3D_RESOLVER::ShortenPath( const wxString& aFullPathName )
{
    wxString fname = aFullPathName;

    if( m_Paths.empty() )
        createPathList();

    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    std::list< SEARCH_PATH >::const_iterator sL = m_Paths.begin();
    std::list< SEARCH_PATH >::const_iterator eL = m_Paths.end();
    size_t idx;

    while( sL != eL )
    {
        // undefined paths do not participate in the file name shortening procedure.
        if( sL->m_Pathexp.empty() )
        {
            ++sL;
            continue;
        }

        wxFileName fpath( sL->m_Pathexp, "" );
        wxString fps = fpath.GetPathWithSep();
        wxString tname;

        idx = fname.find( fps );

        if( std::string::npos != idx && 0 == idx  )
        {
            fname = fname.substr( fps.size() );

#ifdef _WIN32
            // ensure only the '/' separator is used in the internal name
            fname.Replace( "\\", "/" );
#endif

            if( sL->m_Alias.StartsWith( "${" ) || sL->m_Alias.StartsWith( "$(" ) )
            {
                // old style ENV_VAR
                tname = sL->m_Alias;
                tname.Append( "/" );
                tname.append( fname );
            }
            else
            {
                // new style alias
                tname = ":";
                tname.append( sL->m_Alias );
                tname.append( ":" );
                tname.append( fname );
            }

            return tname;
        }

        ++sL;
    }

#ifdef _WIN32
    // it is strange to convert an MSWin full path to use the
    // UNIX separator but this is done for consistency and can
    // be helpful even when transferring project files from
    // MSWin to *NIX.
    fname.Replace( "\\", "/" );
#endif

    return fname;
}



const std::list< SEARCH_PATH >* S3D_RESOLVER::GetPaths( void )
{
    return &m_Paths;
}


bool S3D_RESOLVER::SplitAlias( const wxString& aFileName, wxString& anAlias, wxString& aRelPath )
{
    anAlias.clear();
    aRelPath.clear();

    size_t searchStart = 0;

    if( aFileName.StartsWith( wxT( ":" ) ) )
        searchStart = 1;

    size_t tagpos = aFileName.find( wxT( ":" ), searchStart );

    if( tagpos == wxString::npos || tagpos == searchStart )
        return false;

    if( tagpos + 1 >= aFileName.length() )
        return false;

    anAlias = aFileName.substr( searchStart, tagpos - searchStart );
    aRelPath = aFileName.substr( tagpos + 1 );

    return true;
}


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult )
{
    aResult.clear();

    if( aIndex >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * Bad Hollerith string in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * missing opening quote mark in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * unexpected end of line in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    std::string tnum;

    while( aString[i2] >= '0' && aString[i2] <= '9' )
        tnum.append( 1, aString[i2++] );

    if( tnum.empty() || aString[i2++] != ':' )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * Bad Hollerith string in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    std::istringstream istr;
    istr.str( tnum );
    size_t nchars;
    istr >> nchars;

    if( (i2 + nchars) >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * unexpected end of line in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    if( nchars > 0 )
    {
        aResult = wxString::FromUTF8( aString.substr( i2, nchars ).c_str() );
        i2 += nchars;
    }

    if( i2 >= aString.size() || aString[i2] != '"' )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n * missing closing quote mark in line '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    aIndex = i2 + 1;
    return true;
}


bool S3D_RESOLVER::ValidateFileName( const wxString& aFileName, bool& hasAlias )
{
    // Rules:
    // 1. The generic form of an aliased 3D relative path is:
    //    ALIAS:relative/path
    // 2. ALIAS is a UTF string excluding "{}[]()%~<>\"='`;:.,&?/\\|$"
    // 3. The relative path must be a valid relative path for the platform
    hasAlias = false;

    if( aFileName.empty() )
        return false;

    wxString filename = aFileName;
    wxString lpath;
    size_t aliasStart = aFileName.StartsWith( ':' ) ? 1 : 0;
    size_t aliasEnd = aFileName.find( ':' );

    // ensure that the file separators suit the current platform
#ifdef __WINDOWS__
    filename.Replace( "/", "\\" );

    // if we see the :\ pattern then it must be a drive designator
    if( aliasEnd != wxString::npos )
    {
        size_t pos1 = aFileName.find( ":\\" );

        if( pos1 != wxString::npos && ( pos1 != aliasEnd || pos1 != 1 ) )
            return false;

        // if we have a drive designator then we have no alias
        if( pos1 != wxString::npos )
            aliasEnd = wxString::npos;
    }
#else
    filename.Replace( "\\", "/" );
#endif

    // names may not end with ':'
    if( aliasEnd == aFileName.length() - 1 )
        return false;

    if( aliasEnd != wxString::npos )
    {
        // ensure the alias component is not empty
        if( aliasEnd == aliasStart )
            return false;

        lpath = filename.substr( aliasStart, aliasEnd );

        // check the alias for restricted characters
        if( wxString::npos != lpath.find_first_of( "{}[]()%~<>\"='`;:.,&?/\\|$" ) )
            return false;

        hasAlias = true;
        lpath = aFileName.substr( aliasEnd + 1 );
    }
    else
    {
        lpath = aFileName;

        // in the case of ${ENV_VAR}|$(ENV_VAR)/path, strip the
        // environment string before testing
        aliasEnd = wxString::npos;

        if( aFileName.StartsWith( "${" ) )
            aliasEnd = aFileName.find( '}' );
        else if( aFileName.StartsWith( "$(" ) )
            aliasEnd = aFileName.find( ')' );

        if( aliasEnd != wxString::npos )
            lpath = aFileName.substr( aliasEnd + 1 );
    }

    if( wxString::npos != lpath.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}
