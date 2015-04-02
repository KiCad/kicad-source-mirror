///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_TARGET_PROPERTIES_BASE_H__
#define __DIALOG_TARGET_PROPERTIES_BASE_H__

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
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class TARGET_PROPERTIES_DIALOG_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class TARGET_PROPERTIES_DIALOG_EDITOR_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextSize;
		wxTextCtrl* m_TargetSizeCtrl;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticTextThickness;
		wxTextCtrl* m_TargetThicknessCtrl;
		wxStaticText* m_staticTextThicknessUnits;
		wxStaticText* m_staticTextShape;
		wxChoice* m_TargetShape;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizerButts;
		wxButton* m_sdbSizerButtsOK;
		wxButton* m_sdbSizerButtsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		TARGET_PROPERTIES_DIALOG_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Target Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~TARGET_PROPERTIES_DIALOG_EDITOR_BASE();
	
};

#endif //__DIALOG_TARGET_PROPERTIES_BASE_H__
