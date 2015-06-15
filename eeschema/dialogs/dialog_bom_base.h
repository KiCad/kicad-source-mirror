///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_BOM_BASE_H__
#define __DIALOG_BOM_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOM_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnPluginSelected( wxCommandEvent& event ){ OnPluginSelected( event ); }
		void _wxFB_OnNameEdited( wxCommandEvent& event ){ OnNameEdited( event ); }
		void _wxFB_OnRunPlugin( wxCommandEvent& event ){ OnRunPlugin( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnHelp( wxCommandEvent& event ){ OnHelp( event ); }
		void _wxFB_OnAddPlugin( wxCommandEvent& event ){ OnAddPlugin( event ); }
		void _wxFB_OnRemovePlugin( wxCommandEvent& event ){ OnRemovePlugin( event ); }
		void _wxFB_OnEditPlugin( wxCommandEvent& event ){ OnEditPlugin( event ); }
		void _wxFB_OnCommandLineEdited( wxCommandEvent& event ){ OnCommandLineEdited( event ); }
		
	
	protected:
		enum
		{
			IN_NAMELINE = 1000,
			ID_CREATE_BOM,
			ID_HELP,
			ID_ADD_PLUGIN,
			ID_REMOVEL_PLUGIN,
			ID_CMDLINE
		};
		
		wxStaticText* m_staticTextPluginTitle;
		wxListBox* m_lbPlugins;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxButton* m_buttonNetlist;
		wxButton* m_buttonCancel;
		wxButton* m_buttonHelp;
		wxStaticLine* m_staticline2;
		wxButton* m_buttonAddPlugin;
		wxButton* m_buttonDelPlugin;
		wxButton* m_buttonEdit;
		wxStaticText* m_staticTextCmd;
		wxTextCtrl* m_textCtrlCommand;
		wxStaticText* m_staticTextInfo;
		wxTextCtrl* m_Messages;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPluginSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNameEdited( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunPlugin( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddPlugin( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemovePlugin( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditPlugin( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCommandLineEdited( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_BOM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Bill of Material"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 409,393 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_BOM_BASE();
	
};

#endif //__DIALOG_BOM_BASE_H__
