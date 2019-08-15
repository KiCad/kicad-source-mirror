/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pgm_base.h
 * @brief see class PGM_BASE
 */

#ifndef  PGM_BASE_H_
#define  PGM_BASE_H_

#include <map>
#include <memory>
#include <wx/filename.h>
#include <wx/filehistory.h>
#include <search_stack.h>
#include <wx/gdicmn.h>
#include <bitmaps_png/bitmap_def.h>


///@{
/// \ingroup config

#define USE_ICONS_IN_MENUS_KEY          wxT( "UseIconsInMenus" )
#define ICON_SCALE_KEY                  wxT( "IconScale" )
#define CANVAS_SCALE_KEY                wxT( "CanvasScale" )
#define AUTOSAVE_INTERVAL_KEY           wxT( "AutoSaveInterval" )
#define ENBL_ZOOM_NO_CENTER_KEY         wxT( "ZoomNoCenter" )
#define ENBL_MOUSEWHEEL_PAN_KEY         wxT( "MousewheelPAN" )
#define MIDDLE_BUTT_PAN_LIMITED_KEY     wxT( "MiddleBtnPANLimited" )
#define ENBL_AUTO_PAN_KEY               wxT( "AutoPAN" )
#define FILE_HISTORY_SIZE_KEY           wxT( "FileHistorySize" )
#define GAL_DISPLAY_OPTIONS_KEY         wxT( "GalDisplayOptions" )
#define GAL_ANTIALIASING_MODE_KEY       wxT( "OpenGLAntialiasingMode" )
#define CAIRO_ANTIALIASING_MODE_KEY     wxT( "CairoAntialiasingMode" )
#define WARP_MOUSE_ON_MOVE_KEY           wxT( "MoveWarpsCursor" )
#define IMMEDIATE_ACTIONS_KEY           wxT( "ImmediateActions" )
#define PREFER_SELECT_TO_DRAG_KEY       wxT( "PreferSelectionToDragging" )

///@}

class wxConfigBase;
class wxSingleInstanceChecker;
class wxApp;
class wxMenu;
class wxWindow;

/**
 *   A small class to handle the list of existing translations.
 *   The locale translation is automatic.
 *   The selection of languages is mainly for maintainer's convenience
 *   To add a support to a new translation:
 *   create a new icon (flag of the country) (see Lang_Fr.xpm as an example)
 *   add a new item to s_Languages[].
 */
struct LANGUAGE_DESCR
{
    /// wxWidgets locale identifier (See wxWidgets doc)
    int         m_WX_Lang_Identifier;

    /// KiCad identifier used in menu selection (See id.h)
    int         m_KI_Lang_Identifier;

    /// The menu language icons
    BITMAP_DEF  m_Lang_Icon;

    /// Labels used in menus
    wxString    m_Lang_Label;

    /// Set to true if the m_Lang_Label must not be translated
    bool        m_DoNotTranslate;
};


class FILE_HISTORY : public wxFileHistory
{
public:
    FILE_HISTORY( size_t aMaxFiles, int aBaseFileId );

    void SetMaxFiles( size_t aMaxFiles );
};


// inter program module calling
#define VTBL_ENTRY      virtual


/**
 * Class ENV_VAR_ITEM
 *
 * is a simple helper class to store environment variable values and the status of whether
 * or not they were defined externally to the process created when any of the KiCad applications
 * was launched.
 */
class ENV_VAR_ITEM
{
public:
    ENV_VAR_ITEM( const wxString& aValue = wxEmptyString, bool aIsDefinedExternally = false ) :
        m_value( aValue ),
        m_isDefinedExternally( aIsDefinedExternally )
    {
    }

    ~ENV_VAR_ITEM() throw() {}    // tell SWIG no exception

    bool GetDefinedExternally() const { return m_isDefinedExternally; }
    void SetDefinedExternally( bool aIsDefinedExternally )
    {
        m_isDefinedExternally = aIsDefinedExternally;
    }

    const wxString& GetValue() const { return m_value; }
    void SetValue( const wxString& aValue ) { m_value = aValue; }

private:
    /// The environment variable string value.
    wxString m_value;

    /// Flag to indicate if the environment variable was defined externally to the process.
    bool     m_isDefinedExternally;
};


typedef std::map<wxString, ENV_VAR_ITEM>                 ENV_VAR_MAP;
typedef std::map<wxString, ENV_VAR_ITEM>::iterator       ENV_VAR_MAP_ITER;
typedef std::map<wxString, ENV_VAR_ITEM>::const_iterator ENV_VAR_MAP_CITER;


