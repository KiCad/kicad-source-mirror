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

#include <mdw/client/mdw_library_sources.h>

#include <common.h>
#include <gestfich.h>
#include <kiid.h>
#include <kiplatform/io.h>
#include <lib_id.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mdw/client/mdw_file_lock.h>
#include <paths.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <remote_provider_utils.h>
#include <string_utils.h>

#include <nlohmann/json.hpp>
#include <picosha2.h>

#include <algorithm>
#include <fstream>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/utils.h>


namespace
{

const char* const USER_FILE = "team_library_sources.json";
const char* const SITE_FILE = "team-library-sources.json";


/// The well-formed entries of one file. A missing file has none; false when the file exists but cannot be read
/// or parsed, so an edit never replaces sources it could not see
bool readSources( const wxString& aFile, bool aSite, std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources )
{
    aSources.clear();

    if( !wxFileName::FileExists( aFile ) )
        return true;

    std::ifstream in( aFile.fn_str(), std::ios::binary );

    if( !in.is_open() )
        return false;

    nlohmann::json json = nlohmann::json::parse( in, nullptr, false );

    if( !json.is_object() || !json.contains( "sources" ) || !json["sources"].is_array() )
        return false;

    for( const nlohmann::json& entry : json["sources"] )
    {
        if( !entry.is_object() )
            continue;

        MDW_LIBRARY_SOURCE_CONFIG source;
        source.id = RemoteProviderJsonString( entry, "id" );
        source.backend_id = RemoteProviderJsonString( entry, "backend" );
        source.locator = MDW_LIBRARY_SOURCES::StripCredentials( RemoteProviderJsonString( entry, "locator" ) );
        source.name = RemoteProviderJsonString( entry, "name" );
        source.prefix = RemoteProviderJsonString( entry, "prefix" );
        source.account = RemoteProviderJsonString( entry, "account" );
        source.site = aSite;

        if( !MDW_LIBRARY_SOURCES::IsSafeId( source.id ) || source.backend_id.IsEmpty() || source.locator.IsEmpty()
            || !MDW_LIBRARY_SOURCES::IsSafeName( source.name ) )
        {
            continue;
        }

        aSources.push_back( source );
    }

    return true;
}


/// The first source that already claims aSource's id, folder or locator
const MDW_LIBRARY_SOURCE_CONFIG* clash( const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                        const MDW_LIBRARY_SOURCE_CONFIG&              aSource )
{
    for( const MDW_LIBRARY_SOURCE_CONFIG& other : aSources )
    {
        // Folded, since names differing only by case share a folder on Windows and macOS. A locator is a
        // source's identity everywhere events and lookups name it, so two backends cannot share one
        if( other.id == aSource.id || other.name.IsSameAs( aSource.name, false )
            || MDW_LIBRARY_SOURCES::SameLocator( other.locator, aSource.locator ) )
        {
            return &other;
        }
    }

    return nullptr;
}


MDW_RESULT makeParent( const wxString& aFile )
{
    wxFileName fn( aFile );

    // Another process creating it at the same moment makes Mkdir fail, so the folder existing is what counts
    if( fn.DirExists() || wxFileName::Mkdir( fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) || fn.DirExists() )
        return MDW_RESULT::Ok();

    return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, wxString::Format( _( "Cannot create '%s'." ), fn.GetPath() ) );
}


/// Replace aFile whole, so another process never reads half of it
MDW_RESULT writeFile( const wxString& aFile, const std::string& aText )
{
    MDW_RESULT result = makeParent( aFile );
    wxString   error;

    if( result.IsOk() && !KIPLATFORM::IO::AtomicWriteFile( aFile, aText.data(), aText.size(), &error ) )
        result = MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, error );

    return result;
}


MDW_RESULT writeSources( const wxString& aFile, const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources )
{
    nlohmann::json list = nlohmann::json::array();

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
    {
        list.push_back( { { "id", source.id.utf8_string() },
                          { "backend", source.backend_id.utf8_string() },
                          { "locator", source.locator.utf8_string() },
                          { "name", source.name.utf8_string() },
                          { "prefix", source.prefix.utf8_string() },
                          { "account", source.account.utf8_string() } } );
    }

    return writeFile( aFile, nlohmann::json( { { "sources", list } } ).dump( 2 ) + "\n" );
}


/// Opaque tokens may hold any character, so folders and markers name a digest of the token
wxString pinSuffix( const MDW_SNAPSHOT& aPin )
{
    if( aPin.token.IsEmpty() )
        return wxEmptyString;

    std::string digest;
    picosha2::hash256_hex_string( aPin.token.utf8_string(), digest );

    return wxS( "@" ) + wxString( digest.substr( 0, 12 ) );
}


const std::map<MDW_LIBRARY_TYPE, std::string> LIBRARY_TYPES = { { MDW_LIBRARY_TYPE::SYMBOL, "symbol" },
                                                                 { MDW_LIBRARY_TYPE::FOOTPRINT, "footprint" },
                                                                 { MDW_LIBRARY_TYPE::MODEL_3D, "model_3d" } };

} // namespace


MDW_LIBRARY_SOURCES::MDW_LIBRARY_SOURCES( const wxString& aUserFile, const wxString& aSiteFile,
                                          const wxString& aRoot ) :
        m_userFile( aUserFile ),
        m_siteFile( aSiteFile ),
        m_root( aRoot )
{
}


MDW_LIBRARY_SOURCES& MDW_LIBRARY_SOURCES::Get()
{
    static MDW_LIBRARY_SOURCES sources(
            wxFileName( PATHS::GetUserSettingsPath(), USER_FILE ).GetFullPath(),
            wxFileName( wxFileName( PATHS::GetUserTeamLibrariesPath() ).GetPath(), SITE_FILE ).GetFullPath(),
            wxEmptyString );
    return sources;
}


