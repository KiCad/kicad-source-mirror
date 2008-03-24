/***************************************************************/
/* base_screen.cpp - fonctions des classes du type BASE_SCREEN */
/***************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"


/* defines locaux */
#define CURSOR_SIZE 12  /* taille de la croix du curseur PCB */

/*******************************************************/
/* Class BASE_SCREEN: classe de gestion d'un affichage */
/*******************************************************/
BASE_SCREEN::BASE_SCREEN( int idscreen, KICAD_T aType ) :
    EDA_BaseStruct( aType )
{
    EEDrawList = NULL;   /* Schematic items list */
    m_Type     = idscreen;
    m_ZoomList = NULL;
    m_GridList = NULL;
    m_UndoList = NULL;
    m_RedoList = NULL;
    m_UndoRedoCountMax = 1;
    m_FirstRedraw = TRUE;
    InitDatas();
}


/******************************/
BASE_SCREEN::~BASE_SCREEN()
/******************************/
{
    if( m_ZoomList )
        free( m_ZoomList );

    if( m_GridList )
        free( m_GridList );

    ClearUndoRedoList();
}


/*******************************/
void BASE_SCREEN::InitDatas()
/*******************************/
{
    m_ScreenNumber = m_NumberOfScreen = 1;    /* Hierarchy: Root: ScreenNumber = 1 */
    m_Zoom            = 32;
    m_Grid            = wxSize( 50, 50 );   /* Default grid size */
    m_UserGrid        = g_UserGrid;         /* User Default grid size */
    m_UserGridIsON    = FALSE;
    m_UserGridUnit    = g_UserGrid_Unit;
    m_Diviseur_Grille = 1;
    m_Center          = TRUE;

    /* Init draw offset and default page size */
    switch( m_Type )
    {
    case SCHEMATIC_FRAME:
        m_Center = FALSE;
        m_CurrentSheetDesc = &g_Sheet_A4;
        break;

    default:
    case CVPCB_DISPLAY_FRAME:
    case MODULE_EDITOR_FRAME:
    case PCB_FRAME:
        m_CurrentSheetDesc = &g_Sheet_A4;
        break;

    case GERBER_FRAME:
        m_CurrentSheetDesc = &g_Sheet_GERBER;
        break;
    }

    if( m_Center )
    {
        m_Curseur.x = m_Curseur.y = 0;
        m_DrawOrg.x = -ReturnPageSize().x / 2;
        m_DrawOrg.y = -ReturnPageSize().y / 2;
    }
    else
    {
        m_DrawOrg.x = m_DrawOrg.y = 0;
        m_Curseur.x = ReturnPageSize().x / 2;
        m_Curseur.y = ReturnPageSize().y / 2;
    }

    // DrawOrg est rendu multiple du zoom min :
    m_DrawOrg.x -= m_DrawOrg.x % 256;
    m_DrawOrg.y -= m_DrawOrg.y % 256;

    m_O_Curseur = m_Curseur;

    SetCurItem( NULL );

    /* indicateurs divers */
    m_FlagRefreshReq = 0;               /* Redraw screen requste flag */
    m_FlagModified   = 0;               // Set when any change is made on borad
    m_FlagSave = 1;                     // Used in auto save: set when an auto save is made
}


/******************************************************************/
wxPoint BASE_SCREEN::CursorRealPosition( const wxPoint& ScreenPos )
/******************************************************************/
{
    wxPoint curpos;

//    D(printf("curpos=%d,%d GetZoom=%d, mDrawOrg=%d,%d\n", curpos.x, curpos.y, GetZoom(), m_DrawOrg.x, m_DrawOrg.y );)

    curpos.x = ScreenPos.x * GetZoom();
    curpos.y = ScreenPos.y * GetZoom();

    curpos.x += m_DrawOrg.x;
    curpos.y += m_DrawOrg.y;

    return curpos;
}


/***************************************/
int BASE_SCREEN::GetInternalUnits()
/***************************************/
{
    switch( m_Type )
    {
    default:
    case SCHEMATIC_FRAME:
        return EESCHEMA_INTERNAL_UNIT;
        break;

    case GERBER_FRAME:
    case CVPCB_DISPLAY_FRAME:
    case MODULE_EDITOR_FRAME:
    case PCB_FRAME:
        return PCB_INTERNAL_UNIT;
    }
}


/*****************************************/
wxSize BASE_SCREEN::ReturnPageSize()
/*****************************************/

