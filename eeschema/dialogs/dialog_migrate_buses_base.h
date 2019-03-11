///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 17 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_MIGRATE_BUSES_BASE_H__
#define __DIALOG_MIGRATE_BUSES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class wxListView;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MIGRATE_BUSES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MIGRATE_BUSES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText5;
		wxStaticText* m_staticText7;
		wxListView* m_migration_list;
		wxStaticText* m_staticText6;
		wxComboBox* m_cb_new_name;
		wxButton* m_btn_accept;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
	
	public:
		
		DIALOG_MIGRATE_BUSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Migrate Buses"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~DIALOG_MIGRATE_BUSES_BASE();
	
};

#endif //__DIALOG_MIGRATE_BUSES_BASE_H__
