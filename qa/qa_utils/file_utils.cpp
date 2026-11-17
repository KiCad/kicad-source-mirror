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
#include <system_error>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/strconv.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <wx_filename.h>

#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#ifndef __WINDOWS__
#include <unistd.h>
#endif


using namespace KI_TEST;


namespace
{
/**
 * Report a message about a temporary directory.
 *
 * This cannot use wx logging as it can be called outside the WX
 * Init/Uninit lifetime, so it goes to stderr.
 */
void reportTempDir( const wxString& aMessage )
{
    fprintf( stderr, "%s\n", aMessage.utf8_string().c_str() );
}


// MSVC formats system errors in the ANSI codepage
wxString errorText( const std::error_code& aEc )
{
#ifdef _WIN32
    return wxString( aEc.message().c_str(), wxConvLocal );
#else
    return wxString::FromUTF8( aEc.message() );
#endif
}


/**
 * Remove a temporary directory. Failures are reported but this never thows,
 * so it is safe to call from destructors.
 */
void safeRemoveTempDir( const std::filesystem::path& aPath ) noexcept
{
    std::error_code ec;
    std::filesystem::remove_all( aPath, ec ); // use the noexcept version

    if( !ec )
        return;

    try
    {
        reportTempDir( wxString::Format( wxT( "Cannot remove temporary directory '%s': %s" ),
                                         KI_TEST::ToWxPath( aPath ), errorText( ec ) ) );
    }
    catch( ... )
    {
        // Even a log failure must not throw during unwinding.
    }
}


/**
 * Removes a temporary directory in its destructor unless released.
 *
 * It is armed immediately after the directory is claimed and released only
 * once its owner has fully taken responsibility for it, so a throw anywhere in
 * between cannot leak the directory.
 */
class TEMP_DIR_REMOVER
{
public:
    explicit TEMP_DIR_REMOVER( std::filesystem::path aPath ) noexcept :
            m_path( std::move( aPath ) )
    {
    }

    ~TEMP_DIR_REMOVER()
    {
        if( !m_released )
        {
            // Destructor-safe removal
            safeRemoveTempDir( m_path );
        }
    }

    TEMP_DIR_REMOVER( const TEMP_DIR_REMOVER& ) = delete;
    TEMP_DIR_REMOVER& operator=( const TEMP_DIR_REMOVER& ) = delete;

    const std::filesystem::path& Path() const noexcept { return m_path; }

    void Release() noexcept { m_released = true; }

private:
    std::filesystem::path m_path;
    bool                  m_released = false;
};


bool shouldKeepTemp( const std::filesystem::path& aPath, const wxString& aKeepEnvValue )
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
                                             KI_TEST::ToWxPath( aPath ), envVar, keepEnv ) );
            return true;
        }
    }

    return false;
}


std::filesystem::path createTempDir( const wxString& aPrefix, const wxString& aSuffix )
{
    std::error_code             ec;
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path( ec );

    if( ec )
        throw std::runtime_error( "Cannot determine the temporary directory: " + errorText( ec ).utf8_string() );

    // Claim the directory with a single mkdir. This avoids the race window
    // associated with reserving a name using wxFileName::CreateTempFileName().
    constexpr size_t k_maxAttempts = 1000;
    for( size_t i = 0; i < k_maxAttempts; ++i )
    {
        const std::filesystem::path candidate =
                tempRoot / KI_TEST::ToFsPath( aPrefix + wxS( "_" ) + wxString::Format( wxS( "%zu" ), i ) + aSuffix );

        if( std::filesystem::create_directory( candidate, ec ) )
            return candidate;

        // Any error except "file exists" is fatal. "File exists" just
        // means we should try the next candidate name.
        if( ec && ec != std::errc::file_exists )
            throw std::runtime_error( "Cannot create temporary directory '"
                                      + KI_TEST::ToWxPath( candidate ).utf8_string()
                                      + "': " + errorText( ec ).utf8_string() );
    }

    // Ran out of attempts to find a free temporary directory name.
    throw std::runtime_error( "Cannot find a free temporary directory name with prefix/suffix '"
                              + std::string( aPrefix.utf8_str() ) + "/" + std::string( aSuffix.utf8_str() ) + "'" );
}

} // namespace


