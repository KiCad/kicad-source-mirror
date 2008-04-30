
#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/******************************************************************/
void SetStructFather( EDA_BaseStruct* Struct, BASE_SCREEN* Screen )
/******************************************************************/
{
    switch( Struct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
    case DRAW_JUNCTION_STRUCT_TYPE:
    case TYPE_SCH_TEXT:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_COMPONENT:
    case DRAW_SEGMENT_STRUCT_TYPE:
    case DRAW_BUSENTRY_STRUCT_TYPE:
    case DRAW_SHEET_STRUCT_TYPE:
    case DRAW_MARKER_STRUCT_TYPE:
    case DRAW_NOCONNECT_STRUCT_TYPE:
        Struct->m_Parent = Screen;
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    case DRAW_PICK_ITEM_STRUCT_TYPE:
        break;

    default:
        break;
    }
}


/***************************************************************/
void SCH_ITEM::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
/***************************************************************/

/* place the struct in EEDrawList.
 *  if it is a new item, it it also put in undo list
 *  for an "old" item, saving it in undo list must be done before editiing, and not here!
 */
{
    if( m_Flags & IS_NEW )
    {
        SCH_SCREEN* screen = frame->GetScreen();
        if( !screen->CheckIfOnDrawList( this ) )  //don't want a loop!
            screen->AddToDrawList( this );
        g_ItemToRepeat = this;
        if( frame->m_Ident == SCHEMATIC_FRAME )
            frame->SaveCopyInUndoList( this, IS_NEW );
    }

    m_Flags = 0;
    frame->GetScreen()->SetModify();
    frame->GetScreen()->SetCurItem( NULL );
    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    if( DC )
    {
        frame->DrawPanel->CursorOff( DC );      // Erase schematic cursor
        RedrawOneStruct( frame->DrawPanel, DC, this, GR_DEFAULT_DRAWMODE );
        frame->DrawPanel->CursorOn( DC );       // Display schematic cursor
    }
}


/***********************************************************************/
/* Class SCH_SCREEN: classe de gestion d'un affichage pour schematique */
/***********************************************************************/
static int table_zoom[] = { 1, 2, 4, 8, 16, 32, 64, 128, 0 }; /* Valeurs standards du zoom */

/* Constructeur de SCREEN */
SCH_SCREEN::SCH_SCREEN( int screentype, KICAD_T aType ) :
    BASE_SCREEN( screentype, aType )
{
    EEDrawList = NULL;                  /* Schematic items list */
    m_Zoom = 32;
    m_Grid = wxSize( 50, 50 );          /* pas de la grille */
    SetZoomList( table_zoom );
    SetGridList( g_GridList );
    m_UndoRedoCountMax = 10;
    m_RefCount       = 0;
}


/****************************/
SCH_SCREEN::~SCH_SCREEN()
/****************************/
{
    ClearUndoRedoList();
    FreeDrawList();
}


/***********************************/
void SCH_SCREEN::FreeDrawList()
/***********************************/

/* Routine to clear (free) EESchema drawing list of a screen.
 */
{
    SCH_ITEM* DrawStruct;

    while( EEDrawList != NULL )
    {
        DrawStruct = EEDrawList;
        EEDrawList = EEDrawList->Next();
        SAFE_DELETE( DrawStruct );
    }

    EEDrawList = NULL;
}


/**************************************************************/
void SCH_SCREEN::RemoveFromDrawList( SCH_ITEM * DrawStruct )
/**************************************************************/

/* If found in EEDrawList, remove DrawStruct from EEDrawList.
 *  DrawStruct is not deleted or modified
 */
{
    if( DrawStruct == EEDrawList )
        EEDrawList = EEDrawList->Next();
    else
    {
        EDA_BaseStruct* DrawList = EEDrawList;
        while( DrawList && DrawList->Next() )
        {
            if( DrawList->Pnext == DrawStruct )
            {
                DrawList->Pnext = DrawList->Pnext->Pnext;
                break;
            }
            DrawList = DrawList->Next();
        }
    }
}


/**************************************************************/
bool SCH_SCREEN::CheckIfOnDrawList( SCH_ITEM* st )
/**************************************************************/
{
    SCH_ITEM * DrawList = EEDrawList;

    while( DrawList )
    {
        if( DrawList == st )
            return true;
        DrawList = DrawList->Next();
    }

    return false;
}


