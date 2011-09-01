/*************************************************/
/*  Module to handle Get & Place Library Part    */
/*************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"
#include "kicad_device_context.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"
#include "viewlib_frame.h"
#include "eeschema_id.h"

#include "dialog_get_component.h"

#include <boost/foreach.hpp>


/**
 * Move a component.
 */
static void moveComponent( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase )
{
    SCH_SCREEN*    screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_COMPONENT* component = (SCH_COMPONENT*) screen->GetCurItem();

    if( aErase )
    {
        component->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    }

    component->Move( screen->GetCrossHairPosition() - component->m_Pos );
    component->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
}


/*
 * Abort a place component command in progress.
 */
static void abortMoveComponent( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) aPanel->GetParent();
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_COMPONENT* component = (SCH_COMPONENT*) screen->GetCurItem();

    if( component->IsNew() )
    {
        SAFE_DELETE( component );
    }
    else
    {
        component->SwapData( (SCH_COMPONENT*) frame->GetUndoItem() );
        component->ClearFlags();
    }

    aPanel->Refresh( true );
    screen->SetCurItem( NULL );
}


wxString SCH_EDIT_FRAME::SelectFromLibBrowser( void )
{
    wxSemaphore semaphore( 0, 1 );
    wxString cmpname;

    /* Close the current Lib browser, if open, and open a new one, in "modal" mode */
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Destroy();
        m_ViewlibFrame = NULL;
    }

    m_ViewlibFrame = new LIB_VIEW_FRAME( this, NULL, &semaphore );
    // Show the library viewer frame until it is closed
    while( semaphore.TryWait() == wxSEMA_BUSY ) // Wait for viewer closing event
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    cmpname = m_ViewlibFrame->GetSelectedComponent();
    m_ViewlibFrame->Destroy();

    return cmpname;
}



/*
 * load from a library and place a component
 *  if libname != "", search in lib "libname"
 *  else search in all loaded libs
 */
