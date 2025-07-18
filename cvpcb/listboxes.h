/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LISTBOXES_H
#define LISTBOXES_H

#include <wx/listctrl.h>
#include <footprint_filter.h>
#include <memory>

/*  Forward declarations of all top-level window classes. */
class CVPCB_MAINFRAME;
class COMPONENT;
class FOOTPRINT_LIST;

#define LISTBOX_STYLE     ( wxBORDER_NONE | wxLC_NO_HEADER | wxLC_REPORT | wxLC_VIRTUAL | \
                            wxVSCROLL | wxHSCROLL )

/**
 * Base class to display symbol and footprint lists.
 */
class ITEMS_LISTBOX_BASE : public wxListView
{
public:
    ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                        const wxPoint& aLocation = wxDefaultPosition,
                        const wxSize& aSize = wxDefaultSize, long aStyle = 0 );

    virtual ~ITEMS_LISTBOX_BASE() = default;

    /**
     * @return the index of the selected item in lists allowing only one item selected
     * and the index of the first selected item in lists allowing many selection
     */
    int GetSelection();

    /**
     * Remove all selection in lists which can have more than one item selected.
     */
    void DeselectAll();

    virtual CVPCB_MAINFRAME* GetParent() const;

    /**
     * Update the width of the column based on its contents.
     *
     * @param aLine is the line to calculate the width from. If positive, the
     * width will only be increased if needed. If negative, we start from
     * scratch and all lines are considered, i.e., the column may be shrunk.
     */
    void UpdateWidth( int aLine = -1 );

    void Shutdown() { m_isClosing = true; }

private:
    /**
     * Calculate the width of the given line, and increase the column width
     * if needed. This is effectively the wxListCtrl code for autosizing.
     * NB. it relies on the caller checking the given line number is valid.
     */
    void UpdateLineWidth( unsigned aLine, wxClientDC& dc );

protected:
    bool m_isClosing;

private:
    int  m_columnWidth;
};


class FOOTPRINTS_LISTBOX : public ITEMS_LISTBOX_BASE
{
public:
    /**
     * Filter setting constants. The filter type is a bitwise OR of these flags,
     * and only footprints matching all selected filter types are shown.
     */
    enum FP_FILTER_T: int
    {
        UNFILTERED_FP_LIST                = 0,
        FILTERING_BY_COMPONENT_FP_FILTERS = 0x0001,
        FILTERING_BY_PIN_COUNT            = 0x0002,
        FILTERING_BY_LIBRARY              = 0x0004
    };

    FOOTPRINTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id );
    virtual ~FOOTPRINTS_LISTBOX() = default;

    int      GetCount();
    void     SetSelection( int aIndex, bool aState = true );
    void     SetSelectedFootprint( const LIB_ID& aFPID );
    void     SetString( unsigned linecount, const wxString& text );

    /**
     * Populate the wxListCtrl with the footprints from \a aList that meet the filter
     * criteria defined by \a aFilterType.
     *
     * @param aList is a #FOOTPRINT_LIST item containing the footprints.
     * @param aLibName is wxString containing the name of the selected library.  Can be
     *                 wxEmptyString.
     * @param aComponent is the #COMPONENT used by the filtering criteria.  Can be NULL.
     * @param aFootPrintFilterPattern is the filter used to filter list by names.
     * @param aFilterType defines the criteria to filter \a aList.
     */
    void     SetFootprints( FOOTPRINT_LIST& aList, const wxString& aLibName, COMPONENT* aComponent,
                            const wxString& aFootPrintFilterPattern, int aFilterType );

    wxString GetSelectedFootprint();

    /**
     * This overloaded function MUST be provided for the wxLC_VIRTUAL mode
     * because real data is not handled by ITEMS_LISTBOX_BASE.
     */
    wxString OnGetItemText( long item, long column ) const override;

    // Events functions:
    void     OnLeftClick( wxListEvent& event );
    void     OnLeftDClick( wxListEvent& event );
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE();

private:
    wxArrayString  m_footprintList;
};


class LIBRARY_LISTBOX : public ITEMS_LISTBOX_BASE
{
public:
    LIBRARY_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id );
    virtual ~LIBRARY_LISTBOX() = default;

    int      GetCount();
    void     SetSelection( int index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     Finish();
    void     ClearList();

    wxString GetSelectedLibrary();
    wxString OnGetItemText( long item, long column ) const override;

    void     OnSelectLibrary( wxListEvent& event );

    /**
     * Called on a key press.
     *
     * Call default handler for some special keys, and for "ASCII" keys, select the first
     * footprint that the name starts by the letter.
     *
     * This is the default behavior of a listbox, but because we use virtual lists, the
     * listbox does not know anything to what is displayed, we must handle this behavior
     * here.  Furthermore the footprint name is not at the beginning of displayed lines
     * (the first word is the line number).
     */
    void     OnChar( wxKeyEvent& event );

    DECLARE_EVENT_TABLE();

private:
    wxArrayString  m_libraryList;
};


class SYMBOLS_LISTBOX : public ITEMS_LISTBOX_BASE
{
public:
    SYMBOLS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id );
    virtual ~SYMBOLS_LISTBOX() = default;

    void     Clear();
    int      GetCount();

    /**
     * This overloaded function MUST be provided for the wxLC_VIRTUAL mode
     * because real data is not handled by #ITEMS_LISTBOX_BASE.
     */
    wxString OnGetItemText( long item, long column ) const override;
    wxListItemAttr* OnGetItemAttr( long item) const override;

    /*
     * Enable or disable an item
     */
    void     SetSelection( int index, bool State = true );
    void     SetString( unsigned linecount, const wxString& text );
    void     AppendLine( const wxString& text );
    void     AppendWarning( int index );
    void     RemoveWarning( int index );

    // Events functions:

    /**
     * Called on a key press.
     *
     * Call default handler for some special keys, and for "ASCII" keys, select the first
     * component that the name starts by the letter.
     *
     * This is the default behavior of a listbox, but because we use virtual lists, the
     * listbox does not know anything to what is displayed, we must handle this behavior
     * here.  Furthermore the reference of components is not at the beginning of displayed
     * lines (the first word is the line number).
     */
    void     OnChar( wxKeyEvent& event );

    void     OnSelectComponent( wxListEvent& event );

    DECLARE_EVENT_TABLE();

public:
    wxArrayString                   m_SymbolList;

private:
    std::vector<long>               m_symbolWarning;
    std::unique_ptr<wxListItemAttr> m_warningAttr;
};


#endif  //#ifndef LISTBOXES_H
