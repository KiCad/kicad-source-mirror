/**********************
* class_base_screen.h
**********************/

/* define :
 *  class BASE_SCREEN to handle how to draw a screen (a board, a schematic ...)
 */

#ifndef  __CLASS_BASE_SCREEN_H__
#define  __CLASS_BASE_SCREEN_H__

#include "base_struct.h"
#include "class_undoredo_container.h"
#include "block_commande.h"
#include "common.h"

#include <boost/ptr_container/ptr_vector.hpp>


// Forward declarations:
class Ki_PageDescr;


/**
 * Define list of drawing items for screens.
 *
 * The Boost containter was choosen over the statand C++ contain because you can detach
 * the pointer from a list with the release method.
 */
typedef boost::ptr_vector< EDA_ITEM > EDA_ITEMS;


/* Simple class for handling grid arrays. */
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


/*******************************************************************/
/* Class to handle how to draw a screen (a board, a schematic ...) */
/*******************************************************************/
class BASE_SCREEN : public EDA_ITEM
{
    EDA_ITEMS m_items;          ///< The drawing items associated with this screen.
    GRIDS     m_grids;          ///< List of valid grid sizes.
    EDA_ITEM* m_drawList; ///< Object list for the screen.

public:
    wxPoint m_DrawOrg;          /* offsets for drawing the circuit on the screen */
    wxPoint m_Curseur;          /* Screen cursor coordinate (on grid) in user units. */
    wxPoint m_MousePosition;    /* Mouse cursor coordinate (off grid) in user units. */
    wxPoint m_MousePositionInPixels;
    wxPoint m_O_Curseur;        /* Relative Screen cursor coordinate (on grid)
                                 * in user units.
                                 * (coordinates from last reset position)*/
    // Scrollbars management:
    int     m_ScrollPixelsPerUnitX; /* Pixels per scroll unit in the horizontal direction. */
    int     m_ScrollPixelsPerUnitY; /* Pixels per scroll unit in the vertical direction. */
    wxSize  m_ScrollbarNumber;      /* Current virtual draw area size in scroll
                                     * units.
                                     * m_ScrollbarNumber * m_ScrollPixelsPerUnit = virtual draw area size in pixels
                                     */
    wxPoint m_ScrollbarPos;         /* Current scroll bar position in scroll
                                     * units. */

    wxPoint m_StartVisu;       /* Coordinates in drawing units of the current
                                * view position (upper left corner of device)
                                */

    wxSize m_SizeVisu;         /* taille en pixels de l'ecran (fenetre de visu
                                * Utile pour recadrer les affichages lors de la
                                * navigation dans la hierarchie */
    bool   m_Center;           /* Center on screen.  If TRUE (0.0) is centered
                                * on screen coordinates can be < 0 and
                                * > 0 except for schematics.
                                * FALSE: when coordinates can only be >= 0
                                * Schematic */
    bool m_FirstRedraw;

    // Undo/redo list of commands
    UNDO_REDO_CONTAINER m_UndoList;             /* Objects list for the undo
                                                 * command (old data) */
    UNDO_REDO_CONTAINER m_RedoList;             /* Objects list for the redo
                                                 * command (old data) */
    unsigned            m_UndoRedoCountMax;     // undo/Redo command Max depth

    /* block control */
    BLOCK_SELECTOR      m_BlockLocate;       /* Block description for block
                                              * commands */

    /* Page description */
    Ki_PageDescr*       m_CurrentSheetDesc;
    int             m_ScreenNumber;
    int             m_NumberOfScreen;

    wxString        m_FileName;
    wxString        m_Title;
    wxString        m_Date;
    wxString        m_Revision;
    wxString        m_Company;
    wxString        m_Commentaire1;
    wxString        m_Commentaire2;
    wxString        m_Commentaire3;
    wxString        m_Commentaire4;

private:
    char            m_FlagRefreshReq;       /* indicates that the screen should
                                             * be redrawn */
    char            m_FlagModified;         // indicates current drawing has
                                            // been modified
    char            m_FlagSave;             // Perform automatica file save.
    EDA_ITEM* m_CurrentItem;          ///< Currently selected object
    GRID_TYPE       m_Grid;                 ///< Current grid selection.

    /* Grid and zoom values. */
public:
    wxPoint	m_GridOrigin;

    wxArrayInt m_ZoomList;          /* Array of standard zoom coefficients. */
    int        m_Zoom;              /* Current zoom coefficient. */
    int        m_ZoomScalar;        /* Allow zooming to non-integer increments.
                                     */
    bool       m_IsPrinting;

public:
    BASE_SCREEN( KICAD_T aType = TYPE_SCREEN );
    ~BASE_SCREEN();

    /**
     * Function setCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param current Any object derived from EDA_ITEM
     */
    void SetCurItem( EDA_ITEM* current ) {  m_CurrentItem = current; }
    EDA_ITEM* GetCurItem() const { return m_CurrentItem; }

    /**
     * Function GetDrawItems().
     *
     * @return - A pointer to the first item in the linked list of draw items.
     */
    virtual EDA_ITEM* GetDrawItems() const { return m_drawList; }

    virtual void SetDrawItems( EDA_ITEM* aItem ) { m_drawList = aItem; }

    void         InitDatas();

