/*************************************************/
/*	Module to handle Get & Place Library Part	 */
/*************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "get_component_dialog.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


/* Routines Locales */
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ExitPlaceCmp( WinEDA_DrawPanel* Panel, wxDC* DC );

/* Variables locales */
static int     OldTransMat[2][2];
static wxPoint OldPos;


/*******************************************************/
wxString SelectFromLibBrowser( WinEDA_DrawFrame* parent )
/*******************************************************/
{
    wxString               name;
    WinEDA_ViewlibFrame*   Viewer;
    wxSemaphore            semaphore( 0, 1 );
    WinEDA_SchematicFrame* frame;

    frame = (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

    Viewer = frame->m_ViewlibFrame;
    /* Close the current Lib browser, if open, and open a new one, in "modal" mode */
    if( Viewer )
        Viewer->Destroy();

    Viewer = frame->m_ViewlibFrame =
                 new WinEDA_ViewlibFrame( frame, NULL, &semaphore );
    Viewer->AdjustScrollBars();

    // Show the library viewer frame until it is closed
    while( semaphore.TryWait() == wxSEMA_BUSY )    // Wait for viewer closing event
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    name = g_CurrentViewComponentName;
    return name;
}


/**************************************************************************/
SCH_COMPONENT* WinEDA_SchematicFrame::Load_Component( wxDC*           DC,
                                                      const wxString& libname,
                                                      wxArrayString&  HistoryList,
                                                      bool            UseLibBrowser )
/**************************************************************************/

/* load from a library and place a component
 *  if libname != "", search in lib "libname"
 *  else search in all loaded libs
 */
{
    int                     ii, CmpCount = 0;
    EDA_LibComponentStruct* Entry     = NULL;
    SCH_COMPONENT*          Component = NULL;
    LibraryStruct*          Library   = NULL;
    wxString                Name, keys, msg;
    bool                    AllowWildSeach = TRUE;

    g_ItemToRepeat = NULL;
    DrawPanel->m_IgnoreMouseEvents = TRUE;

    if( !libname.IsEmpty() )
    {
        Library = g_LibraryList;
        while( Library )
        {
            if( Library->m_Name == libname )
            {
                CmpCount = Library->m_NumOfParts;
                break;
            }
            Library = Library->m_Pnext;
        }
    }
    else
    {
        LibraryStruct* lib = g_LibraryList;
        while( lib )
        {
            CmpCount += lib->m_NumOfParts;
            lib = lib->m_Pnext;
        }
    }

    /* Ask for a component name or key words */
    msg.Printf( _( "component selection (%d items loaded):" ), CmpCount );

    Name = GetComponentName( this, HistoryList, msg,
                             UseLibBrowser ? SelectFromLibBrowser : NULL );
    Name.MakeUpper();
    if( Name.IsEmpty() )
    {
        DrawPanel->m_IgnoreMouseEvents = FALSE;
        DrawPanel->MouseToCursorSchema();
        return NULL;    /* annulation de commande */
    }

    if( Name.GetChar( 0 ) == '=' )
    {
        AllowWildSeach = FALSE;
        keys = Name.AfterFirst( '=' );
        if( DataBaseGetName( this, keys, Name ) == 0 )
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
        if( DataBaseGetName( this, keys, Name ) == 0 )
        {
            DrawPanel->m_IgnoreMouseEvents = FALSE;
            DrawPanel->MouseToCursorSchema();
            return NULL;
        }
    }

    Entry = FindLibPart( Name.GetData(), libname, FIND_ROOT );
    if( (Entry == NULL) && AllowWildSeach ) /* Attemp to search with wildcard */
    {
        AllowWildSeach = FALSE;
        wxString wildname = wxChar( '*' ) + Name + wxChar( '*' );
        Name = wildname;
        if( DataBaseGetName( this, keys, Name ) )
            Entry = FindLibPart( Name.GetData(), libname, FIND_ROOT );
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
        DisplayError( this, msg, 10 );
        return NULL;
    }

    AddHistoryComponentName( HistoryList, Name );

    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitPlaceCmp;

    Component = new SCH_COMPONENT( GetScreen()->m_Curseur );
    Component->m_Multi     = 1; /* Selection de l'unite 1 dans le boitier */
    Component->m_Convert   = 1;
    Component->m_ChipName  = Name;
    Component->m_TimeStamp = GetTimeStamp();
    Component->m_Flags     = IS_NEW | IS_MOVED;

    /* Init champ Valeur */
    Component->GetField( VALUE )->m_Pos = Entry->m_Name.m_Pos + Component->m_Pos;
    Component->GetField( VALUE )->ImportValues( Entry->m_Name );
    Component->GetField( VALUE )->m_Text = Component->m_ChipName;

    msg = Entry->m_Prefix.m_Text;
    if( msg.IsEmpty() )
        msg = wxT( "U" );
    msg += wxT( "?" );

    // update the reference -- just the prefix for now.
    Component->SetRef( GetSheet(), msg );

    /* Init champ Reference */
    Component->GetField( REFERENCE )->m_Pos = Entry->m_Prefix.m_Pos + Component->m_Pos;
    Component->GetField( REFERENCE )->ImportValues( Entry->m_Prefix );
    Component->m_PrefixString = Entry->m_Prefix.m_Text;

    /* Init des autres champs si predefinis dans la librairie */
    LibDrawField* EntryField;
    for( EntryField = Entry->m_Fields; EntryField != NULL; EntryField = EntryField->Next() )
    {
        if( EntryField->m_Text.IsEmpty() && EntryField->m_Name.IsEmpty() )
            continue;

        ii = EntryField->m_FieldId;
        if( ii < 2 )        // Reference or value, already done
            continue;

        if( ii >= Component->GetFieldCount() )
        {   // This entry has more than the default count: add extra fields
            while( ii >= Component->GetFieldCount() )
            {
                int field_id = Component->GetFieldCount();
                SCH_CMP_FIELD field( wxPoint( 0, 0 ), field_id, Component, ReturnDefaultFieldName( ii ) );
                Component->AddField( field );
            }
        }

        SCH_CMP_FIELD* curr_field = Component->GetField( ii );

        curr_field->m_Pos = Component->m_Pos + EntryField->m_Pos;
        curr_field->ImportValues( *EntryField );
        curr_field->m_Text = EntryField->m_Text;
        curr_field->m_Name = ( ii < FIELD1 ) ? ReturnDefaultFieldName( ii ) : EntryField->m_Name;
    }

    DrawStructsInGhost( DrawPanel, DC, Component, 0, 0 );

    MsgPanel->EraseMsgBox();
    Component->Display_Infos( this );

    return Component;
}


/**************************************************************************/
/***                Routine de deplacement du composant.                ***/
/***  Appele par GeneralControle grace a  ActiveScreen->ManageCurseur.  ***/
/**************************************************************************/
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    wxPoint        move_vector;

    SCH_SCREEN*    screen = (SCH_SCREEN*) panel->GetScreen();

    SCH_COMPONENT* Component = (SCH_COMPONENT*) screen->GetCurItem();

    /* Effacement du composant */
    if( erase )
    {
        DrawStructsInGhost( panel, DC, Component, 0, 0 );
    }

    move_vector.x = screen->m_Curseur.x - Component->m_Pos.x;
    move_vector.y = screen->m_Curseur.y - Component->m_Pos.y;
    MoveOneStruct( Component, move_vector );

    DrawStructsInGhost( panel, DC, Component, 0, 0 );
}


