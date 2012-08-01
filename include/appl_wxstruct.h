/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file appl_wxstruct.h
 * @brief Base class implementation for all KiCad applications.
 */

#ifndef  APPL_WXSTRUCT_H
#define  APPL_WXSTRUCT_H

/* Use wxFileHistory for most recently used file handling. */
#include <wx/docview.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <param_config.h>
#ifdef KICAD_SCRIPTING_EXPERIMENT
#include <python2.7/Python.h>
#include <wx/wxPython/wxPython.h>
#endif

enum EDA_APP_T {
    APP_UNKNOWN_T,
    APP_EESCHEMA_T,
    APP_PCBNEW_T,
    APP_CVPCB_T,
    APP_GERBVIEW_T,
    APP_KICAD_T
};

class wxConfigBase;
class wxFileConfig;
class wxSingleInstanceChecker;
class wxHtmlHelpController;


/**
 * Class EDA_APP
 * is the base class representing all of KiCad applications.
 */
class EDA_APP : public wxApp
{
#ifdef KICAD_SCRIPTING_EXPERIMENT
    private:
    PyThreadState* m_mainTState;
#endif
protected:
    /// Used mainly to handle default paths libs m_Id = APP_EESCHEMA_T, APP_PCBNEW_T ...
    EDA_APP_T m_Id;

    /// Used to prevent multiple instances of an application from being run at the same time.
    wxSingleInstanceChecker* m_Checker;

    wxString m_Project;

    /// The application specific configuration settings.
    wxConfig* m_settings;

    /// The configuration settings common to all KiCad applications.
    wxConfig* m_commonSettings;

    /// The current project specific settings.
    wxFileConfig* m_projectSettings;

    /// KiCad executable path.
    wxString m_BinDir;

    /// The KICAD system environment variable.
    wxString m_KicadEnv;

    /// The current locale.
    wxLocale* m_Locale;

    /// The current language setting.
    int m_LanguageId;

    /// The file name of the the program selected for browsing pdf files.
    wxString m_PdfBrowser;

    wxPathList               m_searchPaths;
    wxFileHistory            m_fileHistory;
    wxString                 m_HelpFileName;
    wxString                 m_EditorName;
    wxString                 m_CurrentOptionFile;
    wxString                 m_CurrentOptionFileDateAndTime;
    wxPoint                  m_HelpPos;
    wxSize                   m_HelpSize;
    wxHtmlHelpController*    m_HtmlCtrl;
    wxString                 m_Title;
    wxPathList               m_libSearchPaths;
    wxFileName               m_projectFileName;
    wxString                 m_LastVisitedLibPath;

public:
    EDA_APP();
    ~EDA_APP();

    /**
     * Function OnInit
     * this is the first executed function (like main() )
     * @return true if the application can be started.
     */
    bool OnInit(); // should this be virtual
#ifdef KICAD_SCRIPTING_EXPERIMENT
    bool Init_wxPython(); 
#endif
    // This is only called if OnInit() returned true so it's a good place to do
    // any cleanup matching the initializations done there.
 ////   virtual int OnExit() ;//  { return(0) ; }; // TODO FIX THIS HACK MUST BE VIRTUAL
        
    wxHtmlHelpController* GetHtmlHelpController() { return m_HtmlCtrl; }

    void SetHtmlHelpController( wxHtmlHelpController* aController );

    wxString GetHelpFileName() const { return m_HelpFileName; }

    void SetHelpFileName( const wxString& aFileName ) { m_HelpFileName = aFileName; }

    wxConfig* GetSettings() { return m_settings; }

    wxConfig* GetCommonSettings() { return m_commonSettings; }

    wxString GetEditorName() const { return m_EditorName; }

    void SetEditorName( const wxString& aFileName ) { m_EditorName = aFileName; }

    wxString GetCurrentOptionFile() const { return m_CurrentOptionFile; }

    bool IsKicadEnvVariableDefined() const { return !m_KicadEnv.IsEmpty(); }

    wxString GetKicadEnvVariable() const { return m_KicadEnv; }

    wxString GetExecutablePath() const { return m_BinDir; }

    wxLocale* GetLocale() { return m_Locale; }

    wxString GetPdfBrowserFileName() const { return m_PdfBrowser; }

    void SetPdfBrowserFileName( const wxString& aFileName ) { m_PdfBrowser = aFileName; }

