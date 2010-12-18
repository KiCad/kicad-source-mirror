///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_aux_helper_class_h_
#define __dialog_design_rules_aux_helper_class_h_

#include <wx/listctrl.h>

/* helper class to display lists of nets and associated netclasses
 * used in dialog design rules.
 * It s needed because the 2 wxListCtlr used to display lists of nets
 * use the wxLC_VIRTUAL option.
 *  The virtual wxString OnGetItemText(long item, long column) const method
 * must be overlaid.
 */
class NETS_LIST_CTRL: public wxListCtrl
{
private:
    wxArrayString           m_Netnames;             ///< an array to store the list of nets (column 0)
    wxArrayString           m_Classnames;            ///< an array to store the list of netclasse (column 1)
public:   
    NETS_LIST_CTRL(wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxLC_ICON):
            wxListCtrl( parent, id, pos, size, style )
    {
    };
    
    NETS_LIST_CTRL()
    {
    };
    void setRowItems(unsigned aRow, const wxString & aNetname, const wxString & aNetclassName ); 
    void ClearList()
    {
        SetItemCount(0);
        m_Netnames.Clear();
        m_Classnames.Clear();
    }

    virtual wxString OnGetItemText(long item, long column) const;
};


#endif //__dialog_design_rules_aux_helper_class_h_