bool MDW_LIBRARY_SOURCES::TeamLibrariesEnabled()
{
    PGM_BASE* pgm = PgmOrNull();

    return pgm && pgm->MdwEnabled();
}


wxString MDW_LIBRARY_SOURCES::DefaultRoot()
{
    wxString root;

    // Rows name copies through this variable, so copies go wherever it expands to
    if( wxGetEnv( wxS( "KICAD_TEAM_LIBRARIES" ), &root ) && !root.IsEmpty() )
        return ExpandEnvVarSubstitutions( root, nullptr );

    return PATHS::GetUserTeamLibrariesPath();
}


wxString MDW_LIBRARY_SOURCES::root() const
{
    return m_root.IsEmpty() ? DefaultRoot() : m_root;
}


wxString MDW_LIBRARY_SOURCES::StripCredentials( const wxString& aLocator )
{
    int schemeEnd = aLocator.Find( wxS( "://" ) );

    if( schemeEnd == wxNOT_FOUND )
        return aLocator;

    size_t   authorityStart = schemeEnd + 3;
    size_t   authorityEnd = aLocator.find_first_of( wxS( "/?#" ), authorityStart );
    wxString authority = aLocator.Mid( authorityStart, authorityEnd == wxString::npos ? wxString::npos
                                                                                      : authorityEnd - authorityStart );
    int      at = authority.Find( '@', true );

    if( at == wxNOT_FOUND )
        return aLocator;

    wxString scheme = aLocator.Left( schemeEnd ).Lower();
    wxString user = authority.Left( at ).BeforeFirst( ':' );
    wxString host = authority.Mid( at + 1 );

    // An HTTP user name is often a token, while ssh needs its user name to log in at all
    if( scheme == wxS( "http" ) || scheme == wxS( "https" ) || user.IsEmpty() )
        authority = host;
    else
        authority = user + wxS( "@" ) + host;

    return aLocator.Left( authorityStart ) + authority
           + ( authorityEnd == wxString::npos ? wxString() : aLocator.Mid( authorityEnd ) );
}


bool MDW_LIBRARY_SOURCES::IsSafeId( const wxString& aId )
{
    // '@' separates a source id from the pin in a row's marker
    return !aId.IsEmpty() && aId.Find( '@' ) == wxNOT_FOUND;
}


wxString MDW_LIBRARY_SOURCES::DefaultName( const wxString& aLocator )
{
    wxString name = StripCredentials( aLocator ).Strip( wxString::both );

    while( name.EndsWith( wxS( "/" ) ) || name.EndsWith( wxS( "\\" ) ) )
        name.RemoveLast();

    name = name.substr( name.find_last_of( wxS( "/\\:" ) ) + 1 );

    if( name.EndsWith( wxS( ".git" ) ) )
        name.RemoveLast( 4 );

    return IsSafeName( name ) ? name : wxString();
}


wxString MDW_LIBRARY_SOURCES::NameFor( const MDW_LIBRARY_REQUIREMENT& aRequirement )
{
    // The recorded name keeps every teammate's library descriptions alike, but the project file may come from anyone
    return IsSafeName( aRequirement.name ) ? aRequirement.name : DefaultName( aRequirement.locator );
}


bool MDW_LIBRARY_SOURCES::IsSafeName( const wxString& aName )
{
    // Pinned copies' folders carry '@', and copy indexes live in .kicad beside the copies
    if( aName.IsEmpty() || aName.length() > 64 || aName.Find( '@' ) != wxNOT_FOUND || aName.StartsWith( '.' ) )
        return false;

    // Trailing dots and spaces are stripped by Windows, so two such names would share a folder
    if( aName.Last() == '.' || aName.Last() == ' ' )
        return false;

    const wxString illegal = GetIllegalFileNameWxChars();

    for( wxUniChar ch : aName )
    {
        if( ch < 0x20 || illegal.Find( ch ) != wxNOT_FOUND )
            return false;
    }

    // Windows opens a device for these whatever extension follows
    static const std::set<wxString> devices = { wxS( "CON" ), wxS( "PRN" ), wxS( "AUX" ), wxS( "NUL" ),
                                                wxS( "COM1" ), wxS( "COM2" ), wxS( "COM3" ), wxS( "COM4" ),
                                                wxS( "COM5" ), wxS( "COM6" ), wxS( "COM7" ), wxS( "COM8" ),
                                                wxS( "COM9" ), wxS( "LPT1" ), wxS( "LPT2" ), wxS( "LPT3" ),
                                                wxS( "LPT4" ), wxS( "LPT5" ), wxS( "LPT6" ), wxS( "LPT7" ),
                                                wxS( "LPT8" ), wxS( "LPT9" ), wxS( "COM\u00B9" ),
                                                wxS( "COM\u00B2" ), wxS( "COM\u00B3" ), wxS( "LPT\u00B9" ),
                                                wxS( "LPT\u00B2" ), wxS( "LPT\u00B3" ) };

    return !devices.contains( aName.BeforeFirst( '.' ).Strip( wxString::trailing ).Upper() );
}


MDW_LIBRARY_SOURCES::STAMP MDW_LIBRARY_SOURCES::stamp( const wxString& aFile )
{
    std::error_code       ec;
    std::filesystem::path path = ToFsPath( aFile );
    STAMP                 result;

    result.time = std::filesystem::last_write_time( path, ec );
    result.exists = !ec;

    if( result.exists )
        result.size = std::filesystem::file_size( path, ec );

    return result;
}


