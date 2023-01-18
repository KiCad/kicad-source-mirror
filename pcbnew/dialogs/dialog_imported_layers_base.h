///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORTED_LAYERS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORTED_LAYERS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxStaticText* m_lblImportedLayers;
		wxStaticText* m_lblKicadLayers;
		wxListCtrl* m_unmatched_layers_list;
		wxListCtrl* m_kicad_layers_list;
		wxButton* m_button_add;
		wxButton* m_button_remove;
		wxButton* m_button_removeall;
		wxListCtrl* m_matched_layers_list;
		wxButton* m_button_automatch;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnUnMatchedDoubleClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnAddClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveAllClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchedDoubleClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnAutoMatchLayersClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORTED_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Edit Mapping of Imported Layers"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORTED_LAYERS_BASE();

};

