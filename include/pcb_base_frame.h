/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  PCB_BASE_FRAME_H
#define  PCB_BASE_FRAME_H

#include <eda_units.h>
#include <eda_draw_frame.h>
#include <lib_id.h>
#include <pcb_display_options.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_origin_transforms.h>
#include <pcb_screen.h>
#include <vector>

#include <wx/datetime.h>
#include <wx/timer.h>

/* Forward declarations of classes. */
class APP_SETTINGS_BASE;
class BOARD;
class BOARD_CONNECTED_ITEM;
class COLOR_SETTINGS;
class EDA_ITEM;
class FOOTPRINT;
class PAD;
class EDA_3D_VIEWER_FRAME;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class BOARD_DESIGN_SETTINGS;
class LSET;
class ZONE_SETTINGS;
class PCB_PLOT_PARAMS;
class FP_LIB_TABLE;
class PCB_VIEWERS_SETTINGS_BASE;
class PCBNEW_SETTINGS;
class FOOTPRINT_EDITOR_SETTINGS;
struct MAGNETIC_SETTINGS;
class PROGRESS_REPORTER;
class PCB_LAYER_BOX_SELECTOR;

#ifndef __linux__
class NL_PCBNEW_PLUGIN;
#else
class SPNAV_2D_PLUGIN;
#endif

#ifdef wxHAS_INOTIFY
#define wxFileSystemWatcher wxInotifyFileSystemWatcher
#elif defined( wxHAS_KQUEUE ) && defined( wxHAVE_FSEVENTS_FILE_NOTIFICATIONS )
#define wxFileSystemWatcher wxFsEventsFileSystemWatcher
#elif defined( wxHAS_KQUEUE )
#define wxFileSystemWatcher wxKqueueFileSystemWatcher
#elif defined( __WINDOWS__ )
#define wxFileSystemWatcher wxMSWFileSystemWatcher
#else
#define wxFileSystemWatcher wxPollingFileSystemWatcher
#endif

class wxFileSystemWatcher;
class wxFileSystemWatcherEvent;

