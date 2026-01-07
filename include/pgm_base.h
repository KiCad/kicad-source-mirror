/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file pgm_base.h
 * @brief see class PGM_BASE
 */

#ifndef  PGM_BASE_H_
#define  PGM_BASE_H_

#include <bs_thread_pool.hpp>
#include <kicommon.h>
#include <singleton.h>
#include <exception>
#include <map>
#include <future>
#include <mutex>
#include <vector>
#include <memory>
#include <search_stack.h>
#include <settings/environment.h>
#include <wx/filename.h>

class wxApp;
class wxMenu;
class wxWindow;
class wxSplashScreen;
class wxSingleInstanceChecker;

class KISTATUSBAR;
struct LOAD_MESSAGE;
struct BACKGROUND_JOB;
class BACKGROUND_JOBS_MONITOR;
class NOTIFICATIONS_MANAGER;
class COMMON_SETTINGS;
class SETTINGS_MANAGER;
class LIBRARY_MANAGER;
class SCRIPTING;

#ifdef KICAD_IPC_API
class API_PLUGIN_MANAGER;
class KICAD_API_SERVER;
#endif

/**
 * A small class to handle the list of existing translations.
 *
 * The locale translation is automatic.  The selection of languages is mainly for
 * maintainer's convenience.  To add a support to a new translation add a new item
 * to #LanguagesList[].
 */
struct KICOMMON_API LANGUAGE_DESCR
{
    /// wxWidgets locale identifier (See wxWidgets doc)
    int         m_WX_Lang_Identifier;

    /// KiCad identifier used in menu selection (See id.h)
    int         m_KI_Lang_Identifier;

    /// Labels used in menus
    wxString    m_Lang_Label;

    /// Set to true if the m_Lang_Label must not be translated
    bool        m_DoNotTranslate;
};


/**
 * An array containing all the languages that KiCad supports.
 */
KICOMMON_API extern LANGUAGE_DESCR LanguagesList[];

/**
 * Container for data for KiCad programs.
 *
 * The functions are virtual so we can do cross module calls without linking to them.  This
 * used to be a wxApp derivative, but that is difficult under wxPython which shapes the wxApp.
 * So now this is a "side-car" (like a motorcycle side-car) object with a back pointer into
 * the wxApp which initializes it.
 *
 * - OnPgmStart() is virtual, may be overridden, and parallels wxApp::OnInit(), from where it
 *   should called.
 * - OnPgmEnd() is virtual, may be overridden, and parallels wxApp::OnExit(), from where it
 *   should be called.
 */
class KICOMMON_API PGM_BASE
{
public:
    PGM_BASE();
    virtual ~PGM_BASE();

    /**
     * Builds the UTF8 based argv variable
     */
    void BuildArgvUtf8();

    BS::priority_thread_pool& GetThreadPool() { return *m_singleton.m_ThreadPool; }

    GL_CONTEXT_MANAGER* GetGLContextManager() { return m_singleton.m_GLContextManager; }

    /**
     * Specific to MacOSX (not used under Linux or Windows).
     *
     * MacOSX requires it for file association.
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    virtual void MacOpenFile( const wxString& aFileName ) = 0;

    virtual SETTINGS_MANAGER& GetSettingsManager() const { return *m_settings_manager; }

    virtual LIBRARY_MANAGER& GetLibraryManager() const { return *m_library_manager; }

    virtual COMMON_SETTINGS*  GetCommonSettings() const;

    virtual BACKGROUND_JOBS_MONITOR& GetBackgroundJobMonitor() const
    {
        return *m_background_jobs_monitor;
    }

    virtual NOTIFICATIONS_MANAGER& GetNotificationsManager() const
    {
        return *m_notifications_manager;
    }

#ifdef KICAD_IPC_API
    virtual API_PLUGIN_MANAGER& GetPluginManager() const { return *m_plugin_manager; }

    KICAD_API_SERVER& GetApiServer() { return *m_api_server; }
#endif

    virtual void SetTextEditor( const wxString& aFileName );

    /**
     * Return the path to the preferred text editor application.
     *
     * @param   aCanShowFileChooser If no editor is currently set and this argument is
     *          'true' then this method will show a file chooser dialog asking for the
     *          editor's executable.
     * @return  Returns the full path of the editor, or an empty string if no editor has
     *          been set.
     */
    virtual const wxString& GetTextEditor( bool aCanShowFileChooser = true );

    /**
     * Show a dialog that instructs the user to select a new preferred editor.
     *
     * @param   aDefaultEditor Default full path for the default editor this dialog should
     *          show by default.
     * @return  the full path of the editor, or an empty string if no editor was chosen.
     */
    virtual const wxString AskUserForPreferredEditor(
            const wxString& aDefaultEditor = wxEmptyString );

