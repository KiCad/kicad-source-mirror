///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_build_BOM_base__
#define __dialog_build_BOM_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BUILD_BOM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BUILD_BOM_BASE : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_RADIOBOX_SELECT_FORMAT = 1000,
		};
		
		wxCheckBox* m_ListCmpbyRefItems;
		wxCheckBox* m_ListSubCmpItems;
		wxCheckBox* m_ListCmpbyValItems;
		wxCheckBox* m_GenListLabelsbyVal;
		wxCheckBox* m_GenListLabelsbySheet;
		wxRadioBox* m_OutputFormCtrl;
		wxRadioBox* m_OutputSeparatorCtrl;
		wxCheckBox* m_GetListBrowser;
		wxCheckBox* m_AddFootprintField;
		wxCheckBox* m_AddField1;
		wxCheckBox* m_AddField2;
		wxCheckBox* m_AddField3;
		wxCheckBox* m_AddField4;
		wxCheckBox* m_AddField5;
		wxCheckBox* m_AddField6;
		wxCheckBox* m_AddField7;
		wxCheckBox* m_AddField8;
		wxCheckBox* m_AddAllFields;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRadioboxSelectFormatSelected( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_BUILD_BOM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("List of Material"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 415,382 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_BUILD_BOM_BASE();
	
};

#endif //__dialog_build_BOM_base__
