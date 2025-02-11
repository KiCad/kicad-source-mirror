/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <env_vars.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <paths.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <systemdirsappend.h>
#include <symbol_lib_table.h>
#include <lib_symbol.h>
#include <sch_io/database/sch_io_database.h>
#include <dialogs/dialog_database_lib_settings.h>

#include <wx/dir.h>
#include "sim/sim_model.h"

#define OPT_SEP     '|'         ///< options separator character

using namespace LIB_TABLE_T;


static const wxString global_tbl_name( "sym-lib-table" );


const char* SYMBOL_LIB_TABLE::PropPowerSymsOnly = "pwr_sym_only";
const char* SYMBOL_LIB_TABLE::PropNonPowerSymsOnly = "non_pwr_sym_only";
int SYMBOL_LIB_TABLE::m_modifyHash = 1;     // starts at 1 and goes up


/// The global symbol library table.  This is not dynamically allocated because
/// in a multiple project environment we must keep its address constant (since it is
/// the fallback table for multiple projects).
SYMBOL_LIB_TABLE    g_symbolLibraryTable;


bool SYMBOL_LIB_TABLE_ROW::operator==( const SYMBOL_LIB_TABLE_ROW& aRow ) const
{
    return LIB_TABLE_ROW::operator == ( aRow ) && type == aRow.type;
}


void SYMBOL_LIB_TABLE_ROW::SetType( const wxString& aType )
{
    type = SCH_IO_MGR::EnumFromStr( aType );

    if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        type = SCH_IO_MGR::SCH_KICAD;

    plugin.release();
}


bool SYMBOL_LIB_TABLE_ROW::Refresh()
{
    if( !plugin )
    {
        wxArrayString dummyList;

        plugin.reset( SCH_IO_MGR::FindPlugin( type ) );
        SetLoaded( false );
        plugin->SetLibTable( static_cast<SYMBOL_LIB_TABLE*>( GetParent() ) );
        plugin->EnumerateSymbolLib( dummyList, GetFullURI( true ), GetProperties() );
        SetLoaded( true );
        return true;
    }

    return false;
}


void SYMBOL_LIB_TABLE_ROW::GetSubLibraryNames( std::vector<wxString>& aNames ) const
{
    if( !plugin )
        return;

    plugin->GetSubLibraryNames( aNames );
}


wxString SYMBOL_LIB_TABLE_ROW::GetSubLibraryDescription( const wxString& aName ) const
{
    if( !plugin )
        return wxEmptyString;

    return plugin->GetSubLibraryDescription( aName );
}


void SYMBOL_LIB_TABLE_ROW::ShowSettingsDialog( wxWindow* aParent ) const
{
    wxCHECK( plugin, /* void */ );

    if( type != SCH_IO_MGR::SCH_DATABASE )
        return;

    DIALOG_DATABASE_LIB_SETTINGS dlg( aParent, static_cast<SCH_IO_DATABASE*>( plugin.get() ) );
    dlg.ShowModal();
}


