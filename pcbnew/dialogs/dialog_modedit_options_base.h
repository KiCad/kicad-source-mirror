///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 27 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_MODEDIT_OPTIONS_BASE_H__
#define __DIALOG_MODEDIT_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class TEXT_CTRL_EVAL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_POLAR_CTRL 1000
#define wxID_UNITS 1001
#define wxID_SEGMENTS45 1002

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MODEDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MODEDIT_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_EdgeModEWidthTitle;
		TEXT_CTRL_EVAL* m_OptModuleGrLineWidth;
		wxStaticText* m_staticTextGrLineUnit;
		wxStaticText* m_TextModWidthTitle;
		TEXT_CTRL_EVAL* m_OptModuleTextWidth;
		wxStaticText* m_staticTextTextWidthUnit;
		wxStaticText* m_TextModSizeVTitle;
		TEXT_CTRL_EVAL* m_OptModuleTextVSize;
		wxStaticText* m_staticTextTextVSizeUnit;
		wxStaticText* m_TextModSizeHTitle;
		TEXT_CTRL_EVAL* m_OptModuleTextHSize;
		wxStaticText* m_staticTextTextHSizeUnit;
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxStaticText* m_staticTextRef;
		wxTextCtrl* m_textCtrlRefText;
		wxChoice* m_choiceLayerReference;
		wxChoice* m_choiceVisibleReference;
		wxStaticText* m_staticTextValue;
		wxTextCtrl* m_textCtrlValueText;
		wxChoice* m_choiceLayerValue;
		wxChoice* m_choiceVisibleValue;
		wxStaticText* m_staticTextInfo;
		wxCheckBox* m_MagneticPads;
		wxCheckBox* m_Segments_45_Only_Ctrl;
		wxCheckBox* m_dragSelects;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		
		DIALOG_MODEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_MODEDIT_OPTIONS_BASE();
	
};

#endif //__DIALOG_MODEDIT_OPTIONS_BASE_H__
