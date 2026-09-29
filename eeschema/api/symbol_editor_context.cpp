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

#include <api/symbol_editor_context.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <pgm_base.h>
#include <project.h>
#include <project_sch.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


class SYMBOL_EDIT_FRAME_CONTEXT : public SYMBOL_EDITOR_CONTEXT
{
public:
    explicit SYMBOL_EDIT_FRAME_CONTEXT( SYMBOL_EDIT_FRAME* aFrame ) : m_frame( aFrame ) {}

    LIB_SYMBOL* GetCurSymbol() const override { return m_frame->GetCurSymbol(); }

    SCH_SCREEN* GetScreen() const override { return m_frame->GetScreen(); }

    LIB_ID GetLoadedLibId() const override
    {
        if( LIB_SYMBOL* symbol = m_frame->GetCurSymbol() )
            return symbol->GetLibId();

        return LIB_ID();
    }

    bool IsContentModified() const override { return m_frame->IsContentModified(); }

    void SetContentModified( bool aModified = true ) override
    {
        m_frame->GetScreen()->SetContentModified( aModified );
    }

    PROJECT& Prj() const override { return m_frame->Prj(); }

    TOOL_MANAGER* GetToolManager() const override { return m_frame->GetToolManager(); }

    KIWAY* GetKiway() const override { return &m_frame->Kiway(); }

    bool CanAcceptApiCommands() const override { return m_frame->CanAcceptApiCommands(); }

    bool SaveSymbol() override
    {
        LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

        if( !symbol )
            return false;

        wxString libName = symbol->GetLibId().GetLibNickname();

        if( libName.IsEmpty() )
            return false;

        LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_frame->GetLibManager();

        if( libMgr.IsLibraryReadOnly( libName ) )
            return false;

        return m_frame->SaveLibraryHeadless( libName );
    }

    bool SaveSymbolCopy( const wxString& aFileName, bool aOverwrite ) override
    {
        LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

        if( !symbol )
            return false;

        return SaveSymbolCopyToFile( *symbol, aFileName, aOverwrite );
    }

    bool RevertSymbol() override
    {
        LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

        if( !symbol )
            return false;

        LIB_ID libId = symbol->GetLibId();

        if( libId.GetLibNickname().empty() || libId.GetLibItemName().empty() )
            return false;

        LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_frame->GetLibManager();

        if( !libMgr.SymbolExists( libId.GetLibNickname(), libId.GetLibItemName() ) )
            return false;

        m_frame->Revert( libId, /* aConfirm = */ false );
        return true;
    }

    bool LoadSymbol( const LIB_ID& aLibId ) override
    {
        return m_frame->LoadSymbol( aLibId, /* aUnit */ 1, /* aBodyStyle */ 1 );
    }

private:
    SYMBOL_EDIT_FRAME* m_frame;
};


std::shared_ptr<SYMBOL_EDITOR_CONTEXT> CreateSymbolEditorFrameContext( SYMBOL_EDIT_FRAME* aFrame )
{
    return std::make_shared<SYMBOL_EDIT_FRAME_CONTEXT>( aFrame );
}


bool SaveSymbolCopyToFile( const LIB_SYMBOL& aSymbol, const wxString& aFileName, bool aOverwrite )
{
    wxFileName fn( aFileName );
    fn.MakeAbsolute();

    if( !fn.IsOk() || !fn.IsDirWritable() )
        return false;

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( wxString( FILEEXT::KiCadSymbolLibFileExtension ) );

    if( fn.FileExists() && ( !fn.IsFileWritable() || !aOverwrite ) )
        return false;

    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

    if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        pluginType = SCH_IO_MGR::SCH_KICAD;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pluginType ) );

    if( !pi )
        return false;

    try
    {
        if( !fn.FileExists() )
            pi->CreateLibrary( fn.GetFullPath() );

        pi->SaveSymbol( fn.GetFullPath(), std::make_unique<LIB_SYMBOL>( aSymbol ) );
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return true;
}
