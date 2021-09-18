/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fstream>
#include <mutex>
#include <sstream>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <pgm_base.h>
#include <trace_helpers.h>

#include "common.h"
#include "filename_resolver.h"

// configuration file version
#define CFGFILE_VERSION 1
#define RESOLVER_CONFIG wxT( "3Dresolver.cfg" )

// flag bits used to track different one-off messages to users
#define ERRFLG_ALIAS    (1)
#define ERRFLG_RELPATH  (2)
#define ERRFLG_ENVPATH  (4)

#define MASK_3D_RESOLVER "3D_RESOLVER"

static std::mutex mutex_resolver;

static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult );


FILENAME_RESOLVER::FILENAME_RESOLVER() :
        m_pgm( nullptr ),
        m_project( nullptr )
{
    m_errflags = 0;
}


bool FILENAME_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    if( aConfigDir.empty() )
        return false;

    wxFileName cfgdir( ExpandEnvVarSubstitutions( aConfigDir, m_project ), "" );

    cfgdir.Normalize();

    if( !cfgdir.DirExists() )
        return false;

    m_configDir = cfgdir.GetPath();
    createPathList();

    return true;
}


bool FILENAME_RESOLVER::SetProject( PROJECT* aProject, bool* flgChanged )
{
    m_project = aProject;

    if( !aProject )
        return false;

    wxFileName projdir( ExpandEnvVarSubstitutions( aProject->GetProjectPath(), aProject ), "" );

    projdir.Normalize();

    if( !projdir.DirExists() )
        return false;

    m_curProjDir = projdir.GetPath();

    if( flgChanged )
        *flgChanged = false;

    if( m_paths.empty() )
    {
        SEARCH_PATH al;
        al.m_Alias = "${KIPRJMOD}";
        al.m_Pathvar = "${KIPRJMOD}";
        al.m_Pathexp = m_curProjDir;
        m_paths.push_back( al );

        if( flgChanged )
            *flgChanged = true;
    }
    else
    {
        if( m_paths.front().m_Pathexp.Cmp( m_curProjDir ) )
        {
            m_paths.front().m_Pathexp = m_curProjDir;

            if( flgChanged )
                *flgChanged = true;
        }
        else
        {
            return true;
        }
    }

#ifdef DEBUG
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] changed project dir to ";
        ostr << m_paths.front().m_Pathexp.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
    }
#endif

    return true;
}


wxString FILENAME_RESOLVER::GetProjectDir() const
{
    return m_curProjDir;
}


void FILENAME_RESOLVER::SetProgramBase( PGM_BASE* aBase )
{
    m_pgm = aBase;

    if( !m_pgm || m_paths.empty() )
        return;

    // recreate the path list
    m_paths.clear();
    createPathList();
}


bool FILENAME_RESOLVER::createPathList()
{
    if( !m_paths.empty() )
        return true;

    wxString kmod;

    // add an entry for the default search path; at this point
    // we cannot set a sensible default so we use an empty string.
    // the user may change this later with a call to SetProjectDir()

    SEARCH_PATH lpath;
    lpath.m_Alias = "${KIPRJMOD}";
    lpath.m_Pathvar = "${KIPRJMOD}";
    lpath.m_Pathexp = m_curProjDir;
    m_paths.push_back( lpath );
    wxFileName fndummy;
    wxUniChar psep = fndummy.GetPathSeparator();
    std::list< wxString > epaths;

    if( GetKicadPaths( epaths ) )
    {
        for( const wxString& curr_path : epaths )
        {
            wxString pathVal = ExpandEnvVarSubstitutions( curr_path, m_project );

            if( pathVal.empty() )
            {
                lpath.m_Pathexp.clear();
            }
            else
            {
                fndummy.Assign( pathVal, "" );
                fndummy.Normalize();
                lpath.m_Pathexp = fndummy.GetFullPath();
            }

            lpath.m_Alias   = curr_path;
            lpath.m_Pathvar = curr_path;

            if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
                lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

            m_paths.push_back( lpath );
        }
    }

    if( !m_configDir.empty() )
        readPathList();

    if( m_paths.empty() )
        return false;

#ifdef DEBUG
    wxLogTrace( MASK_3D_RESOLVER, " * [3D model] search paths:\n" );
    std::list< SEARCH_PATH >::const_iterator sPL = m_paths.begin();

    while( sPL != m_paths.end() )
    {
        wxLogTrace( MASK_3D_RESOLVER, "   + %s : '%s'\n", (*sPL).m_Alias.GetData(),
            (*sPL).m_Pathexp.GetData() );
        ++sPL;
    }
#endif

    return true;
}