/**************************************************************************/
void WinEDA_SchematicFrame::CmpRotationMiroir(
    SCH_COMPONENT* DrawComponent, wxDC* DC, int type_rotate )
/**************************************************************************/

/* Routine permettant les rotations et les miroirs d'un composant
 *  Si DC = NULL : pas de redessin
 */
{
    if( DrawComponent == NULL )
        return;

    /* Efface le trace precedent */
    if( DC )
    {
        DrawPanel->CursorOff( DC );
        if( DrawComponent->m_Flags )
            DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
        else
        {
            DrawPanel->PostDirtyRect( DrawComponent->GetBoundingBox() );
        }
    }

    DrawComponent->SetRotationMiroir( type_rotate );

    /* Redessine le composant dans la nouvelle position */
    if( DC )
    {
        if( DrawComponent->m_Flags )
            DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
        else
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0,
                                                         0 ),
                                 GR_DEFAULT_DRAWMODE );
        DrawPanel->CursorOn( DC );
    }

    TestDanglingEnds( GetScreen()->EEDrawList, DC );
    GetScreen()->SetModify();
}


/************************************************************/
static void ExitPlaceCmp( WinEDA_DrawPanel* Panel, wxDC* DC )
/************************************************************/

/* Routine de sortie de la fonction de placement de composant
 */
{
    SCH_SCREEN*    screen = (SCH_SCREEN*) Panel->GetScreen();

    SCH_COMPONENT* Component = (SCH_COMPONENT*) screen->GetCurItem();

    if( Component->m_Flags & IS_NEW )    /* Nouveau Placement en cours, on l'efface */
    {
        Component->m_Flags = 0;
        SAFE_DELETE( Component );
    }
    else if( Component )   /* Deplacement ancien composant en cours */
    {
        wxPoint move_vector;

        move_vector.x = OldPos.x - Component->m_Pos.x;
        move_vector.y = OldPos.y - Component->m_Pos.y;

        MoveOneStruct( Component, move_vector );

        memcpy( Component->m_Transform, OldTransMat, sizeof(OldTransMat) );

        Component->m_Flags = 0;
    }

    Panel->Refresh( TRUE );

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    screen->SetCurItem( NULL );
}


