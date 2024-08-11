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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_POSITION_RELATIVE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_POSITION_RELATIVE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_referenceInfo;
		wxButton* m_user_origin_button;
		wxButton* m_grid_origin_button;
		wxButton* m_select_anchor_button;
		wxButton* m_select_point_button;
		wxStaticLine* m_staticline2;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xEntry;
		wxStaticText* m_xUnit;
		wxButton* m_clearX;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yEntry;
		wxStaticText* m_yUnit;
		wxButton* m_clearY;
		wxCheckBox* m_polarCoords;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUseUserOriginClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUseGridOriginClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectItemClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectPointClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTextFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPolarChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_POSITION_RELATIVE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Position Relative To Reference Item"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_POSITION_RELATIVE_BASE();

};

