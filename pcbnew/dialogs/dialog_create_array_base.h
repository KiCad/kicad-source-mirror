///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

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
#include <wx/radiobut.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_DIALOG_CREATE_ARRAY 10000

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
		TEXT_CTRL_EVAL* m_entryNx;
		wxStaticText* m_labelNy;
		TEXT_CTRL_EVAL* m_entryNy;
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
		TEXT_CTRL_EVAL* m_entryStagger;
		wxRadioButton* m_staggerRows;
		wxRadioButton* m_staggerCols;
		wxRadioButton* m_rbItemsRemainInPlace;
		wxRadioButton* m_rbCentreOnSource;
		wxPanel* m_gridPadNumberingPanel;
		wxBoxSizer* m_gridPadNumberingSizer;
		wxCheckBox* m_cbRenumberPads;
		wxRadioBox* m_radioBoxGridNumberingAxis;
		wxCheckBox* m_checkBoxGridReverseNumbering;
		wxRadioBox* m_rbGridStartNumberingOpt;
		wxRadioBox* m_radioBoxGridNumberingScheme;
		wxStaticText* m_labelPriAxisNumbering;
		wxChoice* m_choicePriAxisNumbering;
		wxStaticText* m_labelSecAxisNumbering;
		wxChoice* m_choiceSecAxisNumbering;
		wxStaticText* m_labelGridNumberingOffset;
		wxTextCtrl* m_entryGridPriNumberingOffset;
		wxTextCtrl* m_entryGridSecNumberingOffset;
		wxStaticText* m_labelGridNumberingStep;
		wxTextCtrl* m_entryGridPriNumberingStep;
		wxTextCtrl* m_entryGridSecNumberingStep;
		wxPanel* m_circularPanel;
		wxStaticText* m_labelCentreX;
		wxTextCtrl* m_entryCentreX;
		wxStaticText* m_unitLabelCentreX;
		wxStaticText* m_labelCentreY;
		wxTextCtrl* m_entryCentreY;
		wxStaticText* m_unitLabelCentreY;
		wxButton* m_btnSelectCenterPoint;
		wxButton* m_btnSelectCenterItem;
		wxCheckBox* m_checkBoxFullCircle;
		wxRadioBox* m_rbCircDirection;
		wxStaticText* m_labelCircAngle;
		wxTextCtrl* m_entryCircAngle;
		wxStaticText* m_unitLabelCircAngle;
		wxStaticText* m_labelCircCount;
		TEXT_CTRL_EVAL* m_entryCircCount;
		wxStaticText* m_labelCircOffset;
		TEXT_CTRL_EVAL* m_entryCircOffset;
		wxStaticText* m_unitLabelCircOffset;
		wxCheckBox* m_entryRotateItemsCb;
		wxPanel* m_circularPadNumberingPanel;
		wxStaticBoxSizer* m_circPadNumberingSizer;
		wxRadioBox* m_rbCircStartNumberingOpt;
		wxStaticText* m_labelCircNumbering;
		wxChoice* m_choiceCircNumbering;
		wxStaticText* m_labelCircNumStart;
		wxTextCtrl* m_entryCircNumberingStart;
		wxStaticText* m_labelCircNumStep;
		wxTextCtrl* m_entryCircNumberingStep;
		wxPanel* m_optionsPanel;
		wxPanel* m_itemSourcePanel;
		wxRadioButton* m_radioBtnDuplicateSelection;
		wxRadioButton* m_radioBtnArrangeSelection;
		wxPanel* m_footprintReannotatePanel;
		wxRadioButton* m_radioBtnKeepRefs;
		wxRadioButton* m_radioBtnUniqueRefs;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnParameterChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAxisNumberingChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectCenterButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_CREATE_ARRAY_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_CREATE_ARRAY, const wxString& title = _("Create Array"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CREATE_ARRAY_BASE();

};

