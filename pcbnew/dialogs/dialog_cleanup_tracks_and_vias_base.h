///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <widgets/net_selector.h>
#include <wx/choice.h>
#include <wx/bmpcbox.h>
#include <wx/stattext.h>
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/simplebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_cbRefillZones;
		wxCheckBox* m_cleanShortCircuitOpt;
		wxCheckBox* m_cleanViasOpt;
		wxCheckBox* m_deleteDanglingViasOpt;
		wxCheckBox* m_mergeSegmOpt;
		wxCheckBox* m_deleteUnconnectedOpt;
		wxCheckBox* m_deleteTracksInPadsOpt;
		wxCheckBox* m_netFilterOpt;
		NET_SELECTOR* m_netFilter;
		wxCheckBox* m_netclassFilterOpt;
		wxChoice* m_netclassFilter;
		wxCheckBox* m_layerFilterOpt;
		PCB_LAYER_BOX_SELECTOR* m_layerFilter;
		wxCheckBox* m_selectedItemsFilter;
		wxSimplebook* m_outputBook;
		wxPanel* m_changesPanel;
		wxStaticText* staticChangesLabel;
		wxDataViewCtrl* m_changesDataView;
		wxPanel* m_runningPanel;
		wxStaticText* staticProgressLabel;
		wxTextCtrl* m_tcReport;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetclassFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLayerFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectItem( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickItem( wxMouseEvent& event ) { event.Skip(); }


	public:

		DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleanup Tracks and Vias"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE();

};

