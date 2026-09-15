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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "qa_utils/file_utils.h"

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <wx_filename.h>

#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


using namespace KI_TEST;


/**
 * Report a message about a temporary directory.
 *
 * This cannot use wx logging as it can be called outside the WX
 * Init/Uninit lifetime, so it goes to stderr.
 */
static void reportTempDir( const wxString& aMessage )
{
    fprintf( stderr, "%s\n", aMessage.utf8_string().c_str() );
}


static bool shouldKeepTemp( const std::filesystem::path& aPath, const wxString& aKeepEnvValue )
{
    const wxString envVar = wxT( "KICAD_QA_KEEP_TEMP" );
    wxString       keepEnv;

    if( !wxGetEnv( envVar, &keepEnv ) || keepEnv.IsEmpty() )
        return false;

    // Split the environment variable on commas (like WXTRACE)
    wxStringTokenizer tokenizer( keepEnv, wxT( "," ) );

    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();

        if( token == aKeepEnvValue || token == wxT( "ALL" ) )
        {
            // Probably do want to see this, because if you are keeping the temp dirs, you may
            // want to know which ones they are.
            reportTempDir( wxString::Format( wxT( "Keeping temporary directory '%s' because %s is set to '%s'" ),
                                             wxString::FromUTF8( aPath.string() ), envVar, keepEnv ) );
            return true;
        }
    }

    return false;
}


static std::filesystem::path createTempDir( const wxString& aPrefix )
{
    wxString tempFile = wxFileName::CreateTempFileName( aPrefix + "_" );

    if( tempFile.IsEmpty() )
        throw std::runtime_error( "Cannot create a temporary directory name with prefix '"
                                  + std::string( aPrefix.utf8_str() ) + "'" );

    if( !wxRemoveFile( tempFile ) )
        throw std::runtime_error( "Cannot reclaim temporary name '" + std::string( tempFile.utf8_str() ) + "'" );

    if( !wxMkdir( tempFile ) )
        throw std::runtime_error( "Cannot create temporary directory '" + std::string( tempFile.utf8_str() ) + "'" );

    return std::filesystem::path( std::string( tempFile.utf8_str() ) );
}


bool SCOPED_TEMP_DIR::s_anyTempDirRetained = false;


SCOPED_TEMP_DIR::SCOPED_TEMP_DIR( const wxString& aPrefix ) :
        m_path( createTempDir( aPrefix ) ),
        m_keep( shouldKeepTemp( m_path, aPrefix ) )
{
    if( m_keep )
        s_anyTempDirRetained = true;
}


SCOPED_TEMP_DIR::~SCOPED_TEMP_DIR()
{
    if( m_keep )
        return;

    try
    {
        std::filesystem::remove_all( m_path );
    }
    catch( const std::filesystem::filesystem_error& e )
    {
        reportTempDir( wxString::Format( wxT( "Cannot remove temporary directory '%s': %s" ),
                                         wxString::FromUTF8( m_path.string() ), wxString::FromUTF8( e.what() ) ) );
    }
}


void SCOPED_TEMP_DIR::Retain()
{
    m_keep = true;
    s_anyTempDirRetained = true;

    reportTempDir(
            wxString::Format( wxT( "Keeping temporary directory '%s'" ), wxString::FromUTF8( m_path.string() ) ) );
}


wxString SCOPED_TEMP_DIR::ChildPathStr( const wxString& aName ) const
{
    return wxString::FromUTF8( ( m_path / aName.utf8_string() ).string() );
}


std::filesystem::path SCOPED_TEMP_DIR::CreateChildDir( const wxString& aName ) const
{
    std::filesystem::path childPath = m_path / aName.utf8_string();

    if( !std::filesystem::create_directory( childPath ) )
        throw std::runtime_error( "Cannot create temporary child directory '" + childPath.string() + "'" );

    return childPath;
}


wxString SCOPED_TEMP_DIR::CreateChildDirStr( const wxString& aName ) const
{
    return wxString::FromUTF8( CreateChildDir( aName ).string() );
}


std::filesystem::path SCOPED_TEMP_DIR::CreateChildFile( const wxString& aName ) const
{
    std::filesystem::path childPath = m_path / aName.utf8_string();
    wxFFile               file( wxString::FromUTF8( childPath.string() ), wxT( "w" ) );

    if( !file.IsOpened() )
        throw std::runtime_error( "Cannot create temporary child file '" + childPath.string() + "'" );

    return childPath;
}


wxString SCOPED_TEMP_DIR::CreateChildFileStr( const wxString& aName ) const
{
    return wxString::FromUTF8( CreateChildFile( aName ).string() );
}


SCOPED_TEMP_PROJECT::SCOPED_TEMP_PROJECT( SETTINGS_MANAGER& aManager, const wxString& aPrefix,
                                          const wxString& aName ) :
        m_dir( aPrefix ),
        m_manager( aManager ),
        m_project( nullptr )
{
    wxFileName projectFile( m_dir.PathStr(), aName, FILEEXT::ProjectFileExtension );

    m_manager.LoadProject( projectFile.GetFullPath() );
    m_project = m_manager.GetProject( projectFile.GetFullPath() );

    if( !m_project )
    {
        throw std::runtime_error( "Cannot load temporary project '"
                                  + std::string( projectFile.GetFullPath().utf8_str() ) + "'" );
    }
}


SCOPED_TEMP_PROJECT::~SCOPED_TEMP_PROJECT()
{
    m_manager.UnloadProject( m_project, false );
}


SCOPED_PROCESS_TEMP_DIR::SCOPED_PROCESS_TEMP_DIR( const wxString& aPrefix ) :
        m_dir( aPrefix )
{
    const wxString envValue = m_dir.PathStr();

    wxSetEnv( wxT( "TMPDIR" ), envValue ); // This is the POSIX one
    wxSetEnv( wxT( "TEMP" ), envValue );
    wxSetEnv( wxT( "TMP" ), envValue );
}


SCOPED_PROCESS_TEMP_DIR::~SCOPED_PROCESS_TEMP_DIR()
{
    // If any temporary directory has been retained, we must retain this one too
    if( SCOPED_TEMP_DIR::AnyTempDirRetained() )
        m_dir.Retain();
}