/* Return in internal units the page size
 *  Note: the page size is handled in 1/1000 ", not in internal units
 */
{
    wxSize PageSize;

    switch( m_Type )
    {
    default:
    case SCHEMATIC_FRAME:
        PageSize = m_CurrentSheetDesc->m_Size;
        break;

    case GERBER_FRAME:
    case CVPCB_DISPLAY_FRAME:
    case MODULE_EDITOR_FRAME:
    case PCB_FRAME:
        PageSize.x = m_CurrentSheetDesc->m_Size.x * (PCB_INTERNAL_UNIT / 1000);
        PageSize.y = m_CurrentSheetDesc->m_Size.y * (PCB_INTERNAL_UNIT / 1000);
        break;
    }

    return PageSize;
}


/**************************************************/
void BASE_SCREEN::SetZoomList( const int* zoomlist )
/**************************************************/

/* init liste des zoom (NULL terminated)
 */
{
    int         nbitems;
    const int*  zoom;

    // get list length
    for( nbitems = 1, zoom = zoomlist;  ; zoom++, nbitems++ )
    {
        if( *zoom == 0 )
            break;
    }

    // resize our list
    if( m_ZoomList )
        free( m_ZoomList );

    m_ZoomList = (int*) MyZMalloc( nbitems * sizeof(int) );

    int ii;
    for( ii = 0, zoom = zoomlist; ii < nbitems; zoom++, ii++ )
    {
        m_ZoomList[ii] = *zoom;
    }
}


/***********************************/
void BASE_SCREEN::SetFirstZoom()
/***********************************/
{
    m_Zoom = 1;
}


/******************************/
int BASE_SCREEN::GetZoom() const
/******************************/
{
    return m_Zoom;
}


/***********************************/
void BASE_SCREEN::SetZoom( int coeff )
/***********************************/
{
    m_Zoom = coeff;
    if( m_Zoom < 1 )
        m_Zoom = 1;
}


/********************************/
void BASE_SCREEN::SetNextZoom()
/********************************/

/* Selectionne le prochain coeff de zoom
 */
{
    m_Zoom *= 2;

    if( m_ZoomList == NULL )
        return;

    int ii, zoom_max = 512;
    for( ii = 0; m_ZoomList[ii] != 0; ii++ )
        zoom_max = m_ZoomList[ii];

    if( m_Zoom > zoom_max )
        m_Zoom = zoom_max;
}


/*************************************/
void BASE_SCREEN::SetPreviousZoom()
/*************************************/

/* Selectionne le precedent coeff de zoom
 */
{
    m_Zoom /= 2;
    if( m_Zoom < 1 )
        m_Zoom = 1;
}


/**********************************/
void BASE_SCREEN::SetLastZoom()
/**********************************/

/* ajuste le coeff de zoom au max
 */
{
    if( m_ZoomList == NULL )
        return;
    int ii;
    for( ii = 0; m_ZoomList[ii] != 0; ii++ )
        m_Zoom = m_ZoomList[ii];
}


/********************************************/
void BASE_SCREEN::SetGridList( wxSize* gridlist )
/********************************************/

/* init liste des zoom (NULL terminated)
 */
{
    int     ii, nbitems;
    wxSize* grid;

    // Decompte des items
    for( nbitems = 0, grid = gridlist;  ; grid++, nbitems++ )
    {
        if( (grid->x <= 0) || (grid->y <= 0) )
            break;
    }

    // Init liste
    if( m_GridList )
        free( m_GridList );
    m_GridList = (wxSize*) MyZMalloc( nbitems * sizeof(wxSize) );

    for( ii = 0, grid = gridlist; ii < nbitems; grid++, ii++ )
    {
        m_GridList[ii] = *grid;
    }
}


/**********************************************/
void BASE_SCREEN::SetGrid( const wxSize& size )
/**********************************************/
{
    if( m_GridList == NULL )
        return;

    if( (size.x <= 0) || (size.y <= 0) )
    {
        m_UserGrid     = g_UserGrid;
        m_UserGridIsON = TRUE;
    }
    else
    {
        m_Grid = size;
        m_UserGridIsON = FALSE;
    }
}


/*********************************/
wxSize BASE_SCREEN::GetGrid()
/*********************************/
{
    wxSize grid = m_Grid;
    double xx, scale;

    if( m_GridList == NULL )
        return wxSize( 1, 1 );

    if( m_UserGridIsON || m_Grid.x < 0 || m_Grid.y < 0 )
    {
        if( m_UserGridUnit == INCHES )
            scale = 10000;
        else
            scale = 10000 / 25.4;
        xx     = m_UserGrid.x * scale;
        grid.x = (int) (xx + 0.5);
        xx     = m_UserGrid.y * scale;
        grid.y = (int) (xx + 0.5);
    }
    return grid;
}


