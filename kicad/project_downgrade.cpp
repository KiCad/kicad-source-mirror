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

#include "project_downgrade.h"

#include <filesystem>
#include <system_error>
#include <utility>

#include <json_common.h>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <drc_rules_downgrade.h>
#include <jobs/job_pcb_downgrade.h>
#include <jobs/job_sch_downgrade.h>
#include <kiway.h>
#include <libraries/library_table_parser.h>
#include <project/project_file_downgrade.h>
#include <wildcards_and_files_ext.h>


// Collects the errors a job reports, so a failure message can say why instead of dead-ending
// on a generic line.
class ERROR_COLLECTING_REPORTER : public REPORTER
{
public:
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        if( aSeverity == RPT_SEVERITY_ERROR )
            m_errors += aText;

        return *this;
    }

    bool HasMessage() const override { return !m_errors.IsEmpty(); }

    /// The collected error text appended to aMessage, or aMessage alone when there is none.
    wxString Explain( const wxString& aMessage ) const
    {
        if( m_errors.IsEmpty() )
            return aMessage;

        wxString detail = m_errors;
        detail.Trim();

        return aMessage + wxT( "\n\n" ) + detail;
    }

private:
    wxString m_errors;
};


static bool quoteLegacyLibraryTableValue( const std::string& aValue, bool aPreTen, std::string& aQuoted )
{
    if( aValue.find( '\0' ) != std::string::npos )
        return false;

    if( !aPreTen )
    {
        if( aValue.find( '"' ) == std::string::npos )
        {
            // Frozen 10.0 takes quoted contents literally, including slashes and line breaks.
            aQuoted = '"' + aValue + '"';
            return true;
        }

        // Its bare TOKEN grammar accepts internal quotes, but cannot start with one.
        if( !aValue.empty() && aValue.front() != '"' && aValue.find_first_of( "() \t\n\r" ) == std::string::npos )
        {
            aQuoted = aValue;
            return true;
        }

        return false;
    }

    // Frozen 9.0 uses DSNLEXER and decodes escape sequences.
    aQuoted = '"';

    for( char c : aValue )
    {
        switch( c )
        {
        case '\\': aQuoted += "\\\\"; break;
        case '"': aQuoted += "\\\""; break;
        case '\n': aQuoted += "\\n"; break;
        case '\r': aQuoted += "\\r"; break;
        case '\t': aQuoted += "\\t"; break;
        default: aQuoted += c; break;
        }
    }

    aQuoted += '"';
    return true;
}


static bool legacyLibraryTables( const wxString& aDir, const DOWNGRADE_TARGET& aTarget,
                                 std::vector<std::pair<wxString, wxString>>& aTables, wxString& aError )
{
    wxArrayString files;
    wxDir::GetAllFiles( aDir, &files );

    for( const wxString& path : files )
    {
        wxFileName filename( path );

        if( IsDowngradeWorkingArtifact( path )
            || ( filename.GetFullName() != wxT( "sym-lib-table" ) && filename.GetFullName() != wxT( "fp-lib-table" ) ) )
        {
            continue;
        }

        wxFFile  in( path, wxT( "rb" ) );
        wxString text;

        if( !in.IsOpened() || !in.ReadAll( &text ) )
        {
            aError = wxString::Format( _( "Could not read library table '%s'." ), path );
            return false;
        }

        if( text.IsEmpty() )
            continue;

        LIBRARY_TABLE_PARSER parser;
        auto                 parsed = parser.ParseBuffer( text.ToStdString( wxConvUTF8 ) );

        if( !parsed )
        {
            aError = wxString::Format( _( "Library table '%s' is invalid: %s" ), path, parsed.error().description );
            return false;
        }

        std::string output = filename.GetFullName() == wxT( "sym-lib-table" ) ? "(sym_lib_table" : "(fp_lib_table";
        output += " (version 7)\n";

        for( const LIBRARY_TABLE_ROW_IR& row : parsed->rows )
        {
            if( row.type == "Table" && aTarget.m_boardVersion < 20260206 )
            {
                aError = wxString::Format( _( "Library table '%s' uses a nested table that %s cannot open." ), path,
                                           aTarget.m_name );
                return false;
            }

            output += "  (lib";

            for( const auto& [key, value] : { std::pair{ "name", row.nickname },
                                              { "type", row.type },
                                              { "uri", row.uri },
                                              { "options", row.options },
                                              { "descr", row.description } } )
            {
                // Keep an unspecified plugin type absent, matching the source's default.
                if( std::string( key ) == "type" && value.empty() )
                    continue;

                std::string quoted;

                if( !quoteLegacyLibraryTableValue( value, aTarget.m_boardVersion < 20260206, quoted ) )
                {
                    aError = wxString::Format(
                            _( "Library table '%s' contains a quoted value that %s cannot represent exactly." ), path,
                            aTarget.m_name );
                    return false;
                }

                output += " (" + std::string( key ) + ' ' + quoted + ')';
            }

            if( row.disabled )
                output += " (disabled)";

            if( row.hidden )
                output += " (hidden)";

            output += ")\n";
        }

        output += ")\n";
        aTables.emplace_back( path, wxString::FromUTF8( output.c_str() ) );
    }

    return true;
}


