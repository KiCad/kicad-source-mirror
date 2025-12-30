/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
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

/**
 * @file eda_base_frame.h
 * @brief Base window classes and related definitions.
 */

#ifndef  EDA_BASE_FRAME_H_
#define  EDA_BASE_FRAME_H_


#include <map>
#include <optional>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include <wx/aui/aui.h>
#include <layer_ids.h>
#include <frame_type.h>
#include <hotkeys_basic.h>
#include <kiway_holder.h>
#include <tool/action_toolbar.h>
#include <tool/tools_holder.h>
#include <widgets/ui_common.h>
#include <widgets/wx_infobar.h>
#include <undo_redo_container.h>
#include <units_provider.h>
#include <origin_transforms.h>
#include <ui_events.h>

// Option for main frames
#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS


#define VIEWER3D_FRAMENAME wxT( "Viewer3DFrameName" )
#define QUALIFIED_VIEWER3D_FRAMENAME( parent ) \
                    ( wxString( VIEWER3D_FRAMENAME ) + wxT( ":" ) + parent->GetName() )

#define KICAD_MANAGER_FRAME_NAME   wxT( "KicadFrame" )


class wxChoice;
class wxEvent;
class wxFileName;
class EDA_ITEM;
class EDA_DRAW_PANEL_GAL;
class EDA_MSG_PANEL;
class BASE_SCREEN;
class PAGE_INFO;
class PLOTTER;
class TITLE_BLOCK;
class MSG_PANEL_ITEM;
class TOOL_MANAGER;
class TOOL_DISPATCHER;
class ACTIONS;
class PAGED_DIALOG;
class DIALOG_EDIT_LIBRARY_TABLES;
class PANEL_HOTKEYS_EDITOR;
class FILE_HISTORY;
class SETTINGS_MANAGER;
class SEARCH_STACK;
class APP_SETTINGS_BASE;
class APPEARANCE_CONTROLS_3D;
struct WINDOW_SETTINGS;
struct WINDOW_STATE;
class ACTION_MENU;
class TOOL_INTERACTIVE;
class TOOLBAR_SETTINGS;

#define DEFAULT_MAX_UNDO_ITEMS 0
#define ABS_MAX_UNDO_ITEMS (INT_MAX / 2)

/// This is the handler functor for the update UI events
typedef std::function< void( wxUpdateUIEvent& ) > UIUpdateHandler;


/**
 * The base frame for deriving all KiCad main window classes.
 *
 * This class is not intended to be used directly.  It provides support for automatic calls
 * to SaveSettings() function.  SaveSettings() for a derived class can choose to do nothing,
 * or rely on basic SaveSettings() support in this base class to do most of the work by
 * calling it from the derived class's SaveSettings().  This class is not a #KIWAY_PLAYER
 * because #KICAD_MANAGER_FRAME is derived from it and that class is not a player.
 */
