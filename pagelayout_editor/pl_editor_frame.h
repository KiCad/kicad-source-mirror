/**
 * @file pl_editor_frame.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef  _PL_EDITOR_FRAME_H
#define  _PL_EDITOR_FRAME_H


#include <config_params.h>
#include <draw_frame.h>
#include <class_drawpanel.h>
#include <class_pl_editor_screen.h>
#include <class_pl_editor_layout.h>

class PROPERTIES_FRAME;
class DESIGN_TREE_FRAME;
class WORKSHEET_DATAITEM;


/**
 * Class PL_EDITOR_FRAME
 * is the main window used in the page layout editor.
 */
class PL_EDITOR_FRAME : public EDA_DRAW_FRAME
{
    PL_EDITOR_LAYOUT m_pageLayout;

    int         m_designTreeWidth;      // the last width (in pixels) of m_treePagelayout
    int         m_propertiesFrameWidth; // the last width (in pixels) of m_propertiesPagelayout

    wxChoice*   m_originSelectBox;      // Corner origin choice for coordinates
    int         m_originSelectChoice;   // the last choice for m_originSelectBox
    wxChoice*   m_pageSelectBox;        // The page number sel'ector (page 1 or other pages
                                        // usefull when there are some items which are
                                        // only on page 1, not on page 1

