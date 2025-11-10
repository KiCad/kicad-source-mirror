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
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_ZONE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_ZONE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			ID_DIALOG_COPPER_ZONE_BASE = 27300,
		};

		wxBoxSizer* m_MainBoxSizer;
		wxStaticText* m_staticTextLayerSelection;
		wxDataViewListCtrl* m_layers;
		wxBoxSizer* m_sizerRight;
		wxButton* m_openZoneManager;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onZoneManager( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id = ID_DIALOG_COPPER_ZONE_BASE, const wxString& title = _("Copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_COPPER_ZONE_BASE();

};