// Parse a project file, migrate it down, and write it back. wx file IO throughout, so
// non-ASCII paths work on every platform.
static bool downgradeProjectFileOnDisk( const wxString& aPath, const DOWNGRADE_TARGET& aTarget,
                                        COMPATIBILITY_REPORT& aReport )
{
    wxString text;

    {
        wxFFile in( aPath, wxT( "rb" ) );

        if( !in.IsOpened() || !in.ReadAll( &text ) )
            return false;
    }

    nlohmann::json doc;

    try
    {
        doc = nlohmann::json::parse( text.ToStdString( wxConvUTF8 ) );
        DowngradeProjectFileJson( doc, aTarget, aReport );

        if( aReport.IsBlocked() )
            return false;
    }
    catch( ... )
    {
        return false;
    }

    wxFFile out( aPath, wxT( "wb" ) );

    return out.IsOpened() && out.Write( wxString::FromUTF8( ( doc.dump( 2 ) + "\n" ).c_str() ) );
}


// Read the (version N) stamp of a KiCad s-expression file. Returns 0 if not found. The header
// is ASCII, so the scan works on raw bytes and a chunk cut inside a multibyte character cannot
// blind it. Quoted strings are stripped so text content cannot shadow the header.
static int readSexprVersion( const wxString& aPath )
{
    wxFFile file( aPath, wxT( "rb" ) );

    if( !file.IsOpened() )
        return 0;

    char   buf[8192];
    size_t n = file.Read( buf, sizeof( buf ) - 1 );

    std::string head = StripSexprStrings( std::string( buf, n ) );
    size_t      idx = head.find( "(version " );

    if( idx == std::string::npos )
        return 0;

    return atoi( head.c_str() + idx + 9 );
}


// Final backstop over the exported copy. Anything still stamped newer than the target would not
// open there. Returns the first such file, or empty if all are safe.
static wxString firstTooNewFile( const wxString& aDir, const DOWNGRADE_TARGET& aTarget )
{
    auto scan = [&]( const wxString& aPattern, int aMaxVersion ) -> wxString
    {
        wxArrayString files;
        wxDir::GetAllFiles( aDir, &files, aPattern );

        for( const wxString& f : files )
        {
            if( !IsDowngradeWorkingArtifact( f ) && readSexprVersion( f ) > aMaxVersion )
                return f;
        }

        return wxEmptyString;
    };

    wxString bad = scan( wxT( "*.kicad_sym" ), aTarget.m_symLibVersion );

    if( !bad.IsEmpty() )
        return bad;

    bad = scan( wxT( "*.kicad_mod" ), aTarget.m_boardVersion );

    if( !bad.IsEmpty() )
        return bad;

    bad = scan( wxT( "*.kicad_pcb" ), aTarget.m_boardVersion );

    if( !bad.IsEmpty() )
        return bad;

    return scan( wxT( "*.kicad_sch" ), aTarget.m_schVersion );
}


