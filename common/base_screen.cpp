/***************************************************************/
/* base_screen.cpp - fonctions des classes du type BASE_SCREEN */
/***************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"
#include "base_struct.h"
#include "sch_item_struct.h"
#include "class_base_screen.h"

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
    m_UndoList         = NULL;
    m_RedoList         = NULL;
    m_UndoRedoCountMax = 1;
    m_FirstRedraw      = TRUE;
    m_ScreenNumber     = 1;
    m_NumberOfScreen   = 1;  /* Hierarchy: Root: ScreenNumber = 1 */
    m_ZoomScalar       = 10;
    m_Zoom             = 32 * m_ZoomScalar;
    m_Grid             = wxSize( 50, 50 );   /* Default grid size */
    m_UserGridIsON     = FALSE;
    m_Center           = true;
    m_CurrentSheetDesc = &g_Sheet_A4;

    InitDatas();
}


/******************************/
BASE_SCREEN::~BASE_SCREEN()
/******************************/
{
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

    return wxSize( ( m_CurrentSheetDesc->m_Size.x * internal_units ) / 1000,
                   ( m_CurrentSheetDesc->m_Size.y * internal_units ) / 1000 );
}

/******************************************************************/
wxPoint BASE_SCREEN::CursorRealPosition( const wxPoint& ScreenPos )
/******************************************************************/
/** Function CursorRealPosition
 * @return the position in user units of location ScreenPos
 * @param ScreenPos = the screen (in pixel) position co convert
*/
{
    wxPoint curpos = ScreenPos;
    Unscale( curpos );

    curpos += m_DrawOrg;

    return curpos;
}

/** Function SetScalingFactor
 * calculates the .m_Zoom member to have a given scaling facort
 * @param the the current scale used to draw items on screen
 * draw coordinates are user coordinates * GetScalingFactor( )
*/
void  BASE_SCREEN::SetScalingFactor(double aScale )
{
    int zoom = static_cast<int>( ceil(aScale * m_ZoomScalar) );

    // Limit zoom to max and min allowed values:
    if (zoom < m_ZoomList[0])
        zoom = m_ZoomList[0];
    int idxmax = m_ZoomList.GetCount() - 1;
    if (zoom > m_ZoomList[idxmax])
        zoom = m_ZoomList[idxmax];

    SetZoom( zoom );
}

/**
 * Calculate coordinate value for zooming.
 *
 * Call this method when drawing on the device context.  It scales the
 * coordinate using the current zoom settings.  Zooming in Kicad occurs
 * by actually scaling the entire drawing using the zoom setting.
 *
 * FIXME: We should probably use wxCoord instead of int here but that would
 *        require using wxCoord in all of the other code that makes device
 *        context calls as well.
 */
int BASE_SCREEN::Scale( int coord )
{
#ifdef WX_ZOOM
    return coord;
#else
    if( !m_Zoom )
        return 0;

    if( !m_ZoomScalar || !m_Zoom )
        return 0;

    return wxRound( (double) ( coord * m_ZoomScalar ) / (double) m_Zoom );
#endif
}


void BASE_SCREEN::Scale( wxPoint& pt )
{
    pt.x = Scale( pt.x );
    pt.y = Scale( pt.y );
}


void BASE_SCREEN::Scale( wxSize& sz )
{
    sz.SetHeight( Scale( sz.GetHeight() ) );
    sz.SetWidth( Scale( sz.GetWidth() ) );
}


/**
 * Calculate the physical (unzoomed) location of a coordinate.
 *
 * Call this method when you want to find the unzoomed (physical) location
 * of a coordinate on the drawing.
 */
int BASE_SCREEN::Unscale( int coord )
{
#ifdef WX_ZOOM
    return coord;
#else
    if( !m_Zoom || !m_ZoomScalar )
        return 0;

    return wxRound( (double) ( coord * m_Zoom ) / (double) m_ZoomScalar );
#endif
}

void BASE_SCREEN::Unscale( wxPoint& pt )
{
    pt.x = Unscale( pt.x );
    pt.y = Unscale( pt.y );
}


void BASE_SCREEN::Unscale( wxSize& sz )
{
    sz.SetHeight( Unscale( sz.GetHeight() ) );
    sz.SetWidth( Unscale( sz.GetWidth() ) );
}


void BASE_SCREEN::SetZoomList( const wxArrayInt& zoomlist )
{
    if( !m_ZoomList.IsEmpty() )
        m_ZoomList.Empty();

    m_ZoomList = zoomlist;
}


void BASE_SCREEN::SetFirstZoom()
{
    if( m_ZoomList.IsEmpty() )
        m_Zoom = m_ZoomScalar;
    else
        m_Zoom = m_ZoomList[0];
}


int BASE_SCREEN::GetZoom() const
{
    return m_Zoom;
}


void BASE_SCREEN::SetZoom( int coeff )
{
    m_Zoom = coeff;

    if( m_Zoom < 1 )
        m_Zoom = 1;
}


void BASE_SCREEN::SetNextZoom()
{
    size_t i;

    if( m_ZoomList.IsEmpty() || m_Zoom >= m_ZoomList.Last() )
        return;

    for( i = 0; i < m_ZoomList.GetCount(); i++ )
    {
        if( m_Zoom < m_ZoomList[i] )
        {
            m_Zoom = m_ZoomList[i];
            break;
        }
    }
}


void BASE_SCREEN::SetPreviousZoom()
{
    size_t i;

    if( m_ZoomList.IsEmpty() || m_Zoom <= m_ZoomList[0] )
        return;

    for( i = m_ZoomList.GetCount(); i != 0; i-- )
    {
        if( m_Zoom > m_ZoomList[i - 1] )
        {
            m_Zoom = m_ZoomList[i - 1];
            break;
        }
    }
}


void BASE_SCREEN::SetLastZoom()
{
    if( m_ZoomList.IsEmpty() )
        return;

    m_Zoom = m_ZoomList.Last();
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

    wxSize nearest_grid = m_GridList[0].m_Size;
    for( i = 0; i < m_GridList.GetCount(); i++ )
    {
        if( m_GridList[i].m_Size == size )
        {
            m_Grid = m_GridList[i].m_Size;
            return;
        }
        
        // keep trace of the nearest grill size, if the exact size is not found
        if ( size.x < m_GridList[i].m_Size.x )
            nearest_grid = m_GridList[i].m_Size;
    }

    m_Grid = nearest_grid;

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

