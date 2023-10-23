///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_IMAGE_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_IMAGE_EDITOR_BASE : public wxPanel
{
	private:

	protected:
		wxPanel* m_panelDraw;
		wxStaticText* m_staticTextScale;
		wxTextCtrl* m_textCtrlScale;
		wxStaticText* m_staticTextPPI;
		wxStaticText* m_stPPI_Value;
		wxButton* m_buttonGrey;

		// Virtual event handlers, override them in your derived class
		virtual void OnRedrawPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnGreyScaleConvert( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_IMAGE_EDITOR_BASE();

};