class EDA_BASE_FRAME : public wxFrame, public TOOLS_HOLDER, public KIWAY_HOLDER,
                       public UNITS_PROVIDER
{
public:
    /**
     * Specify whether we are interacting with the undo or redo stacks.
     */
    enum UNDO_REDO_LIST
    {
        UNDO_LIST,
        REDO_LIST
    };

    EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType, const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize, long aStyle,
                    const wxString& aFrameName, KIWAY* aKiway, const EDA_IU_SCALE& aIuScale );

    ~EDA_BASE_FRAME();

    void ChangeUserUnits( EDA_UNITS aUnits );

    virtual void ToggleUserUnits() { }

    /**
     * Return a reference to the default ORIGIN_TRANSFORMS object
     */
    ORIGIN_TRANSFORMS& GetOriginTransforms() override
    {
        return m_originTransforms;
    }


    SETTINGS_MANAGER* GetSettingsManager() const { return m_settingsManager; }

    virtual SEVERITY GetSeverity( int aErrorCode ) const { return RPT_SEVERITY_UNDEFINED; }

    /**
     * Override the default process event handler to implement the auto save feature.
     *
     * @warning If you override this function in a derived class, make sure you call down to
     *          this or the auto save feature will be disabled.
     */
    bool ProcessEvent( wxEvent& aEvent ) override;

    /**
     * Capture the key event before it is sent to the GUI.
     *
     * The basic frame does not capture this event.  Editor frames should override this event
     * function to capture and filter these keys when they are used as hotkeys, and skip it if
     * the key is not used as hotkey (otherwise the key events will be not sent to menus).
     */
    virtual void OnCharHook( wxKeyEvent& aKeyEvent );

    /**
     * The #TOOL_DISPATCHER needs these to work around some issues in wxWidgets where the menu
     * events aren't captured by the menus themselves.
     */
    void OnMenuEvent( wxMenuEvent& event );

    /**
     * Register a UI update handler for the control with ID @c aID
     *
     * @param aID is the control ID to register the handler for
     * @param aConditions are the UI conditions to use for the control states
     */
    virtual void RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions ) override;

    /**
     * Unregister a UI handler for a given ID that was registered using @c RegisterUIUpdateHandler
     *
     * @param aID is the control ID to unregister the handler for
     */
    virtual void UnregisterUIUpdateHandler( int aID ) override;

    /**
     * Handle events generated when the UI is trying to figure out the current state of the
     * UI controls related to #TOOL_ACTIONS (e.g. enabled, checked, etc.).
     *
     * @param aEvent is the wxUpdateUIEvent to be processed.
     * @param aFrame is the frame to get the selection from
     * @param aCond are the #UI SELECTION_CONDITIONS used
     */
    static void HandleUpdateUIEvent( wxUpdateUIEvent& aEvent, EDA_BASE_FRAME* aFrame,
                                     ACTION_CONDITIONS aCond );

    virtual void OnMove( wxMoveEvent& aEvent )
    {
        aEvent.Skip();
    }

    virtual void OnSize( wxSizeEvent& aEvent );

    /**
     * Select the given action in the toolbar group which contains it, if any.
     * This updates the displayed icon/tooltip and UI conditions for that group.
     */
    void SelectToolbarAction( const TOOL_ACTION& aAction )
    {
        if( m_tbLeft )
            m_tbLeft->SelectAction( aAction );

        if( m_tbTopMain )
            m_tbTopMain->SelectAction( aAction );

        if( m_tbTopAux )
            m_tbTopAux->SelectAction( aAction );

        if( m_tbRight )
            m_tbRight->SelectAction( aAction );
    }

    void OnMaximize( wxMaximizeEvent& aEvent );

    int GetAutoSaveInterval() const;

    bool IsType( FRAME_T aType ) const { return m_ident == aType; }
    FRAME_T GetFrameType() const { return m_ident; }

    /**
     * Return a #SEARCH_STACK pertaining to entire program.
     *
     * This is overloaded in #KICAD_MANAGER_FRAME
     */
    virtual const SEARCH_STACK& sys_search();

    virtual wxString help_name();

    // Event handlers for menu events generated by the macOS application menu
    void OnKicadAbout( wxCommandEvent& event );
    void OnPreferences( wxCommandEvent& event );

    /**
     * Display the preferences and settings of all opened editors paged dialog, starting with
     * a particular page
     */
    void ShowPreferences( wxString aStartPage, wxString aStartParentPage );

    void PrintMsg( const wxString& text );

    void CreateInfoBar();

    void RestoreAuiLayout();

    void FinishAUIInitialization();

    /**
     * @return the #WX_INFOBAR that can be displayed on the top of the canvas.
     */
    WX_INFOBAR* GetInfoBar() { return m_infoBar; }

    /**
     * Show the #WX_INFOBAR displayed on the top of the canvas with a message and an error
     * icon on the left of the infobar, and an optional closebox to the right.
     *
     * The infobar will be closed after a timeout.
     *
     * @param aErrorMsg is the message to display.
     * @param aShowCloseButton true to show a close button on the right of the #WX_INFOBAR.
     */
    void ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton = false,
                           WX_INFOBAR::MESSAGE_TYPE aType = WX_INFOBAR::MESSAGE_TYPE::GENERIC );

    /**
     * Show the #WX_INFOBAR displayed on the top of the canvas with a message and an error
     * icon on the left of the infobar, and an optional closebox to the right.
     *
     * The infobar will be closed after a timeout.
     *
     * This version accepts a callback which will be called when the infobar is dismissed
     * (either as a result of user action or a timeout).  This can be useful when the caller
     * wants to make other decorations in the canvas to highlight the error.
     *
     * @param aErrorMsg is the message to display.
     * @param aShowCloseButton true to show a close button on the right of the #WX_INFOBAR.
     * @param aCallback a callback to be called when the infobar is dismissed.
     */
    void ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                           std::function<void(void)> aCallback );

    /**
     * Show the #WX_INFOBAR displayed on the top of the canvas with a message and a warning
     * icon on the left of the infobar.
     *
     * The infobar will be closed after a timeout.
     *
     * @param aErrorMsg is the message to display.
     * @param aShowCloseButton true to show a close button on the right of the #WX_INFOBAR.
     */
    void ShowInfoBarWarning( const wxString& aWarningMsg, bool aShowCloseButton = false );

    /**
     * Show the #WX_INFOBAR displayed on the top of the canvas with a message and an info
     * icon on the left of the infobar.
     *
     * The infobar will be closed after a timeout.
     *
     * @param aErrorMsg is the message to display.
     * @param aShowCloseButton true to show a close button on the right of the #WX_INFOBAR.
     */
    void ShowInfoBarMsg( const wxString& aMsg, bool aShowCloseButton = false );

    /**
     * Return the settings object used in SaveSettings(), and is overloaded in
     * #KICAD_MANAGER_FRAME.
     */
    virtual APP_SETTINGS_BASE* config() const;

    void LoadWindowState( const wxString& aFileName );

    /**
     * Load window settings from the given settings object.
     *
     * Normally called by #LoadSettings() unless the window in question is a child window
     * that* stores its settings somewhere other than #APP_SETTINGS_BASE::m_Window.
     */
    void LoadWindowSettings( const WINDOW_SETTINGS* aCfg );

    /**
     * Save window settings to the given settings object.
     *
     * Normally called by #SaveSettings unless the window in question is a child window that
     * stores its settings somewhere other than #APP_SETTINGS_BASE::m_Window.
     */
    void SaveWindowSettings( WINDOW_SETTINGS* aCfg );

    /**
     * Load common frame parameters from a configuration file.
     *
     * Don't forget to call the base method or your frames won't remember their positions
     * and sizes.
     */
    virtual void LoadSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Save common frame parameters to a configuration data file.
     *
     * Don't forget to call the base class's SaveSettings() from your derived
     * #SaveSettings() otherwise the frames won't remember their positions and sizes.
     */
    virtual void SaveSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Return a pointer to the window settings for this frame.
     *
     * By default, points to aCfg->m_Window for top-level frames.
     *
     * @param aCfg is this frame's config object
     */
    virtual WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Load frame state info from a configuration file.
     */
    virtual void LoadWindowState( const WINDOW_STATE& aState );

    /**
     * Get the configuration base name.
     *
     * This is usually the name of the frame set by CTOR, except for frames shown in
     * multiple modes in which case the m_configName must be set to the base name so
     * that a single configuration can be used.
     *
     * @return a base name prefix used in Load/Save settings to build the full name of keys
     *         used in configuration.
     */
    wxString ConfigBaseName() override
    {
        wxString baseCfgName = m_configName.IsEmpty() ? GetName() : m_configName;
        return baseCfgName;
    }

    /**
     * Save changes to the project local settings.
     *
     * These settings are used to save/restore the view state for a specific project, and
     * should never contain design data.  This method is normally called automatically at
     * various points in the workflow so that the user's most recent display settings are
     * automatically persisted.
     *
     * The method is virtual so you can override it to call the suitable save method.
     * The base method does nothing.
     */
    virtual void SaveProjectLocalSettings() {};

    /**
     * Prompt the user for a hotkey file to read, and read it.
     *
     * @param aActionMap current hotkey map (over which the imported hotkeys will be applied).
     * @param aDefaultShortname a default short name (extension not needed) like
     *                          Eeschema, KiCad...
     */
    void ImportHotkeyConfigFromFile( std::map<std::string, TOOL_ACTION*> aActionMap,
                                     const wxString& aDefaultShortname );

    /**
     * Fetch the file name from the file history list.
     *
     * This removes the selected file, if this file does not exist.  The menu is also updated,
     * if #FILE_HISTORY::UseMenu was called at initialization time.
     *
     * @param cmdId The command ID associated with the \a aFileHistory object.
     * @param type Please document me!
     * @param aFileHistory The FILE_HISTORY in use. If null, the main application file
     *                     history is used
     * @return a wxString containing the selected filename
     */
    wxString GetFileFromHistory( int cmdId, const wxString& type,
                                 FILE_HISTORY* aFileHistory = nullptr );

    /**
     * Remove all files from the file history.
     */
    virtual void ClearFileHistory();

    /**
     * Update the list of recently opened files.
     *
     * The menu is also updated, if FILE_HISTORY::UseMenu was called at init time.
     *
     * @param FullFileName The full file name including the path.
     * @param aFileHistory The FILE_HISTORY in use.  If NULL, the main application file
     *                     history is used.
     */
    void UpdateFileHistory( const wxString& FullFileName, FILE_HISTORY* aFileHistory = nullptr );

    /**
     * Get the frame's main file history.
     *
     * @return the main file history
     */
    FILE_HISTORY& GetFileHistory()
    {
        return *m_fileHistory;
    }

    void SetMruPath( const wxString& aPath ) { m_mruPath = aPath; }

    wxString GetMruPath() const { return m_mruPath; }

    /**
     * Get the full filename + path of the currently opened file in the frame.
     *
     * If no file is open, an empty string is returned.
     *
     * @return the filename and full path to the open file
     */
    virtual wxString GetCurrentFileName() const { return wxEmptyString; }

    virtual void RecreateToolbars();

    /**
     * Update toolbars if desired toolbar icon changed.
     */
    void OnToolbarSizeChanged();

    /**
     * Update the sizes of any controls in the toolbars of the frame.
     */
    virtual void UpdateToolbarControlSizes();

    /**
     * Register a creation factory for toolbar controls that are present in this frame.
     *
     * The factory function takes a single argument of type `ACTION_TOOLBAR*`, which is the toolbar
     * to add the controls to.
     *
     * @param aControlDesc is the control descriptor
     * @param aControlFactory A functor that creates the custom controls and then adds them to the toolbar
     */
    void RegisterCustomToolbarControlFactory( const ACTION_TOOLBAR_CONTROL& aControlDesc,
                                              const ACTION_TOOLBAR_CONTROL_FACTORY& aControlFactory );

    /**
     *
     */
    ACTION_TOOLBAR_CONTROL_FACTORY* GetCustomToolbarControlFactory( const std::string& aName );

    /**
     * Recreate the menu bar.
     *
     * Needed when the language or icons are changed
     */
    void ReCreateMenuBar();

    /**
     * Add the standard KiCad help menu to the menubar.
     */
    void AddStandardHelpMenu( wxMenuBar* aMenuBar );

    wxString GetRunMenuCommandDescription( const TOOL_ACTION& aAction );

    /**
     * Check if \a aFileName can be written.
     *
     * The function performs a number of tests on \a aFileName to verify that it can be saved.
     * If \a aFileName defines a path with no file name, them the path is tested for user write
     * permission.  If \a aFileName defines a file name that does not exist in the path, the
     * path is tested for user write permission.  If \a aFileName defines a file that already
     * exits, the file name is tested for user write permissions.
     *>
     * @note The file name path must be set or an assertion will be raised on debug builds and
     *       return false on release builds.
     * @param aFileName The full path and/or file name of the file to test.
     * @param aVerbose If true will show an error dialog if the file is not writable
     * @return False if \a aFileName cannot be written.
     */
    bool IsWritable( const wxFileName& aFileName, bool aVerbose = true );

    /**
     * Update the status bar information.
     *
     * The status bar can draw itself.  This is not a drawing function per se, but rather
     * updates lines of text held by the components within the status bar which is owned
     * by the wxFrame.
     */
    virtual void UpdateStatusBar() { }

    /**
     * Redraw the menus and what not in current language.
     */
    void ShowChangedLanguage() override;

    /**
     * Notification event that some of the common (suite-wide) settings have changed.
     * Update menus, toolbars, local variables, etc.
     */
    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Process light/dark theme change.
     */
    virtual void ThemeChanged();

    /**
     * Notification event that the project has changed.
     */
    virtual void ProjectChanged() {}

    const wxString& GetAboutTitle() const { return wxGetTranslation( m_aboutTitle ); }

    const wxString& GetUntranslatedAboutTitle() const { return m_aboutTitle; }

    /**
     * Get if the contents of the frame have been modified since the last save.
     *
     * @return true if the contents of the frame have not been saved
     */
    virtual bool IsContentModified() const;

    /**
     * Get the undecorated window size that can be used for restoring the window size.
     *
     * This is needed for GTK, since the normal wxWidgets GetSize() call will return
     * a window size that includes the window decorations added by the window manager.
     *
     * @return the undecorated window size
     */
    wxSize GetWindowSize();

    /**
     * Remove the \a aItemCount of old commands from \a aList and delete commands, pickers
     * and picked items if needed.
     *
     * Because picked items must be deleted only if they are not in use, this is a virtual
     * pure function that must be created for #SCH_SCREEN and #PCB_SCREEN.  Commands are
     * deleted from the older to the last.
     *
     * @param aList = the #UNDO_REDO_CONTAINER of commands.
     * @param aItemCount number of old commands to delete. -1 to remove all old commands
     *                   this will empty the list of commands.
     */
    virtual void ClearUndoORRedoList( UNDO_REDO_LIST aList, int aItemCount = -1 )
    { }

    /**
     * Clear the undo and redo list using #ClearUndoORRedoList()
     *
     * Picked items are deleted by ClearUndoORRedoList() according to their status.
     */
    virtual void ClearUndoRedoList();

    /**
     * Add a command to undo in the undo list.
     *
     * Delete the very old commands when the max count of undo commands is reached.
     */
    virtual void PushCommandToUndoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Add a command to redo in the redo list.
     *
     * Delete the very old commands when the max count of redo commands is reached.
     */
    virtual void PushCommandToRedoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Return the last command to undo and remove it from list, nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromUndoList();

    /**
     * Return the last command to undo and remove it from list, nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromRedoList();

    virtual int GetUndoCommandCount() const { return m_undoList.m_CommandsList.size(); }
    virtual int GetRedoCommandCount() const { return m_redoList.m_CommandsList.size(); }

    virtual wxString GetUndoActionDescription() const;
    virtual wxString GetRedoActionDescription() const;

    int GetMaxUndoItems() const { return m_undoRedoCountMax; }

    /**
     * Must be called after a model change in order to set the "modify" flag and do other
     * frame-specific processing.
     */
    virtual void OnModify();

    bool IsClosing() const { return m_isClosing; }

    bool NonUserClose( bool aForce )
    {
        m_isNonUserClose = true;
        return Close( aForce );
    }

    virtual void ClearToolbarControl( int aId ) { }

    /**
     * Update the UI in response to a change in the system colors.
     */
    virtual void HandleSystemColorChange();

    /**
     * Check if this frame is ready to accept API commands.
     *
     * A frame might not accept commands if a long-running process is underway, a dialog is open,
     * the user is interacting with a tool, etc.
     */
    virtual bool CanAcceptApiCommands() { return IsEnabled(); }

