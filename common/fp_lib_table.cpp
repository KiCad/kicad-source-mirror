/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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


#include <kiface_base.h>
#include <env_vars.h>
#include <footprint_info.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <paths.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <systemdirsappend.h>
#include <fp_lib_table.h>
#include <footprint.h>

#include <wx/dir.h>
#include <wx/hash.h>
#include <locale_io.h>

#define OPT_SEP     '|'         ///< options separator character

using namespace LIB_TABLE_T;


bool FP_LIB_TABLE_ROW::operator==( const FP_LIB_TABLE_ROW& aRow ) const
{
    return LIB_TABLE_ROW::operator == ( aRow ) && type == aRow.type;
}


void FP_LIB_TABLE_ROW::SetType( const wxString& aType )
{
    type = PCB_IO_MGR::EnumFromStr( aType );

    if( PCB_IO_MGR::PCB_FILE_T( -1 ) == type )
        type = PCB_IO_MGR::KICAD_SEXP;

    plugin.reset();
}


bool FP_LIB_TABLE_ROW::LibraryExists() const
{
    if( plugin )
        return plugin->CanReadLibrary( GetFullURI( true ) );

    return false;
}


FP_LIB_TABLE::FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable ) :
    LIB_TABLE( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


void FP_LIB_TABLE::Parse( LIB_TABLE_LEXER* in )
{
    T        tok;
    wxString errMsg;    // to collect error messages

    // This table may be nested within a larger s-expression, or not.
    // Allow for parser of that optional containing s-expression to have looked ahead.
    if( in->CurTok() != T_fp_lib_table )
    {
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_fp_lib_table )
            in->Expecting( T_fp_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        std::unique_ptr<FP_LIB_TABLE_ROW> row = std::make_unique<FP_LIB_TABLE_ROW>();

        if( tok == T_EOF )
            in->Expecting( T_RIGHT );

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        // in case there is a "row integrity" error, tell where later.
        int lineNum = in->CurLineNumber();
        tok = in->NextTok();

        // Optionally parse the current version number
        if( tok == T_version )
        {
            in->NeedNUMBER( "version" );
            m_version = std::stoi( in->CurText() );
            in->NeedRIGHT();
            continue;
        }

        if( tok != T_lib )
            in->Expecting( T_lib );

        // (name NICKNAME)
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_name )
            in->Expecting( T_name );

        in->NeedSYMBOLorNUMBER();

        row->SetNickName( in->FromUTF8() );

        in->NeedRIGHT();

        // After (name), remaining (lib) elements are order independent, and in
        // some cases optional.
        bool    sawType     = false;
        bool    sawOpts     = false;
        bool    sawDesc     = false;
        bool    sawUri      = false;
        bool    sawDisabled = false;

        while( ( tok = in->NextTok() ) != T_RIGHT )
        {
            if( tok == T_EOF )
                in->Unexpected( T_EOF );

            if( tok != T_LEFT )
                in->Expecting( T_LEFT );

            tok = in->NeedSYMBOLorNUMBER();

            switch( tok )
            {
            case T_uri:
                if( sawUri )
                    in->Duplicate( tok );

                sawUri = true;
                in->NeedSYMBOLorNUMBER();
                row->SetFullURI( in->FromUTF8() );
                break;

            case T_type:
                if( sawType )
                    in->Duplicate( tok );

                sawType = true;
                in->NeedSYMBOLorNUMBER();
                row->SetType( in->FromUTF8() );
                break;

            case T_options:
                if( sawOpts )
                    in->Duplicate( tok );

                sawOpts = true;
                in->NeedSYMBOLorNUMBER();
                row->SetOptions( in->FromUTF8() );
                break;

            case T_descr:
                if( sawDesc )
                    in->Duplicate( tok );

                sawDesc = true;
                in->NeedSYMBOLorNUMBER();
                row->SetDescr( in->FromUTF8() );
                break;

            case T_disabled:
                if( sawDisabled )
                    in->Duplicate( tok );

                sawDisabled = true;
                row->SetEnabled( false );
                break;

            case T_hidden:
                // Hiding footprint libraries is not yet supported.  Unclear what path can set this
                // attribute, but clear it on load.
                row->SetVisible();
                break;

            default:
                in->Unexpected( tok );
            }

            in->NeedRIGHT();
        }

        if( !sawType )
            in->Expecting( T_type );

        if( !sawUri )
            in->Expecting( T_uri );

        // All nickNames within this table fragment must be unique, so we do not use doReplace
        // in doInsertRow().  (However a fallBack table can have a conflicting nickName and ours
        // will supersede that one since in FindLib() we search this table before any fall back.)
        wxString       nickname = row->GetNickName();   // store it to be able to used it
                                                        // after row deletion if an error occurs
        bool           doReplace = false;
        LIB_TABLE_ROW* tmp = row.release();

        if( !doInsertRow( tmp, doReplace ) )
        {
            delete tmp;     // The table did not take ownership of the row.

            wxString msg = wxString::Format( _( "Duplicate library nickname '%s' found in "
                                                "footprint library table file line %d." ),
                                             nickname,
                                             lineNum );

            if( !errMsg.IsEmpty() )
                errMsg << '\n';

            errMsg << msg;
        }
    }

    if( !errMsg.IsEmpty() )
        THROW_IO_ERROR( errMsg );
}


