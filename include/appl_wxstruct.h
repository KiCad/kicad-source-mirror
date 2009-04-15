/************************************************************/
/*						appl_wxstruct.h:					*/
/* descriptions des principales classes derivees utilisees: */
/*    Class "EDA_Appl: classe de l'application generale		*/
/************************************************************/

/* Ce fichier doit etre inclus dans "wxstruct.h"
 */

#ifndef  APPL_WXSTRUCT_H
#define  APPL_WXSTRUCT_H

/* Use wxFileHistory for most recently used file handling. */
#include <wx/docview.h>
#include <wx/config.h>
#include <wx/filename.h>

enum id_app_type {
    APP_TYPE_UNKOWN,
    APP_TYPE_EESCHEMA,
    APP_TYPE_PCBNEW,
    APP_TYPE_CVPCB,
    APP_TYPE_GERBVIEW,
    APP_TYPE_KICAD,
};

class wxConfigBase;
class wxFileConfig;
class PARAM_CFG_BASE;
class wxSingleInstanceChecker;
class wxHtmlHelpController;


/**********************************************/
/*  Class representing the entire Application */
/**********************************************/

class WinEDA_App : public wxApp
{
public:
    id_app_type              m_Id;              /* Used mainly to handle default paths libs
                                                 *  m_Id = APP_TYPE_EESCHEMA, APP_TYPE_PCBNEW ...
                                                 */
    wxString                 m_Project;
    wxSingleInstanceChecker* m_Checker;

    wxPoint                  m_HelpPos;
    wxSize                   m_HelpSize;
    wxHtmlHelpController*    m_HtmlCtrl;
    wxConfig*                m_EDA_Config;
    wxConfig*                m_EDA_CommonConfig;
    wxFileConfig*            m_ProjectConfig;
    wxString                 m_HelpFileName;
    wxString                 m_EditorName;
    wxString                 m_CurrentOptionFile; // dernier fichier .cnf utilisé
    wxString                 m_CurrentOptionFileDateAndTime;

    wxString                 m_BinDir; /* Chemin ou reside l'executable
                                        *  (utilisé si KICAD non défini)*/
    wxString                 m_KicadEnv;  /* Chemin de kicad défini dans la
                                           * variable d'environnement KICAD,
                                           * typiquement /usr/local/kicad ou
                                           * c:\kicad */
    bool          m_Env_Defined;                        // TRUE si variable d'environnement KICAD definie

    wxLocale*     m_Locale;                             // Gestion de la localisation
    int           m_LanguageId;                         // indicateur de choix du langage ( 0 = defaut)
    wxString      m_PdfBrowser;                         // Name of the selected browser, for browsing pdf datasheets
    bool          m_PdfBrowserIsDefault;                // True if the pdf browser is the default (m_PdfBrowser not used)
    wxPathList    m_searchPaths;
    wxFileHistory m_fileHistory;

protected:
    wxString      m_Title;
    wxPathList    m_libSearchPaths;
    wxFileName    m_projectFileName;

public:
    WinEDA_App();
    ~WinEDA_App();
    bool      OnInit();
    int       OnRun();

    bool      SetBinDir();
    void      SetDefaultSearchPaths( void );

    /** Function InitEDA_Appl
     * initialise some general parameters
     *  - Default paths (help, libs, bin)and configuration files names
     *  - Language and locale
     *  - fonts
     * @param aName : used as paths in configuration files
     * @param aId = flag : LIBRARY_TYPE_EESCHEMA or LIBRARY_TYPE_PCBNEW
     *    used to choose what default library path must be used
     */
    void      InitEDA_Appl( const wxString& aName, id_app_type aId = APP_TYPE_UNKOWN);

    bool      SetLanguage( bool first_time = FALSE );

    /** Function AddMenuLanguageList
     *
     * Create menu list for language choice, and add it as submenu to a main
     * menu
     *
     * @param   MasterMenu : The main menu. The sub menu list will be accessible
     *          from the menu item with id ID_LANGUAGE_CHOICE
     *
     * @return  the sub menu Language list
     */
    void      AddMenuLanguageList( wxMenu* MasterMenu );
    void      SetLanguageIdentifier( int menu_id );
    void      SetLanguagePath( void );
    void      InitOnLineHelp();

    // Sauvegarde de configurations et options:
    void      GetSettings();
    void      SaveSettings();

    void      WriteProjectConfig( const wxString&  local_config_filename,
                                  const wxString&  GroupName,
                                  PARAM_CFG_BASE** List );

    /** Function SaveCurrentSetupValues()
     * Save the current setup values in m_EDA_Config
     * saved parameters are parameters that have the .m_Setup member set to true
     * @param aList = array of PARAM_CFG_BASE pointers
     */
    void      SaveCurrentSetupValues( PARAM_CFG_BASE** aList );

    /** Function ReadCurrentSetupValues()
     * Raed the current setup values previously saved, from m_EDA_Config
     * saved parameters are parameters that have the .m_Setup member set to true
     * @param aList = array of PARAM_CFG_BASE pointers
     */
    void      ReadCurrentSetupValues( PARAM_CFG_BASE** aList );

    bool      ReadProjectConfig( const wxString& local_config_filename,
                                 const wxString& GroupName, PARAM_CFG_BASE** List,
                                 bool Load_Only_if_New );
    bool      ReCreatePrjConfig( const wxString& local_config_filename,
                                 const wxString& GroupName,
                                 bool            ForceUseLocalConfig );

    void      ReadPdfBrowserInfos();
    void      WritePdfBrowserInfos();

    wxString  FindFileInSearchPaths( const wxString&      filename,
                                     const wxArrayString* subdirs = NULL );

    wxString  GetHelpFile( void );
    wxString  GetLibraryFile( const wxString& filename );
    wxString& GetEditorName();

    const wxString& GetTitle() { return m_Title; }
    void SetTitle( const wxString& title ) { m_Title = title; }

    wxPathList& GetLibraryPathList() { return m_libSearchPaths; }
    wxString FindLibraryPath( const wxString& fileName );

    /** FindLibraryPath
     * Kicad saves user defined library files that are not in the standard
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

    /** ReturnLastVisitedLibraryPath
     * Returns the last visited library directory, or (if void) the first
     * path in lib path list ( but not the CWD )
     * @param aSubPathToSearch = Prefered sub path to search in path list
     */
    wxString ReturnLastVisitedLibraryPath( const wxString & aSubPathToSearch = wxEmptyString);
    void SaveLastVisitedLibraryPath( const wxString & aPath);

    /** ReturnFilenameWithRelativePathInLibPath
     * @return a short filename (with extension) with only a relative path if this filename
     * can be found in library paths
     * @param aFullFilename = filename with path and extension.
     */
    wxString ReturnFilenameWithRelativePathInLibPath(const wxString & aFullFilename);
    
    /** Function RemoveLibraryPath
     * Removes the given ptah from the libary path list
     * @param path = the path to remove
     */
    void RemoveLibraryPath( const wxString& path );
    void InsertLibraryPath( const wxString& path, size_t index );
};

/*
 * Use wxGetApp() to access WinEDA_App.  It is not necessary to keep copies
 * of the application pointer all over the place or worse yet in a global
 * variable.
 */
DECLARE_APP( WinEDA_App );

#endif  /* APPL_WXSTRUCT_H */