SYMBOL_LIB_TABLE::SYMBOL_LIB_TABLE( SYMBOL_LIB_TABLE* aFallBackTable ) :
    LIB_TABLE( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


SYMBOL_LIB_TABLE& SYMBOL_LIB_TABLE::GetGlobalLibTable()
{
    return g_symbolLibraryTable;
}


void SYMBOL_LIB_TABLE::Parse( LIB_TABLE_LEXER* in )
{
    T        tok;
    wxString errMsg;    // to collect error messages

    // This table may be nested within a larger s-expression, or not.
    // Allow for parser of that optional containing s-epression to have looked ahead.
    if( in->CurTok() != T_sym_lib_table )
    {
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_sym_lib_table )
            in->Expecting( T_sym_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        std::unique_ptr< SYMBOL_LIB_TABLE_ROW > row = std::make_unique<SYMBOL_LIB_TABLE_ROW>();

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
        bool    sawHidden   = false;

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
                if( sawHidden )
                    in->Duplicate( tok );
                sawHidden = true;
                row->SetVisible( false );
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

        // all nickNames within this table fragment must be unique, so we do not
        // use doReplace in InsertRow().  (However a fallBack table can have a
        // conflicting nickName and ours will supercede that one since in
        // FindLib() we search this table before any fall back.)
        wxString nickname = row->GetNickName(); // store it to be able to used it
                                                // after row deletion if an error occurs
        LIB_TABLE_ROW* tmp = row.release();

        if( !InsertRow( tmp ) )
        {
            delete tmp;     // The table did not take ownership of the row.

            wxString msg = wxString::Format( _( "Duplicate library nickname '%s' found in symbol "
                                                "library table file line %d" ),
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


void SYMBOL_LIB_TABLE::Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const
{
    aOutput->Print( aIndentLevel, "(sym_lib_table\n" );
    aOutput->Print( aIndentLevel + 1, "(version %d)\n", m_version );

    for( const LIB_TABLE_ROW& row : m_rows )
        row.Format( aOutput, aIndentLevel + 1 );

    aOutput->Print( aIndentLevel, ")\n" );
}


int SYMBOL_LIB_TABLE::GetModifyHash()
{
    int                     hash = 0;
    std::vector< wxString > libNames = GetLogicalLibs();

    for( const auto& libName : libNames )
    {
        const SYMBOL_LIB_TABLE_ROW* row = FindRow( libName, true );

        if( !row || !row->plugin )
        {
            continue;
        }

        hash += row->plugin->GetModifyHash();
    }

    hash += m_modifyHash;

    return hash;
}


void SYMBOL_LIB_TABLE::EnumerateSymbolLib( const wxString& aNickname, wxArrayString& aAliasNames,
                                           bool aPowerSymbolsOnly )
{
    SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, /* void */ );

    wxString options = row->GetOptions();

    if( aPowerSymbolsOnly )
        row->SetOptions( row->GetOptions() + " " + PropPowerSymsOnly );

    row->SetLoaded( false );
    row->plugin->EnumerateSymbolLib( aAliasNames, row->GetFullURI( true ), row->GetProperties() );
    row->SetLoaded( true );

    if( aPowerSymbolsOnly )
        row->SetOptions( options );
}


SYMBOL_LIB_TABLE_ROW* SYMBOL_LIB_TABLE::FindRow( const wxString& aNickname, bool aCheckIfEnabled )
{
    SYMBOL_LIB_TABLE_ROW* row =
            dynamic_cast< SYMBOL_LIB_TABLE_ROW* >( findRow( aNickname, aCheckIfEnabled ) );

    if( !row )
        return nullptr;

    // We've been 'lazy' up until now, but it cannot be deferred any longer,
    // instantiate a PLUGIN of the proper kind if it is not already in this
    // SYMBOL_LIB_TABLE_ROW.
    if( !row->plugin )
        row->setPlugin( SCH_IO_MGR::FindPlugin( row->type ) );

    row->plugin->SetLibTable( this );

    return row;
}


void SYMBOL_LIB_TABLE::LoadSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                      const wxString& aNickname, bool aPowerSymbolsOnly )
{
    SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );

    if( !row || !row->plugin )
        return;

    std::lock_guard<std::mutex> lock( row->GetMutex() );

    wxString options = row->GetOptions();

    if( aPowerSymbolsOnly )
        row->SetOptions( row->GetOptions() + " " + PropPowerSymsOnly );

    row->SetLoaded( false );
    row->plugin->SetLibTable( this );
    row->plugin->EnumerateSymbolLib( aSymbolList, row->GetFullURI( true ), row->GetProperties() );
    row->SetLoaded( true );

    if( aPowerSymbolsOnly )
        row->SetOptions( options );

    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a symbol library.
    // Only at this API layer can we tell the symbol about its actual library nickname.
    for( LIB_SYMBOL* symbol : aSymbolList )
    {
        LIB_ID id = symbol->GetLibId();

        id.SetLibNickname( row->GetNickName() );
        symbol->SetLibId( id );
    }
}


LIB_SYMBOL* SYMBOL_LIB_TABLE::LoadSymbol( const wxString& aNickname, const wxString& aSymbolName )
{
    SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );

    if( !row || !row->plugin )
        return nullptr;

    // If another thread is loading this library at the moment; continue
    std::unique_lock<std::mutex> lock( row->GetMutex(), std::try_to_lock );

    if( !lock.owns_lock() )
        return nullptr;

    LIB_SYMBOL* symbol = row->plugin->LoadSymbol( row->GetFullURI( true ), aSymbolName,
                                                  row->GetProperties() );

    if( symbol )
    {
        // The library cannot know its own name, because it might have been renamed or moved.
        // Therefore footprints cannot know their own library nickname when residing in
        // a symbol library.
        // Only at this API layer can we tell the symbol about its actual library nickname.
        LIB_ID id = symbol->GetLibId();

        id.SetLibNickname( row->GetNickName() );
        symbol->SetLibId( id );

        SIM_MODEL::MigrateSimModel<LIB_SYMBOL, LIB_FIELD>( *symbol, nullptr );
    }

    return symbol;
}


SYMBOL_LIB_TABLE::SAVE_T SYMBOL_LIB_TABLE::SaveSymbol( const wxString& aNickname,
                                                       const LIB_SYMBOL* aSymbol, bool aOverwrite )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, SAVE_SKIPPED );

    if( !row->plugin->IsLibraryWritable( row->GetFullURI( true ) ) )
        return SAVE_SKIPPED;

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString name = aSymbol->GetLibId().GetLibItemName();

        std::unique_ptr<LIB_SYMBOL> symbol( row->plugin->LoadSymbol( row->GetFullURI( true ),
                                                                     name, row->GetProperties() ) );

        if( symbol.get() )
            return SAVE_SKIPPED;
    }

    try
    {
        row->plugin->SaveSymbol( row->GetFullURI( true ), aSymbol, row->GetProperties() );
    }
    catch( const IO_ERROR& )
    {
        return SAVE_SKIPPED;
    }

    return SAVE_OK;
}


