/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <core/kicad_algo.h>
#include <symbol_library_common.h>
#include <confirm.h>
#include <eeschema_id.h>
#include <general.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_symbol.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

#include <dialog_choose_symbol.h>

PICKED_SYMBOL SCH_BASE_FRAME::PickSymbolFromLibBrowser( wxTopLevelWindow* aParent,
                                                        const SYMBOL_LIBRARY_FILTER* aFilter,
                                                        const LIB_ID& aPreselectedLibId,
                                                        int aUnit, int aConvert )
{
    // Close any open non-modal Lib browser, and open a new one, in "modal" mode:
    SYMBOL_VIEWER_FRAME* viewer = (SYMBOL_VIEWER_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

    if( viewer )
        viewer->Destroy();

    viewer = (SYMBOL_VIEWER_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, aParent );

    if( aFilter )
        viewer->SetFilter( aFilter );

    if( aPreselectedLibId.IsValid() )
    {
        viewer->SetSelectedLibrary( aPreselectedLibId.GetLibNickname() );
        viewer->SetSelectedSymbol( aPreselectedLibId.GetLibItemName());
    }

    viewer->SetUnitAndConvert( aUnit, aConvert );

    viewer->Refresh();

    PICKED_SYMBOL sel;
    wxString      symbol;

    if( viewer->ShowModal( &symbol, aParent ) )
    {
        LIB_ID id;

        if( id.Parse( symbol ) == -1 )
            sel.LibId = id;

        sel.Unit = viewer->GetUnit();
        sel.Convert = viewer->GetConvert();
    }

    viewer->Destroy();

    return sel;
}


PICKED_SYMBOL SCH_BASE_FRAME::PickSymbolFromLibTree( const SYMBOL_LIBRARY_FILTER* aFilter,
                                                     std::vector<PICKED_SYMBOL>& aHistoryList,
                                                     bool aUseLibBrowser, int aUnit, int aConvert,
                                                     bool aShowFootprints, const LIB_ID* aHighlight,
                                                     bool aAllowFields )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_SYMBOL::g_Mutex, std::defer_lock );
    SYMBOL_LIB_TABLE*            libs = Prj().SchSymbolLibTable();
    COMMON_SETTINGS*             cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&                project = Prj().GetProjectFile();

    // One DIALOG_CHOOSE_SYMBOL dialog at a time.  User probably can't handle more anyway.
    if( !dialogLock.try_lock() )
        return PICKED_SYMBOL();

    // Make sure settings are loaded before we start running multi-threaded symbol loaders
    Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();


    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> dataPtr
                                    = SYMBOL_TREE_MODEL_ADAPTER::Create( this, libs );
    SYMBOL_TREE_MODEL_ADAPTER* modelAdapter
                                    = static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( dataPtr.get() );
    bool loaded = false;

    if( aFilter )
    {
        const wxArrayString& liblist = aFilter->GetAllowedLibList();

        for( const wxString& nickname : liblist )
        {
            if( libs->HasLibrary( nickname, true ) )
            {
                loaded = true;

                bool pinned = alg::contains( cfg->m_Session.pinned_symbol_libs, nickname )
                                || alg::contains( project.m_PinnedSymbolLibs, nickname );

                if( libs->FindRow( nickname )->GetIsVisible() )
                    modelAdapter->AddLibrary( nickname, pinned );
            }
        }

        modelAdapter->AssignIntrinsicRanks();

        if( aFilter->GetFilterPowerSymbols() )
            modelAdapter->SetFilter( SYMBOL_TREE_MODEL_ADAPTER::SYM_FILTER_POWER );
    }

    std::vector<LIB_SYMBOL>     history_list_storage;
    std::vector<LIB_TREE_ITEM*> history_list;

    history_list_storage.reserve( aHistoryList.size() );

    for( const PICKED_SYMBOL& i : aHistoryList )
    {
        LIB_SYMBOL* symbol = GetLibSymbol( i.LibId );

        // This can be null, for example when a symbol has been deleted from a library
        if( symbol )
        {
            history_list_storage.emplace_back( *symbol );

            for( const std::pair<int, wxString>& fieldDef : i.Fields )
            {
                LIB_FIELD* field = history_list_storage.back().GetFieldById( fieldDef.first );

                if( field )
                    field->SetText( fieldDef.second );
            }

            history_list.push_back( &history_list_storage.back() );
        }
    }

    modelAdapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                                history_list, false, true );

    if( !aHistoryList.empty() )
        modelAdapter->SetPreselectNode( aHistoryList[0].LibId, aHistoryList[0].Unit );

    const std::vector< wxString > libNicknames = libs->GetLogicalLibs();

    if( !loaded )
    {
        if( !modelAdapter->AddLibraries( libNicknames, this ) )
        {
            // loading cancelled by user
            return PICKED_SYMBOL();
        }
    }

    if( aHighlight && aHighlight->IsValid() )
        modelAdapter->SetPreselectNode( *aHighlight, /* aUnit */ 0 );

    wxString dialogTitle;

    if( modelAdapter->GetFilter() == SYMBOL_TREE_MODEL_ADAPTER::SYM_FILTER_POWER )
        dialogTitle.Printf( _( "Choose Power Symbol (%d items loaded)" ), dataPtr->GetItemCount() );
    else
        dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), dataPtr->GetItemCount() );

    DIALOG_CHOOSE_SYMBOL dlg( this, dialogTitle, dataPtr, aConvert, aAllowFields, aShowFootprints,
                              aUseLibBrowser );

    int ret = dlg.ShowModal();

    // Save any changes to column widths, etc.
    modelAdapter->SaveSettings();

    if( ret == wxID_CANCEL )
        return PICKED_SYMBOL();

    PICKED_SYMBOL sel;
    LIB_ID id = dlg.GetSelectedLibId( &sel.Unit );

    if( dlg.IsExternalBrowserSelected() )   // User requested symbol browser.
    {
        sel = PickSymbolFromLibBrowser( this, aFilter, id, sel.Unit, sel.Convert );
        id = sel.LibId;
    }

    if( !id.IsValid() )     // Dialog closed by OK button,
                            // or the selection by lib browser was requested,
                            // but no symbol selected
        return PICKED_SYMBOL();

    if( sel.Unit == 0 )
        sel.Unit = 1;

    sel.Fields = dlg.GetFields();
    sel.LibId = id;

    if( sel.LibId.IsValid() )
    {
        alg::delete_if( aHistoryList, [&sel]( PICKED_SYMBOL const& i )
                                      {
                                          return i.LibId == sel.LibId;
                                      } );

        aHistoryList.insert( aHistoryList.begin(), sel );
    }

    return sel;
}


