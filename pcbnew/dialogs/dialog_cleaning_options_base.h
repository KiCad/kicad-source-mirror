///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CLEANING_OPTIONS_BASE_H__
#define __DIALOG_CLEANING_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANING_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_cleanShortCircuitOpt;
		wxCheckBox* m_cleanViasOpt;
		wxCheckBox* m_mergeSegmOpt;
		wxCheckBox* m_deleteUnconnectedOpt;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_CLEANING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleaning Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 342,179 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CLEANING_OPTIONS_BASE();
	
};

#endif //__DIALOG_CLEANING_OPTIONS_BASE_H__
