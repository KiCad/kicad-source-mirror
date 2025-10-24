/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <pgm_base.h>
#include <clipboard.h>
#include <confirm.h>
#include <kidialog.h>
#include <kiway.h>
#include <widgets/wx_infobar.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <symbol_edit_frame.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <lib_symbol_library_manager.h>
#include <symbol_tree_pane.h>
#include <project/project_file.h>
#include <richio.h>
#include <widgets/lib_tree.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <dialogs/dialog_lib_new_symbol.h>
#include <eda_list_dialog.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <project_sch.h>
#include <string_utils.h>
#include "symbol_saveas_type.h"
#include <widgets/symbol_library_save_as_filedlg_hook.h>
#include <io/kicad/kicad_io_utils.h>
#include <libraries/symbol_library_adapter.h>


void SYMBOL_EDIT_FRAME::UpdateTitle()
{
    wxString title;

    if( GetCurSymbol() && IsSymbolFromSchematic() )
    {
        if( GetScreen() && GetScreen()->IsContentModified() )
            title = wxT( "*" );

        title += m_reference;
        title += wxS( " " ) + _( "[from schematic]" );
    }
    else if( GetCurSymbol() )
    {
        if( GetScreen() && GetScreen()->IsContentModified() )
            title = wxT( "*" );

        title += UnescapeString( GetCurSymbol()->GetLibId().Format() );

        if( m_libMgr && m_libMgr->LibraryExists( GetCurLib() ) && m_libMgr->IsLibraryReadOnly( GetCurLib() ) )
            title += wxS( " " ) + _( "[Read Only Library]" );
    }
    else
    {
        title = _( "[no symbol loaded]" );
    }

    title += wxT( " \u2014 " ) + _( "Symbol Editor" );
    SetTitle( title );
}


void SYMBOL_EDIT_FRAME::SelectActiveLibrary( const wxString& aLibrary )
{
    wxString selectedLib = aLibrary;

    if( selectedLib.empty() )
        selectedLib = SelectLibrary( _( "Select Symbol Library" ), _( "Library:" ) );

    if( !selectedLib.empty() )
        SetCurLib( selectedLib );

    UpdateTitle();
}


bool SYMBOL_EDIT_FRAME::saveCurrentSymbol()
{
    if( GetCurSymbol() )
    {
        if( IsSymbolFromSchematic() )
        {
            SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

            if( !schframe )      // happens when the schematic editor has been closed
            {
                DisplayErrorMessage( this, _( "No schematic currently open." ) );
                return false;
            }
            else
            {
                schframe->SaveSymbolToSchematic( *m_symbol, m_schematicSymbolUUID );
                GetScreen()->SetContentModified( false );
                return true;
            }
        }
        else
        {
            const wxString& libName = GetCurSymbol()->GetLibId().GetLibNickname();

            if( m_libMgr->IsLibraryReadOnly( libName ) )
            {
                wxString msg = wxString::Format( _( "Symbol library '%s' is not writable." ),
                                                 libName );
                wxString msg2 = _( "You must save to a different location." );

                if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) == wxID_OK )
                    return saveLibrary( libName, true );
            }
            else
            {
                return saveLibrary( libName, false );
            }
        }
    }

    return false;
}


bool SYMBOL_EDIT_FRAME::LoadSymbol( const LIB_ID& aLibId, int aUnit, int aBodyStyle )
{
    LIB_ID libId = aLibId;
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &Prj() );

    // Some libraries can't be edited, so load the underlying chosen symbol
    if( auto optRow = manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, aLibId.GetLibNickname() );
        optRow.has_value() )
    {
        const LIBRARY_TABLE_ROW* row = *optRow;
        SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::EnumFromStr( row->Type() );

        if( type == SCH_IO_MGR::SCH_DATABASE
            || type == SCH_IO_MGR::SCH_CADSTAR_ARCHIVE
            || type == SCH_IO_MGR::SCH_HTTP )
        {
            try
            {
                LIB_SYMBOL* readOnlySym = adapter->LoadSymbol( aLibId );

                if( readOnlySym && readOnlySym->GetSourceLibId().IsValid() )
                    libId = readOnlySym->GetSourceLibId();
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;

                msg.Printf( _( "Error loading symbol %s from library '%s'." ),
                            aLibId.GetUniStringLibId(), aLibId.GetUniStringLibItemName() );
                DisplayErrorMessage( this, msg, ioe.What() );
                return false;
            }
        }
    }

    if( GetCurSymbol() && !IsSymbolFromSchematic()
            && GetCurSymbol()->GetLibId() == libId
            && GetUnit() == aUnit
            && GetBodyStyle() == aBodyStyle )
    {
        return true;
    }

    if( GetCurSymbol() && IsSymbolFromSchematic() && GetScreen()->IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current symbol has been modified.  Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return saveCurrentSymbol();
                                   } ) )
        {
            return false;
        }
    }

    SelectActiveLibrary( libId.GetLibNickname() );

    if( LoadSymbolFromCurrentLib( libId.GetLibItemName(), aUnit, aBodyStyle ) )
    {
        m_treePane->GetLibTree()->SelectLibId( libId );
        m_treePane->GetLibTree()->ExpandLibId( libId );

        m_centerItemOnIdle = libId;
        Bind( wxEVT_IDLE, &SYMBOL_EDIT_FRAME::centerItemIdleHandler, this );
        setSymWatcher( &libId );

        return true;
    }

    return false;
}


void SYMBOL_EDIT_FRAME::centerItemIdleHandler( wxIdleEvent& aEvent )
{
    m_treePane->GetLibTree()->CenterLibId( m_centerItemOnIdle );
    Unbind( wxEVT_IDLE, &SYMBOL_EDIT_FRAME::centerItemIdleHandler, this );
}


bool SYMBOL_EDIT_FRAME::LoadSymbolFromCurrentLib( const wxString& aSymbolName, int aUnit,
                                                  int aBodyStyle )
{
    LIB_SYMBOL* symbol = nullptr;

    try
    {
        symbol = PROJECT_SCH::SymbolLibAdapter( &Prj() )->LoadSymbol( GetCurLib(), aSymbolName );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;

        msg.Printf( _( "Error loading symbol %s from library '%s'." ),
                    aSymbolName,
                    GetCurLib() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return false;
    }

    if( !symbol || !LoadOneLibrarySymbolAux( symbol, GetCurLib(), aUnit, aBodyStyle ) )
        return false;

    // Enable synchronized pin edit mode for symbols with interchangeable units
    m_SyncPinEdit = GetCurSymbol()->IsMultiUnit() && !GetCurSymbol()->UnitsLocked();

    ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );

    RebuildSymbolUnitAndBodyStyleLists();

    return true;
}


