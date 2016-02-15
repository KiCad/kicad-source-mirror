/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_sheet_path.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>

#include <dialog_choose_component.h>
#include <component_tree_search_container.h>
#include <dialog_get_component.h>

#include <boost/foreach.hpp>


wxString SCH_BASE_FRAME::SelectComponentFromLibBrowser( const SCHLIB_FILTER* aFilter,
                                                        LIB_ALIAS* aPreselectedAlias,
                                                        int* aUnit, int* aConvert )
{
    // Close any open non-modal Lib browser, and open a new one, in "modal" mode:
    LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

    if( viewlibFrame )
        viewlibFrame->Destroy();

    viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, true );

    if( aFilter )
        viewlibFrame->SetFilter( aFilter );

    if( aPreselectedAlias )
    {
        viewlibFrame->SetSelectedLibrary( aPreselectedAlias->GetLibraryName() );
        viewlibFrame->SetSelectedComponent( aPreselectedAlias->GetName() );
    }

    if( aUnit && *aUnit > 0 )
        viewlibFrame->SetUnit( *aUnit );

    if( aConvert && *aConvert > 0 )
        viewlibFrame->SetConvert( *aConvert );

    viewlibFrame->Refresh();

    wxString cmpname;

    if( viewlibFrame->ShowModal( &cmpname, this ) )
    {
        if( aUnit )
            *aUnit = viewlibFrame->GetUnit();

        if( aConvert )
            *aConvert = viewlibFrame->GetConvert();
    }

    viewlibFrame->Destroy();

    return cmpname;
}


wxString SCH_BASE_FRAME::SelectComponentFromLibrary( const SCHLIB_FILTER* aFilter,
                                                     wxArrayString&  aHistoryList,
                                                     int&            aHistoryLastUnit,
                                                     bool            aUseLibBrowser,
                                                     int*            aUnit,
                                                     int*            aConvert )
{
    wxString        dialogTitle;
    PART_LIBS*      libs = Prj().SchLibs();

    COMPONENT_TREE_SEARCH_CONTAINER search_container( libs );   // Container doing search-as-you-type
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
                search_container.AddLibrary( *currLibrary );
            }
        }

        if( aFilter->GetFilterPowerParts() )
            search_container.SetFilter( COMPONENT_TREE_SEARCH_CONTAINER::CMP_FILTER_POWER );

    }

    if( !loaded )
    {
        BOOST_FOREACH( PART_LIB& lib, *libs )
        {
            search_container.AddLibrary( lib );
        }
    }


    if( !aHistoryList.empty() )
    {
        // This is good for a transition for experienced users: giving them a History. Ideally,
        // we actually make this part even faster to access with a popup on ALT-a or something.
        // the history is under a node named  "-- History --"
        // However, because it is translatable, and we need to have a node name starting by "-- "
        // because we (later) sort all node names alphabetically and this node should be the first,
        // we build it with only with "History" string translatable
        wxString nodename;
        nodename  << wxT("-- ") << _("History") << wxT(" --");
        search_container.AddAliasList( nodename, aHistoryList, NULL );
        search_container.SetPreselectNode( aHistoryList[0], aHistoryLastUnit );
    }

    const int deMorgan = aConvert ? *aConvert : 1;
    dialogTitle.Printf( _( "Choose Component (%d items loaded)" ), search_container.GetComponentsCount() );
    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, &search_container, deMorgan );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxEmptyString;

    wxString cmpName;
    LIB_ALIAS* const alias = dlg.GetSelectedAlias( aUnit );
    if ( alias )
        cmpName = alias->GetName();

    if( dlg.IsExternalBrowserSelected() )   // User requested component browser.
        cmpName = SelectComponentFromLibBrowser( aFilter, alias, aUnit, aConvert);

    if( !cmpName.empty() )
    {
        AddHistoryComponentName( aHistoryList, cmpName );
        if ( aUnit ) aHistoryLastUnit = *aUnit;
    }

    return cmpName;
}


SCH_COMPONENT* SCH_EDIT_FRAME::Load_Component( wxDC*           aDC,
                                               const SCHLIB_FILTER* aFilter,
                                               wxArrayString&  aHistoryList,
                                               int&            aHistoryLastUnit,
                                               bool            aUseLibBrowser )
{
    int unit    = 1;
    int convert = 1;
    SetRepeatItem( NULL );
    m_canvas->SetIgnoreMouseEvents( true );

    wxString name = SelectComponentFromLibrary( aFilter, aHistoryList, aHistoryLastUnit,
                                                aUseLibBrowser, &unit, &convert );

    if( name.IsEmpty() )
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

    LIB_PART* part = Prj().SchLibs()->FindLibPart( name, libsource );

    if( !part )
    {
        wxString msg = wxString::Format( _(
            "Failed to find part '%s' in library" ),
            GetChars( name )
            );
        wxMessageBox( msg );
        return NULL;
    }

    SCH_COMPONENT*  component = new SCH_COMPONENT( *part, m_CurrentSheet, unit, convert,
            GetCrossHairPosition(), true );

    // Set the m_ChipName value, from component name in lib, for aliases
    // Note if part is found, and if name is an alias of a component,
    // alias exists because its root component was found
    component->SetPartName( name );

    // Be sure the link to the corresponding LIB_PART is OK:
    component->Resolve( Prj().SchLibs() );

    // Set the component value that can differ from component name in lib, for aliases
    component->GetField( VALUE )->SetText( name );

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
    GetScreen()->TestDanglingEnds( m_canvas, &dc );
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

    if( LIB_PART* part = Prj().SchLibs()->FindLibPart( component->GetPartName() ) )
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
        component->SetUnitSelection( m_CurrentSheet->Last(), unit );
        component->SetUnit( unit );
        component->ClearFlags();
        component->SetFlags( flags );   // Restore m_Flag modified by SetUnit()

        if( m_autoplaceFields )
            component->AutoAutoplaceFields( GetScreen() );

        screen->TestDanglingEnds( m_canvas, &dc );
        m_canvas->Refresh();
        OnModify();
    }
}


void SCH_EDIT_FRAME::ConvertPart( SCH_COMPONENT* DrawComponent, wxDC* DC )
{
    if( !DrawComponent )
        return;

    if( LIB_PART* part = Prj().SchLibs()->FindLibPart( DrawComponent->GetPartName() ) )
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

        DrawComponent->ClearFlags();
        DrawComponent->SetFlags( flags );   // Restore m_Flag (modified by SetConvert())

        /* Redraw the component in the new position. */
        if( DrawComponent->IsMoving() )
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
            DrawComponent->Draw( m_canvas, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

        GetScreen()->TestDanglingEnds( m_canvas, DC );
        OnModify();
    }
}
