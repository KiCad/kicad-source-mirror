///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_DRC_BASE_H__
#define __DIALOG_DRC_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class DRCLISTBOX;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CHECKBOX_RPT_FILE 1000
#define ID_BUTTON_BROWSE_RPT_FILE 1001
#define ID_STARTDRC 1002
#define ID_LIST_UNCONNECTED 1003
#define ID_DELETE_ALL 1004
#define ID_NOTEBOOK1 1005
#define ID_CLEARANCE_LIST 1006
#define ID_UNCONNECTED_LIST 1007

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DRC_CONTROL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DRC_CONTROL_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_ClearanceTitle;
		wxStaticText* m_TrackMinWidthTitle;
		wxStaticText* m_ViaMinTitle;
		wxStaticText* m_MicroViaMinTitle;
		wxButton* m_BrowseButton;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_Messages;
		wxButton* m_buttonRunDRC;
		wxButton* m_buttonListUnconnected;
		wxButton* m_DeleteAllButton;
		wxButton* m_DeleteCurrentMarkerButton;
		wxStaticText* m_staticTextErrMsg;
		wxNotebook* m_Notebook;
		wxPanel* m_panelClearanceListBox;
		wxPanel* m_panelUnconnectedBox;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnReportCheckBoxClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonBrowseRptFileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStartdrcClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnListUnconnectedClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteOneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangingMarkerList( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickClearance( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnMarkerSelectionEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRightUpClearance( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickUnconnected( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnUnconnectedSelectionEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRightUpUnconnected( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxTextCtrl* m_SetClearance;
		wxTextCtrl* m_SetTrackMinWidthCtrl;
		wxTextCtrl* m_SetViaMinSizeCtrl;
		wxTextCtrl* m_SetMicroViakMinSizeCtrl;
		wxCheckBox* m_CreateRptCtrl;
		wxTextCtrl* m_RptFilenameCtrl;
		DRCLISTBOX* m_ClearanceListBox;
		DRCLISTBOX* m_UnconnectedListBox;
		
		DIALOG_DRC_CONTROL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("DRC Control"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_DRC_CONTROL_BASE();
	
};

#endif //__DIALOG_DRC_BASE_H__
