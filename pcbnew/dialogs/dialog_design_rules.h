///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_h_
#define __dialog_design_rules_h_

#include <../class_board.h>

#include <dialog_design_rules_base.h>


class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


// helper struct to handle a net and its netclass in dialog design rule editor
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

    static const wxString wildCard;     ///< the name of a ficticious netclass which includes all NETs

    PCB_EDIT_FRAME*         m_Parent;
    BOARD*                  m_Pcb;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    static int              s_LastTabSelection;     ///< which tab user had open last

    /**
     * A two column table which gets filled once and never loses any elements, so it is
     * basically constant, except that the NETCUP::clazz member can change for any
     * given row a NET is moved in and out of a class.  clazz reflects the respective
     * NET's current net class.
     */
    NETCUPS                 m_AllNets;

    // List of values to "customize" some tracks and vias
    std::vector <VIA_DIMENSION> m_ViasDimensionsList;
    std::vector <int> m_TracksWidthList;

private:
    void OnNetClassesNameLeftClick( wxGridEvent& event ){ event.Skip(); }
    void OnNetClassesNameRightClick( wxGridEvent& event ){ event.Skip(); }
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOkButtonClick( wxCommandEvent& event );
    void OnAddNetclassClick( wxCommandEvent& event );
    void OnRemoveNetclassClick( wxCommandEvent& event );
    void OnMoveUpSelectedNetClass( wxCommandEvent& event );
    void OnLeftCBSelection( wxCommandEvent& event );
    void OnRightCBSelection( wxCommandEvent& event );
    void OnRightToLeftCopyButton( wxCommandEvent& event );
    void OnLeftToRightCopyButton( wxCommandEvent& event );
    void OnLeftSelectAllButton( wxCommandEvent& event );
    void OnRightSelectAllButton( wxCommandEvent& event );
    bool TestDataValidity( );
    void InitDialogRules();
    void InitGlobalRules();
    void InitRulesList();
    void InitDimensionsLists();
    void InitializeRulesSelectionBoxes();
    void CopyRulesListToBoard();
    void CopyGlobalRulesToBoard();
    void CopyDimensionsListsToBoard( );
    void SetRoutableLayerStatus();
    void FillListBoxWithNetNames( NETS_LIST_CTRL* aListCtrl, const wxString& aNetClass );
    void PrintCurrentSettings( );

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

    void moveSelectedItems( NETS_LIST_CTRL* src, const wxString& newClassName );


public:
    DIALOG_DESIGN_RULES( PCB_EDIT_FRAME* parent );
    ~DIALOG_DESIGN_RULES( ) { };

};

#endif //__dialog_design_rules_h_