void MDW_LIBRARY_SOURCES::refresh() const
{
    STAMP user = stamp( m_userFile );
    STAMP site = stamp( m_siteFile );

    if( m_userStamp == user && m_siteStamp == site )
        return;

    m_userStamp = user;
    m_siteStamp = site;
    m_sources.clear();

    std::vector<MDW_LIBRARY_SOURCE_CONFIG> candidates;
    std::vector<MDW_LIBRARY_SOURCE_CONFIG> userSources;

    readSources( m_siteFile, true, candidates );
    readSources( m_userFile, false, userSources );
    candidates.insert( candidates.end(), userSources.begin(), userSources.end() );

    // Site entries come first, so a site source wins every clash with a user source
    for( MDW_LIBRARY_SOURCE_CONFIG& source : candidates )
    {
        if( !clash( m_sources, source ) )
            m_sources.push_back( std::move( source ) );
    }

    std::erase_if( m_copies,
                   [&]( const auto& aEntry )
                   {
                       const MDW_LIBRARY_COPY& copy = aEntry.second.second->copy;

                       return std::none_of( m_sources.begin(), m_sources.end(),
                                            [&]( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
                                            {
                                                return copy.source_id == aSource.id
                                                       && copy.backend_id == aSource.backend_id
                                                       && copy.locator == aSource.locator;
                                            } );
                   } );
}


std::vector<MDW_LIBRARY_SOURCE_CONFIG> MDW_LIBRARY_SOURCES::Sources() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    refresh();

    return m_sources;
}


std::optional<MDW_LIBRARY_SOURCE_CONFIG> MDW_LIBRARY_SOURCES::Find( const wxString& aId ) const
{
    for( const MDW_LIBRARY_SOURCE_CONFIG& source : Sources() )
    {
        if( source.id == aId )
            return source;
    }

    return std::nullopt;
}


MDW_RESULT MDW_LIBRARY_SOURCES::editUserFile(
        const std::function<MDW_RESULT( std::vector<MDW_LIBRARY_SOURCE_CONFIG>& )>& aEdit )
{
    MDW_RESULT result = makeParent( m_userFile );

    if( !result.IsOk() )
        return result;

    // Every alias of the file must take the same lock, and the atomic write replaces the real file
    const wxString        userFile = KIPLATFORM::IO::ResolveSymlinkTarget( m_userFile );
    std::error_code       ec;
    std::filesystem::path path = ToFsPath( m_userFile );

    // A link to nowhere resolves to itself, and writing would replace the link the user made
    if( std::filesystem::is_symlink( path, ec ) && !std::filesystem::exists( path, ec ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                 wxString::Format( _( "'%s' links to a file that does not exist." ), m_userFile ) );
    }

    // Another process may be editing too, and the later write would drop the earlier one's change
    bool locked = MdwWithFileLock(
            userFile,
            [&]()
            {
                refresh();

                // The raw list, so a user source a site entry shadows survives an unrelated edit
                std::vector<MDW_LIBRARY_SOURCE_CONFIG> user;

                if( !readSources( userFile, false, user ) )
                {
                    result = MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                               wxString::Format( _( "'%s' cannot be read. Fix or remove it first." ),
                                                                 userFile ) );
                    return;
                }

                result = aEdit( user );

                if( result.IsOk() )
                    result = writeSources( userFile, user );
            } );

    if( !locked )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BUSY,
                                 wxString::Format( _( "'%s' is being changed by another KiCad." ), userFile ) );
    }

    m_userStamp.reset();
    return result;
}


MDW_RESULT MDW_LIBRARY_SOURCES::Add( MDW_LIBRARY_SOURCE_CONFIG& aSource )
{
    aSource.locator = StripCredentials( aSource.locator.Strip( wxString::both ) );
    aSource.name = aSource.name.Strip( wxString::both );
    aSource.site = false;

    if( aSource.id.IsEmpty() )
        aSource.id = KIID().AsString();

    if( aSource.backend_id.IsEmpty() || aSource.locator.IsEmpty() || !IsSafeId( aSource.id ) )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER, _( "Choose a backend and enter a location." ) );

    if( !IsSafeName( aSource.name ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                 wxString::Format( _( "'%s' cannot be used as a folder name." ), aSource.name ) );
    }

    std::lock_guard<std::mutex> lock( m_mutex );

    return editUserFile(
            [&]( std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aUser )
            {
                for( const std::vector<MDW_LIBRARY_SOURCE_CONFIG>* list : { &m_sources, &aUser } )
                {
                    const MDW_LIBRARY_SOURCE_CONFIG* other = clash( *list, aSource );

                    if( !other )
                        continue;

                    wxString reason =
                            SameLocator( other->locator, aSource.locator )
                                    ? wxString::Format( _( "'%s' already uses this location." ), other->name )
                                    : wxString::Format( _( "A source named '%s' already exists." ), other->name );

                    return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER, reason );
                }

                aUser.push_back( aSource );
                return MDW_RESULT::Ok();
            } );
}


MDW_RESULT MDW_LIBRARY_SOURCES::Remove( const wxString& aId )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    return editUserFile(
            [&]( std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aUser )
            {
                if( std::erase_if( aUser,
                                   [&]( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
                                   {
                                       return aSource.id == aId;
                                   } ) )
                {
                    return MDW_RESULT::Ok();
                }

                for( const MDW_LIBRARY_SOURCE_CONFIG& source : m_sources )
                {
                    if( source.id == aId )
                    {
                        return MDW_RESULT::Fail(
                                MDW_ERROR_CODE::PERMISSION_DENIED,
                                wxString::Format( _( "'%s' is set up by your site and cannot be removed." ),
                                                  source.name ) );
                    }
                }

                return MDW_RESULT::Fail( MDW_ERROR_CODE::ITEM_NOT_FOUND );
            } );
}


