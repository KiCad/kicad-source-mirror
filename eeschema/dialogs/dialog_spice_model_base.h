///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class WX_GRID;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/stc/stc.h>
#include <wx/notebook.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SPICE_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SPICE_MODEL_BASE : public wxDialog
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_modelPanel;
		wxStaticText* m_staticText122;
		wxTextCtrl* m_modelName;
		wxButton* m_browseButton;
		wxStaticText* m_staticText124;
		wxStaticText* m_staticText125;
		wxCheckBox* m_checkBox2;
		wxNotebook* m_notebook4;
		wxPanel* m_parametersPanel;
		wxStaticText* m_staticText127;
		wxChoice* m_deviceTypeChoice;
		wxStaticText* m_staticText8;
		wxChoice* m_typeChoice;
		WX_GRID* m_paramGrid;
		wxPanel* m_codePanel;
		wxStyledTextCtrl* m_codePreview;
		wxPanel* m_pinAssignmentsPanel;
		WX_GRID* m_pinAssignmentGrid;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_excludeSymbol;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onDeviceTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGridCellChange( wxGridEvent& event ) { event.Skip(); }


	public:

		DIALOG_SPICE_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Spice Model Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxSTAY_ON_TOP );

		~DIALOG_SPICE_MODEL_BASE();

};

