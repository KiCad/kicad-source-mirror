/***************************************************************/
/* base_screen.cpp - fonctions des classes du type BASE_SCREEN */
/***************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "sch_item_struct.h"

/* Implement wxSize array for grid list implementation. */
#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY( GridArray );


/* defines locaux */
#define CURSOR_SIZE 12  /* taille de la croix du curseur PCB */

/*******************************************************/
/* Class BASE_SCREEN: classe de gestion d'un affichage */
/*******************************************************/
BASE_SCREEN::BASE_SCREEN( KICAD_T aType ) : EDA_BaseStruct( aType )
{
    EEDrawList         = NULL;   /* Schematic items list */
    m_ZoomList         = NULL;
    m_UndoList         = NULL;
    m_RedoList         = NULL;
    m_UndoRedoCountMax = 1;
    m_FirstRedraw      = TRUE;
    m_ScreenNumber     = 1;
    m_NumberOfScreen   = 1;  /* Hierarchy: Root: ScreenNumber = 1 */
    m_Zoom             = 32;
    m_Grid             = wxSize( 50, 50 );   /* Default grid size */
    m_UserGridIsON     = FALSE;
    m_Diviseur_Grille  = 1;
    m_Center           = true;
    m_CurrentSheetDesc = &g_Sheet_A4;

    InitDatas();
}


/******************************/
BASE_SCREEN::~BASE_SCREEN()
/******************************/
{
    if( m_ZoomList )
        free( m_ZoomList );

    ClearUndoRedoList();
}


/*******************************/
void BASE_SCREEN::InitDatas()
/*******************************/
{
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
    m_FlagRefreshReq = 0;   /* Redraw screen requste flag */
    m_FlagModified   = 0;   // Set when any change is made on borad
    m_FlagSave = 1;         // Used in auto save: set when an auto save is made
}

/*
 * Get screen units scalar.
 *
 * Default implimentation returns scalar used for schematic screen.  The
 * internal units used by the schematic screen is 1 mil (0.001").  Override
 * this in derived classes that require internal units other than 1 mil.
 */
int BASE_SCREEN::GetInternalUnits( void )
{
    return EESCHEMA_INTERNAL_UNIT;
}

