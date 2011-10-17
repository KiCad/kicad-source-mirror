///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_cvpcb_config_fbp__
#define __dialog_cvpcb_config_fbp__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/listbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CVPCB_CONFIG_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CVPCB_CONFIG_FBP : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_ADD_LIB = 1000,
			ID_INSERT_LIB,
			ID_REMOVE_LIB,
			ID_LIB_UP,
			ID_LIB_DOWN,
			ID_ADD_EQU,
			ID_INSERT_EQU,
			ID_REMOVE_EQU,
			ID_EQU_UP,
			ID_EQU_DOWN,
			ID_BROWSE_MOD_DOC,
			ID_LIB_PATH_SEL,
			ID_INSERT_PATH,
			ID_REMOVE_PATH,
		};
		
		wxListBox* m_ListLibr;
		wxButton* m_buttonAddLib;
		wxButton* m_buttonInsLib;
		wxButton* m_buttonRemoveLib;
		wxButton* m_buttonLibUp;
		wxButton* m_buttonLibDown;
		wxListBox* m_ListEquiv;
		wxButton* m_buttonAddEqu;
		wxButton* m_buttonInsEqu;
		wxButton* m_buttonRemoveEqu;
		wxButton* m_buttonEquUp;
		wxButton* m_buttonEquDown;
		wxTextCtrl* m_TextHelpModulesFileName;
		wxButton* m_buttonModDoc;
		wxListBox* m_listUserPaths;
		wxButton* m_buttonAddPath;
		wxButton* m_buttonInsPath;
		wxButton* m_buttonRemovePath;
		wxListBox* m_DefaultLibraryPathslistBox;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnAddOrInsertLibClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveLibClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonUpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonDownClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowseModDocFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddOrInsertPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveUserPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CVPCB_CONFIG_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 570,625 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_CVPCB_CONFIG_FBP();
	
};

#endif //__dialog_cvpcb_config_fbp__