bool FILENAME_RESOLVER::UpdatePathList( const std::vector< SEARCH_PATH >& aPathList )
{
    wxUniChar envMarker( '$' );

    while( !m_paths.empty() && envMarker != *m_paths.back().m_Alias.rbegin() )
        m_paths.pop_back();

    for( const SEARCH_PATH& path : aPathList )
        addPath( path );

    return WritePathList( m_configDir, RESOLVER_CONFIG, false );
}


wxString FILENAME_RESOLVER::ResolvePath( const wxString& aFileName )
{
    std::lock_guard<std::mutex> lock( mutex_resolver );

    if( aFileName.empty() )
        return wxEmptyString;

    if( m_paths.empty() )
        createPathList();

    // first attempt to use the name as specified:
    wxString tname = aFileName;

    #ifdef _WIN32
    // translate from KiCad's internal UNIX-like path to MSWin paths
    tname.Replace( wxT( "/" ), wxT( "\\" ) );
    #endif

    // Note: variable expansion must preferably be performed via a threadsafe wrapper for the
    // getenv() system call. If we allow the wxFileName::Normalize() routine to perform expansion
    // then we will have a race condition since wxWidgets does not assure a threadsafe wrapper
    // for getenv().
    tname = ExpandEnvVarSubstitutions( tname, m_project );

    wxFileName tmpFN( tname );

    // this case covers full paths, leading expanded vars, and paths relative to the current
    // working directory (which is not necessarily the current project directory)
    if( tmpFN.FileExists() )
    {
        tmpFN.Normalize();
        tname = tmpFN.GetFullPath();

        // special case: if a path begins with ${ENV_VAR} but is not in the resolver's path list
        // then add it.
        if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
            checkEnvVarPath( aFileName );

        return tname;
    }

    // if a path begins with ${ENV_VAR}/$(ENV_VAR) and is not resolved then the file either does
    // not exist or the ENV_VAR is not defined
    if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
    {
        if( !( m_errflags & ERRFLG_ENVPATH ) )
        {
            m_errflags |= ERRFLG_ENVPATH;
            wxString errmsg = "[3D File Resolver] No such path; ensure the environment var is defined";
            errmsg.append( "\n" );
            errmsg.append( tname );
            errmsg.append( "\n" );
            wxLogTrace( tracePathsAndFiles, errmsg );
        }

        return wxEmptyString;
    }

    // at this point aFileName is:
    // a. an aliased shortened name or
    // b. cannot be determined

    // check the path relative to the current project directory;
    // NB: this is not necessarily the same as the current working directory, which has already
    // been checked. This case accounts for partial paths which do not contain ${KIPRJMOD}.
    // This check is performed before checking the path relative to ${KICAD6_3DMODEL_DIR} so that
    // users can potentially override a model within ${KICAD6_3DMODEL_DIR}
    if( !m_paths.begin()->m_Pathexp.empty() && !tname.StartsWith( ":" ) )
    {
        tmpFN.Assign( m_paths.begin()->m_Pathexp, "" );
        wxString fullPath = tmpFN.GetPathWithSep() + tname;

        fullPath = ExpandEnvVarSubstitutions( fullPath, m_project );

        if( wxFileName::FileExists( fullPath ) )
        {
            tmpFN.Assign( fullPath );
            tmpFN.Normalize();
            tname = tmpFN.GetFullPath();
            return tname;
        }

    }

    // check the partial path relative to ${KICAD6_3DMODEL_DIR} (legacy behavior)
    if( !tname.StartsWith( ":" ) )
    {
        wxFileName fpath;
        wxString fullPath( "${KICAD6_3DMODEL_DIR}" );
        fullPath.Append( fpath.GetPathSeparator() );
        fullPath.Append( tname );
        fullPath = ExpandEnvVarSubstitutions( fullPath, m_project );
        fpath.Assign( fullPath );

        if( fpath.Normalize() && fpath.FileExists() )
        {
            tname = fpath.GetFullPath();
            return tname;
        }

    }

    // at this point the filename must contain an alias or else it is invalid
    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( tname, alias, relpath ) )
    {
        if( !( m_errflags & ERRFLG_RELPATH ) )
        {
            // this can happen if the file was intended to be relative to ${KICAD6_3DMODEL_DIR}
            // but ${KICAD6_3DMODEL_DIR} is not set or is incorrect.
            m_errflags |= ERRFLG_RELPATH;
            wxString errmsg = "[3D File Resolver] No such path";
            errmsg.append( "\n" );
            errmsg.append( tname );
            errmsg.append( "\n" );
            wxLogTrace( tracePathsAndFiles, errmsg );
        }

        return wxEmptyString;
    }

    for( const SEARCH_PATH& path : m_paths )
    {
        // ${ENV_VAR} paths have already been checked; skip them
        if( path.m_Alias.StartsWith( "${" ) || path.m_Alias.StartsWith( "$(" ) )
            continue;

        if( !path.m_Alias.Cmp( alias ) && !path.m_Pathexp.empty() )
        {
            wxFileName fpath( wxFileName::DirName( path.m_Pathexp ) );
            wxString fullPath = fpath.GetPathWithSep() + relpath;

            fullPath = ExpandEnvVarSubstitutions( fullPath, m_project );

            if( wxFileName::FileExists( fullPath ) )
            {
                wxFileName tmp( fullPath );

                if( tmp.Normalize() )
                    tname = tmp.GetFullPath();

                return tname;
            }
        }
    }

    if( !( m_errflags & ERRFLG_ALIAS ) )
    {
        m_errflags |= ERRFLG_ALIAS;
        wxString errmsg = "[3D File Resolver] No such path; ensure the path alias is defined";
        errmsg.append( "\n" );
        errmsg.append( tname.substr( 1 ) );
        errmsg.append( "\n" );
        wxLogTrace( tracePathsAndFiles, errmsg );
    }

    return wxEmptyString;
}


