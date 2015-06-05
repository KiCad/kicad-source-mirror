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


/*  Forward declarations of all top-level window classes. */
class CVPCB_MAINFRAME;
class COMPONENT;
class FOOTPRINT_LIST;

#define LISTBOX_STYLE     ( wxSUNKEN_BORDER | wxLC_NO_HEADER | wxLC_REPORT | wxLC_VIRTUAL | \
                            wxVSCROLL | wxHSCROLL )

/*********************************************************************/
/* ListBox (base class) to display lists of components or footprints */
/*********************************************************************/
class ITEMS_LISTBOX_BASE : public wxListView
{
public:
    ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                        const wxPoint& aLocation, const wxSize& aSize,
                        long aStyle = 0 );

    ~ITEMS_LISTBOX_BASE();

    int                      GetSelection();

    virtual CVPCB_MAINFRAME* GetParent() const;

    /* Function UpdateWidth
     *
     * Update the width of the column based on its contents.
     *
     * @param aLine is the line to calculate the width from. If positive, the
     * width will only be increased if needed. If negative, we start from
     * scratch and all lines are considered, i.e., the column may be shrunk.
     */
    void UpdateWidth( int aLine = -1 );

private:
    void UpdateLineWidth( unsigned aLine );

    int columnWidth;
};


/******************************************/
/* ListBox showing the list of footprints */
/******************************************/
class FOOTPRINTS_LISTBOX : public ITEMS_LISTBOX_BASE
{
private:
    wxArrayString  m_footprintList;

public:
    enum FP_FILTER_T
    {
        UNFILTERED   = 0,
        BY_COMPONENT = 0x0001,
        BY_PIN_COUNT = 0x0002,
        BY_LIBRARY   = 0x0004,
    };

    FOOTPRINTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size );
    ~FOOTPRINTS_LISTBOX();

    int      GetCount();
    void     SetSelection( int index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );

    /**
     * Function SetFootprints
     * populates the wxListCtrl with the footprints from \a aList that meet the filter
     * criteria defined by \a aFilterType.
     *
     * @param aList is a #FOOTPRINT_LIST item containing the footprints.
     * @param aLibName is wxString containing the name of the selected library.  Can be
     *                 wxEmptyString.
     * @param aComponent is the #COMPONENT used by the filtering criteria.  Can be NULL.
     * @param aFilterType defines the criteria to filter \a aList.
     */
    void     SetFootprints( FOOTPRINT_LIST& aList, const wxString& aLibName,
                            COMPONENT* aComponent, int aFilterType );

    wxString GetSelectedFootprint();

    /**
     * Function OnGetItemText
     * this overloaded function MUST be provided for the wxLC_VIRTUAL mode
     * because real data is not handled by ITEMS_LISTBOX_BASE
     */
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
    wxArrayString  m_libraryList;

public:
    LIBRARY_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                     const wxPoint& loc, const wxSize& size );
    ~LIBRARY_LISTBOX();

    int      GetCount();
    void     SetSelection( int index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     SetLibraryList( const wxArrayString& aList );

    wxString GetSelectedLibrary();
    wxString OnGetItemText( long item, long column ) const;

    // Events functions:
    void     OnLeftClick( wxListEvent& event );

    void     OnSelectLibrary( wxListEvent& event );

    /**
     * Function OnChar
     * called on a key pressed
     * Call default handler for some special keys,
     * and for "ascii" keys, select the first footprint
     * that the name starts by the letter.
     * This is the defaut behaviour of a listbox, but because we use
     * virtual lists, the listbox does not know anything to what is displayed,
     * we must handle this behaviour here.
     * Furthermore the footprint name is not at the beginning of
     * displayed lines (the first word is the line number)
     */
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

public:

    COMPONENTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                        const wxPoint& loc, const wxSize& size );

    ~COMPONENTS_LISTBOX();

    void     Clear();
    int      GetCount();

    /**
     * Function OnGetItemText
     * this overloaded function MUST be provided for the wxLC_VIRTUAL mode
     * because real data is not handled by ITEMS_LISTBOX_BASE
     */
    wxString OnGetItemText( long item, long column ) const;

    /*
     * Enable or disable an item
     */
    void     SetSelection( int index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );

    // Events functions:

    /**
     * Function OnChar
     * called on a key pressed
     * Call default handler for some special keys,
     * and for "ascii" keys, select the first component
     * that the name starts by the letter.
     * This is the default behavior of a listbox, but because we use
     * virtual lists, the listbox does not know anything to what is displayed,
     * we must handle this behavior here.
     * Furthermore the reference of components is not at the beginning of
     * displayed lines (the first word is the line number)
     */
    void     OnChar( wxKeyEvent& event );

    void     OnSelectComponent( wxListEvent& event );

    DECLARE_EVENT_TABLE()
};


#endif  //#ifndef CVSTRUCT_H
