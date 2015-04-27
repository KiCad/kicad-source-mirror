///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_RESCUE_SUMMARY_BASE_H__
#define __DIALOG_RESCUE_SUMMARY_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RESCUE_SUMMARY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RESCUE_SUMMARY_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText5;
		wxDataViewListCtrl* m_ListOfChanges;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
	
	public:
		
		DIALOG_RESCUE_SUMMARY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Summary of Library Rescue"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 536,385 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_RESCUE_SUMMARY_BASE();
	
};

#endif //__DIALOG_RESCUE_SUMMARY_BASE_H__
