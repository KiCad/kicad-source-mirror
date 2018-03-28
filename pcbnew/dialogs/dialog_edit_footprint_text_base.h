///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_FOOTPRINT_TEXT_BASE_H__
#define __DIALOG_EDIT_FOOTPRINT_TEXT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;
class TEXT_CTRL_EVAL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpcbox.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_FPTEXT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_FPTEXT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_TextDataTitle;
		wxTextCtrl* m_Name;
		wxStaticText* m_widthLabel;
		TEXT_CTRL_EVAL* m_widthCtrl;
		wxStaticText* m_widthUnits;
		wxStaticText* m_heightLabel;
		TEXT_CTRL_EVAL* m_heightCtrl;
		wxStaticText* m_heightUnits;
		wxStaticText* m_thicknessLabel;
		TEXT_CTRL_EVAL* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxStaticText* m_posXLabel;
		TEXT_CTRL_EVAL* m_posXCtrl;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYLabel;
		TEXT_CTRL_EVAL* m_posYCtrl;
		wxStaticText* m_posYUnits;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxCheckBox* m_Show;
		wxCheckBox* m_Italic;
		wxStaticText* stOrienationLabel;
		wxRadioButton* m_Orient0;
		wxRadioButton* m_Orient90;
		wxRadioButton* m_Orient270;
		wxRadioButton* m_Orient180;
		wxRadioButton* m_OrientOther;
		wxTextCtrl* m_OrientValueCtrl;
		wxCheckBox* m_unlock;
		wxStaticLine* m_staticline2;
		wxStaticText* m_statusLine1;
		wxStaticText* m_statusLine2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void ModuleOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOtherOrientation( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_FPTEXT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EDIT_FPTEXT_BASE();
	
};

#endif //__DIALOG_EDIT_FOOTPRINT_TEXT_BASE_H__
