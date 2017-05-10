///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_POSITION_RELATIVE_BASE_H__
#define __DIALOG_POSITION_RELATIVE_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_POSITION_RELATIVE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_POSITION_RELATIVE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_polarCoords;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xEntry;
		wxStaticText* m_xUnit;
		wxButton* m_clearX;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yEntry;
		wxStaticText* m_yUnit;
		wxButton* m_clearY;
		wxStaticText* m_rotLabel;
		wxTextCtrl* m_rotEntry;
		wxStaticText* m_rotUnit;
		wxButton* m_clearRot;
		wxStaticText* m_anchor_xLabel;
		wxTextCtrl* m_anchor_x;
		wxStaticText* m_anchor_yLabel;
		wxTextCtrl* m_anchor_y;
		wxStaticLine* m_staticline1;
		wxButton* m_select_anchor_button;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnPolarChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTextFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectItemClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_POSITION_RELATIVE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Position Relative"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_POSITION_RELATIVE_BASE();
	
};

#endif //__DIALOG_POSITION_RELATIVE_BASE_H__
