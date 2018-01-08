///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIBEDIT_OPTIONS_BASE_H__
#define __DIALOG_LIBEDIT_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/stepped_slider.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIBEDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIBEDIT_OPTIONS_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnScaleSlider( wxScrollEvent& event ){ OnScaleSlider( event ); }
		void _wxFB_OnScaleAuto( wxCommandEvent& event ){ OnScaleAuto( event ); }
		
	
	protected:
		wxStaticText* m_staticText3;
		wxChoice* m_choiceGridSize;
		wxStaticText* m_staticGridUnits;
		wxStaticText* m_staticText5;
		wxSpinCtrl* m_spinLineWidth;
		wxStaticText* m_staticLineWidthUnits;
		wxStaticText* m_staticText52;
		wxSpinCtrl* m_spinPinLength;
		wxStaticText* m_staticPinLengthUnits;
		wxStaticText* m_staticText7;
		wxSpinCtrl* m_spinPinNumSize;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticText9;
		wxSpinCtrl* m_spinPinNameSize;
		wxStaticText* m_staticRepeatXUnits;
		wxStaticText* m_staticText11;
		wxSpinCtrl* m_spinRepeatHorizontal;
		wxStaticText* m_staticText12;
		wxStaticText* m_staticText13;
		wxSpinCtrl* m_spinRepeatVertical;
		wxStaticText* m_staticText14;
		wxStaticText* m_staticText15;
		wxChoice* m_choicePinDisplacement;
		wxStaticText* m_staticText16;
		wxStaticText* m_staticText17;
		wxSpinCtrl* m_spinRepeatLabel;
		wxStaticText* m_staticText18;
		STEPPED_SLIDER* m_scaleSlider;
		wxStaticText* m_staticText19;
		wxCheckBox* m_scaleAuto;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkShowGrid;
		wxCheckBox* m_checkShowPinElectricalType;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnScaleSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnScaleAuto( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_LIBEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Library Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIBEDIT_OPTIONS_BASE();
	
};

#endif //__DIALOG_LIBEDIT_OPTIONS_BASE_H__
