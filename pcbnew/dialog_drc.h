/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_drc.h
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_DRC_H_
#define _DIALOG_DRC_H_

#include <wx/htmllbox.h>

// forward declarations
class DRCLISTBOX;
//end forward declarations

#include "fctsys.h"
#include "pcbnew.h"
#include "class_drawpanel.h"
#include "wxstruct.h"
#include "drc_stuff.h"

#include "dialog_drc_base.h"

// outside @end control identifiers since wxFormBuilder knows not DRCLISTBOX
#define ID_DRCLISTCTRL 14000
#define ID_POPUP_UNCONNECTED_A  14001
#define ID_POPUP_UNCONNECTED_B  14002
#define ID_POPUP_MARKERS_A      14003
#define ID_POPUP_MARKERS_B      14004




/*!
 * DrcDialog class declaration
 */

class DIALOG_DRC_CONTROL: public DIALOG_DRC_CONTROL_BASE
{
public:
    /// Constructors
    DIALOG_DRC_CONTROL( DRC* aTester, WinEDA_PcbFrame* parent );
    ~DIALOG_DRC_CONTROL(){};

private:

    static wxSize		    s_LastSize;		        ///< last position and size
    static wxPoint		    s_LastPos;


    /**
     * Function writeReport
     * outputs the MARKER items and unconnecte DRC_ITEMs with commentary to an
     * open text file.
     * @param fpOut The text file to write the report to.
     */
    void writeReport( FILE* fpOut );

    void InitValues( );

    void SetDrcParmeters( );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX
    void OnReportCheckBoxClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BROWSE_RPT_FILE
    void OnButtonBrowseRptFileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_STARTDRC
    void OnStartdrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LIST_UNCONNECTED
    void OnListUnconnectedClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ALL
    void OnDeleteAllClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ONE
    void OnDeleteOneClick( wxCommandEvent& event );

    /// wxEVT_LEFT_DCLICK event handler for ID_CLEARANCE_LIST
    void OnLeftDClickClearance( wxMouseEvent& event );

    /// wxEVT_RIGHT_UP event handler for ID_CLEARANCE_LIST
    void OnRightUpClearance( wxMouseEvent& event );

    /// wxEVT_LEFT_DCLICK event handler for ID_UNCONNECTED_LIST
    void OnLeftDClickUnconnected( wxMouseEvent& event );

    /// wxEVT_RIGHT_UP event handler for ID_UNCONNECTED_LIST
    void OnRightUpUnconnected( wxMouseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    void OnMarkerSelectionEvent( wxCommandEvent& event );
    void OnUnconnectedSelectionEvent( wxCommandEvent& event );

    void DelDRCMarkers();
    void RedrawDrawPanel();

    void OnPopupMenu( wxCommandEvent& event );


    DRC*                m_tester;
    WinEDA_PcbFrame*    m_Parent;
    int                 m_UnconnectedCount;
};


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

    void            DeleteAllItems()
    {
        m_board->DeleteMARKERs();
    }


    const DRC_ITEM* GetItem( int aIndex )
    {
        const MARKER_PCB* marker = m_board->GetMARKER( aIndex );
        if( marker )
            return &marker->GetReporter();
        return NULL;
    }

    void DeleteItem( int aIndex )
    {
        MARKER_PCB* marker = m_board->GetMARKER( aIndex );
        if( marker )
            m_board->Delete( marker );
    }


    /**
     * Function GetCount
     * returns the number of items in the list.
     */
    int  GetCount()
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

    void            DeleteAllItems()
    {
        if( m_vector )
        {
            for( unsigned i=0; i<m_vector->size();  ++i )
                delete (*m_vector)[i];

            m_vector->clear();
        }
    }


    const DRC_ITEM* GetItem( int aIndex )
    {
        if( m_vector &&  (unsigned)aIndex < m_vector->size() )
        {
            const DRC_ITEM* item = (*m_vector)[aIndex];
            return item;
        }
        return NULL;
    }

    void DeleteItem( int aIndex )
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
    int  GetCount()
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
    wxString OnGetItem( size_t n ) const
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
    wxString OnGetItemMarkup( size_t n ) const
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

#endif  // _DIALOG_DRC_H_

