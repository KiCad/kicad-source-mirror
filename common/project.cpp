/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <wx/log.h>
#include <wx/stdpaths.h>                // required on Mac
#include <kiplatform/environment.h>

#include <algorithm>
#include <pgm_base.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <design_block_library_adapter.h>
#include <string_utils.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <lockfile.h>
#include <macros.h>
#include <git/project_git_utils.h>
#include <git2.h>
#include <project.h>

#include <footprint_library_adapter.h>

#include <project/project_file.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <title_block.h>
#include <local_history.h>



PROJECT::PROJECT() :
        m_readOnly( false ),
        m_textVarsTicker( 0 ),
        m_netclassesTicker( 0 ),
        m_projectFile( nullptr ),
        m_localSettings( nullptr )
{
    m_elems.fill( nullptr );
}


void PROJECT::elemsClear()
{
    // careful here, this should work, but the virtual destructor may not
    // be in the same link image as PROJECT.
    for( unsigned i = 0;  i < m_elems.size();  ++i )
    {
        SetElem( static_cast<PROJECT::ELEM>( i ), nullptr );
    }
}


PROJECT::~PROJECT()
{
    elemsClear();
}


bool PROJECT::TextVarResolver( wxString* aToken ) const
{
    if( aToken->IsSameAs( wxT( "PROJECTNAME" ) )  )
    {
        *aToken = GetProjectName();
        return true;
    }
    else if( aToken->IsSameAs( wxT( "CURRENT_DATE" ) )  )
    {
        *aToken = TITLE_BLOCK::GetCurrentDate();
        return true;
    }
    else if( aToken->IsSameAs( wxT( "VCSHASH" ) ) )
    {
        *aToken = KIGIT::PROJECT_GIT_UTILS::GetCurrentHash( GetProjectFullName(), false );
        return true;
    }
    else if( aToken->IsSameAs( wxT( "VCSSHORTHASH" ) ) )
    {
        *aToken = KIGIT::PROJECT_GIT_UTILS::GetCurrentHash( GetProjectFullName(), true );
        return true;
    }
    else if( GetTextVars().count( *aToken ) > 0 )
    {
        *aToken = GetTextVars().at( *aToken );
        return true;
    }

    return false;
}


std::map<wxString, wxString>& PROJECT::GetTextVars() const
{
    return GetProjectFile().m_TextVars;
}


void PROJECT::ApplyTextVars( const std::map<wxString, wxString>& aVarsMap )
{
    if( aVarsMap.size() == 0 )
        return;

    std::map<wxString, wxString>& existingVarsMap = GetTextVars();

    for( const auto& var : aVarsMap )
    {
        // create or update the existing vars
        existingVarsMap[var.first] = var.second;
    }
}


void PROJECT::setProjectFullName( const wxString& aFullPathAndName )
{
    // Compare paths, rather than inodes, to be less surprising to the user.
    // Create a temporary wxFileName to normalize the path
    wxFileName candidate_path( aFullPathAndName );

    // Edge transitions only.  This is what clears the project
    // data using the Clear() function.
    if( m_project_name.GetFullPath() != candidate_path.GetFullPath() )
    {
        Clear();            // clear the data when the project changes.

        wxLogTrace( tracePathsAndFiles, "%s: old:'%s' new:'%s'", __func__,
                    TO_UTF8( GetProjectFullName() ), TO_UTF8( aFullPathAndName ) );

        m_project_name = aFullPathAndName;

        wxASSERT( m_project_name.IsAbsolute() );

        wxASSERT( m_project_name.GetExt() == FILEEXT::ProjectFileExtension );
    }
}


const wxString PROJECT::GetProjectFullName() const
{
    return m_project_name.GetFullPath();
}


const wxString PROJECT::GetProjectPath() const
{
    return m_project_name.GetPathWithSep();
}


const wxString PROJECT::GetProjectDirectory() const
{
    return m_project_name.GetPath();
}


const wxString PROJECT::GetProjectName() const
{
    return m_project_name.GetName();
}


bool PROJECT::IsNullProject() const
{
    return m_project_name.GetName().IsEmpty();
}


const wxString PROJECT::SymbolLibTableName() const
{
    return libTableName( FILEEXT::SymbolLibraryTableFileName );
}


const wxString PROJECT::FootprintLibTblName() const
{
    return libTableName( FILEEXT::FootprintLibraryTableFileName );
}


const wxString PROJECT::DesignBlockLibTblName() const
{
    return libTableName( FILEEXT::DesignBlockLibraryTableFileName );
}


