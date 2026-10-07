/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DOWNGRADE_SCAN_H
#define DOWNGRADE_SCAN_H

#include <cctype>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/utils.h>

/// A token dated by the format that introduced it, for the fail-closed downgrade gates.
struct DATED_TOKEN
{
    const char* m_token;
    int         m_introducedIn;
};


/// Drop quoted strings so a token inside a text value cannot cause a false refusal.
inline std::string StripSexprStrings( const std::string& aText )
{
    std::string result;
    bool        inString = false;

    for( size_t i = 0; i < aText.size(); i++ )
    {
        char c = aText[i];

        // A quote is real only if an even number of backslashes precede it. This stops a value that
        // ends in a backslash from swallowing the rest of the file.
        if( c == '"' )
        {
            size_t backslashes = 0;

            for( size_t k = i; k > 0 && aText[k - 1] == '\\'; k-- )
                backslashes++;

            if( backslashes % 2 == 0 )
            {
                inString = !inString;
                continue;
            }
        }

        if( !inString )
            result += c;
    }

    return result;
}


inline bool ContainsSexprNode( const std::string& aText, const std::string& aToken )
{
    const std::string needle = "(" + aToken;

    for( size_t pos = aText.find( needle ); pos != std::string::npos; pos = aText.find( needle, pos + 1 ) )
    {
        // Match the whole token, so "(net" is never mistaken for "(net_chain".
        size_t after = pos + needle.size();
        char   next = ( after < aText.size() ) ? aText[after] : ' ';

        if( !std::isalnum( static_cast<unsigned char>( next ) ) && next != '_' )
            return true;
    }

    return false;
}


/// Some features are written as a bareword value, not a node head, so the node scan cannot see
/// them. Match the token as a whole word instead.
inline bool ContainsSexprValue( const std::string& aText, const std::string& aToken )
{
    for( size_t pos = aText.find( aToken ); pos != std::string::npos; pos = aText.find( aToken, pos + 1 ) )
    {
        char   prev = pos > 0 ? aText[pos - 1] : ' ';
        size_t after = pos + aToken.size();
        char   next = after < aText.size() ? aText[after] : ' ';

        bool prevOk = !std::isalnum( static_cast<unsigned char>( prev ) ) && prev != '_';
        bool nextOk = !std::isalnum( static_cast<unsigned char>( next ) ) && next != '_';

        if( prevOk && nextOk )
            return true;
    }

    return false;
}


/// Scan serialized s-expression text against dated node and value denylists. Returns the first
/// token the target cannot parse, or empty if the text is safe.
inline wxString FindUnsupportedToken( const wxString& aSerialized, const DATED_TOKEN* aNodes, size_t aNodeCount,
                                      const DATED_TOKEN* aValues, size_t aValueCount, int aTargetVersion )
{
    // Explicit UTF-8: the default locale conversion returns an empty string on unrepresentable
    // characters, which would silently pass the scan.
    std::string text = StripSexprStrings( aSerialized.ToStdString( wxConvUTF8 ) );

    for( size_t i = 0; i < aNodeCount; i++ )
    {
        if( aTargetVersion < aNodes[i].m_introducedIn && ContainsSexprNode( text, aNodes[i].m_token ) )
            return wxString::FromUTF8( aNodes[i].m_token );
    }

    for( size_t i = 0; i < aValueCount; i++ )
    {
        if( aTargetVersion < aValues[i].m_introducedIn && ContainsSexprValue( text, aValues[i].m_token ) )
            return wxString::FromUTF8( aValues[i].m_token );
    }

    return wxEmptyString;
}


/// Every lowercase node head in the text, e.g. for checking output against a release keyword set.
inline std::set<std::string> ExtractSexprNodeHeads( const std::string& aText )
{
    std::set<std::string> heads;

    for( size_t i = 0; i + 1 < aText.size(); i++ )
    {
        if( aText[i] != '(' || !std::islower( static_cast<unsigned char>( aText[i + 1] ) ) )
            continue;

        size_t j = i + 1;

        while( j < aText.size()
               && ( std::islower( static_cast<unsigned char>( aText[j] ) )
                    || std::isdigit( static_cast<unsigned char>( aText[j] ) ) || aText[j] == '_' ) )
        {
            j++;
        }

        heads.insert( aText.substr( i + 1, j - i - 1 ) );
    }

    return heads;
}


/// Editor working state (autosaves, local history, backups) holds current-format copies nothing
/// downgrades, so exports and scans must skip it.
inline bool IsDowngradeWorkingArtifact( const wxString& aPath )
{
    wxFileName fn( aPath );

    if( fn.GetFullName().StartsWith( wxT( "_autosave-" ) ) )
        return true;

    for( const wxString& dir : fn.GetDirs() )
    {
        if( dir == wxT( ".history" ) || dir == wxT( ".git" ) || dir.EndsWith( wxT( "-backups" ) ) )
            return true;
    }

    return false;
}


