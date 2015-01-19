///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __WIZARD_ADD_FPLIB_BASE_H__
#define __WIZARD_ADD_FPLIB_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/wizard.h>
#include <wx/dynarray.h>
WX_DEFINE_ARRAY_PTR( wxWizardPageSimple*, WizardPages );

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class WIZARD_FPLIB_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class WIZARD_FPLIB_TABLE_BASE : public wxWizard 
{
	private:
	
	protected:
		wxRadioBox* m_rbFpLibFormat;
		wxStaticLine* m_staticline2;
		wxStaticBitmap* m_bitmapGithubURL;
		wxStaticText* m_staticText10;
		wxTextCtrl* m_textCtrlGithubURL;
		wxRadioBox* m_rbPathManagement;
		wxStaticText* m_staticText1;
		wxGrid* m_gridEnvironmentVariablesList;
		wxButton* m_buttonAddEV;
		wxButton* m_buttonRemoveEV;
		wxStaticText* m_textPluginTitle;
		wxStaticText* m_textPluginType;
		wxStaticText* m_textOptionTitle;
		wxStaticText* m_textOption;
		wxStaticText* m_stPathTitle;
		wxStaticText* m_textPath;
		wxStaticText* m_staticText2;
		wxGrid* m_gridFpListLibs;
		wxButton* m_buttonGithubLibList;
		wxButton* m_buttonAddLib;
		wxButton* m_buttonRemoveLib;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnFinish( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanged( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanging( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPluginSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPathManagementSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectEnvVarCell( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAddEVariable( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveEVariable( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGithubLibsList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddFpLibs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveFpLibs( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		WIZARD_FPLIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Library Wizard"), const wxBitmap& bitmap = wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ), const wxPoint& pos = wxDefaultPosition, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		WizardPages m_pages;
		~WIZARD_FPLIB_TABLE_BASE();
	
};

#endif //__WIZARD_ADD_FPLIB_BASE_H__
