/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <id.h>


/**
 * Class GRID_TYPE
 * is for grid arrays.
 */
class GRID_TYPE
{
public:
    int         m_CmdId;    // The command id of this grid ( first id is ID_POPUP_GRID_LEVEL_1000 )
    wxRealPoint m_Size;     // the size in internal unit of the grid (can differ for X and Y axis)

    GRID_TYPE& operator=( const GRID_TYPE& item )
    {
        if( this != &item )
        {
            m_CmdId   = item.m_CmdId;
            m_Size = item.m_Size;
        }

        return *this;
    }

    const bool operator==( const GRID_TYPE& item ) const
    {
        return m_Size == item.m_Size && m_CmdId == item.m_CmdId;
    }
};


typedef std::vector<GRID_TYPE> GRIDS;


/**
 * Class BASE_SCREEN
 * handles how to draw a screen (a board, a schematic ...)
 */
class BASE_SCREEN : public EDA_ITEM
{
private:
    GRIDS       m_grids;            ///< List of valid grid sizes.
    bool        m_FlagModified;     ///< Indicates current drawing has been modified.
    bool        m_FlagSave;         ///< Indicates automatic file save.
    EDA_ITEM*   m_CurrentItem;      ///< Currently selected object
    GRID_TYPE   m_Grid;             ///< Current grid selection.
    wxPoint     m_scrollCenter;     ///< Current scroll center point in logical units.
    wxPoint     m_MousePosition;    ///< Mouse cursor coordinate in logical units.

    /**
     * The cross hair position in logical (drawing) units.  The cross hair is not the cursor
     * position.  It is an addition indicator typically drawn on grid to indicate to the
     * user where the current action will be performed.
     */
    wxPoint     m_crossHairPosition;

    double      m_Zoom;             ///< Current zoom coefficient.

    //----< Old public API now is private, and migratory>------------------------
    // called only from EDA_DRAW_FRAME
    friend class EDA_DRAW_FRAME;

    /**
     * Function getCrossHairPosition
     * return the current cross hair position in logical (drawing) coordinates.
     * @param aInvertY Inverts the Y axis position.
     * @return The cross hair position in drawing coordinates.
     */
    wxPoint getCrossHairPosition( bool aInvertY ) const
    {
        if( aInvertY )
            return wxPoint( m_crossHairPosition.x, -m_crossHairPosition.y );

        return wxPoint( m_crossHairPosition.x, m_crossHairPosition.y );
    }

    /**
     * Function setCrossHairPosition
     * sets the screen cross hair position to \a aPosition in logical (drawing) units.
     * @param aPosition The new cross hair position.
     * @param aGridOrigin Origin point of the snap grid.
     * @param aSnapToGrid Sets the cross hair position to the nearest grid position to
     *                    \a aPosition.
     *
     */
    void setCrossHairPosition( const wxPoint& aPosition, const wxPoint& aGridOrigin, bool aSnapToGrid );

    /**
     * Function getCursorScreenPosition
     * returns the cross hair position in device (display) units.b
     * @return The current cross hair position.
     */
    wxPoint getCrossHairScreenPosition() const;

    /**
     * Function getNearestGridPosition
     * returns the nearest \a aGridSize location to \a aPosition.
     * @param aPosition The position to check.
     * @param aGridOrigin The origin point of the snap grid.
     * @param aGridSize The grid size to locate to if provided.  If NULL then the current
     *                  grid size is used.
     * @return The nearst grid position.
     */
    wxPoint getNearestGridPosition( const wxPoint& aPosition, const wxPoint& aGridOrigin,
                                    wxRealPoint* aGridSize ) const;

    /**
     * Function getCursorPosition
     * returns the current cursor position in logical (drawing) units.
     * @param aOnGrid Returns the nearest grid position at the current cursor position.
     * @param aGridOrigin Origin point of the snap grid.
     * @param aGridSize Custom grid size instead of the current grid size.  Only valid
     *        if \a aOnGrid is true.
     * @return The current cursor position.
     */
    wxPoint getCursorPosition( bool aOnGrid, const wxPoint& aGridOrigin, wxRealPoint* aGridSize ) const;

    void setMousePosition( const wxPoint& aPosition ) { m_MousePosition = aPosition; }

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
    wxPoint refPos( bool useMouse ) const
    {
        return useMouse ? m_MousePosition : m_crossHairPosition;
    }

    const wxPoint& getScrollCenterPosition() const          { return m_scrollCenter; }
    void setScrollCenterPosition( const wxPoint& aPoint )   { m_scrollCenter = aPoint; }

    //----</Old public API now is private, and migratory>------------------------


public:
    static  wxString m_PageLayoutDescrFileName; ///< the name of the page layout descr file,
                                                ///< or emty to used the default pagelayout

    wxPoint     m_DrawOrg;          ///< offsets for drawing the circuit on the screen

