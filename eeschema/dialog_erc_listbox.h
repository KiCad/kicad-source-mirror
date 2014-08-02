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

#ifndef DIALOG_ERC_LISTBOX_H
#define DIALOG_ERC_LISTBOX_H

#include <wx/htmllbox.h>
#include <vector>

#include <fctsys.h>
#include <class_drawpanel.h>
#include <sch_marker.h>

/**
 * Class ERC_HTML_LISTBOX
 * is used to display a DRC_ITEM_LIST.
 */
class ERC_HTML_LISTBOX : public wxHtmlListBox
{
private:
    std::vector<SCH_MARKER*> m_MarkerList;  ///< wxHtmlListBox does not own the list, I do

public:
    ERC_HTML_LISTBOX( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0, const wxString choices[] = NULL,
                      int unused = 0 ) :
        wxHtmlListBox( parent, id, pos, size, style )
    {
    }


    ~ERC_HTML_LISTBOX()
    {
    }


    /**
     * Function AppendToList
     * @param aItem The SCH_MARKER* to add to the current list which will be
     *  displayed in the wxHtmlListBox
     * @param aRefresh = true to refresh the display
     */
    void AppendToList( SCH_MARKER* aItem, bool aRefresh = true )
    {
        m_MarkerList.push_back( aItem);
        SetItemCount( m_MarkerList.size() );
        if( aRefresh )
            Refresh();
    }


    /**
     * Function GetItem
     * returns a requested DRC_ITEM* or NULL.
     */
    const SCH_MARKER* GetItem( unsigned aIndex )
    {
        if( m_MarkerList.size() > aIndex )
        {
            return m_MarkerList[ aIndex ];
        }
        return NULL;
    }


    /**
     * Function OnGetItem
     * returns the html text associated with the DRC_ITEM given by index 'n'.
     * @param n An index into the list.
     * @return wxString - the simple html text to show in the listbox.
     */
    wxString OnGetItem( size_t n ) const
    {
        if( m_MarkerList.size() > n )
        {
            const SCH_MARKER* item = m_MarkerList[ n ];
            if( item )
                return item->GetReporter().ShowHtml();
        }
        return wxString();
    }


    /**
     * Function OnGetItemMarkup
     * returns the html text associated with the given index 'n'.
     * @param n An index into the list.
     * @return wxString - the simple html text to show in the listbox.
     */
    wxString OnGetItemMarkup( size_t n ) const
    {
        return OnGetItem( n );
    }


    /**
     * Function ClearList
     * deletes all items in the list.
     * Does not erase markers in schematic
     */
    void ClearList()
    {
        m_MarkerList.clear();
        SetItemCount( 0 );
        SetSelection( -1 );        // -1 is no selection
        Refresh();
    }
};

#endif

// DIALOG_ERC_LISTBOX_H