/**************************************************************/
void SCH_SCREEN::AddToDrawList( SCH_ITEM* st )
/**************************************************************/
{ //simple function to add to the head of the drawlist.
    st->Pnext  = EEDrawList;
    EEDrawList = st;
}


/*********************************************************************/
/* Class EDA_ScreenList to handle the list of screens in a hierarchy */
/*********************************************************************/

/********************************/
EDA_ScreenList::EDA_ScreenList()
/********************************/
{
    m_Index = 0;
    BuildScreenList( g_RootSheet );
}

/*****************************************/
SCH_SCREEN* EDA_ScreenList::GetFirst()
/*****************************************/
{
    m_Index = 0;
    if( m_List.GetCount() > 0 )
        return m_List[0];
    return NULL;
}


/*****************************************/
SCH_SCREEN* EDA_ScreenList::GetNext()
/*****************************************/
{
    if( m_Index < m_List.GetCount() )
        m_Index++;
    return GetScreen( m_Index );
}


/************************************************/
SCH_SCREEN* EDA_ScreenList::GetScreen( unsigned int index )
/************************************************/

/* return the m_List[index] item
 */
{
    if( index < m_List.GetCount() )
        return m_List[index];
    return NULL;
}


/************************************************/
void EDA_ScreenList::AddScreenToList( SCH_SCREEN* testscreen )
/************************************************/
{
    if( testscreen == NULL )
        return;
    for( unsigned int i = 0; i< m_List.GetCount(); i++ )
    {
        if( m_List[i] == testscreen )
            return;
    }

    m_List.Add( testscreen );
#ifdef DEBUG
    printf( "EDA_ScreenList::AddScreenToList adding %s\n",
        (const char*) testscreen->m_FileName.mb_str(
            ) );
#endif
}


/************************************************************************/
void EDA_ScreenList::BuildScreenList( EDA_BaseStruct* s )
/************************************************************************/
{
    if( s && s->Type() == DRAW_SHEET_STRUCT_TYPE )
    {
        DrawSheetStruct* ds = (DrawSheetStruct*) s;
        s = ds->m_AssociatedScreen;
    }
    if( s && s->Type() == SCREEN_STRUCT_TYPE )
    {
        SCH_SCREEN*     screen = (SCH_SCREEN*) s;
        AddScreenToList( screen );
        EDA_BaseStruct* strct = screen->EEDrawList;
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                BuildScreenList( strct );
            }
            strct = strct->Pnext;
        }
    }
}


/*********************************************************************/
/* Class EDA_SheetList to handle the list of Sheets in a hierarchy */
/*********************************************************************/

/*****************************************/
DrawSheetPath* EDA_SheetList::GetFirst()
/*****************************************/
{
    m_index = 0;
    if( m_count > 0 )
        return &( m_List[0] );
    return NULL;
}


/*****************************************/
DrawSheetPath* EDA_SheetList::GetNext()
/*****************************************/
{
    if( m_index < m_count )
        m_index++;
    return GetSheet( m_index );
}


/************************************************/
DrawSheetPath* EDA_SheetList::GetSheet( int index )
/************************************************/

/* return the m_List[index] item
 */
{
    if( index < m_count )
        return &(m_List[index]);
    return NULL;
}


/************************************************************************/
void EDA_SheetList::BuildSheetList( DrawSheetStruct* sheet )
/************************************************************************/
{
    if( m_List == NULL )
    {
        int count = sheet->CountSheets();
        m_count = count;
        m_index = 0;
        if( m_List )
            free( m_List );m_List = NULL;
        count *= sizeof(DrawSheetPath);
        m_List = (DrawSheetPath*) MyZMalloc( count );
        memset( (void*) m_List, 0, count );
        m_currList.Clear();
    }
    m_currList.Push( sheet );
    m_List[m_index] = m_currList;
    m_index++;
    if( sheet->m_AssociatedScreen != NULL )
    {
        EDA_BaseStruct* strct = m_currList.LastDrawList();
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* sht = (DrawSheetStruct*) strct;
                BuildSheetList( sht );
            }
            strct = strct->Pnext;
        }
    }
    m_currList.Pop();
}

