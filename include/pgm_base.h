/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file pgm_base.h
 * @brief see class PGM_BASE
 */

#ifndef  PGM_BASE_H_
#define  PGM_BASE_H_

#include <map>
#include <wx/filename.h>
#include <search_stack.h>
#include <wx/gdicmn.h>


class wxConfigBase;
class wxSingleInstanceChecker;
class wxApp;
class wxMenu;

// inter program module calling
#define VTBL_ENTRY      virtual


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
    ~PGM_BASE();

    /**
     * Function OnPgmInit
     * this is the first executed function (like main() )
     * @return true if the application can be started.
     */
    virtual bool OnPgmInit( wxApp* aWxApp ) = 0;    // call this from wxApp::OnInit()

    virtual void OnPgmExit() = 0;                   // call this from wxApp::OnExit()

    /**
     * Function MacOpenFile
     * is specific to MacOSX (not used under Linux or Windows).
     * MacOSX requires it for file association.
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    virtual void MacOpenFile( const wxString& aFileName ) = 0;

    //----<Cross Module API>-----------------------------------------------------

    VTBL_ENTRY wxConfigBase* CommonSettings() const                 { return m_common_settings; }

    VTBL_ENTRY void SetEditorName( const wxString& aFileName );

    /**
     * Return the preferred editor name.
     */
    VTBL_ENTRY const wxString& GetEditorName();

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
     * Function AddMenuLanguageList
     * creates a menu list for language choice, and add it as submenu to \a MasterMenu.
     *
     * @param MasterMenu The main menu. The sub menu list will be accessible from the menu
     *                   item with id ID_LANGUAGE_CHOICE
     */
    VTBL_ENTRY void AddMenuLanguageList( wxMenu* MasterMenu );

    /**
     * Function SetLanguageIdentifier
     * sets in .m_language_id member the wxWidgets language identifier Id  from
     * the KiCad menu id (internal menu identifier).
     *
     * @param menu_id The KiCad menuitem id (returned by Menu Event, when
     *                clicking on a menu item)
     */
    VTBL_ENTRY void SetLanguageIdentifier( int menu_id );

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
     * Function App
     * returns a bare naked wxApp, which may come from wxPython, SINGLE_TOP, or kicad.exe.
     * Use this function instead of wxGetApp().
     */
    VTBL_ENTRY wxApp&   App()
    {
        wxASSERT( m_wx_app );
        return *m_wx_app;
    }

    //----</Cross Module API>----------------------------------------------------

    static const wxChar workingDirKey[];

protected:

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
    bool initPgm();

    /**
     * Function loadCommonSettings
     * loads the program (process) settings subset which are stored in .kicad_common
     */
    void loadCommonSettings();

    /**
     * Function saveCommonSettings
     * saves the program (process) settings subset which are stored .kicad_common
     */
    void saveCommonSettings();

    /// prevents multiple instances of a program from being run at the same time.
    wxSingleInstanceChecker* m_pgm_checker;

    /// Configuration settings common to all KiCad program modules,
    /// like as in $HOME/.kicad_common
    wxConfigBase*   m_common_settings;

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
    /// library table.
    std::map<wxString, wxString>  m_local_env_vars;

    wxApp*          m_wx_app;

    // The PGM_* classes can have difficulties at termination if they
    // are not destroyed soon enough.  Relying on a static destructor can be
    // too late for contained objects like wxSingleInstanceChecker.
    void destroy();
};


#if !defined(PGM_KICAD_H_)                  // PGM_KICAD has an alternate
/// The global Program "get" accessor.
extern PGM_BASE& Pgm();
#endif

#endif  // PGM_BASE_H_
