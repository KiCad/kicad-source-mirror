///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_aux_helper_class_h_
#define __dialog_design_rules_aux_helper_class_h_

#include <wx/listctrl.h>

/**
 * Class NETS_LIST_CTRL
 * is a helper to display lists of nets and associated netclasses
 * used in dialog design rules.
 * It's needed because the 2 "wxListCtl"s used to display lists of nets
 * uses the wxLC_VIRTUAL option. The method:
 *
 *   virtual wxString OnGetItemText( long item, long column ) const
 *
 * must be overloaded.
 */
class NETS_LIST_CTRL: public wxListCtrl
{
public:
    NETS_LIST_CTRL( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = wxLC_ICON ):
        wxListCtrl( parent, id, pos, size, style )
    {
    };

    void ClearList()
    {
        SetItemCount( 0 );
        m_Netnames.Clear();
        m_Classnames.Clear();
    }

    /**
     * Function OnGetItemText
     * is an overloaded method needed by wxListCtrl with wxLC_VIRTUAL options
     */
    virtual wxString OnGetItemText( long item, long column ) const;

    /**
     * Function SetRowItems
     * sets the net name and the net class name at @a aRow.
     * @param aRow = row index (if aRow > number of stored row, empty rows will be created)
     * @param aNetname = the string to display in row aRow, column 0
     * @param aNetclassName = the string to display in row aRow, column 1
     */
    void SetRowItems( unsigned aRow, const wxString& aNetname, const wxString& aNetclassName );

private:
    wxArrayString   m_Netnames;     ///< column 0: nets
    wxArrayString   m_Classnames;   ///< column 1: netclasses
};


#endif //__dialog_design_rules_aux_helper_class_h_