MDW_RESULT MDW_LIBRARY_SOURCES::SetAccount( const wxString& aId, const wxString& aAccount )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    return editUserFile(
            [&]( std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aUser )
            {
                for( MDW_LIBRARY_SOURCE_CONFIG& source : aUser )
                {
                    if( source.id == aId )
                    {
                        source.account = aAccount;
                        return MDW_RESULT::Ok();
                    }
                }

                return MDW_RESULT::Fail( MDW_ERROR_CODE::ITEM_NOT_FOUND );
            } );
}


wxString MDW_LIBRARY_SOURCES::LocalRoot( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_SNAPSHOT& aPin ) const
{
    wxFileName dir = wxFileName::DirName( root() );
    dir.AppendDir( aPin.token.IsEmpty() ? aSource.name : Marker( aSource, aPin ) );

    return dir.GetPath();
}


wxString MDW_LIBRARY_SOURCES::SessionKey( const MDW_LIBRARY_SOURCE_CONFIG& aSource ) const
{
    return LocalRoot( aSource ) + wxFILE_SEP_PATH;
}


std::vector<MDW_REQUIREMENT_STATE>
MDW_LIBRARY_SOURCES::Match( const std::vector<MDW_LIBRARY_REQUIREMENT>& aRequirements ) const
{
    std::vector<MDW_LIBRARY_SOURCE_CONFIG> sources = Sources();
    std::vector<MDW_REQUIREMENT_STATE>     states;

    for( const MDW_LIBRARY_REQUIREMENT& requirement : aRequirements )
    {
        MDW_REQUIREMENT_STATE state;
        state.requirement = requirement;

        for( const MDW_LIBRARY_SOURCE_CONFIG& source : sources )
        {
            if( source.backend_id != requirement.backend_id || !SameLocator( source.locator, requirement.locator ) )
                continue;

            state.source_id = source.id;
            state.status = source.prefix == requirement.prefix ? MDW_REQUIREMENT_STATUS::SATISFIED
                                                               : MDW_REQUIREMENT_STATUS::PREFIX_DIFFERS;
            break;
        }

        states.push_back( state );
    }

    return states;
}


MDW_LIBRARY_STATUS MDW_LIBRARY_SOURCES::Status( const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds, bool aEnabled,
                                                const MDW_LIBRARY_REGISTRATIONS& aRegistered ) const
{
    MDW_LIBRARY_STATUS status;

    if( aNeeds.empty() )
        return status;

    std::vector<MDW_REQUIREMENT_STATE> states = Match( aNeeds );

    if( !aEnabled )
    {
        status.notice = MDW_LIBRARY_NOTICE::DISABLED;
        status.states = std::move( states );
        return status;
    }

    std::vector<MDW_REQUIREMENT_STATE> unfetched;

    for( const MDW_REQUIREMENT_STATE& state : states )
    {
        if( state.status != MDW_REQUIREMENT_STATUS::SATISFIED )
        {
            status.notice = MDW_LIBRARY_NOTICE::NOT_SET_UP;
            status.states.push_back( state );
            continue;
        }

        std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = Find( state.source_id );

        if( !source )
            continue;

        wxString                                marker = Marker( *source, state.requirement.pin );
        std::shared_ptr<const MDW_LIBRARY_COPY> copy = SharedCopy( marker );

        // Only symbol and footprint libraries have tables to be registered in, and a row counts only when it
        // reaches this machine's copy rather than wherever the user who shared the table keeps theirs
        auto registered = [&]( const MDW_COPY_LIBRARY& aLibrary )
        {
            if( aLibrary.type != MDW_LIBRARY_TYPE::SYMBOL && aLibrary.type != MDW_LIBRARY_TYPE::FOOTPRINT )
                return true;

            wxString path = ComparablePath( copy->local_root + wxFILE_SEP_PATH + aLibrary.relative_path );

            return aRegistered.contains( { aLibrary.type, aLibrary.nickname, marker, path } );
        };

        if( !copy || !wxFileName::DirExists( copy->local_root )
            || !std::all_of( copy->libraries.begin(), copy->libraries.end(), registered ) )
        {
            unfetched.push_back( state );
        }
    }

    if( status.notice == MDW_LIBRARY_NOTICE::NONE && !unfetched.empty() )
    {
        status.notice = MDW_LIBRARY_NOTICE::NOT_FETCHED;
        status.states = std::move( unfetched );
    }

    return status;
}


bool MDW_LIBRARY_SOURCES::Satisfies( const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                     const MDW_LIBRARY_REQUIREMENT&   aRequirement )
{
    return aSource.backend_id == aRequirement.backend_id && SameLocator( aSource.locator, aRequirement.locator )
           && aSource.prefix == aRequirement.prefix;
}


MDW_LIBRARY_REQUIREMENT MDW_LIBRARY_SOURCES::RequirementFor( const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                                             const MDW_SNAPSHOT&              aPin )
{
    MDW_LIBRARY_REQUIREMENT requirement;
    requirement.id = aSource.id;
    requirement.backend_id = aSource.backend_id;
    requirement.locator = aSource.locator;
    requirement.name = aSource.name;
    requirement.prefix = aSource.prefix;
    requirement.pin = aPin;

    return requirement;
}