void SYMBOL_LIB_TABLE::DeleteSymbol( const wxString& aNickname, const wxString& aSymbolName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, /* void */ );
    return row->plugin->DeleteSymbol( row->GetFullURI( true ), aSymbolName, row->GetProperties() );
}


bool SYMBOL_LIB_TABLE::IsSymbolLibWritable( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, false );
    return row->plugin->IsLibraryWritable( row->GetFullURI( true ) );
}

bool SYMBOL_LIB_TABLE::IsSymbolLibLoaded( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row, false );
    return row->GetIsLoaded();
}


void SYMBOL_LIB_TABLE::DeleteSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, /* void */ );
    row->plugin->DeleteLibrary( row->GetFullURI( true ), row->GetProperties() );
}


void SYMBOL_LIB_TABLE::CreateSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxCHECK( row && row->plugin, /* void */ );
    row->plugin->CreateLibrary( row->GetFullURI( true ), row->GetProperties() );
}


LIB_SYMBOL* SYMBOL_LIB_TABLE::LoadSymbolWithOptionalNickname( const LIB_ID& aLibId )
{
    wxString   nickname = aLibId.GetLibNickname();
    wxString   name     = aLibId.GetLibItemName();

    if( nickname.size() )
    {
        return LoadSymbol( nickname, name );
    }
    else
    {
        // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
        std::vector<wxString> nicks = GetLogicalLibs();

        // Search each library going through libraries alphabetically.
        for( unsigned i = 0;  i < nicks.size();  ++i )
        {
            // FootprintLoad() returns NULL on not found, does not throw exception
            // unless there's an IO_ERROR.
            LIB_SYMBOL* ret = LoadSymbol( nicks[i], name );

            if( ret )
                return ret;
        }

        return nullptr;
    }
}


const wxString SYMBOL_LIB_TABLE::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "SYMBOL_DIR" ) );
}


