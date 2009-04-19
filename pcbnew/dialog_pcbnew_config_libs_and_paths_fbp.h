///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_pcbnew_config_libs_and_paths_fbp__
#define __dialog_pcbnew_config_libs_and_paths_fbp__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PCBNEW_CONFIG_LIBS_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PCBNEW_CONFIG_LIBS_FBP : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_ADD_LIB = 1000,
			ID_REMOVE_LIB,
			ID_SAVE_CFG,
			wxID_BROWSE_MOD_DOC,
			ID_LIB_PATH_SEL,
			wxID_INSERT_PATH,
			wxID_REMOVE_PATH,
		};
		
		wxStaticText* m_InfoBoardFileExt;
		wxStaticText* m_InfoCmpFileExt;
		wxStaticText* m_InfoLibFileExt;
		wxStaticText* m_InfoNetlistFileExt;
		wxStaticText* m_staticTextlibList;
		wxListBox* m_ListLibr;
		wxButton* m_buttonAddLib;
		wxButton* m_buttonIns;
		wxButton* m_buttonRemoveLib;
		
		wxButton* m_buttonOk;
		wxButton* m_buttonCancel;
		wxButton* m_buttonSave;
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
		virtual void OnAddOrInsertLibClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveLibClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSaveCfgClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnBrowseModDocFile( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddOrInsertPath( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveUserPath( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_PCBNEW_CONFIG_LIBS_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 593,612 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PCBNEW_CONFIG_LIBS_FBP();
	
};

#endif //__dialog_pcbnew_config_libs_and_paths_fbp__