void SCH_EDIT_FRAME::SelectUnit( SCH_SYMBOL* aSymbol, int aUnit )
{
    SCH_COMMIT  commit( m_toolManager );
    LIB_SYMBOL* symbol = GetLibSymbol( aSymbol->GetLibId() );

    if( !symbol )
        return;

    int unitCount = symbol->GetUnitCount();

    if( unitCount <= 1 || aSymbol->GetUnit() == aUnit )
        return;

    if( aUnit > unitCount )
        aUnit = unitCount;

    if( !aSymbol->GetEditFlags() )    // No command in progress: save in undo list
        commit.Modify( aSymbol, GetScreen() );

    /* Update the unit number. */
    aSymbol->SetUnitSelection( &GetCurrentSheet(), aUnit );
    aSymbol->SetUnit( aUnit );

    if( !commit.Empty() )
    {
        if( eeconfig()->m_AutoplaceFields.enable )
            aSymbol->AutoAutoplaceFields( GetScreen() );

        commit.Push( _( "Change Unit" ) );
    }
}


void SCH_EDIT_FRAME::ConvertPart( SCH_SYMBOL* aSymbol )
{
    if( !aSymbol || !aSymbol->GetLibSymbolRef() )
        return;

    SCH_COMMIT commit( m_toolManager );
    wxString   msg;

    if( !aSymbol->GetLibSymbolRef()->HasConversion() )
    {
        LIB_ID id = aSymbol->GetLibSymbolRef()->GetLibId();

        msg.Printf( _( "No alternate body style found for symbol '%s' in library '%s'." ),
                    id.GetLibItemName().wx_str(),
                    id.GetLibNickname().wx_str() );
        DisplayError( this,  msg );
        return;
    }

    commit.Modify( aSymbol, GetScreen() );

    aSymbol->SetConvert( aSymbol->GetConvert() + 1 );

    // ensure m_convert = 1 or 2
    // 1 = shape 1 = not converted
    // 2 = shape 2 = first converted shape
    // > 2 is not currently supported
    // When m_convert = val max, return to the first shape
    if( aSymbol->GetConvert() > LIB_ITEM::LIB_CONVERT::DEMORGAN )
        aSymbol->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

    // If selected make sure all the now-included pins are selected
    if( aSymbol->IsSelected() )
        m_toolManager->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, aSymbol );

    commit.Push( _( "Convert Symbol" ) );
}
