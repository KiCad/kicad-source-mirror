///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_pad_properties_base__
#define __dialog_pad_properties_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogPadPropertiesBase
///////////////////////////////////////////////////////////////////////////////
class DialogPadPropertiesBase : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_DIALOG_EDIT_PAD = 1000,
			wxID_PADNUMCTRL,
			wxID_PADNETNAMECTRL,
			ID_LISTBOX_SHAPE_PAD,
			ID_RADIOBOX_DRILL_SHAPE,
			ID_LISTBOX_ORIENT_PAD,
			ID_LISTBOX_TYPE_PAD,
		};
		
		wxStaticText* m_PadNumText;
		wxTextCtrl* m_PadNumCtrl;
		wxStaticText* m_PadNameText;
		wxTextCtrl* m_PadNetNameCtrl;
		wxBoxSizer* m_PadPositionBoxSizer;
		wxBoxSizer* m_DrillShapeBoxSizer;
		wxRadioBox* m_PadShape;
		
		wxRadioBox* m_DrillShapeCtrl;
		wxRadioBox* m_PadOrient;
		wxStaticText* m_PadOrientText;
		wxTextCtrl* m_PadOrientCtrl;
		
		wxRadioBox* m_PadType;
		wxButton* m_buttonOk;
		wxButton* m_buttonCancel;
		
		wxCheckBox* m_PadLayerCu;
		wxCheckBox* m_PadLayerCmp;
		
		wxCheckBox* m_PadLayerAdhCmp;
		wxCheckBox* m_PadLayerAdhCu;
		wxCheckBox* m_PadLayerPateCmp;
		wxCheckBox* m_PadLayerPateCu;
		wxCheckBox* m_PadLayerSilkCmp;
		wxCheckBox* m_PadLayerSilkCu;
		wxCheckBox* m_PadLayerMaskCmp;
		wxCheckBox* m_PadLayerMaskCu;
		wxCheckBox* m_PadLayerECO1;
		wxCheckBox* m_PadLayerECO2;
		wxCheckBox* m_PadLayerDraft;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPadShapeSelection( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ){ event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ){ event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ){ event.Skip(); }
		virtual void PadPropertiesAccept( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DialogPadPropertiesBase( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Pad Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 520,396 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSUNKEN_BORDER );
		~DialogPadPropertiesBase();
	
};

#endif //__dialog_pad_properties_base__
