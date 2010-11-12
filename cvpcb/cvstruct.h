/*********************************************************/
/*                      cvstruct.h                       */
/*********************************************************/

#ifndef CVSTRUCT_H
#define CVSTRUCT_H

#include "wx/listctrl.h"
#include <wx/filename.h>
#include "param_config.h"
#include "cvpcb.h"

/*  Forward declarations of all top-level window classes. */
class FOOTPRINTS_LISTBOX;
class COMPONENTS_LISTBOX;
class DISPLAY_FOOTPRINTS_FRAME;



/**
 * The CVPcb application main window.
 */
class WinEDA_CvpcbFrame : public WinEDA_BasicFrame
{
public:

    bool m_KeepCvpcbOpen;
    FOOTPRINTS_LISTBOX*  m_FootprintList;
    COMPONENTS_LISTBOX*  m_ListCmp;
    DISPLAY_FOOTPRINTS_FRAME* DrawFrame;
    WinEDA_Toolbar*      m_HToolBar;
    wxFileName           m_NetlistFileName;
    wxArrayString        m_ModuleLibNames;
    wxArrayString        m_AliasLibNames;
    wxString             m_UserLibraryPath;
    wxString             m_NetlistFileExtension;
    wxString             m_DocModulesFileName;
    FOOTPRINT_LIST       m_footprints;
    COMPONENT_LIST       m_components;

protected:
    int             m_undefinedComponentCnt;
    bool            m_modified;
    bool            m_rightJustify;
    bool            m_isEESchemaNetlist;
    PARAM_CFG_ARRAY m_projectFileParams;

public:
    WinEDA_CvpcbFrame( const wxString& title,
                       long            style = KICAD_DEFAULT_DRAWFRAME_STYLE );
    ~WinEDA_CvpcbFrame();

    void             OnLeftClick( wxListEvent& event );
    void             OnLeftDClick( wxListEvent& event );
    void             OnSelectComponent( wxListEvent& event );

    void             Update_Config( wxCommandEvent& event );
    void             OnQuit( wxCommandEvent& event );
    void             OnCloseWindow( wxCloseEvent& Event );
    void             OnSize( wxSizeEvent& SizeEvent );
    void             OnChar( wxKeyEvent& event );
    void             ReCreateHToolbar();
    virtual void     ReCreateMenuBar();
    void             SetLanguage( wxCommandEvent& event );

    void             ToFirstNA( wxCommandEvent& event );
    void             ToPreviousNA( wxCommandEvent& event );
    void             DelAssociations( wxCommandEvent& event );
    void             SaveQuitCvpcb( wxCommandEvent& event );
    void             LoadNetList( wxCommandEvent& event );
    void             ConfigCvpcb( wxCommandEvent& event );
    void             OnKeepOpenOnSave( wxCommandEvent& event );
    void             DisplayModule( wxCommandEvent& event );
    void             AssocieModule( wxCommandEvent& event );
    void             WriteStuffList( wxCommandEvent& event );
    void             DisplayDocFile( wxCommandEvent& event );
    void             OnSelectFilteringFootprint( wxCommandEvent& event );

    void             OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event );

    void             SetNewPkg( const wxString& package );
    void             BuildCmpListBox();
    void             BuildFOOTPRINTS_LISTBOX();
    void             CreateScreenCmp();
    int              SaveNetList( const wxString& FullFileName );
    int              SaveComponentList( const wxString& FullFileName );
    bool             ReadNetList();
    int              ReadSchematicNetlist();
    void             LoadProjectFile( const wxString& FileName );
    void             SaveProjectFile( const wxString& fileName );
    virtual void     LoadSettings();
    virtual void     SaveSettings();
    /** DisplayStatus()
     * Displays info to the status line at bottom of the main frame
     */
    void             DisplayStatus();

    PARAM_CFG_ARRAY& GetProjectFileParameters( void );

    DECLARE_EVENT_TABLE()
};


/*********************************************************************/
/* ListBox (base class) to display lists of components or footprints */
/*********************************************************************/
class ITEMS_LISTBOX_BASE : public wxListView
{
public:
    ITEMS_LISTBOX_BASE( WinEDA_CvpcbFrame* aParent, wxWindowID aId,
                        const wxPoint& aLocation, const wxSize& aSize );

    ~ITEMS_LISTBOX_BASE();

    int                        GetSelection();
    void                       OnSize( wxSizeEvent& event );

    virtual WinEDA_CvpcbFrame* GetParent();
};

/******************************************/
/* ListBox showing the list of footprints */
/******************************************/

class FOOTPRINTS_LISTBOX : public ITEMS_LISTBOX_BASE
{
private:
    wxArrayString  m_FullFootprintList;
    wxArrayString  m_FilteredFootprintList;
public:
    wxArrayString* m_ActiveFootprintList;
    bool           m_UseFootprintFullList;

public:
    FOOTPRINTS_LISTBOX( WinEDA_CvpcbFrame* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size,
                        int nbitems, wxString choice[] );
    ~FOOTPRINTS_LISTBOX();

    int      GetCount();
    void     SetSelection( unsigned index, bool State = TRUE );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     SetFootprintFullList( FOOTPRINT_LIST& list );
    void     SetFootprintFilteredList( COMPONENT*      Component,
                                       FOOTPRINT_LIST& list );
    void     SetActiveFootprintList( bool FullList, bool Redraw = FALSE );

    wxString GetSelectedFootprint();
    wxString OnGetItemText( long item, long column ) const;

    // Events functions:
    void     OnLeftClick( wxListEvent& event );
    void     OnLeftDClick( wxListEvent& event );
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};

/****************************************************/
/* ListBox showing the list of schematic components */
/****************************************************/

class COMPONENTS_LISTBOX : public ITEMS_LISTBOX_BASE
{
public:
    wxArrayString      m_ComponentList;
    WinEDA_CvpcbFrame* m_Parent;

public:

    COMPONENTS_LISTBOX( WinEDA_CvpcbFrame* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size,
                        int nbitems, wxString choice[] );

    ~COMPONENTS_LISTBOX();

    void     Clear();
    int      GetCount();
    wxString OnGetItemText( long item, long column ) const;
    void     SetSelection( unsigned index, bool State = TRUE );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );

    // Events functions:
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};


#endif  //#ifndef CVSTRUCT_H
