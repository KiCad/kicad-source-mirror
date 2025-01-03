/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  WX_GERBER_STRUCT_H
#define  WX_GERBER_STRUCT_H

#include <file_history.h>
#include <eda_draw_frame.h>
#include <layer_ids.h>
#include <gerbview.h>
#include <gbr_layout.h>
#include <page_info.h>
#include <gbr_display_options.h>

#include <memory>

#define NO_AVAILABLE_LAYERS UNDEFINED_LAYER

class DCODE_SELECTION_BOX;
class GERBER_LAYER_WIDGET;
class GBR_LAYER_BOX_SELECTOR;
class GERBER_DRAW_ITEM;
class GERBER_FILE_IMAGE;
class GERBER_FILE_IMAGE_LIST;
class GERBVIEW_SETTINGS;
class LSET;
class REPORTER;
class SELECTION;
class wxStaticText;

#ifndef __linux__
class NL_GERBVIEW_PLUGIN;
#else
class SPNAV_2D_PLUGIN;
#endif


/**
 * The main window used in GerbView.
 */

#define GERBVIEW_FRAME_NAME wxT( "GerberFrame" )

class GERBVIEW_FRAME : public EDA_DRAW_FRAME
{
public:
    GERBVIEW_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~GERBVIEW_FRAME();

