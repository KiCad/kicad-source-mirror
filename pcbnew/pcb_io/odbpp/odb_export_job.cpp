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

#include "odb_export_job.h"
#include "odb_entity.h"
#include "odb_util.h"
#include "pcb_io_odbpp.h"

#include <thread_pool.h>
#include <gestfich.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <set>

#include <wx/dir.h>

#include <board.h>
#include <reporter.h>
#include <confirm.h>
#include <kidialog.h>
#include <paths.h>
#include <pcb_edit_frame.h>
#include <progress_reporter.h>
#include <project.h>
#include <io/io_mgr.h>
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/pcb_io_mgr.h>
#include <locale_io.h>
#include <string_utils.h>


namespace
{
class TEMP_ODB_DIRECTORY
{
public:
    bool Create( const wxString& aPrefix = wxS( "kicad-odb" ) )
    {
        m_path = wxFileName::CreateTempFileName( aPrefix );

        if( m_path.IsEmpty() || !wxRemoveFile( m_path ) )
            return false;

        return wxFileName::Mkdir( m_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    ~TEMP_ODB_DIRECTORY()
    {
        if( wxDirExists( m_path ) )
            wxFileName::Rmdir( m_path, wxPATH_RMDIR_RECURSIVE );
        else if( wxFileExists( m_path ) )
            wxRemoveFile( m_path );
    }

    const wxString& Path() const { return m_path; }

private:
    wxString m_path;
};


std::filesystem::path toFsPath( const wxString& aPath )
{
#ifdef __WXMSW__
    return std::filesystem::path( std::wstring( aPath.wc_str() ) );
#else
    return std::filesystem::path( aPath.utf8_string() );
#endif
}


wxString fromFsPath( const std::filesystem::path& aPath )
{
#ifdef __WXMSW__
    return wxString( aPath.wstring() );
#else
    std::u8string utf8 = aPath.u8string();
    return wxString::FromUTF8( reinterpret_cast<const char*>( utf8.data() ), utf8.size() );
#endif
}


bool containsBoardFile( const std::filesystem::path& aTarget, const wxString& aBoardFile )
{
    namespace fs = std::filesystem;

    if( aBoardFile.IsEmpty() )
        return false;

    fs::path        file = toFsPath( aBoardFile );
    std::error_code ec;

    if( !fs::exists( file, ec ) )
        return bool( ec );

    for( fs::path dir = file.parent_path(); !dir.empty(); dir = dir.parent_path() )
    {
        if( fs::equivalent( aTarget, dir, ec ) )
            return true;

        if( ec )
            return true;

        if( dir == dir.root_path() )
            break;
    }

    return false;
}


wxString odbProductBase( const JOB_EXPORT_PCB_ODB& aJob, const BOARD* aBoard )
{
    if( !aJob.m_productName.IsEmpty() )
    {
        wxString requested = ODB::GenLegalEntityName( aJob.m_productName );

        if( !requested.IsEmpty() )
            return requested;
    }

    wxString boardName = wxFileName( aBoard->GetFileName() ).GetName();

    if( boardName.IsEmpty() )
        boardName = wxFileName( aJob.m_filename ).GetName();

    wxString product = ODB::GenLegalEntityName( boardName );

    if( product.IsEmpty() )
        product = wxS( "odb" );

    return product;
}


std::map<std::string, UTF8> odbJobProperties( const JOB_EXPORT_PCB_ODB& aJob, const BOARD* aBoard,
                                              const wxString& aVariant, const wxString& aProductName )
{
    std::map<std::string, UTF8> props;
    props["units"] = aJob.m_units == JOB_EXPORT_PCB_FAB::UNITS::MM ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );
    props["variant"] = aVariant.ToUTF8().data();
    props["mpn"] = aJob.m_colMfgPn.ToUTF8().data();
    props["origin"] = aJob.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::AUX    ? "aux"
                      : aJob.m_origin == JOB_EXPORT_PCB_ODB::ORIGIN::GRID ? "grid"
                                                                          : "absolute";
    props["net_names"] = aJob.m_netNamePolicy == wxS( "anonymize" ) ? "anonymize" : "include";

    switch( aJob.m_dataSet )
    {
    case JOB_EXPORT_PCB_ODB::DATA_SET::FABRICATION: props["data_set"] = "fabrication"; break;
    case JOB_EXPORT_PCB_ODB::DATA_SET::ASSEMBLY: props["data_set"] = "assembly"; break;
    case JOB_EXPORT_PCB_ODB::DATA_SET::TEST: props["data_set"] = "test"; break;
    case JOB_EXPORT_PCB_ODB::DATA_SET::STACKUP: props["data_set"] = "stackup"; break;
    default: props["data_set"] = "all"; break;
    }

    props["sections"] = aJob.m_sections.ToUTF8().data();
    props["layers"] = nlohmann::json( aJob.m_layerOverrides ).dump();

    wxString product = aProductName.IsEmpty() ? odbProductBase( aJob, aBoard ) : aProductName;

    if( !aProductName.IsEmpty() || !aJob.m_productName.IsEmpty() )
        props["product_model_name"] = product.ToUTF8().data();

    return props;
}


bool canReplaceOdbDirectory( const wxString& aTarget, const wxString& aBoardFile,
                             const wxString& aJobFile, wxString& aError )
{
    namespace fs = std::filesystem;
    fs::path        target = toFsPath( aTarget );
    std::error_code ec;

    if( !fs::exists( target, ec ) )
    {
        if( ec )
            aError = wxString::FromUTF8( ec.message() );

        return !ec;
    }

    if( !fs::is_directory( target, ec ) )
    {
        aError = ec ? wxString::FromUTF8( ec.message() ) : _( "Output path is not a directory" );
        return false;
    }

    bool empty = fs::is_empty( target, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    if( empty )
        return true;

    if( !fs::is_regular_file( target / "matrix" / "matrix", ec ) )
    {
        aError = _( "Output directory is not an ODB++ product" );

        if( ec )
            aError += wxS( "\n" ) + wxString::FromUTF8( ec.message() );

        return false;
    }

    static const std::array<fs::path, 9> allowed = {
        fs::path( "ext" ), fs::path( "fonts" ), fs::path( "input" ), fs::path( "matrix" ), fs::path( "misc" ),
        fs::path( "steps" ), fs::path( "symbols" ), fs::path( "user" ), fs::path( "wheels" )
    };

    fs::directory_iterator child( target, ec );
    fs::directory_iterator end;

    while( !ec && child != end )
    {
        fs::path name = child->path().filename();

        if( std::find( allowed.begin(), allowed.end(), name ) == allowed.end() )
        {
            aError = wxString::Format( _( "Output directory contains unrelated entry '%s'" ), fromFsPath( name ) );
            return false;
        }

        child.increment( ec );
    }

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    if( containsBoardFile( target, aBoardFile ) || containsBoardFile( target, aJobFile ) )
    {
        aError = _( "Output directory contains the source board" );
        return false;
    }

    fs::recursive_directory_iterator nested( target, ec );
    fs::recursive_directory_iterator nestedEnd;

    while( !ec && nested != nestedEnd )
    {
        bool regular = nested->is_regular_file( ec );

        if( ec )
        {
            aError = wxString::FromUTF8( ec.message() );
            return false;
        }

        if( regular )
        {
            wxString extension = fromFsPath( nested->path().extension() ).Lower();

            if( extension.StartsWith( wxS( ".kicad_" ) ) || extension == wxS( ".pro" )
                || extension == wxS( ".sch" ) || extension == wxS( ".pcb" ) )
            {
                aError = wxString::Format( _( "Output directory contains project file '%s'" ),
                                            fromFsPath( nested->path().filename() ) );
                return false;
            }
        }

        nested.increment( ec );
    }

    if( ec )
        aError = wxString::FromUTF8( ec.message() );

    return !ec;
}


bool CommitOdbDirectory( const wxString& aSource, const wxString& aTarget,
                         const wxString& aBoardFile, const wxString& aJobFile,
                         REPORTER* aReporter, wxString& aError )
{
    namespace fs = std::filesystem;
    fs::path        source = toFsPath( aSource );
    fs::path        target = toFsPath( aTarget );
    std::error_code ec;

    if( !canReplaceOdbDirectory( aTarget, aBoardFile, aJobFile, aError ) )
        return false;

    if( !fs::exists( target, ec ) )
    {
        if( ec )
        {
            aError = wxString::FromUTF8( ec.message() );
            return false;
        }

        fs::rename( source, target, ec );

        if( ec )
            aError = wxString::FromUTF8( ec.message() );

        return !ec;
    }

    wxString backup = wxFileName::CreateTempFileName( aTarget + wxS( ".kicad-backup-" ) );

    if( backup.IsEmpty() || !wxRemoveFile( backup ) )
    {
        aError = _( "Cannot create temporary backup path" );
        return false;
    }

    fs::path backupPath = toFsPath( backup );
    fs::rename( target, backupPath, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    fs::rename( source, target, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        std::error_code restoreError;
        fs::rename( backupPath, target, restoreError );

        if( restoreError )
            aError += wxS( "\n" ) + wxString::FromUTF8( restoreError.message() );

        return false;
    }

    fs::remove_all( backupPath, ec );

    if( ec && aReporter )
    {
        aReporter->Report( wxString::Format( _( "Cannot remove previous ODB++ directory '%s': %s" ),
                                             backup, wxString::FromUTF8( ec.message() ) ), RPT_SEVERITY_WARNING );
    }

    return true;
}


} // namespace


bool OdbVariantFileTokensCollide( const std::vector<wxString>&                      aTokens,
                                  const std::function<wxString( const wxString& )>& aLocaleFold )
{
    std::set<wxString> localeFolded;
    std::set<wxString> asciiFolded;

    for( const wxString& token : aTokens )
    {
        wxString ascii = token;

        for( size_t ii = 0; ii < ascii.size(); ++ii )
        {
            wxUniChar character = ascii[ii];

            if( character >= 'A' && character <= 'Z' )
                ascii[ii] = static_cast<wxChar>( character.GetValue() - 'A' + 'a' );
        }

        bool newLocale = localeFolded.insert( aLocaleFold( token ) ).second;
        bool newAscii = asciiFolded.insert( ascii ).second;

        if( !newLocale || !newAscii )
            return true;
    }

    return false;
}


static ODB_EXPORT_RESULT generateOneODBPackage( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                                                PCB_EDIT_FRAME* aParentFrame,
                                                PROGRESS_REPORTER* aProgressReporter, REPORTER* aReporter,
                                                wxString outputPath, const wxString& aVariant,
                                                const wxString& aProductName )
{
    LOCALE_IO         toggle;
    ODB_EXPORT_RESULT result;

    if( !aBoard )
    {
        if( aReporter )
            aReporter->Report( _( "No board for ODB++ export." ), RPT_SEVERITY_ERROR );

        return result;
    }

    if( outputPath.IsEmpty() )
        outputPath = wxFileName( aJob.m_filename ).GetPath();

    bool compressed = aJob.m_compressionMode != JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;

    if( !compressed && !outputPath.IsEmpty() )
    {
        std::filesystem::path directory = toFsPath( outputPath );

        if( directory.filename().empty() && directory != directory.root_path() )
            outputPath = fromFsPath( directory.parent_path() );
    }

    wxFileName outputFn( outputPath );
    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( outputFn );

    if( outputFn.GetPath().IsEmpty() && outputFn.HasName() )
        outputFn.MakeAbsolute();

    wxString msg;

    if( !PATHS::EnsurePathExists( outputFn.GetFullPath(), true ) )
    {
        msg.Printf( _( "Cannot create output directory '%s'." ), outputFn.GetFullPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return result;
    }

    if( outputFn.IsDir() && !outputFn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), outputFn.GetPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return result;
    }

    if( compressed )
    {
        bool writable = outputFn.FileExists() ? outputFn.IsFileWritable() : outputFn.IsDirWritable();

        if( !writable )
        {
            msg.Printf( _( "Insufficient permissions to save file '%s'." ), outputFn.GetFullPath() );

            if( aReporter )
                aReporter->Report( msg, RPT_SEVERITY_ERROR );

            return result;
        }

        if( outputFn.Exists() && aParentFrame )
        {
            msg = wxString::Format( _( "Output file '%s' already exists. Do you want to overwrite it?" ),
                                    outputFn.GetFullPath() );
            KIDIALOG confirm( aParentFrame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            confirm.SetOKLabel( _( "Overwrite" ) );

            if( confirm.ShowModal() != wxID_OK )
                return result;
        }
    }
    else
    {
        wxString replaceError;

        if( !canReplaceOdbDirectory( outputFn.GetFullPath(), aBoard->GetFileName(),
                                     aJob.m_filename, replaceError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot use ODB++ directory '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), replaceError ), RPT_SEVERITY_ERROR );
            }

            return result;
        }

        wxDir existing( outputFn.GetFullPath() );

        if( existing.IsOpened() && ( existing.HasFiles() || existing.HasSubDirs() ) && aParentFrame )
        {
            msg = wxString::Format( _( "Output directory '%s' already exists and is not empty. "
                                       "Do you want to overwrite it?" ),
                                    outputFn.GetFullPath() );
            KIDIALOG confirm( aParentFrame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            confirm.SetOKLabel( _( "Overwrite" ) );

            if( confirm.ShowModal() != wxID_OK )
                return result;
        }
    }

    TEMP_ODB_DIRECTORY temporary;
    wxString           prefix = compressed ? wxString( wxS( "kicad-odb" ) )
                                           : outputFn.GetFullPath() + wxS( ".kicad-temp-" );

    if( !temporary.Create( prefix ) )
    {
        if( aReporter )
            aReporter->Report( _( "Cannot create temporary output directory." ), RPT_SEVERITY_ERROR );

        return result;
    }

    wxString treePath = temporary.Path();

    std::map<std::string, UTF8> props = odbJobProperties( aJob, aBoard, aVariant, aProductName );
    wxString                    product = aProductName.IsEmpty() ? odbProductBase( aJob, aBoard ) : aProductName;

    auto saveFile = [&]() -> bool
    {
        try
        {
            IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::ODBPP ) );
            plugin->SetReporter( aReporter );
            plugin->SetProgressReporter( aProgressReporter );
            plugin->SaveBoard( treePath, *aBoard, &props );
            return true;
        }
        catch( const IO_ERROR& error )
        {
            if( aReporter )
            {
                msg = wxString::Format( _( "Error generating ODB++ files '%s'.\n%s" ), treePath, error.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            return false;
        }
    };

    thread_pool&       pool = GetKiCadThreadPool();
    auto               future = pool.submit_task( saveFile );
    std::future_status status = future.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        if( aProgressReporter )
            aProgressReporter->KeepRefreshing();

        status = future.wait_for( std::chrono::milliseconds( 250 ) );
    }

    try
    {
        if( !future.get() )
            return result;
    }
    catch( const std::exception& error )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Exception in ODB++ generation: %s" ), error.what() ),
                               RPT_SEVERITY_ERROR );
        }

        return result;
    }

