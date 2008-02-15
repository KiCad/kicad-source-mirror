/*************************************************/
/*	Module to handle Get & Place Library Part	 */
/*************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"

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
    wxString             name;
    WinEDA_ViewlibFrame* Viewer;
    wxSemaphore          semaphore( 0, 1 );

    Viewer = parent->m_Parent->m_ViewlibFrame;
    /* Close the current Lib browser, if open, and open a new one, in "modal" mode */
    if( Viewer )
        Viewer->Destroy();

    Viewer = parent->m_Parent->m_ViewlibFrame = new
                                              WinEDA_ViewlibFrame(
                 parent->m_Parent->m_SchematicFrame,
                 parent->m_Parent,
                 NULL,
                 &semaphore );
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
EDA_SchComponentStruct* WinEDA_SchematicFrame::Load_Component( wxDC* DC,
                                                               const wxString& libname,
                                                               wxArrayString& HistoryList,
                                                               bool UseLibBrowser )
/**************************************************************************/

/* load from a library and place a component
 *  if libname != "", search in lib "libname"
 *  else search in all loaded libs
 */
{
    int                     ii, CmpCount = 0;
    LibDrawField*           Field;
    EDA_LibComponentStruct* Entry = NULL;
    EDA_SchComponentStruct* DrawLibItem = NULL;
    LibraryStruct*          Library = NULL;
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

    DrawLibItem = new EDA_SchComponentStruct( GetScreen()->m_Curseur );
    DrawLibItem->m_Multi     = 1;/* Selection de l'unite 1 dans le boitier */
    DrawLibItem->m_Convert   = 1;
    DrawLibItem->m_ChipName  = Name;
	DrawLibItem->m_TimeStamp = GetTimeStamp();
    DrawLibItem->m_Flags = IS_NEW | IS_MOVED;

    /* Init champ Valeur */
    DrawLibItem->m_Field[VALUE].m_Pos.x =
        Entry->m_Name.m_Pos.x + DrawLibItem->m_Pos.x;
    DrawLibItem->m_Field[VALUE].m_Pos.y =
        Entry->m_Name.m_Pos.y + DrawLibItem->m_Pos.y;
    DrawLibItem->m_Field[VALUE].m_Orient    = Entry->m_Name.m_Orient;
    DrawLibItem->m_Field[VALUE].m_Size      = Entry->m_Name.m_Size;
    DrawLibItem->m_Field[VALUE].m_Text      = DrawLibItem->m_ChipName;
    DrawLibItem->m_Field[VALUE].m_Attributs = Entry->m_Name.m_Attributs;
    DrawLibItem->m_Field[VALUE].m_HJustify  = Entry->m_Name.m_HJustify;
    DrawLibItem->m_Field[VALUE].m_VJustify  = Entry->m_Name.m_VJustify;

    msg = Entry->m_Prefix.m_Text;
    if( msg.IsEmpty() )
        msg = wxT( "U" );
    msg += wxT( "?" );
	//update the reference -- just the prefix for now. 
	DrawLibItem->SetRef(GetSheet(), msg ); 

    /* Init champ Reference */
    DrawLibItem->m_Field[REFERENCE].m_Pos.x =
        Entry->m_Prefix.m_Pos.x + DrawLibItem->m_Pos.x;
    DrawLibItem->m_Field[REFERENCE].m_Pos.y =
        Entry->m_Prefix.m_Pos.y + DrawLibItem->m_Pos.y;
    DrawLibItem->m_Field[REFERENCE].m_Orient    = Entry->m_Prefix.m_Orient;
    DrawLibItem->m_Field[REFERENCE].m_Size      = Entry->m_Prefix.m_Size;
	DrawLibItem->m_PrefixString = Entry->m_Prefix.m_Text;
    DrawLibItem->m_Field[REFERENCE].m_Attributs = Entry->m_Prefix.m_Attributs;
    DrawLibItem->m_Field[REFERENCE].m_HJustify  = Entry->m_Prefix.m_HJustify;
    DrawLibItem->m_Field[REFERENCE].m_VJustify  = Entry->m_Prefix.m_VJustify;

    /* Init des autres champs si predefinis dans la librairie */
    for( Field = Entry->Fields; Field != NULL; Field = (LibDrawField*) Field->Pnext )
    {
        if( Field->m_Text.IsEmpty() && Field->m_Name.IsEmpty() )
            continue;
        ii = Field->m_FieldId;
        if( ii < 2 )
            continue;
        if( ii >= NUMBER_OF_FIELDS )
            continue;
        DrawLibItem->m_Field[ii].m_Pos.x    += Field->m_Pos.x;
        DrawLibItem->m_Field[ii].m_Pos.y    += Field->m_Pos.y;
        DrawLibItem->m_Field[ii].m_Size      = Field->m_Size;
        DrawLibItem->m_Field[ii].m_Attributs = Field->m_Attributs;
        DrawLibItem->m_Field[ii].m_Orient    = Field->m_Orient;
        DrawLibItem->m_Field[ii].m_Text     = Field->m_Text;
        DrawLibItem->m_Field[ii].m_Name     = Field->m_Name;
        DrawLibItem->m_Field[ii].m_HJustify = Field->m_HJustify;
        DrawLibItem->m_Field[ii].m_VJustify = Field->m_VJustify;
    }

    /* Trace du composant */
    DrawStructsInGhost( DrawPanel, DC, DrawLibItem, 0, 0 );
    MsgPanel->EraseMsgBox();
    DrawLibItem->Display_Infos( this );

    return DrawLibItem;
}