    void doCloseWindow() override;

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl ) override;

    /**
     * Load a list of Gerber and NC drill files and updates the view based on them.
     *
     * @param aPath is the base path for the filenames if they are relative
     * @param aFilenameList is a list of filenames to load
     * @param aFileType is a list of type of files to load (0 = Gerber, 1 = NC drill, 2 Autodetect)
     *        Successfully autodetected files will have their type changed
     * @return true if every file loaded successfully
     */
    bool LoadListOfGerberAndDrillFiles( const wxString& aPath, const wxArrayString& aFilenameList,
                                        std::vector<int>* aFileType );

    void UpdateStatusBar() override;

    GERBVIEW_SETTINGS* gvconfig() const;

    /// Updates the GAL with display settings changes
    void ApplyDisplaySettingsToGAL();

    /**
     * Test whether a given element category is visible. Keep this as an inline function.
     *
     * @param aLayerID is an item id from the enum GERBVIEW_LAYER_ID
     * @return bool - true if the element is visible.
     */
    bool IsElementVisible( int aLayerID ) const;

    /**
     * Change the visibility of an element category.
     *
     * @param aItemIdVisible is an item id from the enum GERBVIEW_LAYER_ID
     * @param aNewState = The new visibility state of the element category
     *  (see enum PCB)
     */
    void SetElementVisibility( int aLayerID, bool aNewState );

    /**
     * @param aVisible true if the grid must be shown.
     */
    void SetGridVisibility( bool aVisible ) override;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings.
     *
     * @return #LSET of the visible layers
     */
    LSET GetVisibleLayers() const;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings.
     *
     * @param aLayerMask The new set of visible layers
     */
    void SetVisibleLayers( const LSET& aLayerMask );

    /**
     * Test whether a given layer is visible.
     *
     * @param aLayer The layer to be tested (still 0-31!)
     * @return true if the layer is visible.
     */
    bool IsLayerVisible( int aLayer ) const;

    /**
     * Return the color of a gerber visible element.
     */
    COLOR4D GetVisibleElementColor( int aLayerID );

    void SetVisibleElementColor( int aLayerID, const COLOR4D& aColor );

    COLOR4D GetLayerColor( int aLayer ) const;
    void SetLayerColor( int aLayer, const COLOR4D& aColor );

    /**
     * Change out all the layers in m_Layers; called upon loading new gerber files.
     */
    void ReFillLayerWidget();

    /**
     * change the currently active layer to \a aLayer and update the #GERBER_LAYER_WIDGET.
     */
    void SetActiveLayer( int aLayer, bool doLayerWidgetUpdate = true );

    /**
     * Return the active layer.
     */
    int GetActiveLayer() const { return m_activeLayer; }

    /**
     * Find the next empty layer.
     *
     * If no empty layers are found, #NO_AVAILABLE_LAYERS is return.
     *
     * @return The first empty layer found or #NO_AVAILABLE_LAYERS.
     */
    int getNextAvailableLayer() const;

    /**
     * Update the currently "selected" layer within the #GERBER_LAYER_WIDGET.
     * The currently active layer is defined by the return value of GetActiveLayer().
     * <p>
     * This function cannot be inline without including layer_widget.h in here and we do not
     * want to do that.
     */
    void syncLayerWidget();

    /**
     * Update the currently "selected" layer within m_SelLayerBox.
     * The currently active layer, as defined by the return value of GetActiveLayer().
     *
     * @param aRebuildLayerBox true to rebuild the layer box of false to just update the selection.
     */
    void syncLayerBox( bool aRebuildLayerBox = false );

    /**
     * Display the short filename (if exists) of the selected layer on the caption of the main
     * GerbView window and some other parameters.
     *
     *  - Name of the layer (found in the gerber file: LN &ltname&gt command) in the status bar
     *  - Name of the Image (found in the gerber file: IN &ltname&gt command) in the status bar
     *    and other data in toolbar
     */
    void UpdateTitleAndInfo();

    /**
     * Display the current grid pane on the status bar.
     */
    void DisplayGridMsg() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    void ToggleLayerManager();

    void ShowChangedLanguage() override;

    /// Handles the changing of the highlighted component/net/attribute
    void OnSelectHighlightChoice( wxCommandEvent& event );

    /**
     * Select the active DCode for the current active layer.
     * Items using this DCode are highlighted.
     */
    void OnSelectActiveDCode( wxCommandEvent& event );

    /**
     * Select the active layer:
     *  - if a file is loaded, it is loaded in this layer
     *  - this layer is displayed on top of other layers
     */
    void OnSelectActiveLayer( wxCommandEvent& event );

    /**
     * Called on request of application quit.
     */
    void OnQuit( wxCommandEvent& event );

    void OnUpdateSelectDCode( wxUpdateUIEvent& aEvent );

    /**
     * Delete the current data and loads a Gerber file selected from history list on current layer.
     */
    void OnGbrFileHistory( wxCommandEvent& event );

    /**
     * Delete the current data and load a drill file in Excellon format selected from
     * history list on current layer.
     */
    void OnDrlFileHistory( wxCommandEvent& event );

    /**
     * Delete the current data and load a zip archive file selected from the
     * history list. The archive is expected containing a set of gerber and drill file
     */
    void OnZipFileHistory( wxCommandEvent& event );

    /**
     * Delete the current data and load a gerber job file selected from the history list.
     */
    void OnJobFileHistory( wxCommandEvent& event );

    /**
     * Extract gerber and drill files from the zip archive, and load them.
     *
     * @param aFullFileName is the full filename of the zip archive
     * @param aReporter a REPORTER to collect warning and error messages
     * @return true if OK, false if a file cannot be readable
     */
    bool unarchiveFiles( const wxString& aFullFileName, REPORTER* aReporter = nullptr );

    /**
     * Load a given file or selected file(s), if the filename is empty.
     *
     * @param aFileName - file name with full path to open or empty string.
     *                    if empty string: a dialog will be opened to select one or
     *                    a set of files
     * @return true if file was opened successfully.
     */
    bool LoadAutodetectedFiles( const wxString& aFileName );

    /**
     * Load a given Gerber file or selected file(s), if the filename is empty.
     *
     * @param aFileName - file name with full path to open or empty string.
     *                    if empty string: a dialog will be opened to select one or
     *                    a set of files
     * @return true if file was opened successfully.
     */
    bool LoadGerberFiles( const wxString& aFileName );
    bool Read_GERBER_File( const wxString& GERBER_FullFileName );

    /**
     * Load a drill (EXCELLON) file or many files.
     *
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file. In this case one one file is loaded
     *                    if empty string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool LoadExcellonFiles( const wxString& aFileName );
    bool Read_EXCELLON_File( const wxString& aFullFileName );

    /**
     * Load a zipped archive file.
     *
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file.
     *                    if empty string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool LoadZipArchiveFile( const wxString& aFileName );


    /**
     * Load a Gerber job file, and load gerber files found in job files.
     *
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file.
     *                    if empty string: user will be prompted for filename(s)
     * @return true if file(s) was opened successfully.
     */
    bool LoadGerberJobFile( const wxString& aFileName );

    // PCB handling
    bool Clear_DrawLayers( bool query );
    void Erase_Current_DrawLayer( bool query );

    void SortLayersByFileExtension();
    void SortLayersByX2Attributes();

    // Adjust draw params: draw offset and draw rotation for a gerber file image
    void SetLayerDrawPrms();

    /**
     * Takes a layer remapping and reorders the layers.
     *
     * @param remapping A map of old layer number -> new layer number mapping.
     *                  Generally sourced from the SortImagesBy* functions.
     */
    void RemapLayers( const std::unordered_map<int, int>& remapping );

    /**
     * Update each layers' differential option. Needed when xor mode changes or the active layer
     * changes (due to changing rendering order) which matters for xor mode but not otherwise.
     */
    void UpdateXORLayers();

    /*
     * Do nothing in GerbView.
     */
    void SaveCopyInUndoList( GERBER_DRAW_ITEM* aItemToCopy,
                             UNDO_REDO aTypeCommand = UNDO_REDO::UNSPECIFIED ) { }

    /**
     * Create a new entry in undo list of commands and add a list of pickers to handle a list
     * of items.
     *
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                             UNDO_REDO aTypeCommand )
    {
        // currently: do nothing in GerbView.
    }

    ///< @copydoc EDA_DRAW_FRAME::ActivateGalCanvas
    void ActivateGalCanvas() override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    SELECTION& GetCurrentSelection() override;

    /**
     * Set the m_gerberLayout member in such as way as to ensure deleting any previous
     * GBR_LAYOUT.
     * @param aLayout The GBR_LAYOUT to put into the frame.
     */
    void SetLayout( GBR_LAYOUT* aLayout )
    {
        delete m_gerberLayout;
        m_gerberLayout = aLayout;
    }

    GBR_LAYOUT* GetGerberLayout() const
    {
        wxASSERT( m_gerberLayout );
        return m_gerberLayout;
    }

    /**
     * Accessors to GERBER_FILE_IMAGE_LIST and GERBER_FILE_IMAGE data
     */
    GERBER_FILE_IMAGE_LIST* GetImagesList() const
    {
        return m_gerberLayout->GetImagesList();
    }

    GERBER_FILE_IMAGE* GetGbrImage( int aIdx ) const;

    unsigned ImagesMaxCount() const;    ///< The max number of file images


    void SetPageSettings( const PAGE_INFO& aPageSettings ) override;
    const PAGE_INFO& GetPageSettings() const override;
    const VECTOR2I   GetPageSizeIU() const override;

    const VECTOR2I& GetGridOrigin() const override { return m_grid_origin; }
    void            SetGridOrigin( const VECTOR2I& aPoint ) override { m_grid_origin = aPoint; }

    const TITLE_BLOCK&  GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    /**
     * Show the dialog box for layer selection.
     *
     * Providing the option to also display a "(Deselect)" radiobutton makes the
     *  GerbView's "Export to Pcbnew" command) more "user friendly",
     * by permitting any layer to be "deselected" immediately after its
     * corresponding radiobutton has been clicked on. (It would otherwise be
     * necessary to first cancel the "Select Layer:" dialog box (invoked after a
     * different radiobutton is clicked on) prior to then clicking on the "Deselect"
     * button provided within the "Layer selection:" dialog box).
     *
     * @param aDefaultLayer = Preselection (NB_PCB_LAYERS for "(Deselect)" layer)
     * @param aCopperLayerCount = number of copper layers
     * @param aGerberName = Name of Gerber file to select KiCad layer for
     * @return new layer value (NB_PCB_LAYERS when "(Deselect)" radiobutton selected),
     *                         or -1 if canceled
     */
    int SelectPCBLayer( int aDefaultLayer, int aCopperLayerCount, const wxString& aGerberName );

    /**
     * @return the color of the grid
     */
    COLOR4D GetGridColor() override;

    ///< @copydoc EDA_DRAW_FRAME::SetGridColor()
    virtual void SetGridColor( const COLOR4D& aColor ) override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override
    {
        wxASSERT( m_gerberLayout );
        return m_gerberLayout->ViewBBox();
    }

    DECLARE_EVENT_TABLE()

