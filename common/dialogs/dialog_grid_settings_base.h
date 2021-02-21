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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/simplebook.h>
#include <wx/statline.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GRID_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GRID_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxSimplebook* m_book;
		wxStaticText* m_staticTextGridPosX;
		wxTextCtrl* m_GridOriginXCtrl;
		wxStaticText* m_TextPosXUnits;
		wxStaticText* m_staticTextGridPosY;
		wxTextCtrl* m_GridOriginYCtrl;
		wxStaticText* m_TextPosYUnits;
		wxChoice* m_currentGridCtrl;
		wxStaticText* m_staticTextSizeX;
		wxTextCtrl* m_OptGridSizeX;
		wxStaticText* m_TextSizeXUnits;
		wxStaticText* m_staticTextSizeY;
		wxTextCtrl* m_OptGridSizeY;
		wxStaticText* m_TextSizeYUnits;
		wxStaticText* m_staticTextGrid1;
		wxChoice* m_grid1Ctrl;
		wxStaticText* m_grid1HotKey;
		wxStaticText* m_staticTextGrid2;
		wxChoice* m_grid2Ctrl;
		wxStaticText* m_grid2HotKey;
		wxStaticLine* m_staticline1;
		wxButton* m_buttonResetOrigin;
		wxButton* m_buttonResetSizes;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnResetGridOriginClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Grid Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GRID_SETTINGS_BASE();

};

