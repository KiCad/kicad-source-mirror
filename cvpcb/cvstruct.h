/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cvstruct.h
 */

#ifndef CVSTRUCT_H
#define CVSTRUCT_H

#include <wx/listctrl.h>
#include <cvpcb.h>

/*  Forward declarations of all top-level window classes. */
class CVPCB_MAINFRAME;
class COMPONENT;
class FOOTPRINT_LIST;


/*********************************************************************/
/* ListBox (base class) to display lists of components or footprints */
/*********************************************************************/
class ITEMS_LISTBOX_BASE : public wxListView
{
public:
    ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                        const wxPoint& aLocation, const wxSize& aSize,
                        long aStyle = wxLC_SINGLE_SEL);

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
    void     SetSelection( unsigned index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     SetFootprintFullList( FOOTPRINT_LIST& list );
    void     SetFootprintFilteredList( COMPONENT*      aComponent,
                                       FOOTPRINT_LIST& aList );
    void     SetFootprintFilteredByPinCount( COMPONENT*      aComponent,
                                             FOOTPRINT_LIST& aList );
    void     SetFootprintFilteredByLibraryList( FOOTPRINT_LIST& list,
                                                wxString SelectedLibrary  );

    /**
     * Set the footprint list. We can have 2 footprint list:
     *  The full footprint list
     *  The filtered footprint list (if the current selected component has a
     * filter for footprints)
     *  @param FullList true = full footprint list, false = filtered footprint list
     *  @param Redraw = true to redraw the window
     */
    void     SetActiveFootprintList( bool FullList, bool Redraw = false );

    wxString GetSelectedFootprint();
    wxString OnGetItemText( long item, long column ) const;

    // Events functions:
    void     OnLeftClick( wxListEvent& event );
    void     OnLeftDClick( wxListEvent& event );
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};

/******************************************/
/* ListBox showing the list of library */
/******************************************/

class LIBRARY_LISTBOX : public ITEMS_LISTBOX_BASE
{
//private:

public:
    wxArrayString  m_LibraryList;

public:
    LIBRARY_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                     const wxPoint& loc, const wxSize& size,
                     int nbitems, wxString choice[] );
    ~LIBRARY_LISTBOX();

    int      GetCount();
    void     SetSelection( unsigned index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     SetLibraryList( wxArrayString list );

    wxString GetSelectedLibrary();
    wxString OnGetItemText( long item, long column ) const;

    // Events functions:
    void     OnLeftClick( wxListEvent& event );
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
    CVPCB_MAINFRAME*   m_Parent;

public:

    COMPONENTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size,
                        int nbitems, wxString choice[] );

    ~COMPONENTS_LISTBOX();

    void     Clear();
    int      GetCount();
    wxString OnGetItemText( long item, long column ) const;
    void     SetSelection( unsigned index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );

    // Events functions:
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};


#endif  //#ifndef CVSTRUCT_H
