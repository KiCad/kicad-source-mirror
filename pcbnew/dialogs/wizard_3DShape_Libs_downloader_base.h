///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE_H__
#define __WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/hyperlink.h>
#include <wx/checklst.h>
#include <wx/srchctrl.h>
#include <wx/grid.h>
#include <wx/wizard.h>
#include <wx/dynarray.h>
WX_DEFINE_ARRAY_PTR( wxWizardPageSimple*, WizardPages );

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE
///////////////////////////////////////////////////////////////////////////////
class WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE : public wxWizard 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxStaticText* m_staticText8;
		wxTextCtrl* m_textCtrlGithubURL;
		wxStaticText* m_staticText9;
		wxTextCtrl* m_downloadDir;
		wxButton* m_btnBrowse;
		wxButton* m_buttonDefault3DPath;
		wxStaticText* m_invalidDir;
		wxStaticBitmap* m_bitmapRepo;
		wxHyperlinkCtrl* m_hyperlinkGithubKicad;
		wxStaticText* m_staticText112;
		wxCheckListBox* m_checkList3Dlibnames;
		wxButton* m_btnSelectAll3Dlibs;
		wxButton* m_btnUnselectAll3Dlibs;
		wxSearchCtrl* m_searchCtrl3Dlibs;
		wxStaticText* m_staticTextlocalfolder;
		wxStaticText* m_LocalFolderInfo;
		wxStaticText* m_staticText12;
		wxGrid* m_gridLibReview;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnWizardFinished( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanged( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnPageChanging( wxWizardEvent& event ) { event.Skip(); }
		virtual void OnBrowseButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDefault3DPathButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectAll3Dlibs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUnselectAll3Dlibs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeSearch( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGridLibReviewSize( wxSizeEvent& event ) { event.Skip(); }
		
	
	public:
		
		WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Add 3D Shape Libraries Wizard"), const wxBitmap& bitmap = wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ), const wxPoint& pos = wxDefaultPosition, long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		WizardPages m_pages;
		~WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE();
	
};

#endif //__WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE_H__
