/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <schframe.h>
#include <kicad_device_context.h>
#include <msgpanel.h>

#include <general.h>
#include <class_library.h>
#include <sch_component.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>

#include <dialog_choose_component.h>
#include <cmp_tree_model_adapter.h>
#include <dialog_get_component.h>


SCH_BASE_FRAME::COMPONENT_SELECTION SCH_BASE_FRAME::SelectComponentFromLibBrowser(
        const SCHLIB_FILTER* aFilter,
        LIB_ALIAS* aPreselectedAlias,
        int aUnit, int aConvert )
{
    // Close any open non-modal Lib browser, and open a new one, in "modal" mode:
    LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

    if( viewlibFrame )
        viewlibFrame->Destroy();

    viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, this );

    if( aFilter )
        viewlibFrame->SetFilter( aFilter );

    if( aPreselectedAlias )
    {
        viewlibFrame->SetSelectedLibrary( aPreselectedAlias->GetLibraryName() );
        viewlibFrame->SetSelectedComponent( aPreselectedAlias->GetName() );
    }

    if( aUnit > 0 )
        viewlibFrame->SetUnit( aUnit );

    if( aConvert > 0 )
        viewlibFrame->SetConvert( aConvert );

    viewlibFrame->Refresh();

    COMPONENT_SELECTION sel;

    if( viewlibFrame->ShowModal( &sel.Name, this ) )
    {
        sel.Unit = viewlibFrame->GetUnit();
        sel.Convert = viewlibFrame->GetConvert();
    }

    viewlibFrame->Destroy();

    return sel;
}


SCH_BASE_FRAME::COMPONENT_SELECTION SCH_BASE_FRAME::SelectComponentFromLibrary(
        const SCHLIB_FILTER*                aFilter,
        std::vector<COMPONENT_SELECTION>&   aHistoryList,
        bool                                aUseLibBrowser,
        int                                 aUnit,
        int                                 aConvert,
        const wxString&                     aHighlight,
        bool                                aAllowFields )
{
    wxString        dialogTitle;
    PART_LIBS*      libs = Prj().SchLibs();

    auto adapter( CMP_TREE_MODEL_ADAPTER::Create( libs ) );
    bool loaded = false;

    if( aFilter )
    {
        const wxArrayString& liblist = aFilter->GetAllowedLibList();

        for( unsigned ii = 0; ii < liblist.GetCount(); ii++ )
        {
            PART_LIB* currLibrary = libs->FindLibrary( liblist[ii] );

            if( currLibrary )
            {
                loaded = true;
                adapter->AddLibrary( *currLibrary );
            }
        }

        if( aFilter->GetFilterPowerParts() )
            adapter->SetFilter( CMP_TREE_MODEL_ADAPTER::CMP_FILTER_POWER );

    }

    if( !loaded )
    {
        for( PART_LIB& lib : *libs )
        {
            adapter->AddLibrary( lib );
        }
    }


    if( !aHistoryList.empty() )
    {
        wxArrayString history_list;

        for( auto const& i : aHistoryList )
            history_list.push_back( i.Name );

        adapter->AddAliasList( "-- " + _( "History" ) + " --", history_list, NULL );
        adapter->SetPreselectNode( aHistoryList[0].Name, aHistoryList[0].Unit );
    }

    if( !aHighlight.IsEmpty() )
        adapter->SetPreselectNode( aHighlight, /* aUnit */ 0 );

    dialogTitle.Printf( _( "Choose Component (%d items loaded)" ),
                        adapter->GetComponentsCount() );
    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, adapter, aConvert, aAllowFields );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return COMPONENT_SELECTION();

    COMPONENT_SELECTION sel;
    LIB_ALIAS* const alias = dlg.GetSelectedAlias( &sel.Unit );

    if( alias == nullptr )      // Dialog closed by OK button, but no symbol selected
        return COMPONENT_SELECTION();

    if( sel.Unit == 0 )
        sel.Unit = 1;

    sel.Fields = dlg.GetFields();

    if ( alias )
        sel.Name = alias->GetName();

    if( dlg.IsExternalBrowserSelected() )   // User requested component browser.
        sel = SelectComponentFromLibBrowser( aFilter, alias, sel.Unit, sel.Convert );

    if( !sel.Name.empty() )
    {
        aHistoryList.erase(
            std::remove_if(
                aHistoryList.begin(),
                aHistoryList.end(),
                [ &sel ]( COMPONENT_SELECTION const& i ) { return i.Name == sel.Name; } ),
            aHistoryList.end() );

        aHistoryList.insert( aHistoryList.begin(), sel );
    }

    return sel;
}