std::vector<MDW_LIBRARY_REQUIREMENT> MDW_LIBRARY_SOURCES::RequirementsFor( const std::set<wxString>& aUsedNicknames,
                                                                           LIBRARY_MANAGER&           aTables,
                                                                           MDW_LIBRARY_TYPE           aType,
                                                                           const MDW_LIBRARY_SOURCES& aSources )
{
    std::vector<MDW_LIBRARY_REQUIREMENT> needed;

    if( aType != MDW_LIBRARY_TYPE::SYMBOL && aType != MDW_LIBRARY_TYPE::FOOTPRINT )
        return needed;

    const LIBRARY_TABLE_TYPE tableType =
            aType == MDW_LIBRARY_TYPE::SYMBOL ? LIBRARY_TABLE_TYPE::SYMBOL : LIBRARY_TABLE_TYPE::FOOTPRINT;
    std::set<wxString> markers;

    // Read the tables before taking the source lock so the two never nest
    for( const wxString& nickname : aUsedNicknames )
    {
        std::optional<LIBRARY_TABLE_ROW*> row = aTables.GetRow( tableType, nickname );
        wxString                          marker = row && *row ? MarkerOf( ( *row )->Options() ) : wxString();

        if( !marker.IsEmpty() && !IsPinned( marker ) )
            markers.insert( marker );
    }

    std::lock_guard<std::mutex> lock( aSources.m_mutex );

    for( const wxString& marker : markers )
    {
        if( std::optional<std::pair<MDW_LIBRARY_SOURCE_CONFIG, wxString>> resolved = aSources.resolveMarker( marker ) )
            Record( needed, { RequirementFor( resolved->first ) } );
    }

    return needed;
}


bool MDW_LIBRARY_SOURCES::Record( std::vector<MDW_LIBRARY_REQUIREMENT>&       aRecorded,
                                  const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeded )
{
    bool added = false;

    for( const MDW_LIBRARY_REQUIREMENT& requirement : aNeeded )
    {
        // A locator names exactly one source, whatever its backend
        const bool known = std::any_of( aRecorded.begin(), aRecorded.end(),
                                        [&]( const MDW_LIBRARY_REQUIREMENT& aKnown )
                                        {
                                            return SameLocator( aKnown.locator, requirement.locator );
                                        } );

        if( !known )
        {
            aRecorded.push_back( requirement );
            added = true;
        }
    }

    return added;
}


bool MDW_LIBRARY_SOURCES::SameLocator( const wxString& aA, const wxString& aB )
{
    return SourceKey( wxEmptyString, aA ) == SourceKey( wxEmptyString, aB );
}


wxString MDW_LIBRARY_SOURCES::SourceKey( const wxString& aBackendId, const wxString& aLocator )
{
    // A query can carry a token, so it reaches only the digest, where it tells databases apart
    wxString full = StripCredentials( aLocator ).BeforeFirst( '#' );
    wxString locator = full.BeforeFirst( '?' );
    wxString query = full.AfterFirst( '?' );
    wxString scheme;
    wxString rest = locator;

    if( locator.Contains( wxS( "://" ) ) )
    {
        scheme = locator.BeforeFirst( ':' ).Lower() + wxS( "://" );
        rest = locator.AfterFirst( ':' ).Mid( 2 );
    }

    wxString host = rest.BeforeFirst( '/' );
    wxString path = rest.Contains( wxS( "/" ) ) ? rest.AfterFirst( '/' ) : wxString();

    if( host.Contains( wxS( "@" ) ) )
        host = host.AfterLast( '@' );

    while( path.EndsWith( wxS( "/" ) ) )
        path.RemoveLast();

    wxString identity = aBackendId + wxS( "\n" ) + scheme + host + wxS( "/" ) + path;

    if( !query.IsEmpty() )
        identity << wxS( "?" ) << query;

    std::string digest;
    picosha2::hash256_hex_string( identity.utf8_string(), digest );

    wxString slug;

    for( wxUniChar c : host + wxS( "-" ) + path.AfterLast( '/' ) )
    {
        bool keep = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_'
                    || c == '.';

        if( keep )
            slug << c;
        else if( !slug.IsEmpty() && !slug.EndsWith( wxS( "-" ) ) )
            slug << '-';
    }

    slug = slug.Left( 40 );

    while( slug.StartsWith( wxS( "." ) ) )
        slug.Remove( 0, 1 );

    while( slug.EndsWith( wxS( "-" ) ) )
        slug.RemoveLast();

    if( slug.IsEmpty() )
        slug = wxS( "library" );

    // A host such as con.example.com would otherwise name a Windows device
    if( !IsSafeName( slug ) )
        slug.Prepend( wxS( "_" ) );

    return slug + wxS( "-" ) + wxString( digest.substr( 0, 10 ) );
}


wxString MDW_LIBRARY_SOURCES::Marker( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_SNAPSHOT& aPin )
{
    if( aPin.token.IsEmpty() )
        return aSource.id;

    return SourceKey( aSource.backend_id, aSource.locator ) + pinSuffix( aPin );
}


