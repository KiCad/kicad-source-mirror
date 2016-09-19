///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 12 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXPORT_STEP_BASE_H__
#define __DIALOG_EXPORT_STEP_BASE_H__

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
#include <wx/filepicker.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_STEP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_STEP_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_txtBrdFile;
		wxFilePickerCtrl* m_filePickerSTEP;
		wxStaticText* m_staticText6;
		wxCheckBox* m_cbDrillOrigin;
		wxCheckBox* m_cbAuxOrigin;
		wxCheckBox* m_cbUserOrigin;
		wxStaticText* m_staticText2;
		wxStaticText* m_staticText5;
		wxChoice* m_STEP_OrgUnitChoice;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_STEP_Xorg;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_STEP_Yorg;
		wxCheckBox* m_cbRemoveVirtual;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export STEP"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXPORT_STEP_BASE();
	
};

#endif //__DIALOG_EXPORT_STEP_BASE_H__
