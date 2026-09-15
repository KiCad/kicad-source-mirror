/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "git_shared_libraries.h"
#include "mdw_git_checkouts.h"
#include "mdw_git_remote.h"
#include "mdw_git_repo.h"

#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_lock_retry.h>
#include <git/kigit_remote_url.h>
#include <git/kigit_symbol_split.h>
#include <git/kigit_transport.h>
#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_file_io.h>
#include <mdw_runtime/mdw_lifecycle.h>
#include <mdw_runtime/mdw_paging.h>
#include <mdw_runtime/mdw_paths.h>
#include <mdw_runtime/mdw_staged_folder.h>

#include <api/mdw/backend_revisions.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>

#include <git2.h>
#include <wx/datetime.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <map>
#include <set>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;

namespace
{

const char COPY_KEY[] = "kicad.mdwlibrary";
const char BRANCH_KEY[] = "kicad.mdwlibrarybranch";
const char REMOTE[] = "origin";

const std::string SYMBOL_EXT = ".kicad_sym";
const std::string SYMBOL_DIR_EXT = ".kicad_symdir";
const std::string FOOTPRINT_DIR_EXT = ".pretty";
const std::string FOOTPRINT_EXT = ".kicad_mod";

/// Ends the message of a check-in that only reformatted its members, so every copy's history agrees on it
const std::string FORMAT_UPGRADE_TRAILER = "KiCad-Format-Upgrade: yes";

/// Starts each trailer line naming a member a whole-file check-in only reformatted, which it therefore did not revise
const std::string REFORMATTED_TRAILER = "KiCad-Reformatted: ";


ws::WorkspaceError gitFailure( const std::string& aWhat )
{
    return MdwFail( ws::WE_INTERNAL, aWhat + ": " + MdwGitError() );
}


std::string shortId( const std::string& aOid )
{
    return aOid.substr( 0, 8 );
}


/// True when aMessage ends with the trailer, as only a format upgrade's check-in writes it
bool isFormatUpgrade( std::string aMessage )
{
    while( !aMessage.empty() && aMessage.back() == '\n' )
        aMessage.pop_back();

    return aMessage.ends_with( "\n\n" + FORMAT_UPGRADE_TRAILER );
}


/// The members the trailer lines ending aMessage name as only reformatted
std::set<std::string> reformattedMembers( const std::string& aMessage )
{
    std::set<std::string> ids;
    const std::size_t     block = aMessage.rfind( "\n\n" );

    if( block == std::string::npos )
        return ids;

    for( std::size_t at = block + 2; at < aMessage.size(); )
    {
        std::size_t end = aMessage.find( '\n', at );

        if( end == std::string::npos )
            end = aMessage.size();

        if( aMessage.compare( at, REFORMATTED_TRAILER.size(), REFORMATTED_TRAILER ) == 0 )
            ids.insert( aMessage.substr( at + REFORMATTED_TRAILER.size(), end - at - REFORMATTED_TRAILER.size() ) );

        at = end + 1;
    }

    return ids;
}


/// True when a line of aComment starts a reformatted member trailer, which would keep an edit from revising it
bool namesReformattedMember( const std::string& aComment )
{
    for( std::size_t at = aComment.find( REFORMATTED_TRAILER ); at != std::string::npos;
         at = aComment.find( REFORMATTED_TRAILER, at + 1 ) )
    {
        if( at == 0 || aComment[at - 1] == '\n' )
            return true;
    }

    return false;
}


/// True when a line of aComment is the trailer, which a comment could otherwise use to pass for an upgrade
bool namesFormatUpgrade( const std::string& aComment )
{
    for( std::size_t at = aComment.find( FORMAT_UPGRADE_TRAILER ); at != std::string::npos;
         at = aComment.find( FORMAT_UPGRADE_TRAILER, at + 1 ) )
    {
        const std::size_t end = at + FORMAT_UPGRADE_TRAILER.size();

        if( ( at == 0 || aComment[at - 1] == '\n' )
            && ( end == aComment.size() || aComment[end] == '\n' || aComment[end] == '\r' ) )
        {
            return true;
        }
    }

    return false;
}


bool isFootprintLibrary( const std::string& aLibrary )
{
    return aLibrary.ends_with( FOOTPRINT_DIR_EXT );
}


/// True for a library held as a folder with one file per member, a .pretty or a .kicad_symdir
bool isFolderLibrary( const std::string& aLibrary )
{
    return isFootprintLibrary( aLibrary ) || aLibrary.ends_with( SYMBOL_DIR_EXT );
}


const std::string& memberExt( const std::string& aLibrary )
{
    return isFootprintLibrary( aLibrary ) ? FOOTPRINT_EXT : SYMBOL_EXT;
}


/// The folder library aPath is a member file of, or empty when it is none's
std::string folderLibrary( const std::string& aPath )
{
    const std::size_t slash = aPath.rfind( '/' );

    if( slash == std::string::npos )
        return std::string();

    const std::string library = aPath.substr( 0, slash );

    return isFolderLibrary( library ) && aPath.ends_with( memberExt( library ) ) ? library : std::string();
}


/// True for a .kicad_sym that is a library itself, which one inside a .kicad_symdir never is
bool isSymbolFile( const std::string& aPath )
{
    return aPath.ends_with( SYMBOL_EXT ) && aPath.find( SYMBOL_DIR_EXT + "/" ) == std::string::npos;
}


std::string memberId( const std::string& aLibrary, const std::string& aName )
{
    return aLibrary + "#" + aName;
}


/// The library and name a member id names; false when it names none
bool splitMemberId( const std::string& aId, std::string& aLibrary, std::string& aName )
{
    // A symbol or footprint name may hold '#' itself, so the id splits after the first library path
    for( std::size_t hash = aId.find( '#' ); hash != std::string::npos; hash = aId.find( '#', hash + 1 ) )
    {
        aLibrary = aId.substr( 0, hash );
        aName = aId.substr( hash + 1 );

        if( !aName.empty() && ( isSymbolFile( aLibrary ) || isFolderLibrary( aLibrary ) ) )
            return true;
    }

    return false;
}


/// The member id of the footprint file at aPath, which sits directly in its .pretty folder
std::string footprintId( const std::string& aPath )
{
    const std::size_t slash = aPath.rfind( '/' );

    return memberId( aPath.substr( 0, slash ),
                     aPath.substr( slash + 1, aPath.size() - slash - 1 - FOOTPRINT_EXT.size() ) );
}


/// The tag kind that records which state a member's release was moved to
const char STATE_KIND[] = "state";


/// Where member aId's releases are tagged, below kicad/rel/
std::string releaseScope( const std::string& aId )
{
    return MDW_GIT_CHECKOUTS::RefComponent( aId ) + "/";
}


/// The local ref naming the upstream version of member aId that a conflict answer kept this copy's edit over
std::string keptRef( const std::string& aId )
{
    return "refs/kicad/kept/" + MDW_GIT_CHECKOUTS::RefComponent( aId );
}


/// Accepts a release manifest only when it describes member aId at the tag's own number
MDW_GIT_RELEASES::VALID partRelease( const std::string& aId )
{
    return [aId]( const nlohmann::json& aManifest, int aNumber )
    {
        auto digest = aManifest.find( "content_digest" );

        return aManifest.value( "item_id", nlohmann::json() ) == aId
               && aManifest.value( "revision", nlohmann::json() ) == aNumber && digest != aManifest.end()
               && digest->is_string() && !digest->get<std::string>().empty();
    };
}


/// True for a release the team can rely on, which a tag that never reached the remote or was moved there is not
bool settled( const MDW_GIT_RELEASE& aRelease )
{
    return aRelease.published && !aRelease.tampered;
}


/// The newest settled release of aReleases, oldest first, or nothing before the first
const MDW_GIT_RELEASE* newestSettled( const std::vector<MDW_GIT_RELEASE>& aReleases )
{
    auto newest = std::find_if( aReleases.rbegin(), aReleases.rend(), settled );

    return newest == aReleases.rend() ? nullptr : &*newest;
}


/// Member aId's newest settled release, or nothing before its first
std::optional<MDW_GIT_RELEASE> newestRelease( const MDW_GIT_RELEASES& aReleases, const std::string& aId )
{
    const std::vector<MDW_GIT_RELEASE> releases = aReleases.List( releaseScope( aId ), partRelease( aId ) );

    if( const MDW_GIT_RELEASE* newest = newestSettled( releases ) )
        return *newest;

    return std::nullopt;
}


/// Where the state tags of member aId's release aNumber live, below kicad/state/
std::string stateScope( const std::string& aId, int aNumber )
{
    return releaseScope( aId ) + std::to_string( aNumber ) + "/";
}


/// Accepts a state tag only when it moves release aNumber of member aId to a state git knows, at the tag's own number
MDW_GIT_RELEASES::VALID stateMove( const std::string& aId, int aNumber )
{
    return [aId, aNumber]( const nlohmann::json& aManifest, int aSequence )
    {
        auto state = aManifest.find( "state" );

        return aManifest.value( "item_id", nlohmann::json() ) == aId
               && aManifest.value( "revision", nlohmann::json() ) == aNumber
               && aManifest.value( "sequence", nlohmann::json() ) == aSequence && state != aManifest.end()
               && state->is_string() && MdwGitState( state->get<std::string>() );
    };
}


/// The state member aId's release aRelease was last moved to, as its newest settled state tag in aStates says
ws::LifecycleState releaseState( const MDW_GIT_RELEASES& aStates, const std::string& aId,
                                 const MDW_GIT_RELEASE& aRelease )
{
    const std::vector<MDW_GIT_RELEASE> moves =
            aStates.List( stateScope( aId, aRelease.number ), stateMove( aId, aRelease.number ) );

    if( const MDW_GIT_RELEASE* newest = newestSettled( moves ) )
        return *MdwGitState( newest->manifest["state"].get<std::string>() );

    return MdwGitReleasedState();
}


/// Set each of member aId's aReleases to the state its state tags in aStates moved it to
void readStates( const MDW_GIT_RELEASES& aStates, const std::string& aId, std::vector<MDW_GIT_RELEASE>& aReleases )
{
    for( MDW_GIT_RELEASE& release : aReleases )
        release.stateId = releaseState( aStates, aId, release ).state_id();
}


/// The state of member aId's newest release while the member at aRevision is what it holds, editable otherwise
ws::LifecycleState memberState( const MDW_GIT_RELEASES& aStates, const std::string& aId,
                                const std::optional<MDW_GIT_RELEASE>& aNewest, const std::string& aRevision )
{
    if( aNewest && aNewest->commit == aRevision )
        return releaseState( aStates, aId, *aNewest );

    ws::LifecycleState state;
    state.set_category( ws::LC_EDITABLE );
    return state;
}


/// Member aId's every release, oldest first, as a placed part is weighed against them
void addMemberReleases( const MDW_GIT_RELEASES& aReleases, const MDW_GIT_RELEASES& aStates, const std::string& aId,
                        google::protobuf::RepeatedPtrField<mdw::MemberRelease>& aOut )
{
    std::vector<MDW_GIT_RELEASE> found = aReleases.List( releaseScope( aId ), partRelease( aId ) );

    readStates( aStates, aId, found );

    for( const MDW_GIT_RELEASE& release : found )
    {
        const ws::Revision  revision = MdwGitPartRevision( release );
        mdw::MemberRelease* out = aOut.Add();

        out->set_revision_id( revision.revision_id() );
        out->set_label( revision.label() );

        // A tag the remote holds differently is no release a placed part can match
        out->set_content_digest( release.tampered ? std::string() : revision.content_digest() );
        *out->mutable_state() = revision.state();
        out->set_published( release.published );
    }
}


/// The file member aName of aLibrary lives in by its name alone, which says nothing for a .kicad_symdir
std::string memberFile( const std::string& aLibrary, const std::string& aName )
{
    if( isFootprintLibrary( aLibrary ) )
        return aLibrary + "/" + aName + FOOTPRINT_EXT;

    return isFolderLibrary( aLibrary ) ? std::string() : aLibrary;
}


/// A copy in an older format was most likely saved by a KiCad that cannot open the team's file
const std::string GET_UPDATES_FIRST =
        "Get Library Updates first, which may need a newer KiCad to open the team's format.";


/**
 * Why a member of aFile read under header aFrom cannot be spliced under header aInto, the team library's, or nothing
 * when it can. The header's version decides how a symbol reads, so a member under another one reads differently.
 */
std::optional<ws::WorkspaceError> formatMismatch( const std::string& aFile, const std::string& aFrom,
                                                  const std::string& aInto, const std::string& aIfNewer,
                                                  const std::string& aIfOlder )
{
    const std::string from = KIGIT::FormatVersion( aFrom );
    const std::string into = KIGIT::FormatVersion( aInto );

    if( from == into )
        return std::nullopt;

    // Getting the team's updates settles only an older copy, so only that one is a conflict to resolve
    if( KIGIT::NewerFormat( from, into ) )
        return MdwFail( ws::WE_NOT_SUPPORTED, aFile + " is in a newer file format than the team library. " + aIfNewer );

    return MdwFail( ws::WE_VERSION_CONFLICT, aFile + " is in an older file format than the team library. " + aIfOlder );
}


/// aFrame, the text outside a .kicad_sym's symbols, without the header fields naming what wrote it and its format
std::string withoutWriter( std::string aFrame )
{
    for( const std::string key : { "(version ", "(generator ", "(generator_version " } )
    {
        std::size_t start = aFrame.find( key );
        std::size_t end = start + key.size();
        bool        quoted = false;

        if( start == std::string::npos )
            continue;

        for( ; end < aFrame.size(); ++end )
        {
            if( quoted && aFrame[end] == '\\' )
                ++end;
            else if( aFrame[end] == '"' )
                quoted = !quoted;
            else if( !quoted && aFrame[end] == ')' )
                break;
        }

        while( start > 0 && std::isspace( static_cast<unsigned char>( aFrame[start - 1] ) ) )
            --start;

        aFrame.erase( start, end + 1 - start );
    }

    return aFrame;
}


/// aText with member aName's symbol replaced by aSymbol, added when absent, or removed when aSymbol is nullopt
std::string withSymbol( std::string aText, const std::string& aName, const std::optional<std::string>& aSymbol )
{
    const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( aText );
    auto                     it = file.spans.find( aName );

    if( it != file.spans.end() )
    {
        std::size_t start = it->second.start;
        std::size_t end = start + it->second.length;

        if( aSymbol )
        {
            aText.replace( start, end - start, *aSymbol );
            return aText;
        }

        // The whole line goes, so removing a symbol leaves no blank one behind
        while( start > 0 && ( aText[start - 1] == ' ' || aText[start - 1] == '\t' ) )
            --start;

        if( end < aText.size() && aText[end] == '\n' )
            ++end;

        aText.erase( start, end - start );
        return aText;
    }

    if( !aSymbol )
        return aText;

    const std::size_t close = file.close;

    if( close == std::string::npos )
        return aText + "\t" + *aSymbol + "\n";

    std::string insert = "\t" + *aSymbol + "\n";

    if( close == 0 || aText[close - 1] != '\n' )
        insert = "\n" + insert;

    aText.insert( close, insert );
    return aText;
}


/// aText with every line ending in CRLF when aCrLf is set, in LF otherwise
std::string withLineEnds( const std::string& aText, bool aCrLf )
{
    std::string out;

    out.reserve( aText.size() );

    for( std::size_t ii = 0; ii < aText.size(); ++ii )
    {
        if( aText[ii] == '\r' && ii + 1 < aText.size() && aText[ii + 1] == '\n' )
            continue;

        if( aText[ii] == '\n' && aCrLf )
            out += '\r';

        out += aText[ii];
    }

    return out;
}


/// aText without the spacing between its tokens, which a save may lay out differently
std::string withoutLayout( const std::string& aText )
{
    std::string out;
    bool        quoted = false;
    bool        gap = false;
    bool        lineStart = true;

    out.reserve( aText.size() );

    for( std::size_t ii = 0; ii < aText.size(); ++ii )
    {
        const char c = aText[ii];

        if( quoted )
        {
            out += c;

            if( c == '\\' && ii + 1 < aText.size() )
                out += aText[++ii];
            else if( c == '"' )
                quoted = false;

            continue;
        }

        // As in DSNLEXER, a '#' that starts a line comments out the rest of it, so the line end is kept
        if( lineStart && c == '#' )
        {
            std::size_t end = std::min( aText.find( '\n', ii ), aText.size() );

            while( end > ii && ( aText[end - 1] == ' ' || aText[end - 1] == '\t' || aText[end - 1] == '\r' ) )
                --end;

            out += '\n';
            out.append( aText, ii, end - ii );
            out += '\n';
            ii = end - 1;
            gap = false;
            lineStart = false;
            continue;
        }

        if( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
        {
            gap = true;
            lineStart = c == '\n' || lineStart;
            continue;
        }

        lineStart = false;

        // A parenthesis ends an atom by itself, so spacing matters only between two atoms
        if( gap && !out.empty() && out.back() != '(' && out.back() != ')' && out.back() != '\n' && c != '('
            && c != ')' )
        {
            out += ' ';
        }

        gap = false;
        quoted = c == '"';
        out += c;
    }

    return out;
}


/// True when aA and aB hold the same member, or both none, however each was laid out
bool sameContent( const std::optional<std::string>& aA, const std::optional<std::string>& aB )
{
    return aA == aB || ( aA && aB && withoutLayout( *aA ) == withoutLayout( *aB ) );
}


/// What a tree or a folder holds as libraries, with each library's type, member text and symbol file text
struct CONTENT
{
    using BY_FILE = std::map<std::string, std::string>;

    std::map<std::string, mdw::SharedLibType> libraries;
    std::map<std::string, std::string>        members;
    std::map<std::string, std::string>        headers;
    std::map<std::string, std::string>        frames;
    std::map<std::string, std::string>        texts;
    std::map<std::string, std::string>        files;   ///< The file holding each member
    std::set<std::string>                     derived; ///< Members that extend another symbol
    std::set<std::string>                     links;   ///< Library paths that are links
    std::map<std::string, BY_FILE>            twice;   ///< Each member's copies in files the folder does not load
    std::string                               error;   ///< Why some of it could not be read, which voids the rest

    void Fail( const std::string& aWhy )
    {
        if( error.empty() )
            error = aWhy;
    }

    /// Record the library the file at aPath belongs to, without its members. False when it is in none
    bool AddLibrary( const std::string& aPath )
    {
        if( isSymbolFile( aPath ) )
        {
            libraries[aPath] = mdw::SLT_SYMBOL;
            return true;
        }

        const std::string library = folderLibrary( aPath );

        if( library.empty() )
            return false;

        libraries[library] = isFootprintLibrary( library ) ? mdw::SLT_FOOTPRINT : mdw::SLT_SYMBOL;
        return true;
    }

    void AddFile( const std::string& aPath, const std::string& aText )
    {
        if( !AddLibrary( aPath ) )
            return;

        // Saves on Windows write CRLF, so every text is read as LF and line endings alone never read as an edit
        const std::string text = withLineEnds( aText, false );

        if( !aPath.ends_with( SYMBOL_EXT ) )
        {
            const std::string id = footprintId( aPath );
            members[id] = text;
            files[id] = aPath;
            return;
        }

        // Each file of a .kicad_symdir is split like a symbol library, as the folder may hold any number of symbols
        const std::string        library = isSymbolFile( aPath ) ? aPath : folderLibrary( aPath );
        const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( text );

        // A symbol the splitter missed would read as removed, and a check-in would delete it
        if( !file.complete )
            Fail( aPath + " is not a symbol library this backend can split into symbols" );

        headers[aPath] = file.header;
        frames[aPath] = file.frame;
        texts[aPath] = text;

        for( const auto& [name, span] : file.spans )
        {
            const std::string id = memberId( library, name );

            // The others are kept, as history holding a member twice still reads and cannot be repaired. The
            // first file by name stands for the folder, so any order of reading picks the same copy
            if( auto it = files.find( id ); it != files.end() && it->second != aPath )
            {
                if( aPath > it->second )
                {
                    twice[id][aPath] = text.substr( span.start, span.length );
                    continue;
                }

                twice[id][it->second] = members[id];
                derived.erase( id );
            }

            members[id] = text.substr( span.start, span.length );
            files[id] = aPath;

            if( span.derived )
                derived.insert( id );
        }
    }

    std::optional<std::string> Member( const std::string& aId ) const
    {
        auto it = members.find( aId );
        return it == members.end() ? std::nullopt : std::optional<std::string>( it->second );
    }

    /// The copy of aId that aFile holds, which is not always the one the folder loads
    std::optional<std::string> Member( const std::string& aId, const std::string& aFile ) const
    {
        if( auto it = files.find( aId ); it != files.end() && it->second == aFile )
            return Member( aId );

        if( auto it = twice.find( aId ); it != twice.end() && it->second.count( aFile ) )
            return it->second.at( aFile );

        return std::nullopt;
    }

    /// Every file holding a copy of aId, the one the folder loads first
    std::vector<std::string> Copies( const std::string& aId ) const
    {
        std::vector<std::string> copies;

        if( auto it = files.find( aId ); it != files.end() )
            copies.push_back( it->second );

        if( auto it = twice.find( aId ); it != twice.end() )
        {
            for( const auto& [file, text] : it->second )
                copies.push_back( file );
        }

        return copies;
    }

    /// Why aId, or any member when aId is empty, is held twice. The folder loads one of the copies, so a write to
    /// either could leave the other in charge
    std::string Twice( const std::string& aId = std::string() ) const
    {
        auto it = aId.empty() ? twice.begin() : twice.find( aId );

        if( it == twice.end() )
            return std::string();

        std::string library;
        std::string name;

        splitMemberId( it->first, library, name );
        return library + " holds " + name + " in both " + files.at( it->first ) + " and " + it->second.begin()->first;
    }

    /// The file holding member aId, empty for a .kicad_symdir symbol this content lacks
    std::string File( const std::string& aId ) const
    {
        if( auto it = files.find( aId ); it != files.end() )
            return it->second;

        std::string library;
        std::string name;

        return splitMemberId( aId, library, name ) ? memberFile( library, name ) : std::string();
    }

    /// Forget the files at aPaths, or below them for a folder, which a checkout without links writes as plain files
    void Drop( const std::set<std::string>& aPaths )
    {
        for( const std::string& path : aPaths )
        {
            auto within = [&path]( const std::string& aFile )
            {
                return aFile == path || aFile.starts_with( path + "/" );
            };

            if( isSymbolFile( path ) || isFolderLibrary( path ) )
                libraries.erase( path );

            for( std::map<std::string, std::string>* byFile : { &headers, &frames, &texts } )
            {
                for( auto it = byFile->begin(); it != byFile->end(); )
                    it = within( it->first ) ? byFile->erase( it ) : std::next( it );
            }

            for( auto it = twice.begin(); it != twice.end(); )
            {
                std::erase_if( it->second,
                               [&]( const auto& aCopy )
                               {
                                   return within( aCopy.first );
                               } );

                it = it->second.empty() ? twice.erase( it ) : std::next( it );
            }

            for( auto it = files.begin(); it != files.end(); )
            {
                if( !within( it->second ) )
                {
                    ++it;
                    continue;
                }

                members.erase( it->first );
                derived.erase( it->first );

                // Another copy is what the folder loads once this one is gone
                if( auto other = twice.find( it->first ); other != twice.end() )
                {
                    auto first = other->second.begin();

                    members[it->first] = first->second;
                    it->second = first->first;
                    other->second.erase( first );

                    if( other->second.empty() )
                        twice.erase( other );

                    ++it;
                    continue;
                }

                it = files.erase( it );
            }
        }
    }

    /// Put aMerged in place of aId in aFile and set aMember as it holds aId. It holds aId alone in aFile's format, or
    /// with aWhole all of aFile but what aSettled answers. Empty when done, otherwise why aMerged was refused
    std::string Resolve( const std::string& aId, const std::string& aMerged, const std::string& aFile, bool aWhole,
                         const std::set<std::string>& aSettled, std::optional<std::string>& aMember )
    {
        std::string library;
        std::string name;

        splitMemberId( aId, library, name );

        const std::string file = aFile;

        aMember.reset();

        if( file.empty() )
            return "no file holds " + aId;

        if( isFootprintLibrary( library ) )
        {
            members.erase( aId );
            files.erase( aId );

            if( !aMerged.empty() )
            {
                members[aId] = aMerged;
                files[aId] = file;
                aMember = aMerged;
            }

            return std::string();
        }

        CONTENT merged;

        if( !aMerged.empty() )
            merged.AddFile( file, aMerged );

        if( !merged.error.empty() )
            return merged.error;

        std::set<std::string> others;
        std::set<std::string> answered;

        for( const auto& [id, held] : files )
        {
            if( held == file && id != aId )
                others.insert( id );
        }

        for( const auto& [id, text] : merged.members )
        {
            if( id != aId )
                answered.insert( id );
        }

        aMember = merged.Member( aId );

        std::optional<std::string> text = Text( file );

        // A header's version decides how every symbol under it reads, so a lone symbol only joins its own format
        const bool sameFormat =
                !text || aMerged.empty() || KIGIT::FormatVersion( aMerged ) == KIGIT::FormatVersion( *text );

        auto unsettled = [&]( std::set<std::string> aIds )
        {
            std::erase_if( aIds,
                           [&]( const std::string& aOther )
                           {
                               return aSettled.count( aOther ) > 0;
                           } );
            return aIds;
        };

        if( aWhole && ( aMerged.empty() || unsettled( answered ) != unsettled( others ) ) )
            return "the merge of " + aId + " holds other symbols than " + file;

        if( !aWhole && !answered.empty() )
            return "the merge of " + aId + " holds other symbols";

        if( !aWhole && !sameFormat )
            return "the merge of " + aId + " is in another file format than " + file;

        if( !aWhole && merged.derived.count( aId ) )
            return "the merge of " + aId + " extends another symbol";

        if( aWhole )
            text = aMerged;
        else if( text )
            text = withSymbol( *text, name, aMember );
        else if( aMember )
            text = aMerged;

        // A folder file left without symbols is removed, as the library writer removes it
        if( text && file != library && KIGIT::SplitSymbols( *text ).spans.empty() )
            text.reset();

        Drop( { file } );

        if( text )
            AddFile( file, *text );

        // Only this member, as a duplicate of another that came with the copy goes with the update
        return error.empty() ? Twice( aId ) : error;
    }

    std::optional<std::string> Frame( const std::string& aPath ) const
    {
        auto it = frames.find( aPath );
        return it == frames.end() ? std::nullopt : std::optional<std::string>( it->second );
    }

    std::optional<std::string> Text( const std::string& aPath ) const
    {
        auto it = texts.find( aPath );
        return it == texts.end() ? std::nullopt : std::optional<std::string>( it->second );
    }
};


/// True when member aId reads the same in aA and aB and sits in the same file, since a symbol moved unchanged to
/// another file of its folder is still a change to check in
bool sameMember( const CONTENT& aA, const CONTENT& aB, const std::string& aId )
{
    const std::optional<std::string> member = aA.Member( aId );

    return member == aB.Member( aId ) && ( !member || aA.File( aId ) == aB.File( aId ) );
}


/// Why aId's change may not be checked in, empty when it may. Only the claim holder checks in a released member,
/// and a move that left its text alone keeps its release
std::string checkInDenial( const MDW_GIT_RELEASES& aReleases, const CONTENT& aBefore, const CONTENT& aNow,
                           const std::string& aId, const MDW_GIT_SHARED_LIBRARIES::CLAIM_TEST& aHoldsClaim )
{
    const std::optional<std::string> was = aBefore.Member( aId );

    if( !was || was == aNow.Member( aId ) || !newestRelease( aReleases, aId ) || ( aHoldsClaim && aHoldsClaim( aId ) ) )
        return std::string();

    return "Released; Revise it, then Release";
}


bool isLibraryFile( const std::string& aPath )
{
    // Files under dot-named folders are tooling, kept like any other non-library file
    if( aPath[0] == '.' || aPath.find( "/." ) != std::string::npos )
        return false;

    return isSymbolFile( aPath ) || !folderLibrary( aPath ).empty();
}


/// False for a folder or a submodule, the entries that hold no text
bool isBlob( const KIGIT_TREE_ENTRY& aEntry )
{
    return !aEntry.isTree && aEntry.mode != GIT_FILEMODE_COMMIT;
}


std::optional<std::string> fileAt( git_repository* aRepo, const std::string& aCommit, const std::string& aPath )
{
    std::optional<KIGIT_TREE_ENTRY> entry = GetGitBackend()->TreeEntryAt( aRepo, aCommit, aPath );

    if( !entry || !isBlob( *entry ) )
        return std::nullopt;

    return GetGitBackend()->ReadBlob( aRepo, entry->oid );
}


/// Give each text of aFiles the line endings of the file it replaces at aHead, CRLF only where every line of that
/// ends so and LF for a new file, then drop any that aHead already holds
void stageLineEnds( git_repository* aRepo, const std::string& aHead,
                    std::map<std::string, std::optional<std::string>>& aFiles )
{
    for( auto it = aFiles.begin(); it != aFiles.end(); )
    {
        const std::optional<std::string> committed = fileAt( aRepo, aHead, it->first );

        if( it->second )
        {
            const bool crlf = committed && committed->find( '\n' ) != std::string::npos
                              && withLineEnds( *committed, true ) == *committed;

            it->second = withLineEnds( *it->second, crlf );
        }

        it = it->second == committed ? aFiles.erase( it ) : std::next( it );
    }
}


/// What commit aCommit holds as libraries below aFolder. With aNamesOnly only the library list, which reads no blob
CONTENT contentAt( git_repository* aRepo, const std::string& aCommit, const std::string& aFolder = std::string(),
                   bool aNamesOnly = false )
{
    CONTENT content;

    if( aCommit.empty() )
        return content;

    auto visit = [&]( const KIGIT_TREE_ENTRY& aEntry )
    {
        const std::size_t slash = aEntry.path.rfind( '/' );
        const std::string name = slash == std::string::npos ? aEntry.path : aEntry.path.substr( slash + 1 );

        // Dot-named folders hold tooling, never libraries
        if( name[0] == '.' )
            return aEntry.isTree ? KIGIT_WALK::SKIP : KIGIT_WALK::CONTINUE;

        if( !isBlob( aEntry ) || !isLibraryFile( aEntry.path ) )
            return KIGIT_WALK::CONTINUE;

        // A tracked link is no member, as the disk scan never reads through one
        if( aEntry.mode == GIT_FILEMODE_LINK )
        {
            content.links.insert( aEntry.path );
            return KIGIT_WALK::CONTINUE;
        }

        if( aNamesOnly )
        {
            content.AddLibrary( aEntry.path );
            return KIGIT_WALK::CONTINUE;
        }

        std::optional<std::string> text = GetGitBackend()->ReadBlob( aRepo, aEntry.oid );

        if( !text )
        {
            content.Fail( "cannot read " + aEntry.path + ": " + MdwGitError() );
            return KIGIT_WALK::STOP;
        }

        content.AddFile( aEntry.path, *text );
        return MdwCurrentRequestCancelled() ? KIGIT_WALK::STOP : KIGIT_WALK::CONTINUE;
    };

    if( GetGitBackend()->WalkTree( aRepo, aCommit, aFolder, visit ) < 0 && !MdwCurrentRequestCancelled() )
        content.Fail( "cannot list the library repository: " + MdwGitError() );

    return content;
}


/// What contentAt finds of the library at aLibrary in commit aCommit, without reading any other library
CONTENT libraryContent( git_repository* aRepo, const std::string& aCommit, const std::string& aLibrary )
{
    CONTENT                         content;
    std::optional<KIGIT_TREE_ENTRY> entry = GetGitBackend()->TreeEntryAt( aRepo, aCommit, aLibrary );

    if( !entry )
        return content;

    if( entry->mode == GIT_FILEMODE_BLOB || entry->mode == GIT_FILEMODE_BLOB_EXECUTABLE )
    {
        if( !isSymbolFile( aLibrary ) || !isLibraryFile( aLibrary ) )
            return content;

        if( std::optional<std::string> text = GetGitBackend()->ReadBlob( aRepo, entry->oid ) )
            content.AddFile( aLibrary, *text );
        else
            content.Fail( "cannot read " + aLibrary + ": " + MdwGitError() );
    }
    else if( entry->isTree && isFolderLibrary( aLibrary )
             && isLibraryFile( aLibrary + "/member" + memberExt( aLibrary ) ) )
    {
        content = contentAt( aRepo, aCommit, aLibrary );
    }

    return content;
}


/// What commit aCommit holds of the file that member aId lives in. A .kicad_symdir file may hold any symbols under
/// any name, so its whole folder is read
CONTENT memberContent( git_repository* aRepo, const std::string& aCommit, const std::string& aId )
{
    std::string library;
    std::string name;
    CONTENT     content;

    if( !splitMemberId( aId, library, name ) )
        return content;

    if( isFolderLibrary( library ) && !isFootprintLibrary( library ) )
        return libraryContent( aRepo, aCommit, library );

    const std::string file = memberFile( library, name );

    if( std::optional<std::string> text = fileAt( aRepo, aCommit, file ) )
        content.AddFile( file, *text );

    return content;
}


/// Whether aOurs, edited from aBase, keeps what aTheirs changed since aBase. Edits to the same lines cannot say
/// which came first, so they count as kept
bool holdsChanges( const std::string& aBase, const std::string& aOurs, const std::string& aTheirs )
{
    if( aTheirs == aBase || aOurs == aTheirs )
        return true;

    git_merge_file_input  base;
    git_merge_file_input  ours;
    git_merge_file_input  theirs;
    git_merge_file_result merged = {};

    git_merge_file_input_init( &base, GIT_MERGE_FILE_INPUT_VERSION );
    git_merge_file_input_init( &ours, GIT_MERGE_FILE_INPUT_VERSION );
    git_merge_file_input_init( &theirs, GIT_MERGE_FILE_INPUT_VERSION );
    base.ptr = aBase.data();
    base.size = aBase.size();
    ours.ptr = aOurs.data();
    ours.size = aOurs.size();
    theirs.ptr = aTheirs.data();
    theirs.size = aTheirs.size();

    if( git_merge_file( &merged, &base, &ours, &theirs, nullptr ) != 0 )
        return false;

    const bool held = !merged.automergeable || std::string_view( merged.ptr, merged.len ) == aOurs;

    git_merge_file_result_free( &merged );
    return held;
}


/// The library file at aPath below aRoot, as aRepo would store it
std::optional<std::string> diskFile( git_repository* aRepo, const fs::path& aRoot, const std::string& aPath )
{
    std::optional<std::string> text = MDW_ReadFile( aRoot / MdwFsPath( aPath ) );

    return text ? GetGitBackend()->FilterText( aRepo, aPath, *text, true ) : std::nullopt;
}


/// False when the file at aPath below aRoot exists but cannot be read; aText is left empty when it is absent
bool presentFile( git_repository* aRepo, const fs::path& aRoot, const std::string& aPath,
                  std::optional<std::string>& aText )
{
    std::error_code ec;

    aText.reset();

    // libstdc++ also sets ec for a missing file, so only its type says it is absent
    if( fs::symlink_status( aRoot / MdwFsPath( aPath ), ec ).type() == fs::file_type::not_found )
        return true;

    aText = diskFile( aRepo, aRoot, aPath );
    return aText.has_value();
}


/// Attribute rules as git parses them, which reads a CRLF line like an LF one
std::optional<std::string> attributeRules( std::optional<std::string> aText )
{
    if( aText )
    {
        for( std::size_t at = aText->find( "\r\n" ); at != std::string::npos; at = aText->find( "\r\n", at ) )
            aText->erase( at, 1 );
    }

    return aText;
}


/// What the copy at aRoot holds as libraries, with each file's text as aRepo would store it. With aNamesOnly only
/// the library list, which reads no file, and with aFolder only what lies below that folder of the copy
CONTENT contentOnDisk( git_repository* aRepo, const fs::path& aRoot, bool aNamesOnly = false,
                       const std::string& aFolder = std::string() )
{
    CONTENT         content;
    std::error_code ec;
    const fs::path  start = aFolder.empty() ? aRoot : aRoot / MdwFsPath( aFolder );

    for( auto it = fs::recursive_directory_iterator( start, ec ); !ec && it != fs::recursive_directory_iterator();
         it.increment( ec ) )
    {
        const std::string name = MdwUtf8Path( it->path().filename() );

        if( !name.empty() && name[0] == '.' )
        {
            if( it->is_directory() )
                it.disable_recursion_pending();

            continue;
        }

        const bool link = it->is_symlink( ec );
        const bool file = !ec && !link && it->is_regular_file( ec );

        if( ec )
            break;

        // Lexical, since fs::relative resolves links and would name a link by its target
        const std::string path = MdwUtf8Path( it->path().lexically_relative( aRoot ) );
        std::string       generic = path;
        std::replace( generic.begin(), generic.end(), '\\', '/' );

        // A link could lead out of the copy, so its target is neither read nor written
        if( link && ( isLibraryFile( generic ) || isFolderLibrary( generic ) ) )
            content.links.insert( generic );

        if( !file || !isLibraryFile( generic ) )
            continue;

        if( aNamesOnly )
        {
            content.AddLibrary( generic );
            continue;
        }

        // A checkout may have changed line endings, which would otherwise read as a local edit to every member
        std::optional<std::string> text = diskFile( aRepo, aRoot, generic );

        if( !text )
        {
            content.Fail( "cannot read " + generic );
            return content;
        }

        content.AddFile( generic, *text );
    }

    // A partial listing would read the files it missed as removed
    if( ec )
        content.Fail( "cannot list " + MdwUtf8Path( aRoot ) + ": " + ec.message() );

    return content;
}


/// True when aId names a commit of aRepo in full, as every snapshot and revision token does
bool isCommit( git_repository* aRepo, const std::string& aId )
{
    std::optional<KIGIT_COMMIT_RECORD> commit = GetGitBackend()->ReadCommit( aRepo, aId );

    return commit && commit->oid == aId;
}


/// The id of the entry at aPath in commit aCommit, the root tree's for an empty path
std::optional<std::string> entryId( git_repository* aRepo, const std::string& aCommit, const std::string& aPath )
{
    std::optional<KIGIT_TREE_ENTRY> entry = GetGitBackend()->TreeEntryAt( aRepo, aCommit, aPath );

    return entry ? std::optional<std::string>( entry->oid ) : std::nullopt;
}


/// True for a library whose symbols may move between its files, so a member's history follows the folder
bool isSymbolFolder( const std::string& aLibrary )
{
    return isFolderLibrary( aLibrary ) && !isFootprintLibrary( aLibrary );
}


/// The library files at or below aUnit in commit aCommit, each with its blob id
std::map<std::string, std::string> unitBlobs( git_repository* aRepo, const std::string& aCommit,
                                              const std::string& aUnit )
{
    std::map<std::string, std::string> blobs;

    if( !isSymbolFolder( aUnit ) )
    {
        std::optional<KIGIT_TREE_ENTRY> entry = GetGitBackend()->TreeEntryAt( aRepo, aCommit, aUnit );

        if( entry && isBlob( *entry ) && entry->mode != GIT_FILEMODE_LINK )
            blobs[aUnit] = entry->oid;

        return blobs;
    }

    GetGitBackend()->WalkTree( aRepo, aCommit, aUnit,
                               [&]( const KIGIT_TREE_ENTRY& aEntry )
                               {
                                   if( isBlob( aEntry ) && aEntry.mode != GIT_FILEMODE_LINK
                                       && isLibraryFile( aEntry.path ) )
                                   {
                                       blobs[aEntry.path] = aEntry.oid;
                                   }

                                   return KIGIT_WALK::CONTINUE;
                               } );

    return blobs;
}


/// Settle each member of aUnits, member files or .kicad_symdir libraries, at aStart into aCache in one first-parent
/// walk. A move unchanged to another file is no revision, and only changed blobs and folder trees are read
void walkRevisions( git_repository* aRepo, const std::string& aStart, const std::set<std::string>& aUnits,
                    MDW_GIT_SHARED_LIBRARIES::FILE_REVISIONS& aCache )
{
    struct UNIT_WALK
    {
        std::map<std::string, std::string> blobs;
        CONTENT                            content;
        std::set<std::string>              pending;
        std::map<std::string, std::string> revisions;
        std::map<std::string, std::string> tracked; ///< The file whose copy of each pending member is followed
    };

    std::string                                     commit = aStart;
    std::map<std::string, UNIT_WALK>                walks;
    std::map<std::string, std::vector<std::string>> folders;
    std::size_t                                     pending = 0;

    if( !entryId( aRepo, commit, std::string() ) )
        return;

    for( const std::string& unit : aUnits )
    {
        UNIT_WALK& walk = walks[unit];

        walk.blobs = unitBlobs( aRepo, commit, unit );

        for( const auto& [file, blob] : walk.blobs )
        {
            if( std::optional<std::string> text = GetGitBackend()->ReadBlob( aRepo, blob ) )
                walk.content.AddFile( file, *text );
        }

        for( const auto& [id, text] : walk.content.members )
        {
            walk.pending.insert( id );
            walk.tracked[id] = walk.content.File( id );
        }

        pending += walk.pending.size();

        // A folder library's own tree says whether anything in it changed
        const std::size_t slash = unit.rfind( '/' );
        const std::string scope = isSymbolFolder( unit )       ? unit
                                  : slash == std::string::npos ? std::string()
                                                               : unit.substr( 0, slash );

        folders[scope].push_back( unit );
    }

    while( !commit.empty() && pending && !MdwCurrentRequestCancelled() )
    {
        std::optional<KIGIT_COMMIT_RECORD> record = GetGitBackend()->ReadCommit( aRepo, commit );
        std::string parent = record && !record->parents.empty() ? record->parents[0] : std::string();

        // A shallow clone's missing parent ends the history like a root commit
        if( !parent.empty() && !entryId( aRepo, parent, std::string() ) )
            parent.clear();

        const bool                  reformatted = record && isFormatUpgrade( record->message );
        const std::set<std::string> reformattedIds =
                record ? reformattedMembers( record->message ) : std::set<std::string>();

        for( auto& [folder, units] : folders )
        {
            if( !parent.empty() && entryId( aRepo, commit, folder ) == entryId( aRepo, parent, folder ) )
                continue;

            for( const std::string& unit : units )
            {
                UNIT_WALK& walk = walks[unit];

                if( walk.pending.empty() )
                    continue;

                std::map<std::string, std::string> before;

                if( !parent.empty() )
                    before = unitBlobs( aRepo, parent, unit );

                std::set<std::string> changed;

                for( const std::map<std::string, std::string>* blobs : { &walk.blobs, &before } )
                {
                    for( const auto& [file, blob] : *blobs )
                    {
                        auto now = walk.blobs.find( file );
                        auto was = before.find( file );

                        if( now == walk.blobs.end() || was == before.end() || now->second != was->second )
                            changed.insert( file );
                    }
                }

                if( !parent.empty() && changed.empty() )
                    continue;

                // Only the files that changed are read, and a member in none of them kept its text
                std::map<std::string, std::string> olderTexts;
                CONTENT                            older;

                for( const std::string& file : changed )
                {
                    if( auto was = before.find( file ); was != before.end() )
                    {
                        if( std::optional<std::string> text = GetGitBackend()->ReadBlob( aRepo, was->second ) )
                        {
                            olderTexts[file] = *text;
                            older.AddFile( file, *text );
                        }
                    }
                }

                for( auto it = walk.pending.begin(); it != walk.pending.end(); )
                {
                    const std::string&               id = *it;
                    std::string&                     file = walk.tracked[id];
                    const std::optional<std::string> now = walk.content.Member( id, file );
                    std::optional<std::string>       was = changed.count( file ) ? older.Member( id, file ) : now;

                    // A member follows its own file. Only where that file lacked it is it followed into another,
                    // one holding it unchanged first, so moving it is no revision
                    if( !was )
                    {
                        std::vector<std::pair<std::string, std::string>> others;

                        for( const std::string& other : older.Copies( id ) )
                            others.emplace_back( other, *older.Member( id, other ) );

                        for( const std::string& other : walk.content.Copies( id ) )
                        {
                            if( !changed.count( other ) )
                                others.emplace_back( other, *walk.content.Member( id, other ) );
                        }

                        auto pick = std::find_if( others.begin(), others.end(),
                                                  [&]( const auto& aOther )
                                                  {
                                                      return aOther.second == now;
                                                  } );

                        if( pick == others.end() )
                            pick = others.begin();

                        if( pick != others.end() )
                        {
                            file = pick->first;
                            was = pick->second;
                        }
                    }

                    // A format upgrade revises no member it kept, but one it added or removed still dates from it
                    const bool kept = ( reformatted || reformattedIds.count( id ) ) && now && was;

                    if( parent.empty() || ( now != was && !kept ) )
                    {
                        walk.revisions[id] = commit;
                        it = walk.pending.erase( it );
                        --pending;
                    }
                    else
                    {
                        ++it;
                    }
                }

                walk.content.Drop( changed );

                for( const auto& [file, text] : olderTexts )
                    walk.content.AddFile( file, text );

                walk.blobs = std::move( before );
            }
        }

        commit = parent;
    }

    // A walk cut short leaves its unsettled units to be walked again
    for( auto& [unit, walk] : walks )
    {
        if( walk.pending.empty() )
            aCache[{ unit, aStart }] = std::move( walk.revisions );
    }
}


/// The revision of each of aIds at aStart, walking history only for units aCache lacks. aKnown, when given, is
/// what aStart holds, and says which file each member is in
std::map<std::string, std::string> memberRevisions( git_repository* aRepo, const std::string& aStart,
                                                    const std::set<std::string>&              aIds,
                                                    MDW_GIT_SHARED_LIBRARIES::FILE_REVISIONS& aCache,
                                                    const CONTENT*                            aKnown = nullptr )
{
    std::map<std::string, std::string> units;
    std::set<std::string>              unknown;

    for( const std::string& id : aIds )
    {
        std::string library;
        std::string name;

        if( !splitMemberId( id, library, name ) )
            continue;

        const std::string file = aKnown && aKnown->files.count( id ) ? aKnown->files.at( id )
                                                                     : memberContent( aRepo, aStart, id ).File( id );

        if( file.empty() )
            continue;

        units[id] = isSymbolFolder( library ) ? library : file;

        if( !aCache.count( { units[id], aStart } ) )
            unknown.insert( units[id] );
    }

    if( !unknown.empty() )
        walkRevisions( aRepo, aStart, unknown, aCache );

    std::map<std::string, std::string> result;

    for( const auto& [id, unit] : units )
    {
        auto revisions = aCache.find( { unit, aStart } );

        if( revisions == aCache.end() )
            continue;

        if( auto it = revisions->second.find( id ); it != revisions->second.end() )
            result[id] = it->second;
    }

    return result;
}


void addLibraries( const CONTENT& aContent, google::protobuf::RepeatedPtrField<mdw::SharedLibrary>& aOut )
{
    for( const auto& [path, type] : aContent.libraries )
    {
        mdw::SharedLibrary* library = aOut.Add();
        std::string         stem = path.substr( path.rfind( '/' ) == std::string::npos ? 0 : path.rfind( '/' ) + 1 );

        stem = stem.substr( 0, stem.rfind( '.' ) );
        library->set_library_id( path );
        library->set_display_name( stem );
        library->set_type( type );
        library->set_relative_path( path );
    }
}


ws::DocumentKind memberKind( const std::string& aId )
{
    std::string library;
    std::string name;

    splitMemberId( aId, library, name );
    return isFootprintLibrary( library ) ? ws::DK_FOOTPRINT : ws::DK_SYMBOL_LIBRARY;
}


std::string configValue( git_repository* aRepo, const char* aKey )
{
    wxString value;

    if( !GetGitBackend()->GetConfigString( aRepo, wxString::FromUTF8( aKey ), value ) )
        return std::string();

    return value.utf8_string();
}


bool setConfigValue( git_repository* aRepo, const char* aKey, const std::string& aValue )
{
    return GetGitBackend()->SetConfigString( aRepo, wxString::FromUTF8( aKey ), wxString::FromUTF8( aValue ) );
}


/// True when aRepo already reads as MdwGitCommittedBytes leaves it, whichever config level says so
bool keepsCommittedBytes( git_repository* aRepo )
{
    return configValue( aRepo, "core.autocrlf" ) == "false" && configValue( aRepo, "core.eol" ) == "lf"
           && ( git_repository_is_bare( aRepo )
                || configValue( aRepo, "core.attributesfile" ) == MDW_GIT_NO_USER_ATTRIBUTES );
}


bool emptyOrAbsent( const fs::path& aRoot )
{
    std::error_code ec;

    if( !fs::exists( aRoot, ec ) )
        return true;

    return fs::is_directory( aRoot, ec ) && fs::directory_iterator( aRoot, ec ) == fs::directory_iterator();
}


/// Remove aPath only when it is an empty folder; true when nothing is left there
bool removeEmptyFolder( const fs::path& aPath )
{
#ifdef _WIN32
    return _wrmdir( aPath.c_str() ) == 0 || errno == ENOENT;
#else
    return ::rmdir( aPath.c_str() ) == 0 || errno == ENOENT;
#endif
}


/// Empty on success, else why aPath could not be written
std::string writeFile( const fs::path& aPath, const std::string& aText )
{
    std::error_code ec;
    fs::create_directories( aPath.parent_path(), ec );

    if( ec )
        return ec.message();

    ec = MDW_ReplaceFile( aPath, aText );
    return ec ? ec.message() : std::string();
}


/// Point a new refs/kicad/pre-update/<UTC time> at aCommit, answering its name or empty when it cannot
std::string keepCommit( git_repository* aRepo, const std::string& aCommit )
{
    const std::string stamp = wxDateTime::Now().Format( wxS( "%Y%m%dT%H%M%SZ" ), wxDateTime::UTC ).utf8_string();

    for( int attempt = 0; attempt < 100; ++attempt )
    {
        const std::string name = std::string( "refs/kicad/pre-update/" ) + stamp
                                 + ( attempt ? "-" + std::to_string( attempt ) : std::string() );
        const int rc = GetGitBackend()->SetReference( aRepo, name, aCommit, std::string(), "pre-update" );

        if( rc == 0 )
            return name;

        if( rc != GIT_EMODIFIED )
            break;
    }

    return std::string();
}


struct COPY
{
    KIGIT::GitRepositoryPtr repo{ nullptr };
    fs::path                root;
    std::string             branch;
    bool                    cloned = false; ///< This request wrote it

    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions; ///< Held by a request that writes the copy

    git_repository* Handle() const { return repo.get(); }

    std::string UpstreamRef() const { return std::string( "refs/remotes/" ) + REMOTE + "/" + branch; }

    std::optional<std::string> Upstream() const { return GetGitBackend()->ReadReference( repo.get(), UpstreamRef() ); }

    std::optional<std::string> Head() const { return GetGitBackend()->ReadReference( repo.get(), "HEAD" ); }

    bool Pinned() const { return KIGIT_COMMON( repo.get() ).IsHeadDetached(); }

    /// Hold the copy's action lock to the end of the request, so another process's staging or branch switch
    /// cannot land between a write's reads and its writes, and refuse a copy switched to another branch
    ws::WorkspaceError LockForWrite()
    {
        actions = KIGIT::RepoActionLock( Handle() );

        if( ws::WorkspaceError error = MdwGitLockActions( actions.get() ); error.code() != ws::WE_OK )
            return error;

        KIGIT_COMMON common( Handle() );

        // Writes publish to branch, so a copy switched elsewhere would publish what it does not show
        if( !common.IsHeadDetached() && common.GetCurrentBranchName().utf8_string() != branch )
            return MdwFail( ws::WE_VERSION_CONFLICT, "The copy is switched away from its branch " + branch );

        return keepCommittedBytes();
    }

private:
    /// Move a copy cloned before MdwGitCommittedBytes onto it. A file still as the old settings checked it out is
    /// written back as committed, and one edited since is left as it is
    ws::WorkspaceError keepCommittedBytes()
    {
        if( keepsCommittedBytes( Handle() ) )
            return MdwOk();

        const std::optional<std::string> head = Head();
        std::string                      failed;

        auto visit = [&]( const KIGIT_TREE_ENTRY& aEntry )
        {
            const KIGIT_WALK next = MdwCurrentRequestCancelled() ? KIGIT_WALK::STOP : KIGIT_WALK::CONTINUE;

            if( aEntry.mode != GIT_FILEMODE_BLOB && aEntry.mode != GIT_FILEMODE_BLOB_EXECUTABLE )
                return next;

            const std::optional<std::string> blob = GetGitBackend()->ReadBlob( Handle(), aEntry.oid );
            const std::optional<std::string> checkedOut =
                    blob ? GetGitBackend()->FilterText( Handle(), aEntry.path, *blob, false ) : std::nullopt;

            if( !checkedOut )
            {
                failed = "Cannot read " + aEntry.path + " as checked out: " + MdwGitError();
                return KIGIT_WALK::STOP;
            }

            const fs::path file = MdwLinkFreePath( root, aEntry.path, false );

            if( *checkedOut == *blob || file.empty() || MDW_ReadFile( file ) != checkedOut )
                return next;

            const unsigned mode = aEntry.mode == GIT_FILEMODE_BLOB_EXECUTABLE ? 0777 : 0666;

            // A save landing since the read is an edit, which keeps its line endings like any other
            auto unsaved = [&]()
            {
                return MDW_ReadFile( file ) == checkedOut;
            };

            if( std::error_code ec = MDW_ReplaceFileIf( file, *blob, unsaved, mode );
                ec && ec != std::errc::operation_canceled )
            {
                failed = "Cannot write " + aEntry.path + " as committed: " + ec.message();
                return KIGIT_WALK::STOP;
            }

            return next;
        };

        // Stopped part way, the settings stay as they were, and the next write carries on where this one ended
        const int walked = head ? GetGitBackend()->WalkTree( Handle(), *head, std::string(), visit ) : 0;

        if( !failed.empty() )
            return MdwFail( ws::WE_INTERNAL, failed );

        if( MdwCurrentRequestCancelled() )
            return MdwCancelled();

        if( walked < 0 )
            return gitFailure( "cannot list the copy" );

        if( MdwGitCommittedBytes( Handle() ) != GIT_OK )
            return gitFailure( "cannot set the copy's line endings" );

        // A handle keeps the attribute settings it first read, so only a new one sees the change
        repo.reset( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( root ) ) );

        return repo ? MdwOk() : gitFailure( "cannot reopen the copy" );
    }
};


// Only a library source advertises the group, so a project session answers as if the calls did not exist
ws::WorkspaceError noLibrarySource()
{
    return MdwFail( ws::WE_NOT_SUPPORTED, "This session is not a library source" );
}


/// The first read failure among aContents, since acting on a partial read would discard what it missed, or the
/// first member one of them holds twice
ws::WorkspaceError unreadable( std::initializer_list<const CONTENT*> aContents )
{
    for( const CONTENT* content : aContents )
    {
        if( !content->error.empty() )
            return MdwFail( ws::WE_INTERNAL, content->error );

        if( std::string why = content->Twice(); !why.empty() )
            return MdwFail( ws::WE_INTERNAL, why );
    }

    return MdwOk();
}


/// True when every member aOurs holds twice is held by aBase in the same files with the same text, so it came with
/// the copy rather than from an edit here
bool sameTwice( const CONTENT& aOurs, const CONTENT& aBase )
{
    for( const auto& [id, copies] : aOurs.twice )
    {
        const std::vector<std::string> files = aOurs.Copies( id );

        if( files != aBase.Copies( id ) )
            return false;

        for( const std::string& file : files )
        {
            if( aOurs.Member( id, file ) != aBase.Member( id, file ) )
                return false;
        }
    }

    return true;
}


/// An error when a library or member aTree holds is a link in aDisk, which would otherwise read as removed
ws::WorkspaceError linkedMembers( const CONTENT& aTree, const CONTENT& aDisk )
{
    for( const std::string& link : aDisk.links )
    {
        bool held = aTree.libraries.count( link ) > 0;

        for( auto it = aTree.files.begin(); !held && it != aTree.files.end(); ++it )
            held = it->second == link || it->second.starts_with( link + "/" );

        if( held )
            return MdwFail( ws::WE_VERSION_CONFLICT, link + " is a link in the copy, and links are not followed" );
    }

    return MdwOk();
}


/// An error when the copy's index differs from HEAD, since moving the copy rewrites the index
ws::WorkspaceError stagedChanges( git_repository* aRepo, const std::string& aHead )
{
    std::optional<std::vector<std::string>> staged = GetGitBackend()->ChangedPaths( aRepo, aHead, std::string() );

    if( !staged )
        return gitFailure( "cannot read the copy's index" );

    if( !staged->empty() )
    {
        return MdwFail( ws::WE_VERSION_CONFLICT,
                        "The copy has changes staged with git; commit or unstage them before getting changes" );
    }

    return MdwOk();
}


/// Every member aContent holds, or only those of aLibrary when one is named
std::set<std::string> memberIds( const CONTENT& aContent, const std::string& aLibrary = std::string() )
{
    std::set<std::string> ids;

    for( const auto& [id, text] : aContent.members )
    {
        std::string library;
        std::string name;

        if( splitMemberId( id, library, name ) && ( aLibrary.empty() || library == aLibrary ) )
            ids.insert( id );
    }

    return ids;
}


/// True when the copy's index and libraries are exactly HEAD. An untracked file in no library is no edit
bool unchanged( git_repository* aRepo )
{
    git_status_options options;
    git_status_list*   list = nullptr;

    git_status_options_init( &options, GIT_STATUS_OPTIONS_VERSION );
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    if( git_status_list_new( &list, aRepo, &options ) != 0 )
        return false;

    KIGIT::GitStatusListPtr listPtr( list );

    for( size_t ii = 0; ii < git_status_list_entrycount( list ); ++ii )
    {
        const git_status_entry* entry = git_status_byindex( list, ii );

        if( entry->status != GIT_STATUS_WT_NEW || isLibraryFile( entry->index_to_workdir->new_file.path ) )
            return false;
    }

    return true;
}


/// The revision aRevisions names member aId by, empty when aContent does not hold it
std::string revisionOf( const std::map<std::string, std::string>& aRevisions, const CONTENT& aContent,
                        const std::string& aId )
{
    auto it = aRevisions.find( aId );
    return aContent.Member( aId ) && it != aRevisions.end() ? it->second : std::string();
}


/// Report member aId in aFile as upstream changed it from aBase to aTheirs, whose revisions name each side
void addChange( google::protobuf::RepeatedPtrField<mdw::MemberChange>& aOut, const std::string& aId,
                const std::string& aFile, const CONTENT& aBase, const CONTENT& aTheirs,
                const std::map<std::string, std::string>& aBaseRev,
                const std::map<std::string, std::string>& aTheirsRev )
{
    mdw::MemberChange* change = aOut.Add();
    change->mutable_member()->set_item_id( aId );
    change->mutable_member()->set_path_hint( aFile );
    change->set_change( !aBase.Member( aId )     ? mdw::CT_ADDED
                        : !aTheirs.Member( aId ) ? mdw::CT_REMOVED
                                                 : mdw::CT_MODIFIED );
    change->set_old_revision_label( shortId( revisionOf( aBaseRev, aBase, aId ) ) );
    change->set_new_revision_label( shortId( revisionOf( aTheirsRev, aTheirs, aId ) ) );
}


/// Check aCommit out into the copy as CheckoutCommit does, a file in the way being a conflict to resolve
ws::WorkspaceError checkout( git_repository* aRepo, const std::string& aCommit, const std::string& aBranch )
{
    const int rc = GetGitBackend()->CheckoutCommit( aRepo, aCommit, aBranch );

    if( rc == GIT_ECONFLICT || rc == GIT_EMERGECONFLICT )
        return MdwFail( ws::WE_VERSION_CONFLICT, "Files in the copy would be overwritten: " + MdwGitError() );

    if( rc != 0 )
        return gitFailure( "cannot move the copy to the snapshot" );

    return MdwOk();
}


ws::WorkspaceError notCopy( const std::string& aRoot )
{
    return MdwFail( ws::WE_INVALID_PARAMETER,
                    aRoot + " is not empty and is not a copy of this library repository, so it is left alone" );
}


/// Why an edit of member aId begun at aHead cannot go on top of the copy's upstream as last fetched, or nothing when
/// it can. Upstream past the copy conflicts only where it changed the member since the two parted
std::optional<ws::WorkspaceError> upstreamChange( COPY& aCopy, const std::string& aHead, const std::string& aId,
                                                  MDW_GIT_SHARED_LIBRARIES::FILE_REVISIONS& aRevisions )
{
    const std::optional<std::string> latest = aCopy.Upstream();

    if( !latest || *latest == aHead
        || GetGitBackend()->IsDescendant( aCopy.Handle(), aHead, *latest ).value_or( false ) )
    {
        return std::nullopt;
    }

    const std::optional<std::string> parted = GetGitBackend()->MergeBase( aCopy.Handle(), aHead, *latest );

    if( !parted )
        return gitFailure( "the copy shares no history with the library repository" );

    const CONTENT before = memberContent( aCopy.Handle(), *parted, aId );
    const CONTENT after = memberContent( aCopy.Handle(), *latest, aId );

    if( ws::WorkspaceError error = unreadable( { &before, &after } ); error.code() != ws::WE_OK )
        return error;

    if( before.Member( aId ) == after.Member( aId ) )
        return MdwFail( ws::WE_COPY_BEHIND, "The library changed since the copy was updated" );

    std::string library;
    std::string name;

    splitMemberId( aId, library, name );

    const std::string changed = memberRevisions( aCopy.Handle(), *latest, { aId }, aRevisions, &after )[aId];
    const std::optional<KIGIT_COMMIT_RECORD> commit =
            GetGitBackend()->ReadCommit( aCopy.Handle(), changed.empty() ? *latest : changed );
    const std::string who = commit ? commit->authorName : std::string( "a teammate" );

    return MdwFail( ws::WE_VERSION_CONFLICT, name + " was changed by " + who + " since this edit began" );
}


/// Take check-in aCommit of aFiles back to its parent aParent, the branch only while it still names aCommit, and
/// answer what stayed in the copy, empty when nothing did
std::string uncommit( COPY& aCopy, const std::string& aParent, const std::string& aCommit,
                      const std::map<std::string, std::optional<std::string>>& aFiles )
{
    if( GetGitBackend()->SetReference( aCopy.Handle(), "refs/heads/" + aCopy.branch, aParent, aCommit,
                                       "refused check-in" ) )
    {
        return "the check-in";
    }

    std::vector<std::string> paths;

    for( const auto& [path, text] : aFiles )
        paths.push_back( path );

    if( GetGitBackend()->MoveStaging( aCopy.Handle(), aCommit, aParent, paths ) )
        return "the check-in's staging";

    return std::string();
}

} // namespace


