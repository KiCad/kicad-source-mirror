/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_screen.h>
#include <id.h>
#include <base_units.h>
#include <trace_helpers.h>


wxString BASE_SCREEN::m_PageLayoutDescrFileName;   // the name of the page layout descr file.


BASE_SCREEN::BASE_SCREEN( KICAD_T aType ) :
    EDA_ITEM( aType )
{
    m_UndoRedoCountMax = DEFAULT_MAX_UNDO_ITEMS;
    m_Initialized      = false;
    m_ScreenNumber     = 1;
    m_NumberOfScreens  = 1;      // Hierarchy: Root: ScreenNumber = 1
    m_Zoom             = 32.0;
    m_Grid.m_Size      = wxRealPoint( 50, 50 );   // Default grid size
    m_Grid.m_CmdId     = ID_POPUP_GRID_LEVEL_50;
    m_Center           = true;
    m_IsPrinting       = false;

    m_FlagModified     = false;     // Set when any change is made on board.
    m_FlagSave         = false;     // Used in auto save set when an auto save is required.
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

    m_LocalOrigin = { 0, 0 };
}


bool BASE_SCREEN::SetZoom( double iu_per_du )
{
    if( iu_per_du == m_Zoom )
        return false;

    wxLogTrace( traceScreen, "Zoom:%.16g  1/Zoom:%.16g", iu_per_du, 1/iu_per_du );

    if( iu_per_du < GetMinAllowedZoom() )
        return false;

    if( iu_per_du > GetMaxAllowedZoom() )
        return false;

    m_Zoom = iu_per_du;

    return true;
}


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

        if( grid.m_CmdId == ID_POPUP_GRID_USER )
        {
            if( aMmFirst )
                msg.Printf( _( "User grid: %.4f mm (%.2f mils)" ),
                            gridValue_mm, gridValueMils );
            else
                msg.Printf( _( "User grid: %.2f mils (%.4f mm)" ),
                            gridValueMils, gridValue_mm );
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


int BASE_SCREEN::SetGrid( const wxRealPoint& size )
{
    wxASSERT( !m_grids.empty() );

    GRID_TYPE nearest_grid = m_grids[0];
    int gridIdx = 0;

    for( GRID_TYPE& grid : m_grids )
    {
        if( grid.m_Size == size )
        {
            m_Grid = grid;
            return grid.m_CmdId - ID_POPUP_GRID_LEVEL_1000;
        }

        // keep track of the nearest larger grid size, if the exact size is not found
        if ( size.x < grid.m_Size.x )
        {
            gridIdx = grid.m_CmdId - ID_POPUP_GRID_LEVEL_1000;
            nearest_grid = grid;
        }
    }

    m_Grid = nearest_grid;
    return gridIdx;
}


int BASE_SCREEN::SetGrid( int aCommandId  )
{
    wxASSERT( !m_grids.empty() );

    for( GRID_TYPE& grid : m_grids )
    {
        if( grid.m_CmdId == aCommandId )
        {
            m_Grid = grid;
            return grid.m_CmdId - ID_POPUP_GRID_LEVEL_1000;
        }
    }

    m_Grid = m_grids[0];
    return m_grids[0].m_CmdId - ID_POPUP_GRID_LEVEL_1000;
}


void BASE_SCREEN::AddGrid( const GRID_TYPE& aGrid )
{
    for( GRID_TYPE& existing : m_grids )
    {
        if( existing.m_Size == aGrid.m_Size && aGrid.m_CmdId != ID_POPUP_GRID_USER )
        {
            wxLogTrace( traceScreen, "Discarding duplicate grid size( %g, %g ).",
                        aGrid.m_Size.x, aGrid.m_Size.y );
            return;
        }

        if( existing.m_CmdId == aGrid.m_CmdId )
        {
            wxLogTrace( traceScreen, wxT( "Changing grid ID %d from size( %g, %g ) to " ) \
                        wxT( "size( %g, %g )." ),
                        aGrid.m_CmdId, existing.m_Size.x,
                        existing.m_Size.y, aGrid.m_Size.x, aGrid.m_Size.y );
            existing.m_Size = aGrid.m_Size;
            return;
        }
    }

    m_grids.push_back( aGrid );
}


void BASE_SCREEN::AddGrid( const wxRealPoint& size, EDA_UNITS_T aUnit, int id )
{
    wxRealPoint new_size;
    GRID_TYPE new_grid;

    new_size.x = From_User_Unit( aUnit, size.x );
    new_size.y = From_User_Unit( aUnit, size.y );
    new_grid.m_CmdId = id;
    new_grid.m_Size = new_size;

    AddGrid( new_grid );
}


GRID_TYPE& BASE_SCREEN::GetGrid( size_t aIndex )
{
    wxCHECK_MSG( !m_grids.empty() && aIndex < m_grids.size(), m_Grid,
                 wxT( "Cannot get grid object outside the bounds of the grid list." ) );

    return m_grids[ aIndex ];
}


bool BASE_SCREEN::GridExists( int aCommandId )
{
    // tests for grid command ID (not an index in grid list, but a wxID) exists in grid list.
    for( GRID_TYPE& grid : m_grids)
    {
        if( grid.m_CmdId == aCommandId )
            return true;
    }

    return false;
}


wxPoint BASE_SCREEN::getNearestGridPosition( const wxPoint& aPosition,
    const wxPoint& aGridOrigin ) const
{
    wxPoint     pt;
    wxRealPoint gridSize = GetGridSize();

    double  offset = fmod( aGridOrigin.x, gridSize.x );
    int x = KiROUND( (aPosition.x - offset) / gridSize.x );

    pt.x = KiROUND( x * gridSize.x + offset );

    offset = fmod( aGridOrigin.y, gridSize.y );

    int y = KiROUND( (aPosition.y - offset) / gridSize.y );
    pt.y = KiROUND ( y * gridSize.y + offset );

    return pt;
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
    if( m_UndoRedoCountMax > 0 )
    {
        int extraitems = GetUndoCommandCount() - m_UndoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( m_UndoList, extraitems );
    }
}


void BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_RedoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_UndoRedoCountMax > 0 )
    {
        int extraitems = GetRedoCommandCount() - m_UndoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( m_RedoList, extraitems );
    }
}


PICKED_ITEMS_LIST* BASE_SCREEN::PopCommandFromUndoList( )
{
    return m_UndoList.PopCommand();
}


PICKED_ITEMS_LIST* BASE_SCREEN::PopCommandFromRedoList( )
{
    return m_RedoList.PopCommand();
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