    void         SetPageSize( wxSize& aPageSize );
    wxSize       ReturnPageSize( void );
    virtual int  GetInternalUnits( void );

    /**
     * Function CursorRealPosition
     * @return the position in user units of location ScreenPos
     * @param ScreenPos = the screen (in pixel) position co convert
     */
    wxPoint      CursorRealPosition( const wxPoint& ScreenPos );

    /**
     * Return the current cursor position in drawing coordinates.
     *
     * This call inverts the Y axis coordinated of m_Curseur to correct for the difference
     * between the wxWidgets GDI and the Kicad drawing coordinates.
     *
     * @return - The cursor position in drawing coordinates.
     */
    wxPoint      GetCursorDrawPosition() { return wxPoint( m_Curseur.x, -m_Curseur.y ); }

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
    virtual void               ClearUndoRedoList();

    /**
     * Function PushCommandToUndoList
     * add a command to undo in undo list
     * delete the very old commands when the max count of undo commands is
     * reached
     * ( using ClearUndoORRedoList)
     */
    virtual void               PushCommandToUndoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Function PushCommandToRedoList
     * add a command to redo in redo list
     * delete the very old commands when the max count of redo commands is
     * reached
     * ( using ClearUndoORRedoList)
     */
    virtual void               PushCommandToRedoList( PICKED_ITEMS_LIST* aItem );

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


    void    SetRefreshReq() { m_FlagRefreshReq = 1; }
    void    ClrRefreshReq() { m_FlagRefreshReq = 0; }
    void    SetModify() { m_FlagModified = 1; m_FlagSave = 0; }
    void    ClrModify() { m_FlagModified = 0; m_FlagSave = 1; }
    void    SetSave() { m_FlagSave = 1; }
    void    ClrSave() { m_FlagSave = 0; }
    int     IsModify() { return m_FlagModified & 1;  }
    int     IsRefreshReq() { return m_FlagRefreshReq & 1;  }
    int     IsSave() { return m_FlagSave & 1;  }


    //----<zoom stuff>---------------------------------------------------------

    /**
     * Function GetScalingFactor
     * @return the the current scale used to draw items on screen
     * draw coordinates are user coordinates * GetScalingFactor( )
     */
    double  GetScalingFactor()
    {
        return (double) m_ZoomScalar / (double) GetZoom();
    }


    /**
     * Function SetScalingFactor
     * @param the the current scale used to draw items on screen
     * draw coordinates are user coordinates * GetScalingFactor( )
     */
    void        SetScalingFactor( double aScale );

    /**
     * Function GetZoom
     * @return the current zoom factor
     * Note: the zoom factor is NOT the scaling factor
     *       the scaling factor is m_ZoomScalar * GetZoom()
     */
    int         GetZoom() const;

    /**
     * Function SetZoom
     * adjusts the current zoom factor
     */
    bool        SetZoom( int coeff );

    /**
     * Function SetZoomList
     * sets the list of zoom factors.
     * @param aZoomList An array of zoom factors in ascending order, zero terminated
     */
    void        SetZoomList( const wxArrayInt& zoomlist );

    int         Scale( int coord );
    double      Scale( double coord );
    void        Scale( wxPoint& pt );
    void        Scale( wxSize& sz );
    void        Scale( wxRealPoint& sz );

    int         Unscale( int coord );
    void        Unscale( wxPoint& pt );
    void        Unscale( wxSize& sz );

    bool        SetNextZoom();
    bool        SetPreviousZoom();
    bool        SetFirstZoom();
    bool        SetLastZoom();

    //----<grid stuff>----------------------------------------------------------

    /**
     * Return the command ID of the currently selected grid.
     *
     * @return int - Currently selected grid command ID.
     */
    int         GetGridId();

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
    GRID_TYPE   GetGrid();

    const wxPoint& GetGridOrigin();
    void        SetGrid( const wxRealPoint& size );
    void        SetGrid( int id );
    void        SetGridList( GRIDS& sizelist );
    void        AddGrid( const GRID_TYPE& grid );
    void        AddGrid( const wxRealPoint& size, int id );
    void        AddGrid( const wxRealPoint& size, UserUnitType aUnit, int id );

    /**
     * Function GetGridCount().
     * Return the size of the grid list.
     *
     * @returns - The size of the grid list.
     */
    size_t      GetGridCount() const { return m_grids.size(); }

    /**
     * Function GetGrid()
     * Returns the grid object at \a aIndex.
     *
     * @param aIndex - The grid list index.
     * @return - The grid object at \a aIndex or the current grid if the grid list is empty.
     */
    GRID_TYPE&  GetGrid( size_t aIndex );

    /**
     * Function GetGrids().
     * Copy the grid list to \a aList.
     *
     * @param aList - List to copy to.
     */
    void         GetGrids( GRIDS& aList );

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
        return useMouse ? m_MousePosition : m_Curseur;
    }


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "BASE_SCREEN" );
    }

    /**
     * Helpers for accessing the draw item list.
     */
    EDA_ITEMS::iterator Begin() { return m_items.begin(); }
    EDA_ITEMS::iterator End() { return m_items.end(); }
    virtual void AddItem( EDA_ITEM* aItem );
    virtual void InsertItem(  EDA_ITEMS::iterator aIter, EDA_ITEM* aItem );

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


#endif  /* #ifndef __CLASS_BASE_SCREEN_H__ */