std::filesystem::path KI_TEST::ToFsPath( const wxString& aPath )
{
#ifdef _WIN32
    return std::filesystem::path( aPath.ToStdWstring() );
#else
    return std::filesystem::path( aPath.utf8_string() );
#endif
}


wxString KI_TEST::ToWxPath( const std::filesystem::path& aPath )
{
#ifdef _WIN32
    return wxString( aPath.wstring() );
#else
    return wxString::FromUTF8( aPath.string() );
#endif
}


bool SCOPED_TEMP_DIR::s_anyTempDirRetained = false;


SCOPED_TEMP_DIR::SCOPED_TEMP_DIR( const wxString& aPrefix, const wxString& aSuffix )
{
    // Protect the temporary directory by creating a remover that will delete it
    // if something goes wrong before the SCOPED_TEMP_DIR takes ownership.
    TEMP_DIR_REMOVER remover( createTempDir( aPrefix, aSuffix ) );

    m_keep = shouldKeepTemp( remover.Path(), aPrefix );
    m_path = remover.Path();

    if( m_keep )
        s_anyTempDirRetained = true;

    // Finally, relinquish ownership to this SCOPED_TEMP_DIR instance.
    remover.Release();
}


SCOPED_TEMP_DIR::~SCOPED_TEMP_DIR()
{
    if( !m_keep )
    {
        // Destructor-safe removal of the temporary directory.
        safeRemoveTempDir( m_path );
    }
}


void SCOPED_TEMP_DIR::Retain()
{
    m_keep = true;
    s_anyTempDirRetained = true;

    reportTempDir(
            wxString::Format( wxT( "Keeping temporary directory '%s'" ), ToWxPath( m_path ) ) );
}


void expectSafeChildPath( const wxString& aChild )
{
    if( !WX_FILENAME::IsSafeChildPath( aChild ) )
        throw std::invalid_argument( "Unsafe child path '" + std::string( aChild.utf8_str() ) + "'" );
}


wxString SCOPED_TEMP_DIR::ChildPathStr( const wxString& aName ) const
{
    expectSafeChildPath( aName );

    return ToWxPath( m_path / ToFsPath( aName ) );
}


std::filesystem::path SCOPED_TEMP_DIR::CreateChildDir( const wxString& aName ) const
{
    expectSafeChildPath( aName );

    std::filesystem::path childPath = m_path / ToFsPath( aName );

    if( !std::filesystem::create_directory( childPath ) )
        throw std::runtime_error( "Cannot create temporary child directory '" + ToWxPath( childPath ).utf8_string()
                                  + "'" );

    return childPath;
}


wxString SCOPED_TEMP_DIR::CreateChildDirStr( const wxString& aName ) const
{
    return ToWxPath( CreateChildDir( aName ) );
}


std::filesystem::path SCOPED_TEMP_DIR::CreateChildFile( const wxString& aName ) const
{
    expectSafeChildPath( aName );

    std::filesystem::path childPath = m_path / ToFsPath( aName );
    wxFFile               file( ToWxPath( childPath ), wxT( "w" ) );

    if( !file.IsOpened() )
        throw std::runtime_error( "Cannot create temporary child file '" + ToWxPath( childPath ).utf8_string() + "'" );

    return childPath;
}


wxString SCOPED_TEMP_DIR::CreateChildFileStr( const wxString& aName ) const
{
    return ToWxPath( CreateChildFile( aName ) );
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


boost::test_tools::assertion_result KI_TEST::WriteBitsBind( boost::unit_test::test_unit_id )
{
#ifdef __WINDOWS__
    boost::test_tools::assertion_result result( true );
#else
    boost::test_tools::assertion_result result( geteuid() != 0 );
#endif

    result.message() << "write bits do not bind the superuser";
    return result;
}
