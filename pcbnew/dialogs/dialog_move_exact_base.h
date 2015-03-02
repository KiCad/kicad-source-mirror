///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_MOVE_EXACT_BASE_H__
#define __DIALOG_MOVE_EXACT_BASE_H__

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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MOVE_EXACT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MOVE_EXACT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_polarCoords;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xEntry;
		wxStaticText* m_xUnit;
		wxBitmapButton* m_clearX;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yEntry;
		wxStaticText* m_yUnit;
		wxBitmapButton* m_clearY;
		wxStaticText* m_rotLabel;
		wxTextCtrl* m_rotEntry;
		wxStaticText* m_rotUnit;
		wxBitmapButton* m_clearRot;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnPolarChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTextFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_MOVE_EXACT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Move item"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 331,200 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_MOVE_EXACT_BASE();
	
};

#endif //__DIALOG_MOVE_EXACT_BASE_H__