bool FP_LIB_TABLE::operator==( const FP_LIB_TABLE& aFpTable ) const
{
    if( m_rows.size() == aFpTable.m_rows.size() )
    {
        for( unsigned i = 0; i < m_rows.size();  ++i )
        {
            if( (FP_LIB_TABLE_ROW&)m_rows[i] != (FP_LIB_TABLE_ROW&)aFpTable.m_rows[i] )
                return false;
        }

        return true;
    }

    return false;
}


void FP_LIB_TABLE::Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const
{
    aOutput->Print( aIndentLevel, "(fp_lib_table\n" );
    aOutput->Print( aIndentLevel + 1, "(version %d)\n", m_version );

    for( LIB_TABLE_ROWS_CITER it = m_rows.begin();  it != m_rows.end();  ++it )
        it->Format( aOutput, aIndentLevel+1 );

    aOutput->Print( aIndentLevel, ")\n" );
}


long long FP_LIB_TABLE::GenerateTimestamp( const wxString* aNickname )
{
    long long hash = 0;

    if( aNickname )
    {
        const FP_LIB_TABLE_ROW* row = FindRow( *aNickname, true );

        wxCHECK( row && row->plugin, hash );

        return row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) ) +
                wxHashTable::MakeKey( *aNickname );
    }

    for( const wxString& nickname : GetLogicalLibs() )
    {
        const FP_LIB_TABLE_ROW* row = nullptr;

        try
        {
            row = FindRow( nickname, true );
        }
        catch( ... )
        {
            // Do nothing if not found: just skip.
        }

        wxCHECK2( row && row->plugin, continue );

        hash += row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) ) +
                wxHashTable::MakeKey( nickname );
    }

    return hash;
}


void FP_LIB_TABLE::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aNickname,
                                       bool aBestEfforts, const LOCALE_IO* aLocale )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    if( !aLocale )
    {
        LOCALE_IO toggle_locale;

        row->plugin->FootprintEnumerate( aFootprintNames, row->GetFullURI( true ), aBestEfforts,
                                         row->GetProperties() );
    }
    else
    {
        row->plugin->FootprintEnumerate( aFootprintNames, row->GetFullURI( true ), aBestEfforts,
                                         row->GetProperties() );
    }
}


const FP_LIB_TABLE_ROW* FP_LIB_TABLE::FindRow( const wxString& aNickname, bool aCheckIfEnabled )
{
    FP_LIB_TABLE_ROW* row = static_cast<FP_LIB_TABLE_ROW*>( findRow( aNickname, aCheckIfEnabled ) );

    if( !row )
    {
        // We don't generally show this string to the user (who is unlikely to know what
        // "fp-lib-table" means), and translating it may produce Sentry KICAD-YP.
        wxString msg = wxString::Format( wxS( "'%s' not found in fp-lib-table." ), aNickname );
        THROW_IO_ERROR( msg );
    }

    // We've been 'lazy' up until now, but it cannot be deferred any longer; instantiate a
    // PCB_IO of the proper kind if it is not already in this FP_LIB_TABLE_ROW.
    if( !row->plugin )
        row->setPlugin( PCB_IO_MGR::PluginFind( row->type ) );

    return row;
}


static void setLibNickname( FOOTPRINT* aModule, const wxString& aNickname,
                            const wxString& aFootprintName )
{
    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a footprint library.
    // Only at this API layer can we tell the footprint about its actual library nickname.
    if( aModule )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the LIB_ID and its two strings, twice each.
        LIB_ID& fpid = (LIB_ID&) aModule->GetFPID();

        // Catch any misbehaving plugin, which should be setting internal footprint name properly:
        wxASSERT( aFootprintName == fpid.GetLibItemName().wx_str() );

        // and clearing nickname
        wxASSERT( !fpid.GetLibNickname().size() );

        fpid.SetLibNickname( aNickname );
    }
}


