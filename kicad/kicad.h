/***********/
/* kicad.h */
/***********/

#ifndef KICAD_H
#define KICAD_H

#include <vector>

#include <wx/treectrl.h>
#include <wx/dragimag.h>
#include <wx/filename.h>

#include "id.h"
#include "wxstruct.h"
#include "appl_wxstruct.h"

extern const wxString g_KicadPrjFilenameExtension;

class RIGHT_KM_FRAME;
class TREEPROJECTFILES;
class TREE_PROJECT_FRAME;


/**
 * Command IDs for Kicad.
 *
 * Please add IDs that are unique to Kicad  here and not in the global id.h
 * file.  This will prevent the entire project from being rebuilt when adding
 * new commands to Kicad.
 */

enum id_kicad_frm {
    ID_LEFT_FRAME = ID_END_LIST,
    ID_PROJECT_TREE,
    ID_PROJECT_TXTEDIT,
    ID_PROJECT_TREE_REFRESH,
    ID_PROJECT_NEWDIR,
    ID_PROJECT_DELETE,
    ID_PROJECT_RENAME,
    ID_PROJECT_OPEN_FILE_WITH_TEXT_EDITOR,

    ID_TO_EDITOR,
    ID_TO_EESCHEMA,
    ID_TO_GERBVIEW,
    ID_TO_BITMAP_CONVERTER,
    ID_TO_PCB_CALCULATOR,
    ID_BROWSE_AN_SELECT_FILE,
    ID_SELECT_PREFERED_EDITOR,
    ID_SELECT_PREFERED_PDF_BROWSER_NAME,
    ID_SELECT_PREFERED_PDF_BROWSER,
    ID_SELECT_DEFAULT_PDF_BROWSER,
    ID_SAVE_AND_ZIP_FILES,
    ID_READ_ZIP_ARCHIVE
};


/* class KICAD_MANAGER_FRAME
 * This is the main kicad frame
 */
class KICAD_MANAGER_FRAME : public EDA_BASE_FRAME
{
public:
    TREE_PROJECT_FRAME* m_LeftWin;
    RIGHT_KM_FRAME*     m_RightWin;
    EDA_TOOLBAR*        m_VToolBar;  // Vertical toolbar (not used)
    wxString            m_BoardFileName;
    wxString            m_SchematicRootFileName;
    wxFileName          m_ProjectFileName;

private:
    int m_LeftWin_Width;

public:

    KICAD_MANAGER_FRAME( wxWindow* parent, const wxString& title,
                         const wxPoint& pos, const wxSize& size );

    ~KICAD_MANAGER_FRAME();

    void OnCloseWindow( wxCloseEvent& Event );
    void OnSize( wxSizeEvent& event );
    void OnSashDrag( wxSashEvent& event );

    /**
     * Function OnLoadProject
     * loads an exiting or creates a new project (.pro) file.
     */
    void OnLoadProject( wxCommandEvent& event );

    /**
     * Function OnSaveProject
     * is the command event hendler to Save the project (.pro) file containing the top level
     * configuration parameters.
     */
    void OnSaveProject( wxCommandEvent& event );

    void OnArchiveFiles( wxCommandEvent& event );
    void OnUnarchiveFiles( wxCommandEvent& event );
    void OnRunPcbNew( wxCommandEvent& event );
    void OnRunCvpcb( wxCommandEvent& event );
    void OnRunEeschema( wxCommandEvent& event );
    void OnRunGerbview( wxCommandEvent& event );
    void OnRunBitmapConverter( wxCommandEvent& event );
    void OnRunPcbCalculator( wxCommandEvent& event );

    void OnOpenTextEditor( wxCommandEvent& event );
    void OnOpenFileInTextEditor( wxCommandEvent& event );
    void OnOpenFileInEditor( wxCommandEvent& event );

    void OnFileHistory( wxCommandEvent& event );
    void OnExit( wxCommandEvent& event );
    void Process_Preferences( wxCommandEvent& event );
    void ReCreateMenuBar();
    void RecreateBaseHToolbar();

    /**
     * Function PrintMsg
     * displays \a aText in the text panel.
     *
     * @param aText The text to display.
     */
    void PrintMsg( const wxString& aText );

    void ClearMsg();
    void SetLanguage( wxCommandEvent& event );
    void OnRefresh( wxCommandEvent& event );
    void OnSelectDefaultPdfBrowser( wxCommandEvent& event );
    void OnSelectPreferredPdfBrowser( wxCommandEvent& event );

    void OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event );
    void OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event );

    void CreateNewProject( const wxString PrjFullFileName );

    /**
     * Function LoadSettings
     * loads the Kicad main frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get loaded.
     */
    void LoadSettings();

    /**
     * Function SaveSettings
     * saves the Kicad main frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get saved.
     */
    void SaveSettings();

    DECLARE_EVENT_TABLE()
};

// Order of this enum changes AddFile() internal working
// please update both
enum TreeFileType {
    TREE_PROJECT = 1,
    TREE_SCHEMA,
    TREE_PCB,
    TREE_GERBER,
    TREE_PDF,
    TREE_TXT,
    TREE_NET,
    TREE_UNKNOWN,
    TREE_DIRECTORY,
    TREE_MAX
};

/** class RIGHT_KM_FRAME
 */
class RIGHT_KM_FRAME : public wxSashLayoutWindow
{
public:
    wxTextCtrl*       m_DialogWin;
private:
    KICAD_MANAGER_FRAME* m_Parent;
    int m_ButtonsPanelHeight;
    wxPanel*          m_ButtPanel;
    int m_ButtonSeparation;                 // button distance in pixels
    wxPoint           m_ButtonsListPosition; /* position of the left bottom corner
                                              *  of the first bitmap button
                                              */
    wxPoint           m_ButtonLastPosition; // position of the last button in the window

public:
    RIGHT_KM_FRAME( KICAD_MANAGER_FRAME* parent );
    ~RIGHT_KM_FRAME() { };
    void OnSize( wxSizeEvent& event );

private:
    /**
     * Function CreateCommandToolbar
     * creates the main tool bar buttons (fast launch buttons)
     */
    void CreateCommandToolbar( void );

    wxBitmapButton* AddBitmapButton( wxWindowID aId, const wxBitmap & aBitmap );

    DECLARE_EVENT_TABLE()
};

#endif
