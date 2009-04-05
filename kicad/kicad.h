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

#include "wxstruct.h"
#include "appl_wxstruct.h"


class WinEDA_CommandFrame;
class WinEDA_TreePrj;
class WinEDA_PrjFrame;



/*******************************************/
/* classe pour la Fenetre generale de kicad*/
/*******************************************/

/* class WinEDA_MainFrame
 * This is the main kicad frame
 */
class WinEDA_MainFrame : public WinEDA_BasicFrame
{
    /* This class is the main entry point of the py API */
public:

    WinEDA_CommandFrame* m_CommandWin;
    WinEDA_PrjFrame*     m_LeftWin;
    wxSashLayoutWindow*  m_BottomWin;
    wxTextCtrl*          m_DialogWin;
    WinEDA_Toolbar*      m_VToolBar; // Verticam Toolbar (not used)
    wxString             m_BoardFileName;
    wxString             m_SchematicRootFileName;
    wxFileName           m_ProjectFileName;

    int     m_LeftWin_Width;
    int     m_CommandWin_Height;

public:

    // Constructor and destructor
    WinEDA_MainFrame( wxWindow* parent, const wxString& title,
                      const wxPoint& pos, const wxSize& size );

    ~WinEDA_MainFrame();

    void        OnCloseWindow( wxCloseEvent& Event );
    void        OnSize( wxSizeEvent& event );
    void        OnPaint( wxPaintEvent& event );
    void        ReDraw( wxDC* DC );
    void        OnSashDrag( wxSashEvent& event );
    void        OnLoadProject( wxCommandEvent& event );
    void        OnSaveProject( wxCommandEvent& event );
    void        OnArchiveFiles( wxCommandEvent& event );
    void        OnUnarchiveFiles( wxCommandEvent& event );
    void        OnRunPcbNew( wxCommandEvent& event );
    void        OnRunCvpcb( wxCommandEvent& event );
    void        OnRunEeschema( wxCommandEvent& event );
    void        OnRunGerbview( wxCommandEvent& event );

#ifdef KICAD_PYTHON
    void        OnRunPythonScript( wxCommandEvent& event );
#endif

    void        OnOpenTextEditor( wxCommandEvent& event );
    void        OnOpenFileInTextEditor( wxCommandEvent& event );
    void        OnOpenFileInEditor( wxCommandEvent& event );

    void        OnFileHistory( wxCommandEvent& event );
    void        OnExit( wxCommandEvent& event );
    void        Process_Preferences( wxCommandEvent& event );
    void        ReCreateMenuBar();
    void        RecreateBaseHToolbar();
    void        PrintMsg( const wxString& text );
    void        ClearMsg();
    void        SetLanguage( wxCommandEvent& event );
    void        OnRefresh( wxCommandEvent& event );
    void        OnSelectDefaultPdfBrowser( wxCommandEvent& event );
    void        OnSelectPreferredPdfBrowser( wxCommandEvent& event );
    void        OnSelectPreferredEditor( wxCommandEvent& event );
    void        OnSelectFont( wxCommandEvent& event );

    void        OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event );
    void        OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event );

    void        CreateNewProject( const wxString PrjFullFileName );

    void        LoadSettings();
    void        SaveSettings();

#ifdef KICAD_PYTHON
    void        OnRefreshPy();

    boost::python::object   GetPrjName() const;

    WinEDA_MainFrame( const WinEDA_MainFrame& ) { }

    WinEDA_MainFrame() { }

    boost::python::object   ToWx();
    void                    AddFastLaunchPy( boost::python::object& button );
    WinEDA_PrjFrame*        GetTree() const;

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


/**************************************************************/
/* class WinEDA_CommandFrame: window handling command buttons */
/**************************************************************/

/** class WinEDA_CommandFrame
 * This is the window handling the main tools to launch eeschema, cvpcb, pcbnew and gerbview
 */
class WinEDA_CommandFrame : public wxSashLayoutWindow
{
public:
    WinEDA_CommandFrame( wxWindow* parent, int id, wxPoint pos, wxSize size, long style );
    ~WinEDA_CommandFrame()
    { }

    /** Function AddFastLaunch
      * add a Bitmap Button (fast launch button) to the window
     */
public: void AddFastLaunch( wxBitmapButton * button );
private:

    /** Function CreateCommandToolbar
      * Create the main buttons (fast launch buttons)
     */
    void    CreateCommandToolbar( void );

private:
    wxPoint m_ButtonLastPosition;   /** position of the last button in the window */
    int     m_ButtonSeparation;     /** button distance in pixels */
};