const FOOTPRINT* FP_LIB_TABLE::GetEnumeratedFootprint( const wxString& aNickname,
                                                       const wxString& aFootprintName,
                                                       const LOCALE_IO* aLocale )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    if( !aLocale )
    {
        LOCALE_IO toggle_locale;

        return row->plugin->GetEnumeratedFootprint( row->GetFullURI( true ), aFootprintName,
                                                    row->GetProperties() );
    }
    else
    {
        return row->plugin->GetEnumeratedFootprint( row->GetFullURI( true ), aFootprintName,
                                                    row->GetProperties() );
    }
}


bool FP_LIB_TABLE::FootprintExists( const wxString& aNickname, const wxString& aFootprintName )
{
    // NOT THREAD-SAFE!  LOCALE_IO is global!

    LOCALE_IO toggle_locale;

    try
    {
        const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
        wxASSERT( row->plugin );

        return row->plugin->FootprintExists( row->GetFullURI( true ), aFootprintName,
                                             row->GetProperties() );
    }
    catch( ... )
    {
        return false;
    }
}


FOOTPRINT* FP_LIB_TABLE::FootprintLoad( const wxString& aNickname,
                                        const wxString& aFootprintName, bool aKeepUUID )
{
    // NOT THREAD-SAFE!  LOCALE_IO is global!

    LOCALE_IO toggle_locale;

    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    FOOTPRINT* ret = row->plugin->FootprintLoad( row->GetFullURI( true ), aFootprintName,
                                                 aKeepUUID, row->GetProperties() );

    setLibNickname( ret, row->GetNickName(), aFootprintName );

    return ret;
}


FP_LIB_TABLE::SAVE_T FP_LIB_TABLE::FootprintSave( const wxString& aNickname,
                                                  const FOOTPRINT* aFootprint, bool aOverwrite )
{
    // NOT THREAD-SAFE!  LOCALE_IO is global!

    LOCALE_IO toggle_locale;

    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString fpname = aFootprint->GetFPID().GetLibItemName();

        std::unique_ptr<FOOTPRINT> footprint( row->plugin->FootprintLoad( row->GetFullURI( true ),
                                                                          fpname,
                                                                          row->GetProperties() ) );

        if( footprint.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->FootprintSave( row->GetFullURI( true ), aFootprint, row->GetProperties() );

    return SAVE_OK;
}


void FP_LIB_TABLE::FootprintDelete( const wxString& aNickname, const wxString& aFootprintName )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    return row->plugin->FootprintDelete( row->GetFullURI( true ), aFootprintName,
                                         row->GetProperties() );
}


bool FP_LIB_TABLE::IsFootprintLibWritable( const wxString& aNickname )
{
    try
    {
        const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );

        if( !row || !row->plugin )
            return false;

        return row->plugin->IsLibraryWritable( row->GetFullURI( true ) );
    }
    catch( ... )
    {
    }

    // aNickname not found, so the library is not writable
    return false;
}


void FP_LIB_TABLE::FootprintLibDelete( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->DeleteLibrary( row->GetFullURI( true ), row->GetProperties() );
}


void FP_LIB_TABLE::FootprintLibCreate( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->CreateLibrary( row->GetFullURI( true ), row->GetProperties() );
}


FOOTPRINT* FP_LIB_TABLE::FootprintLoadWithOptionalNickname( const LIB_ID& aFootprintId,
                                                            bool aKeepUUID )
{
    wxString   nickname = aFootprintId.GetLibNickname();
    wxString   fpname   = aFootprintId.GetLibItemName();

    if( nickname.size() )
    {
        return FootprintLoad( nickname, fpname, aKeepUUID );
    }

    // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
    else
    {
        std::vector<wxString> nicks = GetLogicalLibs();

        // Search each library going through libraries alphabetically.
        for( unsigned i = 0;  i < nicks.size();  ++i )
        {
            // FootprintLoad() returns NULL on not found, does not throw exception
            // unless there's an IO_ERROR.
            FOOTPRINT* ret = FootprintLoad( nicks[i], fpname, aKeepUUID );

            if( ret )
                return ret;
        }

        return nullptr;
    }
}


const wxString FP_LIB_TABLE::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) );
}