MDW_GIT_SHARED_LIBRARIES::MDW_GIT_SHARED_LIBRARIES( MDW_BACKEND_SERVER& aServer, MDW_GIT_REMOTE& aRemote,
                                                    std::mutex& aRepoMutex, const fs::path& aCacheRoot ) :
        m_server( aServer ),
        m_remote( aRemote ),
        m_repoMutex( aRepoMutex ),
        m_cacheRoot( aCacheRoot )
{
}


GIT_IDENTITY MDW_GIT_SHARED_LIBRARIES::identity( git_repository* aRepo ) const
{
    return MdwGitIdentity( aRepo, m_server.HelloContext().author(), wxString::FromUTF8( m_remote.Username() ) );
}


std::string MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( const std::string& aLocator )
{
    const wxString          locator = wxString::FromUTF8( aLocator );
    const KIGIT::REMOTE_URL parsed = KIGIT::ParseRemoteURL( locator );
    std::string             url = parsed.url.utf8_string();

    // ParseRemoteURL leaves git:// as a local path, but a forge serves it like any other remote
    const bool remote =
            parsed.connType != KIGIT::GIT_CONN_TYPE::GIT_CONN_LOCAL || locator.Lower().StartsWith( wxS( "git://" ) );

    while( !url.empty() && ( url.back() == '/' || url.back() == '\\' ) )
        url.pop_back();

    // A forge serves a repository with or without the suffix, while a local folder's name is its own
    if( remote && url.ends_with( ".git" ) )
    {
        url.erase( url.size() - 4 );

        while( !url.empty() && url.back() == '/' )
            url.pop_back();
    }

    return url;
}


