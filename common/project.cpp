/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <design_block_lib_table.h>
#include <fp_lib_table.h>
#include <string_utils.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <macros.h>
#include <project.h>
#include <project/project_file.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>

PROJECT::PROJECT() :
        m_readOnly( false ),
        m_textVarsTicker( 0 ),
        m_netclassesTicker( 0 ),
        m_projectFile( nullptr ),
        m_localSettings( nullptr )
{
    m_elems.fill( nullptr );
}


void PROJECT::ElemsClear()
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
    ElemsClear();
}


bool PROJECT::TextVarResolver( wxString* aToken ) const
{
    if( GetTextVars().count( *aToken ) > 0 )
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
    return libTableName( wxS( "sym-lib-table" ) );
}


const wxString PROJECT::FootprintLibTblName() const
{
    return libTableName( wxS( "fp-lib-table" ) );
}


const wxString PROJECT::DesignBlockLibTblName() const
{
    return libTableName( wxS( "design-block-lib-table" ) );
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

    alg::delete_matching( *pinnedLibsFile, aLibrary );
    Pgm().GetSettingsManager().SaveProject();

    alg::delete_matching( *pinnedLibsCfg, aLibrary );
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


FP_LIB_TABLE* PROJECT::PcbFootprintLibs( KIWAY& aKiway )
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE* tbl = (FP_LIB_TABLE*) GetElem( PROJECT::ELEM::FPTBL );

    if( tbl )
    {
        wxASSERT( tbl->ProjectElementType() == PROJECT::ELEM::FPTBL );
    }
    else
    {
        try
        {
            // Build a new project specific FP_LIB_TABLE with the global table as a fallback.
            // ~FP_LIB_TABLE() will not touch the fallback table, so multiple projects may
            // stack this way, all using the same global fallback table.
            KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

            tbl = (FP_LIB_TABLE*) kiface->IfaceOrAddress( KIFACE_NEW_FOOTPRINT_TABLE );
            tbl->Load( FootprintLibTblName() );

            SetElem( PROJECT::ELEM::FPTBL, tbl );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project footprint library table." ),
                                 ioe.What() );
        }
        catch( ... )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project footprint library table." ) );
        }
    }

    return tbl;
}


DESIGN_BLOCK_LIB_TABLE* PROJECT::DesignBlockLibs()
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    DESIGN_BLOCK_LIB_TABLE* tbl = (DESIGN_BLOCK_LIB_TABLE*) GetElem( ELEM::DESIGN_BLOCK_LIB_TABLE );

    if( tbl )
    {
        wxASSERT( tbl->ProjectElementType() == PROJECT::ELEM::DESIGN_BLOCK_LIB_TABLE );
    }
    else
    {
        try
        {
            tbl = new DESIGN_BLOCK_LIB_TABLE( &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() );
            tbl->Load( DesignBlockLibTblName() );

            SetElem( ELEM::DESIGN_BLOCK_LIB_TABLE, tbl );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project design block library table." ),
                                 ioe.What() );
        }
        catch( ... )
        {
            DisplayErrorMessage( nullptr,
                                 _( "Error loading project design block library table." ) );
        }
    }

    return tbl;
}