/**************************************************************************/
/*** 				Routine de deplacement du composant. 				***/
/***  Appele par GeneralControle grace a  ActiveScreen->ManageCurseur.  ***/
/**************************************************************************/
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    wxPoint move_vector;

    EDA_SchComponentStruct* DrawLibItem = (EDA_SchComponentStruct*)
			panel->m_Parent->GetScreen()->GetCurItem();

    /* Effacement du composant */
    if( erase )
        DrawStructsInGhost( panel, DC, DrawLibItem, 0, 0 );

	move_vector.x = panel->m_Parent->GetScreen()->m_Curseur.x - DrawLibItem->m_Pos.x;
	move_vector.y = panel->m_Parent->GetScreen()->m_Curseur.y - DrawLibItem->m_Pos.y;
    MoveOneStruct( DrawLibItem, move_vector );

    DrawStructsInGhost( panel, DC, DrawLibItem, 0, 0 );
}


/**************************************************************************/
void WinEDA_SchematicFrame::CmpRotationMiroir(
    EDA_SchComponentStruct* DrawComponent, wxDC* DC, int type_rotate )
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
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    }

    DrawComponent->SetRotationMiroir( type_rotate );

    /* Redessine le composant dans la nouvelle position */
    if( DC )
    {
        if( DrawComponent->m_Flags )
            DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
        else
            DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
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
    EDA_SchComponentStruct* DrawLibItem = (EDA_SchComponentStruct*)
			Panel->m_Parent->GetScreen()->GetCurItem();

    if( DrawLibItem->m_Flags & IS_NEW )    /* Nouveau Placement en cours, on l'efface */
    {
        DrawStructsInGhost( Panel, DC, DrawLibItem, 0, 0 );
		SAFE_DELETE( DrawLibItem );
    }
    else if( DrawLibItem )   /* Deplacement ancien composant en cours */
    {
        wxPoint move_vector;
        DrawStructsInGhost( Panel, DC, DrawLibItem, 0, 0 );

        move_vector.x = OldPos.x - DrawLibItem->m_Pos.x;
        move_vector.y = OldPos.y - DrawLibItem->m_Pos.y;

        MoveOneStruct( DrawLibItem, move_vector );

        memcpy( DrawLibItem->m_Transform, OldTransMat, sizeof(OldTransMat) );
        DrawLibItem->Draw( Panel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    }

    DrawLibItem->m_Flags = 0;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
	Panel->m_Parent->GetScreen()->SetCurItem( NULL );
}


/************************************************************************/
void WinEDA_SchematicFrame::SelPartUnit( EDA_SchComponentStruct* DrawComponent,
                                         int unit, wxDC* DC )
/************************************************************************/
/* Selection de l'unite dans les boitiers a multiples Parts */
{
    int m_UnitCount;
    EDA_LibComponentStruct* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = FindLibPart( DrawComponent->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
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
    DrawComponent->m_Multi = unit;

    /* Redessine le composant dans la nouvelle position */
    if( DrawComponent->m_Flags )
        DrawStructsInGhost( DrawPanel, DC, DrawComponent, 0, 0 );
    else
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

	TestDanglingEnds( GetScreen()->EEDrawList, DC );
    GetScreen()->SetModify();
}


/************************************************************************/
void WinEDA_SchematicFrame::ConvertPart( EDA_SchComponentStruct* DrawComponent,
                                         wxDC*                   DC )
/************************************************************************/
{
    int ii;
    EDA_LibComponentStruct* LibEntry;

    if( DrawComponent == NULL )
        return;

    LibEntry = FindLibPart( DrawComponent->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
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
        DrawComponent->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

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
void WinEDA_SchematicFrame::StartMovePart( EDA_SchComponentStruct* Component,
                                           wxDC*                   DC )
/***********************************************************************************/
{
    if( Component == NULL )
        return;
    if( Component->Type() != DRAW_LIB_ITEM_STRUCT_TYPE )
        return;

    if( Component->m_Flags == 0 )
    {
        if( g_ItemToUndoCopy ){
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

    RedrawOneStruct( DrawPanel, DC, Component, g_XorMode );
    Component->m_Flags |= IS_MOVED;
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    DrawPanel->m_AutoPAN_Request = TRUE;

    DrawPanel->CursorOn( DC );
}