protected:
    /// Default style flags used for wxAUI toolbars.
    static constexpr int KICAD_AUI_TB_STYLE = wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_PLAIN_BACKGROUND;

    virtual void doReCreateMenuBar() {}

    virtual void configureToolbars();

    /**
     * Handle the auto save timer event.
     */
    void onAutoSaveTimer( wxTimerEvent& aEvent );


    /**
     * Handle a window iconize event.
     *
     * @param aEvent is the data for the event.
     */
    virtual void handleIconizeEvent( wxIconizeEvent& aEvent ) {}
    void         onIconize( wxIconizeEvent& aEvent );

    /**
     * Return the auto save status of the application.
     *
     * Override this function if your derived frame supports automatic file saving.
     */
    virtual bool isAutoSaveRequired() const { return m_autoSaveRequired; }

    /**
     * This should be overridden by the derived class to handle the auto save feature.
     *
     * @return true if the auto save was successful otherwise false.
     */
    virtual bool doAutoSave();

    virtual bool canCloseWindow( wxCloseEvent& aCloseEvent ) { return true; }
    virtual void doCloseWindow() { }

    void onSystemColorChange( wxSysColourChangedEvent& aEvent );

    /**
     * Called when when the units setting has changed to allow for any derived classes
     * to handle refreshing and controls that have units based measurements in them.
     *
     * The default version only updates the status bar.  Don't forget to call the default
     * in your derived class or the status bar will not get updated properly.
     */
    virtual void unitsChangeRefresh() { }

    /**
     * Setup the UI conditions for the various actions and their controls in this frame.
     */
    virtual void setupUIConditions();

    /**
     * Set the common key-pair for exiting the application (Ctrl-Q) and ties it
     * to the wxID_EXIT event id.
     *
     * This is useful in sub-applications to pass the event up to a non-owning window.
     */
    void initExitKey();

    void ensureWindowIsOnScreen();

    /**
     * Save any design-related project settings associated with this frame.
     *
     * This method should only be called as the result of direct user action, for example from an
     * explicit "Save Project" command or as a consequence of saving a design document.
     */
    virtual void saveProjectSettings() {}

    /**
     * Handle event fired when a file is dropped to the window.
     *
     * In this base class, stores the path of files accepted.
     * Calls DoWithAcceptedFiles() to execute actions on files.
     */
    virtual void OnDropFiles( wxDropFilesEvent& aEvent );

    /**
     * Create a menu list for language choice, and add it as submenu to \a MasterMenu.
     *
     * @param aMasterMenu is the main menu.
     * @param aControlTool is the tool to associate with the menu.
     */
    void AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool );

    /**
     * Execute action on accepted dropped file.
     *
     * Called in OnDropFiles() and should be populated with
     * the action to execute in inherited classes.
     */
    virtual void            DoWithAcceptedFiles();
    std::vector<wxFileName> m_AcceptedFiles;

    DECLARE_EVENT_TABLE()

