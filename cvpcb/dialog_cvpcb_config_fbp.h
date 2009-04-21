///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_cvpcb_config_fbp__
#define __dialog_cvpcb_config_fbp__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/button.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/listbox.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
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
			ID_SAVE_CFG = 1000,
			ID_ADD_LIB,
			ID_INSERT_LIB,
			ID_REMOVE_LIB,
			ID_ADD_EQU,
			ID_INSERT_EQU,
			ID_REMOVE_EQU,
			ID_BROWSE_MOD_DOC,
			ID_LIB_PATH_SEL,
			ID_INSERT_PATH,
			ID_REMOVE_PATH,
		};
		
		wxButton* m_buttonOk;
		wxButton* m_buttonCancel;
		wxButton* m_buttonSave;
		
		wxStaticText* m_InfoCmpFileExt;
		wxStaticText* m_InfoLibFileExt;
		wxStaticText* m_InfoNetlistFileExt;
		wxStaticText* m_InfoEquivFileExt;
		wxStaticText* m_InfoRetroannotFileExt;
		wxRadioBox* m_radioBoxCloseOpt;
		wxStaticText* m_staticTextlibList;
		wxButton* m_buttonAddLib;
		wxButton* m_buttonInsLib;
		wxButton* m_buttonRemoveLib;
		wxListBox* m_ListLibr;
		wxStaticText* m_staticTextEquList;
		wxButton* m_buttonAddEqu;
		wxButton* m_buttonInsEqu;
		wxButton* m_buttonRemoveEqu;
		wxListBox* m_ListEquiv;
		wxStaticLine* m_staticline1;
		wxTextCtrl* m_TextHelpModulesFileName;
		wxButton* m_buttonModDoc;
		wxListBox* m_listUserPaths;
		wxButton* m_buttonAddPath;
		wxButton* m_buttonInsPath;
		wxButton* m_buttonRemovePath;
		wxStaticText* m_staticTextcurrenpaths;
		wxListBox* m_DefaultLibraryPathslistBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSaveCfgClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddOrInsertLibClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveLibClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnBrowseModDocFile( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddOrInsertPath( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveUserPath( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_CVPCB_CONFIG_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 641,612 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_CVPCB_CONFIG_FBP();
	
};

#endif //__dialog_cvpcb_config_fbp__
