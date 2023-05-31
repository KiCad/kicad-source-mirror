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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <widgets/split_button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PACKAGE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PACKAGE_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_bitmap;
		wxStaticText* m_name;
		wxBoxSizer* m_descSizer;
		wxStaticText* m_desc;
		wxButton* m_button;
		SPLIT_BUTTON* m_splitButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnPaint( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnButtonClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PACKAGE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_NONE|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PACKAGE_BASE();

};

