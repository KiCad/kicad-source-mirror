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
class WX_HTML_REPORT_BOX;

#include "dialog_shim.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/html/htmlwin.h>
#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/dataview.h>
#include <wx/listctrl.h>
#include <wx/hyperlink.h>
#include <wx/simplebook.h>
#include <wx/stattext.h>
#include <widgets/number_badge.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DRC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DRC_BASE : public DIALOG_SHIM
{
	private:
		wxPanel* m_panelUnconnectedItems;

	protected:
		wxCheckBox* m_cbRefillZones;
		wxCheckBox* m_cbTestFootprints;
		STD_BITMAP_BUTTON* m_bMenu;
		wxSimplebook* m_runningResultsBook;
		wxPanel* running;
		wxNotebook* m_runningNotebook;
		wxPanel* m_panelMessages;
		WX_HTML_REPORT_BOX* m_messages;
		wxGauge* m_gauge;
		wxPanel* results;
		wxNotebook* m_Notebook;
		wxPanel* m_panelViolations;
		wxDataViewCtrl* m_markerDataView;
		wxDataViewCtrl* m_unconnectedDataView;
		wxPanel* m_panelFootprintWarnings;
		wxDataViewCtrl* m_footprintsDataView;
		wxPanel* m_panelIgnored;
		wxListCtrl* m_ignoredList;
		wxHyperlinkCtrl* m_violationSeveritiesLink;
		wxStaticText* m_showLabel;
		wxCheckBox* m_showAll;
		wxCheckBox* m_showErrors;
		NUMBER_BADGE* m_errorsBadge;
		wxCheckBox* m_showWarnings;
		NUMBER_BADGE* m_warningsBadge;
		wxCheckBox* m_showExclusions;
		NUMBER_BADGE* m_exclusionsBadge;
		wxButton* m_saveReport;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_DeleteCurrentMarkerButton;
		wxButton* m_DeleteAllMarkersButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnActivateDlg( wxActivateEvent& event ) { event.Skip(); }
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnMenu( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorLinkClicked( wxHtmlLinkEvent& event ) { event.Skip(); }
		virtual void OnChangingNotebookPage( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnDRCItemDClick( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnDRCItemRClick( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnDRCItemSelected( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnIgnoredItemRClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnEditViolationSeverities( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnSeverity( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveReport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteOneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunDRCClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_DRC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Design Rules Checker"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DRC_BASE();

};

