/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <macros.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <symbol_lib_table.h>
#include <class_libentry.h>

#define OPT_SEP     '|'         ///< options separator character

using namespace LIB_TABLE_T;


static const wxChar global_tbl_name[] = wxT( "sym-lib-table" );


bool SYMBOL_LIB_TABLE_ROW::operator==( const SYMBOL_LIB_TABLE_ROW& aRow ) const
{
    return LIB_TABLE_ROW::operator == ( aRow ) && type == aRow.type;
}


void SYMBOL_LIB_TABLE_ROW::SetType( const wxString& aType )
{
    type = SCH_IO_MGR::EnumFromStr( aType );

    if( SCH_IO_MGR::SCH_FILE_T( -1 ) == type )
        type = SCH_IO_MGR::SCH_KICAD;
}


SYMBOL_LIB_TABLE::SYMBOL_LIB_TABLE( SYMBOL_LIB_TABLE* aFallBackTable ) :
    LIB_TABLE( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


void SYMBOL_LIB_TABLE::Parse( LIB_TABLE_LEXER* in ) throw()
{
    T       tok;

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
        int offset  = in->CurOffset();

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
        bool    sawType = false;
        bool    sawOpts = false;
        bool    sawDesc = false;
        bool    sawUri  = false;

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
        if( !InsertRow( row.release() ) )
        {
            wxString msg = wxString::Format(
                                _( "'%s' is a duplicate symbol library nickname" ),
                                GetChars( row->GetNickName() ) );
            THROW_PARSE_ERROR( msg, in->CurSource(), in->CurLine(), lineNum, offset );
        }
    }
}


void SYMBOL_LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw()
{
    out->Print( nestLevel, "(sym_lib_table\n" );

    for( LIB_TABLE_ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->Format( out, nestLevel+1 );

    out->Print( nestLevel, ")\n" );
}


void SYMBOL_LIB_TABLE::EnumerateSymbolLib( const wxString& aNickname, wxArrayString& aAliasNames )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    row->plugin->EnumerateSymbolLib( aAliasNames, row->GetFullURI( true ), row->GetProperties() );
}


const SYMBOL_LIB_TABLE_ROW* SYMBOL_LIB_TABLE::FindRow( const wxString& aNickname )
    throw( IO_ERROR )
{
    SYMBOL_LIB_TABLE_ROW* row = dynamic_cast< SYMBOL_LIB_TABLE_ROW* >( findRow( aNickname ) );

    if( !row )
    {
        wxString msg = wxString::Format(
            _( "sym-lib-table files contain no library with nickname '%s'" ),
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


LIB_ALIAS* SYMBOL_LIB_TABLE::LoadSymbol( const wxString& aNickname, const wxString& aAliasName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );

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

        // Catch any misbehaving plugin, which should be setting internal alias name properly:
        wxASSERT( aAliasName == (wxString) id.GetLibItemName() );

        // and clearing nickname
        wxASSERT( !id.GetLibNickname().size() );

        id.SetLibNickname( row->GetNickName() );
    }

    return ret;
}


SYMBOL_LIB_TABLE::SAVE_T SYMBOL_LIB_TABLE::SaveSymbol( const wxString& aNickname,
                                                       const LIB_PART* aSymbol, bool aOverwrite )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );

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
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    return row->plugin->DeleteSymbol( row->GetFullURI( true ), aSymbolName,
                                      row->GetProperties() );
}


void SYMBOL_LIB_TABLE::DeleteAlias( const wxString& aNickname, const wxString& aAliasName )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    return row->plugin->DeleteAlias( row->GetFullURI( true ), aAliasName,
                                     row->GetProperties() );
}


bool SYMBOL_LIB_TABLE::IsSymbolLibWritable( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    return row->plugin->IsSymbolLibWritable( row->GetFullURI( true ) );
}


void SYMBOL_LIB_TABLE::DeleteSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    row->plugin->DeleteSymbolLib( row->GetFullURI( true ), row->GetProperties() );
}


void SYMBOL_LIB_TABLE::CreateSymbolLib( const wxString& aNickname )
{
    const SYMBOL_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (SCH_PLUGIN*) row->plugin );
    row->plugin->CreateSymbolLib( row->GetFullURI( true ), row->GetProperties() );
}


LIB_ALIAS* SYMBOL_LIB_TABLE::LoadSymbolWithOptionalNickname( const LIB_ID& aLibId )
    throw( IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception )
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
    return  "KICAD_SYSTEM_SYMBOLS";
}


bool SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE& aTable )
    throw (IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception )
{
    bool        tableExists = true;
    wxFileName  fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !fn.DirExists() && !fn.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot create global library table path '%s'." ),
                                              GetChars( fn.GetPath() ) ) );
        }

        // Attempt to copy the default global file table from the KiCad
        // template folder to the user's home configuration path.
        wxString fileName = Kiface().KifaceSearch().FindValidPath( global_tbl_name );

        // The fallback is to create an empty global footprint table for the user to populate.
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