bool ClassifyProjectForDowngrade( KIWAY& aKiway, const wxString& aProjectDir, const wxString& aProjectName,
                                  const wxString& aTargetName, COMPATIBILITY_REPORT& aReport, wxString& aError,
                                  std::vector<wxString>* aFoundVariants, bool aDropInsteadOfApproximate,
                                  const wxString& aVariant )
{
    wxFileName boardFn( aProjectDir, aProjectName, FILEEXT::KiCadPcbFileExtension );
    wxFileName schFn( aProjectDir, aProjectName, FILEEXT::KiCadSchematicFileExtension );

    if( !boardFn.FileExists() && !schFn.FileExists() )
    {
        aError = _( "This project has no board or schematic to export." );
        return false;
    }

    const DOWNGRADE_TARGET* target = FindDowngradeTarget( aTargetName );
    wxFileName              proFn( aProjectDir, aProjectName, FILEEXT::ProjectFileExtension );

    // Settings guards run before native jobs can reinterpret a future project schema.
    if( target && proFn.FileExists() )
    {
        wxString text;
        wxFFile  in( proFn.GetFullPath(), wxT( "rb" ) );

        if( in.IsOpened() && in.ReadAll( &text ) )
        {
            try
            {
                nlohmann::json doc = nlohmann::json::parse( text.ToStdString( wxConvUTF8 ) );
                DowngradeProjectFileJson( doc, *target, aReport );
            }
            catch( ... )
            {
                // Native readers diagnose malformed source projects.
            }
        }

        if( aReport.IsBlocked() )
            return true;
    }

    {
        ERROR_COLLECTING_REPORTER reporter;
        JOB_PCB_DOWNGRADE         job;
        job.m_filename = boardFn.FileExists() ? boardFn.GetFullPath() : wxString();
        job.m_target = aTargetName;
        job.m_libraryDir = aProjectDir;
        job.m_variant = boardFn.FileExists() ? aVariant : wxString();
        job.m_dryRun = true;
        job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;

        if( aKiway.ProcessJob( KIWAY::FACE_PCB, &job, &reporter ) != 0 )
        {
            aError = reporter.Explain( _( "Could not read the board to check compatibility." ) );
            return false;
        }

        aReport.Merge( job.m_report );
    }

    {
        ERROR_COLLECTING_REPORTER reporter;
        JOB_SCH_DOWNGRADE         job;
        job.m_filename = schFn.FileExists() ? schFn.GetFullPath() : wxString();
        job.m_target = aTargetName;
        job.m_libraryDir = aProjectDir;
        job.m_variant = schFn.FileExists() ? aVariant : wxString();
        job.m_dryRun = true;
        job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;

        if( aKiway.ProcessJob( KIWAY::FACE_SCH, &job, &reporter ) != 0 )
        {
            aError = reporter.Explain( _( "Could not read the schematic to check compatibility." ) );
            return false;
        }

        aReport.Merge( job.m_report );

        if( aFoundVariants )
            *aFoundVariants = job.m_foundVariants;
    }

    wxFileName              druFn( aProjectDir, aProjectName, FILEEXT::DesignRulesFileExtension );

    if( target )
    {
        std::vector<std::pair<wxString, wxString>> tables;
        wxString                                   tableError;

        if( !legacyLibraryTables( aProjectDir, *target, tables, tableError ) )
            aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Project library tables" ), tableError );
    }

    if( target && druFn.FileExists() )
    {
        wxString rulesText;
        wxFFile  file( druFn.GetFullPath(), wxT( "rb" ) );

        if( file.IsOpened() && file.ReadAll( &rulesText ) )
        {
            DRC_RULES_FILTER_RESULT filtered = FilterDrcRulesForTarget( rulesText, *target );

            if( !filtered.m_error.IsEmpty() )
                aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Invalid custom DRC rules" ), filtered.m_error );

            for( const wxString& name : filtered.m_dropped )
            {
                wxString shown = name.IsEmpty() ? _( "unnamed" ) : name;

                aReport.Add( DOWNGRADE_BUCKET::DROP, wxString::Format( _( "Custom DRC rule '%s'" ), shown ),
                             _( "The target cannot evaluate it with the same meaning. The rule is removed." ), 1 );
            }
        }
    }

    return true;
}


