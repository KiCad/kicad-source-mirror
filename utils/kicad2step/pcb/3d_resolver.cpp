/*
 * This program source code file is part kicad2mcad
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include "kicadpcb.h"

#include "3d_resolver.h"

// configuration file version
#define CFGFILE_VERSION 1
#define S3D_RESOLVER_CONFIG "3Dresolver.cfg"

// flag bits used to track different one-off messages to users
#define ERRFLG_ALIAS    (1)
#define ERRFLG_RELPATH  (2)
#define ERRFLG_ENVPATH  (4)

#define MASK_3D_RESOLVER "3D_RESOLVER"

static wxCriticalSection lock3D_resolver;

static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult );

S3D_RESOLVER::S3D_RESOLVER()
{
    m_errflags = 0;
}


bool S3D_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    if( aConfigDir.empty() )
        return false;

    wxFileName cfgdir( aConfigDir, "" );
    cfgdir.Normalize();

    if( false == cfgdir.DirExists() )
        return false;

    wxFileName kcom( cfgdir.GetPath(), "" );
    kcom.RemoveLastDir();
    kcom.SetFullName( "kicad_common" );
    wxUniChar psep = wxFileName::GetPathSeparator();

    if( kcom.FileExists() )
    {
        wxConfigBase* cfg = new wxFileConfig( "", "", kcom.GetFullPath() );
        wxString subgroup( "EnvironmentVariables" );
        wxString entry;
        wxString val;
        long idx = 0;

        if( cfg->HasGroup( subgroup ) )
        {
            cfg->SetPath( subgroup );

            while( cfg->GetNextEntry( entry, idx ) )
            {
                val = cfg->Read( entry, "" );

                if( val.Last() == psep )
                    val = val.substr( 0, val.length() -1 );

                // only add the EnvVar if it is not currently defined by the shell
                wxString subst = "${";
                subst.append( entry );
                subst.append( "}" );
                wxString exvar = wxExpandEnvVars( subst );

                if( exvar == subst )
                    m_EnvVars.insert(std::pair< wxString, wxString >(entry, val));

                entry.clear();
                val.clear();
            }
        }

        delete cfg;
    }

    m_ConfigDir = cfgdir.GetPath();
    createPathList();

    return true;
}


bool S3D_RESOLVER::SetProjectDir( const wxString& aProjDir, bool* flgChanged )
{
    if( aProjDir.empty() )
        return false;

    wxFileName projdir( aProjDir, "" );
    projdir.Normalize();

    if( false == projdir.DirExists() )
        return false;

    m_curProjDir = projdir.GetPath();
    wxSetEnv( "KIPRJMOD", m_curProjDir );

    if( flgChanged )
        *flgChanged = false;

    if( m_Paths.empty() )
    {
        S3D_ALIAS al;
        al.m_alias = "${KIPRJMOD}";
        al.m_pathvar = "${KIPRJMOD}";
        al.m_pathexp = m_curProjDir;
        m_Paths.push_back( al );
        m_NameMap.clear();

        if( flgChanged )
            *flgChanged = true;

    }
    else
    {
        if( m_Paths.front().m_pathexp.Cmp( m_curProjDir ) )
        {
            m_Paths.front().m_pathexp = m_curProjDir;
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


wxString S3D_RESOLVER::GetProjectDir( void )
{
    return m_curProjDir;
}


bool S3D_RESOLVER::createPathList( void )
{
    if( !m_Paths.empty() )
        return true;

    wxString kmod;

    // add an entry for the default search path; at this point
    // we cannot set a sensible default so we use an empty string.
    // the user may change this later with a call to SetProjectDir()

    S3D_ALIAS lpath;
    lpath.m_alias = "${KIPRJMOD}";
    lpath.m_pathvar = "${KIPRJMOD}";
    lpath.m_pathexp = m_curProjDir;
    m_Paths.push_back( lpath );
    wxFileName fndummy;
    wxUniChar psep = fndummy.GetPathSeparator();
    bool hasKISYS3DMOD = false;

    // iterate over the list of internally defined ENV VARs
    // and add existing paths to the resolver
    std::map< wxString, wxString >::const_iterator mS = m_EnvVars.begin();
    std::map< wxString, wxString >::const_iterator mE = m_EnvVars.end();

    while( mS != mE )
    {
        // filter out URLs, template directories, and known system paths
        if( mS->first == wxString( "KICAD_PTEMPLATES" )
            || mS->first == wxString( "KIGITHUB" )
            || mS->first == wxString( "KISYSMOD" ) )
        {
            ++mS;
            continue;
        }

        if( wxString::npos != mS->second.find( wxString( "://" ) ) )
        {
            ++mS;
            continue;
        }

        fndummy.Assign( mS->second, "" );
        wxString pathVal;

        // ensure system ENV VARs supercede internally defined vars
        if( wxGetEnv( mS->first, &pathVal ) && wxDirExists( pathVal ) )
            fndummy.Assign( pathVal, "" );
        else
            fndummy.Assign( mS->second, "" );

        fndummy.Normalize();

        if( !fndummy.DirExists() )
        {
            ++mS;
            continue;
        }

        wxString tmp( "${" );
        tmp.Append( mS->first );
        tmp.Append( "}" );

        if( tmp == "${KISYS3DMOD}" )
            hasKISYS3DMOD = true;

        lpath.m_alias =  tmp;
        lpath.m_pathvar = tmp;
        lpath.m_pathexp = fndummy.GetFullPath();

        if( !lpath.m_pathexp.empty() && psep == *lpath.m_pathexp.rbegin() )
            lpath.m_pathexp.erase( --lpath.m_pathexp.end() );

        m_Paths.push_back( lpath );

        ++mS;
    }

    // special case: if KISYSMOD is not internally defined but is defined by
    // the system, then create an entry here
    wxString envar;

    if( !hasKISYS3DMOD && wxGetEnv( "KISYS3DMOD", &envar ) )
    {
        lpath.m_alias = "${KISYS3DMOD}";
        lpath.m_pathvar = "${KISYS3DMOD}";
        fndummy.Assign( envar, "" );
        fndummy.Normalize();

        if( fndummy.DirExists() )
        {
            lpath.m_pathexp = fndummy.GetFullPath();

            if( !lpath.m_pathexp.empty() && psep == *lpath.m_pathexp.rbegin() )
                lpath.m_pathexp.erase( --lpath.m_pathexp.end() );

            if( !lpath.m_pathexp.empty() )
                m_Paths.push_back( lpath );
        }

    }

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


bool S3D_RESOLVER::UpdatePathList( std::vector< S3D_ALIAS >& aPathList )
{
    wxUniChar envMarker( '$' );

    while( !m_Paths.empty() && envMarker != *m_Paths.back().m_alias.rbegin() )
        m_Paths.pop_back();

    size_t nI = aPathList.size();

    for( size_t i = 0; i < nI; ++i )
        addPath( aPathList[i] );

    return writePathList();
}


wxString S3D_RESOLVER::ResolvePath( const wxString& aFileName )
{
    wxCriticalSectionLocker lock( lock3D_resolver );

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
    tname.Replace( "/", "\\" );
    #endif

    // Note: variable expansion must preferably be performed via a
    // threadsafe wrapper for the getenv() system call. If we allow the
    // wxFileName::Normalize() routine to perform expansion then
    // we will have a race condition since wxWidgets does not assure
    // a threadsafe wrapper for getenv().
    if( tname.StartsWith( "${" ) || tname.StartsWith( "$(" ) )
        tname = expandVars( tname );

    wxFileName tmpFN( tname );

    // in the case of absolute filenames we don't store a map item
    if( !aFileName.StartsWith( "${" ) && !aFileName.StartsWith( "$(" )
        && !aFileName.StartsWith( ":" ) && tmpFN.IsAbsolute() )
    {
        tmpFN.Normalize();

        if( tmpFN.FileExists() )
            return tmpFN.GetFullPath();

        return wxEmptyString;
    }

    // this case covers full paths, leading expanded vars, and paths
    // relative to the current working directory (which is not necessarily
    // the current project directory)
    if( tmpFN.FileExists() )
    {
        tmpFN.Normalize();
        tname = tmpFN.GetFullPath();
        m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

        // special case: if a path begins with ${ENV_VAR} but is not in the
        // resolver's path list then add it
        if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
            checkEnvVarPath( aFileName );

        return tname;
    }

    // if a path begins with ${ENV_VAR}/$(ENV_VAR) and is not resolved then the
    // file either does not exist or the ENV_VAR is not defined
    if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
    {
        if( !( m_errflags & ERRFLG_ENVPATH ) )
        {
            m_errflags |= ERRFLG_ENVPATH;
            wxString errmsg = "[3D File Resolver] No such path; ensure the environment var is defined";
            errmsg.append( "\n" );
            errmsg.append( tname );
            wxLogMessage( "%s\n", errmsg.ToUTF8() );
        }

        return wxEmptyString;
    }

    // at this point aFileName is:
    // a. an aliased shortened name or
    // b. cannot be determined

    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    // check the path relative to the current project directory;
    // note: this is not necessarily the same as the current working
    // directory, which has already been checked. This case accounts
    // for partial paths which do not contain ${KIPRJMOD}.
    // This check is performed before checking the path relative to
    // ${KISYS3DMOD} so that users can potentially override a model
    // within ${KISYS3DMOD}
    if( !sPL->m_pathexp.empty() && !tname.StartsWith( ":" ) )
    {
        tmpFN.Assign( sPL->m_pathexp, "" );
        wxString fullPath = tmpFN.GetPathWithSep() + tname;

        if( fullPath.StartsWith( "${" ) || fullPath.StartsWith( "$(" ) )
            fullPath = expandVars( fullPath );

        if( wxFileName::FileExists( fullPath ) )
        {
            tmpFN.Assign( fullPath );
            tmpFN.Normalize();
            tname = tmpFN.GetFullPath();
            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

            return tname;
        }

    }

    // check the partial path relative to ${KISYS3DMOD} (legacy behavior)
    if( !tname.StartsWith( ":" ) )
    {
        wxFileName fpath;
        wxString fullPath( "${KISYS3DMOD}" );
        fullPath.Append( fpath.GetPathSeparator() );
        fullPath.Append( tname );
        fullPath = expandVars( fullPath );
        fpath.Assign( fullPath );

        if( fpath.Normalize() && fpath.FileExists() )
        {
            tname = fpath.GetFullPath();
            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );
            return tname;
        }

    }

    // ${ENV_VAR} paths have already been checked; skip them
    while( sPL != ePL && ( sPL->m_alias.StartsWith( "${" )
                           || sPL->m_alias.StartsWith( "$(" ) ) )
        ++sPL;

    // at this point the filename must contain an alias or else it is invalid
    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( tname, alias, relpath ) )
    {
        if( !( m_errflags & ERRFLG_RELPATH ) )
        {
            // this can happen if the file was intended to be relative to
            // ${KISYS3DMOD} but ${KISYS3DMOD} not set or incorrect.
            m_errflags |= ERRFLG_RELPATH;
            wxString errmsg = "[3D File Resolver] No such path";
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

            if( fullPath.StartsWith( "${") || fullPath.StartsWith( "$(" ) )
                fullPath = expandVars( fullPath );

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
        wxString errmsg = "[3D File Resolver] No such path; ensure the path alias is defined";
        errmsg.append( "\n" );
        errmsg.append( tname.substr( 1 ) );
        wxLogTrace( MASK_3D_RESOLVER, "%s\n", errmsg.ToUTF8() );
    }

    return wxEmptyString;
}


bool S3D_RESOLVER::addPath( const S3D_ALIAS& aPath )
{
    if( aPath.m_alias.empty() || aPath.m_pathvar.empty() )
        return false;

    wxCriticalSectionLocker lock( lock3D_resolver );

    S3D_ALIAS tpath = aPath;

    #ifdef _WIN32
    while( tpath.m_pathvar.EndsWith( "\\" ) )
        tpath.m_pathvar.erase( tpath.m_pathvar.length() - 1 );
    #else
    while( tpath.m_pathvar.EndsWith( "/" ) &&  tpath.m_pathvar.length() > 1 )
        tpath.m_pathvar.erase( tpath.m_pathvar.length() - 1 );
    #endif

    wxFileName path( tpath.m_pathvar, "" );
    path.Normalize();

    if( !path.DirExists() )
    {
        // suppress the message if the missing pathvar is the
        // legacy KISYS3DMOD variable
        if( aPath.m_pathvar.compare( "${KISYS3DMOD}" ) )
        {
            wxString msg = _( "The given path does not exist" );
            msg.append( "\n" );
            msg.append( tpath.m_pathvar );
            wxLogMessage( "%s\n", msg.ToUTF8() );
        }

        tpath.m_pathexp.clear();
    }
    else
    {
        tpath.m_pathexp = path.GetFullPath();

        #ifdef _WIN32
        while( tpath.m_pathexp.EndsWith( "\\" ) )
        tpath.m_pathexp.erase( tpath.m_pathexp.length() - 1 );
        #else
        while( tpath.m_pathexp.EndsWith( "/" ) &&  tpath.m_pathexp.length() > 1 )
            tpath.m_pathexp.erase( tpath.m_pathexp.length() - 1 );
        #endif
    }

    wxString pname = path.GetPath();
    std::list< S3D_ALIAS >::iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( !tpath.m_alias.Cmp( sPL->m_alias ) )
        {
            wxString msg = _( "Alias: " );
            msg.append( tpath.m_alias );
            msg.append( "\n" );
            msg.append( _( "This path: " ) );
            msg.append( tpath.m_pathvar );
            msg.append( "\n" );
            msg.append( _( "Existing path: " ) );
            msg.append( sPL->m_pathvar );
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
    if( m_ConfigDir.empty() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "3D configuration directory is unknown";
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
    S3D_ALIAS al;
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
        if( !al.m_alias.Cmp( "KISYS3DMOD" ) )
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


bool S3D_RESOLVER::writePathList( void )
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

    // skip all ${ENV_VAR} alias names
    std::list< S3D_ALIAS >::const_iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator ePL = m_Paths.end();

    while( sPL != ePL && ( sPL->m_alias.StartsWith( "${" )
        || sPL->m_alias.StartsWith( "$(" ) ) )
        ++sPL;

    wxFileName cfgpath( m_ConfigDir, S3D_RESOLVER_CONFIG );
    wxString cfgname = cfgpath.GetFullPath();
    std::ofstream cfgFile;

    if( sPL == ePL )
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
    std::string tstr;

    while( sPL != ePL )
    {
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
    std::list< S3D_ALIAS >::iterator sPL = m_Paths.begin();
    std::list< S3D_ALIAS >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( sPL->m_alias == envar )
            return;

        if( !sPL->m_alias.StartsWith( "${" ) )
            break;

        ++sPL;
    }

    S3D_ALIAS lpath;
    lpath.m_alias = envar;
    lpath.m_pathvar = lpath.m_alias;
    wxFileName tmpFN( lpath.m_alias, "" );
    wxUniChar psep = tmpFN.GetPathSeparator();
    tmpFN.Normalize();

    if( !tmpFN.DirExists() )
        return;

    lpath.m_pathexp = tmpFN.GetFullPath();

    if( !lpath.m_pathexp.empty() && psep == *lpath.m_pathexp.rbegin() )
        lpath.m_pathexp.erase( --lpath.m_pathexp.end() );

    if( lpath.m_pathexp.empty() )
        return;

    m_Paths.insert( sPL, lpath );
    return;
}


wxString S3D_RESOLVER::expandVars( const wxString& aPath )
{
    if( aPath.empty() )
        return wxEmptyString;

    wxString result;

    for( auto i: m_EnvVars )
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

    wxCriticalSectionLocker lock( lock3D_resolver );
    std::list< S3D_ALIAS >::const_iterator sL = m_Paths.begin();
    std::list< S3D_ALIAS >::const_iterator eL = m_Paths.end();
    size_t idx;

    while( sL != eL )
    {
        // undefined paths do not participate in the
        // file name shortening procedure
        if( sL->m_pathexp.empty() )
        {
            ++sL;
            continue;
        }

        wxFileName fpath( sL->m_pathexp, "" );
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

            if( sL->m_alias.StartsWith( "${" ) || sL->m_alias.StartsWith( "$(" ) )
            {
                // old style ENV_VAR
                tname = sL->m_alias;
                tname.Append( "/" );
                tname.append( fname );
            }
            else
            {
                // new style alias
                tname = ":";
                tname.append( sL->m_alias );
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



const std::list< S3D_ALIAS >* S3D_RESOLVER::GetPaths( void )
{
    return &m_Paths;
}


bool S3D_RESOLVER::SplitAlias( const wxString& aFileName,
    wxString& anAlias, wxString& aRelPath )
{
    anAlias.clear();
    aRelPath.clear();

    if( !aFileName.StartsWith( ":" ) )
        return false;

    size_t tagpos = aFileName.find( ":", 1 );

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
    size_t pos0 = aFileName.find( ':' );

    // ensure that the file separators suit the current platform
    #ifdef __WINDOWS__
    filename.Replace( "/", "\\" );

    // if we see the :\ pattern then it must be a drive designator
    if( pos0 != wxString::npos )
    {
        size_t pos1 = aFileName.find( ":\\" );

        if( pos1 != wxString::npos && ( pos1 != pos0 || pos1 != 1 ) )
            return false;

        // if we have a drive designator then we have no alias
        if( pos1 != wxString::npos )
            pos0 = wxString::npos;
    }
    #else
    filename.Replace( "\\", "/" );
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

        // check the alias for restricted characters
        if( wxString::npos != lpath.find_first_of( "{}[]()%~<>\"='`;:.,&?/\\|$" ) )
            return false;

        hasAlias = true;
        lpath = aFileName.substr( pos0 + 1 );
    }
    else
    {
        lpath = aFileName;

        // in the case of ${ENV_VAR}|$(ENV_VAR)/path, strip the
        // environment string before testing
        pos0 = wxString::npos;

        if( aFileName.StartsWith( "${" ) )
            pos0 = aFileName.find( '}' );
        else if( aFileName.StartsWith( "$(" ) )
            pos0 = aFileName.find( ')' );

        if( pos0 != wxString::npos )
            lpath = aFileName.substr( pos0 + 1 );

    }

    if( wxString::npos != lpath.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}