protected:
    void configureToolbars() override;
    void setupUIConditions() override;
    void doReCreateMenuBar() override;

    void handleActivateEvent( wxActivateEvent& aEvent ) override;
    void handleIconizeEvent( wxIconizeEvent& aEvent ) override;

private:
    void updateComponentListSelectBox();
    void updateNetnameListSelectBox();
    void updateAperAttributesSelectBox();
    void updateDCodeSelectBox();
    void unitsChangeRefresh() override;      // See class EDA_DRAW_FRAME

    void OnClearJobFileHistory( wxCommandEvent& aEvent );
    void OnClearZipFileHistory( wxCommandEvent& aEvent );
    void OnClearDrlFileHistory( wxCommandEvent& aEvent );
    void OnClearGbrFileHistory( wxCommandEvent& aEvent );

    void DoWithAcceptedFiles() override;

    /**
     * Loads the file provided or shows a dialog to get the file(s) from the user.
     *
     * @param aFileName Name of the file to open. Shows a dialog if not specified.
     * @param dialogFiletypes File extensions to pass to the dialog if necessary
     * @param dialogTitle Dialog title to use if necessary
     * @param filetype Type of file for parsing, 0 = gerber, 1 = drill, 2 = autodetect
     *
     * @return true if success opening all files, false otherwise
     */
    bool LoadFileOrShowDialog( const wxString& aFileName, const wxString& dialogFiletypes,
                               const wxString& dialogTitle, const int filetype );

    // The Tool Framework initialization
    void setupTools();