bool ExportProjectForDowngrade( KIWAY& aKiway, const wxString& aProjectDir, const wxString& aProjectName,
                                const wxString& aTargetName, const wxString& aDestDir, wxString& aError,
                                const wxString& aVariant, bool aDropInsteadOfApproximate )
{
    const DOWNGRADE_TARGET* target = FindDowngradeTarget( aTargetName );

    if( !target )
    {
        aError = wxString::Format( _( "Unknown target version '%s'. Valid targets: %s" ), aTargetName,
                                   DowngradeTargetNames() );
        return false;
    }

    std::vector<std::pair<wxString, wxString>> tables;

    if( !legacyLibraryTables( aProjectDir, *target, tables, aError ) )
        return false;

    std::error_code       ec;
    std::filesystem::path srcPath = std::filesystem::weakly_canonical( aProjectDir.ToStdWstring(), ec );
    std::filesystem::path destPath = std::filesystem::weakly_canonical( aDestDir.ToStdWstring(), ec );

    // The destination must not be the project folder or live inside it, or the copy would feed
    // on its own output. Existing ancestors are compared by identity, not by spelling, so a
    // case or UNC variation of the same folder cannot slip past.
    for( std::filesystem::path p = destPath; !p.empty() && p != p.parent_path(); p = p.parent_path() )
    {
        std::error_code cmpEc;

        if( p == srcPath || ( std::filesystem::exists( p, cmpEc ) && std::filesystem::equivalent( p, srcPath, cmpEc ) ) )
        {
            aError = _( "Choose a destination outside the current project." );
            return false;
        }
    }

    bool created = std::filesystem::create_directories( destPath, ec );

    if( ec )
    {
        aError = wxString::Format( _( "Could not create the folder '%s'." ), aDestDir );
        return false;
    }

    if( !created && !std::filesystem::is_empty( destPath, ec ) )
    {
        aError = wxString::Format( _( "The folder '%s' is not empty. Choose a new or empty folder." ), aDestDir );
        return false;
    }

    // Take away only what the export put there. A folder the user already had stays, because the
    // export did not create it. Report a cleanup that did not finish rather than claiming the
    // destination is untouched.
    auto failClosed = [&]( const wxString& aMessage ) -> bool
    {
        std::error_code cleanupEc;

        if( created )
        {
            std::filesystem::remove_all( destPath, cleanupEc );
        }
        else
        {
            std::vector<std::filesystem::path> entries;

            for( const auto& entry : std::filesystem::directory_iterator( destPath, cleanupEc ) )
                entries.push_back( entry.path() );

            for( const std::filesystem::path& entry : entries )
                std::filesystem::remove_all( entry, cleanupEc );
        }

        aError = aMessage;

        if( cleanupEc )
        {
            aError << wxS( " " )
                   << wxString::Format( _( "Some files could not be removed from '%s'." ), aDestDir );
        }

        return false;
    };

    // Design files only. Datasheets, gerbers, UI state and other working files stay behind.
    {
        wxArrayString files;
        wxDir::GetAllFiles( aProjectDir, &files );

        for( const wxString& file : files )
        {
            if( IsDowngradeWorkingArtifact( file ) || !IsDowngradeDesignFile( file ) )
                continue;

            wxFileName rel( file );
            rel.MakeRelativeTo( aProjectDir );

            wxFileName out( aDestDir + wxFileName::GetPathSeparator() + rel.GetFullPath() );

            if( !out.DirExists() && !wxFileName::Mkdir( out.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
                return failClosed( _( "Could not copy the project." ) );

            if( !wxCopyFile( file, out.GetFullPath() ) )
                return failClosed( _( "Could not copy the project." ) );
        }
    }

    wxFileName boardFn( aProjectDir, aProjectName, FILEEXT::KiCadPcbFileExtension );
    wxFileName schFn( aProjectDir, aProjectName, FILEEXT::KiCadSchematicFileExtension );

    // Consent was collected against the classification, so the jobs run forced.
    {
        wxFileName destBoard( aDestDir, boardFn.GetFullName() );

        ERROR_COLLECTING_REPORTER reporter;
        JOB_PCB_DOWNGRADE         job;
        job.m_filename = boardFn.FileExists() ? destBoard.GetFullPath() : wxString();
        job.m_inPlace = true;
        job.m_outputFile = destBoard.GetFullPath();
        job.m_target = aTargetName;
        job.m_libraryDir = aDestDir;
        job.m_variant = boardFn.FileExists() ? aVariant : wxString();
        job.m_force = true;
        job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;

        if( aKiway.ProcessJob( KIWAY::FACE_PCB, &job, &reporter ) != 0 )
        {
            return failClosed( reporter.Explain( _( "Export failed: the board could not be converted. Nothing "
                                                    "was written." ) ) );
        }
    }

    {
        wxFileName destSch( aDestDir, schFn.GetFullName() );

        ERROR_COLLECTING_REPORTER reporter;
        JOB_SCH_DOWNGRADE         job;
        job.m_filename = schFn.FileExists() ? destSch.GetFullPath() : wxString();
        job.m_inPlace = true;
        job.m_target = aTargetName;
        job.m_libraryDir = aDestDir;
        job.m_variant = schFn.FileExists() ? aVariant : wxString();
        job.m_force = true;
        job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;

        if( aKiway.ProcessJob( KIWAY::FACE_SCH, &job, &reporter ) != 0 )
        {
            return failClosed( reporter.Explain( _( "Export failed: the schematic could not be converted. Nothing "
                                                    "was written." ) ) );
        }
    }

    wxFileName destPro( aDestDir, aProjectName, FILEEXT::ProjectFileExtension );

    if( destPro.FileExists() )
    {
        COMPATIBILITY_REPORT ignored;

        if( !downgradeProjectFileOnDisk( destPro.GetFullPath(), *target, ignored ) )
            return failClosed( _( "Export failed: could not migrate the project file." ) );
    }

    // The old rules parsers are all-or-nothing, so unknown rules must not reach the copy.
    wxFileName destDru( aDestDir, aProjectName, FILEEXT::DesignRulesFileExtension );

    if( destDru.FileExists() )
    {
        wxString rulesText;

        {
            wxFFile file( destDru.GetFullPath(), wxT( "rb" ) );

            if( !file.IsOpened() || !file.ReadAll( &rulesText ) )
                return failClosed( _( "Export failed: could not read the design rules file." ) );
        }

        DRC_RULES_FILTER_RESULT filtered = FilterDrcRulesForTarget( rulesText, *target );

        if( !filtered.m_error.IsEmpty() )
            return failClosed( wxString::Format( _( "Export failed: %s" ), filtered.m_error ) );

        wxFFile out( destDru.GetFullPath(), wxT( "wb" ) );

        if( !out.IsOpened() || !out.Write( filtered.m_text ) )
            return failClosed( _( "Export failed: could not write the design rules file." ) );
    }

    // Convert table quoting only after the native jobs have finished using the source grammar.
    for( const auto& [source, text] : tables )
    {
        wxFileName relative( source );
        relative.MakeRelativeTo( aProjectDir );
        wxFileName destination( aDestDir + wxFileName::GetPathSeparator() + relative.GetFullPath() );
        wxFFile    out( destination.GetFullPath(), wxT( "wb" ) );

        if( !out.IsOpened() || !out.Write( text ) )
            return failClosed( _( "Export failed: could not write the library table." ) );
    }

    // Final backstop: nothing left in the copy may still be too new for the target to open.
    wxString tooNew = firstTooNewFile( aDestDir, *target );

    if( !tooNew.IsEmpty() )
    {
        // Point at the source path, not the deleted copy, so the user can act on it.
        wxFileName relative( tooNew );
        relative.MakeRelativeTo( aDestDir );
        wxString source = wxFileName( aProjectDir, wxEmptyString ).GetPathWithSep() + relative.GetFullPath();

        return failClosed( wxString::Format( _( "Export failed: '%s' is not handled by the export and %s cannot "
                                                "open it. Remove or update the file and try again. Nothing was "
                                                "written." ),
                                             source, target->m_name ) );
    }

    return true;
}
