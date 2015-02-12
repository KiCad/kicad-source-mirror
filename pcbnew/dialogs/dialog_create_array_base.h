///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CREATE_ARRAY_BASE_H__
#define __DIALOG_CREATE_ARRAY_BASE_H__

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
#include <wx/radiobox.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_DIALOG_CREATE_ARRAY 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CREATE_ARRAY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CREATE_ARRAY_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxNotebook* m_gridTypeNotebook;
		wxPanel* m_gridPanel;
		wxStaticText* m_labelNx;
		wxTextCtrl* m_entryNx;
		wxStaticText* m_labelNy;
		wxTextCtrl* m_entryNy;
		wxStaticText* m_labelDx;
		wxTextCtrl* m_entryDx;
		wxStaticText* m_unitLabelDx;
		wxStaticText* m_labelDy;
		wxTextCtrl* m_entryDy;
		wxStaticText* m_unitLabelDy;
		wxStaticText* m_labelOffsetX;
		wxTextCtrl* m_entryOffsetX;
		wxStaticText* m_unitLabelOffsetX;
		wxStaticText* m_labelOffsetY;
		wxTextCtrl* m_entryOffsetY;
		wxStaticText* m_unitLabelOffsetY;
		wxStaticText* m_labelStagger;
		wxTextCtrl* m_entryStagger;
		wxRadioBox* m_radioBoxGridStaggerType;
		wxStaticText* m_labelGridStaggerType;
		wxRadioBox* m_radioBoxGridNumberingAxis;
		wxCheckBox* m_checkBoxGridReverseNumbering;
		wxCheckBox* m_checkBoxGridRestartNumbering;
		wxRadioBox* m_radioBoxGridNumberingScheme;
		wxStaticText* m_labelPriAxisNumbering;
		wxChoice* m_choicePriAxisNumbering;
		wxStaticText* m_labelSecAxisNumbering;
		wxChoice* m_choiceSecAxisNumbering;
		wxStaticText* m_labelGridNumberingOffset;
		wxTextCtrl* m_entryGridPriNumberingOffset;
		wxTextCtrl* m_entryGridSecNumberingOffset;
		wxPanel* m_circularPanel;
		wxStaticText* m_labelCentreX;
		wxTextCtrl* m_entryCentreX;
		wxStaticText* m_unitLabelCentreX;
		wxStaticText* m_labelCentreY;
		wxTextCtrl* m_entryCentreY;
		wxStaticText* m_unitLabelCentreY;
		wxStaticText* m_labelCircAngle;
		wxTextCtrl* m_entryCircAngle;
		wxStaticText* m_unitLabelCircAngle;
		wxStaticText* m_labelCircCount;
		wxTextCtrl* m_entryCircCount;
		wxStaticText* m_labelCircRotate;
		wxCheckBox* m_entryRotateItemsCb;
		wxCheckBox* m_checkBoxCircRestartNumbering;
		wxStaticText* m_labelCircNumbering;
		wxChoice* m_choiceCircNumberingType;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnParameterChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CREATE_ARRAY_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_CREATE_ARRAY, const wxString& title = _("Create array"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 576,528 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CREATE_ARRAY_BASE();
	
};

#endif //__DIALOG_CREATE_ARRAY_BASE_H__