SCH_COMPONENT* SCH_EDIT_FRAME::Load_Component(
                                wxDC*                            aDC,
                                const SCHLIB_FILTER*             aFilter,
                                SCH_BASE_FRAME::HISTORY_LIST&    aHistoryList,
                                bool                             aUseLibBrowser )
{
    SetRepeatItem( NULL );
    m_canvas->SetIgnoreMouseEvents( true );

    auto sel = SelectComponentFromLibrary( aFilter, aHistoryList,
                                                aUseLibBrowser, 1, 1 );

    if( sel.Name.IsEmpty() )
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

    LIB_PART* part = Prj().SchLibs()->FindLibPart( LIB_ID( wxEmptyString, sel.Name ), libsource );

    if( !part )
    {
        wxString msg = wxString::Format( _(
            "Failed to find part '%s' in library" ),
            GetChars( sel.Name )
            );
        wxMessageBox( msg );
        return NULL;
    }

    SCH_COMPONENT* component = new SCH_COMPONENT( *part, m_CurrentSheet, sel.Unit, sel.Convert,
                                                  GetCrossHairPosition(), true );

    // Set the m_ChipName value, from component name in lib, for aliases
    // Note if part is found, and if name is an alias of a component,
    // alias exists because its root component was found
    LIB_ID libId;

    libId.SetLibItemName( TO_UTF8( sel.Name ), false );
    component->SetLibId( libId );

    // Be sure the link to the corresponding LIB_PART is OK:
    component->Resolve( Prj().SchLibs() );

    // Set any fields that have been modified
    for( auto const& i : sel.Fields )
    {
        auto field = component->GetField( i.first );

        if( field )
            field->SetText( i.second );
    }

    // Set the component value that can differ from component name in lib, for aliases
    component->GetField( VALUE )->SetText( sel.Name );

    MSG_PANEL_ITEMS items;

    component->SetCurrentSheetPath( &GetCurrentSheet() );
    component->GetMsgPanelInfo( items );

    SetMsgPanel( items );
    component->Draw( m_canvas, aDC, wxPoint( 0, 0 ), g_XorMode );
    component->SetFlags( IS_NEW );

    if( m_autoplaceFields )
        component->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

    PrepareMoveItem( (SCH_ITEM*) component, aDC );

    return component;
}


void SCH_EDIT_FRAME::OrientComponent( COMPONENT_ORIENTATION_T aOrientation )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    wxCHECK_RET( item != NULL && item->Type() == SCH_COMPONENT_T,
                 wxT( "Cannot change orientation of invalid schematic item." ) );

    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    m_canvas->MoveCursorToCrossHair();

    if( component->GetFlags() == 0 )
    {
        SaveCopyInUndoList( item, UR_CHANGED );
        GetScreen()->SetCurItem( NULL );
    }

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    component->SetOrientation( aOrientation );

    m_canvas->CrossHairOn( &dc );

    if( GetScreen()->TestDanglingEnds() )
        m_canvas->Refresh();

    OnModify();
}


/*
 * Handle select part in multi-unit part.
 */
void SCH_EDIT_FRAME::OnSelectUnit( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    wxCHECK_RET( item != NULL && item->Type() == SCH_COMPONENT_T,
                 wxT( "Cannot select unit of invalid schematic item." ) );

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    m_canvas->MoveCursorToCrossHair();

    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    int unit = aEvent.GetId() + 1 - ID_POPUP_SCH_SELECT_UNIT1;

    if( LIB_PART* part = Prj().SchLibs()->FindLibPart( component->GetLibId() ) )
    {
        int unitCount = part->GetUnitCount();

        wxCHECK_RET( (unit >= 1) && (unit <= unitCount),
                     wxString::Format( wxT( "Cannot select unit %d from component " ), unit ) +
                     part->GetName() );

        if( unitCount <= 1 || component->GetUnit() == unit )
            return;

        if( unit > unitCount )
            unit = unitCount;

        STATUS_FLAGS flags = component->GetFlags();

        if( !flags )    // No command in progress: save in undo list
            SaveCopyInUndoList( component, UR_CHANGED );

        if( flags )
            component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
            component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );

        /* Update the unit number. */
        component->SetUnitSelection( m_CurrentSheet, unit );
        component->SetUnit( unit );
        component->ClearFlags();
        component->SetFlags( flags );   // Restore m_Flag modified by SetUnit()

        if( m_autoplaceFields )
            component->AutoAutoplaceFields( GetScreen() );

        if( screen->TestDanglingEnds() )
            m_canvas->Refresh();

        OnModify();
    }
}


void SCH_EDIT_FRAME::ConvertPart( SCH_COMPONENT* DrawComponent, wxDC* DC )
{
    if( !DrawComponent )
        return;

    if( LIB_PART* part = Prj().SchLibs()->FindLibPart( DrawComponent->GetLibId() ) )
    {
        if( !part->HasConversion() )
        {
            DisplayError( this, wxT( "No convert found" ) );
            return;
        }

        STATUS_FLAGS flags = DrawComponent->GetFlags();

        if( DrawComponent->GetFlags() )
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );

        DrawComponent->SetConvert( DrawComponent->GetConvert() + 1 );

        // ensure m_Convert = 0, 1 or 2
        // 0 and 1 = shape 1 = not converted
        // 2 = shape 2 = first converted shape
        // > 2 is not used but could be used for more shapes
        // like multiple shapes for a programmable component
        // When m_Convert = val max, return to the first shape
        if( DrawComponent->GetConvert() > 2 )
            DrawComponent->SetConvert( 1 );

        // The alternate symbol may cause a change in the connection status so test the
        // connections so the connection indicators are drawn correctly.
        GetScreen()->TestDanglingEnds();
        DrawComponent->ClearFlags();
        DrawComponent->SetFlags( flags );   // Restore m_Flag (modified by SetConvert())

        /* Redraw the component in the new position. */
        if( DrawComponent->IsMoving() )
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

        OnModify();
    }
}