    virtual bool IsKicadEnvVariableDefined() const               { return !m_kicad_env.IsEmpty(); }

    virtual const wxString& GetKicadEnvVariable() const          { return m_kicad_env; }

    virtual const wxString& GetExecutablePath() const;

    virtual wxLocale* GetLocale()                                { return m_locale; }

    virtual const wxString& GetPdfBrowserName() const            { return m_pdf_browser; }

    virtual void SetPdfBrowserName( const wxString& aFileName )  { m_pdf_browser = aFileName; }

    /**
     * @return true if the PDF browser is the default (system) PDF browser and false if the
     *         PDF browser is the preferred (selected) browser, else returns false if there
     *         is no selected browser.
     */
    virtual bool UseSystemPdfBrowser() const
    {
        return m_use_system_pdf_browser || m_pdf_browser.IsEmpty();
    }

    /**
     * Force the use of system PDF browser, even if a preferred PDF browser is set.
     */
    virtual void ForceSystemPdfBrowser( bool aFlg ) { m_use_system_pdf_browser = aFlg; }

    /**
     * Set the dictionary file name for internationalization.
     *
     * The files are in kicad/internat/xx or kicad/internat/xx_XX and are named kicad.mo
     *
     * @param aErrMsg is the string to return the error message it.
     * @param first_time must be set to true the first time this function is called,
     *                   false otherwise.
     * @return false if there was an error setting the language.
     */
    virtual bool SetLanguage( wxString& aErrMsg, bool first_time = false );

    /**
     * Set the default language without reference to any preferences.
     *
     * Can be used to set the language for dialogs that show before preferences are loaded.
     *
     * @param aErrMsg String to return the error message(s) in.
     * @return false if the language could not be set.
     */
    bool SetDefaultLanguage( wxString& aErrMsg );

    /**
     * Set in .m_language_id member the wxWidgets language identifier ID from the KiCad
     * menu id (internal menu identifier).
     *
     * @param menu_id The KiCad menuitem id (returned by Menu Event, when clicking on a
     *                 menu item)
     */
    virtual void SetLanguageIdentifier( int menu_id );

    /**
     * @return the wxWidgets language identifier Id of the language currently selected.
     */
    virtual int GetSelectedLanguageIdentifier() const { return m_language_id; }

    /**
     * @return the current selected language in rfc3066 format
     */
    virtual wxString GetLanguageTag();

    virtual void SetLanguagePath();

    /**
     * Read the PDF browser choice from the common configuration.
     */
    virtual void ReadPdfBrowserInfos();

    /**
     * Save the PDF browser choice to the common configuration.
     */
    virtual void WritePdfBrowserInfos();

    /**
     * Set the environment variable \a aName to \a aValue.
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
    virtual bool SetLocalEnvVariable( const wxString& aName, const wxString& aValue );

    /**
     * Update the local environment with the contents of the current #ENV_VAR_MAP stored in the
     * #COMMON_SETTINGS.
     *
     * @see GetLocalEnvVariables()
     */
    virtual void SetLocalEnvVariables();

    virtual ENV_VAR_MAP& GetLocalEnvVariables() const;

    /**
     * Return a bare naked wxApp which may come from wxPython, SINGLE_TOP, or kicad.exe.
     *
     * This should return what wxGetApp() returns.
     */
    virtual wxApp&   App();

    static const wxChar workingDirKey[];

    /**
     * Initialize this program.
     *
     * Initialize the process in a KiCad standard way using some generalized techniques:
     *  - Default paths (help, libs, bin) and configuration file names
     *  - Language and locale
     *  - fonts
     *
     * @note Do not initialize anything relating to DSOs or projects.
     *
     * @param aHeadless If true, run in headless mode (e.g. for unit tests)
     * @param aSkipPyInit If true, do not init python stuff.
     * Useful in application that do not use python, to disable python dependency at run time
     * @return true if success, false if failure and program is to terminate.
     */
    bool InitPgm( bool aHeadless = false, bool aSkipPyInit = false, bool aIsUnitTest = false );

    // The PGM_* classes can have difficulties at termination if they
    // are not destroyed soon enough.  Relying on a static destructor can be
    // too late for contained objects like wxSingleInstanceChecker.
    void Destroy();

    /**
     * Save the program (process) settings subset which are stored .kicad_common.
     */
    void SaveCommonSettings();

    /**
     * A exception handler to be used at the top level if exceptions bubble up that for.
     *
     * The purpose is to have a central place to log a wxWidgets error message and/or sentry report.
     *
     * @param aPtr Pass the std::current_exception() from within the catch block.
     */
    void HandleException( std::exception_ptr aPtr, bool aUnhandled = false );

