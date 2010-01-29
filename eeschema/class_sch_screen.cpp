
#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"

#include "program.h"
#include "general.h"
#include "protos.h"


void SetaParent( EDA_BaseStruct* Struct, BASE_SCREEN* Screen )
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
    case TYPE_SCH_MARKER:
    case DRAW_NOCONNECT_STRUCT_TYPE:
        Struct->SetParent( Screen );
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        break;

    default:
        break;
    }
}


/* Default EESchema zoom values. Limited to 17 values to keep a decent size
 * to menus
 */
static int SchematicZoomList[] =
{
    5, 7, 10, 15, 20, 30, 40, 60, 80, 120, 160, 230, 320, 480, 640, 800, 1280
};

#define SCHEMATIC_ZOOM_LIST_CNT  ( sizeof( SchematicZoomList ) / \
                                   sizeof( int ) )
#define MM_TO_SCH_UNITS 1000.0 / 25.4       //schematic internal unites are mils


/* Default grid sizes for the schematic editor.
 * Do NOT add others values (mainly grid values in mm),
 * because they can break the schematic:
 * because wires and pins are considered as connected when the are to the same coordinate
 * we cannot mix coordinates in mils (internal units) and mm
 * (that cannot exactly converted in mils in many cases
 * in fact schematic must only use 50 and 25 mils to place labnels, wires and components
 * others values are useful only for graphic items (mainly in library editor)
 * so use integer values in mils only.
*/
static GRID_TYPE SchematicGridList[] = {
    { ID_POPUP_GRID_LEVEL_50, wxRealPoint( 50, 50 ) },
    { ID_POPUP_GRID_LEVEL_25, wxRealPoint( 25, 25 ) },
    { ID_POPUP_GRID_LEVEL_10, wxRealPoint( 10, 10 ) },
    { ID_POPUP_GRID_LEVEL_5, wxRealPoint( 5, 5 ) },
    { ID_POPUP_GRID_LEVEL_2, wxRealPoint( 2, 2 ) },
    { ID_POPUP_GRID_LEVEL_1, wxRealPoint( 1, 1 ) },
};

#define SCHEMATIC_GRID_LIST_CNT ( sizeof( SchematicGridList ) / \
                                  sizeof( GRID_TYPE ) )


SCH_SCREEN::SCH_SCREEN( KICAD_T type ) : BASE_SCREEN( type )
{
    size_t i;

    EEDrawList = NULL;                  /* Schematic items list */
    m_Zoom = 32;

    for( i = 0; i < SCHEMATIC_ZOOM_LIST_CNT; i++ )
        m_ZoomList.Add( SchematicZoomList[i] );

    for( i = 0; i < SCHEMATIC_GRID_LIST_CNT; i++ )
        AddGrid( SchematicGridList[i] );

    SetGrid( wxRealPoint( 50, 50 ) );   /* Default grid size. */
    m_RefCount = 0;
    m_Center = false;                   /* Suitable for schematic only. For
                                         * libedit and viewlib, must be set
                                         * to true */
    InitDatas();
}


SCH_SCREEN::~SCH_SCREEN()
{
    ClearUndoRedoList();
    FreeDrawList();
}


/* Routine to clear (free) EESchema drawing list of a screen.
 */
void SCH_SCREEN::FreeDrawList()
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


/* If found in EEDrawList, remove DrawStruct from EEDrawList.
 *  DrawStruct is not deleted or modified
 */
void SCH_SCREEN::RemoveFromDrawList( SCH_ITEM * DrawStruct )
{
    if( DrawStruct == EEDrawList )
        EEDrawList = EEDrawList->Next();
    else
    {
        EDA_BaseStruct* DrawList = EEDrawList;
        while( DrawList && DrawList->Next() )
        {
            if( DrawList->Next() == DrawStruct )
            {
                DrawList->SetNext( DrawList->Next()->Next() );
                break;
            }
            DrawList = DrawList->Next();
        }
    }
}


bool SCH_SCREEN::CheckIfOnDrawList( SCH_ITEM* st )
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


void SCH_SCREEN::AddToDrawList( SCH_ITEM* st )
{
    st->SetNext( EEDrawList );
    EEDrawList = st;
}


/*********************************************************************/
/* Class EDA_ScreenList to handle the list of screens in a hierarchy */
/*********************************************************************/

EDA_ScreenList::EDA_ScreenList()
{
    m_Index = 0;
    BuildScreenList( g_RootSheet );
}


SCH_SCREEN* EDA_ScreenList::GetFirst()
{
    m_Index = 0;
    if( m_List.GetCount() > 0 )
        return m_List[0];
    return NULL;
}


SCH_SCREEN* EDA_ScreenList::GetNext()
{
    if( m_Index < m_List.GetCount() )
        m_Index++;
    return GetScreen( m_Index );
}


/* return the m_List[index] item
 */
SCH_SCREEN* EDA_ScreenList::GetScreen( unsigned int index )
{
    if( index < m_List.GetCount() )
        return m_List[index];
    return NULL;
}


void EDA_ScreenList::AddScreenToList( SCH_SCREEN* testscreen )
{
    if( testscreen == NULL )
        return;
    for( unsigned int i = 0; i< m_List.GetCount(); i++ )
    {
        if( m_List[i] == testscreen )
            return;
    }

    m_List.Add( testscreen );
}


void EDA_ScreenList::BuildScreenList( EDA_BaseStruct* s )
{
    if( s && s->Type() == DRAW_SHEET_STRUCT_TYPE )
    {
        SCH_SHEET* ds = (SCH_SHEET*) s;
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
            strct = strct->Next();
        }
    }
}