    bool UseSystemPdfBrowser() const { return m_PdfBrowser.IsEmpty(); }

    wxFileHistory& GetFileHistory() { return m_fileHistory; }

    /**
     * Function SetBinDir
     * finds the path to the executable and store it in EDA_APP::m_BinDir
     *
     * @return TODO
     */
    bool SetBinDir();

    /**
     * Function SetDefaultSearchPaths
     * sets search paths for libraries, modules, internationalization files, etc.
     */
    void SetDefaultSearchPaths( void );

    /**
     * Function MacOpenFile
     * Specific to MacOSX. Not used under Linux or Windows
     * MacOSX: Needed for file association
     * http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    virtual void MacOpenFile(const wxString &fileName);

    /**
     * Function InitEDA_Appl
     * initialize some general parameters
     *  - Default paths (help, libs, bin)and configuration files names
     *  - Language and locale
     *  - fonts
     * @param aName : used as paths in configuration files
     * @param aId = flag : LIBRARY_TYPE_EESCHEMA or LIBRARY_TYPE_PCBNEW
     *              used to choose what default library path must be used
     */
    void InitEDA_Appl( const wxString& aName, EDA_APP_T aId = APP_UNKNOWN_T );

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
    bool SetLanguage( bool first_time = false );

    /**
     * Function AddMenuLanguageList
     * creates a menu list for language choice, and add it as submenu to \a MasterMenu.
     *
     * @param MasterMenu The main menu. The sub menu list will be accessible from the menu
     *                   item with id ID_LANGUAGE_CHOICE
     */
    void AddMenuLanguageList( wxMenu* MasterMenu );

    /**
     * Function SetLanguageIdentifier
     * sets in .m_LanguageId member the wxWidgets language identifier Id  from
     * the KiCad menu id (internal menu identifier).
     *
     * @param menu_id The KiCad menuitem id (returned by Menu Event, when
     *                clicking on a menu item)
     */
    void SetLanguageIdentifier( int menu_id );

    void SetLanguagePath( void );

    /**
     * Function InitOnLineHelp
     * initializes KiCad's online help.
     */
    void InitOnLineHelp();

    /**
     * Function GetSettings
     * gets the application settings.
     * @param aReopenLastUsedDirectory True to switch to last opened directory, false
     *                                 to use current CWD
     */
    void GetSettings( bool aReopenLastUsedDirectory );

    /**
     * Function SaveSettings
     * saves the application settings.
     */
    void SaveSettings();

    /**
     * Function WriteProjectConfig
     *  Save the current "project" parameters
     *  saved parameters are parameters that have the .m_Setup member set to false
     *  saving file is the .pro file project
     */
    void WriteProjectConfig( const wxString&  local_config_filename,
                             const wxString&  GroupName,
                             PARAM_CFG_BASE** List );

    void WriteProjectConfig( const wxString&  fileName,
                             const wxString&  GroupName,
                             const PARAM_CFG_ARRAY& params );

    /**
     * Function SaveCurrentSetupValues
     * saves the current setup values in m_settings.
     * Saved parameters are parameters that have the .m_Setup member set to
     * true.
     * @param aList = array of PARAM_CFG_BASE pointers
     */
    void SaveCurrentSetupValues( PARAM_CFG_BASE** aList );
    void SaveCurrentSetupValues( const PARAM_CFG_ARRAY& List );

    /**
     * Function ReadCurrentSetupValues
     * reads the current setup values previously saved, from m_settings.
     * Saved parameters are parameters that have the .m_Setup member set to
     * true.
     * @param aList = array of PARAM_CFG_BASE pointers
     */
    void ReadCurrentSetupValues( PARAM_CFG_BASE** aList );
    void ReadCurrentSetupValues( const PARAM_CFG_ARRAY& List );

    /**
     * Function ReadProjectConfig
     *  Read the current "project" parameters
     *  Parameters are parameters that have the .m_Setup member set to false
     *  read file is the .pro file project
     *
     * if Load_Only_if_New == true, this file is read only if it differs from
     * the current config (different dates )
     *
     * @return      true if read.
     * Also set:
     *     wxGetApp().m_CurrentOptionFileDateAndTime
     *     wxGetApp().m_CurrentOptionFile
     */
    bool ReadProjectConfig( const wxString&  local_config_filename,
                            const wxString&  GroupName,
                            PARAM_CFG_BASE** List,
                            bool             Load_Only_if_New );
    bool ReadProjectConfig( const wxString&  local_config_filename,
                            const wxString&  GroupName,
                            const PARAM_CFG_ARRAY& List,
                            bool             Load_Only_if_New );

