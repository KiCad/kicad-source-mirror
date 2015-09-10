/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>

#include <fctsys.h>
#include <class_drawpanel.h>
#include <sch_marker.h>
#include <wx/html/htmlwin.h>

/**
 * Class ERC_HTML_LISTFRAME
 * is used to display a DRC_ITEM_LIST.
 */
class ERC_HTML_LISTFRAME : public wxHtmlWindow
{
private:
    std::vector<SCH_MARKER*> m_MarkerListReferences;    // The pointers to markers shown in list

public:
    ERC_HTML_LISTFRAME( wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = 0 ) :
        wxHtmlWindow( parent, id, pos, size, style | wxHW_NO_SELECTION )
    {
    }

    ~ERC_HTML_LISTFRAME()
    {
    }


    /**
     * Function AppendToList
     * @param aItem The SCH_MARKER* to add to the current list which will be
     *  later displayed in the wxHtmlWindow
     */
    void AppendToList( SCH_MARKER* aMarker )
    {
        m_MarkerListReferences.push_back( aMarker );
    }

    /**
     * Function DisplayList();
     * Build the Html marker list and show it
     */
    void DisplayList()
    {
        wxString htmlpage;

        // for each marker, build a link like:
        // <A HREF="marker_index">text to click</A>
        // The "text to click" is the error name (first line of the full error text).
        wxString marker_text;

        for( unsigned ii = 0; ii < m_MarkerListReferences.size(); ii++ )
        {
            marker_text.Printf( wxT( "<A HREF=\"%d\">" ), ii );
            marker_text << m_MarkerListReferences[ii]->GetReporter().ShowHtml();
            marker_text.Replace( wxT( "<ul>" ), wxT( "</A><ul>" ), false );
            htmlpage += marker_text;
        }

        SetPage( htmlpage );
    }

    /**
     * Function GetItem
     * returns a requested DRC_ITEM* or NULL.
     */
    const SCH_MARKER* GetItem( unsigned aIndex )
    {
        if( m_MarkerListReferences.size() > aIndex )
        {
            return m_MarkerListReferences[ aIndex ];
        }

        return NULL;
    }


    /**
     * Function ClearList
     * deletes all items shown in the list.
     * Does not erase markers in schematic
     */
    void ClearList()
    {
        m_MarkerListReferences.clear();
        SetPage( wxEmptyString );
    }
};

#endif

// DIALOG_ERC_LISTBOX_H
