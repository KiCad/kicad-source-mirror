///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_MODULE_FOR_MODEDIT_BASE_H__
#define __DIALOG_EDIT_MODULE_FOR_MODEDIT_BASE_H__

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
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/slider.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NOTEBOOK 1000
#define ID_BROWSE_3D_LIB 1001
#define ID_REMOVE_3D_SHAPE 1002

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MODULE_MODULE_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MODULE_MODULE_EDITOR_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* m_GeneralBoxSizer;
		wxBoxSizer* m_PropRightSizer;
	
	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelProperties;
		wxStaticText* m_staticTextDoc;
		wxTextCtrl* m_DocCtrl;
		wxStaticText* m_staticTextKeywords;
		wxTextCtrl* m_KeywordCtrl;
		wxStaticText* m_staticTextRef;
		wxTextCtrl* m_ReferenceCtrl;
		wxButton* m_button4;
		wxStaticText* m_staticTextVal;
		wxTextCtrl* m_ValueCtrl;
		wxButton* m_button5;
		wxStaticText* m_staticTextFp;
		wxTextCtrl* m_FootprintNameCtrl;
		wxRadioBox* m_AttributsCtrl;
		wxRadioBox* m_AutoPlaceCtrl;
		wxStaticText* m_staticText11;
		wxSlider* m_CostRot90Ctrl;
		wxStaticText* m_staticText12;
		wxSlider* m_CostRot180Ctrl;
		wxStaticText* m_staticTextInfo;
		wxStaticText* m_staticTextNetClearance;
		wxTextCtrl* m_NetClearanceValueCtrl;
		wxStaticText* m_NetClearanceUnits;
		wxStaticLine* m_staticline1;
		wxStaticLine* m_staticline2;
		wxStaticLine* m_staticline3;
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_SolderMaskMarginCtrl;
		wxStaticText* m_SolderMaskMarginUnits;
		wxStaticText* m_staticTextSolderPaste;
		wxTextCtrl* m_SolderPasteMarginCtrl;
		wxStaticText* m_SolderPasteMarginUnits;
		wxStaticText* m_staticTextRatio;
		wxTextCtrl* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxPanel* m_Panel3D;
		wxStaticText* m_staticText3Dname;
		wxListBox* m_3D_ShapeNameListBox;
		wxStaticText* m_staticTextDefault3DPath;
		wxTextCtrl* m_textCtrl3DDefaultPath;
		wxStaticBoxSizer* m_Sizer3DValues;
		wxBoxSizer* m_bSizerShapeScale;
		wxStaticText* m_staticTextShapeScale;
		wxBoxSizer* m_bSizerShapeOffset;
		wxStaticText* m_staticTextShapeOffset;
		wxBoxSizer* m_bSizerShapeRotation;
		wxStaticText* m_staticTextShapeRotation;
		wxButton* m_buttonBrowse;
		wxButton* m_buttonRemove;
		wxButton* m_buttonEdit;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnEditReference( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditValue( wxCommandEvent& event ) { event.Skip(); }
		virtual void On3DShapeNameSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void Add3DShape( wxCommandEvent& event ) { event.Skip(); }
		virtual void Remove3DShape( wxCommandEvent& event ) { event.Skip(); }
		virtual void Edit3DShapeFilename( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_MODULE_MODULE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 486,510 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_MODULE_MODULE_EDITOR_BASE();
	
};

#endif //__DIALOG_EDIT_MODULE_FOR_MODEDIT_BASE_H__