std::optional<std::string> MDW_GIT_SHARED_LIBRARIES::SourceMemberFile( const std::string&  aItemId,
                                                                       std::string&        aRevision,
                                                                       ws::WorkspaceError& aError ) const
{
    if( !m_sourceMember || locator().empty() )
    {
        aError = noLibrarySource();
        return std::nullopt;
    }

    return m_sourceMember( aItemId, aRevision, aError );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::ReleaseMember( const MDW_LIFECYCLE_STORE&         aStore,
                                                            const mdw::ReleaseRevisionRequest& aRequest,
                                                            mdw::ReleaseRevisionReply&         aReply )
{
    if( !m_releaseMember || locator().empty() )
        return noLibrarySource();

    return m_releaseMember( aStore, aRequest, aReply );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::MemberReleases( const std::set<std::string>& aItemIds,
                                                             std::map<std::string, std::vector<MDW_GIT_RELEASE>>& aOut )
{
    if( !m_memberReleases || locator().empty() )
        return noLibrarySource();

    return m_memberReleases( aItemIds, aOut );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::MemberInfo( const std::string& aItemId, ws::ItemInfo& aOut,
                                                         std::string& aPath )
{
    if( !m_memberInfo || locator().empty() )
        return noLibrarySource();

    return m_memberInfo( aItemId, aOut, aPath );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::MemberHistory( const std::string&         aItemId,
                                                            std::vector<ws::Revision>& aOut )
{
    if( !m_memberHistory || locator().empty() )
        return noLibrarySource();

    return m_memberHistory( aItemId, aOut );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::MemberTransitions( const mdw::GetAvailableTransitionsRequest& aRequest,
                                                                mdw::GetAvailableTransitionsReply&         aReply )
{
    if( !m_memberTransitions || locator().empty() )
        return noLibrarySource();

    return m_memberTransitions( aRequest, aReply );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::TransitionMember( const mdw::TransitionLifecycleRequest& aRequest,
                                                               mdw::TransitionLifecycleReply&         aReply )
{
    if( !m_transitionMember || locator().empty() )
        return noLibrarySource();

    return m_transitionMember( aRequest, aReply );
}


std::string MDW_GIT_SHARED_LIBRARIES::locator() const
{
    return m_server.HelloContext().workspace().locator();
}


std::string MDW_GIT_SHARED_LIBRARIES::boundRoot() const
{
    return m_server.HelloContext().workspace().local_root();
}


bool MDW_GIT_SHARED_LIBRARIES::IsMemberId( const std::string& aItemId )
{
    std::string library;
    std::string name;

    return splitMemberId( aItemId, library, name );
}


bool MDW_GIT_SHARED_LIBRARIES::IsCopy( const std::string& aRoot ) const
{
    const fs::path root = MdwFsPath( aRoot );

    if( locator().empty() )
        return false;

    KIGIT::GitRepositoryPtr repo( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( root ) ) );

    if( !repo )
        return false;

    const char*     workdir = git_repository_workdir( repo.get() );
    std::error_code ec;

    // libgit2 reports the real path, which differs from aRoot under a symlinked temp folder
    return workdir && fs::equivalent( MdwFsPath( workdir ), root, ec )
           && configValue( repo.get(), COPY_KEY ) == NormalisedLocator( locator() );
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::UnheldTip( std::string& aTip )
{
    aTip.clear();

    if( !IsCopy( boundRoot() ) )
        return MdwOk();

    KIGIT::GitRepositoryPtr repo( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( MdwFsPath( boundRoot() ) ) ) );
    std::optional<wxString> tip;

    if( !repo || KIGIT_COMMON( repo.get() ).IsHeadDetached() )
        return MdwOk();

    const wxString branch = wxString::FromUTF8( "refs/heads/" + configValue( repo.get(), BRANCH_KEY ) );

    ws::WorkspaceError read = m_remote.Transfer( repo.get(), REMOTE,
                                                 [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
                                                 {
                                                     tip = aTransport.ReadRemoteRef( branch );
                                                     return tip ? GIT_OK : GIT_ERROR;
                                                 } );

    const std::string id = tip ? tip->utf8_string() : std::string();

    // Read once any update under way has finished, which would otherwise read as still lacking the tip
    std::lock_guard<std::mutex> lock( m_repoMutex );
    std::optional<std::string>  head = GetGitBackend()->ReadReference( repo.get(), "HEAD" );

    // The copy's own check-in moved the tip to a commit it holds
    if( id.empty() || !head || id == *head
        || GetGitBackend()->IsDescendant( repo.get(), *head, id ) == std::optional<bool>( true ) )
    {
        return read;
    }

    aTip = id;
    return read;
}


std::optional<std::string> MDW_GIT_SHARED_LIBRARIES::MemberFile( git_repository* aRepo, const std::string& aItemId,
                                                                 std::string& aRevision, ws::WorkspaceError& aError )
{
    std::string library;
    std::string name;

    // The latest snapshot is the fetched branch tip, as every other reader takes it
    if( aRevision.empty() )
    {
        const std::string tip = std::string( "refs/remotes/" ) + REMOTE + "/" + configValue( aRepo, BRANCH_KEY );

        aRevision = GetGitBackend()->ReadReference( aRepo, tip ).value_or( std::string() );
    }

    if( !splitMemberId( aItemId, library, name ) || !isCommit( aRepo, aRevision ) )
    {
        aError = MdwFail( ws::WE_REVISION_NOT_FOUND, "No such revision of " + aItemId );
        return std::nullopt;
    }

    CONTENT                    content = memberContent( aRepo, aRevision, aItemId );
    const std::string          file = content.File( aItemId );
    std::optional<std::string> member = content.Member( aItemId );

    if( !content.error.empty() )
    {
        aError = MdwFail( ws::WE_INTERNAL, content.error );
        return std::nullopt;
    }

    // Another member held twice at this revision does not touch this one, and history cannot be repaired
    if( std::string why = content.Twice( aItemId ); !why.empty() )
    {
        aError = MdwFail( ws::WE_INTERNAL, why );
        return std::nullopt;
    }

    if( !member )
    {
        aError = MdwFail( ws::WE_REVISION_NOT_FOUND, aItemId + " is not in revision " + aRevision );
        return std::nullopt;
    }

    if( content.derived.contains( aItemId ) )
    {
        aError = MdwFail( ws::WE_NOT_SUPPORTED, aItemId + " is derived, and this backend cannot flatten it" );
        return std::nullopt;
    }

    if( isFootprintLibrary( library ) )
        return member;

    return content.headers[file] + *member + "\n)\n";
}


ws::WorkspaceError MDW_GIT_SHARED_LIBRARIES::LocalChanges( const mdw::ListLocalChangesRequest& aRequest,
                                                           mdw::ListLocalChangesReply&         aReply ) const
{
    const fs::path          root = MdwFsPath( aRequest.local_root() );
    KIGIT::GitRepositoryPtr repoPtr( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( root ) ) );
    git_repository*         repo = repoPtr.get();

    if( !repo )
        return gitFailure( "cannot open " + aRequest.local_root() );

    // Empty while the branch is unborn
    const std::string head = GetGitBackend()->ReadReference( repo, "HEAD" ).value_or( std::string() );
    const CONTENT     before = contentAt( repo, head );
    CONTENT           now = contentOnDisk( repo, root );

    now.Drop( before.links );

    if( MdwCurrentRequestCancelled() )
        return MdwCancelled();

    if( ws::WorkspaceError error = unreadable( { &before, &now } ); error.code() != ws::WE_OK )
        return error;

    if( ws::WorkspaceError error = linkedMembers( before, now ); error.code() != ws::WE_OK )
        return error;

    std::set<std::string> ids;
    std::set<std::string> based;

    for( const auto& [id, text] : before.members )
        ids.insert( id );

    for( const auto& [id, text] : now.members )
        ids.insert( id );

    std::vector<std::pair<std::string, mdw::ChangeType>> rows;

    for( const std::string& id : ids )
    {
        std::optional<std::string> was = before.Member( id );
        std::optional<std::string> is = now.Member( id );

        if( sameMember( before, now, id ) )
            continue;

        rows.emplace_back( id, !was ? mdw::CT_ADDED : !is ? mdw::CT_REMOVED : mdw::CT_MODIFIED );

        if( was )
            based.insert( id );
    }

    std::map<std::string, std::string> bases = head.empty()
                                                       ? std::map<std::string, std::string>()
                                                       : memberRevisions( repo, head, based, m_revisions, &before );

    // The token binds the page size and this listing, so a copy changed mid-walk conflicts
    std::string listing = head;

    for( const auto& [id, change] : rows )
    {
        listing += '\0' + id + '\0' + std::to_string( change ) + '\0' + now.Member( id ).value_or( std::string() )
                   + '\0' + now.File( id );
    }

    const std::string snapshot = std::to_string( std::hash<std::string>{}( listing ) );
    size_t            offset = 0;
    size_t            end = 0;
    std::string       next;

    if( ws::WorkspaceError error = MdwPageRange( aRequest.page(), rows.size(), snapshot, offset, end, next );
        error.code() != ws::WE_OK )
    {
        return error;
    }

    const MDW_GIT_RELEASES releases( repo, m_remote, REMOTE, "rel" );

    for( size_t ii = offset; ii < end; ++ii )
    {
        const auto& [id, change] = rows[ii];
        std::string library;
        std::string name;

        splitMemberId( id, library, name );

        mdw::LocalChange* row = aReply.add_changes();

        row->mutable_checkin()->set_name( "checkin" );
        row->mutable_checkin()->set_state( ws::PS_ALLOWED );

        if( std::string why = checkInDenial( releases, before, now, id, m_holdsClaim ); !why.empty() )
        {
            row->mutable_checkin()->set_state( ws::PS_DENIED );
            row->mutable_checkin()->set_reason( why );
        }

        row->mutable_member()->set_item_id( id );
        const std::string file = now.files.count( id ) ? now.File( id ) : before.File( id );

        row->mutable_member()->set_path_hint( file );
        row->set_library_id( library );
        row->set_name( name );
        row->set_change( change );
        row->set_kind( memberKind( id ) );
        row->set_relative_path( file );

        if( before.files.count( id ) )
            row->set_base_relative_path( before.File( id ) );

        // A file HEAD does not hold, such as a new one, has no format a save could be newer than
        if( auto held = before.headers.find( before.files.count( id ) ? before.File( id ) : file );
            held != before.headers.end() )
        {
            row->set_base_format( KIGIT::FormatVersion( held->second ) );
        }

        if( auto it = bases.find( id ); it != bases.end() )
            row->set_base_revision_id( it->second );
    }

    aReply.set_next_page_token( next );
    return MdwOk();
}


/// Clone aUrl into aStaging, an empty folder MdwFillStagedFolder made, marked as a copy of aKey, checked out
/// detached at aPin when one is given and holding the release mirrors that mark a release published
static ws::WorkspaceError cloneStaged( MDW_GIT_REMOTE& aRemote, const std::string& aUrl, const std::string& aKey,
                                       bool aBare, const std::string& aPin, const fs::path& aStaging )
{
    git_repository* repo = nullptr;

    ws::WorkspaceError result = aRemote.Transfer(
            nullptr, std::string(),
            [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
            {
                return aTransport.Clone( wxString::FromUTF8( aUrl ), wxString::FromUTF8( MdwUtf8Path( aStaging ) ),
                                         &repo, aBare, MdwGitCommittedBytes );
            } );

    if( result.code() == ws::WE_OK )
    {
        KIGIT::GitRepositoryPtr staged( repo );
        const std::string       branch = KIGIT_COMMON( repo ).GetCurrentBranchName().utf8_string();

        if( branch.empty() )
            result = MdwFail( ws::WE_ITEM_NOT_FOUND, aKey + " has no branch to share libraries on" );
        else if( !setConfigValue( repo, COPY_KEY, aKey ) || !setConfigValue( repo, BRANCH_KEY, branch ) )
            result = gitFailure( "cannot mark the copy" );
        else if( !aPin.empty() && !isCommit( repo, aPin ) )
            result = MdwFail( ws::WE_REVISION_NOT_FOUND, aPin + " is not a snapshot of this source" );
        else if( !aPin.empty() && GetGitBackend()->CheckoutCommit( repo, aPin, std::string() ) != 0 )
            result = gitFailure( "cannot move the copy to the snapshot" );
        else if( result = MDW_GIT_RELEASES( repo, aRemote, REMOTE, "rel" ).Fetch(); result.code() == ws::WE_OK )
            result = MDW_GIT_RELEASES( repo, aRemote, REMOTE, STATE_KIND ).Fetch();
    }

    return result;
}


void MDW_GIT_SHARED_LIBRARIES::Register()
{
    // Open the copy at aRoot, cloning the locator into it at aPin when it is empty or absent and aClone allows
    auto open = [this]( const std::string& aRoot, bool aClone, COPY& aCopy,
                        const std::string& aPin = std::string() ) -> ws::WorkspaceError
    {
        const std::string url = locator();

        if( url.empty() )
            return noLibrarySource();

        aCopy.root = MdwFsPath( aRoot );

        if( aCopy.root.empty() || !aCopy.root.is_absolute() )
            return MdwFail( ws::WE_INVALID_PARAMETER, "The copy needs an absolute folder" );

        if( emptyOrAbsent( aCopy.root ) )
        {
            if( !aClone )
                return MdwFail( ws::WE_ITEM_NOT_FOUND, aRoot + " holds no copy yet" );

            ws::WorkspaceError cloned = MdwFillStagedFolder(
                    aCopy.root,
                    [this, url, aPin]( const fs::path& aStaging )
                    {
                        return cloneStaged( m_remote, url, NormalisedLocator( url ), false, aPin, aStaging );
                    } );

            if( cloned.code() != ws::WE_OK )
                return cloned;

            aCopy.cloned = true;
        }
        else if( !IsCopy( aRoot ) )
        {
            return notCopy( aRoot );
        }

        aCopy.repo.reset( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( aCopy.root ) ) );

        if( !aCopy.repo )
            return gitFailure( "cannot open " + aRoot );

        aCopy.branch = configValue( aCopy.Handle(), BRANCH_KEY );

        if( aCopy.branch.empty() )
            return notCopy( aRoot );

        return MdwOk();
    };

    // A bare clone of the locator under the plugin's data folder, made on first use and shared by its readers
    auto cached = [this]( COPY& aCopy ) -> ws::WorkspaceError
    {
        const std::string url = locator();

        if( url.empty() )
            return noLibrarySource();

        if( !m_cacheRoot.is_absolute() )
            return MdwFail( ws::WE_INTERNAL, "There is no folder to keep the library cache in" );

        const std::string key = NormalisedLocator( url );

        // A short name keeps Windows paths under MAX_PATH, and the copy key catches a collision
        const std::string name = GetGitBackend()->BlobId( key ).substr( 0, 16 );

        if( name.empty() )
            return gitFailure( "cannot name the library cache" );

        aCopy.root = m_cacheRoot / name;

        std::error_code ec;

        if( !fs::exists( aCopy.root, ec ) )
        {
            ws::WorkspaceError cloned =
                    MdwFillStagedFolder( aCopy.root,
                                         [this, url, key]( const fs::path& aStaging )
                                         {
                                             return cloneStaged( m_remote, url, key, true, std::string(), aStaging );
                                         } );

            // Another reader may have installed the same cache meanwhile, which serves as well
            if( cloned.code() == ws::WE_OK )
                aCopy.cloned = true;
            else if( cloned.code() != ws::WE_VERSION_CONFLICT || !fs::exists( aCopy.root, ec ) )
                return cloned;
        }

        aCopy.repo.reset( GetGitBackend()->OpenRepositoryAt( MdwUtf8Path( aCopy.root ) ) );

        if( !aCopy.repo )
            return gitFailure( "cannot open the library cache" );

        aCopy.branch = configValue( aCopy.Handle(), BRANCH_KEY );

        if( aCopy.branch.empty() || configValue( aCopy.Handle(), COPY_KEY ) != key )
            return gitFailure( "the library cache belongs to another repository" );

        return MdwOk();
    };

    // A read answers from the copy once there is one, so it never writes a folder Materialize has not filled
    auto view = [this, open, cached]( COPY& aCopy ) -> ws::WorkspaceError
    {
        return IsCopy( boundRoot() ) ? open( boundRoot(), false, aCopy ) : cached( aCopy );
    };

    m_sourceMember = [view]( const std::string& aItemId, std::string& aRevision, ws::WorkspaceError& aError )
    {
        COPY copy;

        if( aError = view( copy ); aError.code() != ws::WE_OK )
            return std::optional<std::string>();

        return MemberFile( copy.Handle(), aItemId, aRevision, aError );
    };

    auto snapshot = []( const COPY& aCopy, const std::string& aCommit, const std::string& aLabel, mdw::Snapshot& aOut )
    {
        aOut.set_token( aCommit );
        aOut.set_label( aLabel.empty() ? aCopy.branch + " " + shortId( aCommit ) : aLabel );
    };

    auto fetch = [this]( COPY& aCopy ) -> ws::WorkspaceError
    {
        if( aCopy.cloned )
            return MdwOk();

        const std::optional<std::string> before = aCopy.Upstream();
        MDW_GIT_RELEASES                 releases( aCopy.Handle(), m_remote, REMOTE, "rel" );
        MDW_GIT_RELEASES                 states( aCopy.Handle(), m_remote, REMOTE, STATE_KIND );

        // Tags travel only as release mirrors, since libgit2 forces and prunes any refs/tags mapping, pins included
        const std::vector<wxString> refspecs = { wxString( "+refs/heads/*:refs/remotes/" ) + REMOTE + "/*",
                                                 wxString::FromUTF8( releases.MirrorRefspec() ),
                                                 wxString::FromUTF8( states.MirrorRefspec() ) };

        ws::WorkspaceError result = m_remote.Transfer( aCopy.Handle(), REMOTE,
                                                       [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
                                                       {
                                                           return aTransport.FetchRefspecs( refspecs, true );
                                                       } );

        if( result.code() == ws::WE_OK )
        {
            releases.Adopt();
            states.Adopt();
        }

        // Revisions are keyed by commit, so this only bounds the cache to what the new tip still asks for
        if( before != aCopy.Upstream() )
            m_revisions.clear();

        return result;
    };

    m_releaseMember = [this, open, fetch, snapshotOf = snapshot]( const MDW_LIFECYCLE_STORE&         aStore,
                                                           const mdw::ReleaseRevisionRequest& aRequest,
                                                           mdw::ReleaseRevisionReply& aReply ) -> ws::WorkspaceError
    {
        const std::string& id = aRequest.item_id();
        const std::string& digest = aRequest.content_digest();
        const std::string& memberDigest = aRequest.member_digest();
        std::string        library;
        std::string        name;

        if( !splitMemberId( id, library, name ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, id + " is not a library member" );

        if( digest.empty() )
            return MdwFail( ws::WE_INVALID_PARAMETER, "A release needs the content's digest" );

        // A release commits on the copy's branch, so there must be a copy
        if( !IsCopy( boundRoot() ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "The library has no copy to release from" );

        COPY copy;

        if( ws::WorkspaceError error = open( boundRoot(), false, copy ); error.code() != ws::WE_OK )
            return error;

        if( ws::WorkspaceError error = copy.LockForWrite(); error.code() != ws::WE_OK )
            return error;

        if( copy.Pinned() )
            return MdwFail( ws::WE_INVALID_PARAMETER, "A pinned copy cannot release" );

        std::optional<std::string> head = copy.Head();

        if( !head || !isCommit( copy.Handle(), *head ) )
            return gitFailure( "cannot read the copy's HEAD" );

        std::string file = memberContent( copy.Handle(), *head, id ).File( id );

        // A symbol not checked in yet is in whichever file of its folder holds it here
        if( file.empty() )
            file = contentOnDisk( copy.Handle(), copy.root, false, library ).File( id );

        if( file.empty() )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "No file in the copy holds " + id );

        std::optional<std::string> text = MDW_ReadFile( MdwFsPath( aRequest.local_path() ) );

        if( text )
            text = GetGitBackend()->FilterText( copy.Handle(), file, *text, true );

        if( !text )
            return MdwFail( ws::WE_INVALID_PARAMETER, "Cannot read " + aRequest.local_path() );

        std::optional<std::string> headText = fileAt( copy.Handle(), *head, file );
        CONTENT                    onDisk;
        CONTENT                    atHead;

        onDisk.AddFile( file, *text );

        if( headText )
            atHead.AddFile( file, *headText );

        if( ws::WorkspaceError error = unreadable( { &onDisk, &atHead } ); error.code() != ws::WE_OK )
            return error;

        const std::optional<std::string> released = onDisk.Member( id );

        if( !released )
            return MdwFail( ws::WE_INVALID_PARAMETER, aRequest.local_path() + " does not hold " + name );

        // Released alone, a derived symbol would lose the parent it is drawn from
        if( onDisk.derived.count( id ) )
            return MdwFail( ws::WE_NOT_SUPPORTED,
                            id + " extends another symbol, and derived symbols cannot be released" );

        MDW_GIT_RELEASES  releases( copy.Handle(), m_remote, REMOTE, "rel" );
        const std::string scope = releaseScope( id );

        // Fetched first, so a release that cannot reach the team commits nothing, with the branch to tell a moved
        // upstream from a teammate's edit of this member
        if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
            return error;

        const std::optional<std::string> upstream = copy.Upstream();

        auto shared = [&]( const std::string& aCommit )
        {
            return upstream
                   && ( *upstream == aCommit
                        || GetGitBackend()->IsDescendant( copy.Handle(), *upstream, aCommit ).value_or( false ) );
        };

        auto authorOf = [&]( const std::string& aCommit )
        {
            const std::optional<KIGIT_COMMIT_RECORD> commit = GetGitBackend()->ReadCommit( copy.Handle(), aCommit );

            return commit ? commit->authorName : std::string( "a teammate" );
        };

        const std::vector<MDW_GIT_RELEASE> known = releases.List( scope, partRelease( id ) );
        const MDW_GIT_RELEASE*             newest = newestSettled( known );
        std::optional<MDW_GIT_RELEASE>     release;

        for( const MDW_GIT_RELEASE& candidate : known )
        {
            if( candidate.tampered )
                continue;

            // A tag that recorded its whole member is matched on that, so a metadata edit is a release of its own
            auto              found = candidate.manifest.find( "member_digest" );
            const std::string recorded = found != candidate.manifest.end() && found->is_string()
                                                 ? found->get<std::string>()
                                                 : std::string();
            const bool        same = !recorded.empty() && !memberDigest.empty()
                                             ? recorded == memberDigest
                                             : candidate.manifest.at( "content_digest" ) == digest;

            // An earlier attempt's tag is finished rather than numbered again, unless its commit never left the copy
            if( same && ( candidate.published || shared( candidate.commit ) ) )
            {
                release = candidate;
            }
        }

        const GIT_IDENTITY author = identity( copy.Handle() );
        const std::string  user = author.SignedName();
        const std::string  email = author.SignedEmail();
        std::string memberRevision = release ? release->commit : std::string();

        std::string tip = *head;

        if( !release )
        {
            // The edit began from the release the client lists, so one made since would be published over unseen
            if( aRequest.base_revision_id() != ( newest ? newest->commit : std::string() ) )
            {
                return MdwFail( ws::WE_VERSION_CONFLICT,
                                newest ? "Rev " + std::to_string( newest->number ) + " of " + name + " was released by "
                                                 + newest->releasedBy + " since this edit began"
                                       : name + " has none of the releases this edit began from" );
            }

            if( std::optional<ws::WorkspaceError> refused = upstreamChange( copy, *head, id, m_revisions ) )
                return *refused;

            std::map<std::string, std::string> revisions = memberRevisions( copy.Handle(), *head, { id }, m_revisions );
            auto                               current = revisions.find( id );
            const std::string                  base = current == revisions.end() ? std::string() : current->second;

            // Checked before anything is committed or pushed, and again for each number tried below
            if( std::optional<ws::WorkspaceError> refused = MdwRequireNextLabel(
                        aStore, "release", aRequest.requested_label(), std::to_string( releases.Next( scope ) ) ) )
            {
                return *refused;
            }

            if( atHead.Member( id ) != released )
            {
                std::string since = newest ? newest->commit : std::string();

                // A member never released is judged from before its last check-in
                if( !newest && !base.empty() )
                {
                    const std::optional<KIGIT_COMMIT_RECORD> last = GetGitBackend()->ReadCommit( copy.Handle(), base );

                    if( last && !last->parents.empty() )
                        since = last->parents.front();
                }

                const std::optional<std::string> before =
                        since.empty() ? std::nullopt : memberContent( copy.Handle(), since, id ).Member( id );

                // An update's conflict answer already chose this edit over the member as the copy holds it
                const bool kept =
                        !base.empty() && GetGitBackend()->ReadReference( copy.Handle(), keptRef( id ) ) == base;

                // A check-in since the release is the copy's content, and an edit without it began before it
                if( before && atHead.Member( id ) && !kept
                    && !holdsChanges( *before, *released, *atHead.Member( id ) ) )
                {
                    const std::string who = authorOf( base.empty() ? *head : base );

                    return MdwFail( ws::WE_VERSION_CONFLICT,
                                    name + " was checked in by " + who + " since this edit began" );
                }

                std::map<std::string, std::optional<std::string>> files;

                if( isFootprintLibrary( library ) )
                {
                    files[file] = released;
                }
                else
                {
                    if( std::optional<ws::WorkspaceError> mixed =
                                headText ? formatMismatch( file, onDisk.headers[file], *headText,
                                                           "Check in the changes to " + file + " first.",
                                                           GET_UPDATES_FIRST )
                                         : std::nullopt )
                    {
                        return *mixed;
                    }

                    files[file] = withSymbol( headText ? *headText : onDisk.headers[file] + ")\n", name, released );
                }

                stageLineEnds( copy.Handle(), *head, files );

                if( GetGitBackend()->CommitFiles( copy.Handle(), copy.branch, *head, files, "Release " + id, user,
                                                  email, tip )
                    != 0 )
                {
                    return gitFailure( "cannot commit the release" );
                }

                memberRevision = tip;
            }
            else
            {
                memberRevision = base;
            }

            if( memberRevision.empty() )
                return gitFailure( "cannot find the commit holding " + id );

            // The tag must name a commit the team can reach
            if( !shared( memberRevision ) )
            {
                ws::WorkspaceError  pushError;
                const RefPushResult pushed =
                        m_remote.PushBranch( copy.Handle(), REMOTE, copy.branch, upstream, pushError );

                // Someone pushed since the fetch, and the race gets the answer a later start would have
                if( pushed == RefPushResult::LeaseBroken && fetch( copy ).code() == ws::WE_OK )
                {
                    if( std::optional<ws::WorkspaceError> refused = upstreamChange( copy, *head, id, m_revisions ) )
                        return *refused;
                }

                if( pushed != RefPushResult::Success )
                    return pushError;

                GetGitBackend()->SetReference( copy.Handle(), copy.UpstreamRef(), tip, std::nullopt,
                                               "release library member" );
            }
        }

        ws::WorkspaceError published = releases.PublishNext(
                release,
                [&]( MDW_GIT_RELEASE& aOut ) -> ws::WorkspaceError
                {
                    const int next = releases.Next( scope );

                    if( std::optional<ws::WorkspaceError> refused = MdwRequireNextLabel(
                                aStore, "release", aRequest.requested_label(), std::to_string( next ) ) )
                    {
                        return *refused;
                    }

                    nlohmann::json manifest = { { "item_id", id },
                                                { "member", name },
                                                { "revision", next },
                                                { "content_digest", digest },
                                                { "dependency_digests", nlohmann::json::array() },
                                                { "revision_id", memberRevision },
                                                { "released_by", user },
                                                { "date", static_cast<std::int64_t>( std::time( nullptr ) ) },
                                                { "comment", aRequest.comment() } };

                    if( !memberDigest.empty() )
                        manifest["member_digest"] = memberDigest;

                    return releases.Create( scope, next, memberRevision,
                                            "Release " + std::to_string( next ) + " of " + name, aRequest.comment(),
                                            manifest, user, email, aOut );
                } );

        if( published.code() != ws::WE_OK )
            return published;

        GetGitBackend()->DeleteReference( copy.Handle(), keptRef( id ), std::nullopt );

        aReply.set_outcome( ws::CO_APPLIED );
        *aReply.mutable_revision() = MdwGitPartRevision( *release );
        snapshotOf( copy, tip, std::string(), *aReply.mutable_snapshot() );
        aReply.set_base_snapshot( *head );
        return MdwOk();
    };

    m_memberReleases = [this, view]( const std::set<std::string>&                         aItemIds,
                                     std::map<std::string, std::vector<MDW_GIT_RELEASE>>& aOut ) -> ws::WorkspaceError
    {
        COPY copy;

        if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
            return error;

        MDW_GIT_RELEASES releases( copy.Handle(), m_remote, REMOTE, "rel" );
        MDW_GIT_RELEASES states( copy.Handle(), m_remote, REMOTE, STATE_KIND );

        // A part marked Obsolete must not pass a design check as released
        if( ws::WorkspaceError error = MDW_GIT_RELEASES::Fetch( { &releases, &states } ); error.code() != ws::WE_OK )
            return error;

        const std::optional<std::string> latest = copy.Upstream();
        std::map<std::string, CONTENT>   read;

        for( const std::string& id : aItemIds )
        {
            std::vector<MDW_GIT_RELEASE> found = releases.List( releaseScope( id ), partRelease( id ) );
            std::string                  library;
            std::string                  name;

            readStates( states, id, found );

            if( found.empty() )
            {
                if( !latest || !splitMemberId( id, library, name ) )
                    continue;

                // A .kicad_symdir is read whole for any one symbol, so it is read once for all of them
                const std::string key = isFolderLibrary( library ) && !isFootprintLibrary( library ) ? library : id;
                auto [it, fresh] = read.try_emplace( key );

                if( fresh )
                    it->second = memberContent( copy.Handle(), *latest, id );

                if( !it->second.Member( id ) )
                    continue;
            }

            aOut[id] = std::move( found );
        }

        return MdwOk();
    };

    m_memberInfo = [this, view]( const std::string& aItemId, ws::ItemInfo& aOut,
                                 std::string& aPath ) -> ws::WorkspaceError
    {
        std::string library;
        std::string name;
        COPY        copy;

        if( !splitMemberId( aItemId, library, name ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, aItemId + " is not a library member" );

        if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
            return error;

        const std::optional<std::string> head = copy.Head();
        const CONTENT                    content = head ? memberContent( copy.Handle(), *head, aItemId ) : CONTENT();
        const std::string                file = content.File( aItemId );

        if( !head || !content.Member( aItemId ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "No member " + aItemId );

        const std::string revision =
                memberRevisions( copy.Handle(), *head, { aItemId }, m_revisions, &content )[aItemId];
        MDW_GIT_RELEASES                     releases( copy.Handle(), m_remote, REMOTE, "rel" );
        const std::optional<MDW_GIT_RELEASE> release = newestRelease( releases, aItemId );
        const MDW_GIT_RELEASES               states( copy.Handle(), m_remote, REMOTE, STATE_KIND );

        aOut.set_item_id( aItemId );
        aOut.set_display_name( name );
        aOut.set_folder_id( library );
        aOut.set_path_hint( file );
        aOut.set_kind( MdwDocumentKind( file ) );
        aOut.set_revision_id( revision );
        aOut.set_revision_kind( ws::RK_CHECKIN );
        *aOut.mutable_state() = memberState( states, aItemId, release, revision );

        if( release && release->commit == revision )
        {
            aOut.set_revision_label( MdwGitPartRevision( *release ).label() );
            aOut.set_revision_kind( ws::RK_RELEASE );
        }

        if( !IsCopy( boundRoot() ) )
        {
            aPath.clear();
            return MdwOk();
        }

        std::string local = file;

        // A symbol the copy moved to another file of its folder is edited where the copy holds it
        if( isSymbolFolder( library ) )
        {
            const CONTENT disk = contentOnDisk( copy.Handle(), copy.root, false, library );

            if( disk.files.count( aItemId ) )
                local = disk.File( aItemId );
        }

        aPath = MdwUtf8Path( MdwLinkFreePath( copy.root, local, false ) );
        return MdwOk();
    };

    m_memberHistory = [this, view]( const std::string& aItemId, std::vector<ws::Revision>& aOut ) -> ws::WorkspaceError
    {
        COPY copy;

        if( !IsMemberId( aItemId ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, aItemId + " is not a library member" );

        if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
            return error;

        MDW_GIT_RELEASES             releases( copy.Handle(), m_remote, REMOTE, "rel" );
        std::vector<MDW_GIT_RELEASE> found = releases.List( releaseScope( aItemId ), partRelease( aItemId ) );

        readStates( MDW_GIT_RELEASES( copy.Handle(), m_remote, REMOTE, STATE_KIND ), aItemId, found );

        for( auto it = found.rbegin(); it != found.rend(); ++it )
        {
            if( settled( *it ) )
                aOut.push_back( MdwGitPartRevision( *it ) );
        }

        return MdwOk();
    };

    m_memberTransitions = [this, view]( const mdw::GetAvailableTransitionsRequest& aRequest,
                                        mdw::GetAvailableTransitionsReply&         aReply ) -> ws::WorkspaceError
    {
        const std::string& id = aRequest.item_id();
        COPY               copy;

        if( !IsMemberId( id ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, id + " is not a library member" );

        if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
            return error;

        const std::optional<MDW_GIT_RELEASE> release =
                newestRelease( MDW_GIT_RELEASES( copy.Handle(), m_remote, REMOTE, "rel" ), id );

        // Only the newest release moves, and a member never released has no state to leave
        if( !release || ( !aRequest.revision_id().empty() && aRequest.revision_id() != release->commit ) )
            return MdwOk();

        const std::string now =
                releaseState( MDW_GIT_RELEASES( copy.Handle(), m_remote, REMOTE, STATE_KIND ), id, *release )
                        .state_id();

        for( const ws::CustomState& state : MdwGitStates() )
        {
            if( state.state_id() == now )
                continue;

            ws::LifecycleTransition* transition = aReply.add_transitions();
            transition->set_target_state_id( state.state_id() );
            transition->set_display_name( "Mark " + state.display_name() );
            transition->set_description( state.description() );
            transition->set_target_category( state.category() );
        }

        return MdwOk();
    };

    m_transitionMember = [this, open, fetch]( const mdw::TransitionLifecycleRequest& aRequest,
                                              mdw::TransitionLifecycleReply&         aReply ) -> ws::WorkspaceError
    {
        const std::string&                      id = aRequest.item_id();
        const std::optional<ws::LifecycleState> target = MdwGitState( aRequest.target_state_id() );
        std::string                             library;
        std::string                             name;

        if( !splitMemberId( id, library, name ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, id + " is not a library member" );

        if( !target )
            return MdwFail( ws::WE_INVALID_STATE_TRANSITION, "Git has no state " + aRequest.target_state_id() );

        // The state tag is pushed from the copy, so there must be one
        if( !IsCopy( boundRoot() ) )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "The library has no copy to change states from" );

        COPY copy;

        if( ws::WorkspaceError error = open( boundRoot(), false, copy ); error.code() != ws::WE_OK )
            return error;

        if( ws::WorkspaceError error = copy.LockForWrite(); error.code() != ws::WE_OK )
            return error;

        // Fetched first, so the move starts from the release and state the team holds now
        if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
            return error;

        const std::optional<MDW_GIT_RELEASE> release =
                newestRelease( MDW_GIT_RELEASES( copy.Handle(), m_remote, REMOTE, "rel" ), id );

        if( !release )
            return MdwFail( ws::WE_INVALID_STATE_TRANSITION, name + " has no release to change the state of" );

        if( !aRequest.revision_id().empty() && aRequest.revision_id() != release->commit )
            return MdwFail( ws::WE_VERSION_CONFLICT, "Only the newest release of " + name + " changes state" );

        MDW_GIT_RELEASES               states( copy.Handle(), m_remote, REMOTE, STATE_KIND );
        const std::string              scope = stateScope( id, release->number );
        std::optional<MDW_GIT_RELEASE> move;

        // A retry finishes the tag its first attempt made rather than moving the release again
        for( const MDW_GIT_RELEASE& known : states.List( scope, stateMove( id, release->number ) ) )
        {
            if( !known.tampered && !aRequest.idempotency_id().empty()
                && known.manifest.value( "operation", std::string() ) == aRequest.idempotency_id() )
            {
                move = known;
            }
        }

        // A move to the state it is in would only add a tag
        if( !move && releaseState( states, id, *release ).state_id() == aRequest.target_state_id() )
        {
            return MdwFail( ws::WE_INVALID_STATE_TRANSITION,
                            "Rev " + std::to_string( release->number ) + " of " + name + " is already "
                                    + aRequest.target_state_id() );
        }

        const GIT_IDENTITY author = identity( copy.Handle() );
        const std::string  user = author.SignedName();
        const std::string  email = author.SignedEmail();

        ws::WorkspaceError published = states.PublishNext(
                move,
                [&]( MDW_GIT_RELEASE& aOut ) -> ws::WorkspaceError
                {
                    const int            next = states.Next( scope );
                    const nlohmann::json manifest = { { "item_id", id },
                                                      { "member", name },
                                                      { "revision", release->number },
                                                      { "sequence", next },
                                                      { "state", aRequest.target_state_id() },
                                                      { "revision_id", release->commit },
                                                      { "by", user },
                                                      { "date", static_cast<std::int64_t>( std::time( nullptr ) ) },
                                                      { "comment", aRequest.comment() },
                                                      { "operation", aRequest.idempotency_id() } };

                    const std::string subject = "Rev " + std::to_string( release->number ) + " of " + name + " is "
                                                + aRequest.target_state_id();

                    return states.Create( scope, next, release->commit, subject, aRequest.comment(), manifest, user,
                                          email, aOut );
                } );

        if( published.code() != ws::WE_OK )
            return published;

        aReply.set_outcome( ws::CO_APPLIED );
        *aReply.mutable_state() = *target;
        return MdwOk();
    };

    auto latest = [&]( const COPY& aCopy, std::string& aOut ) -> ws::WorkspaceError
    {
        std::optional<std::string> upstream = aCopy.Upstream();

        if( !upstream )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "The library repository has no branch " + aCopy.branch );

        aOut = *upstream;
        return MdwOk();
    };

    // The listing rows of aIds as aContent, commit aAt's content, holds them
    auto memberRows = [this]( const COPY& aCopy, const std::string& aAt, const CONTENT& aContent,
                              const std::set<std::string>&                            aIds,
                              google::protobuf::RepeatedPtrField<mdw::LibraryMember>& aOut ) -> ws::WorkspaceError
    {
        if( !aContent.error.empty() )
            return MdwFail( ws::WE_INTERNAL, aContent.error );

        std::map<std::string, std::string> revisions =
                memberRevisions( aCopy.Handle(), aAt, aIds, m_revisions, &aContent );

        if( MdwCurrentRequestCancelled() )
            return MdwCancelled();

        MDW_GIT_RELEASES releases( aCopy.Handle(), m_remote, REMOTE, "rel" );
        MDW_GIT_RELEASES states( aCopy.Handle(), m_remote, REMOTE, STATE_KIND );

        for( const std::string& id : aIds )
        {
            std::string library;
            std::string name;

            splitMemberId( id, library, name );

            mdw::LibraryMember* member = aOut.Add();
            member->set_member_id( id );
            member->set_library_id( library );
            member->set_name( name );
            member->set_kind( isFootprintLibrary( library ) ? ws::MK_FOOTPRINT : ws::MK_SYMBOL );
            member->set_path_hint( aContent.File( id ) );

            const std::optional<MDW_GIT_RELEASE> release = newestRelease( releases, id );

            if( release )
            {
                const ws::Revision revision = MdwGitPartRevision( *release );

                member->set_revision_id( revision.revision_id() );
                member->set_revision_label( revision.label() );
                member->set_release_digest( revision.content_digest() );
            }

            addMemberReleases( releases, states, id, *member->mutable_releases() );

            if( auto it = revisions.find( id ); it != revisions.end() )
            {
                member->set_version( it->second );
                *member->mutable_state() = memberState( states, id, release, it->second );
            }
        }

        return MdwOk();
    };

    m_server.RegisterHandler<mdw::ListSharedLibrariesRequest, mdw::ListSharedLibrariesReply>(
            [this, view, fetch, snapshot, latest]( const mdw::ListSharedLibrariesRequest&,
                                                   mdw::ListSharedLibrariesReply& aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                COPY                        copy;
                std::string                 head;

                if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = latest( copy, head ); error.code() != ws::WE_OK )
                    return error;

                addLibraries( contentAt( copy.Handle(), head, std::string(), true ), *aReply.mutable_libraries() );
                snapshot( copy, head, std::string(), *aReply.mutable_latest() );
                return MdwOk();
            } );

    m_server.RegisterHandler<mdw::MaterializeSharedLibrariesRequest, mdw::MaterializeSharedLibrariesReply>(
            [this, open, fetch, snapshot, latest, memberRows]( const mdw::MaterializeSharedLibrariesRequest& aRequest,
                                                               mdw::MaterializeSharedLibrariesReply&         aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                COPY                        copy;
                std::string                 target;

                if( ws::WorkspaceError error = open( aRequest.local_root(), true, copy, aRequest.pin() );
                    error.code() != ws::WE_OK )
                {
                    return error;
                }

                if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
                    return error;

                // Reading the libraries is the slow part left, and has no count to show
                MdwReportProgress( 0, 0, "index" );

                // Taken after the fetch, which only moves remote-tracking refs, so a slow server holds no one up
                if( ws::WorkspaceError error = copy.LockForWrite(); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = latest( copy, target ); error.code() != ws::WE_OK )
                    return error;

                if( !aRequest.pin().empty() )
                    target = aRequest.pin();

                if( !isCommit( copy.Handle(), target ) )
                    return MdwFail( ws::WE_REVISION_NOT_FOUND, aRequest.pin() + " is not a snapshot of this source" );

                std::optional<std::string> head = copy.Head();
                std::optional<std::string> upstream = copy.Upstream();

                // A check-in the server refused lives only in this branch, and moving it would orphan it
                if( !copy.Pinned() && head && upstream && head != upstream
                    && GetGitBackend()->IsDescendant( copy.Handle(), *upstream, *head ) != std::optional<bool>( true ) )
                {
                    return MdwFail( ws::WE_VERSION_CONFLICT,
                                    "The copy holds check-ins the server does not have yet; get changes first" );
                }

                const CONTENT content = contentAt( copy.Handle(), target );

                // Listed before the checkout, so a failure here leaves an existing copy as it was
                if( ws::WorkspaceError error =
                            memberRows( copy, target, content, memberIds( content ), *aReply.mutable_members() );
                    error.code() != ws::WE_OK )
                {
                    return error;
                }

                // A fresh clone reached the target in staging, unless another process moved it after it was published
                const bool reached = copy.cloned && head == target && copy.Pinned() == !aRequest.pin().empty();

                if( !reached )
                {
                    if( ws::WorkspaceError error =
                                checkout( copy.Handle(), target, aRequest.pin().empty() ? copy.branch : std::string() );
                        error.code() != ws::WE_OK )
                    {
                        return error;
                    }
                }

                snapshot( copy, target, aRequest.pin().empty() ? std::string() : shortId( aRequest.pin() ),
                          *aReply.mutable_snapshot() );
                addLibraries( contentOnDisk( copy.Handle(), copy.root, true ), *aReply.mutable_libraries() );
                return MdwOk();
            } );

    m_server.RegisterHandler<mdw::ListMembersRequest, mdw::ListMembersReply>(
            [this, view, fetch, latest, memberRows]( const mdw::ListMembersRequest& aRequest,
                                                     mdw::ListMembersReply&         aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                COPY                        copy;
                std::string                 at;

                if( ws::WorkspaceError error = view( copy ); error.code() != ws::WE_OK )
                    return error;

                if( aRequest.snapshot().empty() )
                {
                    if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
                        return error;

                    if( ws::WorkspaceError error = latest( copy, at ); error.code() != ws::WE_OK )
                        return error;
                }
                else
                {
                    at = aRequest.snapshot();
                }

                if( !isCommit( copy.Handle(), at ) )
                    return MdwFail( ws::WE_REVISION_NOT_FOUND, aRequest.snapshot() + " is not a snapshot" );

                const CONTENT content = libraryContent( copy.Handle(), at, aRequest.library_id() );

                if( !content.libraries.count( aRequest.library_id() ) )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "No library " + aRequest.library_id() );

                const std::set<std::string> ids = memberIds( content, aRequest.library_id() );

                size_t      offset = 0;
                size_t      end = 0;
                std::string next;

                if( ws::WorkspaceError error = MdwPageRange( aRequest.page(), ids.size(), at, offset, end, next );
                    error.code() != ws::WE_OK )
                {
                    return error;
                }

                std::vector<std::string> rows( ids.begin(), ids.end() );
                std::set<std::string>    wanted( rows.begin() + offset, rows.begin() + end );

                aReply.set_next_page_token( next );
                return memberRows( copy, at, content, wanted, *aReply.mutable_members() );
            } );

    m_server.RegisterHandler<mdw::UpdateSharedLibrariesRequest, mdw::UpdateSharedLibrariesReply>(
            [this, open, fetch, snapshot, latest, memberRows]( const mdw::UpdateSharedLibrariesRequest& aRequest,
                                                               mdw::UpdateSharedLibrariesReply&         aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                COPY                        copy;
                std::string                 theirsId;

                if( ws::WorkspaceError error = open( aRequest.local_root(), true, copy ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
                    return error;

                // Reading the libraries is the slow part left, and has no count to show
                MdwReportProgress( 0, 0, "index" );

                if( ws::WorkspaceError error = copy.LockForWrite(); error.code() != ws::WE_OK )
                    return error;

                if( copy.Pinned() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "A pinned copy only moves to another pin" );

                std::optional<std::string> head = copy.Head();

                if( !head || !isCommit( copy.Handle(), *head ) )
                    return gitFailure( "cannot read the copy's HEAD" );

                if( ws::WorkspaceError error = stagedChanges( copy.Handle(), *head ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = latest( copy, theirsId ); error.code() != ws::WE_OK )
                    return error;

                const bool behind = *head != theirsId
                                    && GetGitBackend()->IsDescendant( copy.Handle(), theirsId, *head )
                                               == std::optional<bool>( true );

                // Nothing here to keep, so the copy moves to the tip by checkout and never reads the merge base
                if( ( *head == theirsId || behind ) && aRequest.resolutions().empty() && unchanged( copy.Handle() ) )
                {
                    const CONTENT theirs = contentAt( copy.Handle(), theirsId );

                    if( behind )
                    {
                        const CONTENT         before = contentAt( copy.Handle(), *head );
                        std::set<std::string> upstream;

                        for( const CONTENT* content : std::initializer_list<const CONTENT*>{ &before, &theirs } )
                        {
                            for( const auto& [id, text] : content->members )
                            {
                                if( theirs.Member( id ) != before.Member( id ) )
                                    upstream.insert( id );
                            }
                        }

                        const std::map<std::string, std::string> beforeRev =
                                memberRevisions( copy.Handle(), *head, upstream, m_revisions, &before );
                        const std::map<std::string, std::string> theirsRev =
                                memberRevisions( copy.Handle(), theirsId, upstream, m_revisions, &theirs );

                        for( const std::string& id : upstream )
                        {
                            addChange( *aReply.mutable_changes(), id,
                                       theirs.files.count( id ) ? theirs.File( id ) : before.File( id ), before, theirs,
                                       beforeRev, theirsRev );
                        }
                    }

                    if( ws::WorkspaceError error =
                                memberRows( copy, theirsId, theirs, memberIds( theirs ), *aReply.mutable_members() );
                        error.code() != ws::WE_OK )
                    {
                        return error;
                    }

                    // A safe checkout stops at any file it would overwrite rather than lose it
                    if( behind )
                    {
                        if( ws::WorkspaceError error = checkout( copy.Handle(), theirsId, copy.branch );
                            error.code() != ws::WE_OK )
                        {
                            return error;
                        }
                    }

                    snapshot( copy, theirsId, std::string(), *aReply.mutable_snapshot() );
                    addLibraries( contentOnDisk( copy.Handle(), copy.root, true ), *aReply.mutable_libraries() );
                    return MdwOk();
                }

                const std::optional<std::string> baseId = GetGitBackend()->MergeBase( copy.Handle(), *head, theirsId );

                if( !baseId )
                    return gitFailure( "the copy shares no history with the library repository" );

                const CONTENT base = contentAt( copy.Handle(), *baseId );
                const CONTENT theirs = contentAt( copy.Handle(), theirsId );
                CONTENT       ours = contentOnDisk( copy.Handle(), copy.root );

                ours.Drop( base.links );

                // A library or member turning into a link upstream would read as every local edit removed
                for( const std::string& link : theirs.links )
                {
                    if( !base.links.count( link ) )
                        return MdwFail( ws::WE_VERSION_CONFLICT, link + " became a link upstream" );
                }

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                // The merge base is history, which a member held twice cannot void once upstream fixed it
                if( !base.error.empty() )
                    return MdwFail( ws::WE_INTERNAL, base.error );

                if( ws::WorkspaceError error = unreadable( { &theirs } ); error.code() != ws::WE_OK )
                    return error;

                if( !ours.error.empty() || !sameTwice( ours, base ) )
                    return unreadable( { &ours } );

                if( ws::WorkspaceError error = linkedMembers( base, ours ); error.code() != ws::WE_OK )
                    return error;

                std::set<std::string> ids;

                for( const CONTENT* content : std::initializer_list<const CONTENT*>{ &base, &theirs, &ours } )
                {
                    for( const auto& [id, text] : content->members )
                        ids.insert( id );
                }

                std::set<std::string> local;
                std::set<std::string> upstream;
                std::set<std::string> both;

                for( const std::string& id : ids )
                {
                    if( theirs.Member( id ) != base.Member( id ) )
                        upstream.insert( id );
                }

                std::map<std::string, std::string> theirsRev =
                        memberRevisions( copy.Handle(), theirsId, upstream, m_revisions, &theirs );
                std::map<std::string, std::string> resolved;

                // Each answer that replaces a member, with the member as the copy held it before
                std::map<std::string, std::optional<std::string>> merged;

                // Each answered member as its own answer holds it
                std::map<std::string, std::optional<std::string>> answers;

                // Answers are applied to ours one by one, and each is checked against the copy as it was read
                const CONTENT beforeAnswers = ours;

                // Names ours as read here, so a conflict's answer can tell whether the member was saved since
                auto oursToken = [&beforeAnswers]( const std::string& aId )
                {
                    const std::optional<std::string> member = beforeAnswers.Member( aId );

                    return member ? GetGitBackend()->BlobId( *member ) : std::string();
                };

                // A member gone here or upstream is still placed by the side that holds it
                auto fileOf = [&]( const std::string& aId )
                {
                    for( const CONTENT* content :
                         std::initializer_list<const CONTENT*>{ &beforeAnswers, &theirs, &base } )
                    {
                        if( content->files.count( aId ) )
                            return content->File( aId );
                    }

                    return ours.File( aId );
                };

                std::set<std::string> stale;
                std::set<std::string> settled;

                for( const mdw::ConflictResolution& resolution : aRequest.resolutions() )
                {
                    if( resolution.has_merged() )
                        settled.insert( resolution.item_id() );
                }

                for( const mdw::ConflictResolution& resolution : aRequest.resolutions() )
                {
                    const std::string& id = resolution.item_id();

                    resolved[id] = resolution.theirs_revision_id();

                    if( !resolution.has_merged() )
                        continue;

                    // A merge made against another ours or theirs is never written
                    if( !upstream.count( id ) || revisionOf( theirsRev, theirs, id ) != resolution.theirs_revision_id()
                        || oursToken( id ) != resolution.ours_token() )
                    {
                        resolved.erase( id );
                        stale.insert( id );
                        continue;
                    }

                    const bool whole = !resolution.whole_file_of().empty();

                    // A whole file answers for every part in it, so one saved since it was made is stale too
                    if( whole )
                    {
                        std::optional<std::string> bytes = MDW_ReadFile( copy.root / MdwFsPath( fileOf( id ) ) );

                        if( !bytes || KIGIT::BlobDigest( *bytes ) != resolution.whole_file_of() )
                        {
                            resolved.erase( id );
                            stale.insert( id );
                            continue;
                        }
                    }

                    merged[id] = beforeAnswers.Member( id );

                    if( std::string refused =
                                ours.Resolve( id, resolution.merged(), fileOf( id ), whole, settled, answers[id] );
                        !refused.empty() )
                    {
                        return MdwFail( ws::WE_INVALID_PARAMETER, refused );
                    }

                    // Even a merge back to the base is the user's answer, so it is kept as a local edit
                    local.insert( id );
                }

                // A whole file taken after another answer in it replaces that answer, unless it holds it too
                for( const auto& [id, answer] : answers )
                {
                    if( ours.Member( id ) != answer )
                    {
                        return MdwFail( ws::WE_INVALID_PARAMETER,
                                        "the answers in " + fileOf( id ) + " disagree on " + id );
                    }
                }

                for( const std::string& id : ids )
                {
                    const std::optional<std::string> b = base.Member( id );
                    const std::optional<std::string> t = theirs.Member( id );
                    const std::optional<std::string> o = ours.Member( id );

                    const bool ourEdit = !sameContent( o, b );

                    // A part only saved back here takes upstream's edit, and keeps its own bytes when there is none
                    if( ourEdit || ( o != b && sameContent( t, b ) ) )
                        local.insert( id );

                    if( ourEdit && !sameContent( t, b ) && !sameContent( o, t ) )
                        both.insert( id );
                }

                std::set<std::string> revised = upstream;
                revised.insert( local.begin(), local.end() );

                std::map<std::string, std::string> baseRev =
                        memberRevisions( copy.Handle(), *baseId, revised, m_revisions, &base );

                auto addConflict = [&]( const std::string& aId )
                {
                    std::string library;
                    std::string name;

                    splitMemberId( aId, library, name );

                    mdw::UpdateConflict* conflict = aReply.add_conflicts();
                    conflict->mutable_item()->set_item_id( aId );
                    conflict->mutable_item()->set_path_hint( fileOf( aId ) );
                    conflict->set_kind( memberKind( aId ) );
                    conflict->set_base_revision_id( revisionOf( baseRev, base, aId ) );
                    conflict->set_theirs_revision_id( revisionOf( theirsRev, theirs, aId ) );
                    conflict->set_relative_path( fileOf( aId ) );
                    conflict->set_name( name );
                    conflict->set_ours_token( oursToken( aId ) );
                };

                for( const std::string& id : both )
                {
                    auto answer = resolved.find( id );

                    if( answer != resolved.end() && answer->second == revisionOf( theirsRev, theirs, id ) )
                        continue;

                    addConflict( id );
                    stale.erase( id );
                }

                // A stale answer not asked for again would otherwise be lost unseen
                if( !stale.empty() )
                {
                    return MdwFail( ws::WE_VERSION_CONFLICT,
                                    *stale.begin() + " changed while its conflict was resolved; get the latest again" );
                }

                snapshot( copy, theirsId, std::string(), *aReply.mutable_snapshot() );

                if( aReply.conflicts_size() > 0 )
                    return MdwOk();

                // Listed before any write, so a cancel here leaves the copy as it was
                if( ws::WorkspaceError error =
                            memberRows( copy, theirsId, theirs, memberIds( theirs ), *aReply.mutable_members() );
                    error.code() != ws::WE_OK )
                {
                    return error;
                }

                // Every file upstream changed is rewritten as upstream has it, with this copy's own members kept
                std::optional<std::vector<std::string>> changed =
                        GetGitBackend()->ChangedPaths( copy.Handle(), *baseId, theirsId );

                if( !changed )
                    return gitFailure( "cannot compare the copy with the library repository" );

                std::set<std::string> files( changed->begin(), changed->end() );

                // Either side may have moved a symbol to another file of its folder, so it is placed in one file and
                // taken out of every other
                struct PLACE
                {
                    std::optional<std::string> text;
                    std::string                file;

                    bool operator==( const PLACE& aOther ) const = default;
                };

                auto placeIn = []( const CONTENT& aContent, const std::string& aId )
                {
                    const std::optional<std::string> text = aContent.Member( aId );

                    return PLACE{ text, text ? aContent.File( aId ) : std::string() };
                };

                std::map<std::string, PLACE>                 placed;
                std::map<std::string, std::set<std::string>> symbolsIn;

                for( const std::string& id : ids )
                {
                    const PLACE b = placeIn( base, id );
                    const PLACE o = placeIn( ours, id );
                    const PLACE t = placeIn( theirs, id );

                    // A conflict answer is ours even when it matches the base, a deletion included
                    const bool answered = merged.count( id ) > 0;

                    // Text only saved back here is kept unless upstream has a real edit to take
                    const bool ourText = answered || !sameContent( o.text, b.text )
                                         || ( o.text != b.text && sameContent( t.text, b.text ) );
                    const bool ourChange = ourText || o.file != b.file;
                    PLACE      result = ourChange ? o : t;

                    // Both sides changed it, so the text is ours as the conflict answer kept it, and a move either
                    // made carries over
                    if( ourChange && t != b )
                    {
                        result.text = ourText ? o.text : t.text;
                        result.file = !result.text                                          ? std::string()
                                      : ( o.text && b.text && o.file != b.file ) || !t.text ? o.file
                                                                                            : t.file;
                    }

                    std::set<std::string> held;

                    for( const PLACE* place : std::initializer_list<const PLACE*>{ &b, &o, &t, &result } )
                    {
                        if( place->file.ends_with( SYMBOL_EXT ) )
                            held.insert( place->file );
                    }

                    // A second copy that came with the copy is taken out of its file too
                    for( const CONTENT* side : std::initializer_list<const CONTENT*>{ &base, &ours, &theirs } )
                    {
                        for( const std::string& file : side->Copies( id ) )
                        {
                            if( file.ends_with( SYMBOL_EXT ) )
                                held.insert( file );
                        }
                    }

                    if( held.empty() )
                        continue;

                    placed[id] = result;

                    for( const std::string& file : held )
                    {
                        symbolsIn[file].insert( id );

                        // Every file the symbol leaves or enters is rewritten, not only those upstream changed. An
                        // answer is only in memory, so its file is rewritten too
                        if( result != o || answered )
                            files.insert( file );
                    }
                }

                for( const std::string& path : files )
                {
                    // The remote picks these names, and only a checkout would have refused them
                    if( !MdwIsSafeRepoPath( path ) )
                        return MdwFail( ws::WE_VERSION_CONFLICT, path + " is not a safe path" );
                }

                // Upstream's members that would land under a header of another format, and why one of ours cannot
                std::set<std::string>             reformat;
                std::optional<ws::WorkspaceError> unplaced;

                // aText, read as aSide holds aFile, with every symbol in aFile placed as the update leaves it
                auto placeSymbols = [&]( std::string aText, const CONTENT& aSide, const std::string& aFile, bool& aAny )
                {
                    for( const std::string& id : symbolsIn[aFile] )
                    {
                        const PLACE&                     result = placed.at( id );
                        const std::optional<std::string> member =
                                result.file == aFile ? result.text : std::optional<std::string>();

                        aAny = aAny || member.has_value();

                        const bool inPlace = member == aSide.Member( id, aFile );

                        // An answer reads by its own header, which the same bytes under another one do not share
                        if( inPlace && !( member && merged.count( id ) ) )
                            continue;

                        // An answer was checked against the copy's file, which is not always the frame kept
                        if( member )
                        {
                            const bool     mine = member == ours.Member( id );
                            const CONTENT& from = mine ? ours : theirs;
                            auto           header = from.headers.find( from.File( id ) );

                            if( header != from.headers.end()
                                && KIGIT::FormatVersion( header->second ) != KIGIT::FormatVersion( aText ) )
                            {
                                if( !mine )
                                {
                                    reformat.insert( id );
                                }
                                else if( merged.count( id ) && !unplaced )
                                {
                                    unplaced = MdwFail( ws::WE_INVALID_PARAMETER,
                                                        "The answer for " + id + " is in another format than " + aFile
                                                                + ". Save the library, then get the updates again." );
                                }
                                else if( !unplaced )
                                {
                                    unplaced = formatMismatch( aFile, header->second, aText,
                                                               "Upgrade the team library's format first.",
                                                               "Save the library, then get the updates again." );
                                }
                            }
                        }

                        if( inPlace )
                            continue;

                        std::string library;
                        std::string name;

                        splitMemberId( id, library, name );
                        aText = withSymbol( aText, name, member );
                    }

                    return aText;
                };

                std::set<std::string> attributeFiles;

                for( const std::string& file : files )
                {
                    for( std::string dir = file; !dir.empty(); )
                    {
                        const std::size_t slash = dir.rfind( '/' );

                        dir = slash == std::string::npos ? std::string() : dir.substr( 0, slash );
                        attributeFiles.insert( dir.empty() ? ".gitattributes" : dir + "/.gitattributes" );
                    }
                }

                // Attributes kept as edited here would disagree with upstream's, which every write follows
                for( const std::string& file : attributeFiles )
                {
                    std::optional<std::string> diskText;

                    if( !presentFile( copy.Handle(), copy.root, file, diskText ) )
                        return MdwFail( ws::WE_INTERNAL, "Cannot read " + file );

                    // A clone can write an attribute file with CRLF before any attribute says otherwise
                    diskText = attributeRules( diskText );

                    if( diskText != attributeRules( fileAt( copy.Handle(), *baseId, file ) )
                        && diskText != attributeRules( fileAt( copy.Handle(), theirsId, file ) ) )
                    {
                        return MdwFail( ws::WE_VERSION_CONFLICT,
                                        file + " was edited here and differs from upstream's" );
                    }
                }

                // Every write is planned before any is made, so a refusal leaves the copy as it was
                std::map<std::string, std::optional<std::string>> writes;

                for( const std::string& file : files )
                {
                    const std::optional<std::string> theirsText = fileAt( copy.Handle(), theirsId, file );
                    std::optional<std::string>       result = theirsText;

                    if( !isLibraryFile( file ) )
                    {
                        const std::optional<std::string> baseText = fileAt( copy.Handle(), *baseId, file );
                        std::optional<std::string>       diskText;

                        // A file that cannot be read cannot be shown unchanged, so it is never replaced
                        if( !presentFile( copy.Handle(), copy.root, file, diskText ) )
                            return MdwFail( ws::WE_INTERNAL, "Cannot read " + file );

                        // Something else's file edited here stays as this copy has it
                        if( attributeFiles.count( file ) ? attributeRules( diskText ) != attributeRules( baseText )
                                                         : diskText != baseText )
                        {
                            continue;
                        }
                    }
                    else if( file.ends_with( SYMBOL_EXT ) )
                    {
                        const std::optional<std::string> baseFrame = base.Frame( file );
                        const std::optional<std::string> theirsFrame = theirs.Frame( file );
                        const std::optional<std::string> oursFrame = ours.Frame( file );

                        bool any = false;

                        // Text outside the symbols changed on both sides, so an answered conflict in the file settles
                        // it as the side the user kept. Use answers with upstream's member, Keep with ours
                        std::optional<bool> keptOurs;

                        if( oursFrame != baseFrame && theirsFrame != baseFrame && theirsFrame != oursFrame )
                        {
                            for( const std::string& id : symbolsIn[file] )
                            {
                                if( !resolved.count( id ) )
                                    continue;

                                const bool used = merged.count( id ) && ours.Member( id ) == theirs.Member( id );

                                keptOurs = keptOurs.value_or( false ) || !used;
                            }

                            // Saves from different builds can change only writer fields, so retain the newer format
                            if( !keptOurs && oursFrame && theirsFrame
                                && withoutWriter( *oursFrame ) == withoutWriter( *theirsFrame ) )
                            {
                                keptOurs = KIGIT::NewerFormat( KIGIT::FormatVersion( *oursFrame ),
                                                               KIGIT::FormatVersion( *theirsFrame ) );
                            }
                        }

                        // A file deleted here has no text of ours to keep, and upstream's new symbols stay
                        const bool deletedHere = base.Text( file ) && !ours.Text( file );

                        if( oursFrame == baseFrame || keptOurs == false || ( deletedHere && theirsText ) )
                        {
                            std::string text =
                                    theirsText ? *theirsText
                                               : ( ours.headers.count( file ) ? ours.headers.at( file ) : "" ) + ")\n";

                            text = placeSymbols( text, theirsText ? theirs : CONTENT(), file, any );
                            result = theirsText || any ? std::optional<std::string>( text ) : std::nullopt;
                        }
                        else if( theirsFrame == baseFrame || theirsFrame == oursFrame || keptOurs == true )
                        {
                            // Text outside the symbols changed only here, so this copy's file takes upstream's members
                            result = ours.Text( file );

                            if( result )
                                result = placeSymbols( *result, ours, file, any );
                        }
                        else
                        {
                            return MdwFail( ws::WE_VERSION_CONFLICT,
                                            file + " changed outside its symbols both here and upstream" );
                        }

                        // A folder file left without symbols is removed, as the library writer removes it, and so is
                        // one deleted here
                        if( result && ( deletedHere || !folderLibrary( file ).empty() )
                            && KIGIT::SplitSymbols( *result ).spans.empty() )
                        {
                            result.reset();
                        }
                    }
                    else if( const std::string id = footprintId( file ); local.count( id ) )
                    {
                        // Unlike any other local edit, a merged resolution is not on disk yet
                        if( !merged.count( id ) )
                            continue;

                        result = ours.Member( id );
                    }

                    if( MdwLinkFreePath( copy.root, file, true ).empty() )
                        return MdwFail( ws::WE_VERSION_CONFLICT, file + " lies behind a link in the copy" );

                    writes[file] = result;
                }

                if( unplaced )
                    return *unplaced;

                // Only a parser can move a member between formats, so the client settles these as conflicts
                if( !reformat.empty() )
                {
                    aReply.clear_members();

                    for( const std::string& id : reformat )
                        addConflict( id );

                    return MdwOk();
                }

                std::map<std::string, std::string> texts;

                for( const auto& [file, text] : writes )
                {
                    if( text )
                        texts[file] = *text;
                }

                // Written as a checkout would write them, so the copy keeps one convention for line endings. Upstream's
                // attributes apply, not those in the files this update replaces
                for( auto& [file, text] : GetGitBackend()->FilterTextsAt( copy.Handle(), theirsId, texts, false ) )
                {
                    if( !text )
                        return gitFailure( "cannot convert " + file );

                    writes[file] = std::move( text );
                }

                // Each file's bytes before the update, so a failure part way can put every earlier write back
                std::map<std::string, std::optional<std::string>> originals;

                for( const auto& [file, text] : writes )
                {
                    const fs::path  path = copy.root / MdwFsPath( file );
                    std::error_code ec;

                    if( fs::symlink_status( path, ec ).type() == fs::file_type::not_found )
                        originals[file] = std::nullopt;
                    else if( std::optional<std::string> bytes = MDW_ReadFile( path ) )
                        originals[file] = std::move( bytes );
                    else
                        return MdwFail( ws::WE_INTERNAL, "Cannot read " + file );
                }

                // Local commits leave the branch with the fast-forward, so a ref keeps them reachable
                std::string backup;

                if( *baseId != *head )
                {
                    backup = keepCommit( copy.Handle(), *head );

                    if( backup.empty() )
                        return gitFailure( "cannot keep the copy's own commits" );
                }

                std::vector<std::string> written;
                std::set<fs::path>       madeFolders;

                auto putBack = [&]( ws::WorkspaceError aError, bool aIndexTouched ) -> ws::WorkspaceError
                {
                    std::string lost;

                    auto note = [&lost]( const std::string& aWhat )
                    {
                        lost += ( lost.empty() ? "" : ", " ) + aWhat;
                    };

                    for( auto it = written.rbegin(); it != written.rend(); ++it )
                    {
                        const fs::path                    path = copy.root / MdwFsPath( *it );
                        const std::optional<std::string>& original = originals.at( *it );
                        const std::optional<std::string>& installed = writes.at( *it );
                        std::error_code                   ec;

                        const bool untouched =
                                installed ? MDW_ReadFile( path ) == installed
                                          : fs::symlink_status( path, ec ).type() == fs::file_type::not_found;

                        // A file saved since this update wrote it is someone's newer work, so it stays
                        if( !untouched || MdwLinkFreePath( copy.root, *it, true ).empty() )
                        {
                            note( *it );
                            continue;
                        }

                        if( original ? !writeFile( path, *original ).empty() : !fs::remove( path, ec ) && ec )
                            note( *it );
                    }

                    // Deepest first, as a set orders a folder before those inside it
                    for( auto it = madeFolders.rbegin(); it != madeFolders.rend(); ++it )
                        removeEmptyFolder( *it );

                    // The index matched HEAD before the update, so HEAD's tree is what it held
                    if( aIndexTouched && GetGitBackend()->ResetIndex( copy.Handle(), *head ) != 0 )
                        note( "the index" );

                    if( !backup.empty() )
                        GetGitBackend()->DeleteReference( copy.Handle(), backup, std::nullopt );

                    if( !lost.empty() )
                        aError.set_message( aError.message() + ", and " + lost + " could not be put back" );

                    return aError;
                };

                for( const auto& [file, text] : writes )
                {
                    const fs::path  path = copy.root / MdwFsPath( file );
                    std::error_code ec;

                    // Checked again just before the write, since a link made after planning would still be followed
                    if( MdwLinkFreePath( copy.root, file, true ).empty() )
                    {
                        return putBack( MdwFail( ws::WE_VERSION_CONFLICT, file + " lies behind a link in the copy" ),
                                        false );
                    }

                    for( fs::path folder = path.parent_path();
                         folder != copy.root && folder.has_relative_path() && !fs::exists( folder, ec );
                         folder = folder.parent_path() )
                    {
                        madeFolders.insert( folder );
                    }

                    if( text )
                    {
                        if( std::string failure = writeFile( path, *text ); !failure.empty() )
                            return putBack( MdwFail( ws::WE_INTERNAL, "Cannot write " + file + ": " + failure ),
                                            false );
                    }
                    else if( !fs::remove( path, ec ) && ec )
                    {
                        return putBack( MdwFail( ws::WE_INTERNAL, "Cannot remove " + file + ": " + ec.message() ),
                                        false );
                    }

                    written.push_back( file );
                }

                const std::string branchRef = "refs/heads/" + copy.branch;

                // Local check-ins become local edits here, since their content is already on disk
                if( GetGitBackend()->ResetIndex( copy.Handle(), theirsId ) != 0
                    || GetGitBackend()->SetReference( copy.Handle(), branchRef, theirsId, std::nullopt, "update" )
                               != 0 )
                {
                    return putBack( gitFailure( "cannot move the copy forward" ), true );
                }

                for( const std::string& id : upstream )
                {
                    if( !local.count( id ) )
                        addChange( *aReply.mutable_changes(), id, fileOf( id ), base, theirs, baseRev, theirsRev );
                }

                // An answer that took upstream's edit in changed the part here, though the copy had its own edit
                for( const auto& [id, before] : merged )
                {
                    if( !sameContent( before, ours.Member( id ) ) )
                        addChange( *aReply.mutable_changes(), id, fileOf( id ), base, theirs, baseRev, theirsRev );
                }

                std::set<std::string> answered;

                for( const mdw::ConflictResolution& resolution : aRequest.resolutions() )
                    answered.insert( resolution.item_id() );

                const std::map<std::string, std::string> standing =
                        memberRevisions( copy.Handle(), theirsId, answered, m_revisions, &theirs );

                // Answered against the member as the copy now holds it, so a release need not carry that version
                for( const mdw::ConflictResolution& resolution : aRequest.resolutions() )
                {
                    auto it = standing.find( resolution.item_id() );

                    if( it != standing.end() && !it->second.empty() && it->second == resolution.theirs_revision_id() )
                    {
                        GetGitBackend()->SetReference( copy.Handle(), keptRef( it->first ), it->second, std::nullopt,
                                                       "kept" );
                    }
                }

                addLibraries( contentOnDisk( copy.Handle(), copy.root, true ), *aReply.mutable_libraries() );
                return MdwOk();
            } );

    m_server.RegisterHandler<mdw::CheckInSharedLibraryRequest, mdw::CheckInSharedLibraryReply>(
            [this, open, fetch, snapshot]( const mdw::CheckInSharedLibraryRequest& aRequest,
                                           mdw::CheckInSharedLibraryReply&         aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                COPY                        copy;

                if( locator().empty() )
                    return noLibrarySource();

                if( !IsCopy( aRequest.local_root() ) )
                    return MdwFail( ws::WE_INVALID_PARAMETER, aRequest.local_root() + " is not a copy of this source" );

                if( ws::WorkspaceError error = open( aRequest.local_root(), false, copy ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = copy.LockForWrite(); error.code() != ws::WE_OK )
                    return error;

                if( copy.Pinned() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "A pinned copy cannot check in" );

                // Fetched first, so the release rules and a teammate's check-in are judged as the team has them
                if( ws::WorkspaceError error = fetch( copy ); error.code() != ws::WE_OK )
                    return error;

                std::optional<std::string> head = copy.Head();

                if( !head || !isCommit( copy.Handle(), *head ) )
                    return gitFailure( "cannot read the copy's HEAD" );

                const CONTENT before = contentAt( copy.Handle(), *head );
                CONTENT       now = contentOnDisk( copy.Handle(), copy.root );

                now.Drop( before.links );

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                if( ws::WorkspaceError error = unreadable( { &before, &now } ); error.code() != ws::WE_OK )
                    return error;

                if( ws::WorkspaceError error = linkedMembers( before, now ); error.code() != ws::WE_OK )
                    return error;

                std::vector<std::string> wanted( aRequest.item_ids().begin(), aRequest.item_ids().end() );
                std::vector<std::string> changed;

                if( namesFormatUpgrade( aRequest.comment() ) )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "The comment holds the format upgrade trailer" );

                if( namesReformattedMember( aRequest.comment() ) )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "The comment holds a reformatted member trailer" );

                // History takes a format upgrade's word that it revised no member, so it may neither add nor remove one
                if( aRequest.format_upgrade() )
                {
                    if( !wanted.empty() || aRequest.whole_files().empty() )
                        return MdwFail( ws::WE_INVALID_PARAMETER, "A format upgrade checks in whole files only" );

                    auto membersOf = [&]( const CONTENT& aContent, const std::string& aFile )
                    {
                        std::set<std::string> ids;

                        for( const auto& [id, file] : aContent.files )
                        {
                            if( file == aFile )
                                ids.insert( id );
                        }

                        for( const auto& [id, copies] : aContent.twice )
                        {
                            if( copies.count( aFile ) )
                                ids.insert( id );
                        }

                        return ids;
                    };

                    for( const auto& [file, digest] : aRequest.whole_files() )
                    {
                        if( !before.texts.count( file ) || !now.texts.count( file )
                            || membersOf( before, file ) != membersOf( now, file ) )
                        {
                            return MdwFail( ws::WE_INVALID_PARAMETER,
                                            file + " is not a symbol library that keeps its symbols" );
                        }

                        const std::string team = KIGIT::FormatVersion( before.texts.at( file ) );
                        const std::string ours = KIGIT::FormatVersion( now.texts.at( file ) );

                        // An older KiCad's upgrade would put back the format the team's file was raised from
                        if( !team.empty() && !ours.empty() && KIGIT::NewerFormat( team, ours ) )
                        {
                            return MdwFail( ws::WE_NOT_SUPPORTED,
                                            file + " is already in a newer file format than this KiCad writes" );
                        }
                    }
                }

                // Files named whole are all the caller asks for, so no member is added to them
                if( wanted.empty() && aRequest.whole_files().empty() )
                {
                    std::set<std::string> ids;

                    for( const CONTENT* content : std::initializer_list<const CONTENT*>{ &before, &now } )
                    {
                        for( const auto& [id, text] : content->members )
                            ids.insert( id );
                    }

                    for( const std::string& id : ids )
                    {
                        if( !sameMember( before, now, id ) )
                            wanted.push_back( id );
                    }
                }

                auto result = [&]( const std::string& aId ) -> mdw::MemberResult*
                {
                    std::string        library;
                    std::string        name;
                    mdw::MemberResult* row = aReply.add_results();

                    splitMemberId( aId, library, name );
                    row->set_library_id( library );
                    row->set_name( name );
                    row->set_item_id( aId );
                    return row;
                };

                const MDW_GIT_RELEASES releases( copy.Handle(), m_remote, REMOTE, "rel" );

                // Each file's new text is HEAD's with only the listed members replaced
                std::map<std::string, std::optional<std::string>> files;

                // Each member's files, and the header each spliced member was read under, by the file it went into
                std::map<std::string, std::vector<std::string>> touchedBy;
                std::map<std::string, std::string>              splicedFrom;

                for( const std::string& id : wanted )
                {
                    std::string library;
                    std::string name;

                    if( !splitMemberId( id, library, name ) || sameMember( before, now, id ) )
                    {
                        *result( id )->mutable_error() = MdwFail( ws::WE_ITEM_NOT_FOUND, id + " has no local change" );
                        continue;
                    }

                    if( std::string why = checkInDenial( releases, before, now, id, m_holdsClaim ); !why.empty() )
                    {
                        *result( id )->mutable_error() = MdwFail( ws::WE_PERMISSION_DENIED, why );
                        continue;
                    }

                    const std::string        is = now.files.count( id ) ? now.File( id ) : std::string();
                    const std::string        was = before.files.count( id ) ? before.File( id ) : std::string();
                    std::vector<std::string> touched;

                    if( !is.empty() )
                        touched.push_back( is );

                    // A symbol another file now holds leaves the file it was in
                    if( !was.empty() && was != is )
                        touched.push_back( was );

                    for( const std::string& file : touched )
                    {
                        const std::optional<std::string> member = file == is ? now.Member( id ) : std::nullopt;

                        if( isFootprintLibrary( library ) )
                        {
                            files[file] = member;
                            continue;
                        }

                        if( !files.count( file ) )
                        {
                            std::optional<std::string> text = fileAt( copy.Handle(), *head, file );
                            auto                       header = now.headers.find( file );

                            files[file] =
                                    text ? *text
                                         : ( header != now.headers.end() ? header->second : std::string() ) + ")\n";
                        }

                        files[file] = withSymbol( *files[file], name, member );

                        if( member )
                            splicedFrom[file] = now.headers[file];
                    }

                    touchedBy[id] = touched;
                    changed.push_back( id );
                }

                auto skip = [&]( const std::string& aFile, const std::string& aWhy )
                {
                    mdw::WholeFileSkip* skipped = aReply.add_skipped_files();

                    skipped->set_path( aFile );
                    skipped->set_reason( aWhy );
                };

                std::set<std::string> adopted;

                // Adopt a file that was only reformatted, or whose real edits are all members checked in here
                for( const auto& [file, digest] : aRequest.whole_files() )
                {
                    // A member's splice stands for its file when the file cannot be adopted
                    auto skipUnspliced = [&]( const std::string& aWhy )
                    {
                        if( !files.count( file ) )
                            skip( file, aWhy );
                    };

                    if( !now.texts.count( file ) )
                    {
                        skipUnspliced( "It is not a library file in the copy." );
                        continue;
                    }

                    // One read is digested and then committed, so an edit saved since listing cannot go in unseen
                    std::optional<std::string> raw = MDW_ReadFile( copy.root / MdwFsPath( file ) );

                    if( !raw || KIGIT::BlobDigest( *raw ) != digest )
                        return MdwFail( ws::WE_VERSION_CONFLICT, file + " changed since it was listed" );

                    // Only an edit to a released member keeps to the release rules, since the request vouches that the
                    // file's other members were only reformatted
                    auto denied = [&]( const CONTENT& aContent )
                    {
                        for( const auto& [id, member] : aContent.files )
                        {
                            if( member == file && std::ranges::count( wanted, id )
                                && !checkInDenial( releases, before, now, id, m_holdsClaim ).empty() )
                            {
                                return true;
                            }
                        }

                        return false;
                    };

                    if( !aRequest.format_upgrade() && ( denied( before ) || denied( now ) ) )
                    {
                        skipUnspliced( "It holds an edit to a released member. Revise the member to check it in." );
                        continue;
                    }

                    std::optional<std::string> text = GetGitBackend()->FilterText( copy.Handle(), file, *raw, true );
                    std::optional<std::string> committed = fileAt( copy.Handle(), *head, file );

                    if( !text )
                    {
                        skipUnspliced( "It cannot be read." );
                        continue;
                    }

                    const std::string ours = KIGIT::FormatVersion( *text );
                    const std::string team = committed ? KIGIT::FormatVersion( *committed ) : std::string();

                    // Adopted whole, an older file would take the team library back to a format it was upgraded from
                    if( !aRequest.format_upgrade() && !ours.empty() && !team.empty()
                        && KIGIT::NewerFormat( team, ours ) )
                    {
                        skipUnspliced( "It is in an older file format than the team library. " + GET_UPDATES_FIRST );
                    }
                    else if( text != committed )
                    {
                        files[file] = std::move( *text );
                        adopted.insert( file );
                    }
                }

                std::map<std::string, ws::WorkspaceError> refusedFiles;

                // A released member the file rewrites cannot go in beside its other changes without being revised
                auto unrevised = [&]( const std::string& aFile )
                {
                    std::string names;

                    for( const auto& [id, file] : now.files )
                    {
                        std::string library;
                        std::string name;

                        if( file != aFile || checkInDenial( releases, before, now, id, m_holdsClaim ).empty() )
                            continue;

                        names += ( names.empty() ? "" : ", " )
                                 + ( splitMemberId( id, library, name ) ? name : "the released part" );
                    }

                    return names;
                };

                // An adopted file carries its own header, so only a member left spliced under HEAD's can differ
                for( const auto& [file, header] : splicedFrom )
                {
                    const std::optional<std::string>& text = files.at( file );
                    const std::string                 names = unrevised( file );
                    const std::string                 ifNewer =
                            names.empty() ? std::string( "Check in every change in the file together." )
                                          : "Revise " + names + ", then check in every change in the file together.";

                    if( std::optional<ws::WorkspaceError> mixed =
                                text ? formatMismatch( file, header, *text, ifNewer, GET_UPDATES_FIRST )
                                     : std::nullopt )
                    {
                        refusedFiles[file] = *mixed;
                    }
                }

                auto refusedFile = [&]( const std::string& aId ) -> const ws::WorkspaceError*
                {
                    auto touched = touchedBy.find( aId );

                    if( touched == touchedBy.end() )
                        return nullptr;

                    for( const std::string& file : touched->second )
                    {
                        if( auto refused = refusedFiles.find( file ); refused != refusedFiles.end() )
                            return &refused->second;
                    }

                    return nullptr;
                };

                // A member moved between files goes in with both or neither, so a refusal spreads to its other file
                for( bool grew = !refusedFiles.empty(); grew; )
                {
                    grew = false;

                    for( const auto& [id, touched] : touchedBy )
                    {
                        if( const ws::WorkspaceError* refused = refusedFile( id ) )
                        {
                            const ws::WorkspaceError why = *refused;

                            for( const std::string& file : touched )
                                grew = refusedFiles.emplace( file, why ).second || grew;
                        }
                    }
                }

                std::erase_if( changed,
                               [&]( const std::string& aId )
                               {
                                   const ws::WorkspaceError* refused = refusedFile( aId );

                                   if( refused )
                                       *result( aId )->mutable_error() = *refused;

                                   return refused != nullptr;
                               } );

                for( const auto& [file, why] : refusedFiles )
                {
                    files.erase( file );

                    if( aRequest.whole_files().count( file ) )
                        skip( file, why.message() );
                }

                stageLineEnds( copy.Handle(), *head, files );

                if( files.empty() )
                    return MdwOk();

                // A folder file left without symbols is removed, as the library writer removes it
                for( auto& [file, text] : files )
                {
                    if( text && file.ends_with( SYMBOL_EXT ) && !folderLibrary( file ).empty()
                        && KIGIT::SplitSymbols( *text ).spans.empty() )
                    {
                        text.reset();
                    }
                }

                bool behind = false;

                // A moved upstream answers every member, so the check-in is refused whole
                for( const std::string& id : changed )
                {
                    if( std::optional<ws::WorkspaceError> refused = upstreamChange( copy, *head, id, m_revisions ) )
                    {
                        *result( id )->mutable_error() = *refused;
                        behind = true;
                    }
                }

                if( behind )
                    return MdwOk();

                const GIT_IDENTITY author = identity( copy.Handle() );
                const std::string  name = author.SignedName();
                const std::string  email = author.SignedEmail();
                std::string        message = aRequest.comment().empty() ? "Update library members" : aRequest.comment();

                if( aRequest.format_upgrade() )
                    message += "\n\n" + FORMAT_UPGRADE_TRAILER;

                std::string reformatted;

                // History would date a member an adopted file only reformatted by this commit, and so end its release
                for( const auto& [id, file] : now.files )
                {
                    if( aRequest.format_upgrade() || !adopted.count( file ) || !files.count( file )
                        || std::ranges::count( changed, id ) || !before.files.count( id ) || before.File( id ) != file
                        || before.Member( id ) == now.Member( id ) )
                    {
                        continue;
                    }

                    reformatted += "\n" + REFORMATTED_TRAILER + id;
                }

                if( !reformatted.empty() )
                    message += "\n" + reformatted;

                const std::string branchRef = "refs/heads/" + copy.branch;
                std::string       commitId;

                if( GetGitBackend()->CommitFiles( copy.Handle(), copy.branch, *head, files, message, name, email,
                                                  commitId )
                    != 0 )
                {
                    return gitFailure( "cannot commit the check-in" );
                }

                std::optional<std::string>  upstream = copy.Upstream();
                std::vector<GIT_REF_RESULT> pushed;
                GIT_REF_UPDATE              push;

                push.localRef = wxString::FromUTF8( branchRef );
                push.remoteRef = wxString::FromUTF8( branchRef );
                push.leased = true;
                push.expectedOld = upstream ? wxString::FromUTF8( *upstream ) : wxString();

                ws::WorkspaceError pushError;

                const RefPushResult sent = m_remote.Push( copy.Handle(), REMOTE, { push }, pushed, pushError );

                // A refused change stays a working change, but a dropped push may have landed, so Update carries it
                if( pushError.code() != ws::WE_OK )
                {
                    // A refused login or write answered before any pack was taken
                    bool refused = sent == RefPushResult::Rejected || sent == RefPushResult::LeaseBroken
                                   || pushError.code() == ws::WE_AUTH_FAILED
                                   || pushError.code() == ws::WE_PERMISSION_DENIED;

                    if( !refused && fetch( copy ).code() == ws::WE_OK )
                        refused = copy.Upstream() != commitId;

                    if( refused )
                    {
                        if( std::string stayed = uncommit( copy, *head, commitId, files ); !stayed.empty() )
                            pushError.set_message( pushError.message() + ", and " + stayed + " stayed in the copy" );
                    }

                    if( changed.empty() )
                        return pushError;

                    for( const std::string& id : changed )
                        *result( id )->mutable_error() = pushError;

                    return MdwOk();
                }

                GetGitBackend()->SetReference( copy.Handle(), copy.UpstreamRef(), commitId, std::nullopt,
                                               "check in library members" );

                for( const std::string& id : changed )
                {
                    GetGitBackend()->DeleteReference( copy.Handle(), keptRef( id ), std::nullopt );

                    mdw::MemberResult* row = result( id );
                    row->set_outcome( ws::CO_APPLIED );
                    row->mutable_revision()->set_revision_id( commitId );
                    row->mutable_revision()->set_label( shortId( commitId ) );
                    row->mutable_revision()->set_kind( ws::RK_CHECKIN );
                    row->mutable_revision()->set_comment( message );
                    row->mutable_revision()->mutable_state()->set_category( ws::LC_EDITABLE );
                }

                snapshot( copy, commitId, std::string(), *aReply.mutable_snapshot() );
                *aReply.mutable_copy_snapshot() = aReply.snapshot();
                return MdwOk();
            } );
}
