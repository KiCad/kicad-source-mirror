///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_h_
#define __dialog_design_rules_h_

#include "dialog_design_rules_base.h"


class DIALOG_DESIGN_RULES : public DIALOG_DESIGN_RULES_BASE
{
	private:
        WinEDA_PcbFrame * m_Parent;
        int m_ActivesLayersCount;
        BOARD * m_Pcb;
        int m_Changes;
        LAYER_T m_LayersType[4];
        wxString m_LayersTypeName[4];
        std::vector<NETINFO_ITEM*> m_StockNets;     // full list of nets on board
        std::vector<int> m_NetsLinkToClasses;     // index to affect each net to an existing net class

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
        void Init();
        void InitRulesList();
        void InitializeRulesSelectionBoxes();
        void CopyRulesListToBoard();
        void SetRoutableLayerStatus( );
        void FillListBoxWithNetsNames(wxListBox* aListBox, int aNetclassIndex);

	public:
		DIALOG_DESIGN_RULES( WinEDA_PcbFrame* parent );
		~DIALOG_DESIGN_RULES( ) { };

};

#endif //__dialog_design_rules_h_