bool SYMBOL_EDIT_FRAME::LoadOneLibrarySymbolAux( LIB_SYMBOL* aEntry, const wxString& aLibrary,
                                                 int aUnit, int aBodyStyle )
{
    bool rebuildMenuAndToolbar = false;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library '%s' has empty name field.", aLibrary );
        return false;
    }

    m_toolManager->RunAction( ACTIONS::cancelInteractive );

    // Symbols from the schematic are edited in place and not managed by the library manager.
    if( IsSymbolFromSchematic() )
    {
        delete m_symbol;
        m_symbol = nullptr;

        SCH_SCREEN* screen = GetScreen();
        delete screen;
        SetScreen( m_dummyScreen );
        m_isSymbolFromSchematic = false;
        rebuildMenuAndToolbar = true;
    }

    LIB_SYMBOL* lib_symbol = m_libMgr->GetBufferedSymbol( aEntry->GetName(), aLibrary );
    wxCHECK( lib_symbol, false );

    m_unit = aUnit > 0 ? aUnit : 1;
    m_bodyStyle = aBodyStyle > 0 ? aBodyStyle : 1;

    // The buffered screen for the symbol
    SCH_SCREEN* symbol_screen = m_libMgr->GetScreen( lib_symbol->GetName(), aLibrary );

    SetScreen( symbol_screen );
    SetCurSymbol( new LIB_SYMBOL( *lib_symbol ), true );
    SetCurLib( aLibrary );

    if( rebuildMenuAndToolbar )
    {
        ReCreateMenuBar();
        RecreateToolbars();
        GetInfoBar()->Dismiss();
    }

    UpdateTitle();
    RebuildSymbolUnitAndBodyStyleLists();

    ClearUndoRedoList();

    if( !IsSymbolFromSchematic() )
    {
        LIB_ID libId = GetCurSymbol()->GetLibId();
        setSymWatcher( &libId );
    }

    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );

    // Display the document information based on the entry selected just in
    // case the entry is an alias.
    UpdateMsgPanel();
    Refresh();

    return true;
}


void SYMBOL_EDIT_FRAME::SaveAll()
{
    saveAllLibraries( false );
    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::CreateNewSymbol( const wxString& aInheritFrom )
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive );

    wxString lib = getTargetLib();

    if( !m_libMgr->LibraryExists( lib ) )
    {
        lib = SelectLibrary( _( "New Symbol" ), _( "Create symbol in library:" ) );

        if( !m_libMgr->LibraryExists( lib ) )
            return;
    }

    const auto validator =
            [&]( wxString newName ) -> bool
            {
                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Symbol must have a name." ) );
                    return false;
                }

                if( !lib.empty() && m_libMgr->SymbolNameInUse( newName, lib ) )
                {
                    wxString msg;

                    msg.Printf( _( "Symbol '%s' already exists in library '%s'." ),
                                UnescapeString( newName ),
                                lib );

                    KIDIALOG errorDlg( this, msg, _( "Confirmation" ),
                                       wxOK | wxCANCEL | wxICON_WARNING );

                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK;
                }

                return true;
            };

    wxArrayString symbolNamesInLib;
    m_libMgr->GetSymbolNames( lib, symbolNamesInLib );

    DIALOG_LIB_NEW_SYMBOL dlg( this, symbolNamesInLib, aInheritFrom, validator );

    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    NEW_SYMBOL_PROPERTIES props;

    props.name = dlg.GetName();
    props.parentSymbolName = dlg.GetParentSymbolName();
    props.reference = dlg.GetReference();
    props.unitCount = dlg.GetUnitCount();
    props.pinNameInside = dlg.GetPinNameInside();
    props.pinTextPosition = dlg.GetPinTextPosition();
    props.powerSymbol = dlg.GetPowerSymbol();
    props.showPinNumber = dlg.GetShowPinNumber();
    props.showPinName = dlg.GetShowPinName();
    props.unitsInterchangeable = dlg.GetUnitsInterchangeable();
    props.includeInBom = dlg.GetIncludeInBom();
    props.includeOnBoard = dlg.GetIncludeOnBoard();
    props.alternateBodyStyle = dlg.GetAlternateBodyStyle();
    props.keepFootprint = dlg.GetKeepFootprint();
    props.keepDatasheet = dlg.GetKeepDatasheet();
    props.transferUserFields = dlg.GetTransferUserFields();
    props.keepContentUserFields = dlg.GetKeepContentUserFields();

    m_libMgr->CreateNewSymbol( lib, props );
    SyncLibraries( false );
    LoadSymbol( props.name, lib, 1 );
}


void SYMBOL_EDIT_FRAME::Save()
{
    wxString libName;

    if( IsLibraryTreeShown() )
        libName = GetTreeLIBID().GetUniStringLibNickname();

    if( libName.empty() )
    {
        saveCurrentSymbol();
    }
    else if( m_libMgr->IsLibraryReadOnly( libName ) )
    {
        wxString msg = wxString::Format( _( "Symbol library '%s' is not writable." ),
                                         libName );
        wxString msg2 = _( "You must save to a different location." );

        if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) == wxID_OK )
            saveLibrary( libName, true );
    }
    else
    {
        saveLibrary( libName, false );
    }

    if( IsLibraryTreeShown() )
        m_treePane->GetLibTree()->RefreshLibTree();

    UpdateTitle();
}


void SYMBOL_EDIT_FRAME::SaveLibraryAs()
{
    const wxString& libName = GetTargetLibId().GetLibNickname();

    if( !libName.IsEmpty() )
    {
        saveLibrary( libName, true );
        m_treePane->GetLibTree()->RefreshLibTree();
    }
}


void SYMBOL_EDIT_FRAME::SaveSymbolCopyAs( bool aOpenCopy )
{
    saveSymbolCopyAs( aOpenCopy );

    m_treePane->GetLibTree()->RefreshLibTree();
}


/**
 * Get a list of all the symbols in the parental chain of a symbol,
 * with the "leaf" symbol at the start and the "rootiest" symbol at the end.
 *
 * If the symbol is not an alias, the list will contain only the symbol itself.
 *
 * If aIncludeLeaf is false, the leaf symbol (the one that was actually named)
 * is not included in the list, so the list may be empty if the symbol is not derived.
 */