bool FILENAME_RESOLVER::addPath( const SEARCH_PATH& aPath )
{
    if( aPath.m_Alias.empty() || aPath.m_Pathvar.empty() )
        return false;

    std::lock_guard<std::mutex> lock( mutex_resolver );

    SEARCH_PATH tpath = aPath;

    #ifdef _WIN32
    while( tpath.m_Pathvar.EndsWith( wxT( "\\" ) ) )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
    #else
    while( tpath.m_Pathvar.EndsWith( wxT( "/" ) ) && tpath.m_Pathvar.length() > 1 )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
    #endif

    wxFileName path( ExpandEnvVarSubstitutions( tpath.m_Pathvar, m_project ), "" );

    path.Normalize();

    if( !path.DirExists() )
    {
        // suppress the message if the missing pathvar is the
        // legacy KICAD6_3DMODEL_DIR variable
        if( aPath.m_Pathvar.compare( wxT( "${KICAD6_3DMODEL_DIR}" ) ) )
        {
            wxString msg = _( "The given path does not exist" );
            msg.append( wxT( "\n" ) );
            msg.append( tpath.m_Pathvar );
            wxMessageBox( msg, _( "3D model search path" ) );
        }

        tpath.m_Pathexp.clear();
    }
    else
    {
        tpath.m_Pathexp = path.GetFullPath();

#ifdef _WIN32
        while( tpath.m_Pathexp.EndsWith( wxT( "\\" ) ) )
        tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#else
        while( tpath.m_Pathexp.EndsWith( wxT( "/" ) ) && tpath.m_Pathexp.length() > 1 )
            tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#endif
    }

    std::list< SEARCH_PATH >::iterator sPL = m_paths.begin();
    std::list< SEARCH_PATH >::iterator ePL = m_paths.end();

    while( sPL != ePL )
    {
        if( !tpath.m_Alias.Cmp( sPL->m_Alias ) )
        {
            wxString msg = _( "Alias: " );
            msg.append( tpath.m_Alias );
            msg.append( wxT( "\n" ) );
            msg.append( _( "This path:" ) + wxS( " " ) );
            msg.append( tpath.m_Pathvar );
            msg.append( wxT( "\n" ) );
            msg.append( _( "Existing path:" ) + wxS( " " ) );
            msg.append( sPL->m_Pathvar );
            wxMessageBox( msg, _( "Bad alias (duplicate name)" ) );

            return false;
        }

        ++sPL;
    }

    m_paths.push_back( tpath );
    return true;
}


