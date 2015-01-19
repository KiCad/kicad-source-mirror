/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file base_screen.cpp
 * @brief BASE_SCREEN object implementation.
 */

#include <fctsys.h>
#include <macros.h>
#include <common.h>
#include <base_struct.h>
#include <class_base_screen.h>
#include <id.h>
#include <base_units.h>

wxString BASE_SCREEN::m_PageLayoutDescrFileName;   // the name of the page layout descr file.

BASE_SCREEN::BASE_SCREEN( KICAD_T aType ) :
    EDA_ITEM( aType )
{
    m_UndoRedoCountMax = 10;     // undo/Redo command Max depth, 10 is a reasonable value
    m_FirstRedraw      = true;
    m_ScreenNumber     = 1;
    m_NumberOfScreens  = 1;      // Hierarchy: Root: ScreenNumber = 1
    m_Zoom             = 32.0;
    m_Grid.m_Size      = wxRealPoint( 50, 50 );   // Default grid size
    m_Grid.m_Id        = ID_POPUP_GRID_LEVEL_50;
    m_Center           = true;
    m_IsPrinting       = false;
    m_ScrollPixelsPerUnitX = 1;
    m_ScrollPixelsPerUnitY = 1;

    m_FlagModified     = false;     // Set when any change is made on board.
    m_FlagSave         = false;     // Used in auto save set when an auto save is required.

    SetCurItem( NULL );
}


BASE_SCREEN::~BASE_SCREEN()
{
}


void BASE_SCREEN::InitDataPoints( const wxSize& aPageSizeIU )
{
    if( m_Center )
    {
        m_crossHairPosition.x = 0;
        m_crossHairPosition.y = 0;

        m_DrawOrg.x = -aPageSizeIU.x / 2;
        m_DrawOrg.y = -aPageSizeIU.y / 2;
    }
    else
    {
        m_crossHairPosition.x = aPageSizeIU.x / 2;
        m_crossHairPosition.y = aPageSizeIU.y / 2;

        m_DrawOrg.x = 0;
        m_DrawOrg.y = 0;
    }

    m_O_Curseur.x = m_O_Curseur.y = 0;
}


double BASE_SCREEN::GetScalingFactor() const
{
    double scale = 1.0 / GetZoom();
    return scale;
}


void BASE_SCREEN::SetScalingFactor( double aScale )
{
    // Limit zoom to max and min allowed values:
    double zoom = Clamp( GetMinAllowedZoom(), aScale, GetMaxAllowedZoom() );

    SetZoom( zoom );
}


bool BASE_SCREEN::SetFirstZoom()
{
    return SetZoom( GetMinAllowedZoom() );
}


bool BASE_SCREEN::SetLastZoom()
{
    return SetZoom( GetMaxAllowedZoom() );
}


bool BASE_SCREEN::SetZoom( double iu_per_du )
{
    if( iu_per_du == m_Zoom )
        return false;

    //wxLogDebug( "Zoom:%.16g  1/Zoom:%.16g", iu_per_du, 1/iu_per_du );

    if( iu_per_du < GetMinAllowedZoom() )
        return false;

    if( iu_per_du > GetMaxAllowedZoom() )
        return false;

    m_Zoom = iu_per_du;

    return true;
}


bool BASE_SCREEN::SetNextZoom()
{
    for( unsigned i=0; i < m_ZoomList.size();  ++i )
    {
        if( m_Zoom < m_ZoomList[i] )
        {
            SetZoom( m_ZoomList[i] );
            return true;
        }
    }

    return false;
}


bool BASE_SCREEN::SetPreviousZoom()
{
    for( unsigned i = m_ZoomList.size(); i != 0;  --i )
    {
        if( m_Zoom > m_ZoomList[i - 1] )
        {
            SetZoom( m_ZoomList[i - 1] );
            return true;
        }
    }

    return false;
}

/* Build the list of human readable grid list.
 * The list shows the grid size both in mils or mm.
 * aMmFirst = true to have mm first and mils after
 *            false to have mils first and mm after
 */
int BASE_SCREEN::BuildGridsChoiceList( wxArrayString& aGridsList, bool aMmFirst) const
{
    wxString msg;
    wxRealPoint curr_grid_size = GetGridSize();
    int idx = -1;
    int idx_usergrid = -1;

    for( size_t i = 0; i < GetGridCount(); i++ )
    {
        const GRID_TYPE& grid = m_grids[i];
        double gridValueMils = To_User_Unit( INCHES, grid.m_Size.x ) * 1000;
        double gridValue_mm = To_User_Unit( MILLIMETRES, grid.m_Size.x );

        if( grid.m_Id == ID_POPUP_GRID_USER )
        {
            msg = _( "User Grid" );
            idx_usergrid = i;
        }
        else
        {
            if( aMmFirst )
                msg.Printf( _( "Grid: %.4f mm (%.2f mils)" ),
                            gridValue_mm, gridValueMils );
            else
                msg.Printf( _( "Grid: %.2f mils (%.4f mm)" ),
                            gridValueMils, gridValue_mm );
        }

        aGridsList.Add( msg );

        if( curr_grid_size == grid.m_Size )
            idx = i;
    }

    if( idx < 0 )
        idx = idx_usergrid;

    return idx;
}