class PCM_SYM_LIB_TRAVERSER final : public wxDirTraverser
{
public:
    explicit PCM_SYM_LIB_TRAVERSER( const wxString& aPath, SYMBOL_LIB_TABLE& aTable,
                                    const wxString& aPrefix ) :
            m_lib_table( aTable ),
            m_path_prefix( aPath ),
            m_lib_prefix( aPrefix )
    {
        wxFileName f( aPath, "" );
        m_prefix_dir_count = f.GetDirCount();
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        wxFileName file = wxFileName::FileName( aFilePath );

        // consider a file to be a lib if it's name ends with .kicad_sym and
        // it is under $KICADn_3RD_PARTY/symbols/<pkgid>/ i.e. has nested level of at least +2
        if( file.GetExt() == wxT( "kicad_sym" ) && file.GetDirCount() >= m_prefix_dir_count + 2 )
        {
            wxString versionedPath = wxString::Format( wxS( "${%s}" ),
                                       ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ) );

            wxArrayString parts = file.GetDirs();
            parts.RemoveAt( 0, m_prefix_dir_count );
            parts.Insert( versionedPath, 0 );
            parts.Add( file.GetFullName() );

            wxString libPath = wxJoin( parts, '/' );

            if( !m_lib_table.HasLibraryWithPath( libPath ) )
            {
                wxString name = parts.Last().substr( 0, parts.Last().length() - 10 );
                wxString nickname = wxString::Format( "%s%s", m_lib_prefix, name );

                if( m_lib_table.HasLibrary( nickname ) )
                {
                    int increment = 1;
                    do
                    {
                        nickname = wxString::Format( "%s%s_%d", m_lib_prefix, name, increment );
                        increment++;
                    } while( m_lib_table.HasLibrary( nickname ) );
                }

                m_lib_table.InsertRow(
                        new SYMBOL_LIB_TABLE_ROW( nickname, libPath, wxT( "KiCad" ), wxEmptyString,
                                                  _( "Added by Plugin and Content Manager" ) ) );
            }
        }

        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override { return wxDIR_CONTINUE; }

private:
    SYMBOL_LIB_TABLE& m_lib_table;
    wxString          m_path_prefix;
    wxString          m_lib_prefix;
    size_t            m_prefix_dir_count;
};


bool SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE& aTable )
{
    bool        tableExists = true;
    wxFileName  fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !fn.DirExists() && !fn.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
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

        wxString fileName = ss.FindValidPath( global_tbl_name );

        // The fallback is to create an empty global symbol table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            SYMBOL_LIB_TABLE    emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.Clear();
    aTable.Load( fn.GetFullPath() );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>();

    wxCHECK( settings, false );

    wxString packagesPath;
    const ENV_VAR_MAP& vars = Pgm().GetLocalEnvVariables();

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( vars, wxT( "3RD_PARTY" ) ) )
        packagesPath = *v;

    if( settings->m_PcmLibAutoAdd )
    {
        // Scan for libraries in PCM packages directory
        wxFileName d( packagesPath, "" );
        d.AppendDir( "symbols" );

        if( d.DirExists() )
        {
            PCM_SYM_LIB_TRAVERSER traverser( packagesPath, aTable, settings->m_PcmLibPrefix );
            wxDir                 dir( d.GetPath() );

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

            if( path.StartsWith( packagesPath ) && !wxFile::Exists( path ) )
                to_remove.push_back( row.GetNickName() );
        }

        for( const wxString& nickName : to_remove )
        {
            SYMBOL_LIB_TABLE_ROW* row = aTable.FindRow( nickName );

            wxCHECK2( row, continue );

            aTable.RemoveRow( row );
        }
    }

    return tableExists;
}


bool SYMBOL_LIB_TABLE::operator==( const SYMBOL_LIB_TABLE& aOther ) const
{
    if( m_rows.size() != aOther.m_rows.size() )
        return false;

    unsigned i;

    for( i = 0; i < m_rows.size(); ++i )
    {
        const SYMBOL_LIB_TABLE_ROW& curr = static_cast<const SYMBOL_LIB_TABLE_ROW&>( m_rows[i] );
        const SYMBOL_LIB_TABLE_ROW& curr_other = static_cast<const SYMBOL_LIB_TABLE_ROW&>( aOther.m_rows[i] );

        if( curr != curr_other )
            return false;
    }

    return true;
}


wxString SYMBOL_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( PATHS::GetUserSettingsPath() );
    fn.SetName( global_tbl_name );

    return fn.GetFullPath();
}


const wxString& SYMBOL_LIB_TABLE::GetSymbolLibTableFileName()
{
    return global_tbl_name;
}
