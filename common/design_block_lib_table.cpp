/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <design_block_info.h>
#include <lib_id.h>
#include <lib_table_lexer.h>
#include <paths.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <systemdirsappend.h>
#include <design_block_lib_table.h>
#include <design_block.h>

#include <wx/dir.h>
#include <wx/hash.h>

#define OPT_SEP '|' ///< options separator character

/// The global design block library table.  This is not dynamically allocated because
/// in a multiple project environment we must keep its address constant (since it is
/// the fallback table for multiple projects).
DESIGN_BLOCK_LIB_TABLE GDesignBlockTable;

/// The global footprint info table.  This is performance-intensive to build so we
/// keep a hash-stamped global version.  Any deviation from the request vs. stored
/// hash will result in it being rebuilt.
DESIGN_BLOCK_LIST_IMPL GDesignBlockList;


using namespace LIB_TABLE_T;


static const wxChar global_tbl_name[] = wxT( "design-block-lib-table" );


bool DESIGN_BLOCK_LIB_TABLE_ROW::operator==( const DESIGN_BLOCK_LIB_TABLE_ROW& aRow ) const
{
    return LIB_TABLE_ROW::operator==( aRow ) && type == aRow.type;
}


void DESIGN_BLOCK_LIB_TABLE_ROW::SetType( const wxString& aType )
{
    type = DESIGN_BLOCK_IO_MGR::EnumFromStr( aType );

    if( DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T( -1 ) == type )
        type = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;

    plugin.reset();
}


DESIGN_BLOCK_LIB_TABLE::DESIGN_BLOCK_LIB_TABLE( DESIGN_BLOCK_LIB_TABLE* aFallBackTable ) :
        LIB_TABLE( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


void DESIGN_BLOCK_LIB_TABLE::Parse( LIB_TABLE_LEXER* in )
{
    T        tok;
    wxString errMsg; // to collect error messages

    // This table may be nested within a larger s-expression, or not.
    // Allow for parser of that optional containing s-epression to have looked ahead.
    if( in->CurTok() != T_design_block_lib_table )
    {
        in->NeedLEFT();

        if( ( tok = in->NextTok() ) != T_design_block_lib_table )
            in->Expecting( T_design_block_lib_table );
    }

    while( ( tok = in->NextTok() ) != T_RIGHT )
    {
        std::unique_ptr<DESIGN_BLOCK_LIB_TABLE_ROW> row =
                std::make_unique<DESIGN_BLOCK_LIB_TABLE_ROW>();

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
        bool sawType = false;
        bool sawOpts = false;
        bool sawDesc = false;
        bool sawUri = false;
        bool sawDisabled = false;

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
                // Hiding design block libraries is not yet supported.  Unclear what path can set this
                // attribute, but clear it on load.
                row->SetVisible();
                break;

            default: in->Unexpected( tok );
            }

            in->NeedRIGHT();
        }

        if( !sawType )
            in->Expecting( T_type );

        if( !sawUri )
            in->Expecting( T_uri );

        // All nickNames within this table fragment must be unique, so we do not use doReplace
        // in doInsertRow().  (However a fallBack table can have a conflicting nickName and ours
        // will supercede that one since in FindLib() we search this table before any fall back.)
        wxString nickname = row->GetNickName(); // store it to be able to used it
                                                // after row deletion if an error occurs
        bool           doReplace = false;
        LIB_TABLE_ROW* tmp = row.release();

        if( !doInsertRow( tmp, doReplace ) )
        {
            delete tmp; // The table did not take ownership of the row.

            wxString msg = wxString::Format( _( "Duplicate library nickname '%s' found in "
                                                "design block library table file line %d." ),
                                             nickname, lineNum );

            if( !errMsg.IsEmpty() )
                errMsg << '\n';

            errMsg << msg;
        }
    }

    if( !errMsg.IsEmpty() )
        THROW_IO_ERROR( errMsg );
}