bool FILENAME_RESOLVER::readPathList()
{
    if( m_configDir.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "3D configuration directory is unknown";
        ostr << " * " << errmsg.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        return false;
    }

    wxFileName cfgpath( m_configDir, RESOLVER_CONFIG );
    cfgpath.Normalize();
    wxString cfgname = cfgpath.GetFullPath();

    size_t nitems = m_paths.size();

    std::ifstream cfgFile;
    std::string   cfgLine;

    if( !wxFileName::Exists( cfgname ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "no 3D configuration file";
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
        wxString errmsg = "Could not open configuration file";
        ostr << " * " << errmsg.ToUTF8() << " '" << cfgname.ToUTF8() << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
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

        // never add on KICAD6_3DMODEL_DIR from a config file
        if( !al.m_Alias.Cmp( wxT( "KICAD6_3DMODEL_DIR" ) ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Pathvar ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Description ) )
            continue;

        addPath( al );
    }

    cfgFile.close();

    if( vnum < CFGFILE_VERSION )
        WritePathList( m_configDir, RESOLVER_CONFIG, false );

    return( m_paths.size() != nitems );
}


bool FILENAME_RESOLVER::WritePathList( const wxString& aDir, const wxString& aFilename,
                                       bool aResolvePaths )
{
    if( aDir.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "3D configuration directory is unknown" );
        ostr << " * " << errmsg.ToUTF8();
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );
        wxMessageBox( errmsg, _( "Write 3D search path list" ) );

        return false;
    }

    std::list<SEARCH_PATH>::const_iterator sPL = m_paths.begin();

    if( !aResolvePaths )
    {
        // skip all ${ENV_VAR} alias names

        while( sPL != m_paths.end()
                && ( sPL->m_Alias.StartsWith( "${" ) || sPL->m_Alias.StartsWith( "$(" ) ) )
        {
            ++sPL;
        }
    }

    wxFileName cfgpath( aDir, aFilename );
    wxString cfgname = cfgpath.GetFullPath();
    std::ofstream cfgFile;

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
    std::string tstr;

    while( sPL != m_paths.end() )
    {
        tstr = sPL->m_Alias.ToUTF8();
        cfgFile << "\"" << tstr.size() << ":" << tstr << "\",";

        if( aResolvePaths )
            tstr = ExpandEnvVarSubstitutions( sPL->m_Pathvar, m_project ).ToUTF8();
        else
            tstr = sPL->m_Pathvar.ToUTF8();

        cfgFile << "\"" << tstr.size() << ":" << tstr << "\",";

        tstr = sPL->m_Description.ToUTF8();
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


void FILENAME_RESOLVER::checkEnvVarPath( const wxString& aPath )
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
    auto sPL = m_paths.begin();
    auto ePL = m_paths.end();

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
    wxFileName tmpFN( ExpandEnvVarSubstitutions( lpath.m_Alias, m_project ), "" );

    wxUniChar psep = tmpFN.GetPathSeparator();
    tmpFN.Normalize();

    if( !tmpFN.DirExists() )
        return;

    lpath.m_Pathexp = tmpFN.GetFullPath();

    if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
        lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

    if( lpath.m_Pathexp.empty() )
        return;

    m_paths.insert( sPL, lpath );
}


