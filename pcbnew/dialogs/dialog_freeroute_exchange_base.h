///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_FREEROUTE_EXCHANGE_BASE_H__
#define __DIALOG_FREEROUTE_EXCHANGE_BASE_H__

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
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_BUTTON_EXPORT_DSN 1000
#define wxID_BUTTON_LAUNCH 1001
#define wxID_BUTTON_IMPORT 1002

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FREEROUTE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FREEROUTE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText2;
		wxButton* m_ExportDSN;
		wxButton* m_buttonLaunchFreeroute;
		wxButton* m_buttonImport;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnExportButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLaunchButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnImportButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FREEROUTE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FREEROUTE_BASE();
	
};

#endif //__DIALOG_FREEROUTE_EXCHANGE_BASE_H__
