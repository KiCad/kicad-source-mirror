///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 30 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_IMAGE_EDITOR_BASE_H__
#define __DIALOG_IMAGE_EDITOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMAGE_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMAGE_EDITOR_BASE : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panelDraw;
		wxButton* m_buttonMirrorX;
		wxButton* m_buttonMirrorY;
		wxButton* m_buttonRotate;
		wxStaticText* m_staticTextScale;
		wxTextCtrl* m_textCtrlScale;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRedrawPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnMirrorX_click( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMirrorY_click( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRotateClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOK_Button( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Image Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 340,256 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_IMAGE_EDITOR_BASE();
	
};

#endif //__DIALOG_IMAGE_EDITOR_BASE_H__