/**
 * Class PGM_BASE
 * keeps program (whole process) data for KiCad programs.
 * The VTBL_ENTRY functions are VTBL_ENTRY so we can do cross module calls
 * without linking to them.  This used to be a wxApp derivative, but that
 * is difficult under wxPython which shapes the wxApp. So now this is a "side-car"
 * (like a motorcycle side-car) object with a back pointer into the wxApp
 * which initializes it.
 * <p>
 * OnPgmStart() is virtual, may be overridden, and parallels
 * wxApp::OnInit(), from where it should called.
 * <p>
 * OnPgmEnd() is virtual, may be overridden, and parallels wxApp::OnExit(),
 * from where it should be called.
 */
class PGM_BASE
{
public:
    PGM_BASE();
    virtual ~PGM_BASE();

#if 0
    /*

    Derived classes must implement these two functions: OnPgmInit() and
    OnPgmExit(), and since they are only called from same source file as their
    implementation, these need not be virtual here. In fact, in the case of
    python project manager's class PGM_PYTHON, these functions are actually
    written in python. In total there are three implementations, corresponding
    to the three defines given by kiface.h's KFCTL_* #defines.

    */

    /**
     * Function OnPgmInit
     * this is the first executed function (like main() )
     * @return true if the application can be started.
     */
    virtual bool OnPgmInit() = 0;           // call this from wxApp::OnInit()

    virtual void OnPgmExit() = 0;           // call this from wxApp::OnExit()
#endif

    //----<Cross Module API>-----------------------------------------------------

    /**
     * Function MacOpenFile
     * is specific to MacOSX (not used under Linux or Windows).
     * MacOSX requires it for file association.
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    VTBL_ENTRY void MacOpenFile( const wxString& aFileName ) = 0;

    VTBL_ENTRY wxConfigBase* CommonSettings() const { return m_common_settings.get(); }

    VTBL_ENTRY void SetEditorName( const wxString& aFileName );

    /**
     * Return the preferred editor name.
     * @param   aCanShowFileChooser If no editor is currently set and this argument is
     *          'true' then this method will show a file chooser dialog asking for the
     *          editor's executable.
     * @return  Returns the full path of the editor, or an empty string if no editor has
     *          been set.
     */
    VTBL_ENTRY const wxString& GetEditorName( bool aCanShowFileChooser = true );

    /**
     * Shows a dialog that instructs the user to select a new preferred editor.
     * @param   aDefaultEditor Default full path for the default editor this dialog should
     *          show by default.
     * @return  Returns the full path of the editor, or an empty string if no editor was
     *          chosen.
     */
    VTBL_ENTRY const wxString AskUserForPreferredEditor(
                                        const wxString& aDefaultEditor = wxEmptyString );

    VTBL_ENTRY bool IsKicadEnvVariableDefined() const               { return !m_kicad_env.IsEmpty(); }

    VTBL_ENTRY const wxString& GetKicadEnvVariable() const          { return m_kicad_env; }

    VTBL_ENTRY const wxString& GetExecutablePath() const            { return m_bin_dir; }

    VTBL_ENTRY wxLocale* GetLocale()                                { return m_locale; }

    VTBL_ENTRY const wxString& GetPdfBrowserName() const            { return m_pdf_browser; }

    VTBL_ENTRY void SetPdfBrowserName( const wxString& aFileName )  { m_pdf_browser = aFileName; }

    /**
     * Function UseSystemPdfBrowser
     * returns true if the PDF browser is the default (system) PDF browser
     * and false if the PDF browser is the preferred (selected) browser, else
     * returns false if there is no selected browser
     */
    VTBL_ENTRY bool UseSystemPdfBrowser() const
    {
        return m_use_system_pdf_browser || m_pdf_browser.IsEmpty();
    }

    /**
     * Function ForceSystemPdfBrowser
     * forces the use of system PDF browser, even if a preferred PDF browser is set.
     */
    VTBL_ENTRY void ForceSystemPdfBrowser( bool aFlg ) { m_use_system_pdf_browser = aFlg; }

    /**
     * Function SetLanguage
     * sets the dictionary file name for internationalization.
     * <p>
     * The files are in kicad/internat/xx or kicad/internat/xx_XX and are named kicad.mo
     * </p>
     * @param   first_time  must be set to true the first time this funct is
     *          called, false otherwise
     * @return  true if the language can be set (i.e. if the locale is available)
     */
    VTBL_ENTRY bool SetLanguage( bool first_time = false );

    /**
     * Function SetLanguageIdentifier
     * sets in .m_language_id member the wxWidgets language identifier Id  from
     * the KiCad menu id (internal menu identifier).
     *
     * @param menu_id The KiCad menuitem id (returned by Menu Event, when
     *                clicking on a menu item)
     */
    VTBL_ENTRY void SetLanguageIdentifier( int menu_id );