/*********************************/
void BASE_SCREEN::SetNextGrid()
/*********************************/

/* Selectionne la prochaine grille
 */
{
    int ii;

    if( m_GridList == NULL )
        return;

    for( ii = 0; ; ii++ )
    {
        if( m_GridList[ii].x <= 0 )
            break;
        if( (m_Grid.x == m_GridList[ii].x) && (m_Grid.y == m_GridList[ii].y) )
            break;
    }

    if( (m_GridList[ii].x > 0) && (ii > 0) )
        m_Grid = m_GridList[ii - 1];
}


/*************************************/
void BASE_SCREEN::SetPreviousGrid()
/*************************************/

/* Selectionne le precedent coeff de grille
 */
{
    int ii;

    if( m_GridList == NULL )
        return;

    for( ii = 0; ; ii++ )
    {
        if( m_GridList[ii].x <= 0 )
            break;
        if( (m_Grid.x == m_GridList[ii].x) && (m_Grid.y == m_GridList[ii].y) )
            break;
    }

    if( (m_GridList[ii].x > 0) && (m_GridList[ii + 1].x > 0) )
        m_Grid = m_GridList[ii + 1];
}


/**********************************/
void BASE_SCREEN::SetFirstGrid()
/**********************************/

/* ajuste le coeff de grille a 1
 */
{
    if( m_GridList == NULL )
        return;

    int ii = 0;
    while( m_GridList[ii].x > 0 )
        ii++;

    m_Grid = m_GridList[ii - 1];
}


/**********************************/
void BASE_SCREEN::SetLastGrid()
/**********************************/

/* ajuste le coeff de grille au max
 */
{
    if( m_GridList == NULL )
        return;
    m_Grid = m_GridList[0];
}


/*****************************************/
void BASE_SCREEN::ClearUndoRedoList()
/*****************************************/

/* free the undo and the redo lists
 */
{
    EDA_BaseStruct* nextitem;

    while( m_UndoList )
    {
        nextitem = m_UndoList->Pnext;
        delete m_UndoList;
        m_UndoList = nextitem;
    }

    while( m_RedoList )
    {
        nextitem = m_RedoList->Pnext;
        delete m_RedoList;
        m_RedoList = nextitem;
    }
}


/***********************************************************/
void BASE_SCREEN::AddItemToUndoList( EDA_BaseStruct* newitem )
/************************************************************/

/* Put newitem in head of undo list
 *  Deletes olds items if > count max.
 */
{
    int             ii;
    EDA_BaseStruct* item, * nextitem;

    if( newitem == NULL )
        return;

    newitem->Pnext = m_UndoList;
    m_UndoList = newitem;

    /* Free first items, if count max reached */
    for( ii = 0, item = m_UndoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Pnext == NULL )
            return;
        item = item->Pnext;
    }

    if( item == NULL )
        return;

    nextitem    = item->Pnext;
    item->Pnext = NULL; // Set end of chain

    // Delete the extra  items
    for( item = nextitem; item != NULL; item = nextitem )
    {
        nextitem = item->Pnext;
        delete item;
    }
}


/***********************************************************/
void BASE_SCREEN::AddItemToRedoList( EDA_BaseStruct* newitem )
/***********************************************************/
{
    int             ii;
    EDA_BaseStruct* item, * nextitem;

    if( newitem == NULL )
        return;

    newitem->Pnext = m_RedoList;
    m_RedoList = newitem;
    /* Free first items, if count max reached */
    for( ii = 0, item = m_RedoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Pnext == NULL )
            break;
        item = item->Pnext;
    }

    if( item == NULL )
        return;

    nextitem    = item->Pnext;
    item->Pnext = NULL; // Set end of chain

    // Delete the extra items
    for( item = nextitem; item != NULL; item = nextitem )
    {
        nextitem = item->Pnext;
        delete item;
    }
}


/*****************************************************/
EDA_BaseStruct* BASE_SCREEN::GetItemFromUndoList()
/*****************************************************/
{
    EDA_BaseStruct* item = m_UndoList;

    if( item )
        m_UndoList = item->Pnext;
    return item;
}


/******************************************************/
EDA_BaseStruct* BASE_SCREEN::GetItemFromRedoList()
/******************************************************/
{
    EDA_BaseStruct* item = m_RedoList;

    if( item )
        m_RedoList = item->Pnext;
    return item;
}


#if defined(DEBUG)
/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void BASE_SCREEN::Show( int nestLevel, std::ostream& os )
{
    EDA_BaseStruct* item = EEDrawList;

    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
        ">\n";

    for(  ; item;  item = item->Next() )
    {
        item->Show( nestLevel+1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif

