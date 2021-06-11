///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class COLOR_SWATCH;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_3D_COLORS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_3D_COLORS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* backgroundTopLabel;
		COLOR_SWATCH* m_backgroundTop;
		wxStaticText* backgroundBotLabel;
		COLOR_SWATCH* m_backgroundBottom;
		wxStaticText* silkscreenTopLabel;
		COLOR_SWATCH* m_silkscreenTop;
		wxStaticText* silkscreenBottomLabel;
		COLOR_SWATCH* m_silkscreenBottom;
		wxStaticText* solderMaskTopLabel;
		COLOR_SWATCH* m_solderMaskTop;
		wxStaticText* solderMaskBottomLabel;
		COLOR_SWATCH* m_solderMaskBottom;
		wxStaticText* solderPasteLabel;
		COLOR_SWATCH* m_solderPaste;
		wxStaticText* surfaceFinishLabel;
		COLOR_SWATCH* m_surfaceFinish;
		wxStaticText* boardBodyLabel;
		COLOR_SWATCH* m_boardBody;
		wxButton* m_loadStackup;

		// Virtual event handlers, overide them in your derived class
		virtual void OnLoadColorsFromBoardStackup( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_3D_COLORS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_3D_COLORS_BASE();

};

