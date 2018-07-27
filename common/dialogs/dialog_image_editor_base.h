///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_IMAGE_EDITOR_BASE_H__
#define __DIALOG_IMAGE_EDITOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMAGE_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMAGE_EDITOR_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxPanel* m_panelDraw;
		wxButton* m_buttonMirrorX;
		wxButton* m_buttonMirrorY;
		wxButton* m_buttonRotate;
		wxButton* m_buttonGrey;
		wxButton* m_buttonHalfSize;
		wxButton* m_buttonUndoLast;
		wxStaticText* m_staticTextScale;
		wxTextCtrl* m_textCtrlScale;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRedrawPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnMirrorX_click( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMirrorY_click( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRotateClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGreyScaleConvert( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHalfSize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUndoLastChange( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Image Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_IMAGE_EDITOR_BASE();
	
};

#endif //__DIALOG_IMAGE_EDITOR_BASE_H__
