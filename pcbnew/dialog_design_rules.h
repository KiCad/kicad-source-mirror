///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_h_
#define __dialog_design_rules_h_

#include "dialog_design_rules_base.h"

struct NETCUP
{
    NETCUP( const wxString& aNet, const wxString& aClass )
    {
        net = aNet;
        clazz = aClass;
    }

    wxString    net;            ///< a net name
    wxString    clazz;          ///< a class name
};


typedef std::vector<NETCUP>     NETCUPS;
typedef std::vector<NETCUP*>    PNETCUPS;

class DIALOG_DESIGN_RULES : public DIALOG_DESIGN_RULES_BASE
{

private:

    static const wxString wildCard;

    WinEDA_PcbFrame*        m_Parent;
    BOARD*                  m_Pcb;

    std::vector<wxString>   m_NetClasses;
    NETCUPS                 m_AllNets;

private:
    void OnLayerCountClick( wxCommandEvent& event );
    void OnLayerGridLeftClick( wxGridEvent& event ){ event.Skip(); }
    void OnLayerGridRighttClick( wxGridEvent& event ){ event.Skip(); }
    void OnNetClassesGridLeftClick( wxGridEvent& event ){ event.Skip(); }
    void OnNetClassesGridRightClick( wxGridEvent& event ){ event.Skip(); }
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOkButtonClick( wxCommandEvent& event );
    void OnAddNetclassClick( wxCommandEvent& event );
    void OnRemoveNetclassClick( wxCommandEvent& event );
    void OnLeftCBSelection( wxCommandEvent& event );
    void OnRightCBSelection( wxCommandEvent& event );
    void OnRightToLeftCopyButton( wxCommandEvent& event );
    void OnLeftToRightCopyButton( wxCommandEvent& event );
    void OnLeftSelectAllButton( wxCommandEvent& event );
    void OnRightSelectAllButton( wxCommandEvent& event );
    bool TestDataValidity( );
    void Init();
    void InitRulesList();
    void InitializeRulesSelectionBoxes();
    void CopyRulesListToBoard();
    void SetRoutableLayerStatus();
    void FillListBoxWithNetNames( wxListCtrl* aListCtrl, const wxString& aNetClass );

    /**
     * Function swapNetClass
     * replaces one net class name with another in the master list, m_AllNets.
     */
    void swapNetClass( const wxString& oldClass, const wxString& newClass )
    {
        for( NETCUPS::iterator i = m_AllNets.begin(); i!=m_AllNets.end();  ++i )
        {
            if( i->clazz == oldClass )
                i->clazz = newClass;
        }
    }

    void makePointers( PNETCUPS* aList, const wxString& aNetClassName );

    void setNetClass( const wxString& aNetName, const wxString& aClassName );

    static void setRowItem(  wxListCtrl* aListCtrl, int aRow, NETCUP* aNetAndClass );

    void moveSelectedItems( wxListCtrl* src, const wxString& newClassName );


public:
    DIALOG_DESIGN_RULES( WinEDA_PcbFrame* parent );
    ~DIALOG_DESIGN_RULES( ) { };

};

#endif //__dialog_design_rules_h_
