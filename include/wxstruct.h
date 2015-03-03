/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011-2015 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file wxstruct.h
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

#include <colors.h>
#include <fctsys.h>
#include <common.h>
#include <layers_id_colors_and_visibility.h>
#include <frame_type.h>

#ifdef USE_WX_OVERLAY
#include <wx/overlay.h>
#endif

// Option for main frames
#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS


// Readability helper definitions for creating backup files.
#define CREATE_BACKUP_FILE    true
#define NO_BACKUP_FILE        false


class EDA_ITEM;
class EDA_RECT;
class EDA_DRAW_PANEL;
class EDA_DRAW_PANEL_GAL;
class EDA_MSG_PANEL;
class BASE_SCREEN;
class PARAM_CFG_BASE;
class PAGE_INFO;
class PLOTTER;
class TITLE_BLOCK;
class MSG_PANEL_ITEM;


enum id_librarytype {
    LIBRARY_TYPE_EESCHEMA,
    LIBRARY_TYPE_PCBNEW,
    LIBRARY_TYPE_DOC,
    LIBRARY_TYPE_SYMBOL
};


/// Custom trace mask to enable and disable auto save tracing.
extern const wxChar traceAutoSave[];


/**
 * Class EDA_BASE_FRAME
 * is the base frame for deriving all KiCad main window classes.  This class is not
 * intended to be used directly.  It provides support for automatic calls to
 * a SaveSettings() function.  SaveSettings() for a derived class can choose
 * to do nothing, or rely on basic SaveSettings() support in this base class to do
 * most of the work by calling it from the derived class's SaveSettings().
 * <p>
 * This class is not a KIWAY_PLAYER because KICAD_MANAGER_FRAME is derived from it
 * and that class is not a player.
 */
class EDA_BASE_FRAME : public wxFrame
{
    /**
     * Function windowClosing
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

protected:
    FRAME_T      m_Ident;           ///< Id Type (pcb, schematic, library..)
    wxPoint      m_FramePos;
    wxSize       m_FrameSize;

    wxAuiToolBar* m_mainToolBar;    ///< Standard horizontal Toolbar
    wxString     m_FrameName;       ///< name used for writing and reading setup
                                    ///< It is "SchematicFrame", "PcbFrame" ....
    wxString     m_AboutTitle;      ///< Name of program displayed in About.

    wxAuiManager m_auimgr;

    /// Flag to indicate if this frame supports auto save.
    bool         m_hasAutoSave;

    /// Flag to indicate the last auto save state.
    bool         m_autoSaveState;

    /// The auto save interval time in seconds.
    int          m_autoSaveInterval;

    /// The timer used to implement the auto save feature;
    wxTimer*     m_autoSaveTimer;

    wxString     m_perspective;     ///< wxAuiManager perspective.

    /**
     * Function onAutoSaveTimer
     * handles the auto save timer event.
     */
    void onAutoSaveTimer( wxTimerEvent& aEvent );

    /**
     * Function autoSaveRequired
     * returns the auto save status of the application.  Override this function if
     * your derived frame supports automatic file saving.
     */
    virtual bool isAutoSaveRequired() const { return false; }

    /**
     * Function doAutoSave
     * should be overridden by the derived class to handle the auto save feature.
     *
     * @return true if the auto save was successful otherwise false.
     */
    virtual bool doAutoSave();

    /**
     * Function config
     * returns the wxConfigBase used in SaveSettings(), and is overloaded in
     * KICAD_MANAGER_FRAME
     */
    virtual wxConfigBase* config();

    /**
     * Function sys_search
     * returns a SEARCH_STACK pertaining to entire program, and is overloaded in
     * KICAD_MANAGER_FRAME
     */
    virtual const SEARCH_STACK& sys_search();