bool DESIGN_BLOCK_LIB_TABLE::operator==( const DESIGN_BLOCK_LIB_TABLE& aDesignBlockTable ) const
{
    if( m_rows.size() == aDesignBlockTable.m_rows.size() )
    {
        for( unsigned i = 0; i < m_rows.size(); ++i )
        {
            if( (DESIGN_BLOCK_LIB_TABLE_ROW&) m_rows[i]
                != (DESIGN_BLOCK_LIB_TABLE_ROW&) aDesignBlockTable.m_rows[i] )
                return false;
        }

        return true;
    }

    return false;
}


void DESIGN_BLOCK_LIB_TABLE::Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const
{
    aOutput->Print( aIndentLevel, "(design_block_lib_table\n" );
    aOutput->Print( aIndentLevel + 1, "(version %d)\n", m_version );

    for( LIB_TABLE_ROWS_CITER it = m_rows.begin(); it != m_rows.end(); ++it )
        it->Format( aOutput, aIndentLevel + 1 );

    aOutput->Print( aIndentLevel, ")\n" );
}


long long DESIGN_BLOCK_LIB_TABLE::GenerateTimestamp( const wxString* aNickname )
{
    long long hash = 0;

    if( aNickname )
    {
        const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( *aNickname, true );

        wxCHECK( row && row->plugin, hash );

        return row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) )
               + wxHashTable::MakeKey( *aNickname );
    }

    for( const wxString& nickname : GetLogicalLibs() )
    {
        const DESIGN_BLOCK_LIB_TABLE_ROW* row = nullptr;

        try
        {
            row = FindRow( nickname, true );
        }
        catch( ... )
        {
            // Do nothing if not found: just skip.
        }

        wxCHECK2( row && row->plugin, continue );

        hash += row->plugin->GetLibraryTimestamp( row->GetFullURI( true ) )
                + wxHashTable::MakeKey( nickname );
    }

    return hash;
}


void DESIGN_BLOCK_LIB_TABLE::DesignBlockEnumerate( wxArrayString&  aDesignBlockNames,
                                                   const wxString& aNickname, bool aBestEfforts )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->DesignBlockEnumerate( aDesignBlockNames, row->GetFullURI( true ), aBestEfforts,
                                       row->GetProperties() );
}


void DESIGN_BLOCK_LIB_TABLE::PrefetchLib( const wxString& aNickname )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->PrefetchLib( row->GetFullURI( true ), row->GetProperties() );
}


const DESIGN_BLOCK_LIB_TABLE_ROW* DESIGN_BLOCK_LIB_TABLE::FindRow( const wxString& aNickname,
                                                                   bool            aCheckIfEnabled )
{
    DESIGN_BLOCK_LIB_TABLE_ROW* row =
            static_cast<DESIGN_BLOCK_LIB_TABLE_ROW*>( findRow( aNickname, aCheckIfEnabled ) );

    if( !row )
    {
        wxString msg = wxString::Format(
                _( "design-block-lib-table files contain no library named '%s'." ), aNickname );
        THROW_IO_ERROR( msg );
    }

    if( !row->plugin )
        row->setPlugin( DESIGN_BLOCK_IO_MGR::FindPlugin( row->type ) );

    return row;
}


static void setLibNickname( DESIGN_BLOCK* aModule, const wxString& aNickname,
                            const wxString& aDesignBlockName )
{
    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore design blocks cannot know their own library nickname when residing in
    // a design block library.
    // Only at this API layer can we tell the design block about its actual library nickname.
    if( aModule )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the LIB_ID and its two strings, twice each.
        LIB_ID& dbid = (LIB_ID&) aModule->GetLibId();

        // Catch any misbehaving plugin, which should be setting internal design block name properly:
        wxASSERT( aDesignBlockName == dbid.GetLibItemName().wx_str() );

        // and clearing nickname
        wxASSERT( !dbid.GetLibNickname().size() );

        dbid.SetLibNickname( aNickname );
    }
}


