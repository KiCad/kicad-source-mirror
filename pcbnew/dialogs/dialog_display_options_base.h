///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_DISPLAY_OPTIONS_BASE_H__
#define __DIALOG_DISPLAY_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DISPLAY_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_SHOW_CLEARANCE = 1000
		};
		
		wxBoxSizer* sLeftSizer;
		wxRadioBox* m_ShowNetNamesOption;
		wxCheckBox* m_OptDisplayPadNumber;
		wxCheckBox* m_OptDisplayPadNoConn;
		wxRadioBox* m_OptDisplayTracksClearance;
		wxCheckBox* m_OptDisplayPadClearence;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_DISPLAY_OPTIONS_BASE();
	
};

#endif //__DIALOG_DISPLAY_OPTIONS_BASE_H__
