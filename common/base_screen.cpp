/********************************************************/
/* base_screen.cpp - BASE_SCREEN object implementation. */
/********************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"
#include "base_struct.h"
#include "class_base_screen.h"
#include "id.h"


#define CURSOR_SIZE 12  /* size of the cross cursor. */


BASE_SCREEN::BASE_SCREEN( KICAD_T aType ) : EDA_ITEM( aType )
{
    m_drawList         = NULL;   /* Draw items list */
    m_UndoRedoCountMax = 10;     /* undo/Redo command Max depth, 10 is a
                                  * reasonable value */
    m_FirstRedraw      = TRUE;
    m_ScreenNumber     = 1;
    m_NumberOfScreen   = 1;  /* Hierarchy: Root: ScreenNumber = 1 */
    m_ZoomScalar       = 10;
    m_Zoom             = 32 * m_ZoomScalar;
    m_Grid.m_Size      = wxRealPoint( 50, 50 );   /* Default grid size */
    m_Grid.m_Id        = ID_POPUP_GRID_LEVEL_50;
    m_Center           = true;
    m_CurrentSheetDesc = &g_Sheet_A4;
    m_IsPrinting       = false;
    m_ScrollPixelsPerUnitX = 1;
    m_ScrollPixelsPerUnitY = 1;

    InitDatas();
}


BASE_SCREEN::~BASE_SCREEN()
{
}


void BASE_SCREEN::InitDatas()
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

    m_O_Curseur.x = m_O_Curseur.y = 0;

    SetCurItem( NULL );

    m_FlagRefreshReq = 0;   /* Redraw screen request flag */
    m_FlagModified   = 0;   // Set when any change is made on broad
    m_FlagSave = 1;         // Used in auto save: set when an auto save is made
}

/**
 * Get screen units scalar.
 *
 * Default implementation returns scalar used for schematic screen.  The
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
    wxSize size = m_CurrentSheetDesc->m_Size;
    size.x =  (int)( (double)size.x * internal_units / 1000 );
    size.y =  (int)( (double)size.y * internal_units / 1000 );

    return size;
}

void BASE_SCREEN::SetPageSize( wxSize& aPageSize )
{
    int internal_units = GetInternalUnits();

    m_CurrentSheetDesc->m_Size.x = (int) ((double)aPageSize.x * 1000 / internal_units);
    m_CurrentSheetDesc->m_Size.y = (int) ((double)aPageSize.y * 1000 / internal_units);
}


/**
 * Function SetScalingFactor
 * calculates the .m_Zoom member to have a given scaling factor
 * @param aScale - the the current scale used to draw items on screen
 * draw coordinates are user coordinates * GetScalingFactor()
 */
void BASE_SCREEN::SetScalingFactor(double aScale )
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

void BASE_SCREEN::SetZoomList( const wxArrayInt& zoomlist )
{
    if( !m_ZoomList.IsEmpty() )
        m_ZoomList.Empty();

    m_ZoomList = zoomlist;
}


bool BASE_SCREEN::SetFirstZoom()
{
    if( m_ZoomList.IsEmpty() )
    {
        if( m_Zoom != m_ZoomScalar )
        {
            m_Zoom = m_ZoomScalar;
            return true;
        }
    }
    else if( m_Zoom != m_ZoomList[0] )
    {
        m_Zoom = m_ZoomList[0];
        return true;
    }

    return false;
}


int BASE_SCREEN::GetZoom() const
{
    return m_Zoom;
}


bool BASE_SCREEN::SetZoom( int coeff )
{
    if( coeff == m_Zoom )
        return false;

    m_Zoom = coeff;

    if( m_Zoom < 1 )
        m_Zoom = 1;

    return true;
}


bool BASE_SCREEN::SetNextZoom()
{
    size_t i;

    if( m_ZoomList.IsEmpty() || m_Zoom >= m_ZoomList.Last() )
        return false;

    for( i = 0; i < m_ZoomList.GetCount(); i++ )
    {
        if( m_Zoom < m_ZoomList[i] )
        {
            m_Zoom = m_ZoomList[i];
            return true;
        }
    }

    return false;
}


bool BASE_SCREEN::SetPreviousZoom()
{
    size_t i;

    if( m_ZoomList.IsEmpty() || m_Zoom <= m_ZoomList[0] )
        return false;

    for( i = m_ZoomList.GetCount(); i != 0; i-- )
    {
        if( m_Zoom > m_ZoomList[i - 1] )
        {
            m_Zoom = m_ZoomList[i - 1];
            return true;
        }
    }

    return false;
}


bool BASE_SCREEN::SetLastZoom()
{
    if( m_ZoomList.IsEmpty() || m_Zoom == m_ZoomList.Last() )
        return false;

    m_Zoom = m_ZoomList.Last();
    return true;
}


void BASE_SCREEN::SetGridList( GRIDS& gridlist )
{
    if( !m_grids.empty() )
        m_grids.clear();

    m_grids = gridlist;
}


void BASE_SCREEN::GetGrids( GRIDS& aList )
{
    for( size_t i = 0;  i < m_grids.size();  i++ )
        aList.push_back( m_grids[ i ] );
}