/// Files the project export carries over. Everything else in the project folder stays behind:
/// UI state, backups, and non-KiCad files like datasheets and gerbers. Case-sensitive on
/// purpose, so the filter agrees with the converter globs on every platform.
inline bool IsDowngradeDesignFile( const wxString& aPath )
{
    wxFileName fn( aPath );
    wxString   ext = fn.GetExt();

    if( ext == wxT( "kicad_pro" ) || ext == wxT( "kicad_sch" ) || ext == wxT( "kicad_pcb" ) || ext == wxT( "kicad_dru" )
        || ext == wxT( "kicad_wks" ) || ext == wxT( "kicad_sym" ) || ext == wxT( "kicad_mod" ) )
    {
        return true;
    }

    wxString name = fn.GetFullName();
    return name == wxT( "sym-lib-table" ) || name == wxT( "fp-lib-table" );
}


/// Read back a downgraded output and require the target stamp plus a clean token scan.
/// An output that cannot be read back to verify counts as a failure too.
inline bool VerifyDowngradedFile( const wxString& aPath, int aStampVersion,
                                  const std::function<wxString( const wxString& )>& aFindForbidden,
                                  wxString*                                         aForbidden = nullptr )
{
    wxString content;
    bool     readOk;

    {
        wxFFile file( aPath, wxT( "rb" ) );
        readOk = file.IsOpened() && file.ReadAll( &content );
    }

    if( !readOk )
        return false;

    if( !content.Contains( wxString::Format( wxT( "(version %d)" ), aStampVersion ) ) )
        return false;

    wxString forbidden = aFindForbidden( content );

    if( aForbidden )
        *aForbidden = forbidden;

    return forbidden.IsEmpty();
}


/// What a per-file converter did with one library file.
enum class DOWNGRADE_FILE_RESULT
{
    CONVERTED, ///< The temp file holds the downgraded copy
    SKIPPED,   ///< Not a convertible file, leave it alone
    REFUSED    ///< The target cannot represent this file
};


/// Stage related design and library files together, retaining originals until every write is ready.
class DOWNGRADE_FILE_TRANSACTION
{
public:
    DOWNGRADE_FILE_TRANSACTION() = default;
    DOWNGRADE_FILE_TRANSACTION( const DOWNGRADE_FILE_TRANSACTION& ) = delete;
    DOWNGRADE_FILE_TRANSACTION& operator=( const DOWNGRADE_FILE_TRANSACTION& ) = delete;

    ~DOWNGRADE_FILE_TRANSACTION()
    {
        for( const ENTRY& entry : m_entries )
        {
            removeOwnedFile( entry.m_temp );

            if( !entry.m_keepBackup )
                removeOwnedFile( entry.m_backup );
        }
    }

    /// Return an exclusively-created sibling staging file, or empty if it cannot be created.
    wxString Stage( const wxString& aDestination )
    {
        wxFileName destination( aDestination );

        if( m_finished || destination.GetFullName().IsEmpty() || !destination.MakeAbsolute() || !destination.DirExists()
            || wxDirExists( destination.GetFullPath() ) )
        {
            return wxEmptyString;
        }

        for( const ENTRY& entry : m_entries )
        {
            if( wxFileName( entry.m_destination ).SameAs( destination ) )
                return wxEmptyString;
        }

        wxString temporary = wxFileName::CreateTempFileName( destination.GetFullPath() + wxT( ".downgrade_tmp." ) );

        if( temporary.IsEmpty() )
            return wxEmptyString;

        ENTRY entry;
        entry.m_temp = temporary;
        entry.m_destination = destination.GetFullPath();
        m_entries.push_back( std::move( entry ) );
        return temporary;
    }

    /// Forget a skipped conversion without exposing its empty staging file as an output.
    void Discard( const wxString& aTemporary )
    {
        for( auto it = m_entries.begin(); it != m_entries.end(); ++it )
        {
            if( it->m_temp == aTemporary )
            {
                removeOwnedFile( it->m_temp );
                m_entries.erase( it );
                return;
            }
        }
    }

    /// Replace all destinations, rolling back earlier replacements if a rename fails.
    /// Returns the failed destination, or empty after a successful commit.
    wxString Commit()
    {
        if( m_finished )
            return m_entries.empty() ? wxString() : m_entries.front().m_destination;

        m_finished = true;

        // Back up every existing destination before the first replacement, including symlinks.
        for( ENTRY& entry : m_entries )
        {
            if( !wxFileExists( entry.m_temp ) || wxDirExists( entry.m_destination ) )
                return entry.m_destination;

            std::error_code error;
            auto            destination = nativePath( entry.m_destination );
            bool            symlink = std::filesystem::is_symlink( destination, error );

            if( error && error != std::errc::no_such_file_or_directory )
                return entry.m_destination;

            entry.m_hadOriginal = symlink || wxFileExists( entry.m_destination );

            if( !entry.m_hadOriginal )
                continue;

            if( wxFileExists( entry.m_destination ) )
            {
                auto permissions = std::filesystem::status( destination, error ).permissions();

                if( error )
                    return entry.m_destination;

                std::filesystem::permissions( nativePath( entry.m_temp ), permissions, error );

                if( error )
                    return entry.m_destination;
            }

            entry.m_backup = wxFileName::CreateTempFileName( entry.m_destination + wxT( ".downgrade_backup." ) );

            if( entry.m_backup.IsEmpty() )
                return entry.m_destination;

            if( symlink )
            {
                // copy_symlink creates the backup exclusively and preserves relative link targets.
                if( !wxRemoveFile( entry.m_backup ) )
                    return entry.m_destination;

                wxString backup = entry.m_backup;
                entry.m_backup.clear();
                std::filesystem::copy_symlink( destination, nativePath( backup ), error );

                if( error )
                    return entry.m_destination;

                entry.m_backup = backup;
            }
            else if( !wxCopyFile( entry.m_destination, entry.m_backup, true ) )
            {
                return entry.m_destination;
            }
        }

        for( ENTRY& entry : m_entries )
        {
            // An overwrite may disturb its destination even when the rename reports a failure.
            entry.m_attemptedReplacement = true;

            if( !wxRenameFile( entry.m_temp, entry.m_destination, true ) )
            {
                rollback();
                return entry.m_destination;
            }

            entry.m_temp.clear();
        }

        return wxEmptyString;
    }