SCH_COMPONENT* SCH_EDIT_FRAME::Load_Component( wxDC*           DC,
                                               const wxString& libname,
                                               wxArrayString&  HistoryList,
                                               bool            UseLibBrowser )
{
    int             CmpCount  = 0;
    int             unit      = 1;
    int             convert   = 1;
    LIB_COMPONENT*  Entry     = NULL;
    SCH_COMPONENT*  Component = NULL;
    CMP_LIBRARY*    Library   = NULL;
    wxString        Name, keys, msg;
    bool            AllowWildSeach = true;
    static wxString lastCommponentName;

    m_itemToRepeat = NULL;
    DrawPanel->m_IgnoreMouseEvents = true;

    if( !libname.IsEmpty() )
    {
        Library = CMP_LIBRARY::FindLibrary( libname );

        if( Library != NULL )
            CmpCount = Library->GetCount();
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

    DIALOG_GET_COMPONENT dlg( this, GetComponentDialogPosition(), HistoryList,
                              msg, UseLibBrowser );
    dlg.SetComponentName( lastCommponentName );

    if ( dlg.ShowModal() == wxID_CANCEL )
    {
        DrawPanel->m_IgnoreMouseEvents = false;
        DrawPanel->MoveCursorToCrossHair();
        return NULL;
    }

    if( dlg.m_GetExtraFunction )
    {
        Name = SelectFromLibBrowser();
        unit = m_ViewlibFrame->GetUnit();
        convert = m_ViewlibFrame->GetConvert();
    }
    else
    {
        Name = dlg.GetComponentName();
    }

    if( Name.IsEmpty() )
    {
        DrawPanel->m_IgnoreMouseEvents = false;
        DrawPanel->MoveCursorToCrossHair();
        return NULL;
    }

#ifndef KICAD_KEEPCASE
    Name.MakeUpper();
#endif

    if( Name.GetChar( 0 ) == '=' )
    {
        AllowWildSeach = false;
        keys = Name.AfterFirst( '=' );
        Name = DataBaseGetName( this, keys, Name );

        if( Name.IsEmpty() )
        {
            DrawPanel->m_IgnoreMouseEvents = false;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( Name == wxT( "*" ) )
    {
        AllowWildSeach = false;

        if( GetNameOfPartToLoad( this, Library, Name ) == 0 )
        {
            DrawPanel->m_IgnoreMouseEvents = false;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( Name.Contains( wxT( "?" ) ) || Name.Contains( wxT( "*" ) ) )
    {
        AllowWildSeach = false;
        Name = DataBaseGetName( this, keys, Name );
        if( Name.IsEmpty() )
        {
            DrawPanel->m_IgnoreMouseEvents = false;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }

    Entry = CMP_LIBRARY::FindLibraryComponent( Name, libname );

    if( ( Entry == NULL ) && AllowWildSeach ) /* Search with wildcard */
    {
        AllowWildSeach = false;
        wxString wildname = wxChar( '*' ) + Name + wxChar( '*' );
        Name = wildname;
        Name = DataBaseGetName( this, keys, Name );

        if( !Name.IsEmpty() )
            Entry = CMP_LIBRARY::FindLibraryComponent( Name, libname );

        if( Entry == NULL )
        {
            DrawPanel->m_IgnoreMouseEvents = false;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }

    DrawPanel->m_IgnoreMouseEvents = false;
    DrawPanel->MoveCursorToCrossHair();

    if( Entry == NULL )
    {
        msg = _( "Failed to find part " ) + Name + _( " in library" );
        DisplayError( this, msg );
        return NULL;
    }

    lastCommponentName = Name;
    AddHistoryComponentName( HistoryList, Name );
    DrawPanel->SetMouseCapture( moveComponent, abortMoveComponent );

    Component = new SCH_COMPONENT( *Entry, GetSheet(), unit, convert,
                                   GetScreen()->GetCrossHairPosition(), true );
    // Set the m_ChipName value, from component name in lib, for aliases
    // Note if Entry is found, and if Name is an alias of a component,
    // alias exists because its root component was found
    Component->SetLibName( Name );

    // Set the component value that can differ from component name in lib, for aliases
    Component->GetField( VALUE )->m_Text = Name;
    Component->DisplayInfo( this );
    Component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );

    return Component;
}


/*
 * Routine to rotate and mirror a component.
 */
void SCH_EDIT_FRAME::OnChangeComponentOrientation( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    wxCHECK_RET( item != NULL && item->Type() == SCH_COMPONENT_T,
                 wxT( "Cannot change orientation of invalid schematic item." ) );

    SCH_COMPONENT* component = (SCH_COMPONENT*) item;

    int orientation;

    switch( aEvent.GetId() )
    {
    case ID_POPUP_SCH_MIRROR_X_CMP:
        orientation = CMP_MIRROR_X;
        break;

    case ID_POPUP_SCH_MIRROR_Y_CMP:
        orientation = CMP_MIRROR_Y;
        break;

    case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
        orientation = CMP_ROTATE_COUNTERCLOCKWISE;
        break;

    case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
        orientation = CMP_ROTATE_CLOCKWISE;
        break;

    case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
    default:
        orientation = CMP_NORMAL;
    }

    DrawPanel->MoveCursorToCrossHair();

    if( component->GetFlags() == 0 )
        SaveCopyInUndoList( item, UR_CHANGED );

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    // Erase the previous component in it's current orientation.

    DrawPanel->CrossHairOff( &dc );

    if( component->GetFlags() )
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawPanel->RefreshDrawingRect( component->GetBoundingBox() );

    component->SetOrientation( orientation );

    /* Redraw the component in the new position. */
    if( component->GetFlags() )
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    DrawPanel->CrossHairOn( &dc );
    GetScreen()->TestDanglingEnds( DrawPanel, &dc );
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

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    DrawPanel->MoveCursorToCrossHair();

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
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode );

    /* Update the unit number. */
    component->SetUnitSelection( GetSheet(), unit );
    component->SetUnit( unit );
    component->ClearFlags();
    component->SetFlags( flags );   // Restore m_Flag modified by SetUnit()

    /* Redraw the component in the new position. */
    if( flags )
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    screen->TestDanglingEnds( DrawPanel, &dc );
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

    int flags = DrawComponent->m_Flags;
    if( DrawComponent->m_Flags )
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

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
    if( DrawComponent->m_Flags & IS_MOVED )
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );
    OnModify( );
}


void SCH_EDIT_FRAME::StartMovePart( SCH_COMPONENT* aComponent, wxDC* aDC )
{
    wxCHECK_RET( (aComponent != NULL) && (aComponent->Type() == SCH_COMPONENT_T),
                 wxT( "Cannot move invalid component.  Bad Programmer!" ) );

    if( aComponent->GetFlags() == 0 )
        SetUndoItem( aComponent );

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aComponent->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    DrawPanel->SetMouseCapture( moveComponent, abortMoveComponent );
    GetScreen()->SetCurItem( aComponent );

#if 1

    // switch from normal mode to xor mode for the duration of the move, first
    // by erasing fully any "normal drawing mode" primitives with the
    // RefreshDrawingRect(), then by drawing the first time in xor mode so that
    // subsequent xor drawing modes will fully erase this first copy.

    aComponent->SetFlags( IS_MOVED ); // omit redrawing the component, erase only
    DrawPanel->RefreshDrawingRect( aComponent->GetBoundingBox() );

    aComponent->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );

#else

    aComponent->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    aComponent->SetFlags( IS_MOVED );

    moveComponent( DrawPanel, aDC, false );
#endif

    DrawPanel->m_AutoPAN_Request = true;

    DrawPanel->CrossHairOn( aDC );
}