    virtual wxString help_name();

public:
    EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aFrameName );

    ~EDA_BASE_FRAME();

    /**
     * Function ProcessEvent
     * overrides the default process event handler to implement the auto save feature.
     *
     * @warning If you override this function in a derived class, make sure you call
     *          down to this or the auto save feature will be disabled.
     */
    bool ProcessEvent( wxEvent& aEvent );       // override wxFrame::ProcessEvent()

    bool Enable( bool enable );                 // override wxFrame::Enable virtual

    void SetAutoSaveInterval( int aInterval ) { m_autoSaveInterval = aInterval; }

    int GetAutoSaveInterval() const { return m_autoSaveInterval; }

    wxString GetName() const { return m_FrameName; }

    bool IsType( FRAME_T aType ) const { return m_Ident == aType; }

    void GetKicadHelp( wxCommandEvent& event );

    void GetKicadAbout( wxCommandEvent& event );

    /**
     * Function CopyVersionInfoToClipboard
     * copies the version information to the clipboard for bug reporting purposes.
     */
    void CopyVersionInfoToClipboard( wxCommandEvent& event );

    void PrintMsg( const wxString& text );

    /**
     * Append the copy version information to clipboard help menu entry to \a aMenu.
     *
     * @param aMenu - The menu to append.
     */
    void AddHelpVersionInfoMenuEntry( wxMenu* aMenu );

    /**
     * Function LoadSettings
     * loads common frame parameters from a configuration file.
     *
     * Don't forget to call the base method or your frames won't
     * remember their positions and sizes.
     */
    virtual void LoadSettings( wxConfigBase* aCfg );

    /**
     * Function SaveSettings
     * saves common frame parameters to a configuration data file.
     *
     * Don't forget to call the base class's SaveSettings() from
     * your derived SaveSettings() otherwise the frames won't remember their
     * positions and sizes.
     */
    virtual void SaveSettings( wxConfigBase* aCfg );

    /**
     * Function SaveProjectSettings
     * saves changes to the project settings to the project (.pro) file.
     * The method is virtual so you can override it to call the suitable save method.
     * The base method do nothing
     * @param aAskForSave = true to open a dialog before saving the settings
     */
    virtual void SaveProjectSettings( bool aAskForSave ) {};

    /**
     * Function OnSelectPreferredEditor
     * Open a dialog to select the editor that will used in KiCad
     * to edit or display files (reports ... )
     * The full filename editor is saved in configuration (global params)
     */
    virtual void OnSelectPreferredEditor( wxCommandEvent& event );

    // Read/Save and Import/export hotkeys config

    /**
     * Function ReadHotkeyConfig
     * Read configuration data and fill the current hotkey list with hotkeys
     * @param aDescList = current hotkey list descr. to initialize.
     */
    int ReadHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList );

    /**
     * Function WriteHotkeyConfig
     * Store the current hotkey list
     * It is stored using the standard wxConfig mechanism or a file.
     *
     * @param aDescList = pointer to the current hotkey list.
     * @param aFullFileName = a wxString pointer to a full file name.
     *  if NULL, use the standard wxConfig mechanism (default)
     * the output format is: shortcut  "key"  "function"
     * lines starting with # are comments
     */
    int WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList, wxString* aFullFileName = NULL);

    /**
     * Function ReadHotkeyConfigFile
     * Read an old configuration file (&ltfile&gt.key) and fill the current hotkey list
     * with hotkeys
     * @param aFilename = file name to read.
     * @param aDescList = current hotkey list descr. to initialize.
     */
    int ReadHotkeyConfigFile( const wxString& aFilename, struct EDA_HOTKEY_CONFIG* aDescList );

    /**
     * Function ImportHotkeyConfigFromFile
     * Prompt the user for an old hotkey file to read, and read it.
     * @param aDescList = current hotkey list descr. to initialize.
     * @param aDefaultShortname = a default short name (extention not needed)
     *     like eechema, kicad...
     */
    void ImportHotkeyConfigFromFile( EDA_HOTKEY_CONFIG* aDescList,
                                     const wxString& aDefaultShortname );

    /**
     * Function ExportHotkeyConfigToFile
     * Prompt the user for an old hotkey file to read, and read it.
     * @param aDescList = current hotkey list descr. to initialize.
     * @param aDefaultShortname = a default short name (extention not needed)
     *     like eechema, kicad...
     */
    void ExportHotkeyConfigToFile( EDA_HOTKEY_CONFIG* aDescList,
                                   const wxString& aDefaultShortname );

    /**
     * Function GetFileFromHistory
     * fetches the file name from the file history list.
     * and removes the selected file, if this file does not exists
     * Note also the menu is updated, if wxFileHistory::UseMenu
     * was called at init time
     * @param cmdId The command ID associated with the \a aFileHistory object.
     * @param type Please document me!
     * @param aFileHistory The wxFileHistory in use. If null, the main application file
     *                     history is used
     * @return a wxString containing the selected filename
     */
    wxString GetFileFromHistory( int cmdId, const wxString& type,
                                 wxFileHistory* aFileHistory = NULL );

    /**
     * Function UpdateFileHistory
     * Updates the list of recently opened files.
     * Note also the menu is updated, if wxFileHistory::UseMenu
     * was called at init time
     * @param FullFileName The full file name including the path.
     * @param aFileHistory The wxFileHistory in use.
     * If NULL, the main application file history is used.
     */
    void UpdateFileHistory( const wxString& FullFileName, wxFileHistory * aFileHistory = NULL );

    /**
     * Function ReCreateMenuBar
     * Creates recreates the menu bar.
     * Needed when the language is changed
     */
    virtual void ReCreateMenuBar();

    /**
     * Function IsWritable
     * checks if \a aFileName can be written.
     * <p>
     * The function performs a number of tests on \a aFileName to verify that it
     * can be saved.  If \a aFileName defines a path with no file name, them the
     * path is tested for user write permission.  If \a aFileName defines a file
     * name that does not exist in the path, the path is tested for user write
     * permission.  If \a aFileName defines a file that already exits, the file
     * name is tested for user write permissions.
     * </p>
     *
     * @note The file name path must be set or an assertion will be raised on debug
     *       builds and return false on release builds.
     * @param aFileName The full path and/or file name of the file to test.
     * @return False if \a aFileName cannot be written.
     */
    bool IsWritable( const wxFileName& aFileName );

    /**
     * Function CheckForAutoSaveFile
     * checks if an auto save file exists for \a aFileName and takes the appropriate
     * action depending on the user input.
     * <p>
     * If an auto save file exists for \a aFileName, the user is prompted if they wish
     * to replace file \a aFileName with the auto saved file.  If the user chooses to
     * replace the file, the backup file of \a aFileName is removed, \a aFileName is
     * renamed to the backup file name, and the auto save file is renamed to \a aFileName.
     * If user chooses to keep the existing version of \a aFileName, the auto save file
     * is removed.
     * </p>
     * @param aFileName A wxFileName object containing the file name to check.
     * @param aBackupFileExtension A wxString object containing the backup file extension
     *                             used to create the backup file name.
     */
    void CheckForAutoSaveFile( const wxFileName& aFileName, const wxString& aBackupFileExtension );

    /**
     * Function ShowChangedLanguage
     * redraws the menus and what not in current language.
     */
    virtual void ShowChangedLanguage();
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
class EDA_PANEINFO : public wxAuiPaneInfo
{

public:

