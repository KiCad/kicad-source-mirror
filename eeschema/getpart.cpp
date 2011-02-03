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

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"
#include "viewlib_frame.h"

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

    /* Close the current Lib browser, if open, and open a new one, in
     * "modal" mode */
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Destroy();
        m_ViewlibFrame = NULL;
    }

    m_ViewlibFrame = new LIB_VIEW_FRAME( this, NULL, &semaphore );
    m_ViewlibFrame->AdjustScrollBars();

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
        DrawPanel->MouseToCursorSchema();
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
        DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
            return NULL;
        }
    }
    else if( Name == wxT( "*" ) )
    {
        AllowWildSeach = FALSE;

        if( GetNameOfPartToLoad( this, Library, Name ) == 0 )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
            return NULL;
        }
    }

    DrawPanel->m_IgnoreMouseEvents = FALSE;
    DrawPanel->MouseToCursorSchema();

    if( Entry == NULL )
    {
        msg = _( "Failed to find part " ) + Name + _( " in library" );
        DisplayError( this, msg );
        return NULL;
    }

    lastCommponentName = Name;
    AddHistoryComponentName( HistoryList, Name );

    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitPlaceCmp;

    Component = new SCH_COMPONENT( *Entry, GetSheet(), unit, convert,
                                   GetScreen()->m_Curseur, true );
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

    move_vector = screen->m_Curseur - Component->m_Pos;
    Component->Move( move_vector );
    Component->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
}


/*
 * Routine to rotate and mirror a component.
 *
 ** If DC == NULL: no repaint
 */
void SCH_EDIT_FRAME::CmpRotationMiroir( SCH_COMPONENT* DrawComponent, wxDC* DC, int type_rotate )
{
    if( DrawComponent == NULL )
        return;

    /* Deletes the previous component. */
    if( DC )
    {
        DrawPanel->CursorOff( DC );

        if( DrawComponent->m_Flags )
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
        {
            DrawPanel->RefreshDrawingRect( DrawComponent->GetBoundingBox() );
        }
    }

    DrawComponent->SetOrientation( type_rotate );

    /* Redraw the component in the new position. */
    if( DC )
    {
        if( DrawComponent->m_Flags )
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
        else
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        DrawPanel->CursorOn( DC );
    }

    GetScreen()->TestDanglingEnds( DrawPanel, DC );
    OnModify( );
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

    Panel->Refresh( TRUE );
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    screen->SetCurItem( NULL );
}


/*
 * Handle select part in multi-part component.
 */
void SCH_EDIT_FRAME::SelPartUnit( SCH_COMPONENT* DrawComponent, int unit, wxDC* DC )
{
    int m_UnitCount;
    LIB_COMPONENT* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = CMP_LIBRARY::FindLibraryComponent( DrawComponent->GetLibName() );

    if( LibEntry == NULL )
        return;

    m_UnitCount = LibEntry->GetPartCount();

    if( m_UnitCount <= 1 )
        return;

    if( DrawComponent->GetUnit() == unit )
        return;

    if( unit < 1 )
        unit = 1;

    if( unit > m_UnitCount )
        unit = m_UnitCount;

    int curr_flg = DrawComponent->m_Flags;
    if( ! curr_flg )    // No command in progress: save in undo list
        SaveCopyInUndoList( DrawComponent, UR_CHANGED );

    if( curr_flg )
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    /* Update the unit number. */
    DrawComponent->SetUnitSelection( GetSheet(), unit );
    DrawComponent->SetUnit( unit );
    DrawComponent->m_Flags = curr_flg;  // Restore m_Flag modified by SetUnit();

    /* Redraw the component in the new position. */
    if( DrawComponent->m_Flags )
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode, g_GhostColor );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );
    OnModify( );
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

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = Component->m_Pos;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitPlaceCmp;
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

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
#endif

    DrawPanel->m_AutoPAN_Request = TRUE;

    DrawPanel->CursorOn( DC );
}