wxDECLARE_EVENT( EDA_EVT_BOARD_CHANGED, wxCommandEvent );

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
     * @param aTitle is the new title of the 3D frame, or nullptr to do not change the frame title
     */
    virtual void Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle = nullptr );

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
    BOX2I GetBoardBoundingBox( bool aBoardEdgesOnly = false ) const;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) override;
    const PAGE_INFO& GetPageSettings() const override;
    const VECTOR2I GetPageSizeIU() const override;

    const VECTOR2I& GetGridOrigin() const override;
    void            SetGridOrigin( const VECTOR2I& aPoint ) override;

    const VECTOR2I& GetAuxOrigin() const;

    const VECTOR2I GetUserOrigin() const;

    /**
     * Return a reference to the default #ORIGIN_TRANSFORMS object.
     */
    ORIGIN_TRANSFORMS& GetOriginTransforms() override;

    wxString MessageTextFromCoord( int aValue, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) const
    {
        return MessageTextFromValue( m_originTransforms.ToDisplay( aValue, aCoordType ) );
    }

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    /**
     * Return the BOARD_DESIGN_SETTINGS for the open project.
     */
    virtual BOARD_DESIGN_SETTINGS& GetDesignSettings() const;

    /**
     * Helper to retrieve the current color settings.
     */
    virtual COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override
    {
        wxFAIL_MSG( wxT( "Color settings requested for a PCB_BASE_FRAME that does not "
                         "override!" ) );
        return nullptr;
    }

    void SetDrawBgColor( const COLOR4D& aColor ) override;

    /**
     * Display options control the way tracks, vias, outlines and other things are shown
     * (for instance solid or sketch mode).
     */
    const PCB_DISPLAY_OPTIONS& GetDisplayOptions() const { return m_displayOptions; }

    /**
     * Update the current display options.
     *
     * @param aOptions is the options struct to apply.
     * @param aRefresh will refresh the view after updating.
     */
    void SetDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions, bool aRefresh = true );

    /**
     * Return the #PCB_PLOT_PARAMS for the BOARD owned by this frame.
     */
    virtual const PCB_PLOT_PARAMS& GetPlotSettings() const;
    virtual void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings );

    /**
     * Reload the footprint from the library.
     *
     * @param aFootprint is the footprint to reload.
     */
    virtual void ReloadFootprint( FOOTPRINT* aFootprint )
    {
        wxFAIL_MSG( wxT( "Attempted to reload a footprint for PCB_BASE_FRAME that does not "
                         "override!" ) );
    }

    /**
     * Set the #m_Pcb member in such as way as to ensure deleting any previous #BOARD.
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

    EDA_ITEM* ResolveItem( const KIID& aId, bool aAllowNullptrReturn = false ) const override;

    void FocusOnItem( EDA_ITEM* aItem ) override;
    void FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );
    void FocusOnItems( std::vector<BOARD_ITEM*> aItems, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    void HideSolderMask();
    void ShowSolderMask();

    // General
    virtual void ShowChangedLanguage() override;
    virtual void UpdateStatusBar() override;

    PCB_SCREEN* GetScreen() const override { return (PCB_SCREEN*) EDA_DRAW_FRAME::GetScreen(); }

    /**
     * Show the 3D view frame.
     *
     * If it does not exist, it is created.  If it exists, it is brought to the foreground.
     */
    EDA_3D_VIEWER_FRAME* CreateAndShow3D_Frame();

    /**
     * @return global configuration options.
     */
    GENERAL_COLLECTORS_GUIDE GetCollectorsGuide();

    /**
     * Must be called after a change in order to set the "modify" flag and update other data
     * structures and GUI elements.
     */
    void OnModify() override;

    /**
     * Create a new footprint at position 0,0.
     *
     * The new footprint contains only 2 texts: a reference and a value:
     *  Reference = REF**
     *  Value = "VAL**" or Footprint name in lib
     *
     * @note They are dummy texts which will be replaced by the actual texts when the
     *       footprint is placed on a board and a netlist is read.
     *
     * @param aFootprintName is the name of the new footprint in library.
     * @param aLibName optional, if specified is the library for the new footprint
     */
    FOOTPRINT* CreateNewFootprint( wxString aFootprintName, const wxString& aLibName );

    /**
     * Place \a aFootprint at the current cursor position and updates footprint coordinates
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
    FOOTPRINT* SelectFootprintFromLibrary( LIB_ID aPreselect = LIB_ID() );

    /**
     * Add the given footprint to the board.
     *
     * @param aDC is the current Device Context, to draw the new footprint (can be NULL ).
     */
    virtual void AddFootprintToBoard( FOOTPRINT* aFootprint );

    /**
     * Create the entire board ratsnest.
     *
     * This must be called after a board change (changes for pads, footprints or a read
     * netlist ).
     *
     * @param aDisplayStatus  if true, display the computation results.
     */
    void Compile_Ratsnest( bool aDisplayStatus );

    /**
     * Create a new entry in undo list of commands.
     *
     * @param aItemToCopy is the board item modified by the command to undo.
     * @param aTypeCommand is the command type (see enum #UNDO_REDO).
     */
    virtual void SaveCopyInUndoList( EDA_ITEM* aItemToCopy, UNDO_REDO aTypeCommand ) {};

    /**
     * Create a new entry in undo list of commands.
     *
     * @param aItemsList is the list of items modified by the command to undo.
     * @param aTypeCommand is the command type (see enum #UNDO_REDO)
     */
    virtual void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO aTypeCommand ) {};

    /**
     * As SaveCopyInUndoList, but appends the changes to the last undo item on the stack.
     */
    virtual void AppendCopyToUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                       UNDO_REDO aTypeCommand ) {};


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
    PCB_LAYER_ID SelectOneLayer( PCB_LAYER_ID aDefaultLayer,
                                 const LSET& aNotAllowedLayersMask = LSET(),
                                 wxPoint aDlgPosition = wxDefaultPosition );

    /**
     * Change the active layer in the frame.
     *
     * @param aLayer New layer to make active.
     */
    virtual void SwitchLayer( PCB_LAYER_ID aLayer );

    virtual void SetActiveLayer( PCB_LAYER_ID aLayer ) { GetScreen()->m_Active_Layer = aLayer; }
    virtual PCB_LAYER_ID GetActiveLayer() const { return GetScreen()->m_Active_Layer; }

    SEVERITY GetSeverity( int aErrorCode ) const override;

    virtual void OnDisplayOptionsChanged() {}

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    PCBNEW_SETTINGS* GetPcbNewSettings() const;

    FOOTPRINT_EDITOR_SETTINGS* GetFootprintEditorSettings() const;

    virtual PCB_VIEWERS_SETTINGS_BASE* GetViewerSettingsBase() const;

    virtual MAGNETIC_SETTINGS* GetMagneticItemsSettings();

    void CommonSettingsChanged( int aFlags ) override;

    PCB_DRAW_PANEL_GAL* GetCanvas() const override;

    virtual void ActivateGalCanvas() override;

    /**
     * Add \a aListener to post #EDA_EVT_BOARD_CHANGED command events to.
     *
     * @warning The caller is responsible for removing any listeners that are no long valid.
     *
     * @note This only gets called when the board editor is in stand alone mode.  Changing
     *       projects in the project manager closes the board editor when a new project is
     *       loaded.
     */
    void AddBoardChangeListener( wxEvtHandler* aListener );

    /**
     * Remove \a aListener to from the board changed listener list.
     */
    void RemoveBoardChangeListener( wxEvtHandler* aListener );

    /**
     * Handler for FP change events.  Responds to the filesystem watcher set in #setFPWatcher.
    */
    void OnFPChange( wxFileSystemWatcherEvent& aEvent );

    /**
     * Handler for the filesystem watcher debounce timer.
     */
    void OnFpChangeDebounceTimer( wxTimerEvent& aEvent );

    void GetLibraryItemsForListDialog( wxArrayString& aHeaders,
                                       std::vector<wxArrayString>& aItemsToDisplay );

