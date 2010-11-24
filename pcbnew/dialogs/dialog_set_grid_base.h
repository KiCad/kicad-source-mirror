///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_set_grid_base__
#define __dialog_set_grid_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SET_GRID_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SET_GRID_BASE : public wxDialog 
{
	private:
	
	protected:
		wxRadioBox* m_UnitGrid;
		
		wxStaticText* m_staticTextSizeX;
		wxTextCtrl* m_OptGridSizeX;
		wxStaticText* m_staticTextSizeY;
		wxTextCtrl* m_OptGridSizeY;
		wxStaticText* m_staticTextGridPosX;
		wxTextCtrl* m_GridOriginXCtrl;
		wxStaticText* m_TextPosXUnits;
		wxStaticText* m_staticTextGridPosY;
		wxTextCtrl* m_GridOriginYCtrl;
		wxStaticText* m_TextPosYUnits;
		wxButton* m_buttonReset;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnResetGridOrgClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_SET_GRID_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Grid Origin and User Grid Size"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 374,267 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SET_GRID_BASE();
	
};

#endif //__dialog_set_grid_base__