MDW_TEAM_SOURCE MDW_LIBRARY_SOURCES::TeamSourceOf( const wxString& aNickname, const std::optional<wxString>& aRowMarker,
                                                   const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                                   const std::vector<MDW_LIBRARY_REQUIREMENT>&   aNeeds )
{
    auto needName = []( const MDW_LIBRARY_REQUIREMENT& aNeed )
    {
        return aNeed.name.IsEmpty() ? aNeed.locator : aNeed.name;
    };

    if( aRowMarker )
    {
        if( aRowMarker->IsEmpty() )
            return {};

        // A project table carries pinned markers, which name the source by backend and locator alone
        auto marks = [&]( const wxString& aId, const wxString& aBackendId, const wxString& aLocator )
        {
            if( IsPinned( *aRowMarker ) )
                return aRowMarker->BeforeFirst( '@' ) == SourceKey( aBackendId, aLocator );

            return !aId.IsEmpty() && *aRowMarker == aId;
        };

        for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
        {
            if( marks( source.id, source.backend_id, source.locator ) )
                return { source.name, source.locator };
        }

        for( const MDW_LIBRARY_REQUIREMENT& need : aNeeds )
        {
            if( marks( need.id, need.backend_id, need.locator ) )
                return { needName( need ), need.locator };
        }

        return { *aRowMarker, wxEmptyString };
    }

    MDW_TEAM_SOURCE found;
    size_t          longest = 0;
    bool            configured = false;

    auto consider = [&]( const wxString& aPrefix, const wxString& aName, const wxString& aLocator, bool aConfigured )
    {
        // A nickname is the prefix and a library name, which is never empty
        if( aPrefix.IsEmpty() || aNickname.length() <= aPrefix.length() || !aNickname.StartsWith( aPrefix ) )
            return;

        if( aPrefix.length() > longest || ( aPrefix.length() == longest && aConfigured && !configured ) )
        {
            found = { aName, aLocator };
            longest = aPrefix.length();
            configured = aConfigured;
        }
    };

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
        consider( source.prefix, source.name, source.locator, true );

    for( const MDW_LIBRARY_REQUIREMENT& need : aNeeds )
        consider( need.prefix, needName( need ), need.locator, false );

    if( !found.name.IsEmpty() )
        return found;

    // Without a prefix only the source knows its nicknames, so one this computer lacks cannot be ruled out
    for( const MDW_LIBRARY_REQUIREMENT& need : aNeeds )
    {
        if( !need.prefix.IsEmpty() )
            continue;

        bool satisfied = std::any_of( aSources.begin(), aSources.end(),
                                      [&]( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
                                      {
                                          return Satisfies( aSource, need );
                                      } );

        if( !satisfied )
            return { needName( need ), need.locator };
    }

    return {};
}


wxString MDW_LIBRARY_SOURCES::BackupRoot( const MDW_LIBRARY_COPY& aCopy ) const
{
    wxFileName backups( root(), wxEmptyString );

    backups.AppendDir( wxS( ".kicad" ) );
    backups.AppendDir( wxS( "backups" ) );
    backups.AppendDir( wxFileName( aCopy.local_root ).GetFullName() );

    return backups.GetPath();
}


wxString MDW_LIBRARY_SOURCES::ComparablePath( const wxString& aPath )
{
    wxFileName path( aPath );
    // Case folds only where the filesystem ignores it, so a differently typed home folder still matches
    path.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_CASE );

    // A folder typed with its trailing separator is the same folder
    return path.IsDir() ? path.GetPath() : path.GetFullPath();
}


bool MDW_LIBRARY_SOURCES::IsPinned( const wxString& aMarker )
{
    return aMarker.Contains( wxS( "@" ) );
}


wxString MDW_LIBRARY_SOURCES::indexFile( const wxString& aFolder ) const
{
    wxFileName file( root(), aFolder + wxS( ".json" ) );
    file.AppendDir( wxS( ".kicad" ) );

    return file.GetFullPath();
}


std::optional<std::pair<MDW_LIBRARY_SOURCE_CONFIG, wxString>>
MDW_LIBRARY_SOURCES::resolveMarker( const wxString& aMarker ) const
{
    refresh();

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : m_sources )
    {
        if( aMarker == source.id )
            return std::make_pair( source, source.name );

        // A pinned copy's folder is its marker
        if( IsPinned( aMarker ) && aMarker.BeforeFirst( '@' ) == SourceKey( source.backend_id, source.locator ) )
            return std::make_pair( source, aMarker );
    }

    return std::nullopt;
}


MDW_RESULT MDW_LIBRARY_SOURCES::WriteCopy( const MDW_LIBRARY_COPY& aCopy, wxString* aGeneration )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    refresh();

    wxFileName copyRoot( aCopy.local_root );
    wxString   folder = copyRoot.GetFullName();

    auto owner = std::find_if( m_sources.begin(), m_sources.end(),
                               [&]( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
                               {
                                   return aSource.id == aCopy.source_id;
                               } );

    if( owner == m_sources.end() || copyRoot.GetPath() != wxFileName::DirName( root() ).GetPath()
        || ( folder != owner->name
             && !folder.StartsWith( SourceKey( owner->backend_id, owner->locator ) + wxS( "@" ) ) ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                 wxString::Format( _( "'%s' is not a library source's copy." ), aCopy.local_root ) );
    }

    // Members listed before the source moved belong to where it was
    if( aCopy.backend_id != owner->backend_id || aCopy.locator != owner->locator )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::VERSION_CONFLICT,
                                 wxString::Format( _( "'%s' changed while its copy was being written." ),
                                                   owner->name ) );
    }

    nlohmann::json libraries = nlohmann::json::array();
    nlohmann::json members = nlohmann::json::array();

    for( const MDW_COPY_LIBRARY& library : aCopy.libraries )
    {
        auto type = LIBRARY_TYPES.find( library.type );

        libraries.push_back( { { "id", library.library_id.utf8_string() },
                               { "nickname", library.nickname.utf8_string() },
                               { "type", type == LIBRARY_TYPES.end() ? "" : type->second },
                               { "path", library.relative_path.utf8_string() } } );
    }

    for( const MDW_COPY_MEMBER& member : aCopy.members )
    {
        nlohmann::json entry = { { "id", member.member_id.utf8_string() },
                                 { "library", member.library_id.utf8_string() },
                                 { "name", member.name.utf8_string() },
                                 { "path", member.path_hint.utf8_string() } };

        if( !member.checked_in.IsEmpty() )
            entry["checkedIn"] = member.checked_in.utf8_string();

        members.push_back( entry );
    }

    // A snapshot token repeats whenever upstream has not moved, so it cannot tell two writes apart
    const wxString generation = KIID().AsString();

    nlohmann::json json = { { "source", aCopy.source_id.utf8_string() },
                            { "backend", owner->backend_id.utf8_string() },
                            { "locator", owner->locator.utf8_string() },
                            { "prefix", owner->prefix.utf8_string() },
                            { "snapshot", { { "token", aCopy.snapshot.token.utf8_string() },
                                            { "label", aCopy.snapshot.label.utf8_string() } } },
                            { "pinned", aCopy.pinned },
                            { "libraries", libraries },
                            { "members", members } };

    json["generation"] = generation.utf8_string();

    m_copies.erase( indexFile( folder ) );

    MDW_RESULT written = writeFile( indexFile( folder ), json.dump( 1 ) + "\n" );

    if( written.IsOk() && aGeneration )
        *aGeneration = generation;

    return written;
}


