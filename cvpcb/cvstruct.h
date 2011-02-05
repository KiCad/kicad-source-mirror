/*********************************************************/
/*                      cvstruct.h                       */
/*********************************************************/

#ifndef CVSTRUCT_H
#define CVSTRUCT_H

#include "wx/listctrl.h"
#include "cvpcb.h"

/*  Forward declarations of all top-level window classes. */
class CVPCB_MAINFRAME;


/*********************************************************************/
/* ListBox (base class) to display lists of components or footprints */
/*********************************************************************/
class ITEMS_LISTBOX_BASE : public wxListView
{
public:
    ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                        const wxPoint& aLocation, const wxSize& aSize );

    ~ITEMS_LISTBOX_BASE();

    int                        GetSelection();
    void                       OnSize( wxSizeEvent& event );

    virtual CVPCB_MAINFRAME* GetParent();
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
    FOOTPRINTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
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
    CVPCB_MAINFRAME* m_Parent;

public:

    COMPONENTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
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