static std::vector<std::shared_ptr<LIB_SYMBOL>> GetParentChain( const LIB_SYMBOL& aSymbol, bool aIncludeLeaf = true )
{
    std::vector<std::shared_ptr<LIB_SYMBOL>> chain;
    std::shared_ptr<LIB_SYMBOL>              sym = aSymbol.SharedPtr();

    if( aIncludeLeaf )
        chain.push_back( sym );

    while( sym->IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> parent = sym->GetParent().lock();
        chain.push_back( parent );
        sym = parent;
    }

    return chain;
}


/**
 * Check if a planned overwrite would put a symbol into it's own inheritance chain.
 * This causes infinite loops and other unpleasantness and makes not sense - inheritance
 * must be acyclic.
 *
 * Returns a pair of bools:
 *   - first: true if the symbol would be saved into it's own ancestry
 *   - second: true if the symbol would be saved into it's own descendents
 */
static std::pair<bool, bool> CheckSavingIntoOwnInheritance( LIB_SYMBOL_LIBRARY_MANAGER& aLibMgr,
                                                            LIB_SYMBOL&                 aSymbol,
                                                            const wxString& aNewSymbolName,
                                                            const wxString& aNewLibraryName )
{
    const wxString& oldLibraryName = aSymbol.GetLibId().GetLibNickname();

    // Cannot be intersecting if in different libs
    if( aNewLibraryName != oldLibraryName )
        return { false, false };

    // Or if the target symbol doesn't exist
    if( !aLibMgr.SymbolNameInUse( aNewSymbolName, aNewLibraryName ) )
        return { false, false };

    bool inAncestry = false;
    bool inDescendents = false;

    {
        const std::vector<std::shared_ptr<LIB_SYMBOL>> parentChainFromUs = GetParentChain( aSymbol, true );

        // Ignore the leaf symbol (0) - that must match
        for( size_t i = 1; i < parentChainFromUs.size(); ++i )
        {
            // Attempting to overwrite a symbol in the parental chain
            if( parentChainFromUs[i]->GetName() == aNewSymbolName )
            {
                inAncestry = true;
                break;
            }
        }
    }

    {
        LIB_SYMBOL* targetSymbol = aLibMgr.GetSymbol( aNewSymbolName, aNewLibraryName );
        const std::vector<std::shared_ptr<LIB_SYMBOL>> parentChainFromTarget = GetParentChain( *targetSymbol, true );
        const wxString                                 oldSymbolName = aSymbol.GetName();

        // Ignore the leaf symbol - it'll match if we're saving the symbol
        // to the same name, and that would be OK
        for( size_t i = 1; i < parentChainFromTarget.size(); ++i )
        {
            if( parentChainFromTarget[i]->GetName() == oldSymbolName )
            {
                inDescendents = true;
                break;
            }
        }
    }

    return { inAncestry, inDescendents };
}


/**
 * Get a list of all the symbols in the parental chain of a symbol that have conflicts
 * when transposed to a different library.
 *
 * This doesn't check for dangerous conflicts like saving into a symbol's own inheritance,
 * this is just about which symbols will get overwritten if saved with these names.
 */
static std::vector<wxString> CheckForParentalChainConflicts( LIB_SYMBOL_LIBRARY_MANAGER& aLibMgr,
                                                             LIB_SYMBOL&                 aSymbol,
                                                             bool                        aFlattenSymbol,
                                                             const wxString& newSymbolName,
                                                             const wxString& newLibraryName )
{
    std::vector<wxString> conflicts;
    const wxString&       oldLibraryName = aSymbol.GetLibId().GetLibNickname();

    if( newLibraryName == oldLibraryName || aFlattenSymbol )
    {
        // Saving into the same library - the only conflict could be the symbol itself
        // Different library and flattening - ditto
        if( aLibMgr.SymbolNameInUse( newSymbolName, newLibraryName ) )
            conflicts.push_back( newSymbolName );
    }
    else
    {
        // In a different library with parents - check the whole chain
        const std::vector<std::shared_ptr<LIB_SYMBOL>> parentChain = GetParentChain( aSymbol, true );

        for( size_t i = 0; i < parentChain.size(); ++i )
        {
            if( i == 0 )
            {
                // This is the leaf symbol which the user actually named
                if( aLibMgr.SymbolNameInUse( newSymbolName, newLibraryName ) )
                    conflicts.push_back( newSymbolName );
            }
            else
            {
                std::shared_ptr<LIB_SYMBOL> chainSymbol = parentChain[i];

                if( aLibMgr.SymbolNameInUse( chainSymbol->GetName(), newLibraryName ) )
                    conflicts.push_back( chainSymbol->GetName() );
            }
        }
    }

    return conflicts;
}


/**
 * This is a class that handles state involved in saving a symbol copy as a new symbol.
 *
 * This isn't as simple as it sounds, because you also have to copy all the parent symbols
 * (which is a chain affair).
 */
class SYMBOL_SAVE_AS_HANDLER
{
public:
    enum class CONFLICT_STRATEGY
    {
        // Just overwrite any existing symbols in the target library
        OVERWRITE,
        // Add a suffix until we find a name that doesn't conflict
        RENAME,
        // Could have a mode that asks for every one, be then we'll need a fancier
        // SAVE_SYMBOL_AS_DIALOG subdialog with Overwrite/Rename/Prompt/Cancel
        // PROMPT
    };

    SYMBOL_SAVE_AS_HANDLER( LIB_SYMBOL_LIBRARY_MANAGER& aLibMgr, CONFLICT_STRATEGY aStrategy, bool aValueFollowsName ) :
            m_libMgr( aLibMgr ),
            m_strategy( aStrategy ),
            m_valueFollowsName( aValueFollowsName )
    {
    }

