///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RULE_AREA_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RULE_AREA_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* bMainSizer;

	protected:
		wxStaticText* m_staticTextLayerSelection;
		wxDataViewListCtrl* m_layers;
		wxStaticText* m_nameLabel;
		wxTextCtrl* m_tcName;
		wxCheckBox* m_cbLocked;
		wxNotebook* m_areaPropertiesNb;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineDisplayCtrl;
		wxStaticText* m_stBorderHatchPitchText;
		wxTextCtrl* m_outlineHatchPitchCtrl;
		wxStaticText* m_outlineHatchUnits;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onLayerListRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSizeLayersList( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_RULE_AREA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Rule Area Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 708,471 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxBORDER_SUNKEN );

		~DIALOG_RULE_AREA_PROPERTIES_BASE();

};

