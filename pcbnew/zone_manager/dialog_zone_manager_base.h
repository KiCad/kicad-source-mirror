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

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/dataview.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ZONE_MANAGER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ZONE_MANAGER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainBoxSizer;
		wxBoxSizer* m_sizerTop;
		wxPanel* m_listPanel;
		wxSearchCtrl* m_filterCtrl;
		wxCheckBox* m_checkName;
		wxCheckBox* m_checkNet;
		wxStaticText* m_layerFilterLabel;
		wxChoice* m_layerFilter;
		wxDataViewCtrl* m_viewZonesOverview;
		wxBoxSizer* m_sizerZoneOP;
		STD_BITMAP_BUTTON* m_btnMoveTop;
		STD_BITMAP_BUTTON* m_btnMoveUp;
		STD_BITMAP_BUTTON* m_btnMoveDown;
		STD_BITMAP_BUTTON* m_btnMoveBottom;
		STD_BITMAP_BUTTON* m_btnAutoAssign;
		wxPanel* m_zonePanel;
		wxBoxSizer* m_rightColumn;
		wxBoxSizer* m_sizerProperties;
		wxBoxSizer* m_sizerPreview;
		wxStaticLine* m_staticline1;
		wxBoxSizer* m_sizerBottom;
		wxCheckBox* m_checkRepour;
		wxButton* m_updateDisplayedZones;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onDialogResize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlSearch( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlTextChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLayerFilterChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTableChar( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnTableCharHook( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnDataViewCtrlSelectionChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnViewZonesOverviewOnLeftUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnMoveTopClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDownClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveBottomClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAutoAssignClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateDisplayedZonesClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOk( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_ZONE_MANAGER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Zone Manager"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_ZONE_MANAGER_BASE();

};