std::optional<MDW_LIBRARY_COPY> MDW_LIBRARY_SOURCES::Copy( const wxString& aMarker ) const
{
    std::shared_ptr<const INDEXED_COPY> indexed = cachedCopy( aMarker );

    if( !indexed )
        return std::nullopt;

    return indexed->copy;
}


MDW_RESULT MDW_LIBRARY_SOURCES::DropCopy( const wxString& aMarker )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    std::optional<std::pair<MDW_LIBRARY_SOURCE_CONFIG, wxString>> resolved = resolveMarker( aMarker );

    if( !resolved )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::ITEM_NOT_FOUND );

    const wxString file = indexFile( resolved->second );

    m_copies.erase( file );

    if( wxFileName::FileExists( file ) && !wxRemoveFile( file ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, wxString::Format( _( "Could not remove '%s'." ), file ) );
    }

    return MDW_RESULT::Ok();
}


bool MDW_LIBRARY_SOURCES::HasCopy( const MDW_LIBRARY_SOURCE_CONFIG& aSource ) const
{
    // The folder named for the source may hold another location's copy, which this source does not need
    if( cachedCopy( Marker( aSource ) ) )
        return true;

    // A pinned copy's folder, and so its index, is named for the location and the snapshot
    wxDir    dir( wxFileName::DirName( root() ).GetPath() + wxFILE_SEP_PATH + wxS( ".kicad" ) );
    wxString name;

    return dir.IsOpened()
           && dir.GetFirst( &name, SourceKey( aSource.backend_id, aSource.locator ) + wxS( "@*.json" ), wxDIR_FILES );
}


std::shared_ptr<const MDW_LIBRARY_COPY> MDW_LIBRARY_SOURCES::SharedCopy( const wxString& aMarker ) const
{
    std::shared_ptr<const INDEXED_COPY> indexed = cachedCopy( aMarker );

    if( !indexed )
        return nullptr;

    return std::shared_ptr<const MDW_LIBRARY_COPY>( indexed, &indexed->copy );
}


std::shared_ptr<const MDW_LIBRARY_SOURCES::INDEXED_COPY>
MDW_LIBRARY_SOURCES::cachedCopy( const wxString& aMarker ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    std::optional<std::pair<MDW_LIBRARY_SOURCE_CONFIG, wxString>> resolved = resolveMarker( aMarker );

    if( !resolved )
        return nullptr;

    const auto& [source, folder] = *resolved;
    wxString file = indexFile( folder );
    STAMP    current = stamp( file );

    if( !current.exists )
        return nullptr;

    // Keyed by the index path, since the same folder under another root is another copy
    auto cached = m_copies.find( file );

    // A source's id or locator may have changed since, and an index for another source names other members
    if( cached != m_copies.end() && cached->second.first == current )
    {
        const MDW_LIBRARY_COPY& copy = cached->second.second->copy;

        if( copy.source_id == source.id && copy.backend_id == source.backend_id
            && copy.locator == source.locator )
        {
            return cached->second.second;
        }
    }

    std::ifstream  in( file.fn_str(), std::ios::binary );
    nlohmann::json json = nlohmann::json::parse( in, nullptr, false );

    // An index written for another source, or for this one before it moved, names members it no longer has
    if( !json.is_object() || RemoteProviderJsonString( json, "backend" ) != source.backend_id
        || RemoteProviderJsonString( json, "locator" ) != source.locator )
    {
        return nullptr;
    }

    // Another configuration sharing this folder set the same source up under its own id, and its nicknames match
    if( RemoteProviderJsonString( json, "source" ) != source.id
        && ( !json.contains( "prefix" ) || RemoteProviderJsonString( json, "prefix" ) != source.prefix ) )
    {
        return nullptr;
    }

    auto              indexed = std::make_shared<INDEXED_COPY>();
    MDW_LIBRARY_COPY* copy = &indexed->copy;
    copy->source_id = source.id;
    copy->backend_id = source.backend_id;
    copy->locator = source.locator;
    copy->local_root = wxFileName( root(), folder ).GetFullPath();
    copy->generation = RemoteProviderJsonString( json, "generation" );
    auto pinned = json.find( "pinned" );
    copy->pinned = pinned != json.end() && pinned->is_boolean() && pinned->get<bool>();

    if( json.contains( "snapshot" ) && json["snapshot"].is_object() )
    {
        copy->snapshot.token = RemoteProviderJsonString( json["snapshot"], "token" );
        copy->snapshot.label = RemoteProviderJsonString( json["snapshot"], "label" );
    }

    auto list = [&]( const char* aKey )
    {
        auto it = json.find( aKey );
        return it != json.end() && it->is_array() ? *it : nlohmann::json::array();
    };

    for( const nlohmann::json& entry : list( "libraries" ) )
    {
        if( !entry.is_object() )
            continue;

        MDW_COPY_LIBRARY library;
        library.library_id = RemoteProviderJsonString( entry, "id" );
        library.nickname = RemoteProviderJsonString( entry, "nickname" );
        library.relative_path = RemoteProviderJsonString( entry, "path" );

        for( const auto& [type, name] : LIBRARY_TYPES )
        {
            if( RemoteProviderJsonString( entry, "type" ) == wxString::FromUTF8( name ) )
                library.type = type;
        }

        copy->libraries.push_back( library );
    }

    for( const nlohmann::json& entry : list( "members" ) )
    {
        if( entry.is_object() )
        {
            copy->members.push_back(
                    { RemoteProviderJsonString( entry, "id" ), RemoteProviderJsonString( entry, "library" ),
                      RemoteProviderJsonString( entry, "name" ), RemoteProviderJsonString( entry, "path" ),
                      RemoteProviderJsonString( entry, "checkedIn" ) } );
        }
    }

    // The first of any duplicates wins, as a scan in order would find it
    for( size_t ii = 0; ii < copy->libraries.size(); ++ii )
    {
        const MDW_COPY_LIBRARY& library = copy->libraries[ii];

        indexed->libraryByPath.emplace(
                std::make_pair( library.type,
                                ComparablePath( copy->local_root + wxFILE_SEP_PATH + library.relative_path ) ),
                ii );
    }

    for( size_t ii = 0; ii < copy->members.size(); ++ii )
    {
        const MDW_COPY_MEMBER& member = copy->members[ii];

        indexed->memberByName.emplace( std::make_pair( member.library_id, member.name ), ii );
        indexed->memberById.emplace( member.member_id, ii );
    }

    m_copies[file] = { current, indexed };
    return indexed;
}


