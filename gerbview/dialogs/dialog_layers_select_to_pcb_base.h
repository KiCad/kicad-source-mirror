///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LAYERS_SELECT_TO_PCB_BASE_H__
#define __DIALOG_LAYERS_SELECT_TO_PCB_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/statline.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

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
		void _wxFB_OnOkClick( wxCommandEvent& event ){ OnOkClick( event ); }
		
	
	protected:
		enum
		{
			ID_LAYERS_MAP_DIALOG_BASE = 1000,
			ID_M_STATICLINESEP,
			ID_M_STATICTEXTCOPPERLAYERCOUNT,
			ID_M_COMBOCOPPERLAYERSCOUNT,
			ID_STORE_CHOICE,
			ID_GET_PREVIOUS_CHOICE,
			ID_RESET_CHOICE
		};
		
		wxStaticBoxSizer* sbSizerLayersTable;
		wxFlexGridSizer* m_flexLeftColumnBoxSizer;
		wxStaticLine* m_staticlineSep;
		wxStaticText* m_staticTextCopperlayerCount;
		wxComboBox* m_comboCopperLayersCount;
		wxButton* m_buttonStore;
		wxButton* m_buttonRetrieve;
		wxButton* m_buttonReset;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnBrdLayersCountSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStoreSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGetSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResetClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		LAYERS_MAP_DIALOG_BASE( wxWindow* parent, wxWindowID id = ID_LAYERS_MAP_DIALOG_BASE, const wxString& title = _("Layer selection:"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,286 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~LAYERS_MAP_DIALOG_BASE();
	
};

#endif //__DIALOG_LAYERS_SELECT_TO_PCB_BASE_H__
