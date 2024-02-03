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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DIELECTRIC_MATERIAL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DIELECTRIC_MATERIAL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTexMaterial;
		wxStaticText* m_staticTextEpsilonR;
		wxStaticText* m_staticTextLossTg;
		wxTextCtrl* m_tcMaterial;
		wxTextCtrl* m_tcEpsilonR;
		wxTextCtrl* m_tcLossTg;
		wxStaticText* m_staticText;
		wxListCtrl* m_lcMaterials;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void onListKeyDown( wxListEvent& event ) { event.Skip(); }


	public:

		DIALOG_DIELECTRIC_MATERIAL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Dielectric Material Characteristics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DIELECTRIC_MATERIAL_BASE();

};