void BASE_SCREEN::SetGridList( GRIDS& gridlist )
{
    if( !m_grids.empty() )
        m_grids.clear();

    m_grids = gridlist;
}


int BASE_SCREEN::SetGrid( const wxRealPoint& size )
{
    wxASSERT( !m_grids.empty() );

    GRID_TYPE nearest_grid = m_grids[0];
    int gridIdx = 0;

    for( unsigned i = 0; i < m_grids.size(); i++ )
    {
        if( m_grids[i].m_Size == size )
        {
            m_Grid = m_grids[i];
            return m_grids[i].m_Id - ID_POPUP_GRID_LEVEL_1000;
        }

        // keep track of the nearest larger grid size, if the exact size is not found
        if ( size.x < m_grids[i].m_Size.x )
        {
            gridIdx = m_grids[i].m_Id - ID_POPUP_GRID_LEVEL_1000;
            nearest_grid = m_grids[i];
        }
    }

    m_Grid = nearest_grid;

    wxLogWarning( wxT( "Grid size( %f, %f ) not in grid list, falling back " ) \
                  wxT( "to grid size( %f, %f )." ),
                  size.x, size.y, m_Grid.m_Size.x, m_Grid.m_Size.y );

    return gridIdx;
}


int BASE_SCREEN::SetGrid( int aCommandId  )
{
    wxASSERT( !m_grids.empty() );

    for( unsigned i = 0; i < m_grids.size(); i++ )
    {
        if( m_grids[i].m_Id == aCommandId )
        {
            m_Grid = m_grids[i];
            return m_grids[i].m_Id - ID_POPUP_GRID_LEVEL_1000;
        }
    }

    m_Grid = m_grids[0];

    wxLogWarning( wxT( "Grid ID %d not in grid list, falling back to " ) \
                  wxT( "grid size( %g, %g )." ), aCommandId,
                  m_Grid.m_Size.x, m_Grid.m_Size.y );

    return m_grids[0].m_Id - ID_POPUP_GRID_LEVEL_1000;
}


void BASE_SCREEN::AddGrid( const GRID_TYPE& grid )
{
    for( unsigned i = 0; i < m_grids.size(); i++ )
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


void BASE_SCREEN::AddGrid( const wxRealPoint& size, EDA_UNITS_T aUnit, int id )
{
    wxRealPoint new_size;
    GRID_TYPE new_grid;

    new_size.x = From_User_Unit( aUnit, size.x );
    new_size.y = From_User_Unit( aUnit, size.y );
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


wxPoint BASE_SCREEN::getNearestGridPosition( const wxPoint& aPosition,
    const wxPoint& aGridOrigin, wxRealPoint* aGridSize ) const
{
    wxPoint     pt;
    wxRealPoint gridSize;

    if( aGridSize )
        gridSize = *aGridSize;
    else
        gridSize = GetGridSize();

    double  offset = fmod( aGridOrigin.x, gridSize.x );
    int x = KiROUND( (aPosition.x - offset) / gridSize.x );

    pt.x = KiROUND( x * gridSize.x + offset );

    offset = fmod( aGridOrigin.y, gridSize.y );

    int y = KiROUND( (aPosition.y - offset) / gridSize.y );
    pt.y = KiROUND ( y * gridSize.y + offset );

    return pt;
}


wxPoint BASE_SCREEN::getCursorPosition( bool aOnGrid, const wxPoint& aGridOrigin, wxRealPoint* aGridSize ) const
{
    if( aOnGrid )
        return getNearestGridPosition( m_crossHairPosition, aGridOrigin, aGridSize );

    return m_crossHairPosition;
}


wxPoint BASE_SCREEN::getCrossHairScreenPosition() const
{
    wxPoint pos = m_crossHairPosition - m_DrawOrg;
    double  scalar = GetScalingFactor();

    pos.x = KiROUND( (double) pos.x * scalar );
    pos.y = KiROUND( (double) pos.y * scalar );

    return pos;
}


void BASE_SCREEN::setCrossHairPosition( const wxPoint& aPosition, const wxPoint& aGridOrigin, bool aSnapToGrid )
{
    if( aSnapToGrid )
        m_crossHairPosition = getNearestGridPosition( aPosition, aGridOrigin, NULL );
    else
        m_crossHairPosition = aPosition;
}


void BASE_SCREEN::ClearUndoRedoList()
{
    ClearUndoORRedoList( m_UndoList );
    ClearUndoORRedoList( m_RedoList );
}


void BASE_SCREEN::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_UndoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    int extraitems = GetUndoCommandCount() - m_UndoRedoCountMax;

    if( extraitems > 0 ) // Delete the extra items
        ClearUndoORRedoList( m_UndoList, extraitems );
}


void BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_RedoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
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


#if defined(DEBUG)

void BASE_SCREEN::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    /* this class will eventually go away, but here's a place holder until then.
    for( EDA_ITEM* item = m_drawList;  item;  item = item->Next() )
    {
        item->Show( nestLevel+1, os );
    }
    */

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
