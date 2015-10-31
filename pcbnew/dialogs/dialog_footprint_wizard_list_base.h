///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_FOOTPRINT_WIZARD_LIST_BASE_H__
#define __DIALOG_FOOTPRINT_WIZARD_LIST_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_WIZARD_LIST_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_WIZARD_LIST_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_footprintGeneratorsGrid;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCellFpGeneratorClick( wxGridEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FOOTPRINT_WIZARD_LIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Generators"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 501,273 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FOOTPRINT_WIZARD_LIST_BASE();
	
};

#endif //__DIALOG_FOOTPRINT_WIZARD_LIST_BASE_H__