    /**
     * A common assert handler to be used between single_top and kicad.
     *
     * This lets us have a common set of assert handling, including triggering sentry reports.
     *
     * @param aFile the file path of the assert.
     * @param aLine the line number of the assert.
     * @param aFunc the function name the assert is within.
     * @param aCond the condition of the assert.
     * @param aMsg the attached assert message (can be empty).
     */
    void HandleAssert( const wxString& aFile, int aLine, const wxString& aFunc,
                       const wxString& aCond, const wxString& aMsg );

    /**
     * Determine if the application is running with a GUI.
     *
     * @return true if there is a GUI and false otherwise.
     */
    bool IsGUI();


    void ShowSplash();
    void HideSplash();

    /**
     * Allow access to the wxSingleInstanceChecker to test for other running KiCads.
     */
    std::unique_ptr<wxSingleInstanceChecker>& SingleInstance()
    {
        return m_pgm_checker;
    }

    /**
     * Starts a background job to preload the global and project design block libraries.
     * Design block handling code is not associated with a particular KIFACE so this is
     * handled here unlike symbol/footprint loading which are taken care of by the KIFACEs.
     */
    void PreloadDesignBlockLibraries( KIWAY* aKiway );

    /**
     * Register a status bar to receive library load warning messages.
     * Multiple status bars can be registered (one per open frame).
     */
    void RegisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Unregister a status bar from receiving library load warning messages.
     */
    void UnregisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Add library load messages to all registered status bars.
     * Thread-safe - can be called from background threads.
     */
    void AddLibraryLoadMessages( const std::vector<LOAD_MESSAGE>& aMessages );

    /**
     * Clear library load messages from all registered status bars.
     */
    void ClearLibraryLoadMessages();

    /**
     * wxWidgets on MSW tends to crash if you spool up more than one print job at a time.
     */
    bool m_Printing;

    std::vector<void*> m_ModalDialogs;

    bool m_Quitting;

    bool m_PropertyGridInitialized;

protected:
    /// Load internal settings from #COMMON_SETTINGS.
    void loadCommonSettings();

    /// Trap all changes in here, simplifies debugging.
    void setLanguageId( int aId )       { m_language_id = aId; }

#ifdef KICAD_USE_SENTRY
    void     sentryInit();
    wxString sentryCreateUid();
#endif

protected:
    std::unique_ptr<SETTINGS_MANAGER> m_settings_manager;
    std::unique_ptr<LIBRARY_MANAGER> m_library_manager;
    std::unique_ptr<BACKGROUND_JOBS_MONITOR> m_background_jobs_monitor;
    std::unique_ptr<NOTIFICATIONS_MANAGER> m_notifications_manager;

    std::unique_ptr<SCRIPTING> m_python_scripting;

    /// Check if there is another copy of Kicad running at the same time.
    std::unique_ptr<wxSingleInstanceChecker> m_pgm_checker;

#ifdef KICAD_IPC_API
    std::unique_ptr<API_PLUGIN_MANAGER> m_plugin_manager;
    std::unique_ptr<KICAD_API_SERVER> m_api_server;
#endif

    wxString        m_kicad_env;              ///< The KICAD system environment variable.

    wxLocale*       m_locale;
    int             m_language_id;

    bool            m_use_system_pdf_browser;
    wxString        m_pdf_browser;            ///< Filename of the app selected for browsing PDFs.

    wxString        m_text_editor;

    KICAD_SINGLETON m_singleton;

#ifdef KICAD_USE_SENTRY
    wxFileName      m_sentry_optin_fn;
    wxFileName      m_sentry_uid_fn;
    wxString        m_sentryUid;
#endif

    /**
     * argv parameters converted to utf8 form because wxWidgets has opinions.
     *
     * This will return argv as either force converted to ASCII in char* or wchar_t only.
     */
    char** m_argvUtf8;

    int m_argcUtf8;

    wxSplashScreen* m_splash;

    std::shared_ptr<BACKGROUND_JOB> m_libraryPreloadBackgroundJob;
    std::future<void>               m_libraryPreloadReturn;
    std::atomic_bool                m_libraryPreloadInProgress;
    std::atomic_bool                m_libraryPreloadAbort;

    std::vector<KISTATUSBAR*>       m_libraryLoadStatusBars;
    mutable std::mutex              m_libraryLoadStatusBarsMutex;
};


/**
 * The global program "get" accessor.
 *
 * Implemented in:
 *    1. common/single_top.cpp
 *    2. kicad/kicad.cpp
 *    3. scripting/kiway.i
 */
KICOMMON_API extern PGM_BASE& Pgm();

/// Return a reference that can be nullptr when running a shared lib from a script, not from
/// a kicad app.
KICOMMON_API extern PGM_BASE* PgmOrNull();

KICOMMON_API extern void SetPgm( PGM_BASE* pgm );


#endif  // PGM_BASE_H_
