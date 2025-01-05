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

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GRID_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GRID_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_gridsLabel;
		wxListBox* m_currentGridCtrl;
		STD_BITMAP_BUTTON* m_addGridButton;
		STD_BITMAP_BUTTON* m_editGridButton;
		STD_BITMAP_BUTTON* m_moveUpButton;
		STD_BITMAP_BUTTON* m_moveDownButton;
		STD_BITMAP_BUTTON* m_removeGridButton;
		wxStaticText* m_staticText21;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticTextGrid1;
		wxChoice* m_grid1Ctrl;
		wxStaticText* m_grid1HotKey;
		wxStaticText* m_staticTextGrid2;
		wxChoice* m_grid2Ctrl;
		wxStaticText* m_grid2HotKey;
		wxStaticText* m_overridesLabel;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkGridOverrideConnected;
		wxChoice* m_gridOverrideConnectedChoice;
		wxCheckBox* m_checkGridOverrideWires;
		wxChoice* m_gridOverrideWiresChoice;
		wxCheckBox* m_checkGridOverrideVias;
		wxChoice* m_gridOverrideViasChoice;
		wxCheckBox* m_checkGridOverrideText;
		wxChoice* m_gridOverrideTextChoice;
		wxCheckBox* m_checkGridOverrideGraphics;
		wxChoice* m_gridOverrideGraphicsChoice;

		// Virtual event handlers, override them in your derived class
		virtual void OnEditGrid( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddGrid( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateEditGrid( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnMoveGridUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateMoveUp( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnMoveGridDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateMoveDown( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnRemoveGrid( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateRemove( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		PANEL_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GRID_SETTINGS_BASE();

};

