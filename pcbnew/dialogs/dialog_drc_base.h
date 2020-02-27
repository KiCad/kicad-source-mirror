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
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NOTEBOOK1 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DRC_CONTROL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DRC_CONTROL_BASE : public DIALOG_SHIM
{
	private:
		wxPanel* m_panelUnconnectedItems;

	protected:
		wxStaticText* m_MinWidthLabel;
		wxStaticText* m_MinWidthUnits;
		wxStaticText* m_ViaMinLabel;
		wxStaticText* m_ViaMinUnits;
		wxStaticText* m_uViaMinLabel;
		wxStaticText* m_uViaMinUnits;
		wxCheckBox* m_cbRefillZones;
		wxCheckBox* m_cbReportAllTrackErrors;
		wxCheckBox* m_cbReportTracksToZonesErrors;
		wxCheckBox* m_cbTestFootprints;
		wxTextCtrl* m_Messages;
		wxNotebook* m_Notebook;
		wxPanel* m_panelViolations;
		wxDataViewCtrl* m_markerDataView;
		wxDataViewCtrl* m_unconnectedDataView;
		wxPanel* m_panelFootprintWarnings;
		wxDataViewCtrl* m_footprintsDataView;
		wxStaticText* m_showLabel;
		wxCheckBox* m_showAll;
		wxCheckBox* m_showErrors;
		wxCheckBox* m_showWarnings;
		wxCheckBox* m_showInfos;
		wxButton* m_saveReport;
		wxStaticLine* m_staticline1;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_DeleteCurrentMarkerButton;
		wxButton* m_DeleteAllMarkersButton;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnActivateDlg( wxActivateEvent& event ) { event.Skip(); }
		virtual void OnChangingNotebookPage( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnDRCItemDClick( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnDRCItemSelected( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnSeverity( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveReport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteOneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunDRCClick( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxTextCtrl* m_MinWidthCtrl;
		wxTextCtrl* m_ViaMinCtrl;
		wxTextCtrl* m_uViaMinCtrl;

		DIALOG_DRC_CONTROL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("DRC Control"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_DRC_CONTROL_BASE();

};