private:
    /**
     * (with its unexpected name so it does not collide with the real OnWindowClose()
     * function provided in derived classes) is called just before a window
     * closing, and is used to call a derivation specific SaveSettings().
     *
     * #SaveSettings() is called for all derived wxFrames in this base class overload.
     * Calling it from a destructor is deprecated since the wxFrame's position is not
     * available in the destructor on linux.  In other words, you should not need to
     * call #SaveSettings() anywhere, except in this one function found only in this class.
     */
    void windowClosing( wxCloseEvent& event );

    /**
     * Collect common initialization functions used in all CTORs
     */
    void commonInit( FRAME_T aFrameType );

    wxWindow* findQuasiModalDialog();

    /**
     * Return true if the frame is shown in our modal mode and false  if the frame is
     * shown as an usual frame.
     *
     * In modal mode, the caller that created the frame is responsible to Destroy()
     * this frame after closing.
     */
    virtual bool IsModal() const { return false; }

#ifdef __WXMSW__
    /**
     * Windows specific override of the wxWidgets message handler for a window.
     */
    WXLRESULT MSWWindowProc( WXUINT message, WXWPARAM wParam, WXLPARAM lParam ) override;
#endif

 protected:
    FRAME_T         m_ident;                // Id Type (pcb, schematic, library..)
    wxPoint         m_framePos;
    wxSize          m_frameSize;
    bool            m_maximizeByDefault;
    int             m_displayIndex;

    // These contain the frame size and position for when it is not maximized
    wxPoint         m_normalFramePos;
    wxSize          m_normalFrameSize;

    wxString                m_aboutTitle;        // Name of program displayed in About.

    wxAuiManager            m_auimgr;
    wxString                m_perspective;       // wxAuiManager perspective.
    nlohmann::json          m_auiLayoutState;
    WX_INFOBAR*             m_infoBar;           // Infobar for the frame
    APPEARANCE_CONTROLS_3D* m_appearancePanel;
    wxString                m_configName;        // Prefix used to identify some params (frame
                                                 // size) and to name some config files (legacy
                                                 // hotkey files)
    SETTINGS_MANAGER*       m_settingsManager;

    FILE_HISTORY*           m_fileHistory;       // The frame's recently opened file list
    bool                    m_supportsAutoSave;
    bool                    m_autoSavePending;
    bool                    m_autoSaveRequired;
    wxTimer*                m_autoSaveTimer;
    bool                    m_autoSavePermissionError;

    int                     m_undoRedoCountMax;  // undo/Redo command Max depth

    UNDO_REDO_CONTAINER     m_undoList;          // Objects list for the undo command (old data)
    UNDO_REDO_CONTAINER     m_redoList;          // Objects list for the redo command (old data)

    wxString                m_mruPath;           // Most recently used path.

    ORIGIN_TRANSFORMS       m_originTransforms;  // Default display origin transforms object.

    /// Map containing the UI update handlers registered with wx for each action.
    std::map<int, UIUpdateHandler> m_uiUpdateMap;

    /// Set by the close window event handler after frames are asked if they can close.
    /// Allows other functions when called to know our state is cleanup.
    bool            m_isClosing;

    /// Set by #NonUserClose() to indicate that the user did not request the current close.
    bool            m_isNonUserClose;

    /**
     * Associate file extensions with action to execute.
     */
    std::map<const wxString, TOOL_ACTION*> m_acceptedExts;

    // Toolbar Settings - this is not owned by the frame
    TOOLBAR_SETTINGS*    m_toolbarSettings;

    // Toolbar UI elements
    ACTION_TOOLBAR*      m_tbTopMain;
    ACTION_TOOLBAR*      m_tbTopAux;  // Additional tools under main toolbar
    ACTION_TOOLBAR*      m_tbRight;       // Drawing tools (typically on right edge of window)
    ACTION_TOOLBAR*      m_tbLeft;    // Options (typically on left edge of window)

    std::map<std::string, ACTION_TOOLBAR_CONTROL_FACTORY> m_toolbarControlFactories;
};


