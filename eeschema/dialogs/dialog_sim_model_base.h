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

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/listbox.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/advprops.h>
#include <wx/panel.h>
#include <wx/stc/stc.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SIM_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SIM_MODEL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxNotebook* m_notebook;
		wxPanel* m_modelPanel;
		wxRadioButton* m_rbLibraryModel;
		wxStaticText* m_pathLabel;
		wxTextCtrl* m_libraryPathText;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_modelNameLabel;
		wxSearchCtrl* m_modelFilter;
		wxListBox* m_modelListBox;
		wxStaticText* m_pinLabel;
		wxComboBox* m_pinCombobox;
		wxCheckBox* m_differentialCheckbox;
		wxStaticText* m_pinModelLabel;
		wxComboBox* m_pinModelCombobox;
		wxStaticText* m_waveformLabel;
		wxChoice* m_waveformChoice;
		wxRadioButton* m_rbBuiltinModel;
		wxStaticText* m_deviceLabel;
		wxChoice* m_deviceChoice;
		wxStaticText* m_deviceSubtypeLabel;
		wxChoice* m_deviceSubtypeChoice;
		wxNotebook* m_modelNotebook;
		wxPanel* m_parametersPanel;
		wxPropertyGridManager* m_paramGridMgr;
		wxPropertyGridPage* m_paramGrid;
		wxPanel* m_codePanel;
		wxStyledTextCtrl* m_codePreview;
		wxCheckBox* m_saveInValueCheckbox;
		wxPanel* m_pinAssignmentsPanel;
		WX_GRID* m_pinAssignmentsGrid;
		wxStaticText* m_subcktLabel;
		wxStyledTextCtrl* m_subckt;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onRadioButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLibraryPathLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onLibraryPathTextKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onLibraryPathText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLibraryPathTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseButtonUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onModelNameLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onFilterCharHook( wxKeyEvent& event ) { event.Skip(); }
		virtual void onModelFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelNameChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onIbisPinLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPinCombobox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelNameComboboxKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onPinComboboxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelNameComboboxUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onDifferentialCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOverrideCheckboxUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onIbisModelLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPinModelCombobox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPinModelComboboxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWaveformChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeChoiceUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onTypeLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPageChanging( wxNotebookEvent& event ) { event.Skip(); }
		virtual void onSizeParamGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void onPinAssignmentsGridCellChange( wxGridEvent& event ) { event.Skip(); }
		virtual void onPinAssignmentsGridSize( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_SIM_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Model Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SIM_MODEL_BASE();

};

