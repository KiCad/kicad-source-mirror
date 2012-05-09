/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_base_screen.h
 * @brief BASE_SCREEN class implementation.
 */

#ifndef  CLASS_BASE_SCREEN_H_
#define  CLASS_BASE_SCREEN_H_

#include <base_struct.h>
#include <class_undoredo_container.h>
#include <block_commande.h>
#include <common.h>


/**
 * Class GRID_TYPE
 * is for grid arrays.
 */
class GRID_TYPE
{
public:
    int         m_Id;
    wxRealPoint m_Size;

    GRID_TYPE& operator=( const GRID_TYPE& item )
    {
        if( this != &item )
        {
            m_Id   = item.m_Id;
            m_Size = item.m_Size;
        }

        return *this;
    }

    const bool operator==( const GRID_TYPE& item ) const
    {
        return m_Size == item.m_Size && m_Id == item.m_Id;
    }
};


typedef std::vector< GRID_TYPE > GRIDS;


/**
 * Class BASE_SCREEN
 * handles how to draw a screen (a board, a schematic ...)
 */
class BASE_SCREEN : public EDA_ITEM
{
    GRIDS       m_grids;          ///< List of valid grid sizes.
    wxString    m_fileName;       ///< File used to load the screen.
    char        m_FlagRefreshReq; ///< Indicates that the screen should be redrawn.
    bool        m_FlagModified;   ///< Indicates current drawing has been modified.
    bool        m_FlagSave;       ///< Indicates automatic file save.
    EDA_ITEM*   m_CurrentItem;    ///< Currently selected object
    GRID_TYPE   m_Grid;           ///< Current grid selection.
    wxPoint     m_scrollCenter;   ///< Current scroll center point in logical units.
    wxPoint     m_MousePosition;  ///< Mouse cursor coordinate in logical units.


    /**
     * The cross hair position in logical (drawing) units.  The cross hair is not the cursor
     * position.  It is an addition indicator typically drawn on grid to indicate to the
     * user where the current action will be performed.
     */
    wxPoint m_crossHairPosition;

    double     m_Zoom;          ///< Current zoom coefficient.


public:
    wxPoint m_DrawOrg;          ///< offsets for drawing the circuit on the screen

    wxPoint m_O_Curseur;        /* Relative Screen cursor coordinate (on grid)
                                 * in user units. (coordinates from last reset position)*/

    // Scrollbars management:
    int     m_ScrollPixelsPerUnitX; ///< Pixels per scroll unit in the horizontal direction.
    int     m_ScrollPixelsPerUnitY; ///< Pixels per scroll unit in the vertical direction.

    wxSize  m_ScrollbarNumber;      /* Current virtual draw area size in scroll units.
                                     * m_ScrollbarNumber * m_ScrollPixelsPerUnit =
                                     * virtual draw area size in pixels */

    wxPoint m_ScrollbarPos;     ///< Current scroll bar position in scroll units.

    wxPoint m_StartVisu;        /* Coordinates in drawing units of the current
                                 * view position (upper left corner of device)
                                 */

    bool   m_Center;             /* Center on screen.  If true (0.0) is centered
                                  * on screen coordinates can be < 0 and
                                  * > 0 except for schematics.
                                  * false: when coordinates can only be >= 0
                                  * Schematic */
    bool    m_FirstRedraw;

    // Undo/redo list of commands
    UNDO_REDO_CONTAINER m_UndoList;         ///< Objects list for the undo command (old data)
    UNDO_REDO_CONTAINER m_RedoList;         ///< Objects list for the redo command (old data)
    unsigned            m_UndoRedoCountMax; ///< undo/Redo command Max depth

    // block control
    BLOCK_SELECTOR      m_BlockLocate;      ///< Block description for block commands

    int                 m_ScreenNumber;
    int                 m_NumberOfScreen;

    wxPoint	            m_GridOrigin;

    std::vector<double> m_ZoomList;         ///< standard zoom (i.e. scale) coefficients.
    bool                m_IsPrinting;

public:
    BASE_SCREEN( KICAD_T aType = SCREEN_T );
    ~BASE_SCREEN();

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from EDA_ITEM
     */
    void SetCurItem( EDA_ITEM* aItem ) { m_CurrentItem = aItem; }

    EDA_ITEM* GetCurItem() const { return m_CurrentItem; }

    void InitDataPoints( const wxSize& aPageSizeInternalUnits );

    void SetFileName( const wxString& aFileName ) { m_fileName = aFileName; }

