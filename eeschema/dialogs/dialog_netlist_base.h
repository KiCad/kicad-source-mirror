///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_NETLIST_BASE_H__
#define __DIALOG_NETLIST_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class NETLIST_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class NETLIST_DIALOG_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnNetlistTypeSelection( wxNotebookEvent& event ){ OnNetlistTypeSelection( event ); }
		void _wxFB_GenNetlist( wxCommandEvent& event ){ GenNetlist( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnAddPlugin( wxCommandEvent& event ){ OnAddPlugin( event ); }
		void _wxFB_OnDelPlugin( wxCommandEvent& event ){ OnDelPlugin( event ); }
		
	
	protected:
		enum
		{
			ID_CHANGE_NOTEBOOK_PAGE = 1000,
			ID_CREATE_NETLIST,
			ID_ADD_PLUGIN,
			ID_DEL_PLUGIN
		};
		
		wxNotebook* m_NoteBook;
		wxButton* m_buttonNetlist;
		wxButton* m_buttonCancel;
		wxButton* m_buttonAddPlugin;
		wxButton* m_buttonDelPlugin;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_cbUseDefaultNetlistName;
		wxStaticText* m_staticTextDefaultFN;
		wxTextCtrl* m_textCtrlDefaultFileName;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnNetlistTypeSelection( wxNotebookEvent& event ) { event.Skip(); }
		virtual void GenNetlist( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddPlugin( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelPlugin( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		NETLIST_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 404,334 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~NETLIST_DIALOG_BASE();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class NETLIST_DIALOG_ADD_PLUGIN_BASE
///////////////////////////////////////////////////////////////////////////////
class NETLIST_DIALOG_ADD_PLUGIN_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnOKClick( wxCommandEvent& event ){ OnOKClick( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnBrowsePlugins( wxCommandEvent& event ){ OnBrowsePlugins( event ); }
		
	
	protected:
		enum
		{
			wxID_BROWSE_PLUGINS = 1000
		};
		
		wxStaticText* m_staticTextCmd;
		wxTextCtrl* m_textCtrlCommand;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxButton* m_buttonOK;
		wxButton* m_buttonCancel;
		wxButton* m_buttonPlugin;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowsePlugins( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		NETLIST_DIALOG_ADD_PLUGIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plugins:"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 312,144 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~NETLIST_DIALOG_ADD_PLUGIN_BASE();
	
};

#endif //__DIALOG_NETLIST_BASE_H__
