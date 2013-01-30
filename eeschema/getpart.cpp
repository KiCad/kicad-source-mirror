/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <appl_wxstruct.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxEeschemaStruct.h>
#include <kicad_device_context.h>
#include <msgpanel.h>

#include <general.h>
#include <protos.h>
#include <class_library.h>
#include <sch_component.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>

#include <dialog_get_component.h>

#include <boost/foreach.hpp>


wxString SCH_BASE_FRAME::SelectComponentFromLibBrowser( void )
{
    wxSemaphore semaphore( 0, 1 );
    wxString cmpname;

    // Close the current Lib browser, if open, and open a new one, in "modal" mode:
    LIB_VIEW_FRAME * viewlibFrame = LIB_VIEW_FRAME::GetActiveLibraryViewer();;
    if( viewlibFrame )
        viewlibFrame->Destroy();

    viewlibFrame = new LIB_VIEW_FRAME( this, NULL, &semaphore,
                        KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT );
    // Show the library viewer frame until it is closed
    // Wait for viewer closing event:
    while( semaphore.TryWait() == wxSEMA_BUSY )
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    cmpname = viewlibFrame->GetSelectedComponent();
    viewlibFrame->Destroy();

    return cmpname;
}

/*
 * Function SelectComponentFromLib
 * Calls the library viewer to select component to import into schematic.
 * if the library viewer is currently running, it is closed and reopened
 * in modal mode.
 * param aLibname = the lib name or an empty string.
 *     if aLibname is empty, the full list of libraries is used
 * param aList = list of previously loaded components
 * param aUseLibBrowser = bool to call the library viewer to select the component
 * param aUnit = a point to int to return the selected unit (if any)
 * param aConvert = a point to int to return the selected De Morgan shape (if any)
 *
 * return the component name
 */
wxString SCH_BASE_FRAME::SelectComponentFromLibrary( const wxString& aLibname,
                                                     wxArrayString&  aHistoryList,
                                                     bool            aUseLibBrowser,
                                                     int*            aUnit,
                                                     int*            aConvert )
{
    int             CmpCount  = 0;
    LIB_COMPONENT*  libEntry     = NULL;
    CMP_LIBRARY*    currLibrary   = NULL;
    wxString        cmpName, keys, msg;
    bool            allowWildSeach = true;

    if( !aLibname.IsEmpty() )
    {
        currLibrary = CMP_LIBRARY::FindLibrary( aLibname );

        if( currLibrary != NULL )
            CmpCount = currLibrary->GetCount();
    }
    else
    {
        BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
        {
            CmpCount += lib.GetCount();
        }
    }

    /* Ask for a component name or key words */
    msg.Printf( _( "component selection (%d items loaded):" ), CmpCount );

    DIALOG_GET_COMPONENT dlg( this, aHistoryList, msg, aUseLibBrowser );

    if( aHistoryList.GetCount() )
        dlg.SetComponentName( aHistoryList[0] );

    if ( dlg.ShowModal() == wxID_CANCEL )
        return wxEmptyString;

    if( dlg.m_GetExtraFunction )
    {
        cmpName = SelectComponentFromLibBrowser();
        if( aUnit )
            *aUnit = LIB_VIEW_FRAME::GetUnit();
        if( aConvert )
            *aConvert = LIB_VIEW_FRAME::GetConvert();
        if( !cmpName.IsEmpty() )
            AddHistoryComponentName( aHistoryList, cmpName );
        return cmpName;
    }
    else
        cmpName = dlg.GetComponentName();

    if( cmpName.IsEmpty() )
        return wxEmptyString;

    // Here, cmpName contains the component name,
    // or "*" if the Select All dialog button was pressed

#ifndef KICAD_KEEPCASE
    cmpName.MakeUpper();
#endif

    if( dlg.IsKeyword() )
    {
        allowWildSeach = false;
        keys = cmpName;
        cmpName = DataBaseGetName( this, keys, cmpName );

        if( cmpName.IsEmpty() )
            return wxEmptyString;
     }
    else if( cmpName == wxT( "*" ) )
    {
        allowWildSeach = false;

        if( GetNameOfPartToLoad( this, currLibrary, cmpName ) == 0 )
            return wxEmptyString;
    }
    else if( cmpName.Contains( wxT( "?" ) ) || cmpName.Contains( wxT( "*" ) ) )
    {
        allowWildSeach = false;
        cmpName = DataBaseGetName( this, keys, cmpName );

        if( cmpName.IsEmpty() )
            return wxEmptyString;
    }

    libEntry = CMP_LIBRARY::FindLibraryComponent( cmpName, aLibname );

    if( ( libEntry == NULL ) && allowWildSeach ) // Search with wildcard
    {
        allowWildSeach = false;
        wxString wildname = wxChar( '*' ) + cmpName + wxChar( '*' );
        cmpName = wildname;
        cmpName = DataBaseGetName( this, keys, cmpName );

        if( !cmpName.IsEmpty() )
            libEntry = CMP_LIBRARY::FindLibraryComponent( cmpName, aLibname );

        if( libEntry == NULL )
            return wxEmptyString;
    }

    if( libEntry == NULL )
    {
        msg.Printf( _( "Failed to find part <%s> in library" ), GetChars( cmpName ) );
        DisplayError( this, msg );
        return wxEmptyString;
    }

    AddHistoryComponentName( aHistoryList, cmpName );
    return cmpName;
}


