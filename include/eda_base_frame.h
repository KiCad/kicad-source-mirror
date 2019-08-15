/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  WXSTRUCT_H_
#define  WXSTRUCT_H_


#include <vector>

#include <wx/socket.h>
#include <wx/log.h>
#include <wx/config.h>
#include <wx/wxhtml.h>
#include <wx/laywin.h>
#include <wx/aui/aui.h>
#include <wx/docview.h>
#include <fctsys.h>
#include <common.h>
#include <layers_id_colors_and_visibility.h>
#include <frame_type.h>
#include <hotkeys_basic.h>
#include <kiway_holder.h>

// Option for main frames
#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS


#define KICAD_MANAGER_FRAME_NAME   wxT( "KicadFrame" )


// Readability helper definitions for creating backup files.
#define CREATE_BACKUP_FILE    true
#define NO_BACKUP_FILE        false

class EDA_ITEM;
class EDA_RECT;
class EDA_DRAW_PANEL_GAL;
class EDA_MSG_PANEL;
class BASE_SCREEN;
class PARAM_CFG_BASE;
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


enum id_librarytype {
    LIBRARY_TYPE_EESCHEMA,
    LIBRARY_TYPE_PCBNEW,
    LIBRARY_TYPE_DOC,
    LIBRARY_TYPE_SYMBOL
};


/**
 * The base frame for deriving all KiCad main window classes.
 *
 * This class is not intended to be used directly.  It provides support for automatic calls
 * to SaveSettings() function.  SaveSettings() for a derived class can choose to do nothing,
 * or rely on basic SaveSettings() support in this base class to do most of the work by
 * calling it from the derived class's SaveSettings().
 * <p>
 * This class is not a KIWAY_PLAYER because KICAD_MANAGER_FRAME is derived from it
 * and that class is not a player.
 * </p>
 */
class EDA_BASE_FRAME : public wxFrame, public KIWAY_HOLDER
{
    /**
     * (with its unexpected name so it does not collide with the real OnWindowClose()
     * function provided in derived classes) is called just before a window
     * closing, and is used to call a derivation specific
     * SaveSettings().  SaveSettings() is called for all derived wxFrames in this
     * base class overload.  (Calling it from a destructor is deprecated since the
     * wxFrame's position is not available in the destructor on linux.)  In other words,
     * you should not need to call SaveSettings() anywhere, except in this
     * one function found only in this class.
     */
    void windowClosing( wxCloseEvent& event );

    wxWindow* findQuasiModalDialog();

protected:
    FRAME_T         m_Ident;                // Id Type (pcb, schematic, library..)
    wxPoint         m_FramePos;
    wxSize          m_FrameSize;

    wxString        m_AboutTitle;           // Name of program displayed in About.

    wxAuiManager    m_auimgr;

    wxString        m_configName;           // Prefix used to identify some params (frame size...)
                                            // and to name some config files (legacy hotkey files)

    TOOL_MANAGER*   m_toolManager;
    ACTIONS*        m_actions;

    std::vector<std::string> m_toolStack;   // Stack of user-level "tools".  Not to be confused
                                            // with TOOL_BASE-derived instances, many of which
                                            // implement multiple user-level "tools".  The user-
                                            // level "tools" are TOOL_ACTIONSs internally.

    bool            m_immediateActions;     // Preference for immediate actions.  If false, the
                                            // first invocation of a hotkey will just select the
                                            // relevant tool.
    bool            m_dragSelects;          // Prefer selection to dragging.
    bool            m_moveWarpsCursor;      // cursor is warped to move/drag origin
    bool            m_hasAutoSave;
    bool            m_autoSaveState;
    int             m_autoSaveInterval;     // The auto save interval time in seconds.
    wxTimer*        m_autoSaveTimer;

    wxString        m_perspective;          // wxAuiManager perspective.

    wxString        m_mruPath;              // Most recently used path.

    EDA_UNITS_T     m_userUnits;

    ///> Default style flags used for wxAUI toolbars
    static constexpr int KICAD_AUI_TB_STYLE = wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_PLAIN_BACKGROUND;

    /**
     * @return the suffix to be appended to the file extension on backup
     */
    static wxString GetBackupSuffix()
    {
        return wxT( "-bak" );
    }

    /**
     * @return the string to prepend to a file name for automatic save.
     */
    static wxString GetAutoSaveFilePrefix()
    {
        return wxT( "_autosave-" );
    }

    /**
     * Handle the auto save timer event.
     */
    void onAutoSaveTimer( wxTimerEvent& aEvent );

