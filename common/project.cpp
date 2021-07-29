/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/stdpaths.h>

#include <confirm.h>
#include <core/arraydim.h>
#include <fp_lib_table.h>
#include <string_utils.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <macros.h>
#include <project.h>
#include <project/project_file.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>


PROJECT::PROJECT() :
        m_readOnly( false ),
        m_projectFile( nullptr ),
        m_localSettings( nullptr )
{
    memset( m_elems, 0, sizeof( m_elems ) );
}


void PROJECT::ElemsClear()
{
    // careful here, this should work, but the virtual destructor may not
    // be in the same link image as PROJECT.
    for( unsigned i = 0;  i < arrayDim( m_elems );  ++i )
    {
        SetElem( ELEM_T( i ), nullptr );
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

        wxASSERT( m_project_name.GetExt() == ProjectFileExtension );

        // until multiple projects are in play, set an environment variable for the
        // the project pointer.
        {
            wxString path = m_project_name.GetPath();

            wxSetEnv( PROJECT_VAR_NAME, path );
        }
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
    return libTableName( "sym-lib-table" );
}


const wxString PROJECT::FootprintLibTblName() const
{
    return libTableName( "fp-lib-table" );
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

#ifndef __WXMAC__
        fn.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );
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
        fn.SetName( "prj-" + aLibTableName );
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
        for( auto pair : GetProjectFile().GetSheets() )
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

    if( ndx < arrayDim( m_rstrings ) )
    {
        m_rstrings[ndx] = aString;
    }
    else
    {
        wxASSERT( 0 );      // bad index
    }
}


const wxString& PROJECT::GetRString( RSTRING_T aIndex )
{
    unsigned ndx = unsigned( aIndex );

    if( ndx < arrayDim( m_rstrings ) )
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


PROJECT::_ELEM* PROJECT::GetElem( ELEM_T aIndex )
{
    // This is virtual, so implement it out of line
    if( unsigned( aIndex ) < arrayDim( m_elems ) )
    {
        return m_elems[aIndex];
    }

    return nullptr;
}


void PROJECT::SetElem( ELEM_T aIndex, _ELEM* aElem )
{
    // This is virtual, so implement it out of line
    if( unsigned( aIndex ) < arrayDim( m_elems ) )
    {
        delete m_elems[aIndex];
        m_elems[aIndex] = aElem;
    }
}


const wxString PROJECT::AbsolutePath( const wxString& aFileName ) const
{
    wxFileName fn = aFileName;

    if( !fn.IsAbsolute() )
    {
        wxString pro_dir = wxPathOnly( GetProjectFullName() );
        fn.Normalize( wxPATH_NORM_ALL, pro_dir );
    }

    return fn.GetFullPath();
}


FP_LIB_TABLE* PROJECT::PcbFootprintLibs( KIWAY& aKiway )
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE* tbl = (FP_LIB_TABLE*) GetElem( ELEM_FPTBL );

    if( tbl )
    {
        wxASSERT( tbl->Type() == FP_LIB_TABLE_T );
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

            SetElem( ELEM_FPTBL, tbl );
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