const DESIGN_BLOCK*
DESIGN_BLOCK_LIB_TABLE::GetEnumeratedDesignBlock( const wxString& aNickname,
                                                  const wxString& aDesignBlockName )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    return row->plugin->GetEnumeratedDesignBlock( row->GetFullURI( true ), aDesignBlockName,
                                                  row->GetProperties() );
}


bool DESIGN_BLOCK_LIB_TABLE::DesignBlockExists( const wxString& aNickname,
                                                const wxString& aDesignBlockName )
{
    try
    {
        const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
        wxASSERT( row->plugin );

        return row->plugin->DesignBlockExists( row->GetFullURI( true ), aDesignBlockName,
                                               row->GetProperties() );
    }
    catch( ... )
    {
        return false;
    }
}


DESIGN_BLOCK* DESIGN_BLOCK_LIB_TABLE::DesignBlockLoad( const wxString& aNickname,
                                                       const wxString& aDesignBlockName,
                                                       bool            aKeepUUID )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    DESIGN_BLOCK* ret = row->plugin->DesignBlockLoad( row->GetFullURI( true ), aDesignBlockName,
                                                      aKeepUUID, row->GetProperties() );

    setLibNickname( ret, row->GetNickName(), aDesignBlockName );

    return ret;
}


DESIGN_BLOCK_LIB_TABLE::SAVE_T
DESIGN_BLOCK_LIB_TABLE::DesignBlockSave( const wxString&     aNickname,
                                         const DESIGN_BLOCK* aDesignBlock, bool aOverwrite )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );

    if( !aOverwrite )
    {
        // Try loading the design block to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString DesignBlockname = aDesignBlock->GetLibId().GetLibItemName();

        std::unique_ptr<DESIGN_BLOCK> design_block( row->plugin->DesignBlockLoad(
                row->GetFullURI( true ), DesignBlockname, row->GetProperties() ) );

        if( design_block.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->DesignBlockSave( row->GetFullURI( true ), aDesignBlock, row->GetProperties() );

    return SAVE_OK;
}


void DESIGN_BLOCK_LIB_TABLE::DesignBlockDelete( const wxString& aNickname,
                                                const wxString& aDesignBlockName )

{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    return row->plugin->DesignBlockDelete( row->GetFullURI( true ), aDesignBlockName,
                                           row->GetProperties() );
}


bool DESIGN_BLOCK_LIB_TABLE::IsDesignBlockLibWritable( const wxString& aNickname )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    return row->plugin->IsLibraryWritable( row->GetFullURI( true ) );
}


void DESIGN_BLOCK_LIB_TABLE::DesignBlockLibDelete( const wxString& aNickname )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->DeleteLibrary( row->GetFullURI( true ), row->GetProperties() );
}


void DESIGN_BLOCK_LIB_TABLE::DesignBlockLibCreate( const wxString& aNickname )
{
    const DESIGN_BLOCK_LIB_TABLE_ROW* row = FindRow( aNickname, true );
    wxASSERT( row->plugin );
    row->plugin->CreateLibrary( row->GetFullURI( true ), row->GetProperties() );
}


DESIGN_BLOCK*
DESIGN_BLOCK_LIB_TABLE::DesignBlockLoadWithOptionalNickname( const LIB_ID& aDesignBlockId,
                                                             bool          aKeepUUID )
{
    wxString nickname = aDesignBlockId.GetLibNickname();
    wxString DesignBlockname = aDesignBlockId.GetLibItemName();

    if( nickname.size() )
    {
        return DesignBlockLoad( nickname, DesignBlockname, aKeepUUID );
    }

    // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
    else
    {
        std::vector<wxString> nicks = GetLogicalLibs();

        // Search each library going through libraries alphabetically.
        for( unsigned i = 0; i < nicks.size(); ++i )
        {
            // DesignBlockLoad() returns NULL on not found, does not throw exception
            // unless there's an IO_ERROR.
            DESIGN_BLOCK* ret = DesignBlockLoad( nicks[i], DesignBlockname, aKeepUUID );

            if( ret )
                return ret;
        }

        return nullptr;
    }
}


