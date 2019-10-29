/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <fctsys.h>
#include <pgm_base.h>
#include <kiway.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <msgpanel.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <general.h>
#include <class_library.h>
#include <sch_component.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>
#include <symbol_lib_table.h>

#include <dialog_choose_component.h>
#include <symbol_tree_model_adapter.h>


SCH_BASE_FRAME::COMPONENT_SELECTION SCH_BASE_FRAME::SelectComponentFromLibBrowser(
        wxTopLevelWindow* aParent, const SCHLIB_FILTER* aFilter, const LIB_ID& aPreselectedLibId,
        int aUnit, int aConvert )
{
    // Close any open non-modal Lib browser, and open a new one, in "modal" mode:
    LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

    if( viewlibFrame )
        viewlibFrame->Destroy();

    viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, aParent );

    if( aFilter )
        viewlibFrame->SetFilter( aFilter );

    if( aPreselectedLibId.IsValid() )
    {
        viewlibFrame->SetSelectedLibrary( aPreselectedLibId.GetLibNickname() );
        viewlibFrame->SetSelectedComponent( aPreselectedLibId.GetLibItemName() );
    }

    viewlibFrame->SetUnitAndConvert( aUnit, aConvert );

    viewlibFrame->Refresh();

    COMPONENT_SELECTION sel;
    wxString            symbol;

    if( viewlibFrame->ShowModal( &symbol, aParent ) )
    {
        LIB_ID id;

        if( id.Parse( symbol, LIB_ID::ID_SCH ) == -1 )
            sel.LibId = id;

        sel.Unit = viewlibFrame->GetUnit();
        sel.Convert = viewlibFrame->GetConvert();
    }

    viewlibFrame->Destroy();

    return sel;
}


SCH_BASE_FRAME::COMPONENT_SELECTION SCH_BASE_FRAME::SelectCompFromLibTree(
        const SCHLIB_FILTER* aFilter,
        std::vector<COMPONENT_SELECTION>& aHistoryList,
        bool aUseLibBrowser,
        int aUnit,
        int aConvert,
        bool aShowFootprints,
        const LIB_ID* aHighlight,
        bool aAllowFields )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_COMPONENT::g_Mutex, std::defer_lock );
    wxString                     dialogTitle;
    SYMBOL_LIB_TABLE*            libs = Prj().SchSymbolLibTable();

    // One CHOOSE_COMPONENT dialog at a time.  User probaby can't handle more anyway.
    if( !dialogLock.try_lock() )
        return COMPONENT_SELECTION();

    auto adapterPtr( SYMBOL_TREE_MODEL_ADAPTER::Create( libs ) );
    auto adapter = static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( adapterPtr.get() );
    bool loaded = false;

    if( aFilter )
    {
        const wxArrayString& liblist = aFilter->GetAllowedLibList();

        for( unsigned ii = 0; ii < liblist.GetCount(); ii++ )
        {
            if( libs->HasLibrary( liblist[ii], true ) )
            {
                loaded = true;
                adapter->AddLibrary( liblist[ii] );
            }
        }

        adapter->AssignIntrinsicRanks();

        if( aFilter->GetFilterPowerParts() )
            adapter->SetFilter( SYMBOL_TREE_MODEL_ADAPTER::CMP_FILTER_POWER );
    }

    std::vector< LIB_TREE_ITEM* > history_list;

    for( auto const& i : aHistoryList )
    {
        LIB_ALIAS* alias = GetLibAlias( i.LibId );

        // This can be null, for example when a symbol has been deleted from a library
        if( alias )
            history_list.push_back( alias );
    }

    adapter->DoAddLibrary( "-- " + _( "Recently Used" ) + " --", wxEmptyString, history_list, true );

    if( !aHistoryList.empty() )
        adapter->SetPreselectNode( aHistoryList[0].LibId, aHistoryList[0].Unit );

    const std::vector< wxString > libNicknames = libs->GetLogicalLibs();

    if( !loaded )
        adapter->AddLibraries( libNicknames, this );

    if( aHighlight && aHighlight->IsValid() )
        adapter->SetPreselectNode( *aHighlight, /* aUnit */ 0 );

    if( adapter->GetFilter() == SYMBOL_TREE_MODEL_ADAPTER::CMP_FILTER_POWER )
        dialogTitle.Printf( _( "Choose Power Symbol (%d items loaded)" ), adapter->GetItemCount() );
    else
        dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), adapter->GetItemCount() );

    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, adapterPtr, aConvert,
                                 aAllowFields, aShowFootprints, aUseLibBrowser );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return COMPONENT_SELECTION();

    COMPONENT_SELECTION sel;
    LIB_ID id = dlg.GetSelectedLibId( &sel.Unit );

    if( dlg.IsExternalBrowserSelected() )   // User requested component browser.
    {
        sel = SelectComponentFromLibBrowser( this, aFilter, id, sel.Unit, sel.Convert );
        id = sel.LibId;
    }

    if( !id.IsValid() )     // Dialog closed by OK button,
                            // or the selection by lib browser was requested,
                            // but no symbol selected
        return COMPONENT_SELECTION();

    SetUseAllUnits( dlg.GetUseAllUnits() );
    SetRepeatComponent( dlg.GetKeepSymbol() );

    if( sel.Unit == 0 )
        sel.Unit = 1;

    sel.Fields = dlg.GetFields();
    sel.LibId = id;

    if( sel.LibId.IsValid() )
    {
        aHistoryList.erase(
            std::remove_if(
                aHistoryList.begin(),
                aHistoryList.end(),
                [ &sel ]( COMPONENT_SELECTION const& i ){ return i.LibId == sel.LibId; } ),
            aHistoryList.end() );

        aHistoryList.insert( aHistoryList.begin(), sel );
    }

    return sel;
}