    /**
     * Function HorizontalToolbarPane
     * Change *this to a horizontal toolbar for KiCad.
     */
    EDA_PANEINFO& HorizontalToolbarPane()
    {
        ToolbarPane();
        CloseButton( false );
        LeftDockable( false );
        RightDockable( false );
        //====================  Remove calls below here for movable toolbars //
        Gripper( false );
        DockFixed( true );
        Movable( false );
        Resizable( true );
        return *this;
    }

    /**
     * Function VerticalToolbarPane
     * Change *this to a vertical toolbar for KiCad.
     */
    EDA_PANEINFO& VerticalToolbarPane()
    {
        ToolbarPane();
        CloseButton( false );
        TopDockable( false );
        BottomDockable( false );
        //====================  Remove calls below here for movable toolbars //
        Gripper( false );
        DockFixed( true );
        Movable( false );
        Resizable( true );
        return *this;
    }

    /**
     * Function MessageToolbarPane
     * Change *this to a message pane for KiCad.
     *
     */
    EDA_PANEINFO& MessageToolbarPane()
    {
        Gripper( false );
        DockFixed( true );
        Movable( false );
        Floatable( false );
        CloseButton( false );
        CaptionVisible( false );
        return *this;
    }

    /**
     * Function LayersToolbarPane
     * Change *this to a layers toolbar for KiCad.
     */
    EDA_PANEINFO& LayersToolbarPane()
    {
        CloseButton( false );
        return *this;
    }

    /**
     * Function InfoToolbarPane
     * Change *this to a information panel for for KiCad.
     *
     * Info panes are used for vertical display of information next to the center pane.
     * Used in CvPcb and the library viewer primarily.
     */
    EDA_PANEINFO& InfoToolbarPane()
    {
        Gripper( false );
        CloseButton( false );
        CaptionVisible( false );
        return *this;
    }
};

#endif  // WXSTRUCT_H_
