/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcb_base_frame.h
 * @brief Classes used in Pcbnew, CvPcb and GerbView.
 */

#ifndef  PCB_BASE_FRAME_H
#define  PCB_BASE_FRAME_H


#include <eda_item.h>
#include <board.h>
#include <eda_draw_frame.h>
#include <outline_mode.h>
#include <lib_id.h>
#include <pcb_display_options.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_origin_transforms.h>
#include <pcb_screen.h>
#include <richio.h>
#include <vector>


/* Forward declarations of classes. */
class APP_SETTINGS_BASE;
class BOARD;
class BOARD_CONNECTED_ITEM;
class COLOR_SETTINGS;
class FOOTPRINT;
class PAD;
class EDA_3D_VIEWER_FRAME;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class BOARD_DESIGN_SETTINGS;
class ZONE_SETTINGS;
class PCB_PLOT_PARAMS;
class FP_LIB_TABLE;
class PCBNEW_SETTINGS;
class FOOTPRINT_EDITOR_SETTINGS;
struct MAGNETIC_SETTINGS;
class PROGRESS_REPORTER;


wxDECLARE_EVENT( BOARD_CHANGED, wxCommandEvent );

/**
 * Base PCB main window class for Pcbnew, Gerbview, and CvPcb footprint viewer.
 */
