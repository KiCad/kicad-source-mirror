///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIBEDIT_DIMENSIONS_BASE_H__
#define __DIALOG_LIBEDIT_DIMENSIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIBEDIT_DIMENSIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIBEDIT_DIMENSIONS_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnSaveSetupClick( wxCommandEvent& event ){ OnSaveSetupClick( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnOkClick( wxCommandEvent& event ){ OnOkClick( event ); }
		
	
	protected:
		wxStaticText* m_staticText3;
		wxChoice* m_choiceGridSize;
		wxStaticText* m_staticGridUnits;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_CurrentGraphicLineThicknessCtrl;
		wxStaticText* m_staticLineWidthUnits;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_CurrentGraphicTextSizeCtrl;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticText9;
		wxChoice* m_choiceRepeatHorizontal;
		wxStaticText* m_staticRepeatXUnits;
		wxStaticText* m_staticText12;
		wxChoice* m_choiceRepeatVertical;
		wxStaticText* m_staticRepeatYUnits;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_CurrentPinLenghtCtrl;
		wxStaticText* m_staticText161;
		wxStaticText* m_CurrentPinNameSizeText;
		wxTextCtrl* m_CurrentPinNameSizeCtrl;
		wxStaticText* m_staticText18;
		wxStaticText* m_CurrentPinNumberSizeText;
		wxTextCtrl* m_CurrentPinNumberSizeCtrl;
		wxStaticText* m_staticText20;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_spinRepeatLabel;
		wxStaticLine* m_staticline1;
		wxButton* m_buttonSave;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSaveSetupClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_LIBEDIT_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Library Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 412,349 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIBEDIT_DIMENSIONS_BASE();
	
};

#endif //__DIALOG_LIBEDIT_DIMENSIONS_BASE_H__