    bool DoSave( LIB_SYMBOL& symbol, const wxString& aNewSymName, const wxString& aNewLibName, bool aFlattenSymbol )
    {
        std::unique_ptr<LIB_SYMBOL>              flattenedSymbol; // for ownership
        std::vector<std::shared_ptr<LIB_SYMBOL>> parentChain;

        const bool sameLib = aNewLibName == symbol.GetLibId().GetLibNickname().wx_str();

        if( aFlattenSymbol )
        {
            // If we're not copying parent symbols, we need to flatten the symbol
            // and only save that.
            flattenedSymbol = symbol.Flatten();
            wxCHECK( flattenedSymbol, false );

            parentChain.push_back( flattenedSymbol->SharedPtr() );
        }
        else if( sameLib )
        {
            // If we're saving into the same library, we don't need to check the parental chain
            // because we can just keep the same parent symbol
            parentChain.push_back( symbol.SharedPtr() );
        }
        else
        {
            // Need to copy all parent symbols
            parentChain = GetParentChain( symbol, true );
        }

        std::vector<wxString> newNames;

        // Iterate backwards (i.e. from the root down)
        for( int i = (int) parentChain.size() - 1; i >= 0; --i )
        {
            std::shared_ptr<LIB_SYMBOL>& oldSymbol = parentChain[i];
            LIB_SYMBOL                   new_symbol( *oldSymbol );

            wxString newName;
            if( i == 0 )
            {
                // This is the leaf symbol which the user actually named
                newName = aNewSymName;
            }
            else
            {
                // Somewhere in the inheritance chain, use the conflict resolution strategy
                newName = oldSymbol->GetName();
            }

            newName = resolveConflict( newName, aNewLibName );
            new_symbol.SetName( newName );

            if( m_valueFollowsName )
                new_symbol.GetValueField().SetText( newName );

            if( i == (int) parentChain.size() - 1 )
            {
                // This is the root symbol
                // Nothing extra to do, it's just a simple symbol with no parents
            }
            else
            {
                // Get the buffered new copy in the new library (with the name we gave it)
                LIB_SYMBOL* newParent = m_libMgr.GetSymbol( newNames.back(), aNewLibName );

                // We should have stored this already, why didn't we get it back?
                wxASSERT( newParent );
                new_symbol.SetParent( newParent );
            }

            newNames.push_back( newName );
            m_libMgr.UpdateSymbol( &new_symbol, aNewLibName );
        }

        return true;
    }

private:
    wxString resolveConflict( const wxString& proposed, const wxString& aNewLibName ) const
    {
        switch( m_strategy )
        {
        case CONFLICT_STRATEGY::OVERWRITE:
        {
            // In an overwrite strategy, we don't care about conflicts
            return proposed;
        }
        case CONFLICT_STRATEGY::RENAME:
        {
            // In a rename strategy, we need to find a name that doesn't conflict
            int suffix = 1;

            while( true )
            {
                wxString newName = wxString::Format( "%s_%d", proposed, suffix );

                if( !m_libMgr.SymbolNameInUse( newName, aNewLibName ) )
                    return newName;

                ++suffix;
            }
            break;
        }
            // No default
        }

        wxFAIL_MSG( "Invalid conflict strategy" );
        return "";
    }

    LIB_SYMBOL_LIBRARY_MANAGER& m_libMgr;
    CONFLICT_STRATEGY           m_strategy;
    bool                        m_valueFollowsName;
};


enum SAVE_AS_IDS
{
    ID_MAKE_NEW_LIBRARY = wxID_HIGHEST + 1,
    ID_OVERWRITE_CONFLICTS,
    ID_RENAME_CONFLICTS,
};


class SAVE_SYMBOL_AS_DIALOG : public EDA_LIST_DIALOG
{
public:
    using SymLibNameValidator = std::function<int( const wxString& libName, const wxString& symbolName )>;

    struct PARAMS
    {
        wxString                                  m_SymbolName;
        wxString                                  m_LibraryName;
        bool                                      m_FlattenSymbol;
        SYMBOL_SAVE_AS_HANDLER::CONFLICT_STRATEGY m_ConflictStrategy;
    };