    wxString GetFileName() const { return m_fileName; }

    /**
     * Function MilsToIuScalar
     * returns the scalar required to convert mils to internal units.
     *
     * @note This is a temporary hack until the derived objects SCH_SCREEN and PCB_SCREEN
     *       no longer need to be derived from BASE_SCREEN.  I does allow removal of the
     *       obsolete GetInternalUnits function.
     */
    virtual int MilsToIuScalar() { return 1; }

    /**
     * Function GetCrossHairPosition
     * return the current cross hair position in logical (drawing) coordinates.
     * @param aInvertY Inverts the Y axis position.
     * @return The cross hair position in drawing coordinates.
     */
    wxPoint GetCrossHairPosition( bool aInvertY = false ) const
    {
        if( aInvertY )
            return wxPoint( m_crossHairPosition.x, -m_crossHairPosition.y );

        return wxPoint( m_crossHairPosition.x, m_crossHairPosition.y );
    }

    /**
     * Function SetCrossHairPosition
     * sets the screen cross hair position to \a aPosition in logical (drawing) units.
     * @param aPosition The new cross hair position.
     * @param aSnapToGrid Sets the cross hair position to the nearest grid position to
     *                    \a aPosition.
     *
     */
    void SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid = true );

    /* general Undo/Redo command control */

    /**
     * Function ClearUndoORRedoList (virtual).
     * this function must remove the aItemCount old commands from aList
     * and delete commands, pickers and picked items if needed
     * Because picked items must be deleted only if they are not in use, this
     * is a virtual pure function that must be created for SCH_SCREEN and
     * PCB_SCREEN
     * @param aList = the UNDO_REDO_CONTAINER of commands
     * @param aItemCount = number of old commands to delete. -1 to remove all
     *                     old commands this will empty the list of commands.
     *  Commands are deleted from the older to the last.
     */
    virtual void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 ) = 0;

    /**
     * Function ClearUndoRedoList
     * clear undo and redo list, using ClearUndoORRedoList()
     * picked items are deleted by ClearUndoORRedoList() according to their
     * status
     */
    virtual void ClearUndoRedoList();

    /**
     * Function PushCommandToUndoList
     * add a command to undo in undo list
     * delete the very old commands when the max count of undo commands is
     * reached
     * ( using ClearUndoORRedoList)
     */
    virtual void PushCommandToUndoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Function PushCommandToRedoList
     * add a command to redo in redo list
     * delete the very old commands when the max count of redo commands is
     * reached
     * ( using ClearUndoORRedoList)
     */
    virtual void PushCommandToRedoList( PICKED_ITEMS_LIST* aItem );

    /** PopCommandFromUndoList
     * return the last command to undo and remove it from list
     * nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromUndoList();

    /** PopCommandFromRedoList
     * return the last command to undo and remove it from list
     * nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromRedoList();

    int GetUndoCommandCount()
    {
        return m_UndoList.m_CommandsList.size();
    }


    int GetRedoCommandCount()
    {
        return m_RedoList.m_CommandsList.size();
    }


    void SetModify() { m_FlagModified = true; }
    void ClrModify() { m_FlagModified = false;; }
    void SetSave() { m_FlagSave = true; }
    void ClrSave() { m_FlagSave = false; }
    int IsModify() { return m_FlagModified;  }
    int IsSave() { return m_FlagSave;  }


    //----<zoom stuff>---------------------------------------------------------

    /**
     * Function GetZoom
     * returns the current "zoom factor", which is a measure of
     * "internal units per device unit", or "world units per device unit".
     * A device unit is typically a pixel.
     */
    double GetZoom() const      { return m_Zoom; }

    /**
     * Function SetZoom
     * adjusts the current zoom factor.
     *
     * @param iu_per_du is the number of internal units (world units) per
     *   device units (pixels typically).
     */
    bool SetZoom( double iu_per_du );

    bool SetNextZoom();
    bool SetPreviousZoom();
    bool SetFirstZoom();
    bool SetLastZoom();

    /**
     * Function GetMaxAllowedZoom
     * returns the maximum allowed zoom factor, which was established as the last entry
     * in m_ZoomList.
     */
    double GetMaxAllowedZoom() const    { return m_ZoomList.size() ? *m_ZoomList.rbegin() : 1.0; }

    /**
     * Function GetMinAllowedZoom
     * returns the minimum allowed zoom factor, which was established as the first entry
     * in m_ZoomList.
     */
    double GetMinAllowedZoom() const    { return m_ZoomList.size() ? *m_ZoomList.begin() : 1.0; }

    /**
     * Function SetScalingFactor
     * sets the scaling factor of "internal unit per device unit".
     * If the output device is a screen, then "device units" are pixels.  The
     * "logical unit" is wx terminology, and corresponds to KiCad's "Internal Unit (IU)".
     * <p>
     * This scaling factor is "internal units per device unit".  This function is
     * the same thing currently as SetZoom(), but clamps the argument within a
     * legal range.

     * @param iu_per_du is the current scale used to draw items onto the device
     *   context wxDC.
     */
    void SetScalingFactor( double iu_per_du );

    /**
     * Function GetScalingFactor
     * returns the inverse of the current scale used to draw items on screen.
     * <p>
     * This function somehow got designed to be the inverse of SetScalingFactor().
     * <p>
     * device coordinates = user coordinates * GetScalingFactor()
     */
    double GetScalingFactor() const;


    //----<grid stuff>----------------------------------------------------------

    /**
     * Return the command ID of the currently selected grid.
     *
     * @return int - Currently selected grid command ID.
     */
    int GetGridId();

    /**
     * Return the grid size of the currently selected grid.
     *
     * @return wxRealPoint - The currently selected grid size.
     */
    wxRealPoint GetGridSize();

    /**
     * Return the grid object of the currently selected grid.
     *
     * @return GRID_TYPE - The currently selected grid.
     */
    GRID_TYPE GetGrid();

    const wxPoint& GetGridOrigin();
    void SetGrid( const wxRealPoint& size );

    /**
     * Function SetGrid
     * sets the grid size from command ID.
     */
    void SetGrid( int id );
    void SetGridList( GRIDS& sizelist );
    void AddGrid( const GRID_TYPE& grid );
    void AddGrid( const wxRealPoint& size, int id );
    void AddGrid( const wxRealPoint& size, EDA_UNITS_T aUnit, int id );

    /**
     * Function GetGridCount().
     * Return the size of the grid list.
     *
     * @returns - The size of the grid list.
     */
    size_t GetGridCount() const { return m_grids.size(); }

    /**
     * Function GetGrid()
     * Returns the grid object at \a aIndex.
     *
     * @param aIndex - The grid list index.
     * @return - The grid object at \a aIndex or the current grid if the grid list is empty.
     */
    GRID_TYPE& GetGrid( size_t aIndex );

    /**
     * Function GetGrids().
     * Copy the grid list to \a aList.
     *
     * @param aList - List to copy to.
     */
    void GetGrids( GRIDS& aList );

    void SetMousePosition( const wxPoint& aPosition ) { m_MousePosition = aPosition; }

    /**
     * Function RefPos
     * Return the reference position, coming from either the mouse position
     * or the cursor position.
     *
     * @param useMouse If true, return mouse position, else cursor's.
     *
     * @return wxPoint - The reference point, either the mouse position or
     *                   the cursor position.
     */
    wxPoint RefPos( bool useMouse )
    {
        return useMouse ? m_MousePosition : m_crossHairPosition;
    }

    /**
     * Function GetCursorPosition
     * returns the current cursor position in logical (drawing) units.
     * @param aOnGrid Returns the nearest grid position at the current cursor position.
     * @param aGridSize Custom grid size instead of the current grid size.  Only valid
     *        if \a aOnGrid is true.
     * @return The current cursor position.
     */
    wxPoint GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize = NULL );

    /**
     * Function GetCursorScreenPosition
     * returns the cross hair position in device (display) units.b
     * @return The current cross hair position.
     */
    wxPoint GetCrossHairScreenPosition() const;

    /**
     * Function GetNearestGridPosition
     * returns the nearest \a aGridSize location to \a aPosition.
     * @param aPosition The position to check.
     * @param aGridSize The grid size to locate to if provided.  If NULL then the current
     *                  grid size is used.
     * @return The nearst grid position.
     */
    wxPoint GetNearestGridPosition( const wxPoint& aPosition, wxRealPoint* aGridSize = NULL );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "BASE_SCREEN" );
    }

    inline bool IsBlockActive() const { return !m_BlockLocate.IsIdle(); }

    void ClearBlockCommand() { m_BlockLocate.Clear(); }

    const wxPoint& GetScrollCenterPosition() const { return m_scrollCenter; }
    void SetScrollCenterPosition( const wxPoint& aCenterPosition )
    {
        m_scrollCenter = aCenterPosition;
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
#endif
};

#endif  // CLASS_BASE_SCREEN_H_
