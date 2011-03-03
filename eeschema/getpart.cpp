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


static void ShowWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                             bool aErase );
static void ExitPlaceCmp( EDA_DRAW_PANEL* Panel, wxDC* DC );

static TRANSFORM OldTransform;
static wxPoint   OldPos;


wxString SCH_EDIT_FRAME::SelectFromLibBrowser( void )
{
    wxSemaphore semaphore( 0, 1 );

    /* Close the current Lib browser, if open, and open a new one, in "modal" mode */
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Destroy();
        m_ViewlibFrame = NULL;
    }

    m_ViewlibFrame = new LIB_VIEW_FRAME( this, NULL, &semaphore );
    m_ViewlibFrame->AdjustScrollBars( wxPoint( 0 , 0 ) );

    // Show the library viewer frame until it is closed
    while( semaphore.TryWait() == wxSEMA_BUSY ) // Wait for viewer closing event
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    return m_ViewlibFrame->GetSelectedComponent();
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
    bool            AllowWildSeach = TRUE;
    static wxString lastCommponentName;

    m_itemToRepeat = NULL;
    DrawPanel->m_IgnoreMouseEvents = TRUE;

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
        DrawPanel->m_IgnoreMouseEvents = FALSE;
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
        DrawPanel->m_IgnoreMouseEvents = FALSE;
        DrawPanel->MoveCursorToCrossHair();
        return NULL;
    }

#ifndef KICAD_KEEPCASE
    Name.MakeUpper();
#endif

    if( Name.GetChar( 0 ) == '=' )
    {
        AllowWildSeach = FALSE;
        keys = Name.AfterFirst( '=' );
        Name = DataBaseGetName( this, keys, Name );

        if( Name.IsEmpty() )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( Name == wxT( "*" ) )
    {
        AllowWildSeach = FALSE;

        if( GetNameOfPartToLoad( this, Library, Name ) == 0 )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( Name.Contains( wxT( "?" ) ) || Name.Contains( wxT( "*" ) ) )
    {
        AllowWildSeach = FALSE;
        Name = DataBaseGetName( this, keys, Name );
        if( Name.IsEmpty() )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }

    Entry = CMP_LIBRARY::FindLibraryComponent( Name, libname );

    if( ( Entry == NULL ) && AllowWildSeach ) /* Search with wildcard */
    {
        AllowWildSeach = FALSE;
        wxString wildname = wxChar( '*' ) + Name + wxChar( '*' );
        Name = wildname;
        Name = DataBaseGetName( this, keys, Name );

        if( !Name.IsEmpty() )
            Entry = CMP_LIBRARY::FindLibraryComponent( Name, libname );

        if( Entry == NULL )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }

    DrawPanel->m_IgnoreMouseEvents = FALSE;
    DrawPanel->MoveCursorToCrossHair();

    if( Entry == NULL )
    {
        msg = _( "Failed to find part " ) + Name + _( " in library" );
        DisplayError( this, msg );
        return NULL;
    }

    lastCommponentName = Name;
    AddHistoryComponentName( HistoryList, Name );
    DrawPanel->SetMouseCapture( ShowWhileMoving, ExitPlaceCmp );

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


/**
 * Move a component.
 */
static void ShowWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                             bool aErase )
{
    wxPoint        move_vector;
    SCH_SCREEN*    screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_COMPONENT* Component = (SCH_COMPONENT*) screen->GetCurItem();

    if( aErase )
    {
        Component->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    }

    move_vector = screen->GetCrossHairPosition() - Component->m_Pos;
    Component->Move( move_vector );
    Component->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
}


/*
 * Routine to rotate and mirror a component.
 *
 ** If DC == NULL: no repaint
 */
void SCH_EDIT_FRAME::OnChangeComponentOrientation( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    // Ensure the struct is a component (could be a struct of a
    // component, like Field, text..)
    if( screen->GetCurItem() == NULL || screen->GetCurItem()->Type() != SCH_COMPONENT_T )
    {
        screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL || screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            return;
    }

    SCH_COMPONENT* component = (SCH_COMPONENT*) screen->GetCurItem();

    int orientation;

    switch( aEvent.GetId() )
    {
    case ID_POPUP_SCH_MIROR_X_CMP:
        orientation = CMP_MIRROR_X;
        break;

    case ID_POPUP_SCH_MIROR_Y_CMP:
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

    if( screen->GetCurItem()->GetFlags() == 0 )
        SaveCopyInUndoList( screen->GetCurItem(), UR_CHANGED );

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
 * Abort a place component command in progress.
 */
static void ExitPlaceCmp( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    SCH_SCREEN*    screen = (SCH_SCREEN*) Panel->GetScreen();

    SCH_COMPONENT* Component = (SCH_COMPONENT*) screen->GetCurItem();

    if( Component->m_Flags & IS_NEW )
    {
        Component->m_Flags = 0;
        SAFE_DELETE( Component );
    }
    else if( Component )
    {
        wxPoint move_vector = OldPos - Component->m_Pos;
        Component->Move( move_vector );
        Component->SetTransform( OldTransform );
        Component->m_Flags = 0;
    }

    Panel->Refresh( true );
    screen->SetCurItem( NULL );
}


/*
 * Handle select part in multi-part component.
 */
void SCH_EDIT_FRAME::OnSelectUnit( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    if( screen->GetCurItem() == NULL )
        return;

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    // Verify the selected item is a component, it may be part of a component such as a field
    // or text item.
    if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
    {
        screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            return;
    }

    DrawPanel->MoveCursorToCrossHair();

    SCH_COMPONENT* component = (SCH_COMPONENT*) screen->GetCurItem();

    wxCHECK_RET( (component != NULL) && (component->Type() == SCH_COMPONENT_T),
                 wxT( "Cannot select unit for invalid component." ) );

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
    component->SetFlags( flags );  // Restore m_Flag modified by SetUnit();

    /* Redraw the component in the new position. */
    if( flags )
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        component->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    GetScreen()->TestDanglingEnds( DrawPanel, &dc );
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

    /* Redraw the component in the new position. */
    if( DrawComponent->m_Flags & IS_MOVED )
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );
    OnModify( );
}


void SCH_EDIT_FRAME::StartMovePart( SCH_COMPONENT* Component, wxDC* DC )
{
    if( Component == NULL )
        return;

    if( Component->Type() != SCH_COMPONENT_T )
        return;

    if( Component->m_Flags == 0 )
    {
        if( g_ItemToUndoCopy )
        {
            SAFE_DELETE( g_ItemToUndoCopy );
        }

        g_ItemToUndoCopy = Component->Clone();
    }

    DrawPanel->CrossHairOff( DC );
    GetScreen()->SetCrossHairPosition( Component->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    DrawPanel->SetMouseCapture( ShowWhileMoving, ExitPlaceCmp );
    GetScreen()->SetCurItem( Component );
    OldPos = Component->m_Pos;
    OldTransform = Component->GetTransform();

#if 1

    // switch from normal mode to xor mode for the duration of the move, first
    // by erasing fully any "normal drawing mode" primitives with the
    // RefreshDrawingRect(), then by drawing the first time in xor mode so that
    // subsequent xor drawing modes will fully erase this first copy.

    Component->m_Flags |= IS_MOVED; // omit redrawing the component, erase only
    DrawPanel->RefreshDrawingRect( Component->GetBoundingBox() );

    Component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );

#else

    Component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    Component->m_Flags |= IS_MOVED;

    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, FALSE );
#endif

    DrawPanel->m_AutoPAN_Request = TRUE;

    DrawPanel->CrossHairOn( DC );
}