    /// Recovery details are nonempty only if restoring an original or removing a new output failed.
    const wxString& GetRecoveryError() const { return m_recoveryError; }

private:
    struct ENTRY
    {
        wxString m_temp;
        wxString m_destination;
        wxString m_backup;
        bool     m_hadOriginal = false;
        bool     m_attemptedReplacement = false;
        bool     m_keepBackup = false;
    };

    static void removeOwnedFile( const wxString& aPath )
    {
        if( ownedFileExists( aPath ) )
            wxRemoveFile( aPath );
    }

    static bool ownedFileExists( const wxString& aPath )
    {
        if( aPath.IsEmpty() )
            return false;

        std::error_code error;
        return wxFileExists( aPath ) || std::filesystem::is_symlink( nativePath( aPath ), error );
    }

    static std::filesystem::path nativePath( const wxString& aPath )
    {
#ifdef _WIN32
        return std::filesystem::path( aPath.ToStdWstring() );
#else
        return std::filesystem::path( aPath.utf8_string() );
#endif
    }

    void rollback()
    {
        for( auto it = m_entries.rbegin(); it != m_entries.rend(); ++it )
        {
            ENTRY& entry = *it;

            if( !entry.m_attemptedReplacement )
                continue;

            if( entry.m_hadOriginal )
            {
                if( wxRenameFile( entry.m_backup, entry.m_destination, true ) )
                {
                    entry.m_backup.clear();
                }
                else
                {
                    entry.m_keepBackup = true;
                    m_recoveryError += wxString::Format( _( "Could not restore '%s'. The original is saved at '%s'." ),
                                                         entry.m_destination, entry.m_backup )
                                       + wxT( "\n" );
                }
            }
            else if( ownedFileExists( entry.m_destination ) && !wxRemoveFile( entry.m_destination ) )
            {
                m_recoveryError +=
                        wxString::Format( _( "Could not remove incomplete output '%s'." ), entry.m_destination )
                        + wxT( "\n" );
            }
        }
    }

    std::vector<ENTRY> m_entries;
    bool               m_finished = false;
    wxString           m_recoveryError;
};


/// Downgrade every library file matching aGlob under aDir in place. aConvert writes one file's
/// downgraded copy to the temp path. With aTransaction, leave replacements staged for its owner.
/// Otherwise commit all library replacements together after every converted file is verified.
/// Returns the first file the target could not open or replace, or empty when all are safe.
inline wxString
DowngradeLibraryFilesInPlace( const wxString& aDir, const wxString& aGlob, int aStampVersion,
                              const std::function<wxString( const wxString& )>& aFindForbidden,
                              const std::function<DOWNGRADE_FILE_RESULT( const wxString&, const wxString& )>& aConvert,
                              DOWNGRADE_FILE_TRANSACTION* aTransaction = nullptr, wxString* aRecoveryError = nullptr )
{
    if( aRecoveryError )
        aRecoveryError->clear();

    wxArrayString files;
    wxDir::GetAllFiles( aDir, &files, aGlob );

    DOWNGRADE_FILE_TRANSACTION  localTransaction;
    DOWNGRADE_FILE_TRANSACTION& transaction = aTransaction ? *aTransaction : localTransaction;

    for( const wxString& file : files )
    {
        if( IsDowngradeWorkingArtifact( file ) )
            continue;

        wxString temporary = transaction.Stage( file );

        if( temporary.IsEmpty() )
            return file;

        switch( aConvert( file, temporary ) )
        {
        case DOWNGRADE_FILE_RESULT::SKIPPED: transaction.Discard( temporary ); continue;

        case DOWNGRADE_FILE_RESULT::REFUSED: return file;

        case DOWNGRADE_FILE_RESULT::CONVERTED: break;
        }

        if( !VerifyDowngradedFile( temporary, aStampVersion, aFindForbidden ) )
            return file;
    }

    if( aTransaction )
        return wxEmptyString;

    wxString failed = transaction.Commit();

    if( aRecoveryError )
        *aRecoveryError = transaction.GetRecoveryError();

    return failed;
}

#endif // DOWNGRADE_SCAN_H
