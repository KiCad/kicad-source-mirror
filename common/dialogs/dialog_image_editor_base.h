///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
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
		wxButton* m_buttonGrey;
		wxStaticText* m_staticTextScale;
		wxTextCtrl* m_textCtrlScale;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRedrawPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnGreyScaleConvert( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Image Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_IMAGE_EDITOR_BASE();
	
};

#endif //__DIALOG_IMAGE_EDITOR_BASE_H__