/*
 * load from a library and place a component
 *  if libname != "", search in lib "libname"
 *  else search in all loaded libs
 */
SCH_COMPONENT* SCH_EDIT_FRAME::Load_Component( wxDC*           aDC,
                                               const wxString& aLibname,
                                               wxArrayString&  aHistoryList,
                                               bool            aUseLibBrowser )
{
    int unit    = 1;
    int convert = 1;
    m_itemToRepeat = NULL;
    m_canvas->SetIgnoreMouseEvents( true );

    wxString Name = SelectComponentFromLibrary( aLibname, aHistoryList, aUseLibBrowser,
                                                &unit, &convert );

    if( Name.IsEmpty() )
    {
        m_canvas->SetIgnoreMouseEvents( false );
        m_canvas->MoveCursorToCrossHair();
        return NULL;
    }

#ifndef KICAD_KEEPCASE
    Name.MakeUpper();
#endif

    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( Name, aLibname );

    m_canvas->SetIgnoreMouseEvents( false );
    m_canvas->MoveCursorToCrossHair();

    if( Entry == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to find part <%s> in library" ), GetChars( Name ) );
        wxMessageBox( msg );
        return NULL;
    }

    SCH_COMPONENT*  component;
    component = new SCH_COMPONENT( *Entry, m_CurrentSheet, unit, convert,
                                   GetScreen()->GetCrossHairPosition(), true );

    // Set the m_ChipName value, from component name in lib, for aliases
    // Note if Entry is found, and if Name is an alias of a component,
    // alias exists because its root component was found
    component->SetLibName( Name );

    // Set the component value that can differ from component name in lib, for aliases
    component->GetField( VALUE )->m_Text = Name;

    MSG_PANEL_ITEMS items;
    component->SetCurrentSheetPath( &GetCurrentSheet() );
    component->GetMsgPanelInfo( items );
    SetMsgPanel( items );
    component->Draw( m_canvas, aDC, wxPoint( 0, 0 ), g_XorMode );
    component->SetFlags( IS_NEW );
    MoveItem( (SCH_ITEM*) component, aDC );

    return component;
}


/*
 * Routine to rotate and mirror a component.
 */
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

    // Erase the previous component in it's current orientation.

    m_canvas->CrossHairOff( &dc );

    if( component->GetFlags() )
        component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );
    else
    {
        component->SetFlags( IS_MOVED );    // do not redraw the component
        m_canvas->RefreshDrawingRect( component->GetBoundingBox() );
        component->ClearFlags( IS_MOVED );
    }

    component->SetOrientation( aOrientation );

    /* Redraw the component in the new position. */
    if( component->GetFlags() )
        component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode );
    else
        component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    m_canvas->CrossHairOn( &dc );
    GetScreen()->TestDanglingEnds( m_canvas, &dc );
    OnModify();
}


/*
 * Handle select part in multi-part component.
 */
void SCH_EDIT_FRAME::OnSelectUnit( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    wxCHECK_RET( item != NULL && item->Type() == SCH_COMPONENT_T,
                 wxT( "Cannot select unit of invalid schematic item." ) );

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    m_canvas->MoveCursorToCrossHair();

    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    int unit = aEvent.GetId() + 1 - ID_POPUP_SCH_SELECT_UNIT1;

    LIB_COMPONENT* libEntry = CMP_LIBRARY::FindLibraryComponent( component->GetLibName() );

    if( libEntry == NULL )
        return;

    wxCHECK_RET( (unit >= 1) && (unit <= libEntry->GetPartCount()),
                 wxString::Format( wxT( "Cannot select unit %d from component "), unit ) +
                 libEntry->GetName() );

    int unitCount = libEntry->GetPartCount();

    if( (unitCount <= 1) || (component->GetUnit() == unit) )
        return;

    if( unit < 1 )
        unit = 1;

    if( unit > unitCount )
        unit = unitCount;

    int flags = component->GetFlags();

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

    /* Redraw the component in the new position. */
    if( flags )
        component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        component->Draw( m_canvas, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    screen->TestDanglingEnds( m_canvas, &dc );
    OnModify();
}


void SCH_EDIT_FRAME::ConvertPart( SCH_COMPONENT* DrawComponent, wxDC* DC )
{
    LIB_COMPONENT* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = CMP_LIBRARY::FindLibraryComponent( DrawComponent->GetLibName() );

    if( LibEntry == NULL )
        return;

    if( !LibEntry->HasConversion() )
    {
        DisplayError( this, wxT( "No convert found" ) );
        return;
    }

    int flags = DrawComponent->GetFlags();

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
    OnModify( );
}