/************************************************************************/
void WinEDA_SchematicFrame::SelPartUnit( SCH_COMPONENT* DrawComponent,
                                         int unit, wxDC* DC )
/************************************************************************/
/* Selection de l'unite dans les boitiers a multiples Parts */
{
    int m_UnitCount;
    EDA_LibComponentStruct* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = FindLibPart(
        DrawComponent->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( LibEntry == NULL )
        return;

    m_UnitCount = LibEntry->m_UnitCount;

    if( m_UnitCount <= 1 )
        return;

    if( DrawComponent->m_Multi == unit )
        return;
    if( unit < 1 )
        unit = 1;
    if( unit > m_UnitCount )
        unit = m_UnitCount;

    /* Efface le trace precedent */
    if( DrawComponent->m_Flags )
        DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    /* Mise a jour du numero d'unite */
    DrawComponent->SetUnitSelection( GetSheet(), unit );
    DrawComponent->m_Multi = unit;

    /* Redessine le composant dans la nouvelle position */
    if( DrawComponent->m_Flags )
        DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0,
                                                     0 ), GR_DEFAULT_DRAWMODE );

    TestDanglingEnds( GetScreen()->EEDrawList, DC );
    GetScreen()->SetModify();
}


/************************************************************************/
void WinEDA_SchematicFrame::ConvertPart( SCH_COMPONENT* DrawComponent,
                                         wxDC*          DC )
/************************************************************************/
{
    int ii;
    EDA_LibComponentStruct* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = FindLibPart(
        DrawComponent->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( LibEntry == NULL )
        return;

    if( ( ii = LookForConvertPart( LibEntry ) ) < 2 )
    {
        DisplayError( this, wxT( "No Convert found" ), 10 ); return;
    }

    /* Efface le trace precedent */
    if( DrawComponent->m_Flags )
        DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    DrawComponent->m_Convert++;
    if( DrawComponent->m_Convert > ii )
        DrawComponent->m_Convert = 1;

    /* Redessine le composant dans la nouvelle position */
    if( DrawComponent->m_Flags & IS_MOVED )
        DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0,
                                                     0 ), GR_DEFAULT_DRAWMODE );

    TestDanglingEnds( GetScreen()->EEDrawList, DC );
    GetScreen()->SetModify();
}


/**********************************************************/
int LookForConvertPart( EDA_LibComponentStruct* LibEntry )
/**********************************************************/

/* Retourne la plus grande valeur trouvee dans la liste des elements
 *  "drawings" du composant LibEntry, pour le membre .Convert
 *  Si il n'y a pas de representation type "convert", la valeur
 *  retournee est 0 ou 1
 *  Si il y a une representation type "convert",
 *  la valeur retournee est > 1 (typiquement 2)
 */
{
    int ii;
    LibEDA_BaseStruct* DrawLibEntry;

    DrawLibEntry = LibEntry->m_Drawings;
    ii = 0;
    while( DrawLibEntry )
    {
        if( ii < DrawLibEntry->m_Convert )
            ii = DrawLibEntry->m_Convert;
        DrawLibEntry = DrawLibEntry->Next();
    }

    return ii;
}


/***********************************************************************************/
void WinEDA_SchematicFrame::StartMovePart( SCH_COMPONENT* Component,
                                           wxDC*          DC )
/***********************************************************************************/
{
    if( Component == NULL )
        return;
    if( Component->Type() != TYPE_SCH_COMPONENT )
        return;

    if( Component->m_Flags == 0 )
    {
        if( g_ItemToUndoCopy )
        {
            SAFE_DELETE( g_ItemToUndoCopy );
        }
        g_ItemToUndoCopy = Component->GenCopy();
    }

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = Component->m_Pos;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitPlaceCmp;
    GetScreen()->SetCurItem( Component );
    OldPos = Component->m_Pos;
    memcpy( OldTransMat, Component->m_Transform, sizeof(OldTransMat) );

#if 1

    // switch from normal mode to xor mode for the duration of the move, first
    // by erasing fully any "normal drawing mode" primitives with the PostDirtyRect(),
    // then by drawing the first time in xor mode so that subsequent xor
    // drawing modes will fully erase this first copy.

    Component->m_Flags |= IS_MOVED; // omit redrawing the component, erase only
    DrawPanel->PostDirtyRect( Component->GetBoundingBox() );

    DrawStructsInGhost( DrawPanel, DC, Component, 0, 0 );

#else

    RedrawOneStruct( DrawPanel, DC, Component, g_XorMode );

    Component->m_Flags |= IS_MOVED;

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
#endif

    DrawPanel->m_AutoPAN_Request = TRUE;

    DrawPanel->CursorOn( DC );
}