    SAVE_SYMBOL_AS_DIALOG( SYMBOL_EDIT_FRAME* aParent,
                           PARAMS& aParams,
                           SymLibNameValidator aValidator,
                           const std::vector<wxString>& aParentSymbolNames ) :
            EDA_LIST_DIALOG( aParent, _( "Save Symbol As" ), false ),
            m_validator( std::move( aValidator ) ),
            m_params( aParams )
    {
        wxArrayString              headers;
        std::vector<wxArrayString> itemsToDisplay;

        if( aParentSymbolNames.size() )
        {
            // This is a little trick to word - when saving to another library, "copy parents" makes sense,
            // but when in the same library, the parents will be untouched in any case.
            const wxString aParentNames = AccumulateDescriptions( aParentSymbolNames );
            AddExtraCheckbox(
                    wxString::Format( "Flatten/remove symbol inheritance (current parent symbols: %s)", aParentNames ),
                    &m_params.m_FlattenSymbol );
        }

        aParent->GetLibraryItemsForListDialog( headers, itemsToDisplay );
        initDialog( headers, itemsToDisplay, m_params.m_LibraryName );

        SetListLabel( _( "Save in library:" ) );
        SetOKLabel( _( "Save" ) );

        wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

        wxStaticText* label = new wxStaticText( this, wxID_ANY, _( "Name:" ) );
        bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

        m_symbolNameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString );
        bNameSizer->Add( m_symbolNameCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxButton* newLibraryButton = new wxButton( this, ID_MAKE_NEW_LIBRARY, _( "New Library..." ) );
        m_ButtonsSizer->Prepend( 80, 20 );
        m_ButtonsSizer->Prepend( newLibraryButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

        GetSizer()->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

        Bind( wxEVT_BUTTON,
                [this]( wxCommandEvent& )
                {
                    EndModal( ID_MAKE_NEW_LIBRARY );
                }, ID_MAKE_NEW_LIBRARY );

        // Move nameTextCtrl to the head of the tab-order
        if( GetChildren().DeleteObject( m_symbolNameCtrl ) )
            GetChildren().Insert( m_symbolNameCtrl );

        SetInitialFocus( m_symbolNameCtrl );

        SetupStandardButtons();

        Layout();
        GetSizer()->Fit( this );

        Centre();
    }

protected:
    wxString getSymbolName() const
    {
        wxString symbolName = m_symbolNameCtrl->GetValue();
        symbolName.Trim( true );
        symbolName.Trim( false );
        symbolName.Replace( " ", "_" );
        return EscapeString( symbolName, CTX_LIBID );
    }

    bool TransferDataToWindow() override
    {
        m_symbolNameCtrl->SetValue( UnescapeString( m_params.m_SymbolName ) );
        return true;
    }

    bool TransferDataFromWindow() override
    {
        // This updates m_params.m_FlattenSymbol
        // Do this now, so the validator can use it
        GetExtraCheckboxValues();

        m_params.m_SymbolName = getSymbolName();
        m_params.m_LibraryName = GetTextSelection();

        int ret = m_validator( m_params.m_LibraryName, m_params.m_SymbolName );

        if( ret == wxID_CANCEL )
            return false;

        if( ret == ID_OVERWRITE_CONFLICTS )
            m_params.m_ConflictStrategy = SYMBOL_SAVE_AS_HANDLER::CONFLICT_STRATEGY::OVERWRITE;
        else if( ret == ID_RENAME_CONFLICTS )
            m_params.m_ConflictStrategy = SYMBOL_SAVE_AS_HANDLER::CONFLICT_STRATEGY::RENAME;

        return true;
    }

private:
    wxTextCtrl*         m_symbolNameCtrl;
    SymLibNameValidator m_validator;
    PARAMS&             m_params;
};


void SYMBOL_EDIT_FRAME::saveSymbolCopyAs( bool aOpenCopy )
{
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( !symbol )
        return;

    LIB_ID   old_lib_id = symbol->GetLibId();
    wxString symbolName = old_lib_id.GetLibItemName();
    wxString libraryName = old_lib_id.GetLibNickname();
    bool     valueFollowsName = symbol->GetValueField().GetText() == symbolName;
    wxString msg;
    bool     done = false;
    bool     flattenSymbol = false;

    // This is the function that will be called when the user clicks OK in the dialog and checks
    // if the proposed name has problems, and asks for clarification.
    const auto dialogValidatorFunc =
            [&]( const wxString& newLib, const wxString& newName ) -> int
            {
                if( newLib.IsEmpty() )
                {
                    wxMessageBox( _( "A library must be specified." ) );
                    return wxID_CANCEL;
                }

                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Symbol must have a name." ) );
                    return wxID_CANCEL;
                }

                if( m_libMgr->IsLibraryReadOnly( newLib ) )
                {
                    msg = wxString::Format( _( "Library '%s' is read-only. Choose a "
                                               "different library to save the symbol '%s' to." ),
                                            newLib,
                                            UnescapeString( newName ) );
                    wxMessageBox( msg );
                    return wxID_CANCEL;
                }

                /**
                 * If we save over a symbol that is in the inheritance chain of the symbol we're
                 * saving, we'll end up with a circular inheritance chain, which is bad.
                 */
                const auto& [inAncestry, inDescendents] = CheckSavingIntoOwnInheritance( *m_libMgr, *symbol,
                                                                                         newName, newLib );

                if( inAncestry )
                {
                    msg = wxString::Format( _( "Symbol '%s' cannot replace another symbol '%s' "
                                               "that it descends from" ),
                                            symbolName,
                                            UnescapeString( newName ) );
                    wxMessageBox( msg );
                    return wxID_CANCEL;
                }

                if( inDescendents )
                {
                    msg = wxString::Format( _( "Symbol '%s' cannot replace another symbol '%s' "
                                               "that is a descendent of it." ),
                                            symbolName,
                                            UnescapeString( newName ) );
                    wxMessageBox( msg );
                    return wxID_CANCEL;
                }

                const std::vector<wxString> conflicts =
                        CheckForParentalChainConflicts( *m_libMgr, *symbol, flattenSymbol, newName, newLib );

                if( conflicts.size() == 1 && conflicts.front() == newName )
                {
                    // The simplest case is when the symbol itself has a conflict
                    msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'. "
                                               "Do you want to overwrite it?" ),
                                            UnescapeString( newName ),
                                            newLib );

                    KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK ? ID_OVERWRITE_CONFLICTS : (int) wxID_CANCEL;
                }
                else if( !conflicts.empty() )
                {
                    // If there are conflicts in the parental chain, we need to ask the user
                    // if they want to overwrite all of them.
                    // A more complex UI might allow the user to re-parent the symbol to an
                    // existing symbol in the target lib, or rename all the parents somehow.
                    msg = wxString::Format( _( "The following symbols in the inheritance chain of "
                                               "'%s' already exist in library '%s':\n" ),
                                            UnescapeString( symbolName ),
                                            newLib );

                    for( const wxString& conflict : conflicts )
                        msg += wxString::Format( "  %s\n", conflict );

                    msg += _( "\nDo you want to overwrite all of them, or rename the new symbols?" );

                    KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxYES_NO | wxCANCEL | wxICON_WARNING );
                    errorDlg.SetYesNoCancelLabels( _( "Overwrite All" ), _( "Rename All" ), _( "Cancel" ) );

                    switch( errorDlg.ShowModal() )
                    {
                    case wxID_YES: return ID_OVERWRITE_CONFLICTS;
                    case wxID_NO:  return ID_RENAME_CONFLICTS;
                    default:       return wxID_CANCEL;
                    }
                }

                return wxID_OK;
            };

    auto strategy = SYMBOL_SAVE_AS_HANDLER::CONFLICT_STRATEGY::OVERWRITE;

    std::vector<wxString> parentSymbolNames;
    if( symbol->IsDerived() )
    {
        // The parents are everything but the leaf symbol
        std::vector<std::shared_ptr<LIB_SYMBOL>> parentChain = GetParentChain( *symbol, false );

        for( const auto& parent : parentChain )
            parentSymbolNames.push_back( parent->GetName() );
    }

    SAVE_SYMBOL_AS_DIALOG::PARAMS params{
        symbolName,
        libraryName,
        flattenSymbol,
        strategy,
    };

    // Keep asking the user for a new name until they give a valid one or cancel the operation
    while( !done )
    {
        SAVE_SYMBOL_AS_DIALOG dlg( this, params, dialogValidatorFunc, parentSymbolNames );

        int ret = dlg.ShowModal();

        switch( ret )
        {
        case wxID_CANCEL:
            return;

        case wxID_OK: // No conflicts
        case ID_OVERWRITE_CONFLICTS:
        case ID_RENAME_CONFLICTS:
        {
            done = true;
            break;
        }
        case ID_MAKE_NEW_LIBRARY:
        {
            wxFileName newLibrary( AddLibraryFile( true ) );
            params.m_LibraryName = newLibrary.GetName();

            // Go round again to ask for the symbol name
            break;
        }

        default:
            break;
        }
    }

    SYMBOL_SAVE_AS_HANDLER saver( *m_libMgr, params.m_ConflictStrategy, valueFollowsName );

    saver.DoSave( *symbol, params.m_SymbolName, params.m_LibraryName, params.m_FlattenSymbol );

    SyncLibraries( false );

    if( aOpenCopy )
        LoadSymbol( params.m_SymbolName, params.m_LibraryName, 1 );
}


