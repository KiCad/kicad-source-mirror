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
#include <class_library.h>
#include <confirm.h>
#include <eeschema_id.h>
#include <general.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <pgm_base.h>
#include <sch_component.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

#include <dialog_choose_symbol.h>
#include <symbol_tree_model_adapter.h>

PICKED_SYMBOL SCH_BASE_FRAME::PickSymbolFromLibBrowser( wxTopLevelWindow* aParent,
                                                        const SCHLIB_FILTER* aFilter,
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
        viewer->SetSelectedComponent( aPreselectedLibId.GetLibItemName() );
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


PICKED_SYMBOL SCH_BASE_FRAME::PickSymbolFromLibTree( const SCHLIB_FILTER* aFilter,
                                                     std::vector<PICKED_SYMBOL>& aHistoryList,
                                                     bool aUseLibBrowser, int aUnit, int aConvert,
                                                     bool aShowFootprints, const LIB_ID* aHighlight,
                                                     bool aAllowFields )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_SYMBOL::g_Mutex, std::defer_lock );
    SYMBOL_LIB_TABLE*            libs = Prj().SchSymbolLibTable();

    // One CHOOSE_COMPONENT dialog at a time.  User probaby can't handle more anyway.
    if( !dialogLock.try_lock() )
        return PICKED_SYMBOL();

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> adapter = SYMBOL_TREE_MODEL_ADAPTER::Create( this, libs );
    bool loaded = false;

    if( aFilter )
    {
        const wxArrayString& liblist = aFilter->GetAllowedLibList();

        for( unsigned ii = 0; ii < liblist.GetCount(); ii++ )
        {
            if( libs->HasLibrary( liblist[ii], true ) )
            {
                loaded = true;
                static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( adapter.get() )->AddLibrary( liblist[ii] );
            }
        }

        adapter->AssignIntrinsicRanks();

        if( aFilter->GetFilterPowerParts() )
            adapter->SetFilter( SYMBOL_TREE_MODEL_ADAPTER::CMP_FILTER_POWER );
    }

    std::vector< LIB_TREE_ITEM* > history_list;

    for( const PICKED_SYMBOL& i : aHistoryList )
    {
        LIB_PART* symbol = GetLibPart( i.LibId );

        // This can be null, for example when a symbol has been deleted from a library
        if( symbol )
            history_list.push_back( symbol );
    }

    adapter->DoAddLibrary( "-- " + _( "Recently Used" ) + " --", wxEmptyString, history_list, true );

    if( !aHistoryList.empty() )
        adapter->SetPreselectNode( aHistoryList[0].LibId, aHistoryList[0].Unit );

    const std::vector< wxString > libNicknames = libs->GetLogicalLibs();

    if( !loaded )
         static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( adapter.get() )->AddLibraries( libNicknames,
                                                  this );

    if( aHighlight && aHighlight->IsValid() )
        adapter->SetPreselectNode( *aHighlight, /* aUnit */ 0 );

    wxString dialogTitle;

    if( adapter->GetFilter() == SYMBOL_TREE_MODEL_ADAPTER::CMP_FILTER_POWER )
        dialogTitle.Printf( _( "Choose Power Symbol (%d items loaded)" ), adapter->GetItemCount() );
    else
        dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), adapter->GetItemCount() );

    DIALOG_CHOOSE_SYMBOL dlg( this, dialogTitle, adapter, aConvert, aAllowFields, aShowFootprints,
                              aUseLibBrowser );

    if( dlg.ShowModal() == wxID_CANCEL )
        return PICKED_SYMBOL();

    PICKED_SYMBOL sel;
    LIB_ID id = dlg.GetSelectedLibId( &sel.Unit );

    if( dlg.IsExternalBrowserSelected() )   // User requested component browser.
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
        aHistoryList.erase( std::remove_if( aHistoryList.begin(), aHistoryList.end(),
                                            [ &sel ]( PICKED_SYMBOL const& i )
                                            {
                                                return i.LibId == sel.LibId;
                                            } ),
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
        SaveCopyInUndoList( GetScreen(), aComponent, UNDO_REDO::CHANGED, false );

    /* Update the unit number. */
    aComponent->SetUnitSelection( &GetCurrentSheet(), aUnit );
    aComponent->SetUnit( aUnit );
    aComponent->ClearFlags();
    aComponent->SetFlags( savedFlags ); // Restore m_Flag modified by SetUnit()

    if( !aComponent->GetEditFlags() )   // No command in progress: update schematic
    {
        if( eeconfig()->m_AutoplaceFields.enable )
            aComponent->AutoAutoplaceFields( GetScreen() );

        TestDanglingEnds();

        UpdateItem( aComponent );
        OnModify();
    }
}


void SCH_EDIT_FRAME::ConvertPart( SCH_COMPONENT* aComponent )
{
    if( !aComponent || !aComponent->GetPartRef() )
        return;

    wxString msg;

    if( !aComponent->GetPartRef()->HasConversion() )
    {
        LIB_ID id = aComponent->GetPartRef()->GetLibId();

        msg.Printf( _( "No alternate body style found for symbol \"%s\" in library \"%s\"." ),
                    id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );
        DisplayError( this,  msg );
        return;
    }

    STATUS_FLAGS savedFlags = aComponent->GetFlags();

    aComponent->SetConvert( aComponent->GetConvert() + 1 );

    // ensure m_convert = 1 or 2
    // 1 = shape 1 = not converted
    // 2 = shape 2 = first converted shape
    // > 2 is not used but could be used for more shapes
    // like multiple shapes for a programmable component
    // When m_convert = val max, return to the first shape
    if( aComponent->GetConvert() > LIB_ITEM::LIB_CONVERT::DEMORGAN )
        aComponent->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

    TestDanglingEnds();
    aComponent->ClearFlags();
    aComponent->SetFlags( savedFlags );   // Restore m_flags (modified by SetConvert())

    // If selected make sure all the now-included pins are selected
    if( aComponent->IsSelected() )
        m_toolManager->RunAction( EE_ACTIONS::addItemToSel, true, aComponent );

    UpdateItem( aComponent );
    OnModify();
}