class PCB_BASE_FRAME : public EDA_DRAW_FRAME
{
public:
    PCB_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                    const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                    long aStyle, const wxString& aFrameName );

    ~PCB_BASE_FRAME();

    /**
     * @return a reference to the child 3D viewer frame, when exists, or NULL
     */
    EDA_3D_VIEWER_FRAME* Get3DViewerFrame();

    /**
     * Update the 3D view, if the viewer is opened by this frame.
     *
     * @param aMarkDirty alerts the 3D view that data is stale (it may not refresh instantly)
     * @param aRefresh will tell the 3D view to refresh immediately
     * @param aTitle is the new title of the 3D frame, or nullptr to do not change the
     *               frame title
     */
    virtual void Update3DView( bool aMarkDirty, bool aRefresh,
                               const wxString* aTitle = nullptr );

    /**
     * Attempt to load \a aFootprintId from the footprint library table.
     *
     * @param aFootprintId is the #LIB_ID of component footprint to load.
     * @return the #FOOTPRINT if found or NULL if \a aFootprintId not found in any of the
     *         libraries in table returned from #Prj().PcbFootprintLibs().
     */
    FOOTPRINT* LoadFootprint( const LIB_ID& aFootprintId );

    /**
     * Calculate the bounding box containing all board items (or board edge segments).
     *
     * @param aBoardEdgesOnly is true if we are interested in board edge segments only.
     * @return the board's bounding box.
     */
    EDA_RECT GetBoardBoundingBox( bool aBoardEdgesOnly = false ) const;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override
    {
        /* "Zoom to Fit" calls this with "aIncludeAllVisible" as true.  Since that feature
         * always ignored the page and border, this function returns a bbox without them
         * as well when passed true.  This technically is not all things visible, but it
         * keeps behavior consistent.
         *
         * When passed false, this function returns a bbox of just the board edge. This
         * allows things like fabrication text or anything else outside the board edge to
         * be ignored, and just zooms up to the board itself.
         *
         * Calling "GetBoardBoundingBox(true)" when edge cuts are turned off will return
         * the entire page and border, so we call "GetBoardBoundingBox(false)" instead.
         */
        if( aIncludeAllVisible || !m_pcb->IsLayerVisible( Edge_Cuts ) )
            return GetBoardBoundingBox( false );
        else
            return GetBoardBoundingBox( true );
    }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) override;
    const PAGE_INFO& GetPageSettings() const override;
    const wxSize GetPageSizeIU() const override;

    const wxPoint& GetGridOrigin() const override;
    void SetGridOrigin( const wxPoint& aPoint ) override;

    const wxPoint& GetAuxOrigin() const;

    const wxPoint GetUserOrigin() const;

    /**
     * Return a reference to the default ORIGIN_TRANSFORMS object
     */
    ORIGIN_TRANSFORMS& GetOriginTransforms() override;

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    /**
     * Returns the BOARD_DESIGN_SETTINGS for the open project.
     *
     * Overloaded in FOOTPRINT_EDIT_FRAME.
     */
    virtual BOARD_DESIGN_SETTINGS& GetDesignSettings() const;

    /**
     * Helper to retrieve the current color settings.
     *
     * @return a pointer to the active COLOR_SETTINGS.
     */
    virtual COLOR_SETTINGS* GetColorSettings() const override
    {
        wxFAIL_MSG( "Color settings requested for a PCB_BASE_FRAME that does not override!" );
        return nullptr;
    }

    PCBNEW_SETTINGS& Settings() { return *m_settings; }

    void SetDrawBgColor( const COLOR4D& aColor ) override;

    /**
     * Display options control the way tracks, vias, outlines and other things are shown
     * (for instance solid or sketch mode).
     */
    const PCB_DISPLAY_OPTIONS& GetDisplayOptions() const { return m_displayOptions; }

    /**
     * Updates the current display options from the given options struct
     * @param aOptions is the options struct to apply
     * @param aRefresh will refresh the view after updating
     */
    void SetDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions, bool aRefresh = true );

    const ZONE_SETTINGS& GetZoneSettings() const;
    void SetZoneSettings( const ZONE_SETTINGS& aSettings );

    /**
     * Return the #PCB_PLOT_PARAMS for the BOARD owned by this frame.
     *
     * Overloaded in FOOTPRINT_EDIT_FRAME.
     */
    virtual const PCB_PLOT_PARAMS& GetPlotSettings() const;
    virtual void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings );

    /**
     * Set the #m_Pcb member in such as way as to ensure deleting any previous #BOARD.
     *
     * @param aBoard is the #BOARD to put into the frame.
     */
    virtual void SetBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );

    BOARD* GetBoard() const
    {
        wxASSERT( m_pcb );
        return m_pcb;
    }

    /**
     * @return the primary data model.
     */
    virtual BOARD_ITEM_CONTAINER* GetModel() const = 0;

    EDA_ITEM* GetItem( const KIID& aId ) const override;

    void FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    // General
    virtual void ReCreateOptToolbar() override { }
    virtual void ShowChangedLanguage() override;
    virtual void ReCreateMenuBar() override;
    virtual void UpdateStatusBar() override;

    PCB_SCREEN* GetScreen() const override { return (PCB_SCREEN*) EDA_DRAW_FRAME::GetScreen(); }

    /**
     * Shows the 3D view frame.
     *
     * If it does not exist, it is created.  If it exists, it is brought to the foreground.
     */
    EDA_3D_VIEWER_FRAME* CreateAndShow3D_Frame();

    /**
     * @return global configuration options.
     */
    GENERAL_COLLECTORS_GUIDE GetCollectorsGuide();

    /**
     * Put up a dialog and allows the user to pick a library, for unspecified use.
     *
     * @param aNicknameExisting is the current choice to highlight.
     * @return the library or wxEmptyString on abort.
     */
    wxString SelectLibrary( const wxString& aNicknameExisting );

    /**
     * @return a reference to the footprint found by its reference on the current board. The
     *         reference is entered by the user from a dialog (by awxTextCtlr, or a list of
     *         available references)
     */
    FOOTPRINT* GetFootprintFromBoardByReference();

    /**
     * Must be called after a change in order to set the "modify" flag of the current screen
     * and update the date in frame reference.
     *
     * Do not forget to call this basic OnModify function to update info in derived OnModify
     * functions.
     */
    virtual void OnModify();

    /**
     * Creates a new footprint, at position 0,0.
     *
     * The new footprint contains only 2 texts: a reference and a value:
     *  Reference = REF**
     *  Value = "VAL**" or Footprint name in lib
     *
     * @note They are dummy texts, which will be replaced by the actual texts when the
     *       footprint is placed on a board and a netlist is read.
     *
     * @param aFootprintName is the name of the new footprint in library.
     * @param aQuiet prevents user dialogs from being shown
     */
    FOOTPRINT* CreateNewFootprint( const wxString& aFootprintName, bool aQuiet = false );

    /**
     * Places \a aFootprint at the current cursor position and updates footprint coordinates
     * with the new position.
     *
     * @param aRecreateRatsnest A bool true redraws the footprint ratsnest.
     */
    void PlaceFootprint( FOOTPRINT* aFootprint, bool aRecreateRatsnest = true );

    void ShowPadPropertiesDialog( PAD* aPad );

    /**
     * Open a dialog to select a footprint.
     *
     * @param aPreslect if valid, the #LIB_ID to select (otherwise the global history is used).
     */
    FOOTPRINT* SelectFootprintFromLibTree( LIB_ID aPreselect = LIB_ID() );

    /**
     * Add the given footprint to the board.
     *
     * @param aDC is the current Device Context, to draw the new footprint (can be NULL ).
     */
    virtual void AddFootprintToBoard( FOOTPRINT* aFootprint );

    /**
     * Launch the footprint viewer to select the name of a footprint to load.
     *
     * @return the selected footprint name or an empty string if no selection was made.
     */
    wxString SelectFootprintFromLibBrowser();

    /**
     * Create the entire board ratsnest.
     *
     * This must be called after a board change (changes for pads, footprints or a read
     * netlist ).
     *
     * @param aDC is the current device context (can be NULL).
     * @param aDisplayStatus  if true, display the computation results.
     */
    void Compile_Ratsnest( bool aDisplayStatus );

    /**
     * Create a new entry in undo list of commands.
     *
     * @param aItemToCopy is the board item modified by the command to undo.
     * @param aTypeCommand is the command type (see enum #UNDO_REDO).
     */
    virtual void SaveCopyInUndoList( EDA_ITEM* aItemToCopy, UNDO_REDO aTypeCommand ) = 0;

    /**
     * Creates a new entry in undo list of commands.
     *
     * @param aItemsList is the list of items modified by the command to undo.
     * @param aTypeCommand is the command type (see enum #UNDO_REDO)
     */
    virtual void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO aTypeCommand ) = 0;


    /**
     * Show the dialog box for a layer selection.
     *
     * @param aDefaultLayer is the default layer to select.  Use UNDEFINED_LAYER if no selection
     *                      is desired.
     * @param aNotAllowedLayersMask is a layer mask for not allowed layers.  Use 0 to show all
     *                              layers in use.
     * @param aDlgPosition is the position of dialog (default is centered).
     * @return the selected layer id.
     */
    PCB_LAYER_ID SelectOneLayer( PCB_LAYER_ID aDefaultLayer, LSET aNotAllowedLayersMask = LSET(),
                              wxPoint aDlgPosition = wxDefaultPosition );

    virtual void SwitchLayer( wxDC* DC, PCB_LAYER_ID layer );

    virtual void SetActiveLayer( PCB_LAYER_ID aLayer ) { GetScreen()->m_Active_Layer = aLayer; }
    virtual PCB_LAYER_ID GetActiveLayer() const { return GetScreen()->m_Active_Layer; }

    SEVERITY GetSeverity( int aErrorCode ) const override;

    virtual void OnDisplayOptionsChanged() {}

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    PCBNEW_SETTINGS* GetPcbNewSettings() const;

    FOOTPRINT_EDITOR_SETTINGS* GetFootprintEditorSettings() const;

    virtual MAGNETIC_SETTINGS* GetMagneticItemsSettings();

    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    /**
     * Display the current grid pane on the status bar.
     */
    void DisplayGridMsg() override;

    PCB_DRAW_PANEL_GAL* GetCanvas() const override;

    virtual void ActivateGalCanvas() override;

protected:
    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;

    /**
     * Attempts to load \a aFootprintId from the footprint library table.
     *
     * @param aFootprintId is the #LIB_ID of component footprint to load.
     * @return the #FOOTPRINT if found or NULL if \a aFootprintId not found in any of the
     *         libraries in the table returned from #Prj().PcbFootprintLibs().
     * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
     *                 occurs while reading footprint library files.
     */
    FOOTPRINT* loadFootprint( const LIB_ID& aFootprintId );

    virtual void unitsChangeRefresh() override;

protected:
    BOARD*                  m_pcb;
    PCB_DISPLAY_OPTIONS     m_displayOptions;
    PCB_ORIGIN_TRANSFORMS   m_originTransforms;
    PCBNEW_SETTINGS*        m_settings; // No ownership, just a shortcut
};

#endif  // PCB_BASE_FRAME_H
