/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2012-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint_info.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <systemdirsappend.h>
#include <fp_lib_table.h>
#include <class_module.h>

#define OPT_SEP     '|'         ///< options separator character

using namespace LIB_TABLE_T;


static const wxChar global_tbl_name[] = wxT( "fp-lib-table" );


bool FP_LIB_TABLE_ROW::operator==( const FP_LIB_TABLE_ROW& aRow ) const
{
    return LIB_TABLE_ROW::operator == ( aRow ) && type == aRow.type;
}


void FP_LIB_TABLE_ROW::SetType( const wxString& aType )
{
    type = IO_MGR::EnumFromStr( aType );

    if( IO_MGR::PCB_FILE_T( -1 ) == type )
        type = IO_MGR::KICAD_SEXP;
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
    // Allow for parser of that optional containing s-epression to have looked ahead.
    if( in->CurTok() != T_fp_lib_table )
    {
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_fp_lib_table )
            in->Expecting( T_fp_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        std::unique_ptr< FP_LIB_TABLE_ROW > row( new FP_LIB_TABLE_ROW );

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
                                _( "Duplicate library nickname \"%s\" found in footprint library "
                                   "table file line %d" ), GetChars( nickname ), lineNum );

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
    if( rows.size() == aFpTable.rows.size() )
    {
        for( unsigned i = 0; i < rows.size();  ++i )
        {
            if( (FP_LIB_TABLE_ROW&)rows[i] != (FP_LIB_TABLE_ROW&)aFpTable.rows[i] )
                return false;
        }

        return true;
    }

    return false;
}


void FP_LIB_TABLE::Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const
{
    aOutput->Print( aIndentLevel, "(fp_lib_table\n" );

    for( LIB_TABLE_ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
        it->Format( aOutput, aIndentLevel+1 );

    aOutput->Print( aIndentLevel, ")\n" );
}


long long FP_LIB_TABLE::GenerateTimestamp( const wxString* aNickname )
{
    if( aNickname )
    {
        const FP_LIB_TABLE_ROW* row = FindRow( *aNickname );
        wxASSERT( (PLUGIN*) row->plugin );
        return row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) ) + wxHashTable::MakeKey( *aNickname );
    }

    long long hash = 0;
    for( wxString const& nickname : GetLogicalLibs() )
    {
        const FP_LIB_TABLE_ROW* row = FindRow( nickname );
        wxASSERT( (PLUGIN*) row->plugin );
        hash += row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) ) + wxHashTable::MakeKey( nickname );
    }

    return hash;
}


void FP_LIB_TABLE::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintEnumerate( aFootprintNames, row->GetFullURI( true ),
                                     row->GetProperties() );
}


void FP_LIB_TABLE::PrefetchLib( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->PrefetchLib( row->GetFullURI( true ), row->GetProperties() );
}


const FP_LIB_TABLE_ROW* FP_LIB_TABLE::FindRow( const wxString& aNickname )
{
    // Do not optimize this code.  Is done this way specifically to fix a runtime
    // error with clang 4.0.1.
    LIB_TABLE_ROW* ltrow = findRow( aNickname );
    FP_LIB_TABLE_ROW* row = dynamic_cast< FP_LIB_TABLE_ROW* >( ltrow );

    if( !row )
    {
        wxString msg = wxString::Format(
            _( "fp-lib-table files contain no library with nickname \"%s\"" ),
            GetChars( aNickname ) );

        THROW_IO_ERROR( msg );
    }

    // We've been 'lazy' up until now, but it cannot be deferred any longer,
    // instantiate a PLUGIN of the proper kind if it is not already in this
    // FP_LIB_TABLE_ROW.
    if( !row->plugin )
        row->setPlugin( IO_MGR::PluginFind( row->type ) );

    return row;
}


static void setLibNickname( MODULE* aModule,
                            const wxString& aNickname, const wxString& aFootprintName )
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


const MODULE* FP_LIB_TABLE::GetEnumeratedFootprint( const wxString& aNickname,
                                                    const wxString& aFootprintName )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    return row->plugin->GetEnumeratedFootprint( row->GetFullURI( true ), aFootprintName,
                                                row->GetProperties() );
}


bool FP_LIB_TABLE::FootprintExists( const wxString& aNickname, const wxString& aFootprintName )
{
    try
    {
        const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
        wxASSERT( (PLUGIN*) row->plugin );

        return row->plugin->FootprintExists( row->GetFullURI( true ), aFootprintName,
                                             row->GetProperties() );
    }
    catch( ... )
    {
        return false;
    }
}


MODULE* FP_LIB_TABLE::FootprintLoad( const wxString& aNickname, const wxString& aFootprintName )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    MODULE* ret = row->plugin->FootprintLoad( row->GetFullURI( true ), aFootprintName,
                                              row->GetProperties() );

    setLibNickname( ret, row->GetNickName(), aFootprintName );

    return ret;
}


FP_LIB_TABLE::SAVE_T FP_LIB_TABLE::FootprintSave( const wxString& aNickname,
                                                  const MODULE* aFootprint, bool aOverwrite )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString fpname = aFootprint->GetFPID().GetLibItemName();

        std::unique_ptr<MODULE> footprint( row->plugin->FootprintLoad( row->GetFullURI( true ),
                                           fpname, row->GetProperties() ) );

        if( footprint.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->FootprintSave( row->GetFullURI( true ), aFootprint, row->GetProperties() );

    return SAVE_OK;
}


void FP_LIB_TABLE::FootprintDelete( const wxString& aNickname, const wxString& aFootprintName )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->FootprintDelete( row->GetFullURI( true ), aFootprintName,
                                         row->GetProperties() );
}


bool FP_LIB_TABLE::IsFootprintLibWritable( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->IsFootprintLibWritable( row->GetFullURI( true ) );
}


void FP_LIB_TABLE::FootprintLibDelete( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibDelete( row->GetFullURI( true ), row->GetProperties() );
}


void FP_LIB_TABLE::FootprintLibCreate( const wxString& aNickname )
{
    const FP_LIB_TABLE_ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibCreate( row->GetFullURI( true ), row->GetProperties() );
}


MODULE* FP_LIB_TABLE::FootprintLoadWithOptionalNickname( const LIB_ID& aFootprintId )
{
    wxString   nickname = aFootprintId.GetLibNickname();
    wxString   fpname   = aFootprintId.GetLibItemName();

    if( nickname.size() )
    {
        return FootprintLoad( nickname, fpname );
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
            MODULE* ret = FootprintLoad( nicks[i], fpname );

            if( ret )
                return ret;
        }

        return NULL;
    }
}


const wxString FP_LIB_TABLE::GlobalPathEnvVariableName()
{
    return  "KISYSMOD";
}


bool FP_LIB_TABLE::LoadGlobalTable( FP_LIB_TABLE& aTable )
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

        // The fallback is to create an empty global footprint table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            FP_LIB_TABLE    emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.Load( fn.GetFullPath() );

    return tableExists;
}


wxString FP_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( GetKicadConfigPath() );
    fn.SetName( global_tbl_name );

    return fn.GetFullPath();
}
