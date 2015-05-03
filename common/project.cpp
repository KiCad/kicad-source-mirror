
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <wx/stdpaths.h>

#include <fctsys.h>
#include <macros.h>
#include <pgm_base.h>
#include <project.h>
#include <common.h>         // NAMELESS_PROJECT
#include <confirm.h>
#include <kicad_string.h>
#include <config_params.h>
#include <wildcards_and_files_ext.h>


PROJECT::PROJECT()
{
    memset( m_elems, 0, sizeof(m_elems) );
}


void PROJECT::ElemsClear()
{
    DBG( printf( "%s: clearing all _ELEMS for project %s\n", __func__, TO_UTF8( GetProjectFullName() ) );)

    // careful here, this should work, but the virtual destructor may not
    // be in the same link image as PROJECT.
    for( unsigned i = 0;  i < DIM( m_elems );  ++i )
    {
        SetElem( ELEM_T( i ), NULL );
    }
}


PROJECT::~PROJECT()
{
    ElemsClear();
}


void PROJECT::SetProjectFullName( const wxString& aFullPathAndName )
{
    // Edge transitions only.  This is what clears the project
    // data using the Clear() function.
    if( m_project_name != aFullPathAndName )
    {
        Clear();            // clear the data when the project changes.

        DBG(printf( "%s: old:'%s' new:'%s'\n", __func__, TO_UTF8( GetProjectFullName() ), TO_UTF8( aFullPathAndName ) );)

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


const wxString PROJECT::FootprintLibTblName() const
{
    wxFileName  fn = GetProjectFullName();
    wxString    path = fn.GetPath();

    // DBG(printf( "path:'%s'  fn:'%s'\n", TO_UTF8(path), TO_UTF8(fn.GetFullPath()) );)

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
            The footprint library table name used when no project file is passed
            to Pcbnew or CvPcb. This is used temporarily to store the project
            specific library table until the project file being edited is saved.
            It is then moved to the file fp-lib-table in the folder where the
            project file is saved.
        */
        fn.SetName( wxT( "prj-fp-lib-table" ) );
    }
    else    // normal path.
    {
        fn.SetName( wxT( "fp-lib-table" ) );
    }

    fn.ClearExt();

    return fn.GetFullPath();
}


void PROJECT::SetRString( RSTRING_T aIndex, const wxString& aString )
{
    unsigned ndx = unsigned( aIndex );

    if( ndx < DIM( m_rstrings ) )
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

    if( ndx < DIM( m_rstrings ) )
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

    if( unsigned( aIndex ) < DIM( m_elems ) )
    {
        return m_elems[aIndex];
    }
    return NULL;
}


void PROJECT::SetElem( ELEM_T aIndex, _ELEM* aElem )
{
    // This is virtual, so implement it out of line

    if( unsigned( aIndex ) < DIM( m_elems ) )
    {
#if defined(DEBUG) && 0
        if( aIndex == ELEM_SCH_PART_LIBS )
        {
            printf( "%s: &m_elems[%i]:%p  aElem:%p\n", __func__, aIndex, &m_elems[aIndex], aElem );
        }
#endif
        delete m_elems[aIndex];
        m_elems[aIndex] = aElem;
    }
}


static bool copy_pro_file_template( const SEARCH_STACK& aSearchS, const wxString& aDestination )
{
    if( aDestination.IsEmpty() )
    {
        DBG( printf( "%s: destination is empty.\n", __func__ );)
        return false;
    }

    wxString templateFile = wxT( "kicad." ) + ProjectFileExtension;

    wxString kicad_pro_template = aSearchS.FindValidPath( templateFile );

    if( !kicad_pro_template )
    {
        DBG( printf( "%s: template file '%s' not found using search paths.\n", __func__, TO_UTF8( templateFile ) );)

        wxFileName  templ( wxStandardPaths::Get().GetDocumentsDir(),
                            wxT( "kicad" ), ProjectFileExtension );

        if( !templ.IsFileReadable() )
        {
            wxString msg = wxString::Format( _(
                    "Unable to find '%s' template config file." ),
                    GetChars( templateFile ) );

            DisplayError( NULL, msg );

            return false;
        }

        kicad_pro_template = templ.GetFullPath();
    }

    DBG( printf( "%s: using template file '%s' as project file.\n", __func__, TO_UTF8( kicad_pro_template ) );)

    wxCopyFile( kicad_pro_template, aDestination );

    return true;
}


wxConfigBase* PROJECT::configCreate( const SEARCH_STACK& aSList,
        const wxString& aGroupName, const wxString& aProjectFileName )
{
    wxConfigBase*   cfg = 0;
    wxString        cur_pro_fn = !aProjectFileName ? GetProjectFullName() : aProjectFileName;

    if( wxFileName( cur_pro_fn ).IsFileReadable() )
    {
        // Note: currently, aGroupName is not used.
        // Previoulsy, the version of aGroupName was tested, but it
        // was useless, and if the version is important,
        // this is not the right place here, because configCreate does know anything
        // about info stored in this config file.
        cfg = new wxFileConfig( wxEmptyString, wxEmptyString, cur_pro_fn, wxEmptyString );
        return cfg;
    }

    // No suitable pro file was found, either does not exist, or not readable.
    // Use the template kicad.pro file.  Find it by using caller's SEARCH_STACK.
    copy_pro_file_template( aSList, cur_pro_fn );

    cfg = new wxFileConfig( wxEmptyString, wxEmptyString, cur_pro_fn, wxEmptyString );

    return cfg;
}


void PROJECT::ConfigSave( const SEARCH_STACK& aSList, const wxString& aGroupName,
        const PARAM_CFG_ARRAY& aParams, const wxString& aFileName )
{
    std::auto_ptr<wxConfigBase> cfg( configCreate( aSList, aGroupName, aFileName ) );

    if( !cfg.get() )
    {
        // could not find template
        return;
    }

    cfg->SetPath( wxCONFIG_PATH_SEPARATOR );

    cfg->Write( wxT( "update" ), DateAndTime() );

    // @todo: pass in aLastClient wxString:
    cfg->Write( wxT( "last_client" ), Pgm().App().GetAppName() );

    // Save parameters
    cfg->DeleteGroup( aGroupName );     // Erase all data
    cfg->Flush();

    cfg->SetPath( aGroupName );
    cfg->Write( wxT( "version" ), CONFIG_VERSION );

    cfg->SetPath( wxCONFIG_PATH_SEPARATOR );

    wxConfigSaveParams( cfg.get(), aParams, aGroupName );

    cfg->SetPath( UNIX_STRING_DIR_SEP );

    // cfg is deleted here by std::auto_ptr, that saves the *.pro file to disk
}


bool PROJECT::ConfigLoad( const SEARCH_STACK& aSList, const wxString&  aGroupName,
        const PARAM_CFG_ARRAY& aParams, const wxString& aForeignProjectFileName )
{
    std::auto_ptr<wxConfigBase> cfg( configCreate( aSList, aGroupName, aForeignProjectFileName ) );

    if( !cfg.get() )
    {
        // could not find template
        return false;
    }

    cfg->SetPath( wxCONFIG_PATH_SEPARATOR );

    wxString timestamp = cfg->Read( wxT( "update" ) );

    m_pro_date_and_time = timestamp;

    // We do not want expansion of env var values when reading our project config file
    bool state = cfg.get()->IsExpandingEnvVars();
    cfg.get()->SetExpandEnvVars( false );

    wxConfigLoadParams( cfg.get(), aParams, aGroupName );

    cfg.get()->SetExpandEnvVars( state );

    return true;
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
