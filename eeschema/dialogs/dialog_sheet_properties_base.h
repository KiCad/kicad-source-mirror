///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;
class WX_GRID;
class WX_INFOBAR;

#include "widgets/color_swatch.h"
#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SHEET_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SHEET_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxBoxSizer* m_longForm;
		WX_GRID* m_grid;
		STD_BITMAP_BUTTON* m_bpAdd;
		STD_BITMAP_BUTTON* m_bpMoveUp;
		STD_BITMAP_BUTTON* m_bpMoveDown;
		STD_BITMAP_BUTTON* m_bpDelete;
		wxStaticText* m_pageNumberStaticText;
		wxTextCtrl* m_pageNumberTextCtrl;
		wxCheckBox* m_cbExcludeFromSim;
		wxCheckBox* m_cbExcludeFromBom;
		wxCheckBox* m_cbExcludeFromBoard;
		wxCheckBox* m_cbDNP;
		wxStaticText* m_borderLabel;
		wxStaticText* m_borderWidthLabel;
		wxTextCtrl* m_borderWidthCtrl;
		wxStaticText* m_borderWidthUnits;
		wxStaticText* m_borderColorLabel;
		COLOR_SWATCH* m_borderSwatch;
		wxStaticText* m_fillLabel;
		wxStaticText* m_backgroundColorLabel;
		COLOR_SWATCH* m_backgroundSwatch;
		wxBoxSizer* m_sizerBottom;
		wxStaticText* m_hierarchicalPathLabel;
		wxStaticText* m_hierarchicalPath;
		wxStdDialogButtonSizer* m_stdDialogButtonSizer;
		wxButton* m_stdDialogButtonSizerOK;
		wxButton* m_stdDialogButtonSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SHEET_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sheet Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~DIALOG_SHEET_PROPERTIES_BASE();

};