    /**
     * @return the wxWidgets language identifier Id of the language currently selected
     */
    VTBL_ENTRY int GetSelectedLanguageIdentifier() const { return m_language_id; }

    VTBL_ENTRY void SetLanguagePath();

    /**
     * Function ReadPdfBrowserInfos
     * reads the PDF browser choice from the common configuration.
     */
    VTBL_ENTRY void ReadPdfBrowserInfos();

    /**
     * Function WritePdfBrowserInfos
     * saves the PDF browser choice to the common configuration.
     */
    VTBL_ENTRY void WritePdfBrowserInfos();

    /**
     * Function SetLocalEnvVariable
     *
     * Sets the environment variable \a aName to \a aValue.
     *
     * This function first checks to see if the environment variable \a aName is already
     * defined.  If it is not defined, then the environment variable \a aName is set to
     * a value.  Otherwise, the environment variable is left unchanged.  This allows the user
     * to override environment variables for testing purposes.
     *
     * @param aName is a wxString containing the environment variable name.
     * @param aValue is a wxString containing the environment variable value.
     * @return true if the environment variable \a Name was set to \a aValue.
     */
    VTBL_ENTRY bool SetLocalEnvVariable( const wxString& aName, const wxString& aValue );

    /**
     * Function SetLocalEnvVariables
     *
     * sets the internal local environment variable map to \a aEnvVarMap, updates the entries
     * in the .kicad_common configuration file, and sets the environment variable to the new
     * settings.
     *
     * @param aEnvVarMap is a ENV_VAR_MAP object containing the new environment variables.
     */
    VTBL_ENTRY void SetLocalEnvVariables( const ENV_VAR_MAP& aEnvVarMap );

    VTBL_ENTRY const ENV_VAR_MAP& GetLocalEnvVariables() const
    {
        return m_local_env_vars;
    }

    /**
     * Function App
     * returns a bare naked wxApp, which may come from wxPython, SINGLE_TOP, or kicad.exe.
     * Should return what wxGetApp() returns.
     */
    VTBL_ENTRY wxApp&   App();

    //----</Cross Module API>----------------------------------------------------

    static const wxChar workingDirKey[];

    /**
     * Function initPgm
     * initializes this program (process) in a KiCad standard way,
     * using some generalized techniques.
     *  - Default paths (help, libs, bin) and configuration file names
     *  - Language and locale
     *  - fonts
     * <p>
     * But nothing relating to DSOs or projects.
     * @return bool - true if success, false if failure and program is to terminate.
     */
    bool InitPgm();

    // The PGM_* classes can have difficulties at termination if they
    // are not destroyed soon enough.  Relying on a static destructor can be
    // too late for contained objects like wxSingleInstanceChecker.
    void Destroy();

    /**
     * Function saveCommonSettings
     * saves the program (process) settings subset which are stored .kicad_common
     */
    void SaveCommonSettings();

    /**
     * wxWidgets on MSW tends to crash if you spool up more than one print job at a time.
     */
    bool m_Printing;

protected:

    /**
     * Function loadCommonSettings
     * loads the program (process) settings subset which are stored in .kicad_common
     */
    void loadCommonSettings();

    /// prevents multiple instances of a program from being run at the same time.
    wxSingleInstanceChecker* m_pgm_checker;

    /// Configuration settings common to all KiCad program modules,
    /// like as in $HOME/.kicad_common
    std::unique_ptr<wxConfigBase> m_common_settings;

    /// full path to this program
    wxString        m_bin_dir;

    /// The KICAD system environment variable.
    wxString        m_kicad_env;

    /// The current locale.
    wxLocale*       m_locale;

    /// The current language setting.
    int             m_language_id;

    /// true to use the selected PDF browser, if exists, or false to use the default
    bool            m_use_system_pdf_browser;

    /// Trap all changes in here, simplifies debugging
    void setLanguageId( int aId )       { m_language_id = aId; }

    /**
     * Function setExecutablePath
     * finds the path to the executable and stores it in PGM_BASE::m_bin_dir
     * @return bool - true if success, else false.
     */
    bool setExecutablePath();

    /// The file name of the the program selected for browsing pdf files.
    wxString        m_pdf_browser;
    wxString        m_editor_name;
    wxSize          m_help_size;

    /// Local environment variable expansion settings such as KIGITHUB, KISYSMOD, and KISYS3DMOD.
    ENV_VAR_MAP     m_local_env_vars;

    /// Flag to indicate if the environment variable overwrite warning dialog should be shown.
    bool            m_show_env_var_dialog;
};


/// The global Program "get" accessor.
/// Implemented in: 1) common/single_top.cpp,  2) kicad/kicad.cpp, and 3) scripting/kiway.i
extern PGM_BASE& Pgm();

#endif  // PGM_BASE_H_