void SYMBOL_EDIT_FRAME::ExportSymbol()
{
    wxString msg;
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( !symbol )
    {
        ShowInfoBarError( _( "There is no symbol selected to save." ) );
        return;
    }

    wxFileName fn;

    fn.SetName( symbol->GetName().Lower() );
    fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

    wxFileDialog dlg( this, _( "Export Symbol" ), m_mruPath, fn.GetFullName(),
                      FILEEXT::KiCadSymbolLibFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    auto strategy = SYMBOL_SAVE_AS_HANDLER::CONFLICT_STRATEGY::OVERWRITE;

    fn = dlg.GetPath();
    fn.MakeAbsolute();

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    wxString                    libraryName;
    std::unique_ptr<LIB_SYMBOL> flattenedSymbol = symbol->Flatten();

    for( const wxString& candidate : m_libMgr->GetLibraryNames() )
    {
        if( auto uri = manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, candidate, true ); uri )
        {
            if( *uri == fn.GetFullPath() )
                libraryName = candidate;
        }
    }

    if( !libraryName.IsEmpty() )
    {
        SYMBOL_SAVE_AS_HANDLER saver( *m_libMgr, strategy, false );

        if( m_libMgr->IsLibraryReadOnly( libraryName ) )
        {
            msg = wxString::Format( _( "Library '%s' is read-only." ), libraryName );
            DisplayError( this, msg );
            return;
        }

        if( m_libMgr->SymbolNameInUse( symbol->GetName(), libraryName ) )
        {
            msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'." ),
                                    symbol->GetName(), libraryName );

            KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() == wxID_CANCEL )
                return;
        }

        saver.DoSave( *flattenedSymbol, symbol->GetName(), libraryName, false );

        SyncLibraries( false );
        return;
    }

    LIB_SYMBOL* old_symbol = nullptr;
    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

    if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        pluginType = SCH_IO_MGR::SCH_KICAD;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pluginType ) );

    if( fn.FileExists() )
    {
        try
        {
            old_symbol = pi->LoadSymbol( fn.GetFullPath(), symbol->GetName() );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error occurred attempting to load symbol library file '%s'." ),
                        fn.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
            return;
        }

        if( old_symbol )
        {
            msg.Printf( _( "Symbol %s already exists in library '%s'." ),
                        UnescapeString( symbol->GetName() ),
                        fn.GetFullName() );

            KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() == wxID_CANCEL )
                return;
        }
    }

    if( !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save library '%s'." ), fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    try
    {
        if( !fn.FileExists() )
            pi->CreateLibrary( fn.GetFullPath() );

        // The flattened symbol is most likely what the user would want.  As some point in
        // the future as more of the symbol library inheritance is implemented, this may have
        // to be changes to save symbols of inherited symbols.
        pi->SaveSymbol( fn.GetFullPath(), flattenedSymbol.release() );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Failed to create symbol library file '%s'." ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
        msg.Printf( _( "Error creating symbol library '%s'." ), fn.GetFullName() );
        SetStatusText( msg );
        return;
    }

    m_mruPath = fn.GetPath();

    msg.Printf( _( "Symbol %s saved to library '%s'." ),
                UnescapeString( symbol->GetName() ),
                fn.GetFullPath() );
    SetStatusText( msg );
}


void SYMBOL_EDIT_FRAME::UpdateAfterSymbolProperties( wxString* aOldName )
{
    wxCHECK( m_symbol, /* void */ );

    wxString lib = GetCurLib();

    if( !lib.IsEmpty() && aOldName && *aOldName != m_symbol->GetName() )
    {
        // Test the current library for name conflicts
        if( m_libMgr->SymbolNameInUse( m_symbol->GetName(), lib ) )
        {
            wxString msg = wxString::Format( _( "Symbol name '%s' already in use." ),
                                             UnescapeString( m_symbol->GetName() ) );

            DisplayErrorMessage( this, msg );
            m_symbol->SetName( *aOldName );
        }
        else
        {
            m_libMgr->UpdateSymbolAfterRename( m_symbol, *aOldName, lib );
        }

        // Reselect the renamed symbol
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, m_symbol->GetName() ) );
    }

    RebuildSymbolUnitAndBodyStyleLists();
    UpdateTitle();

    // N.B. The view needs to be rebuilt first as the Symbol Properties change may invalidate
    // the view pointers by rebuilting the field table
    RebuildView();
    UpdateMsgPanel();

    OnModify();
}


