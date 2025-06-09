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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PLUGIN_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PLUGIN_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* apiLabel;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText3;
		wxCheckBox* m_cbEnableApi;
		wxStaticText* pythonLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText2;
		wxFilePickerCtrl* m_pickerPythonInterpreter;
		wxButton* m_btnDetectAutomatically;
		wxStaticText* m_stApiStatus;

		// Virtual event handlers, override them in your derived class
		virtual void OnEnableApiChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPythonInterpreterChanged( wxFileDirPickerEvent& event ) { event.Skip(); }
		virtual void OnBtnDetectAutomaticallyClicked( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxStaticText* m_stPythonStatus;

		PANEL_PLUGIN_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PLUGIN_SETTINGS_BASE();

};