wxSize BASE_SCREEN::ReturnPageSize( void )
{
    int internal_units = GetInternalUnits();

    return wxSize( m_CurrentSheetDesc->m_Size.x * (internal_units / 1000),
                   m_CurrentSheetDesc->m_Size.y * (internal_units / 1000) );
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
void BASE_SCREEN::SetGridList( GridArray& gridlist )
/********************************************/

/* init liste des zoom (NULL terminated)
 */
{
    if( !m_GridList.IsEmpty() )
        m_GridList.Clear();

    m_GridList = gridlist;
}


/**********************************************/
void BASE_SCREEN::SetGrid( const wxSize& size )
/**********************************************/
{
    wxASSERT( !m_GridList.IsEmpty() );

    size_t i;

    for( i = 0; i < m_GridList.GetCount(); i++ )
    {
        if( m_GridList[i].m_Size == size )
        {
            m_Grid = m_GridList[i].m_Size;
            return;
        }
    }

    m_Grid = m_GridList[0].m_Size;

    wxLogWarning( _( "Grid size( %d, %d ) not in grid list, falling back to " \
                     "grid size( %d, %d )." ),
                  size.x, size.y, m_Grid.x, m_Grid.y );
}

/* Set grid size from command ID. */
void BASE_SCREEN::SetGrid( int id  )
{
    wxASSERT( !m_GridList.IsEmpty() );

    size_t i;

    for( i = 0; i < m_GridList.GetCount(); i++ )
    {
        if( m_GridList[i].m_Id == id )
        {
            m_Grid = m_GridList[i].m_Size;
            return;
        }
    }

    m_Grid = m_GridList[0].m_Size;

    wxLogWarning( _( "Grid ID %d not in grid list, falling back to " \
                     "grid size( %d, %d )." ), id, m_Grid.x, m_Grid.y );
}

void BASE_SCREEN::AddGrid( const GRID_TYPE& grid )
{
    size_t i;

    for( i = 0; i < m_GridList.GetCount(); i++ )
    {
        if( m_GridList[i].m_Size == grid.m_Size )
        {
            wxLogDebug( wxT( "Discarding duplicate grid size( %d, %d )." ),
                        grid.m_Size.x, grid.m_Size.y );
            return;
        }
        if( m_GridList[i].m_Id == grid.m_Id )
        {
            wxLogDebug( wxT( "Changing grid ID %d from size( %d, %d ) to " \
                             "size( %d, %d )." ),
                        grid.m_Id, m_GridList[i].m_Size.x,
                        m_GridList[i].m_Size.y, grid.m_Size.x, grid.m_Size.y );
            m_GridList[i].m_Size = grid.m_Size;
            return;
        }
    }

    // wxLogDebug( wxT( "Adding grid ID %d size( %d, %d ) to grid list." ), grid.m_Id, grid.m_Size.x, grid.m_Size.y );

    m_GridList.Add( grid );
}

void BASE_SCREEN::AddGrid( const wxSize& size, int id )
{
    GRID_TYPE grid;

    grid.m_Size = size;
    grid.m_Id = id;
    AddGrid( grid );
}

void BASE_SCREEN::AddGrid( const wxRealPoint& size, int units, int id )
{
    double x, y;
    wxSize new_size;
    GRID_TYPE new_grid;

    if( units == MILLIMETRE )
    {
        x = size.x / 25.4;
        y = size.y / 25.4;
    }
    else if( units == CENTIMETRE )
    {
        x = size.x / 2.54;
        y = size.y / 2.54;
    }
    else
    {
        x = size.x;
        y = size.y;
    }

    new_size = wxSize( (int) round( x * (double) GetInternalUnits() ),
                       (int) round( y * (double) GetInternalUnits() ) );

    new_grid.m_Id = id;
    new_grid.m_Size = new_size;
    AddGrid( new_grid );
}

/*********************************/
wxSize BASE_SCREEN::GetGrid()
/*********************************/
{
    return m_Grid;
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
        nextitem = m_UndoList->Next();
        delete m_UndoList;
        m_UndoList = nextitem;
    }

    while( m_RedoList )
    {
        nextitem = m_RedoList->Next();
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
    EDA_BaseStruct* item;
    EDA_BaseStruct* nextitem;

    if( newitem == NULL )
        return;

    newitem->SetNext( m_UndoList );
    m_UndoList = newitem;

    /* Free first items, if count max reached */
    for( ii = 0, item = m_UndoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Next() == NULL )
            return;
        item = item->Next();
    }

    if( item == NULL )
        return;

    nextitem    = item->Next();
    item->SetNext( NULL ); // Set end of chain

    // Delete the extra  items
    for( item = nextitem; item != NULL; item = nextitem )
    {
        nextitem = item->Next();
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

    newitem->SetNext( m_RedoList );
    m_RedoList = newitem;
    /* Free first items, if count max reached */
    for( ii = 0, item = m_RedoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Next() == NULL )
            break;
        item = item->Next();
    }

    if( item == NULL )
        return;

    nextitem    = item->Next();
    item->SetNext( NULL );      // Set end of chain

    // Delete the extra items
    for( item = nextitem; item != NULL; item = nextitem )
    {
        nextitem = item->Next();
        delete item;
    }
}


/*****************************************************/
EDA_BaseStruct* BASE_SCREEN::GetItemFromUndoList()
/*****************************************************/
{
    EDA_BaseStruct* item = m_UndoList;

    if( item )
        m_UndoList = item->Next();
    return item;
}


/******************************************************/
EDA_BaseStruct* BASE_SCREEN::GetItemFromRedoList()
/******************************************************/
{
    EDA_BaseStruct* item = m_RedoList;

    if( item )
        m_RedoList = item->Next();
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
    SCH_ITEM* item = EEDrawList;

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