/***********************************************************/
/* Classes pour l'arbre de hierarchie de gestion du projet */
/***********************************************************/

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

    TreePrjItemData( TreeFileType type, const wxString& data, wxTreeCtrl* parent );
    TreePrjItemData() : m_Parent( NULL ) { }

    TreePrjItemData( const TreePrjItemData& src ) :
        m_Type( src.m_Type )
        , m_FileName( src.m_FileName )
        , m_Parent( src.m_Parent )
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


    wxString    GetDir() const;

    void        OnRename( wxTreeEvent& event, bool check = true );
    bool        Rename( const wxString& name, bool check = true );
    bool        Delete( bool check = true );
    void        Move( TreePrjItemData* dest );
    void        Activate(WinEDA_PrjFrame* prjframe);

    const wxMenu* GetMenu()
    {
        return &m_fileMenu;
    }


    void                    SetState( int state );

#ifdef KICAD_PYTHON
    boost::python::object   GetFileNamePy() const;
    bool                    RenamePy( const boost::python::str& newname, bool check = true );

    boost::python::object   GetDirPy() const;

    boost::python::object   GetIdPy() const;

    boost::python::object   GetMenuPy();

#endif
};

/** class WinEDA_PrjFrame
 * Window to display the tree files
 */
class WinEDA_PrjFrame : public wxSashLayoutWindow
{
private:

    std::vector<wxMenu*>    m_ContextMenus;
    std::vector<wxString>   m_Filters;

    wxMenu*  m_PopupMenu;
    wxCursor m_DragCursor;
    wxCursor m_Default;

protected:
    wxMenu*             GetContextMenu( int type );
    void                NewFile( TreeFileType type );
    void                NewFile( const wxString& name, TreeFileType type, wxTreeItemId& root );
    TreePrjItemData*    GetSelectedData();

public:
    WinEDA_MainFrame* m_Parent;
    WinEDA_TreePrj*   m_TreeProject;

    wxTreeItemId      m_root;

public:
    static wxString                 GetFileExt( TreeFileType type );
    static wxString                 GetFileWildcard( TreeFileType type );

    WinEDA_PrjFrame( WinEDA_MainFrame* parent,
                     const wxPoint& pos, const wxSize& size );
    ~WinEDA_PrjFrame() { }
    void                            OnSelect( wxTreeEvent& Event );
    void                            OnRenameAsk( wxTreeEvent& Event );
    void                            OnRename( wxTreeEvent& Event );
    void                            OnDragStart( wxTreeEvent& event );
    void                            OnDragEnd( wxTreeEvent& event );
    void                            OnRight( wxTreeEvent& Event );
    void                            ReCreateTreePrj();

    void                            OnTxtEdit( wxCommandEvent& event );

    void                            OnDeleteFile( wxCommandEvent& event );
    void                            OnRenameFile( wxCommandEvent& event );

    void                            OnNewFile( wxCommandEvent& event );
    void                            OnNewDirectory( wxCommandEvent& event );
    void                            OnNewSchFile( wxCommandEvent& event );
    void                            OnNewBrdFile( wxCommandEvent& event );
    void                            OnNewPyFile( wxCommandEvent& event );
    void                            OnNewGerberFile( wxCommandEvent& event );
    void                            OnNewTxtFile( wxCommandEvent& event );
    void                            OnNewNetFile( wxCommandEvent& event );

    void                            ClearFilters();

    const std::vector<wxString>&    GetFilters();
    void                            RemoveFilter( const wxString& filter );

#ifdef KICAD_PYTHON
    boost::python::object           ToWx();

    WinEDA_PrjFrame()
    {
    }


    WinEDA_PrjFrame( const WinEDA_PrjFrame& )
    {
    }


    void                    OnRunPy( wxCommandEvent& event );

    boost::python::object GetMenuPy( TreeFileType );

    boost::python::object GetFtExPy( TreeFileType ) const;

    void                    RemoveFilterPy( const boost::python::str& filter );
    void                    AddFilter( const boost::python::str& filter );

    boost::python::object   GetTreeCtrl();
    TreePrjItemData*        GetItemData( const boost::python::object& item );
    void                    AddFilePy( const boost::python::str& name, boost::python::object& root );
    void                    NewFilePy( const boost::python::str& name,
                                       TreeFileType              type,
                                       boost::python::object&    root );

    TreePrjItemData*        FindItemData( const boost::python::str& name );

    boost::python::object   GetCurrentMenu();
    int                     AddStatePy( boost::python::object& bitmap );

#endif

    bool                    AddFile( const wxString& name, wxTreeItemId& root );

    DECLARE_EVENT_TABLE()
};


/** Class TreeCtrl
 * This is the class to show (as a tree) the files in the project directory
 */
class WinEDA_TreePrj : public wxTreeCtrl
{
    DECLARE_DYNAMIC_CLASS( WinEDA_TreePrj )
private:
    WinEDA_PrjFrame* m_Parent;
    wxImageList*     m_ImageList;

public:

    WinEDA_PrjFrame* GetParent()
    {
        return m_Parent;
    }


    WinEDA_TreePrj( WinEDA_PrjFrame* parent );
    ~WinEDA_TreePrj();
private:
    /* overlayed sort function */
    int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 );
};

#endif
