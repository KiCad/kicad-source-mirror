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
    ID_BROWSE_AN_SELECT_FILE,
    ID_SELECT_PREFERED_EDITOR,
    ID_SELECT_PREFERED_PDF_BROWSER_NAME,
    ID_SELECT_PREFERED_PDF_BROWSER,
    ID_SELECT_DEFAULT_PDF_BROWSER,
    ID_SAVE_AND_ZIP_FILES,
    ID_READ_ZIP_ARCHIVE
};


/* class WinEDA_MainFrame
 * This is the main kicad frame
 */
class WinEDA_MainFrame : public EDA_BASE_FRAME
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

    WinEDA_MainFrame( wxWindow* parent, const wxString& title,
                      const wxPoint& pos, const wxSize& size );

    ~WinEDA_MainFrame();

    /**
     * Function CreateCommandToolbar
     * Create the main buttons (fast launch buttons)
     */
    void OnCloseWindow( wxCloseEvent& Event );
    void OnSize( wxSizeEvent& event );
    void OnSashDrag( wxSashEvent& event );
    void OnLoadProject( wxCommandEvent& event );
    void OnSaveProject( wxCommandEvent& event );
    void OnArchiveFiles( wxCommandEvent& event );
    void OnUnarchiveFiles( wxCommandEvent& event );
    void OnRunPcbNew( wxCommandEvent& event );
    void OnRunCvpcb( wxCommandEvent& event );
    void OnRunEeschema( wxCommandEvent& event );
    void OnRunGerbview( wxCommandEvent& event );
    void OnRunBitmapConverter( wxCommandEvent& event );

    void OnOpenTextEditor( wxCommandEvent& event );
    void OnOpenFileInTextEditor( wxCommandEvent& event );
    void OnOpenFileInEditor( wxCommandEvent& event );

    void OnFileHistory( wxCommandEvent& event );
    void OnExit( wxCommandEvent& event );
    void Process_Preferences( wxCommandEvent& event );
    void ReCreateMenuBar();
    void RecreateBaseHToolbar();
    void PrintMsg( const wxString& text );
    void ClearMsg();
    void SetLanguage( wxCommandEvent& event );
    void OnRefresh( wxCommandEvent& event );
    void OnSelectDefaultPdfBrowser( wxCommandEvent& event );
    void OnSelectPreferredPdfBrowser( wxCommandEvent& event );

    void OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event );
    void OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event );

    void CreateNewProject( const wxString PrjFullFileName );

    void LoadSettings();
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
    WinEDA_MainFrame* m_Parent;
    int m_ButtonsPanelHeight;
    wxPanel*          m_ButtPanel;
    int m_ButtonSeparation;                 // button distance in pixels
    wxPoint           m_ButtonsListPosition; /* position of the left bottom corner
                                              *  of the first bitmap button
                                              */
    wxPoint           m_ButtonLastPosition; // position of the last button in the window

public:
    RIGHT_KM_FRAME( WinEDA_MainFrame* parent );
    ~RIGHT_KM_FRAME() { };
    void OnSize( wxSizeEvent& event );

private:
    void CreateCommandToolbar( void );
    wxBitmapButton* AddBitmapButton( wxWindowID aId, const wxBitmap & aBitmap );

    DECLARE_EVENT_TABLE()
};

#endif