    wxPoint     m_grid_origin;

protected:
    /// The last filename chosen to be proposed to the user
    wxString                m_lastFileName;
    DESIGN_TREE_FRAME*      m_treePagelayout;
    PROPERTIES_FRAME*       m_propertiesPagelayout;

private:
    // list of PARAM_CFG_xxx to read/write parameters saved in config
    PARAM_CFG_ARRAY         m_configSettings;

public:
    PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PL_EDITOR_FRAME();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl );   // overload KIWAY_PLAYER

    /**
     * Function LoadPageLayoutDescrFile
     * Loads a .kicad_wks page layout descr file
     * @param aFullFileName = the filename.
     */
    bool LoadPageLayoutDescrFile( const wxString& aFullFileName );

    /**
     * Function SavePageLayoutDescrFile
     * Save the current layout in a .kicad_wks page layout descr file
     * @param aFullFileName = the filename.
     */
    bool SavePageLayoutDescrFile( const wxString& aFullFileName );

    /**
     * Function InsertPageLayoutDescrFile
     * Loads a .kicad_wks page layout descr file, and add items
     * to the current layout list
     * @param aFullFileName = the filename.
     */
    bool InsertPageLayoutDescrFile( const wxString& aFullFileName );

    void    OnCloseWindow( wxCloseEvent& Event );

    // Virtual basic functions:
    void    RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void    ReCreateHToolbar();

    void SetPageSettings(const PAGE_INFO&);
    const PAGE_INFO& GetPageSettings () const;                  // overload EDA_DRAW_FRAME
    const wxSize GetPageSizeIU() const;                         // overload EDA_DRAW_FRAME

    /**
     * Function GetZoomLevelIndicator
     * returns a human readable value which can be displayed as zoom
     * level indicator in dialogs.
     * Virtual from the base class
     */
    const wxString GetZoomLevelIndicator() const;

    PL_EDITOR_SCREEN* GetScreen() const                         // overload EDA_DRAW_FRAME
    {
        return (PL_EDITOR_SCREEN*) EDA_DRAW_FRAME::GetScreen();
    }

    const wxPoint& GetAuxOrigin() const                         // overload EDA_DRAW_FRAME
    {
        static wxPoint dummy;   // ( 0,0 );
        return dummy;
    }
    void SetAuxOrigin( const wxPoint& aPosition ) {}            // overload EDA_DRAW_FRAME

    const wxPoint& GetGridOrigin() const                        // overload EDA_DRAW_FRAME
    {
        return m_grid_origin;
    }
    void SetGridOrigin( const wxPoint& aPoint )                 // overload EDA_DRAW_FRAME
    {
        m_grid_origin = aPoint;
    }

    const TITLE_BLOCK& GetTitleBlock() const;                   // overload EDA_DRAW_FRAME
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock );       // overload EDA_DRAW_FRAME

    void UpdateStatusBar();                                     // overload EDA_DRAW_FRAME

    /**
     * Must be called to initialize parameters when a new page layout
     * description is loaded
     */
    void OnNewPageLayout();

    /**
     * creates or updates the right vertical toolbar.
     * @note This is currently not used.
     */
    void    ReCreateVToolbar();

    /**
     * Create or update the left vertical toolbar (option toolbar
     * @note This is currently not used.
     */
    void    ReCreateOptToolbar();

    void    ReCreateMenuBar();
    void    OnLeftClick( wxDC* aDC, const wxPoint& aMousePos );
    void    OnLeftDClick( wxDC* aDC, const wxPoint& aMousePos );
    bool    OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu );
    double  BestZoom();

    // Events created by clicking on the design tree list:
    void OnTreeSelection( wxTreeEvent& event );
    void OnTreeMiddleClick( wxTreeEvent& event );
    void OnTreeRightClick( wxTreeEvent& event );

    const PL_EDITOR_LAYOUT& GetPageLayout() const { return m_pageLayout; }

    /**
     * Page layout editor can show the title block using a page number
     * 1 or an other number.
     * This is because some items can be shown (or not) only on page 1
     * (a feature  which look like word processing option
     * "page 1 differs from other pages".
     * @return true if the page 1 is selected, and false if not
     */
    bool GetPageNumberOption() const
    {
        return m_pageSelectBox->GetSelection() == 0;
    }

    /**
     * Displays the short filename (if exists) loaded file
     * on the caption of the main window
     */
    void                UpdateTitleAndInfo();

    /**
     * Populates the applicatios settings list.
     * (list of parameters that must be saved in project parameters)
     * Currently, only the settings that are needed at start up by the main window are
     * defined here.  There are other locally used settings scattered throughout the
     * source code (mainly in dialogs).  If you need to define a configuration
     * setting that need to be loaded at run time, this is the place to define it.
     */
    PARAM_CFG_ARRAY&    GetConfigurationSettings( void );

    void LoadSettings( wxConfigBase* aCfg );    // override virtual

    void SaveSettings( wxConfigBase* aCfg );    // override virtual

    void                Process_Special_Functions( wxCommandEvent& event );
    void                OnSelectOptionToolbar( wxCommandEvent& event );

    /**
     * called when the user select one of the 4 page corner as corner
     * reference (or the left top paper corner)
     */
    void                OnSelectCoordOriginCorner( wxCommandEvent& event );

    /**
     * Toggle the display mode between the normal mode and the editor mode:
     * In normal mode, title block texts are shown like they will be
     * shown in other kicad applications: the format symbols in texts
     * are replaced by the actual text.
     * In editor mode, the format symbols in texts are not replaced
     * by the actual text, and therefore format symbols are displayed.
     */
    void                OnSelectTitleBlockDisplayMode( wxCommandEvent& event );

    void OnUpdateTitleBlockDisplayNormalMode( wxUpdateUIEvent& event );
    void OnUpdateTitleBlockDisplaySpecialMode( wxUpdateUIEvent& event );

    /**
     * Function OnQuit
     * called on request of application quit
     */
    void                OnQuit( wxCommandEvent& event );

    /**
     * Function OnHotKey.
     *  ** Commands are case insensitive **
     *  Some commands are relatives to the item under the mouse cursor
     * @param aDC = current device context
     * @param aHotkeyCode = hotkey code (ascii or wxWidget code for special keys)
     * @param aPosition The cursor position in logical (drawing) units.
     * @param aItem = NULL or pointer on a EDA_ITEM under the mouse cursor
     */
    bool OnHotKey( wxDC* aDC, int aHotkeyCode, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    void                Process_Settings( wxCommandEvent& event );
    void                Process_Config( wxCommandEvent& event );

    /**
     * Function ToPlotter
     * Open a dialog frame to create plot and drill files
     * relative to the current board
     */
    void                ToPlotter( wxCommandEvent& event );

    /**
     * Function ToPrinter
     * Open a dialog frame to print layers
     */
    void                ToPrinter( wxCommandEvent& event );

    void                Files_io( wxCommandEvent& event );
    bool                GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    /** Virtual function PrintPage
     * used to print a page
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMasklayer = a 32 bits mask: bit n = 1 -> layer n is printed
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void    PrintPage( wxDC* aDC, LSET aPrintMasklayer,
                               bool aPrintMirrorMode, void * aData );

    void OnFileHistory( wxCommandEvent& event );

    /**
     * @return the filename of the current layout descr file
     * If this is the default (no loaded file) returns a emtpy name
     * or a new design.
     */
    const wxString& GetCurrFileName() const;

    /**
     * Stores the current layout descr file filename
     */
    void SetCurrFileName( const wxString& aName );

    /**
     * Function RebuildDesignTree
     * Re creates the design graphic tree list items,
     * and ensures each item has an unique name
     */
    void RebuildDesignTree();

    /**
     * Function AddPageLayoutItem
     * Add a new item to the page layout item list.
     * @param aType = the type of item:
     *  WS_TEXT, WS_SEGMENT, WS_RECT, WS_POLYPOLYGON
     * @param aIdx = the position in list to insert the new item.
     * @return a reference to the new item
     */
    WORKSHEET_DATAITEM * AddPageLayoutItem( int aType, int aIdx = -1 );

    /**
     * Function GetSelectedItem
     * @return the current selected item, or NULL
     */
    WORKSHEET_DATAITEM * GetSelectedItem();

    /**
     * Function Locate
     * @return the page layout item found at position aPosition
     * @param aPosition = the position (in user units) of the reference point
     */
    WORKSHEET_DATAITEM *Locate( const wxPoint& aPosition );

    /**
     * Initialize a move item command
     * @param aItem is the item to move
     */
    void MoveItem( WORKSHEET_DATAITEM* aItem );

    /**
     * Save in Undo list the layout, and place an item being moved.
     * @param aItem is the item moved
     */
    void PlaceItem( WORKSHEET_DATAITEM* aItem );

    /**
     * Must be called after a change
     * in order to set the "modify" flag of the current screen
     */
    void OnModify()
    {
        GetScreen()->SetModify();
    }

    /**
     * Save a copy of the description (in a S expr string)
     * for Undo/redo commands
     */
    void SaveCopyInUndoList();

    /** Redo the last edition:
     * - Place the current edited layout in undo list
     * - Get the previous version of the current edited layput
     */
    void GetLayoutFromRedoList( wxCommandEvent& event );

    /** Undo the last edition:
     * - Place the current layout in Redo list
     * - Get the previous version of the current edited layout
     */
    void GetLayoutFromUndoList( wxCommandEvent& event );

    /**
     * Remove the last command in Undo List.
     * Used to clean the Undo stack after a cancel command
     */
    void RemoveLastCommandInUndoList();

    DECLARE_EVENT_TABLE()
};

#endif /* _PL_EDITOR_FRAME_H */