wxString FILENAME_RESOLVER::ShortenPath( const wxString& aFullPathName )
{
    wxString fname = aFullPathName;

    if( m_paths.empty() )
        createPathList();

    std::lock_guard<std::mutex> lock( mutex_resolver );

    std::list< SEARCH_PATH >::const_iterator sL = m_paths.begin();
    size_t idx;

    while( sL != m_paths.end() )
    {
        // undefined paths do not participate in the
        // file name shortening procedure
        if( sL->m_Pathexp.empty() )
        {
            ++sL;
            continue;
        }

        wxFileName fpath;

        // in the case of aliases, ensure that we use the most recent definition
        if( sL->m_Alias.StartsWith( "${" ) || sL->m_Alias.StartsWith( "$(" ) )
        {
            wxString tpath = ExpandEnvVarSubstitutions( sL->m_Alias, m_project );

            if( tpath.empty() )
            {
                ++sL;
                continue;
            }

            fpath.Assign( tpath, wxT( "" ) );
        }
        else
        {
            fpath.Assign( sL->m_Pathexp, wxT( "" ) );
        }

        wxString fps = fpath.GetPathWithSep();
        wxString tname;

        idx = fname.find( fps );

        if( idx == 0 )
        {
            fname = fname.substr( fps.size() );

            #ifdef _WIN32
            // ensure only the '/' separator is used in the internal name
            fname.Replace( wxT( "\\" ), wxT( "/" ) );
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
    fname.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    return fname;
}



const std::list< SEARCH_PATH >* FILENAME_RESOLVER::GetPaths() const
{
    return &m_paths;
}


bool FILENAME_RESOLVER::SplitAlias( const wxString& aFileName,
    wxString& anAlias, wxString& aRelPath ) const
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
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "bad Hollerith string on line";
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "missing opening quote mark in config file";
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "invalid entry (unexpected end of line)";
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
        wxString errmsg = "bad Hollerith string on line";
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
        wxString errmsg = "invalid entry (unexpected end of line)";
        ostr << " * " << errmsg.ToUTF8() << "\n'" << aString << "'";
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", ostr.str().c_str() );

        return false;
    }

    if( nchars > 0 )
    {
        aResult = wxString::FromUTF8( aString.substr( i2, nchars ).c_str() );
        i2 += nchars;
    }

    if( i2 >= aString.size() || aString[i2] != '"' )
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


bool FILENAME_RESOLVER::ValidateFileName( const wxString& aFileName, bool& hasAlias ) const
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
    size_t aliasStart = aFileName.StartsWith( ':' ) ? 1 : 0;
    size_t aliasEnd = aFileName.find( ':', aliasStart );

    // ensure that the file separators suit the current platform
#ifdef __WINDOWS__
    filename.Replace( wxT( "/" ), wxT( "\\" ) );

    // if we see the :\ pattern then it must be a drive designator
    if( aliasEnd != wxString::npos )
    {
        size_t pos1 = filename.find( wxT( ":\\" ) );

        if( pos1 != wxString::npos && ( pos1 != aliasEnd || pos1 != 1 ) )
            return false;

        // if we have a drive designator then we have no alias
        if( pos1 != wxString::npos )
            aliasEnd = wxString::npos;
    }
#else
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    // names may not end with ':'
    if( aliasEnd == aFileName.length() -1 )
        return false;

    if( aliasEnd != wxString::npos )
    {
        // ensure the alias component is not empty
        if( aliasEnd == aliasStart )
            return false;

        lpath = filename.substr( aliasStart, aliasEnd );

        // check the alias for restricted characters
        if( wxString::npos != lpath.find_first_of( wxT( "{}[]()%~<>\"='`;:.,&?/\\|$" ) ) )
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

    // Test for forbidden chars in filenames. Should be wxFileName::GetForbiddenChars()
    // On MSW, the list returned by wxFileName::GetForbiddenChars() contains separators
    // '\'and '/' used here because lpath is a full path (after last ':').
    // So remove separators
    wxString lpath_no_sep = lpath;
#ifdef __WINDOWS__
    lpath_no_sep.Replace( "/", " " );
    lpath_no_sep.Replace( "\\", " " );
#endif

    if( wxString::npos != lpath_no_sep.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}


bool FILENAME_RESOLVER::GetKicadPaths( std::list< wxString >& paths ) const
{
    paths.clear();

    if( !m_pgm )
        return false;

    bool hasKisys3D = false;


    // iterate over the list of internally defined ENV VARs
    // and add them to the paths list
    ENV_VAR_MAP_CITER mS = m_pgm->GetLocalEnvVariables().begin();
    ENV_VAR_MAP_CITER mE = m_pgm->GetLocalEnvVariables().end();

    while( mS != mE )
    {
        // filter out URLs, template directories, and known system paths
        if( mS->first == wxString( "KICAD_PTEMPLATES" )
            || mS->first == wxString( "KICAD6_FOOTPRINT_DIR" ) )
        {
            ++mS;
            continue;
        }

        if( wxString::npos != mS->second.GetValue().find( wxString( "://" ) ) )
        {
            ++mS;
            continue;
        }

        wxString tmp( "${" );
        tmp.Append( mS->first );
        tmp.Append( "}" );
        paths.push_back( tmp );

        if( tmp == "${KICAD6_3DMODEL_DIR}" )
            hasKisys3D = true;

        ++mS;
    }

    if( !hasKisys3D )
        paths.emplace_back("${KICAD6_3DMODEL_DIR}" );

    return true;
}