void SYMBOL_EDIT_FRAME::DeleteSymbolFromLibrary()
{
    std::vector<LIB_ID> toDelete = GetSelectedLibIds();

    if( toDelete.empty() )
        toDelete.emplace_back( GetTargetLibId() );

    for( LIB_ID& libId : toDelete )
    {
        if( m_libMgr->IsSymbolModified( libId.GetLibItemName(), libId.GetLibNickname() )
            && !IsOK( this, wxString::Format( _( "The symbol '%s' has been modified.\n"
                                                 "Do you want to remove it from the library?" ),
                                              libId.GetUniStringLibItemName() ) ) )
        {
            continue;
        }

        wxArrayString derived;

        if( m_libMgr->GetDerivedSymbolNames( libId.GetLibItemName(), libId.GetLibNickname(), derived ) > 0 )
        {
            wxString msg = _( "Deleting a base symbol will delete all symbols derived from it.\n\n" );

            msg += libId.GetLibItemName().wx_str() + _( " (base)\n" );

            for( const wxString& name : derived )
                msg += name + wxT( "\n" );

            KICAD_MESSAGE_DIALOG_BASE dlg( this, msg, _( "Warning" ), wxYES_NO | wxICON_WARNING | wxCENTER );
            dlg.SetExtendedMessage( wxT( " " ) );
            dlg.SetYesNoLabels( _( "Delete All Listed Symbols" ), _( "Cancel" ) );

            if( dlg.ShowModal() == wxID_NO )
                continue;
        }

        if( GetCurSymbol() )
        {
            for( const std::shared_ptr<LIB_SYMBOL>& symbol : GetParentChain( *GetCurSymbol() ) )
            {
                if( symbol->GetLibId() == libId )
                {
                    emptyScreen();
                    break;
                }
            }
        }

        m_libMgr->RemoveSymbol( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::CopySymbolToClipboard()
{
    std::vector<LIB_ID> symbols;

    if( GetTreeLIBIDs( symbols ) == 0 )
        return;

    STRING_FORMATTER formatter;

    for( LIB_ID& libId : symbols )
    {
        LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(),
                                                          libId.GetLibNickname() );

        if( !symbol )
            continue;

        std::unique_ptr<LIB_SYMBOL> tmp = symbol->Flatten();
        SCH_IO_KICAD_SEXPR::FormatLibSymbol( tmp.get(), formatter );
    }

    std::string prettyData = formatter.GetString();
    KICAD_FORMAT::Prettify( prettyData, KICAD_FORMAT::FORMAT_MODE::COMPACT_TEXT_PROPERTIES );

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    auto data = new wxTextDataObject( wxString( prettyData.c_str(), wxConvUTF8 ) );
    clipboard->SetData( data );

    clipboard->Flush();
}


void SYMBOL_EDIT_FRAME::DuplicateSymbol( bool aFromClipboard )
{
    LIB_ID libId = GetTargetLibId();
    wxString lib = libId.GetLibNickname();

    if( !m_libMgr->LibraryExists( lib ) )
        return;

    std::vector<LIB_SYMBOL*> newSymbols;

    if( aFromClipboard )
    {
        std::string clipboardData = GetClipboardUTF8();

        try
        {
            newSymbols = SCH_IO_KICAD_SEXPR::ParseLibSymbols( clipboardData, "Clipboard" );
        }
        catch( IO_ERROR& e )
        {
            wxLogMessage( wxS( "Can not paste: %s" ), e.Problem() );
        }
    }
    else if( LIB_SYMBOL* srcSymbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(), lib ) )
    {
        newSymbols.emplace_back( new LIB_SYMBOL( *srcSymbol ) );

        // Derive from same parent.
        if( srcSymbol->IsDerived() )
        {
            if( std::shared_ptr<LIB_SYMBOL> srcParent = srcSymbol->GetParent().lock() )
                newSymbols.back()->SetParent( srcParent.get() );
        }
    }

    if( newSymbols.empty() )
        return;

    for( LIB_SYMBOL* symbol : newSymbols )
    {
        ensureUniqueName( symbol, lib );
        m_libMgr->UpdateSymbol( symbol, lib );

        LoadOneLibrarySymbolAux( symbol, lib, GetUnit(), GetBodyStyle() );
    }

    SyncLibraries( false );
    m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, newSymbols[0]->GetName() ) );

    for( LIB_SYMBOL* symbol : newSymbols )
        delete symbol;
}


void SYMBOL_EDIT_FRAME::ensureUniqueName( LIB_SYMBOL* aSymbol, const wxString& aLibrary )
{
    if( aSymbol )
    {
        int      i = 1;
        wxString newName = aSymbol->GetName();

        // Append a number to the name until the name is unique in the library.
        while( m_libMgr->SymbolNameInUse( newName, aLibrary ) )
            newName.Printf( "%s_%d", aSymbol->GetName(), i++ );

        aSymbol->SetName( newName );
    }
}


void SYMBOL_EDIT_FRAME::Revert( bool aConfirm )
{
    LIB_ID libId = GetTargetLibId();
    const wxString& libName = libId.GetLibNickname();

    // Empty if this is the library itself that is selected.
    const wxString& symbolName = libId.GetLibItemName();

    wxString msg = wxString::Format( _( "Revert '%s' to last version saved?" ),
                                     symbolName.IsEmpty() ? libName : symbolName );

    if( aConfirm && !ConfirmRevertDialog( this, msg ) )
        return;

    bool reload_currentSymbol = false;
    wxString curr_symbolName = symbolName;

    if( GetCurSymbol() )
    {
        // the library itself is reverted: the current symbol will be reloaded only if it is
        // owned by this library
        if( symbolName.IsEmpty() )
        {
            LIB_ID curr_libId = GetCurSymbol()->GetLibId();
            reload_currentSymbol = libName == curr_libId.GetLibNickname().wx_str();

            if( reload_currentSymbol )
                curr_symbolName = curr_libId.GetUniStringLibItemName();
        }
        else
        {
            reload_currentSymbol = IsCurrentSymbol( libId );
        }
    }

    int unit = m_unit;

    if( reload_currentSymbol )
        emptyScreen();

    if( symbolName.IsEmpty() )
    {
        m_libMgr->RevertLibrary( libName );
    }
    else
    {
        libId = m_libMgr->RevertSymbol( libId.GetLibItemName(), libId.GetLibNickname() );

        m_treePane->GetLibTree()->SelectLibId( libId );
        m_libMgr->ClearSymbolModified( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    if( reload_currentSymbol && m_libMgr->SymbolExists( curr_symbolName, libName ) )
        LoadSymbol( curr_symbolName, libName, unit );

    m_treePane->Refresh();
}


void SYMBOL_EDIT_FRAME::RevertAll()
{
    wxCHECK_RET( m_libMgr, "Library manager object not created." );

    Revert( false );
    m_libMgr->RevertAll();
}


void SYMBOL_EDIT_FRAME::LoadSymbol( const wxString& aAlias, const wxString& aLibrary, int aUnit )
{
    if( GetCurSymbol() && IsSymbolFromSchematic() && GetScreen()->IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current symbol has been modified.  Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return saveCurrentSymbol();
                                   } ) )
        {
            return;
        }
    }

    LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( aAlias, aLibrary );

    if( !symbol )
    {
        DisplayError( this, wxString::Format( _( "Symbol %s not found in library '%s'." ),
                                              aAlias,
                                              aLibrary ) );
        m_treePane->GetLibTree()->RefreshLibTree();
        return;
    }

    // Optimize default edit options for this symbol
    // Usually if units are locked, graphic items are specific to each unit
    // and if units are interchangeable, graphic items are common to units
    SYMBOL_EDITOR_DRAWING_TOOLS* tools = GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    tools->SetDrawSpecificUnit( symbol->UnitsLocked() );

    LoadOneLibrarySymbolAux( symbol, aLibrary, aUnit, 0 );
}