const MDW_COPY_LIBRARY* MDW_LIBRARY_SOURCES::libraryAt( const INDEXED_COPY& aCopy, MDW_LIBRARY_TYPE aType,
                                                        const wxString& aPath )
{
    auto it = aCopy.libraryByPath.find( std::make_pair( aType, ComparablePath( aPath ) ) );

    return it == aCopy.libraryByPath.end() ? nullptr : &aCopy.copy.libraries[it->second];
}


wxString MDW_LIBRARY_SOURCES::MarkerOf( const wxString& aRowOptions )
{
    std::map<std::string, UTF8> options = LIBRARY_TABLE::ParseOptions( aRowOptions.utf8_string() );
    auto                        marker = options.find( "mdw" );

    return marker == options.end() ? wxString() : wxString( marker->second.wx_str() );
}


std::shared_ptr<const MDW_LIBRARY_SOURCES::INDEXED_COPY>
MDW_LIBRARY_SOURCES::copyForRow( const wxString& aRowOptions ) const
{
    wxString marker = MarkerOf( aRowOptions );

    return marker.IsEmpty() ? nullptr : cachedCopy( marker );
}


std::optional<MDW_LIBRARY_ROW> MDW_LIBRARY_SOURCES::ForRowAt( MDW_LIBRARY_TYPE aType, const wxString& aRowOptions,
                                                              const wxString& aRowPath ) const
{
    std::shared_ptr<const INDEXED_COPY> indexed = copyForRow( aRowOptions );

    if( !indexed )
        return std::nullopt;

    const MDW_COPY_LIBRARY* library = libraryAt( *indexed, aType, aRowPath );

    if( !library )
        return std::nullopt;

    const MDW_LIBRARY_COPY& copy = indexed->copy;

    return MDW_LIBRARY_ROW{ copy.source_id,  copy.locator,  library->library_id,
                            copy.local_root, copy.snapshot, copy.pinned };
}


std::optional<MDW_ITEM_REF> MDW_LIBRARY_SOURCES::MemberForAt( MDW_LIBRARY_TYPE aType, const wxString& aRowOptions,
                                                              const wxString& aRowPath,
                                                              const wxString& aItemName ) const
{
    std::shared_ptr<const INDEXED_COPY> indexed = copyForRow( aRowOptions );

    if( !indexed )
        return std::nullopt;

    const MDW_COPY_LIBRARY* library = libraryAt( *indexed, aType, aRowPath );

    if( !library )
        return std::nullopt;

    auto it = indexed->memberByName.find( std::make_pair( library->library_id, aItemName ) );

    if( it == indexed->memberByName.end() )
        return std::nullopt;

    const MDW_COPY_MEMBER& member = indexed->copy.members[it->second];

    return MDW_ITEM_REF{ member.member_id, member.path_hint, indexed->copy.locator };
}


std::optional<MDW_ITEM_REF> MDW_LIBRARY_SOURCES::MemberOf( LIBRARY_MANAGER& aTables, MDW_LIBRARY_TYPE aType,
                                                           const LIB_ID& aLibId ) const
{
    if( aType != MDW_LIBRARY_TYPE::SYMBOL && aType != MDW_LIBRARY_TYPE::FOOTPRINT )
        return std::nullopt;

    LIBRARY_TABLE_TYPE tableType =
            aType == MDW_LIBRARY_TYPE::SYMBOL ? LIBRARY_TABLE_TYPE::SYMBOL : LIBRARY_TABLE_TYPE::FOOTPRINT;
    std::optional<LIBRARY_TABLE_ROW*> row = aTables.GetRow( tableType, aLibId.GetLibNickname().wx_str() );

    if( !row || !*row )
        return std::nullopt;

    return MemberForAt( aType, ( *row )->Options(), LIBRARY_MANAGER::GetFullURI( *row, true, &aTables.Project() ),
                        aLibId.GetLibItemName().wx_str() );
}


std::optional<LIB_ID> MDW_LIBRARY_SOURCES::LocalLibId( MDW_LIBRARY_TYPE aType, const MDW_ITEM_REF& aMember ) const
{
    if( aMember.item_id.IsEmpty() )
        return std::nullopt;

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : Sources() )
    {
        if( !aMember.source.IsEmpty() && !SameLocator( aMember.source, source.locator ) )
            continue;

        std::shared_ptr<const INDEXED_COPY> indexed = cachedCopy( Marker( source ) );

        if( !indexed )
            continue;

        auto found = indexed->memberById.find( aMember.item_id );

        if( found == indexed->memberById.end() )
            continue;

        const MDW_COPY_MEMBER& member = indexed->copy.members[found->second];

        for( const MDW_COPY_LIBRARY& library : indexed->copy.libraries )
        {
            if( library.type == aType && library.library_id == member.library_id )
                return LIB_ID( library.nickname, member.name );
        }
    }

    return std::nullopt;
}
