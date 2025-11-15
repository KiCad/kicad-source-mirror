///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-62-g497c85bd-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_STATISTICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_STATISTICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* topNotebook;
		wxPanel* m_generalPanel;
		wxStaticText* m_componentsLabel;
		wxGrid* m_gridComponents;
		wxStaticText* m_padsLabel;
		wxGrid* m_gridPads;
		wxStaticText* m_boardLabel;
		wxGrid* m_gridBoard;
		wxStaticText* m_viasLabel;
		wxGrid* m_gridVias;
		wxCheckBox* m_checkBoxSubtractHoles;
		wxCheckBox* m_checkBoxSubtractHolesFromCopper;
		wxCheckBox* m_checkBoxExcludeComponentsNoPins;
		wxPanel* m_drillsPanel;
		WX_GRID* m_gridDrills;
		wxButton* m_buttonSaveReport;
		wxStdDialogButtonSizer* m_sdbControlSizer;
		wxButton* m_sdbControlSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void windowSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void checkboxClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void drillGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void saveReportClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Board Statistics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_BOARD_STATISTICS_BASE();

};

