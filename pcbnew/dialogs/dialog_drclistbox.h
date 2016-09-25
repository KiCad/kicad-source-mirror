/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


#ifndef _DIALOG_DRCLISTBOX_H_
#define _DIALOG_DRCLISTBOX_H_

#include <wx/htmllbox.h>

#include <fctsys.h>
#include <pcbnew.h>
#include <class_drawpanel.h>
#include <wxstruct.h>
#include <drc_stuff.h>
#include <class_marker_pcb.h>
#include <class_board.h>

#include <dialog_drc_base.h>


// outside @end control identifiers since wxFormBuilder knows not DRCLISTBOX
#define ID_DRCLISTCTRL 14000
#define ID_POPUP_UNCONNECTED_A  14001
#define ID_POPUP_UNCONNECTED_B  14002
#define ID_POPUP_MARKERS_A      14003
#define ID_POPUP_MARKERS_B      14004


/**
 * Class DRC_LIST_MARKERS
 * is an implementation of the interface named DRC_ITEM_LIST which uses
 * a BOARD instance to fulfill the interface.  No ownership is taken of the
 * BOARD.
 */
class DRC_LIST_MARKERS : public DRC_ITEM_LIST
{
    BOARD*          m_board;

public:

    DRC_LIST_MARKERS( BOARD* aBoard ) :
        m_board(aBoard)
    {
    }

    /* no destructor since we do not own anything to delete, not even the BOARD.
    ~DRC_LIST_MARKERS() {}
    */


    //-----<Interface DRC_ITEM_LIST>---------------------------------------

    void            DeleteAllItems() override
    {
        m_board->DeleteMARKERs();
    }


    const DRC_ITEM* GetItem( int aIndex ) override
    {
        const MARKER_PCB* marker = m_board->GetMARKER( aIndex );
        if( marker )
            return &marker->GetReporter();
        return NULL;
    }

    void DeleteItem( int aIndex ) override
    {
        MARKER_PCB* marker = m_board->GetMARKER( aIndex );
        if( marker )
            m_board->Delete( marker );
    }


    /**
     * Function GetCount
     * returns the number of items in the list.
     */
    int  GetCount() override
    {
        return m_board->GetMARKERCount();
    }

    //-----</Interface DRC_ITEM_LIST>--------------------------------------

};


/**
 * Class DRC_LIST_UNCONNECTED
 * is an implementation of the interface named DRC_ITEM_LIST which uses
 * a vector of pointers to DRC_ITEMs to fulfill the interface.  No ownership is taken of the
 * vector, which will reside in class DRC
 */
class DRC_LIST_UNCONNECTED : public DRC_ITEM_LIST
{
    DRC_LIST*         m_vector;

public:

    DRC_LIST_UNCONNECTED( DRC_LIST* aList ) :
        m_vector(aList)
    {
    }

    /* no destructor since we do not own anything to delete, not even the BOARD.
    ~DRC_LIST_UNCONNECTED() {}
    */


    //-----<Interface DRC_ITEM_LIST>---------------------------------------

    void            DeleteAllItems() override
    {
        if( m_vector )
        {
            for( unsigned i=0; i<m_vector->size();  ++i )
                delete (*m_vector)[i];

            m_vector->clear();
        }
    }


    const DRC_ITEM* GetItem( int aIndex ) override
    {
        if( m_vector &&  (unsigned)aIndex < m_vector->size() )
        {
            const DRC_ITEM* item = (*m_vector)[aIndex];
            return item;
        }
        return NULL;
    }

    void DeleteItem( int aIndex ) override
    {
        if( m_vector &&  (unsigned)aIndex < m_vector->size() )
        {
            delete (*m_vector)[aIndex];
            m_vector->erase( m_vector->begin()+aIndex );
        }
    }


    /**
     * Function GetCount
     * returns the number of items in the list.
     */
    int  GetCount() override
    {
        if( m_vector )
        {
            return m_vector->size();
        }
        return 0;
    }

    //-----</Interface DRC_ITEM_LIST>--------------------------------------

};



/**
 * Class DRCLISTBOX
 * is used to display a DRC_ITEM_LIST.
 */
class DRCLISTBOX : public wxHtmlListBox
{
private:
    DRC_ITEM_LIST* m_list;     ///< wxHtmlListBox does not own the list, I do

public:
    DRCLISTBOX( wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = 0, const wxString choices[] = NULL, int unused = 0)
        : wxHtmlListBox( parent, id, pos, size, style )
    {
        m_list = 0;
    }


    ~DRCLISTBOX()
    {
        delete m_list;  // I own it, I destroy it.
    }


    /**
     * Function SetList
     * sets the DRC_ITEM_LIST for this listbox.  Ownership of the DRC_ITEM_LIST is
     * transfered to this DRCLISTBOX.
     * @param aList The DRC_ITEM_LIST* containing the DRC_ITEMs which will be
     *  displayed in the wxHtmlListBox
     */
    void SetList( DRC_ITEM_LIST* aList )
    {
        delete m_list;

        m_list = aList;
        SetItemCount( aList->GetCount() );
        Refresh();
    }


    /**
     * Function GetItem
     * returns a requested DRC_ITEM* or NULL.
     */
    const DRC_ITEM* GetItem( int aIndex )
    {
        if( m_list )
        {
            return m_list->GetItem( aIndex );
        }
        return NULL;
    }


    /**
     * Function OnGetItem
     * returns the html text associated with the DRC_ITEM given by index 'n'.
     * @param n An index into the list.
     * @return wxString - the simple html text to show in the listbox.
     */
    wxString OnGetItem( size_t n ) const override
    {
        if( m_list )
        {
            const DRC_ITEM*   item = m_list->GetItem( (int) n );
            if( item )
                return item->ShowHtml();
        }
        return wxString();
    }


    /**
     * Function OnGetItem
     * returns the html text associated with the given index 'n'.
     * @param n An index into the list.
     * @return wxString - the simple html text to show in the listbox.
     */
    wxString OnGetItemMarkup( size_t n ) const override
    {
        return OnGetItem( n );
    }


    /**
     * Function DeleteElement
     * will delete one of the items in the list.
     * @param aIndex The index into the list to delete.
     */
    void DeleteItem( int aIndex )
    {
        if( m_list )
        {
            int selection = GetSelection();

            m_list->DeleteItem( aIndex );
            int count = m_list->GetCount();
            SetItemCount( count );

            // if old selection >= new count
            if( selection >= count )
                SetSelection( count-1 );    // -1 is "no selection"
            Refresh();
        }
    }


    /**
     * Function DeleteAllItems
     * deletes all items in the list.
     */
    void DeleteAllItems()
    {
        if( m_list )
        {
            m_list->DeleteAllItems();
            SetItemCount(0);
            SetSelection( -1 );    // -1 is no selection
            Refresh();
        }
    }
};

#endif  // _DIALOG_DRCLISTBOX_H_

