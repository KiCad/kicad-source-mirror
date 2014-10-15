///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EESCHEMA_OPTIONS_BASE_H__
#define __DIALOG_EESCHEMA_OPTIONS_BASE_H__

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
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EESCHEMA_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EESCHEMA_OPTIONS_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnSize( wxSizeEvent& event ){ OnSize( event ); }
		void _wxFB_OnChooseUnits( wxCommandEvent& event ){ OnChooseUnits( event ); }
		void _wxFB_OnMiddleBtnPanEnbl( wxCommandEvent& event ){ OnMiddleBtnPanEnbl( event ); }
		void _wxFB_OnTemplateFieldDeselected( wxListEvent& event ){ OnTemplateFieldDeselected( event ); }
		void _wxFB_OnTemplateFieldSelected( wxListEvent& event ){ OnTemplateFieldSelected( event ); }
		void _wxFB_OnAddButtonClick( wxCommandEvent& event ){ OnAddButtonClick( event ); }
		void _wxFB_OnDeleteButtonClick( wxCommandEvent& event ){ OnDeleteButtonClick( event ); }
		
	
	protected:
		enum
		{
			ID_M_SPINAUTOSAVEINTERVAL = 1000,
			xwID_ANY,
			wxID_ADD_FIELD,
			wxID_DELETE_FIELD
		};
		
		wxNotebook* m_notebook1;
		wxPanel* m_panel1;
		wxStaticText* m_staticText2;
		wxChoice* m_choiceUnits;
		wxStaticText* m_staticText3;
		wxChoice* m_choiceGridSize;
		wxStaticText* m_staticGridUnits;
		wxStaticText* m_staticText51;
		wxSpinCtrl* m_spinBusWidth;
		wxStaticText* m_staticBusWidthUnits;
		wxStaticText* m_staticText5;
		wxSpinCtrl* m_spinLineWidth;
		wxStaticText* m_staticLineWidthUnits;
		wxStaticText* m_staticText7;
		wxSpinCtrl* m_spinTextSize;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticText9;
		wxSpinCtrl* m_spinRepeatHorizontal;
		wxStaticText* m_staticRepeatXUnits;
		wxStaticText* m_staticText12;
		wxSpinCtrl* m_spinRepeatVertical;
		wxStaticText* m_staticRepeatYUnits;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_spinRepeatLabel;
		wxStaticText* m_staticText221;
		wxSpinCtrl* m_spinAutoSaveInterval;
		wxStaticText* m_staticText23;
		wxStaticText* m_staticText26;
		wxChoice* m_choiceSeparatorRefId;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_checkShowGrid;
		wxCheckBox* m_checkShowHiddenPins;
		wxCheckBox* m_checkEnableZoomNoCenter;
		wxCheckBox* m_checkEnableMiddleButtonPan;
		wxCheckBox* m_checkMiddleButtonPanLimited;
		wxCheckBox* m_checkAutoPan;
		wxCheckBox* m_checkHVOrientation;
		wxCheckBox* m_checkPageLimits;
		wxPanel* m_panel2;
		wxListCtrl* templateFieldListCtrl;
		wxStaticText* fieldNameLabel;
		wxTextCtrl* fieldNameTextCtrl;
		wxStaticText* fieldDefaultValueLabel;
		wxTextCtrl* fieldDefaultValueTextCtrl;
		wxCheckBox* fieldVisibleCheckbox;
		wxButton* addFieldButton;
		wxButton* deleteFieldButton;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnChooseUnits( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMiddleBtnPanEnbl( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTemplateFieldDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnTemplateFieldSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnAddButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EESCHEMA_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Schematic Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~DIALOG_EESCHEMA_OPTIONS_BASE();
	
};

#endif //__DIALOG_EESCHEMA_OPTIONS_BASE_H__