public:
    wxChoice* m_SelComponentBox;                // a choice box to display and highlight component
                                                // graphic items
    wxChoice* m_SelNetnameBox;                  // a choice box to display and highlight netlist
                                                // graphic items
    wxChoice* m_SelAperAttributesBox;           // a choice box to display aperture attributes and
                                                // highlight items
    GBR_LAYER_BOX_SELECTOR* m_SelLayerBox;      // The combobox to select the current active
                                                // graphic layer
                                                // (which is drawn on top on the other layers
    DCODE_SELECTION_BOX*    m_DCodeSelector;    // a list box to select the dcode Id to highlight.
    wxTextCtrl*             m_TextInfo;         // a wxTextCtrl used to display some info about
                                                // gerber data (format..)

    GERBER_LAYER_WIDGET*    m_LayersManager;

protected:
    FILE_HISTORY            m_zipFileHistory;
    FILE_HISTORY            m_drillFileHistory;
    FILE_HISTORY            m_jobFileHistory;

    wxString                m_lastFileName;     // The last filename chosen to be proposed to the
                                                // user.

private:
    bool                m_show_layer_manager_tools;

    GBR_LAYOUT*         m_gerberLayout;
    int                 m_activeLayer;
    VECTOR2I            m_grid_origin;
    PAGE_INFO           m_paper;            // used only to show paper limits to screen
    wxStaticText*       m_cmpText;          // a message on the auxiliary toolbar,
                                            // relative to the m_SelComponentBox
    wxStaticText*       m_netText;          // a message on the auxiliary toolbar,
                                            // relative to the m_SelNetnameBox
    wxStaticText*       m_apertText;        // a message on the auxiliary toolbar,
                                            // relative to the m_SelAperAttributesBox
    wxStaticText*       m_dcodeText;        // a message on the auxiliary toolbar,
                                            // relative to the m_DCodeSelector

#ifndef __linux__
    std::unique_ptr<NL_GERBVIEW_PLUGIN> m_spaceMouse;
#else
    std::unique_ptr<SPNAV_2D_PLUGIN> m_spaceMouse;
#endif
};

#endif /* WX_GERBER_STRUCT_H */