void PROJECT::PinLibrary( const wxString& aLibrary, enum LIB_TYPE_T aLibType )
{
    COMMON_SETTINGS*       cfg = Pgm().GetCommonSettings();
    std::vector<wxString>* pinnedLibsCfg = nullptr;
    std::vector<wxString>* pinnedLibsFile = nullptr;

    switch( aLibType )
    {
    case LIB_TYPE_T::SYMBOL_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedSymbolLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_symbol_libs;
        break;
    case LIB_TYPE_T::FOOTPRINT_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedFootprintLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_fp_libs;
        break;
    case LIB_TYPE_T::DESIGN_BLOCK_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedDesignBlockLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_design_block_libs;
        break;
    default:
        wxFAIL_MSG( "Cannot pin library: invalid library type" );
        return;
    }

    if( !alg::contains( *pinnedLibsFile, aLibrary ) )
        pinnedLibsFile->push_back( aLibrary );

    Pgm().GetSettingsManager().SaveProject();

    if( !alg::contains( *pinnedLibsCfg, aLibrary ) )
        pinnedLibsCfg->push_back( aLibrary );

    cfg->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( cfg ) );
}


void PROJECT::UnpinLibrary( const wxString& aLibrary, enum LIB_TYPE_T aLibType )
{
    COMMON_SETTINGS*       cfg = Pgm().GetCommonSettings();
    std::vector<wxString>* pinnedLibsCfg = nullptr;
    std::vector<wxString>* pinnedLibsFile = nullptr;

    switch( aLibType )
    {
    case LIB_TYPE_T::SYMBOL_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedSymbolLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_symbol_libs;
        break;
    case LIB_TYPE_T::FOOTPRINT_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedFootprintLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_fp_libs;
        break;
    case LIB_TYPE_T::DESIGN_BLOCK_LIB:
        pinnedLibsFile = &m_projectFile->m_PinnedDesignBlockLibs;
        pinnedLibsCfg = &cfg->m_Session.pinned_design_block_libs;
        break;
    default:
        wxFAIL_MSG( "Cannot unpin library: invalid library type" );
        return;
    }

    std::erase( *pinnedLibsFile, aLibrary );
    Pgm().GetSettingsManager().SaveProject();

    std::erase( *pinnedLibsCfg, aLibrary );
    cfg->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( cfg ) );
}


const wxString PROJECT::libTableName( const wxString& aLibTableName ) const
{
    wxFileName  fn = GetProjectFullName();
    wxString    path = fn.GetPath();

    // if there's no path to the project name, or the name as a whole is bogus or its not
    // write-able then use a template file.
    if( !fn.GetDirCount() || !fn.IsOk() || !wxFileName::IsDirWritable( path ) )
    {
        // return a template filename now.

        // this next line is likely a problem now, since it relies on an
        // application title which is no longer constant or known.  This next line needs
        // to be re-thought out.

#ifdef __WXMAC__
        fn.AssignDir( KIPLATFORM::ENV::GetUserConfigPath() );
#else
        // don't pollute home folder, temp folder seems to be more appropriate
        fn.AssignDir( wxStandardPaths::Get().GetTempDir() );
#endif

#if defined( __WINDOWS__ )
        fn.AppendDir( wxT( "kicad" ) );
#endif

        /*
         * The library table name used when no project file is passed to the appropriate
         * code.  This is used temporarily to store the project specific library table
         * until the project file being edited is saved.  It is then moved to the correct
         * file in the folder where the project file is saved.
         */
        fn.SetName( wxS( "prj-" ) + aLibTableName );
    }
    else    // normal path.
    {
        fn.SetName( aLibTableName );
    }

    fn.ClearExt();

    return fn.GetFullPath();
}


const wxString PROJECT::GetSheetName( const KIID& aSheetID )
{
    if( m_sheetNames.empty() )
    {
        for( const std::pair<KIID, wxString>& pair : GetProjectFile().GetSheets() )
            m_sheetNames[pair.first] = pair.second;
    }

    if( m_sheetNames.count( aSheetID ) )
        return m_sheetNames.at( aSheetID );
    else
        return aSheetID.AsString();
}


void PROJECT::SetRString( RSTRING_T aIndex, const wxString& aString )
{
    unsigned ndx = unsigned( aIndex );

    if( ndx < m_rstrings.size() )
        m_rstrings[ndx] = aString;
    else
        wxASSERT( 0 );      // bad index
}


const wxString& PROJECT::GetRString( RSTRING_T aIndex )
{
    unsigned ndx = unsigned( aIndex );

    if( ndx < m_rstrings.size() )
    {
        return m_rstrings[ndx];
    }
    else
    {
        static wxString no_cookie_for_you;

        wxASSERT( 0 );      // bad index

        return no_cookie_for_you;
    }
}