const wxString DESIGN_BLOCK_LIB_TABLE::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "DESIGN_BLOCK_DIR" ) );
}


class PCM_DESIGN_BLOCK_LIB_TRAVERSER final : public wxDirTraverser
{
public:
    explicit PCM_DESIGN_BLOCK_LIB_TRAVERSER( const wxString& aPath, DESIGN_BLOCK_LIB_TABLE& aTable,
                                             const wxString& aPrefix ) :
            m_lib_table( aTable ),
            m_path_prefix( aPath ), m_lib_prefix( aPrefix )
    {
        wxFileName f( aPath, wxS( "" ) );
        m_prefix_dir_count = f.GetDirCount();
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override { return wxDIR_CONTINUE; }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        wxFileName dir = wxFileName::DirName( dirPath );

        // consider a directory to be a lib if it's name ends with .pretty and
        // it is under $KICADn_3RD_PARTY/design_blocks/<pkgid>/ i.e. has nested level of at least +3
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
                        nickname =
                                wxString::Format( wxS( "%s%s_%d" ), m_lib_prefix, name, increment );
                        increment++;
                    } while( m_lib_table.HasLibrary( nickname ) );
                }

                m_lib_table.InsertRow( new DESIGN_BLOCK_LIB_TABLE_ROW(
                        nickname, libPath, wxT( "KiCad" ), wxEmptyString,
                        _( "Added by Plugin and Content Manager" ) ) );
            }
        }

        return wxDIR_CONTINUE;
    }

private:
    DESIGN_BLOCK_LIB_TABLE& m_lib_table;
    wxString                m_path_prefix;
    wxString                m_lib_prefix;
    size_t                  m_prefix_dir_count;
};


bool DESIGN_BLOCK_LIB_TABLE::LoadGlobalTable( DESIGN_BLOCK_LIB_TABLE& aTable )
{
    bool       tableExists = true;
    wxFileName fn = GetGlobalTableFileName();

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

        const ENV_VAR_MAP&      envVars = Pgm().GetLocalEnvVariables();
        std::optional<wxString> v =
                ENV_VAR::GetVersionedEnvVarValue( envVars, wxT( "TEMPLATE_DIR" ) );

        if( v && !v->IsEmpty() )
            ss.AddPaths( *v, 0 );

        wxString fileName = ss.FindValidPath( global_tbl_name );

        // The fallback is to create an empty global design block table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            DESIGN_BLOCK_LIB_TABLE emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.clear();
    aTable.Load( fn.GetFullPath() );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>();

    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
    wxString           packagesPath;

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( env, wxT( "3RD_PARTY" ) ) )
        packagesPath = *v;

    if( settings->m_PcmLibAutoAdd )
    {
        // Scan for libraries in PCM packages directory

        wxFileName d( packagesPath, wxS( "" ) );
        d.AppendDir( wxS( "design_blocks" ) );

        if( d.DirExists() )
        {
            PCM_DESIGN_BLOCK_LIB_TRAVERSER traverser( packagesPath, aTable,
                                                      settings->m_PcmLibPrefix );
            wxDir                          dir( d.GetPath() );

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


DESIGN_BLOCK_LIB_TABLE& DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable()
{
    return GDesignBlockTable;
}


DESIGN_BLOCK_LIST_IMPL& DESIGN_BLOCK_LIB_TABLE::GetGlobalList()
{
    return GDesignBlockList;
}


wxString DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( PATHS::GetUserSettingsPath() );
    fn.SetName( global_tbl_name );

    return fn.GetFullPath();
}