/**
 * Specialization of the wxAuiPaneInfo class for KiCad panels.
 *
 * Documentation for wxAui is poor at this time. The following notes spring from errors made in
 * previous KiCad implementations:
 *
 * wxAuiPaneInfo.ToolbarPane() and .Defaults() are used to clear and then prepare the objects so
 * only use them once at the beginning of configuration..
 *
 * Panels are organized in layers, from 0 (close to the center) and increasing outward. Note
 * that for ToolbarPanes, layer 0 considered a special default value, and ToolbarPanes on
 * layer 0 are pushed to layer 10 automatically. Use Layer 1 for the inner layer as a work-
 * around.
 *
 * Each panel has rows, starting at 0. Each row has positions starting at 0. Each item in a panel
 * can have its row and position set.
 *
 * Eventually panels will be movable. Each initialization function sets up the panel for this,
 * then after a //==// break has additional calls to anchor toolbars in a way that matches
 * present functionality.
 */
class EDA_PANE : public wxAuiPaneInfo
{
public:
    EDA_PANE()
    {
        Gripper( false );
        CloseButton( false );
        PaneBorder( false );
    }

    /**
     * Turn *this to a horizontal toolbar for KiCad.
     */
    EDA_PANE& HToolbar()
    {
        SetFlag( optionToolbar, true );
        CaptionVisible( false );
        TopDockable().BottomDockable();
        DockFixed( true );
        Movable( false );
        Resizable( true );      // expand to fit available space
        return *this;
    }