PROJECT::_ELEM* PROJECT::GetElem( PROJECT::ELEM aIndex )
{
    // This is virtual, so implement it out of line

    if( static_cast<unsigned>( aIndex ) < m_elems.size() )
        return m_elems[static_cast<unsigned>( aIndex )];

    return nullptr;
}


void PROJECT::SetElem( PROJECT::ELEM aIndex, _ELEM* aElem )
{
    // This is virtual, so implement it out of line
    if( static_cast<unsigned>( aIndex ) < m_elems.size() )
    {
        delete m_elems[static_cast<unsigned>(aIndex)];
        m_elems[static_cast<unsigned>( aIndex )] = aElem;
    }
}


const wxString PROJECT::AbsolutePath( const wxString& aFileName ) const
{
    wxFileName fn = aFileName;

    // Paths which start with an unresolved variable reference are more likely to be
    // absolute than relative.
    if( aFileName.StartsWith( wxT( "${" ) ) )
        return aFileName;

    if( !fn.IsAbsolute() )
    {
        wxString pro_dir = wxPathOnly( GetProjectFullName() );
        fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS, pro_dir );
    }

    return fn.GetFullPath();
}


FOOTPRINT_LIBRARY_ADAPTER* PROJECT::FootprintLibAdapter( KIWAY& aKiway )
{
    KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );
    return static_cast<FOOTPRINT_LIBRARY_ADAPTER*>( kiface->IfaceOrAddress( KIFACE_FOOTPRINT_LIBRARY_ADAPTER ) );
}


DESIGN_BLOCK_LIBRARY_ADAPTER* PROJECT::DesignBlockLibs()
{
    std::scoped_lock lock( m_designBlockLibsMutex );

    LIBRARY_MANAGER& mgr = Pgm().GetLibraryManager();
    std::optional<LIBRARY_MANAGER_ADAPTER*> adapter = mgr.Adapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK );

    if( !adapter )
    {
        mgr.RegisterAdapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK,
                             std::make_unique<DESIGN_BLOCK_LIBRARY_ADAPTER>( mgr ) );

        std::optional<LIBRARY_MANAGER_ADAPTER*> created = mgr.Adapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK );
        wxCHECK( created && ( *created )->Type() == LIBRARY_TABLE_TYPE::DESIGN_BLOCK, nullptr );
        return static_cast<DESIGN_BLOCK_LIBRARY_ADAPTER*>( *created );
    }

    wxCHECK( ( *adapter )->Type() == LIBRARY_TABLE_TYPE::DESIGN_BLOCK, nullptr );
    return static_cast<DESIGN_BLOCK_LIBRARY_ADAPTER*>( *adapter );
}


LOCKFILE* PROJECT::GetProjectLock() const
{
    return m_project_lock.get();
}


void PROJECT::SetProjectLock( LOCKFILE* aLockFile )
{
    m_project_lock.reset( aLockFile );
}


void PROJECT::SaveToHistory( const wxString& aProjectPath, std::vector<wxString>& aFiles )
{
    wxString projectFile = GetProjectFullName();

    if( projectFile.IsEmpty() )
        return;

    wxFileName projectFn( projectFile );
    wxFileName requestedFn( aProjectPath );
    // wxPATH_NORM_ALL is now deprecated.
    // So define a similar option
    int norm_opt = wxPATH_NORM_ENV_VARS|wxPATH_NORM_DOTS|wxPATH_NORM_TILDE|wxPATH_NORM_ABSOLUTE
                   |wxPATH_NORM_LONG|wxPATH_NORM_SHORTCUT;

    if( !projectFn.Normalize( norm_opt ) || !requestedFn.Normalize( norm_opt ) )
        return;

    if( projectFn.GetFullPath() != requestedFn.GetFullPath() )
        return;

    wxFileName historyDir( projectFn.GetPath(), wxS( ".history" ) );

    if( !historyDir.DirExists() )
    {
        if( !historyDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
            return;
    }

    // Save project file (.kicad_pro)
    wxFileName historyProFile( historyDir.GetFullPath(), projectFn.GetName(),
                               projectFn.GetExt() );
    wxCopyFile( projectFile, historyProFile.GetFullPath(), true );
    aFiles.push_back( historyProFile.GetFullPath() );

    // Save project local settings (.kicad_prl) if it exists
    wxFileName prlFile( projectFn.GetPath(), projectFn.GetName(), FILEEXT::ProjectLocalSettingsFileExtension );

    if( prlFile.FileExists() )
    {
        wxFileName historyPrlFile( historyDir.GetFullPath(), prlFile.GetName(),
                                   prlFile.GetExt() );
        wxCopyFile( prlFile.GetFullPath(), historyPrlFile.GetFullPath(), true );
        aFiles.push_back( historyPrlFile.GetFullPath() );
    }
}
