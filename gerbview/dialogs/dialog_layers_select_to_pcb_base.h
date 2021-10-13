///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
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

#define ID_LAYERS_MAP_DIALOG_BASE 1000
#define ID_M_STATICLINESEP 1001
#define ID_M_STATICTEXTCOPPERLAYERCOUNT 1002
#define ID_M_COMBOCOPPERLAYERSCOUNT 1003
#define ID_STORE_CHOICE 1004
#define ID_GET_PREVIOUS_CHOICE 1005
#define ID_RESET_CHOICE 1006

///////////////////////////////////////////////////////////////////////////////
/// Class LAYERS_MAP_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class LAYERS_MAP_DIALOG_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_OnBrdLayersCountSelection( wxCommandEvent& event ){ OnBrdLayersCountSelection( event ); }
		void _wxFB_OnStoreSetup( wxCommandEvent& event ){ OnStoreSetup( event ); }
		void _wxFB_OnGetSetup( wxCommandEvent& event ){ OnGetSetup( event ); }
		void _wxFB_OnResetClick( wxCommandEvent& event ){ OnResetClick( event ); }


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
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnBrdLayersCountSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStoreSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGetSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResetClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		LAYERS_MAP_DIALOG_BASE( wxWindow* parent, wxWindowID id = ID_LAYERS_MAP_DIALOG_BASE, const wxString& title = _("Layer Selection"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~LAYERS_MAP_DIALOG_BASE();

};