    if( compressed )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Compressing output" ) );

        ARCHIVE_FORMAT format = aJob.m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ
                                ? ARCHIVE_FORMAT::TGZ : ARCHIVE_FORMAT::ZIP;
        wxString archiveError;

        if( !WriteDirectoryArchive( treePath, outputFn.GetFullPath(), format, product, &archiveError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot write ODB++ archive '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), archiveError ),
                                   RPT_SEVERITY_ERROR );
            }

            return result;
        }
    }
    else
    {
        wxString commitError;

        if( !CommitOdbDirectory( treePath, outputFn.GetFullPath(), aBoard->GetFileName(),
                                 aJob.m_filename, aReporter, commitError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot replace ODB++ directory '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), commitError ), RPT_SEVERITY_ERROR );
            }

            return result;
        }
    }

    if( aProgressReporter )
        aProgressReporter->SetCurrentProgress( 1 );

    result.m_ok = true;
    result.m_outputs.push_back( outputFn.GetFullPath() );
    return result;
}


ODB_EXPORT_RESULT GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard, PCB_EDIT_FRAME* aParentFrame,
                                      PROGRESS_REPORTER* aProgressReporter, REPORTER* aReporter )
{
    ODB_EXPORT_RESULT result;

    if( !aBoard )
        return generateOneODBPackage( aJob, aBoard, aParentFrame, aProgressReporter, aReporter,
                                      wxString(), wxString(), wxString() );

    std::vector<wxString> variants = aJob.m_variantNames.empty()
                                     ? std::vector<wxString>{ aBoard->GetCurrentVariant() } : aJob.m_variantNames;
    wxString rawPath = aJob.GetWorkingOutputPath().IsEmpty() ? aJob.GetConfiguredOutputPath()
                                                            : aJob.GetWorkingOutputPath();
    bool separate = aJob.m_variantPackaging == JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::SEPARATE
                    && variants.size() > 1;
    std::set<wxString> productNames;
    std::vector<wxString> separateProducts;
    std::vector<wxString> separateTokens;
    ODB::VARIANT_NAMES legalNames = ODB::VARIANT_NAMES::Build( aBoard->GetVariantNames() );
    wxString productBase = odbProductBase( aJob, aBoard );

    if( aReporter && !legalNames.m_renamed.empty() )
    {
        wxString renamed;

        for( const wxString& name : legalNames.m_renamed )
            renamed += wxString::Format( wxS( " '%s'" ), name );

        aReporter->Report( wxString::Format( _( "ODB++ variant names adjusted after conversion:%s" ), renamed ),
                           RPT_SEVERITY_WARNING );
    }

    if( aReporter && !legalNames.m_listsFit )
        aReporter->Report( _( "ODB++ variant list exceeds 1000 characters; list attributes omitted." ),
                           RPT_SEVERITY_WARNING );

    for( wxString& variant : variants )
    {
        if( variant.CmpNoCase( GetDefaultVariantName() ) == 0 )
            variant.clear();

        if( !variant.IsEmpty() && !aBoard->HasVariant( variant ) )
        {
            if( aReporter )
                aReporter->Report( wxString::Format( _( "Unknown board variant '%s'." ), variant ),
                                   RPT_SEVERITY_ERROR );

            return result;
        }

        for( const wxString& boardVariant : aBoard->GetVariantNames() )
        {
            if( variant.CmpNoCase( boardVariant ) == 0 )
            {
                variant = boardVariant;
                break;
            }
        }

        if( separate )
        {
            wxString token = GetVariantFileToken( variant );

            if( token.IsEmpty() )
                token = wxS( "default" );

            separateTokens.push_back( token );

            wxString legal = variant.IsEmpty() ? wxString( wxS( "default" ) )
                                                : legalNames.LegalName( variant );
            wxString suffix = legal.Left( 62 );
            auto productFor = [&]( const wxString& aSuffix )
            {
                return productBase.Left( 63 - aSuffix.length() ) + wxS( "_" ) + aSuffix;
            };
            wxString product = productFor( suffix );
            int      duplicate = 2;

            while( !productNames.insert( product ).second )
            {
                wxString number = wxString::Format( wxS( "_%d" ), duplicate++ );
                suffix = legal.Left( 62 - number.length() ) + number;
                product = productFor( suffix );
            }

            separateProducts.push_back( product );
        }
    }

    if( separate
        && OdbVariantFileTokensCollide( separateTokens,
                                        []( const wxString& aToken )
                                        {
                                            return aToken.Lower();
                                        } ) )
    {
        if( aReporter )
            aReporter->Report( _( "ODB++ variant output names collide after conversion." ), RPT_SEVERITY_ERROR );

        return result;
    }

    if( !separate )
    {
        wxString path = aJob.ResolveOutputPath( ExpandVariantOutputPath( rawPath,
                                                                        GetVariantFileToken( variants.front() ) ),
                                               aJob.GetOutputPathIsDirectory(), aBoard->GetProject() );
        return generateOneODBPackage( aJob, aBoard, aParentFrame, aProgressReporter, aReporter,
                                      path, variants.front(), wxString() );
    }

    if( rawPath.IsEmpty() )
        rawPath = wxS( "odb" );

    for( size_t index = 0; index < variants.size(); ++index )
    {
        const wxString& variant = variants[index];
        const wxString& fileToken = separateTokens[index];

        wxString path = rawPath;

        if( path.Contains( wxS( "${VARIANT}" ) ) )
        {
            path = ExpandVariantOutputPath( path, fileToken );
        }
        else
        {
            while( !path.IsEmpty() && wxFileName::IsPathSeparator( path.Last() ) )
                path.RemoveLast();

            wxFileName outputFn( path );
            outputFn.SetName( outputFn.GetName() + wxS( "-" ) + fileToken );
            path = outputFn.GetFullPath();
        }

        path = aJob.ResolveOutputPath( path, aJob.GetOutputPathIsDirectory(), aBoard->GetProject() );
        ODB_EXPORT_RESULT one = generateOneODBPackage( aJob, aBoard, aParentFrame, aProgressReporter,
                                                       aReporter, path, variant, separateProducts[index] );
        result.m_outputs.insert( result.m_outputs.end(), one.m_outputs.begin(), one.m_outputs.end() );

        if( !one.m_ok )
            return result;
    }

    result.m_ok = true;
    return result;
}


