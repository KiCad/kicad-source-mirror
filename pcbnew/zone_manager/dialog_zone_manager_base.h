///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include "widgets/std_bitmap_button.h"
#include <wx/checkbox.h>
#include <wx/srchctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ZONE_MANAGER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ZONE_MANAGER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			ID_DIALOG_COPPER_ZONE_BASE = 1000,
			VIEW_ZONE_TABLE,
			BTN_MOVE_UP,
			BTN_MOVE_DOWN,
			CHECK_NAME,
			CHECK_NET
		};

		wxBoxSizer* m_MainBoxSizer;
		wxBoxSizer* m_sizerTop;
		wxDataViewCtrl* m_viewZonesOverview;
		wxBoxSizer* m_sizerZoneOP;
		STD_BITMAP_BUTTON* m_btnMoveUp;
		STD_BITMAP_BUTTON* m_btnMoveDown;
		wxCheckBox* m_checkName;
		wxCheckBox* m_checkNet;
		wxSearchCtrl* m_filterCtrl;
		wxStaticBoxSizer* m_sizerProperties;
		wxCheckBox* m_checkRepour;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnTableChar( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnTableCharHook( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnDataViewCtrlSelectionChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnViewZonesOverviewOnLeftUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlSearch( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlTextChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterCtrlEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRepourCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonApplyClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_ZONE_MANAGER_BASE( wxWindow* parent, wxWindowID id = ID_DIALOG_COPPER_ZONE_BASE, const wxString& title = _("Zone Manager"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_ZONE_MANAGER_BASE();

};

