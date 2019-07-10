/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2019 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <systemdirsappend.h>
#include <symbol_lib_table.h>
#include <class_libentry.h>

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

    if( SCH_IO_MGR::SCH_FILE_T( -1 ) == type )
        type = SCH_IO_MGR::SCH_LEGACY;
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
        std::unique_ptr< SYMBOL_LIB_TABLE_ROW > row( new SYMBOL_LIB_TABLE_ROW );

        if( tok == T_EOF )
            in->Expecting( T_RIGHT );

        if( tok != T_LEFT )
            in->Expecting( T_LEFT );

        // in case there is a "row integrity" error, tell where later.
        int lineNum = in->CurLineNumber();

        if( ( tok = in->NextTok() ) != T_lib )
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

            wxString msg = wxString::Format(
                                _( "Duplicate library nickname \"%s\" found in symbol library "
                                   "table file line %d" ), GetChars( nickname ), lineNum );

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

    for( LIB_TABLE_ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
    {
        it->Format( aOutput, aIndentLevel+1 );
    }

    aOutput->Print( aIndentLevel, ")\n" );
}


int SYMBOL_LIB_TABLE::GetModifyHash()
{
    int                     hash = 0;
    std::vector< wxString > libNames = GetLogicalLibs();

    for( const auto& libName : libNames )
    {
        const SYMBOL_LIB_TABLE_ROW* row = FindRow( libName );

        if( !row || !row->plugin )
        {
            wxFAIL;
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
    SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */ );

    wxString options = row->GetOptions();

    if( aPowerSymbolsOnly )
        row->SetOptions( row->GetOptions() + " " + PropPowerSymsOnly );

    row->plugin->EnumerateSymbolLib( aAliasNames, row->GetFullURI( true ), row->GetProperties() );

    if( aPowerSymbolsOnly )
        row->SetOptions( options );
}


SYMBOL_LIB_TABLE_ROW* SYMBOL_LIB_TABLE::FindRow( const wxString& aNickname )

{
    SYMBOL_LIB_TABLE_ROW* row = dynamic_cast< SYMBOL_LIB_TABLE_ROW* >( findRow( aNickname ) );

    if( !row )
    {
        wxString msg = wxString::Format(
            _( "sym-lib-table files contain no library with nickname \"%s\"" ),
            GetChars( aNickname ) );

        THROW_IO_ERROR( msg );
    }

    // We've been 'lazy' up until now, but it cannot be deferred any longer,
    // instantiate a PLUGIN of the proper kind if it is not already in this
    // SYMBOL_LIB_TABLE_ROW.
    if( !row->plugin )
        row->setPlugin( SCH_IO_MGR::FindPlugin( row->type ) );

    return row;
}


void SYMBOL_LIB_TABLE::LoadSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                                      const wxString& aNickname, bool aPowerSymbolsOnly )
{
    SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */  );

    wxString options = row->GetOptions();

    if( aPowerSymbolsOnly )
        row->SetOptions( row->GetOptions() + " " + PropPowerSymsOnly );

    row->plugin->EnumerateSymbolLib( aAliasList, row->GetFullURI( true ), row->GetProperties() );

    if( aPowerSymbolsOnly )
        row->SetOptions( options );

    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a symbol library.
    // Only at this API layer can we tell the symbol about its actual library nickname.
    for( LIB_ALIAS* alias : aAliasList )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the LIB_ID and its two strings, twice each.
        LIB_ID& id = (LIB_ID&) alias->GetPart()->GetLibId();

        id.SetLibNickname( row->GetNickName() );
    }
}


LIB_ALIAS* SYMBOL_LIB_TABLE::LoadSymbol( const wxString& aNickname, const wxString& aAliasName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, nullptr );

    LIB_ALIAS* ret = row->plugin->LoadSymbol( row->GetFullURI( true ), aAliasName,
                                              row->GetProperties() );

    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a symbol library.
    // Only at this API layer can we tell the symbol about its actual library nickname.
    if( ret )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the LIB_ID and its two strings, twice each.
        LIB_ID& id = (LIB_ID&) ret->GetPart()->GetLibId();

        id.SetLibNickname( row->GetNickName() );
    }

    return ret;
}


SYMBOL_LIB_TABLE::SAVE_T SYMBOL_LIB_TABLE::SaveSymbol( const wxString& aNickname,
                                                       const LIB_PART* aSymbol, bool aOverwrite )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, SAVE_SKIPPED );

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString name = aSymbol->GetLibId().GetLibItemName();

        std::unique_ptr< LIB_ALIAS > symbol( row->plugin->LoadSymbol( row->GetFullURI( true ),
                                                                      name,
                                                                      row->GetProperties() ) );

        if( symbol.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->SaveSymbol( row->GetFullURI( true ), aSymbol, row->GetProperties() );

    return SAVE_OK;
}


void SYMBOL_LIB_TABLE::DeleteSymbol( const wxString& aNickname, const wxString& aSymbolName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */ );
    return row->plugin->DeleteSymbol( row->GetFullURI( true ), aSymbolName,
                                      row->GetProperties() );
}


void SYMBOL_LIB_TABLE::DeleteAlias( const wxString& aNickname, const wxString& aAliasName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */ );
    return row->plugin->DeleteAlias( row->GetFullURI( true ), aAliasName,
                                     row->GetProperties() );
}


bool SYMBOL_LIB_TABLE::IsSymbolLibWritable( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, false );
    return row->plugin->IsSymbolLibWritable( row->GetFullURI( true ) );
}


void SYMBOL_LIB_TABLE::DeleteSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */ );
    row->plugin->DeleteSymbolLib( row->GetFullURI( true ), row->GetProperties() );
}


void SYMBOL_LIB_TABLE::CreateSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxCHECK( row && row->plugin, /* void */ );
    row->plugin->CreateSymbolLib( row->GetFullURI( true ), row->GetProperties() );
}


LIB_ALIAS* SYMBOL_LIB_TABLE::LoadSymbolWithOptionalNickname( const LIB_ID& aLibId )
{
    wxString   nickname = aLibId.GetLibNickname();
    wxString   name     = aLibId.GetLibItemName();

    if( nickname.size() )
    {
        return LoadSymbol( nickname, name );
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
            LIB_ALIAS* ret = LoadSymbol( nicks[i], name );

            if( ret )
                return ret;
        }

        return NULL;
    }
}


const wxString SYMBOL_LIB_TABLE::GlobalPathEnvVariableName()
{
    return  "KICAD_SYMBOL_DIR";
}


bool SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE& aTable )
{
    bool        tableExists = true;
    wxFileName  fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !fn.DirExists() && !fn.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot create global library table path \"%s\"." ),
                                              GetChars( fn.GetPath() ) ) );
        }

        // Attempt to copy the default global file table from the KiCad
        // template folder to the user's home configuration path.
        SEARCH_STACK ss;

        SystemDirsAppend( &ss );

        wxString templatePath =
            Pgm().GetLocalEnvVariables().at( wxT( "KICAD_TEMPLATE_DIR" ) ).GetValue();

        if( !templatePath.IsEmpty() )
            ss.AddPaths( templatePath, 0 );

        wxString fileName = ss.FindValidPath( global_tbl_name );

        // The fallback is to create an empty global symbol table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            SYMBOL_LIB_TABLE    emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.Load( fn.GetFullPath() );

    return tableExists;
}


wxString SYMBOL_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( GetKicadConfigPath() );
    fn.SetName( global_tbl_name );

    return fn.GetFullPath();
}


const wxString& SYMBOL_LIB_TABLE::GetSymbolLibTableFileName()
{
    return global_tbl_name;
}