wxString UpdateOdbVariantOutputPath( const wxString& aPath, bool aDirectory, bool aSeparate,
                                     bool aRemoveAutomaticSuffix )
{
    if( aPath.IsEmpty() )
        return aPath;

    const wxString suffix = wxS( "-${VARIANT}" );

    if( aDirectory )
    {
        wxString component = aPath;

        while( !component.IsEmpty() && wxFileName::IsPathSeparator( component.Last() ) )
            component.RemoveLast();

        wxString separators = aPath.Mid( component.length() );

        if( aRemoveAutomaticSuffix && component.EndsWith( suffix ) )
            component.RemoveLast( suffix.length() );

        if( aSeparate && !component.EndsWith( suffix ) )
            component += suffix;

        return component + ( separators.IsEmpty() ? wxString( wxFileName::GetPathSeparator() ) : separators );
    }

    int      separator = std::max( aPath.Find( '/', true ), aPath.Find( '\\', true ) );
    int      dot = aPath.Find( '.', true );
    size_t   stemEnd = dot > separator ? static_cast<size_t>( dot ) : aPath.length();
    wxString stem = aPath.Left( stemEnd );

    if( aRemoveAutomaticSuffix && stem.EndsWith( suffix ) )
        stem.RemoveLast( suffix.length() );

    if( aSeparate && !stem.EndsWith( suffix ) )
        stem += suffix;

    return stem + aPath.Mid( stemEnd );
}


