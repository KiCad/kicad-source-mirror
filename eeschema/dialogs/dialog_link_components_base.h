///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/combobox.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LINK_COMPONENTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LINK_COMPONENTS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxTextCtrl* m_filterSource;
		wxTextCtrl* m_filterDest;
		wxListBox* m_listSources;
		wxListBox* m_listTargets;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_filterNets;
		wxListBox* m_srcNets;
		wxGrid* m_gridKiLinks;
		wxStaticText* m_staticText18;
		wxComboBox* m_cbLinkClass;
		wxStaticText* m_txtSource;
		wxStaticText* m_txtDest;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSrcFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTargetFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSrcSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTargetSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGridCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnGridLabelClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LINK_COMPONENTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Link Components"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LINK_COMPONENTS_BASE();

};

