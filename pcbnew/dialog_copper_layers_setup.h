///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_LAYERS_SETUP
///////////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_h_
#define __dialog_design_rules_h_

#include "dialog_copper_layers_setup_base.h"


class DIALOG_COPPER_LAYERS_SETUP : public DIALOG_COPPER_LAYERS_SETUP_BASE
{
	private:
        WinEDA_PcbFrame * m_Parent;
        int m_ActivesLayersCount;
        BOARD * m_Pcb;
        LAYER_T m_LayersType[4];
        wxString m_LayersTypeName[4];

	private:
		void OnCancelButtonClick( wxCommandEvent& event );
		void OnOkButtonClick( wxCommandEvent& event );
		void OnLayerCountClick( wxCommandEvent& event );
		void OnLayerGridLeftClick( wxGridEvent& event ){ event.Skip(); }
		void OnLayerGridRighttClick( wxGridEvent& event ){ event.Skip(); }
        void Init();
        void SetRoutableLayerStatus( );
        bool TestDataValidity();

	public:
		DIALOG_COPPER_LAYERS_SETUP( WinEDA_PcbFrame* parent );
		~DIALOG_COPPER_LAYERS_SETUP( ) { };

};

#endif //__dialog_design_rules_h_