    /**
     * Turn *this into a vertical toolbar for KiCad.
     */
    EDA_PANE& VToolbar()
    {
        SetFlag( optionToolbar, true );
        CaptionVisible( false );
        LeftDockable().RightDockable();
        DockFixed( true );
        Movable( false );
        Resizable( true );      // expand to fit available space
        return *this;
    }

    /**
     * Turn *this into a captioned palette suitable for a symbol tree, layers manager, etc.
     */
    EDA_PANE& Palette()
    {
        CaptionVisible( true );
        PaneBorder( true );
        return *this;
    }

    /**
     * Turn *this into an undecorated pane suitable for a drawing canvas.
     */
    EDA_PANE& Canvas()
    {
        CaptionVisible( false );
        Layer( 0 );
        PaneBorder( true );
        Resizable( true );      // expand to fit available space
        return *this;
    }

    /**
     * Turn *this into a messages pane for KiCad.
     */
    EDA_PANE& Messages()
    {
        CaptionVisible( false );
        BottomDockable( true );
        DockFixed( true );
        Movable( false );
        Resizable( true );      // expand to fit available space
        return *this;
    }

    /**
     * Turn *this into a infobar for KiCad.
     */
    EDA_PANE& InfoBar()
    {
        CaptionVisible( false );
        Movable( false );
        Resizable( true );
        PaneBorder( false );
        DockFixed( true );
        return *this;
    }
};

#endif  // EDA_BASE_FRAME_H_
