///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/hyperlink.h>
#include <wx/dirctrl.h>
#include <wx/checklst.h>
#include <wx/srchctrl.h>
#include <wx/dataview.h>
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
		wxStaticText* m_staticText1;
		wxRadioButton* m_radioAddLocal;
		wxRadioButton* m_radioAddGithub;
		wxTextCtrl* m_textCtrlGithubURL;
		wxCheckBox* m_downloadGithub;
		wxStaticText* m_downloadDir;
		wxButton* m_btnBrowse;
		wxStaticText* m_invalidDir;
		wxStaticBitmap* m_bitmapRepo;
		wxHyperlinkCtrl* m_hyperlinkGithubKicad;
		wxStaticText* m_staticText7;
		wxGenericDirCtrl* m_filePicker;
		wxStaticText* m_staticText112;
		wxCheckListBox* m_checkListGH;
		wxButton* m_btnSelectAllGH;
		wxButton* m_btnUnselectAllGH;
		wxSearchCtrl* m_searchCtrlGH;
		wxStaticText* m_staticText1121;
		wxDataViewListCtrl* m_listCtrlReview;
		wxDataViewColumn* m_dvLibName;
		wxDataViewColumn* m_dvLibStatus;
		wxDataViewColumn* m_dvLibFormat;
		wxStaticText* m_staticText12;
		wxRadioButton* m_radioGlobal;
		wxRadioButton* m_radioProject;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnWizardFinished( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanged( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanging( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnCheckSaveCopy( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowseButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectAllGH( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUnselectAllGH( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeSearch( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		WIZARD_FPLIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Add Footprint Libraries Wizard"), const wxBitmap& bitmap = wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ), const wxPoint& pos = wxDefaultPosition, long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		WizardPages m_pages;
		~WIZARD_FPLIB_TABLE_BASE();
	
};

#endif //__WIZARD_ADD_FPLIB_BASE_H__
