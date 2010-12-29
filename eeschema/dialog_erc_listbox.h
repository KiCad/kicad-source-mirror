/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_erc.h
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef DIALOG_ERC_LISTBOX_H
#define DIALOG_ERC_LISTBOX_H

#include <wx/htmllbox.h>
#include <vector>

#include "fctsys.h"
#include "class_drawpanel.h"
#include "sch_marker.h"

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
     */
    void AppendToList( SCH_MARKER* aItem )
    {
        m_MarkerList.push_back( aItem);
        SetItemCount( m_MarkerList.size() );
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
        if( m_MarkerList.size() > n && n >= 0 )
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