    /**
     * Return the auto save status of the application.
     *
     * Override this function if your derived frame supports automatic file saving.
     */
    virtual bool isAutoSaveRequired() const { return false; }

    /**
     * This should be overridden by the derived class to handle the auto save feature.
     *
     * @return true if the auto save was successful otherwise false.
     */
    virtual bool doAutoSave();

    /**
     * Called when when the units setting has changed to allow for any derived classes
     * to handle refreshing and controls that have units based measurements in them.  The
     * default version only updates the status bar.  Don't forget to call the default
     * in your derived class or the status bar will not get updated properly.
     */
    virtual void unitsChangeRefresh() { }

    DECLARE_EVENT_TABLE()

public:
    EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aFrameName, KIWAY* aKiway );

    ~EDA_BASE_FRAME();

    /**
     * Return the user units currently in use.
     */
    EDA_UNITS_T GetUserUnits() const { return m_userUnits; }
    void SetUserUnits( EDA_UNITS_T aUnits ) { m_userUnits = aUnits; }

    void ChangeUserUnits( EDA_UNITS_T aUnits )
    {
        SetUserUnits( aUnits );
        unitsChangeRefresh();
    }

    /**
     * Return the MVC controller.
     */
    TOOL_MANAGER* GetToolManager() const { return m_toolManager; }

    /**
     * NB: the definition of "tool" is different at the user level.  The implementation uses
     * a single TOOL_BASE derived class to implement several user "tools", such as rectangle
     * and circle, or wire and bus.  So each user-level tool is actually a TOOL_ACTION.
     */
    virtual void PushTool( const std::string& actionName );
    virtual void PopTool( const std::string& actionName );

    bool ToolStackIsEmpty() { return m_toolStack.empty(); }

    std::string CurrentToolName() const;
    bool IsCurrentTool( const TOOL_ACTION& aAction ) const;

    virtual void DisplayToolMsg( const wxString& msg ) {};

    /**
     * Indicates that hotkeys should perform an immediate action even if another tool is
     * currently active.  If false, the first hotkey should select the relevant tool.
     */
    bool GetDoImmediateActions() const { return m_immediateActions; }

    /**
     * Indicates that a drag should draw a selection rectangle, even when started over an
     * item.
     */
    bool GetDragSelects() const { return m_dragSelects; }

    /**
     * Indicates that a move operation should warp the mouse pointer to the origin of the
     * move object.  This improves snapping, but some users are alergic to mouse warping.
     */
    bool GetMoveWarpsCursor() const { return m_moveWarpsCursor; }

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
    virtual void OnCharHook( wxKeyEvent& event );

    /**
     * Workaround some issues in wxWidgets where the menu events aren't captured by the
     * menus themselves.
     */
    void OnMenuOpen( wxMenuEvent& event );

    void SetAutoSaveInterval( int aInterval );

    int GetAutoSaveInterval() const { return m_autoSaveInterval; }

    bool IsType( FRAME_T aType ) const { return m_Ident == aType; }

    /**
     * Return a SEARCH_STACK pertaining to entire program.
     *
     * This is overloaded in #KICAD_MANAGER_FRAME
     */
    virtual const SEARCH_STACK& sys_search();

    virtual wxString help_name();

    void OnKicadAbout( wxCommandEvent& event );
    void OnPreferences( wxCommandEvent& event );

    void PrintMsg( const wxString& text );

    /**
     * Returns the wxConfigBase used in SaveSettings(), and is overloaded in
     * KICAD_MANAGER_FRAME
     */
    virtual wxConfigBase* config();

    /**
     * Function InstallPreferences
     * Allow a frame to load its preference panels (if any) into the preferences dialog.
     * @param aParent a paged dialog into which the preference panels should be installed
     */
    virtual void InstallPreferences( PAGED_DIALOG* , PANEL_HOTKEYS_EDITOR* ) { }

    /**
     * Load common frame parameters from a configuration file.
     *
     * Don't forget to call the base method or your frames won't
     * remember their positions and sizes.
     */
    virtual void LoadSettings( wxConfigBase* aCfg );

    /**
     * Saves common frame parameters to a configuration data file.
     *
     * Don't forget to call the base class's SaveSettings() from
     * your derived SaveSettings() otherwise the frames won't remember their
     * positions and sizes.
     */
    virtual void SaveSettings( wxConfigBase* aCfg );

    /**
     * @return a base name prefix used in Load/Save settings to build the full name of keys 
     * used in config.
     * This is usually the name of the frame set by CTOR, except for frames shown in multiple
     * modes in which case the m_configName must be set to the base name so that a single
     * config can be used.
     */
    wxString ConfigBaseName()
    {
        wxString baseCfgName = m_configName.IsEmpty() ? GetName() : m_configName;
        return baseCfgName;
    }

    /**
     * Save changes to the project settings to the project (.pro) file.
     *
     * The method is virtual so you can override it to call the suitable save method.
     * The base method do nothing
     * @param aAskForSave = true to open a dialog before saving the settings
     */
    virtual void SaveProjectSettings( bool aAskForSave ) {};

    // Read/Save and Import/export hotkeys config

    /**
     * Prompt the user for a hotkey file to read, and read it.
     *
     * @param aActionMap = current hotkey map (over which the imported hotkeys will be applied)
     * @param aDefaultShortname = a default short name (extension not needed)
     *     like eechema, kicad...
     */
    void ImportHotkeyConfigFromFile( std::map<std::string, TOOL_ACTION*> aActionMap,
                                     const wxString& aDefaultShortname );

    /**
     * Fetches the file name from the file history list.
     *
     * This removes the selected file, if this file does not exist.  The menu is also updated,
     * if wxFileHistory::UseMenu was called at init time
     *
     * @param cmdId The command ID associated with the \a aFileHistory object.
     * @param type Please document me!
     * @param aFileHistory The wxFileHistory in use. If null, the main application file
     *                     history is used
     * @return a wxString containing the selected filename
     */
    wxString GetFileFromHistory( int cmdId, const wxString& type,
                                 wxFileHistory* aFileHistory = NULL );

    /**
     * Update the list of recently opened files.
     *
     * The menu is also updated, if wxFileHistory::UseMenu was called at init time.
     *
     * @param FullFileName The full file name including the path.
     * @param aFileHistory The wxFileHistory in use.
     * If NULL, the main application file history is used.
     */
    void UpdateFileHistory( const wxString& FullFileName, wxFileHistory * aFileHistory = NULL );

    void SetMruPath( const wxString& aPath ) { m_mruPath = aPath; }

    wxString GetMruPath() const { return m_mruPath; }

    /**
     * Recreates the menu bar.
     *
     * Needed when the language is changed
     */
    virtual void ReCreateMenuBar();

    /**
     * Adds the standard KiCad help menu to the menubar.
     */
    void AddStandardHelpMenu( wxMenuBar* aMenuBar );

    /**
     * Checks if \a aFileName can be written.
     * <p>
     * The function performs a number of tests on \a aFileName to verify that it can be saved.
     * If \a aFileName defines a path with no file name, them the path is tested for user write
     * permission.  If \a aFileName defines a file name that does not exist in the path, the
     * path is tested for user write permission.  If \a aFileName defines a file that already
     * exits, the file name is tested for user write permissions.
     * </p>
     * @note The file name path must be set or an assertion will be raised on debug builds and
     *       return false on release builds.
     * @param aFileName The full path and/or file name of the file to test.
     * @return False if \a aFileName cannot be written.
     */
    bool IsWritable( const wxFileName& aFileName );

    /**
     * Check if an auto save file exists for \a aFileName and takes the appropriate action
     * depending on the user input.
     * <p>
     * If an auto save file exists for \a aFileName, the user is prompted if they wish to
     * replace file \a aFileName with the auto saved file.  If the user chooses to replace the
     * file, the backup file of \a aFileName is removed, \a aFileName is renamed to the backup
     * file name, and the auto save file is renamed to \a aFileName.  If user chooses to keep
     * the existing version of \a aFileName, the auto save file is removed.
     * </p>
     * @param aFileName A wxFileName object containing the file name to check.
     */
    void CheckForAutoSaveFile( const wxFileName& aFileName );

    /**
     * Update the status bar information.
     *
     * The status bar can draw itself.  This is not a drawing function per se, but rather 
     * updates lines of text held by the components within the status bar which is owned 
     * by the wxFrame.
     */
    virtual void UpdateStatusBar() { }

    /**
     * Update the toolbars (mostly settings/check buttons/checkboxes) with the current 
     * controller state.
     */
    virtual void SyncToolbars() { };

    /**
     * Redraw the menus and what not in current language.
     */
    virtual void ShowChangedLanguage();

    /**
     * Notification event that some of the common (suite-wide) settings have changed.
     * Update menus, toolbars, local variables, etc.
     */
    virtual void CommonSettingsChanged( bool aEnvVarsChanged );
    
    /**
     * Notification to refresh the drawing canvas (if any).
     */
    virtual void RefreshCanvas() { };

    const wxString& GetAboutTitle() const { return m_AboutTitle; }
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
 * can have it's row and position set.
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
};

#endif  // WXSTRUCT_H_
