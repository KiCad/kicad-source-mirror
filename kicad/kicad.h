/***********/
/* kicad.h */
/***********/

#ifndef KICAD_H
#define KICAD_H

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include <vector>

#include <wx/treectrl.h>
#include <wx/dragimag.h>
#include <wx/filename.h>

#include "id.h"
#include "wxstruct.h"
#include "appl_wxstruct.h"

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
    ID_PROJECT_RUNPY,
    ID_PROJECT_NEWFILE,
    ID_PROJECT_NEWPY,
    ID_PROJECT_NEWTXT,
    ID_PROJECT_NEWDIR,
    ID_PROJECT_DELETE,
    ID_PROJECT_RENAME,
    ID_PROJECT_OPEN_FILE_WITH_TEXT_EDITOR,

    ID_TO_EDITOR,
    ID_TO_EESCHEMA,
    ID_TO_GERBVIEW,
    ID_RUN_PYTHON,
    ID_BROWSE_AN_SELECT_FILE,
    ID_SELECT_PREFERED_EDITOR,
    ID_SELECT_PREFERED_PDF_BROWSER_NAME,
    ID_SELECT_PREFERED_PDF_BROWSER,
    ID_SELECT_DEFAULT_PDF_BROWSER,
    ID_SAVE_AND_ZIP_FILES,
    ID_READ_ZIP_ARCHIVE,
};


/* class WinEDA_MainFrame
 * This is the main kicad frame
 */
class WinEDA_MainFrame : public WinEDA_BasicFrame
{
    /* This class is the main entry point of the py API */
public:
    TREE_PROJECT_FRAME* m_LeftWin;
    RIGHT_KM_FRAME*  m_RightWin;
    WinEDA_Toolbar*  m_VToolBar;     // Vertical toolbar (not used)
    wxString         m_BoardFileName;
    wxString         m_SchematicRootFileName;
    wxFileName       m_ProjectFileName;

private:
    int m_LeftWin_Width;

public:

    WinEDA_MainFrame( wxWindow* parent, const wxString& title,
                      const wxPoint& pos, const wxSize& size );

    ~WinEDA_MainFrame();

    /** Function CreateCommandToolbar
     * Create the main buttons (fast launch buttons)
     */
    void                  OnCloseWindow( wxCloseEvent& Event );
    void                  OnSize( wxSizeEvent& event );
    void                  OnSashDrag( wxSashEvent& event );
    void                  OnLoadProject( wxCommandEvent& event );
    void                  OnSaveProject( wxCommandEvent& event );
    void                  OnArchiveFiles( wxCommandEvent& event );
    void                  OnUnarchiveFiles( wxCommandEvent& event );
    void                  OnRunPcbNew( wxCommandEvent& event );
    void                  OnRunCvpcb( wxCommandEvent& event );
    void                  OnRunEeschema( wxCommandEvent& event );
    void                  OnRunGerbview( wxCommandEvent& event );

#ifdef KICAD_PYTHON
    void                  OnRunPythonScript( wxCommandEvent& event );

#endif

    void                  OnOpenTextEditor( wxCommandEvent& event );
    void                  OnOpenFileInTextEditor( wxCommandEvent& event );
    void                  OnOpenFileInEditor( wxCommandEvent& event );

    void                  OnFileHistory( wxCommandEvent& event );
    void                  OnExit( wxCommandEvent& event );
    void                  Process_Preferences( wxCommandEvent& event );
    void                  ReCreateMenuBar();
    void                  RecreateBaseHToolbar();
    void                  PrintMsg( const wxString& text );
    void                  ClearMsg();
    void                  SetLanguage( wxCommandEvent& event );
    void                  OnRefresh( wxCommandEvent& event );
    void                  OnSelectDefaultPdfBrowser( wxCommandEvent& event );
    void                  OnSelectPreferredPdfBrowser( wxCommandEvent& event );
    void                  OnSelectPreferredEditor( wxCommandEvent& event );

    void                  OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event );
    void                  OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event );

    void                  CreateNewProject( const wxString PrjFullFileName );

    void                  LoadSettings();
    void                  SaveSettings();

#ifdef KICAD_PYTHON
    void                  OnRefreshPy();

    boost::python::object GetPrjName() const;

    WinEDA_MainFrame( const WinEDA_MainFrame& ) { }

    WinEDA_MainFrame() { }

    boost::python::object ToWx();
    void                  AddFastLaunchPy( boost::python::object& button );
    TREE_PROJECT_FRAME*      GetTree() const;

#endif

    DECLARE_EVENT_TABLE()
};

// Order of this enum changes AddFile() internal working
// please update both
enum TreeFileType {
    TREE_PROJECT = 1,
    TREE_SCHEMA,
    TREE_PCB,
    TREE_PY,
    TREE_GERBER,
    TREE_PDF,
    TREE_TXT,
    TREE_NET,
    TREE_UNKNOWN,
    TREE_DIRECTORY,
    TREE_MAX,
};

/** class RIGHT_KM_FRAME
 */
class RIGHT_KM_FRAME : public wxSashLayoutWindow
{
public:
    wxTextCtrl*      m_DialogWin;
private:
    WinEDA_MainFrame* m_Parent;
    int m_ButtonsPanelHeight;
    wxPanel*          m_ButtPanel;
    wxPoint           m_ButtonLastPosition;     /* position of the last button in the window */
    int m_ButtonSeparation;                     /* button distance in pixels */

public:
    RIGHT_KM_FRAME( WinEDA_MainFrame* parent );
    ~RIGHT_KM_FRAME() { };
    void OnSize( wxSizeEvent& event );

private:
    void CreateCommandToolbar( void );
    void AddFastLaunch( wxBitmapButton* button );

    DECLARE_EVENT_TABLE()
};


/*********************************/
/* Classes for the project tree. */
/*********************************/

/** class TreePrjItemData
 * Handle one item (a file or a directory name) for the tree file
 */
class TreePrjItemData : public wxTreeItemData
{
public:
    TreeFileType m_Type;
    bool         m_IsRootFile;  // True if m_Filename is a root schematic (same name as project)
    wxString     m_FileName;    // Filename for a file, or directory name

private:
    wxTreeCtrl*  m_Parent;
    wxMenu       m_fileMenu;
    int          m_State;

public:

    TreePrjItemData( TreeFileType type, const wxString& data,
                     wxTreeCtrl* parent );
    TreePrjItemData() : m_Parent( NULL ) { }

    TreePrjItemData( const TreePrjItemData& src ) :
        m_Type( src.m_Type ),
        m_FileName( src.m_FileName ),
        m_Parent( src.m_Parent )
    {
        SetState( src.m_State );
    }


    TreeFileType GetType() const
    {
        return m_Type;
    }


    wxString GetFileName() const
    {
        return m_FileName;
    }


    void SetFileName( const wxString& name )
    {
        m_FileName = name;
    }


    wxString GetDir() const;

    void     OnRename( wxTreeEvent& event, bool check = true );
    bool     Rename( const wxString& name, bool check = true );
    bool     Delete( bool check = true );
    void     Move( TreePrjItemData* dest );
    void     Activate( TREE_PROJECT_FRAME* prjframe );

    const wxMenu* GetMenu()
    {
        return &m_fileMenu;
    }


    void                  SetState( int state );

#ifdef KICAD_PYTHON
    boost::python::object GetFileNamePy() const;
    bool                  RenamePy( const boost::python::str& newname,
                                    bool                      check = true );

    boost::python::object GetDirPy() const;

    boost::python::object GetIdPy() const;

    boost::python::object GetMenuPy();

#endif
};

#endif