bool SYMBOL_EDIT_FRAME::saveLibrary( const wxString& aLibrary, bool aNewFile )
{
    wxFileName fn;
    wxString   msg;
    SYMBOL_SAVEAS_TYPE     type = SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS;
    SCH_IO_MGR::SCH_FILE_T fileType = SCH_IO_MGR::SCH_FILE_T::SCH_KICAD;
    PROJECT& prj = Prj();

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &prj );

    m_toolManager->RunAction( ACTIONS::cancelInteractive );

    if( !aNewFile && ( aLibrary.empty() || !adapter->HasLibrary( aLibrary ) ) )
    {
        ShowInfoBarError( _( "No library specified." ) );
        return false;
    }

    if( aNewFile )
    {
        SEARCH_STACK* search = PROJECT_SCH::SchSearchS( &prj );

        // Get a new name for the library
        wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

        if( !default_path )
            default_path = search->LastVisitedPath();

        fn.SetName( aLibrary );
        fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

        wxString wildcards = FILEEXT::KiCadSymbolLibFileWildcard();

        wxFileDialog dlg( this, wxString::Format( _( "Save Library '%s' As..." ), aLibrary ), default_path,
                          fn.GetFullName(), wildcards, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        SYMBOL_LIBRARY_SAVE_AS_FILEDLG_HOOK saveAsHook( type );
        dlg.SetCustomizeHook( saveAsHook );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        if( fn.GetExt().IsEmpty() )
            fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

        type = saveAsHook.GetOption();
    }
    else
    {
        std::optional<LIBRARY_TABLE_ROW*> optRow = adapter->GetRow( aLibrary );
        wxCHECK( optRow, false );

        fn = LIBRARY_MANAGER::GetFullURI( *optRow, true );
        fileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

        if( fileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
            fileType = SCH_IO_MGR::SCH_KICAD;
    }

    // Verify the user has write privileges before attempting to save the library file.
    if( !aNewFile && m_libMgr->IsLibraryReadOnly( aLibrary ) )
        return false;

    ClearMsgPanel();

    // Copy .kicad_symb file to .bak.
    if( !backupFile( fn, "bak" ) )
        return false;

    if( !m_libMgr->SaveLibrary( aLibrary, fn.GetFullPath(), fileType ) )
    {
        msg.Printf( _( "Failed to save changes to symbol library file '%s'." ),
                    fn.GetFullPath() );
        DisplayErrorMessage( this, _( "Error Saving Library" ), msg );
        return false;
    }

    if( !aNewFile )
    {
        m_libMgr->ClearLibraryModified( aLibrary );

        // Update the library modification time so that we don't reload based on the watcher
        if( aLibrary == getTargetLib() )
            SetSymModificationTime( fn.GetModificationTime() );
    }
    else
    {
        bool resyncLibTree = false;
        wxString originalLibNickname = getTargetLib();
        wxString forceRefresh;

        switch( type )
        {
        case SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY:
            resyncLibTree = replaceLibTableEntry( originalLibNickname, fn.GetFullPath() );
            forceRefresh = originalLibNickname;
            break;

        case SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath() );
            break;

        case SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath(), LIBRARY_TABLE_SCOPE::PROJECT );
            break;

        default:
            break;
        }

        if( resyncLibTree )
        {
            FreezeLibraryTree();
            SyncLibraries( true, false, forceRefresh );
            ThawLibraryTree();
        }
    }

    ClearMsgPanel();
    msg.Printf( _( "Symbol library file '%s' saved." ), fn.GetFullPath() );
    RebuildSymbolUnitAndBodyStyleLists();

    return true;
}


bool SYMBOL_EDIT_FRAME::saveAllLibraries( bool aRequireConfirmation )
{
    wxString msg, msg2;
    bool     doSave = true;
    int      dirtyCount = 0;
    bool     applyToAll = false;
    bool     retv = true;

    for( const wxString& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
            dirtyCount++;
    }

    for( const wxString& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
        {
            if( aRequireConfirmation && !applyToAll )
            {
                msg.Printf( _( "Save changes to '%s' before closing?" ), libNickname );

                switch( UnsavedChangesDialog( this, msg, dirtyCount > 1 ? &applyToAll : nullptr ) )
                {
                case wxID_YES: doSave = true;  break;
                case wxID_NO:  doSave = false; break;
                default:
                case wxID_CANCEL: return false;
                }
            }

            if( doSave )
            {
                // If saving under existing name fails then do a Save As..., and if that
                // fails then cancel close action.
                if( m_libMgr->IsLibraryReadOnly( libNickname ) )
                {
                    msg.Printf( _( "Symbol library '%s' is not writable." ), libNickname );
                    msg2 = _( "You must save to a different location." );

                    if( dirtyCount == 1 )
                    {
                        if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) != wxID_OK )
                        {
                            retv = false;
                            continue;
                        }
                    }
                    else
                    {
                        m_infoBar->Dismiss();
                        m_infoBar->ShowMessageFor( msg + wxS( "  " ) + msg2,
                                                   2000, wxICON_EXCLAMATION );

                        while( m_infoBar->IsShownOnScreen() )
                            wxSafeYield();

                        retv = false;
                        continue;
                    }
                }
                else if( saveLibrary( libNickname, false ) )
                {
                    continue;
                }

                if( !saveLibrary( libNickname, true ) )
                    retv = false;
            }
        }
    }

    UpdateTitle();
    return retv;
}


void SYMBOL_EDIT_FRAME::UpdateSymbolMsgPanelInfo()
{
    EDA_DRAW_FRAME::ClearMsgPanel();

    if( !m_symbol )
        return;

    wxString msg = m_symbol->GetName();

    AppendMsgPanel( _( "Name" ), UnescapeString( msg ), 8 );

    if( m_symbol->IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> parent = m_symbol->GetParent().lock();

        msg = parent ? parent->GetName() : _( "Undefined!" );
        AppendMsgPanel( _( "Parent" ), UnescapeString( msg ), 8 );
    }

    if( m_symbol->IsGlobalPower() )
        msg = _( "Power Symbol" );
    else if( m_symbol->IsLocalPower() )
        msg = _( "Power Symbol (Local)" );
    else
        msg = _( "Symbol" );

    AppendMsgPanel( _( "Type" ), msg, 8 );
    AppendMsgPanel( _( "Description" ), m_symbol->GetDescription(), 8 );
    AppendMsgPanel( _( "Keywords" ), m_symbol->GetKeyWords() );
    AppendMsgPanel( _( "Datasheet" ), m_symbol->GetDatasheetField().GetText() );
}