protected:
    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;

    void handleActivateEvent( wxActivateEvent& aEvent ) override;

    void handleIconizeEvent( wxIconizeEvent& aEvent ) override;

    virtual void doReCreateMenuBar() override;

    /**
     * Attempt to load \a aFootprintId from the footprint library table.
     *
     * @param aFootprintId is the #LIB_ID of component footprint to load.
     * @return the #FOOTPRINT if found or NULL if \a aFootprintId not found in any of the
     *         libraries in the table returned from #Prj().PcbFootprintLibs().
     * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
     *                 occurs while reading footprint library files.
     */
    FOOTPRINT* loadFootprint( const LIB_ID& aFootprintId );

    virtual void unitsChangeRefresh() override;

    void rebuildConnectivity();

    /**
     * Create or removes a watcher on the specified footprint.
     *
     * @param aFootprint If nullptr, the watcher is removed.  Otherwise, set a change watcher
     */
    void setFPWatcher( FOOTPRINT* aFootprint );

protected:
    BOARD*                  m_pcb;
    PCB_DISPLAY_OPTIONS     m_displayOptions;
    PCB_ORIGIN_TRANSFORMS   m_originTransforms;

private:
#ifndef __linux__
    std::unique_ptr<NL_PCBNEW_PLUGIN>    m_spaceMouse;
#else
    std::unique_ptr<SPNAV_2D_PLUGIN>    m_spaceMouse;
#endif

    std::unique_ptr<wxFileSystemWatcher> m_watcher;
    wxFileName                           m_watcherFileName;
    wxDateTime                           m_watcherLastModified;
    wxTimer                              m_watcherDebounceTimer;
    bool                                 m_inFpChangeTimerEvent;

    std::vector<wxEvtHandler*> m_boardChangeListeners;
};

#endif  // PCB_BASE_FRAME_H
