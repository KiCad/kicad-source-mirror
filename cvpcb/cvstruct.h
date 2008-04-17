/***********************************************************/
/*						wxstruct.h:							 */
/* descriptions des principales classes derivees utilisees */
/***********************************************************/

#ifndef CVSTRUCT_H
#define CVSTRUCT_H

#ifndef eda_global
#define eda_global extern
#endif


#include "wx/listctrl.h"

/*  Forward declarations of all top-level window classes. */
class FootprintListBox;
class ListBoxCmp;
class WinEDA_DisplayFrame;
class STORECMP;

#define LIST_BOX_TYPE wxListView

/******************************************************/
/* classe derivee pour la Fenetre principale de cvpcb */
/******************************************************/

class WinEDA_CvpcbFrame : public WinEDA_BasicFrame
{
public:

    FootprintListBox*    m_FootprintList;
    ListBoxCmp*          m_ListCmp;
    WinEDA_DisplayFrame* DrawFrame;
    WinEDA_Toolbar*      m_HToolBar; // Toolbar horizontal haut d'ecran

private:
    wxMenu* m_FilesMenu;

    // Constructor and destructor
public:
    WinEDA_CvpcbFrame( WinEDA_App * parent, const wxString &title,
        long style = KICAD_DEFAULT_DRAWFRAME_STYLE );
    ~WinEDA_CvpcbFrame();

    void            OnLeftClick( wxListEvent& event );
    void            OnLeftDClick( wxListEvent& event );
    void            OnSelectComponent( wxListEvent& event );

    void            Update_Config( wxCommandEvent& event ); /* enregistrement de la config */
    void            OnQuit( wxCommandEvent& event );
    void            OnCloseWindow( wxCloseEvent& Event );
    void            OnSize( wxSizeEvent& SizeEvent );
    void            OnChar( wxKeyEvent& event );
    void            ReCreateHToolbar();
    virtual void    ReCreateMenuBar();
    void            SetLanguage( wxCommandEvent& event );
    void            AddFontSelectionMenu( wxMenu* main_menu );
    void            ProcessFontPreferences( wxCommandEvent& event );

    void            ToFirstNA( wxCommandEvent& event );
    void            ToPreviousNA( wxCommandEvent& event );
    void            DelAssociations( wxCommandEvent& event );
    void            SaveQuitCvpcb( wxCommandEvent& event );
    void            LoadNetList( wxCommandEvent& event );
    void            ConfigCvpcb( wxCommandEvent& event );
    void            DisplayModule( wxCommandEvent& event );
    void            AssocieModule( wxCommandEvent& event );
    void            WriteStuffList( wxCommandEvent& event );
    void            DisplayDocFile( wxCommandEvent& event );
    void            OnSelectFilteringFootprint( wxCommandEvent& event );
    void            SetNewPkg( const wxString& package );
    void            BuildCmpListBox();
    void            BuildFootprintListBox();
    void            CreateScreenCmp();
    void            CreateConfigWindow();
    int             SaveNetList( const wxString& FullFileName );
    int             SaveComponentList( const wxString& FullFileName );
    bool            ReadInputNetList( const wxString& FullFileName );
    void            ReadNetListe();
    int             rdpcad();
    int             ReadSchematicNetlist();
    int             ReadFootprintFilterList( FILE* f );
    int             ReadViewlogicWirList();
    int             ReadViewlogicNetList();

    DECLARE_EVENT_TABLE()
};


/***********************************************/
/* ListBox derivee pour l'affichage des listes */
/***********************************************/
class ListBoxBase : public LIST_BOX_TYPE
{
public:
    WinEDA_CvpcbFrame* m_Parent;

public:

    ListBoxBase( WinEDA_CvpcbFrame * parent, wxWindowID id,
        const wxPoint &loc, const wxSize &size );

    ~ListBoxBase();

    int     GetSelection();
    void    OnSize( wxSizeEvent& event );
};

/************************************************************/
/* ListBox derivee pour l'affichage de la liste des Modules */
/************************************************************/

class FootprintListBox : public ListBoxBase
{
private:
    wxArrayString  m_FullFootprintList;
    wxArrayString  m_FilteredFootprintList;
public:
    wxArrayString* m_ActiveFootprintList;
    bool           m_UseFootprintFullList;

public:
    FootprintListBox( WinEDA_CvpcbFrame * parent,
        wxWindowID id, const wxPoint &loc, const wxSize &size,
        int nbitems, wxString choice[] );
    ~FootprintListBox();

    int         GetCount();
    void        SetSelection( unsigned index, bool State = TRUE );
    void        SetString( unsigned linecount, const wxString& text );
    void        AppendLine( const wxString& text );
    void        SetFootprintFullList();
    void        SetFootprintFilteredList( STORECMP* Component );
    void        SetActiveFootprintList( bool FullList, bool Redraw = FALSE );

    wxString    GetSelectedFootprint();
    wxString    OnGetItemText( long item, long column ) const;
    void        OnLeftClick( wxListEvent& event );
    void        OnLeftDClick( wxListEvent& event );

    DECLARE_EVENT_TABLE()
};

/***************************************************************/
/* ListBox derivee pour l'affichage de la liste des Composants */
/***************************************************************/

class ListBoxCmp : public ListBoxBase
{
public:
    wxArrayString      m_ComponentList;
    WinEDA_CvpcbFrame* m_Parent;

public:

    ListBoxCmp( WinEDA_CvpcbFrame * parent, wxWindowID id,
        const wxPoint &loc, const wxSize &size,
        int nbitems, wxString choice[] );

    ~ListBoxCmp();

    void        Clear();
    int         GetCount();
    wxString    OnGetItemText( long item, long column ) const;
    void        SetSelection( unsigned index, bool State = TRUE );
    void        SetString( unsigned linecount, const wxString& text );
    void        AppendLine( const wxString& text );

    DECLARE_EVENT_TABLE()
};


/*******************************************************/
/* class WWinEDA_DisplayFrame: public WinEDA_DrawFrame */
/*******************************************************/

class WinEDA_DisplayFrame : public WinEDA_BasePcbFrame
{
public:

public:
    WinEDA_DisplayFrame( wxWindow * father, WinEDA_App * parent,
        const wxString &title,
        const wxPoint &pos, const wxSize &size,
        long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_DisplayFrame();

    void    OnCloseWindow( wxCloseEvent& Event );
    void    Process_Special_Functions( wxCommandEvent& event );
    void    RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void    ReCreateHToolbar();
    void    ReCreateVToolbar();
    void    RecreateMenuBar();
    void    OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void    SetToolbars();
    void    GeneralControle( wxDC* DC, wxPoint Mouse );
    void    InstallOptionsDisplay( wxCommandEvent& event );
    MODULE* Get_Module( const wxString& CmpName );

    void    Process_Settings( wxCommandEvent& event );
    void    Show3D_Frame( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


#endif  //#ifndef CVSTRUCT_H