class PCM_FP_LIB_TRAVERSER final : public wxDirTraverser
{
public:
    explicit PCM_FP_LIB_TRAVERSER( const wxString& aPath, FP_LIB_TABLE& aTable,
                                   const wxString& aPrefix ) :
            m_lib_table( aTable ),
            m_path_prefix( aPath ),
            m_lib_prefix( aPrefix )
    {
        wxFileName f( aPath, wxS( "" ) );
        m_prefix_dir_count = f.GetDirCount();
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override { return wxDIR_CONTINUE; }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        wxFileName dir = wxFileName::DirName( dirPath );

        // consider a directory to be a lib if it's name ends with .pretty and
        // it is under $KICADn_3RD_PARTY/footprints/<pkgid>/ i.e. has nested level of at least +3
        if( dirPath.EndsWith( wxS( ".pretty" ) ) && dir.GetDirCount() >= m_prefix_dir_count + 3 )
        {
            wxString versionedPath = wxString::Format(
                    wxS( "${%s}" ), ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ) );

            wxArrayString parts = dir.GetDirs();
            parts.RemoveAt( 0, m_prefix_dir_count );
            parts.Insert( versionedPath, 0 );

            wxString libPath = wxJoin( parts, '/' );

            if( !m_lib_table.HasLibraryWithPath( libPath ) )
            {
                wxString name = parts.Last().substr( 0, parts.Last().length() - 7 );
                wxString nickname = wxString::Format( wxS( "%s%s" ), m_lib_prefix, name );

                if( m_lib_table.HasLibrary( nickname ) )
                {
                    int increment = 1;
                    do
                    {
                        nickname = wxString::Format( wxS( "%s%s_%d" ), m_lib_prefix, name,
                                                     increment );
                        increment++;
                    } while( m_lib_table.HasLibrary( nickname ) );
                }

                m_lib_table.InsertRow(
                        new FP_LIB_TABLE_ROW( nickname, libPath, wxT( "KiCad" ), wxEmptyString,
                                              _( "Added by Plugin and Content Manager" ) ) );
            }
        }

        return wxDIR_CONTINUE;
    }

private:
    FP_LIB_TABLE& m_lib_table;
    wxString      m_path_prefix;
    wxString      m_lib_prefix;
    size_t        m_prefix_dir_count;
};


bool FP_LIB_TABLE::LoadGlobalTable( FP_LIB_TABLE& aTable )
{
    bool        tableExists = true;
    wxFileName  fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !wxFileName::DirExists( fn.GetPath() )
            && !wxFileName::Mkdir( fn.GetPath(), 0x777, wxPATH_MKDIR_FULL ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot create global library table path '%s'." ),
                                              fn.GetPath() ) );
        }

        // Attempt to copy the default global file table from the KiCad
        // template folder to the user's home configuration path.
        SEARCH_STACK ss;

        SystemDirsAppend( &ss );

        const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();
        std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( envVars,
                                                                      wxT( "TEMPLATE_DIR" ) );

        if( v && !v->IsEmpty() )
            ss.AddPaths( *v, 0 );

        wxString fileName = ss.FindValidPath( FILEEXT::FootprintLibraryTableFileName );

        // The fallback is to create an empty global footprint table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            FP_LIB_TABLE    emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.Load( fn.GetFullPath() );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
    wxString packagesPath;

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( env, wxT( "3RD_PARTY" ) ) )
        packagesPath = *v;

    if( settings->m_PcmLibAutoAdd )
    {
        // Scan for libraries in PCM packages directory

        wxFileName d( packagesPath, wxS( "" ) );
        d.AppendDir( wxS( "footprints" ) );

        if( d.DirExists() )
        {
            PCM_FP_LIB_TRAVERSER traverser( packagesPath, aTable, settings->m_PcmLibPrefix );
            wxDir                dir( d.GetPath() );

            dir.Traverse( traverser );
        }
    }

    if( settings->m_PcmLibAutoRemove )
    {
        // Remove PCM libraries that no longer exist
        std::vector<wxString> to_remove;

        for( size_t i = 0; i < aTable.GetCount(); i++ )
        {
            LIB_TABLE_ROW& row = aTable.At( i );
            wxString       path = row.GetFullURI( true );

            if( path.StartsWith( packagesPath ) && !wxDir::Exists( path ) )
                to_remove.push_back( row.GetNickName() );
        }

        for( const wxString& nickName : to_remove )
            aTable.RemoveRow( aTable.FindRow( nickName ) );
    }

    return tableExists;
}


wxString FP_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( PATHS::GetUserSettingsPath() );
    fn.SetName( FILEEXT::FootprintLibraryTableFileName );

    return fn.GetFullPath();
}
