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
#include "dialog_shim.h"
#include <wx/gdicmn.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GRIDITEM_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GRIDITEM_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebookGridDefs;
		wxPanel* m_rectangleByCorners;
		wxGridBagSizer* m_gbsRectangleByCorners;
		wxPanel* m_rectangleByCornerSize;
		wxGridBagSizer* m_gbsRectangleByCornerSize;
		wxPanel* m_rectangleByCenterSize;
		wxGridBagSizer* m_gbsRectangleByCenterSize;
		wxPanel* m_polarCenterRadius;
		wxGridBagSizer* m_gbsPolarCenterRadius;
		wxCheckBox* m_locked;
		wxBoxSizer* m_upperSizer;
		wxStaticText* m_gridTypeLabel;
		wxChoice* m_gridTypeCtrl;
		wxStaticText* m_orientationLabel;
		wxTextCtrl* m_orientationCtrl;
		wxStaticText* m_orientationUnits;
		wxStaticText* m_spacingXLabel;
		wxTextCtrl* m_spacingXCtrl;
		wxStaticText* m_spacingXUnits;
		wxStaticText* m_spacingYLabel;
		wxTextCtrl* m_spacingYCtrl;
		wxStaticText* m_spacingYUnits;
		wxStaticText* m_tickIntervalLabel;
		wxTextCtrl* m_tickIntervalCtrl;
		wxStaticText* m_tickIntervalUnits;
		wxStaticText* m_priorityLabel;
		wxTextCtrl* m_priorityCtrl;
		wxStaticText* m_affectsLabel;
		wxCheckBox* m_affectsCursor;
		wxCheckBox* m_affectsRouting;
		wxCheckBox* m_affectsPlacement;
		wxStdDialogButtonSizer* m_StandardButtonsSizer;
		wxButton* m_StandardButtonsSizerOK;
		wxButton* m_StandardButtonsSizerCancel;

	public:

		DIALOG_GRIDITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("%s Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GRIDITEM_PROPERTIES_BASE();

};