IPC2581::SECTION_SET OdbDefaultSections( IPC2581::MODE aMode )
{
    IPC2581::SECTION_SET defaults;

    if( aMode == IPC2581::MODE::USERDEF )
    {
        defaults.set();
        defaults.Set( IPC2581::SECTION::DFX, false );
    }
    else
    {
        defaults = IPC2581::RequiredSections( aMode ) | IPC2581::RecommendedOptionalSections( aMode );
    }

    return defaults;
}


wxString OdbSectionKeyForSelection( IPC2581::MODE aMode, const IPC2581::SECTION_SET& aSelection )
{
    if( aSelection == OdbDefaultSections( aMode ) )
        return wxString();

    return IPC2581::SectionKeyString( aSelection );
}


wxString OdbPreviewReferenceName( const wxString& aReference, const std::vector<ODB_MATRIX_PREVIEW_ROW>& aRows )
{
    auto reference = std::find_if( aRows.begin(), aRows.end(),
                                   [&]( const ODB_MATRIX_PREVIEW_ROW& aCandidate )
                                   {
                                       return aCandidate.m_matrix.m_id == aReference;
                                   } );

    return reference == aRows.end() ? aReference : reference->m_matrix.m_name;
}


std::vector<ODB_MATRIX_PREVIEW_ROW> PreviewOdbMatrix( BOARD* aBoard, const JOB_EXPORT_PCB_ODB& aJob )
{
    if( !aBoard )
        return {};

    std::map<std::string, UTF8> props = odbJobProperties( aJob, aBoard, aBoard->GetCurrentVariant(), wxString() );
    props["layers"] = "[]";
    PCB_IO_ODBPP plugin;
    plugin.ConfigureExport( *aBoard, &props );
    ODB_MATRIX_ENTITY matrix( aBoard, &plugin );
    matrix.InitEntityData();
    std::vector<ODB_MATRIX_PREVIEW_ROW> preview;
    preview.reserve( matrix.GetMatrixLayers().size() );

    for( const ODB_MATRIX_ENTITY::MATRIX_LAYER& layer : matrix.GetMatrixLayers() )
    {
        ODB_MATRIX_PREVIEW_ROW row;
        row.m_matrix.m_row = static_cast<int>( layer.m_rowNumber );
        row.m_matrix.m_name = layer.m_layerName;
        row.m_matrix.m_type = wxString::FromUTF8( ODB::Enum2String( layer.m_type ) );
        row.m_matrix.m_context = wxString::FromUTF8( ODB::Enum2String( layer.m_context ) );
        row.m_matrix.m_polarity = wxString::FromUTF8( ODB::Enum2String( layer.m_polarity ) );
        row.m_matrix.m_id = wxString::Format( "%u", layer.m_uid );

        if( layer.m_span )
        {
            row.m_matrix.m_startName = layer.m_span->first;
            row.m_matrix.m_endName = layer.m_span->second;
        }

        if( layer.m_addType )
            row.m_matrix.m_addType = wxString::FromUTF8( ODB::Enum2String( *layer.m_addType ) );

        if( layer.m_diType )
            row.m_matrix.m_dielectricType = wxString::FromUTF8( ODB::Enum2String( *layer.m_diType ) );

        if( layer.m_ref )
            row.m_matrix.m_ref = wxString::Format( "%u", *layer.m_ref );

        if( layer.m_cuTop )
            row.m_matrix.m_cuTop = wxString::Format( "%u", *layer.m_cuTop );

        if( layer.m_cuBottom )
            row.m_matrix.m_cuBottom = wxString::Format( "%u", *layer.m_cuBottom );

        row.m_boardLayer = layer.m_info.m_layer;
        row.m_editable = layer.m_info.m_role == ODB_LAYER_ROLE::BOARD_LAYER && row.m_boardLayer != UNDEFINED_LAYER;
        row.m_displayLayer = row.m_editable ? aBoard->GetLayerName( row.m_boardLayer )
                                            : wxString::Format( "(%s)", row.m_matrix.m_type.Lower() );
        preview.push_back( std::move( row ) );
    }

    return preview;
}
