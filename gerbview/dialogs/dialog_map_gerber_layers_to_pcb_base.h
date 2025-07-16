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
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextLayerSel;
		wxBoxSizer* m_bSizerLayerList;
		wxFlexGridSizer* m_flexLeftColumnBoxSizer;
		wxStaticLine* m_staticlineSep;
		wxFlexGridSizer* m_flexRightColumnBoxSizer;
		wxStaticText* m_staticTextCopperlayerCount;
		wxComboBox* m_comboCopperLayersCount;
		wxButton* m_buttonStore;
		wxButton* m_buttonRetrieve;
		wxButton* m_buttonReset;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnBrdLayersCountSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStoreSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGetSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResetClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Layer Selection"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE();

};

