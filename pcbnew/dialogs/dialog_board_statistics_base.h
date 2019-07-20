///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_BOARD_STATISTICS_BASE_H__
#define __DIALOG_BOARD_STATISTICS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_STATISTICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_STATISTICS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_gridComponents;
		wxGrid* m_gridPads;
		wxGrid* m_gridBoard;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_checkBoxSubtractHoles;
		wxCheckBox* m_checkBoxExcludeComponentsNoPins;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbControlSizer;
		wxButton* m_sdbControlSizerOK;
		
		// Virtual event handlers, overide them in your derived class
		virtual void checkboxClicked( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Board Statistics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~DIALOG_BOARD_STATISTICS_BASE();
	
};

#endif //__DIALOG_BOARD_STATISTICS_BASE_H__
