/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_draw_frame.h>
#include <pl_editor_screen.h>
#include <pl_editor_layout.h>
#include <pl_draw_panel_gal.h>

class PROPERTIES_FRAME;
class WS_DATA_ITEM;


/**
 * Class PL_EDITOR_FRAME
 * is the main window used in the page layout editor.
 */

class PL_EDITOR_FRAME : public EDA_DRAW_FRAME
{
    PL_EDITOR_LAYOUT m_pageLayout;

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
    PROPERTIES_FRAME*       m_propertiesPagelayout;

private:
    // list of PARAM_CFG_xxx to read/write parameters saved in config
    PARAM_CFG_ARRAY         m_configSettings;

public:
    PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PL_EDITOR_FRAME() {}

    PROPERTIES_FRAME* GetPropertiesFrame() { return m_propertiesPagelayout; }

    /**
     * Show the dialog displaying the list of WS_DATA_ITEM items in the page layout
     */
    void ShowDesignInspector();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl ) override;

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
     * Loads a .kicad_wks page layout descr file, and add items to the current layout list
     * @param aFullFileName = the filename.
     */
    bool InsertPageLayoutDescrFile( const wxString& aFullFileName );


    /*
     * Function OnExit
     * Event handler for the wxID_EXIT and wxID_CLOSE events
     */
    void OnExit( wxCommandEvent& aEvent );

    /*
     * Function OnCloseWindow
     * Event handler for the close event
     */
    void OnCloseWindow( wxCloseEvent& aEvent );

    // The Tool Framework initalization
    void setupTools();

    // Virtual basic functions:
    void    ReCreateHToolbar() override;

    void SetPageSettings(const PAGE_INFO&) override;
    const PAGE_INFO& GetPageSettings () const override;
    const wxSize GetPageSizeIU() const override;

    /**
     * Function GetZoomLevelIndicator
     * returns a human readable value which can be displayed in dialogs.
     */
    const wxString GetZoomLevelIndicator() const override;

    PL_DRAW_PANEL_GAL* GetCanvas() const override;

    PL_EDITOR_SCREEN* GetScreen() const override
    {
        return (PL_EDITOR_SCREEN*) EDA_DRAW_FRAME::GetScreen();
    }

    const wxPoint& GetAuxOrigin() const override
    {
        static wxPoint dummy;   // ( 0,0 );
        return dummy;
    }
    void SetAuxOrigin( const wxPoint& aPosition ) override {}

    const wxPoint& GetGridOrigin() const override
    {
        return m_grid_origin;
    }

    void SetGridOrigin( const wxPoint& aPoint ) override
    {
        m_grid_origin = aPoint;
    }

    /**
     * calculate the position (in page, in iu) of the corner used as coordinate origin
     * of items
     */
    wxPoint ReturnCoordOriginCorner() const;

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    void DisplayGridMsg();

    void UpdateStatusBar() override;

    /**
     * Must be called to initialize parameters when a new page layout description is loaded
     */
    void OnNewPageLayout();

    /**
     * creates or updates the right vertical toolbar.
     */
    void    ReCreateVToolbar() override;

    /**
     * Create or update the left vertical toolbar (option toolbar
     * @note This is currently not used.
     */
    void    ReCreateOptToolbar() override;

    void    ReCreateMenuBar() override;
    double  BestZoom() override;

    void    SyncToolbars() override;

    const PL_EDITOR_LAYOUT& GetPageLayout() const { return m_pageLayout; }
    PL_EDITOR_LAYOUT& GetPageLayout() { return m_pageLayout; }

    const BOX2I GetDocumentExtents() const override;

    /**
     * Page layout editor can show the title block using a page number 1 or another number.
     * This is because some items can be shown (or not) only on page 1 (a feature  which
     * looks like word processing option "page 1 differs from other pages").
     * @return true if the page 1 is selected, and false if not
     */
    bool GetPageNumberOption() const
    {
        return m_pageSelectBox->GetSelection() == 0;
    }

    /**
     * Displays the short filename (if exists) loaded file on the caption of the main window
     */
    void UpdateTitleAndInfo();

    /**
     * Populates the applicatios settings list.
     * (list of parameters that must be saved in project parameters)
     * Currently, only the settings that are needed at start up by the main window are
     * defined here.  There are other locally used settings scattered throughout the
     * source code (mainly in dialogs).  If you need to define a configuration
     * setting that need to be loaded at run time, this is the place to define it.
     */
    PARAM_CFG_ARRAY& GetConfigurationSettings() { return m_configSettings; }

    void LoadSettings( wxConfigBase* aCfg ) override;

    void SaveSettings( wxConfigBase* aCfg ) override;

    void OnSelectPage( wxCommandEvent& event );

    /**
     * called when the user select one of the 4 page corner as corner reference (or the
     * left top paper corner)
     */
    void OnSelectCoordOriginCorner( wxCommandEvent& event );

    /**
     * Toggle the display mode between the normal mode and the editor mode:
     * In normal mode, title block texts are shown like they will be shown in other kicad
     * applications: the format symbols in texts are replaced by the actual text.
     * In editor mode, the format symbols in texts are not replaced by the actual text,
     * and therefore format symbols are displayed.
     */
    void OnSelectTitleBlockDisplayMode( wxCommandEvent& event );

    void OnUpdateTitleBlockDisplayNormalMode( wxUpdateUIEvent& event );
    void OnUpdateTitleBlockDisplayEditMode( wxUpdateUIEvent& event );

    /**
     * Function ToPrinter
     * Open a dialog frame to print layers
     */
    void ToPrinter( bool doPreview );

    void Files_io( wxCommandEvent& event );

    /** Virtual function PrintPage
     * used to print a page
     * @param aDC = wxDC given by the calling print function
     */
    virtual void PrintPage( wxDC* aDC ) override;

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
     * Refresh the library tree and redraw the window
     */
    void HardRedraw() override;

    /**
     * Function AddPageLayoutItem
     * Add a new item to the page layout item list.
     * @param aType = the type of item:
     *  WS_TEXT, WS_SEGMENT, WS_RECT, WS_POLYPOLYGON
     * @param aIdx = the position in list to insert the new item.
     * @return a reference to the new item
     */
    WS_DATA_ITEM* AddPageLayoutItem( int aType );

    /**
     * Must be called after a change in order to set the "modify" flag
     */
    void OnModify()
    {
        GetScreen()->SetModify();
    }

    /**
     * Save a copy of the description (in a S expr string) for Undo/redo commands.
     * Optionally save the pageInfo and titleBlock as well.
     */
    void SaveCopyInUndoList( bool aSavePageSettingsAndTitleBlock = false );

    /** Redo the last edit:
     * - Place the current edited layout in undo list
     * - Get the previous version of the current edited layput
     */
    void GetLayoutFromRedoList();

    /** Undo the last edit:
     * - Place the current layout in Redo list
     * - Get the previous version of the current edited layout
     */
    void GetLayoutFromUndoList();

    /**
     * Apply the last command in Undo List without stacking a Redo. Used to clean the
     * Undo stack after cancelling a command.
     */
    void RollbackFromUndo();

protected:
    bool saveCurrentPageLayout();

    DECLARE_EVENT_TABLE()
};

#endif /* _PL_EDITOR_FRAME_H */