void SCH_EDIT_FRAME::SelectUnit( SCH_COMPONENT* aComponent, int aUnit )
{
    LIB_PART* part = GetLibPart( aComponent->GetLibId() );

    if( !part )
        return;

    int unitCount = part->GetUnitCount();

    if( unitCount <= 1 || aComponent->GetUnit() == aUnit )
        return;

    if( aUnit > unitCount )
        aUnit = unitCount;

    STATUS_FLAGS savedFlags = aComponent->GetFlags();

    if( !aComponent->GetEditFlags() )    // No command in progress: save in undo list
        SaveCopyInUndoList( aComponent, UR_CHANGED );

    /* Update the unit number. */
    aComponent->SetUnitSelection( g_CurrentSheet, aUnit );
    aComponent->SetUnit( aUnit );
    aComponent->ClearFlags();
    aComponent->SetFlags( savedFlags ); // Restore m_Flag modified by SetUnit()

    if( !aComponent->GetEditFlags() )   // No command in progress: update schematic
    {
        if( m_autoplaceFields )
            aComponent->AutoAutoplaceFields( GetScreen() );

        TestDanglingEnds();

        RefreshItem( aComponent );
        OnModify();
    }
}


void SCH_EDIT_FRAME::ConvertPart( SCH_COMPONENT* aComponent )
{
    if( !aComponent )
        return;

    LIB_ID id = aComponent->GetLibId();
    LIB_PART* part = GetLibPart( id );

    if( part )
    {
        wxString msg;

        if( !part->HasConversion() )
        {
            msg.Printf( _( "No alternate body style found for symbol \"%s\" in library \"%s\"." ),
                        id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );
            DisplayError( this,  msg );
            return;
        }

        STATUS_FLAGS savedFlags = aComponent->GetFlags();

        aComponent->SetConvert( aComponent->GetConvert() + 1 );

        // ensure m_Convert = 1 or 2
        // 1 = shape 1 = not converted
        // 2 = shape 2 = first converted shape
        // > 2 is not used but could be used for more shapes
        // like multiple shapes for a programmable component
        // When m_Convert = val max, return to the first shape
        if( aComponent->GetConvert() > LIB_ITEM::LIB_CONVERT::DEMORGAN )
            aComponent->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

        // The alternate symbol may cause a change in the connection status so test the
        // connections so the connection indicators are drawn correctly.
        aComponent->UpdatePins();
        TestDanglingEnds();
        aComponent->ClearFlags();
        aComponent->SetFlags( savedFlags );   // Restore m_Flags (modified by SetConvert())

        // If selected make sure all the now-included pins are selected
        if( aComponent->IsSelected() )
            m_toolManager->RunAction( EE_ACTIONS::addItemToSel, true, aComponent );

        RefreshItem( aComponent );
        OnModify();
    }
}