    wxPoint     m_O_Curseur;        ///< Relative Screen cursor coordinate (on grid)
                                    ///< in user units. (coordinates from last reset position)

    // Scrollbars management:
    int         m_ScrollPixelsPerUnitX; ///< Pixels per scroll unit in the horizontal direction.
    int         m_ScrollPixelsPerUnitY; ///< Pixels per scroll unit in the vertical direction.

    wxSize      m_ScrollbarNumber;  /**< Current virtual draw area size in scroll units.
                                     * m_ScrollbarNumber * m_ScrollPixelsPerUnit =
                                     * virtual draw area size in pixels */

    wxPoint     m_ScrollbarPos;     ///< Current scroll bar position in scroll units.

    wxPoint     m_StartVisu;        /**< Coordinates in drawing units of the current
                                     * view position (upper left corner of device)
                                     */

    bool        m_Center;           /**< Center on screen.  If true (0.0) is centered
                                     * on screen coordinates can be < 0 and
                                     * > 0 except for schematics.
                                     * false: when coordinates can only be >= 0
                                     * Schematic */
    bool        m_FirstRedraw;

    // Undo/redo list of commands
    UNDO_REDO_CONTAINER m_UndoList;         ///< Objects list for the undo command (old data)
    UNDO_REDO_CONTAINER m_RedoList;         ///< Objects list for the redo command (old data)
    unsigned            m_UndoRedoCountMax; ///< undo/Redo command Max depth

    // block control
    BLOCK_SELECTOR      m_BlockLocate;      ///< Block description for block commands

    int                 m_ScreenNumber;
    int                 m_NumberOfScreens;

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

    /**
     * Function MilsToIuScalar
     * returns the scalar required to convert mils to internal units.
     *
     * @note This is a temporary hack until the derived objects SCH_SCREEN and PCB_SCREEN
     *       no longer need to be derived from BASE_SCREEN.  I does allow removal of the
     *       obsolete GetInternalUnits function.
     */
    virtual int MilsToIuScalar() { return 1; }

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

    int GetUndoCommandCount() const
    {
        return m_UndoList.m_CommandsList.size();
    }

    int GetRedoCommandCount() const
    {
        return m_RedoList.m_CommandsList.size();
    }

    void SetModify()        { m_FlagModified = true; }
    void ClrModify()        { m_FlagModified = false; }
    void SetSave()          { m_FlagSave = true; }
    void ClrSave()          { m_FlagSave = false; }
    bool IsModify() const   { return m_FlagModified; }
    bool IsSave() const     { return m_FlagSave; }


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
    int GetGridCmdId() const { return m_Grid.m_CmdId; }

    /**
     * Return the grid size of the currently selected grid.
     *
     * @return wxRealPoint - The currently selected grid size.
     */
    const wxRealPoint& GetGridSize() const { return m_Grid.m_Size; }

    /**
     * Return the grid object of the currently selected grid.
     *
     * @return GRID_TYPE - The currently selected grid.
     */
    const GRID_TYPE& GetGrid() const { return m_Grid; }

    /**
     * set the current grid size m_Grid.
     * The size must be existing in grid list (in m_grids)
     * If not, the near existing grid size is used
     * @param size = the size of the new grid
     * @return the grid id offset (id from ID_POPUP_GRID_LEVEL_1000 )
     * of the currently selected grid.
     */
    int SetGrid( const wxRealPoint& size );

    /**
     * Function SetGrid
     * sets the grid size from command ID (not an index in grid list, but a wxID).
     * @param aCommandId = the wxWidgets command ID
     * @return the grid id offset (id from ID_POPUP_GRID_LEVEL_1000 )
     * of the currently selected grid.
     */
    int SetGrid( int aCommandId );

    void SetGridList( GRIDS& sizelist );
    void AddGrid( const GRID_TYPE& grid );
    void AddGrid( const wxRealPoint& size, int id );
    void AddGrid( const wxRealPoint& size, EDA_UNITS_T aUnit, int id );

    /**
     * Function GridExists
     * tests for grid command ID (not an index in grid list, but a wxID) exists in grid list.
     * @param aCommandId = the wxWidgets command ID
     * @return true if the grid exists in grid list.
     */
    bool GridExists( int aCommandId );

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
     * Returns the current list of grids.
     */
    const GRIDS& GetGrids() const
    {
        return m_grids;
    }

    /**
     * Function BuildGridsChoiceList().
     * Build the human readable list of grid list, for menus or combo boxes
     * the list shows the grid size both in mils or mm.
     * @param aGridsList = a wxArrayString to populate
     * @param aMmFirst = true to have mm first and mils after
     *                   false to have mils first and mm after
     * @return the index of the curr grid in list, if found or -1
     */
    int BuildGridsChoiceList( wxArrayString& aGridsList, bool aMmFirst) const;


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

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
#endif
};

#endif  // CLASS_BASE_SCREEN_H_
