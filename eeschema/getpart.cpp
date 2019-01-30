/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file getpart.cpp
 * @brief functions to get and place library components.
 */

#include <algorithm>
#include <fctsys.h>
#include <pgm_base.h>
#include <kiway.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <kicad_device_context.h>
#include <msgpanel.h>

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
        wxTopLevelWindow* aParent,
        const SCHLIB_FILTER* aFilter, const LIB_ID& aPreselectedLibId,
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

    wxString symbol = sel.LibId.Format();

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


SCH_BASE_FRAME::COMPONENT_SELECTION SCH_BASE_FRAME::SelectComponentFromLibTree(
        const SCHLIB_FILTER*                aFilter,
        std::vector<COMPONENT_SELECTION>&   aHistoryList,
        bool                                aAllowBrowser,
        int                                 aUnit,
        int                                 aConvert,
        bool                                aShowFootprints,
        const LIB_ID*                       aHighlight,
        bool                                aAllowFields )
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
                                 aAllowFields, aShowFootprints, aAllowBrowser );

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


SCH_COMPONENT* SCH_EDIT_FRAME::Load_Component( const SCHLIB_FILTER*           aFilter,
                                               SCH_BASE_FRAME::HISTORY_LIST&  aHistoryList,
                                               bool                           aAllowBrowser )
{
    wxString msg;

    SetRepeatItem( NULL );
    m_canvas->SetIgnoreMouseEvents( true );

    auto sel = SelectComponentFromLibTree( aFilter, aHistoryList, aAllowBrowser, 1, 1,
                                           m_footprintPreview );

    if( !sel.LibId.IsValid() )
    {
        m_canvas->SetIgnoreMouseEvents( false );
        m_canvas->MoveCursorToCrossHair();
        return NULL;
    }

    m_canvas->SetIgnoreMouseEvents( false );
    m_canvas->MoveCursorToCrossHair();

    wxString libsource;     // the library name to use. If empty, load from any lib

    if( aFilter )
        libsource = aFilter->GetLibSource();

    LIB_ID libId = sel.LibId;

    LIB_PART* part = GetLibPart( libId, true );

    if( !part )
        return NULL;

    SCH_COMPONENT* component = new SCH_COMPONENT( *part, libId, m_CurrentSheet,
                                                  sel.Unit, sel.Convert,
                                                  GetCrossHairPosition(), true );

    // Be sure the link to the corresponding LIB_PART is OK:
    component->Resolve( *Prj().SchSymbolLibTable() );

    // Set any fields that have been modified
    for( auto const& i : sel.Fields )
    {
        auto field = component->GetField( i.first );

        if( field )
            field->SetText( i.second );
    }

    MSG_PANEL_ITEMS items;

    component->SetCurrentSheetPath( &GetCurrentSheet() );
    component->GetMsgPanelInfo( m_UserUnits, items );

    SetMsgPanel( items );
    component->SetFlags( IS_NEW );

    if( m_autoplaceFields )
        component->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

    PrepareMoveItem( component );

    return component;
}


void SCH_EDIT_FRAME::OrientComponent( COMPONENT_ORIENTATION_T aOrientation )
{
    SCH_SCREEN*    screen = GetScreen();
    SCH_ITEM*      item = screen->GetCurItem();
    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    GetCanvas()->MoveCursorToCrossHair();

    if( item->GetFlags() == 0 )
        SetUndoItem( item );

    component->SetOrientation( aOrientation );

    m_canvas->CrossHairOn( );

    if( item->GetFlags() == 0 )
    {
        addCurrentItemToScreen();
        SchematicCleanUp( true );
    }

    TestDanglingEnds();

    RefreshItem( item );
    OnModify();
}


void SCH_EDIT_FRAME::OnSelectUnit( wxCommandEvent& aEvent )
{
    SCH_SCREEN*    screen = GetScreen();
    SCH_ITEM*      item = screen->GetCurItem();
    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    GetCanvas()->MoveCursorToCrossHair();

    int unit = aEvent.GetId() + 1 - ID_POPUP_SCH_SELECT_UNIT1;

    LIB_PART* part = GetLibPart( component->GetLibId() );

    if( !part )
        return;

    int unitCount = part->GetUnitCount();

    if( unitCount <= 1 || component->GetUnit() == unit )
        return;

    if( unit > unitCount )
        unit = unitCount;

    STATUS_FLAGS flags = component->GetFlags();

    if( !flags )    // No command in progress: save in undo list
        SaveCopyInUndoList( component, UR_CHANGED );

    /* Update the unit number. */
    component->SetUnitSelection( m_CurrentSheet, unit );
    component->SetUnit( unit );
    component->ClearFlags();
    component->SetFlags( flags );   // Restore m_Flag modified by SetUnit()

    if( m_autoplaceFields )
        component->AutoAutoplaceFields( GetScreen() );

    TestDanglingEnds();

    RefreshItem( component );
    OnModify();
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

        STATUS_FLAGS flags = aComponent->GetFlags();

        aComponent->SetConvert( aComponent->GetConvert() + 1 );

        // ensure m_Convert = 0, 1 or 2
        // 0 and 1 = shape 1 = not converted
        // 2 = shape 2 = first converted shape
        // > 2 is not used but could be used for more shapes
        // like multiple shapes for a programmable component
        // When m_Convert = val max, return to the first shape
        if( aComponent->GetConvert() > 2 )
            aComponent->SetConvert( 1 );

        // The alternate symbol may cause a change in the connection status so test the
        // connections so the connection indicators are drawn correctly.
        TestDanglingEnds();
        aComponent->ClearFlags();
        aComponent->SetFlags( flags );   // Restore m_Flag (modified by SetConvert())

        RefreshItem( aComponent );
        OnModify();
    }
}