void BASE_SCREEN::SetGrid( const wxRealPoint& size )
{
    wxASSERT( !m_grids.empty() );

    size_t i;

    GRID_TYPE nearest_grid = m_grids[0];

    for( i = 0; i < m_grids.size(); i++ )
    {
        if( m_grids[i].m_Size == size )
        {
            m_Grid = m_grids[i];
            return;
        }

        // keep trace of the nearest grill size, if the exact size is not found
        if ( size.x < m_grids[i].m_Size.x )
            nearest_grid = m_grids[i];
    }

    m_Grid = nearest_grid;

    wxLogWarning( wxT( "Grid size( %f, %f ) not in grid list, falling back " ) \
                  wxT( "to grid size( %f, %f )." ),
                  size.x, size.y, m_Grid.m_Size.x, m_Grid.m_Size.y );
}


/* Set grid size from command ID. */
void BASE_SCREEN::SetGrid( int id  )
{
    wxASSERT( !m_grids.empty() );

    size_t i;

    for( i = 0; i < m_grids.size(); i++ )
    {
        if( m_grids[i].m_Id == id )
        {
            m_Grid = m_grids[i];
            return;
        }
    }

    m_Grid = m_grids[0];

    wxLogWarning( wxT( "Grid ID %d not in grid list, falling back to " ) \
                  wxT( "grid size( %g, %g )." ), id, m_Grid.m_Size.x,
                  m_Grid.m_Size.y );
}


void BASE_SCREEN::AddGrid( const GRID_TYPE& grid )
{
    size_t i;

    for( i = 0; i < m_grids.size(); i++ )
    {
        if( m_grids[i].m_Size == grid.m_Size && grid.m_Id != ID_POPUP_GRID_USER )
        {
            wxLogDebug( wxT( "Discarding duplicate grid size( %g, %g )." ),
                        grid.m_Size.x, grid.m_Size.y );
            return;
        }

        if( m_grids[i].m_Id == grid.m_Id )
        {
            wxLogDebug( wxT( "Changing grid ID %d from size( %g, %g ) to " ) \
                        wxT( "size( %g, %g )." ),
                        grid.m_Id, m_grids[i].m_Size.x,
                        m_grids[i].m_Size.y, grid.m_Size.x, grid.m_Size.y );
            m_grids[i].m_Size = grid.m_Size;
            return;
        }
    }

    m_grids.push_back( grid );
}


void BASE_SCREEN::AddGrid( const wxRealPoint& size, int id )
{
    GRID_TYPE grid;

    grid.m_Size = size;
    grid.m_Id = id;
    AddGrid( grid );
}


void BASE_SCREEN::AddGrid( const wxRealPoint& size, UserUnitType aUnit, int id )
{
    double x, y;
    wxRealPoint new_size;
    GRID_TYPE new_grid;

    switch( aUnit )
    {
    case MILLIMETRES:
        x = size.x / 25.4;
        y = size.y / 25.4;
        break;

    default:
    case INCHES:
    case UNSCALED_UNITS:
        x = size.x;
        y = size.y;
        break;
    }

    new_size.x = x * GetInternalUnits();
    new_size.y = y * GetInternalUnits();

    new_grid.m_Id = id;
    new_grid.m_Size = new_size;
    AddGrid( new_grid );
}


GRID_TYPE& BASE_SCREEN::GetGrid( size_t aIndex )
{
    wxCHECK_MSG( !m_grids.empty() && aIndex < m_grids.size(), m_Grid,
                 wxT( "Cannot get grid object outside the bounds of the grid list." ) );

    return m_grids[ aIndex ];
}


GRID_TYPE BASE_SCREEN::GetGrid()
{
    return m_Grid;
}


const wxPoint& BASE_SCREEN::GetGridOrigin()
{
    return m_GridOrigin;
}


wxRealPoint BASE_SCREEN::GetGridSize()
{
    return m_Grid.m_Size;
}


int BASE_SCREEN::GetGridId()
{
    return m_Grid.m_Id;
}


/* free the undo and the redo lists
 */
void BASE_SCREEN::ClearUndoRedoList()
{
    ClearUndoORRedoList( m_UndoList );
    ClearUndoORRedoList( m_RedoList );
}


/* Put aNewitem in top of undo list
 *  Deletes old items if > count max.
 */
void BASE_SCREEN::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_UndoList.PushCommand( aNewitem );

    /* Delete the extra items, if count max reached */
    int extraitems = GetUndoCommandCount() - m_UndoRedoCountMax;

    if( extraitems > 0 ) // Delete the extra items
        ClearUndoORRedoList( m_UndoList, extraitems );
}


void BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_RedoList.PushCommand( aNewitem );

    /* Delete the extra items, if count max reached */
    int extraitems = GetRedoCommandCount() - m_UndoRedoCountMax;

    if( extraitems > 0 ) // Delete the extra items
        ClearUndoORRedoList( m_RedoList, extraitems );
}


PICKED_ITEMS_LIST* BASE_SCREEN::PopCommandFromUndoList( )
{
    return m_UndoList.PopCommand( );
}


PICKED_ITEMS_LIST* BASE_SCREEN::PopCommandFromRedoList( )
{
    return m_RedoList.PopCommand( );
}


void BASE_SCREEN::AddItem( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Attempt to add NULL item pointer to " ) + GetClass() +
                 wxT( "item list" ) );
    m_items.push_back( aItem );
}


void BASE_SCREEN::InsertItem( EDA_ITEMS::iterator aIter, EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Attempt to insert NULL item pointer to " ) + GetClass() +
                 wxT( "item list" ) );
    m_items.insert( aIter, aItem );
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
    EDA_ITEM* item = m_drawList;

    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    for(  ; item;  item = item->Next() )
    {
        item->Show( nestLevel+1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif
