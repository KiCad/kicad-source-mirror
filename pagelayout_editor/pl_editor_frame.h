/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <eda_draw_frame.h>
#include <base_screen.h>
#include "pl_editor_layout.h"
#include "pl_draw_panel_gal.h"

#include <memory>

class PL_DRAW_PANEL_GAL;
class PROPERTIES_FRAME;
class DS_DATA_ITEM;
class wxChoice;

#ifndef __linux__
class NL_PL_EDITOR_PLUGIN;
#else
class SPNAV_2D_PLUGIN;
#endif

/**
 * The main window used in the drawing sheet editor.
 */
class PL_EDITOR_FRAME : public EDA_DRAW_FRAME
{
public:
    PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PL_EDITOR_FRAME();

    PROPERTIES_FRAME* GetPropertiesFrame() { return m_propertiesPagelayout; }

    /**
     * Show the dialog displaying the list of DS_DATA_ITEM items in the page layout
     */
    void ShowDesignInspector();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl ) override;

    /**
     * Load a .kicad_wks drawing sheet file.
     *
     * @param aFullFileName is the filename.
     */
    bool LoadDrawingSheetFile( const wxString& aFullFileName );

    /**
     * Save the current layout in a .kicad_wks drawing sheet file.
     *
     * @param aFullFileName is the filename.
     */
    bool SaveDrawingSheetFile( const wxString& aFullFileName );

    /**
     * Load a .kicad_wks drawing sheet file, and add items to the current layout list.
     *
     * @param aFullFileName is the filename.
     */
    bool InsertDrawingSheetFile( const wxString& aFullFileName );

    /**
     * Get if the drawing sheet has been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Event handler for the wxID_EXIT and wxID_CLOSE events.
     */
    void OnExit( wxCommandEvent& aEvent );

    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;

    // The Tool Framework initialization
    void setupTools();

    void UpdateToolbarControlSizes() override;

    void SetPageSettings(const PAGE_INFO&) override;
    const PAGE_INFO& GetPageSettings () const override;
    const VECTOR2I   GetPageSizeIU() const override;

    PL_DRAW_PANEL_GAL* GetCanvas() const override;
    SELECTION& GetCurrentSelection() override;

    const VECTOR2I& GetGridOrigin() const override { return m_grid_origin; }
    void SetGridOrigin( const VECTOR2I& aPoint ) override { m_grid_origin = aPoint; }

    /**
     * Calculate the position (in page, in iu) of the corner used as coordinate origin
     * of items.
     */
    VECTOR2I ReturnCoordOriginCorner() const;

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    void CommonSettingsChanged( int aFlags ) override;

    void DisplayGridMsg() override;

    void UpdateStatusBar() override;

    /**
     * Must be called to initialize parameters when a new drawing sheet is loaded
     */
    void OnNewDrawingSheet();

    const PL_EDITOR_LAYOUT& GetPageLayout() const { return m_pageLayout; }
    PL_EDITOR_LAYOUT& GetPageLayout() { return m_pageLayout; }

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    /**
     * Drawing sheet editor can show the title block using a page number 1 or another number.
     *
     * This is because some items can be shown (or not) only on page 1 (a feature  which
     * looks like word processing option "page 1 differs from other pages").
     *
     * @return true if the page 1 is selected, and false if not.
     */
    bool GetPageNumberOption() const;

    /**
     * Display the short filename (if exists) loaded file on the caption of the main window.
     */
    void UpdateTitleAndInfo();

    /**
     * Display the size of the sheet to the message panel.
     */
    void UpdateMsgPanelInfo();

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    void OnSelectPage( wxCommandEvent& event );

    /**
     * Called when the user select one of the 4 page corner as corner reference (or the
     * left top paper corner).
     */
    void OnSelectCoordOriginCorner( wxCommandEvent& event );

    /**
     * Open a dialog frame to print layers.
     */
    void ToPrinter( bool doPreview );

    void Files_io( wxCommandEvent& event );

    void OnFileHistory( wxCommandEvent& event );
    void OnClearFileHistory( wxCommandEvent& aEvent );

    /**
     * @return the filename of the current layout descr file
     * If this is the default (no loaded file) returns a empty name
     * or a new design.
     */
    wxString GetCurrentFileName() const override;

    /**
     * Store the current layout description file filename.
     */
    void SetCurrentFileName( const wxString& aName );

    /**
     * Refresh the library tree and redraw the window.
     */
    void HardRedraw() override;

    /**
     * Add a new item to the drawing sheet item list.
     *
     * @param aType is the type of item:
     *  DS_TEXT, DS_SEGMENT, DS_RECT, DS_POLYPOLYGON
     * @param aIdx is the position in list to insert the new item.
     * @return a reference to the new item.
     */
    DS_DATA_ITEM* AddDrawingSheetItem( int aType );

    /**
     * Must be called after a change in order to set the "modify" flag.
     */
    void OnModify() override;

    /**
     * Save a copy of the description (in a S expr string) for Undo/redo commands.
     */
    void SaveCopyInUndoList();

    /**
     * Redo the last edit:
     *  - Place the current edited layout in undo list.
     *  - Get the previous version of the current edited layout.
     */
    void GetLayoutFromRedoList();

    /**
     * Undo the last edit:
     *  - Place the current layout in Redo list.
     *  - Get the previous version of the current edited layout.
     */
    void GetLayoutFromUndoList();

    /**
     * Apply the last command in Undo List without stacking a Redo. Used to clean the
     * Undo stack after canceling a command.
     */
    void RollbackFromUndo();

    void ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount = -1 ) override;

protected:
    bool saveCurrentPageLayout();

    void configureToolbars() override;

    void setupUIConditions() override;

    void doReCreateMenuBar() override;

    void DoWithAcceptedFiles() override;

    DECLARE_EVENT_TABLE();

private:
    void handleActivateEvent( wxActivateEvent& aEvent ) override;
    void handleIconizeEvent( wxIconizeEvent& aEvent ) override;

protected:
    /// The last filename chosen to be proposed to the user
    PROPERTIES_FRAME* m_propertiesPagelayout;

private:
    PL_EDITOR_LAYOUT  m_pageLayout;

    int               m_propertiesFrameWidth; // last width (in pixels) of m_propertiesPagelayout

    wxChoice*         m_originSelectBox;      // Corner origin choice for coordinates
    int               m_originSelectChoice;   // the last choice for m_originSelectBox
    wxChoice*         m_pageSelectBox;        // The page number sel'ector (page 1 or other pages
                                              // useful when there are some items which are
    wxString          m_mruImagePath;         // Most recently used path for placing a new image
                                              // only on page 1, not on page 1
    VECTOR2I          m_grid_origin;

#ifndef __linux__
    std::unique_ptr<NL_PL_EDITOR_PLUGIN> m_spaceMouse;
#else
    std::unique_ptr<SPNAV_2D_PLUGIN> m_spaceMouse;
#endif

    wxString m_originChoiceList[5] =
        {
            _("Left Top paper corner"),
            _("Right Bottom page corner"),
            _("Left Bottom page corner"),
            _("Right Top page corner"),
            _("Left Top page corner")
        };
};

#endif /* _PL_EDITOR_FRAME_H */