    /**
     * Creates or recreates the KiCad project file. (filename.pro)
     * Initialize:
     * G_Prj_Config
     * G_Prj_Config_LocalFilename
     * G_Prj_Default_Config_FullFilename
     * Return:
     * True if local config
     * False if default config
     */
    bool ReCreatePrjConfig( const wxString& local_config_filename,
                            const wxString& GroupName,
                            bool            ForceUseLocalConfig );

    /**
     * Function ReadPdfBrowserInfos
     * read the PDF browser choice from the common configuration.
     */
    void ReadPdfBrowserInfos();

    /* Function WritePdfBrowserInfos
     * save the PDF browser choice to the common configuration.
     */
    void WritePdfBrowserInfos();

    /**
     * Function FindFileInSearchPaths
     * looks in search paths for \a filename.
     */
    wxString FindFileInSearchPaths( const wxString&      filename,
                                    const wxArrayString* subdirs = NULL );

    /**
     * Function GetHelpFile
     * get the help file path.
     * <p>
     * Return the KiCad help file with path.  The base paths defined in
     * m_searchPaths are tested for a valid file.  The path returned can
     * be relative depending on the paths added to m_searchPaths.  See the
     * documentation for wxPathList for more information. If the help file
     * for the current locale is not found, an attempt to find the English
     * version of the help file is made.
     * wxEmptyString is returned if help file not found.
     * Help file is searched in directories in this order:
     *  help/\<canonical name\> like help/en_GB
     *  help/\<short name\> like help/en
     *  help/en
     * </p>
     */
    wxString GetHelpFile( void );

    wxString GetLibraryFile( const wxString& filename );

    /**
     * Return the preferred editor name.
     */
    wxString& GetEditorName();

    const wxString& GetTitle() { return m_Title; }
    void SetTitle( const wxString& title ) { m_Title = title; }

    wxPathList& GetLibraryPathList() { return m_libSearchPaths; }
    wxString FindLibraryPath( const wxString& fileName );

    /**
     * Function FindLibraryPath
     * KiCad saves user defined library files that are not in the standard
     * library search path list with the full file path.  Calling the library
     * search path list with a user library file will fail.  This helper method
     * solves that problem.
     * @param fileName
     * @return a wxEmptyString if library file is not found.
     */
    wxString FindLibraryPath( const wxFileName& fileName )
    {
        return FindLibraryPath( fileName.GetFullPath() );
    }


    /**
     * Function ReturnLastVisitedLibraryPath
     * returns the last visited library directory, or (if void) the first
     * path in lib path list ( but not the CWD )
     *
     * @param aSubPathToSearch = Preferred sub path to search in path list
     */
    wxString ReturnLastVisitedLibraryPath( const wxString& aSubPathToSearch = wxEmptyString );

    void SaveLastVisitedLibraryPath( const wxString& aPath );

    /**
     * Function  ReturnFilenameWithRelativePathInLibPath
     * @return a short filename (with extension) with only a relative path if
     *         this filename can be found in library paths
     * @param aFullFilename The filename with path and extension.
     */
    wxString ReturnFilenameWithRelativePathInLibPath( const wxString& aFullFilename );

    /**
     * Function RemoveLibraryPath
     * Removes the given path(s) from the library path list
     * @param aPaths = path or path list to remove. paths must be separated by
     * ";"
     */
    void RemoveLibraryPath( const wxString& aPaths );

    /**
     * Function InsertLibraryPath
     * insert path(s) int lib paths list.
     * @param aPaths = path or path list to add. paths must be separated by ";"
     * @param aIndex = insertion point
     */
    void InsertLibraryPath( const wxString& aPaths, size_t aIndex );
};

/*
 * Use wxGetApp() to access EDA_APP.  It is not necessary to keep copies
 * of the application pointer all over the place or worse yet in a global
 * variable.
 */
DECLARE_APP( EDA_APP )

#endif  /* APPL_WXSTRUCT_H */
