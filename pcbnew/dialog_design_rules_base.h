///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_base__
#define __dialog_design_rules_base__

#include <wx/intl.h>

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/html/htmlwin.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DESIGN_RULES_BASE : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_ADD_NETCLASS = 1000,
			wxID_REMOVE_NETCLASS,
			ID_LEFT_CHOICE_CLICK,
			ID_LEFT_TO_RIGHT_COPY,
			ID_RIGHT_TO_LEFT_COPY,
			ID_RIGHT_CHOICE_CLICK,
		};
		
		wxNotebook* m_DRnotebook;
		wxPanel* m_panelNetClassesEditor;
		wxGrid* m_grid;
		wxButton* m_addButton;
		wxButton* m_removeButton;
		wxButton* m_moveUpButton;
		wxChoice* m_leftClassChoice;
		wxListCtrl* m_leftListCtrl;
		wxButton* m_buttonRightToLeft;
		wxButton* m_buttonLeftToRight;
		wxButton* m_buttonLeftSelAll;
		wxButton* m_buttonRightSelAll;
		wxChoice* m_rightClassChoice;
		wxListCtrl* m_rightListCtrl;
		wxPanel* m_panelGolbalDesignRules;
		wxRadioBox* m_OptViaType;
		wxStaticText* m_ViaMinTitle;
		wxTextCtrl* m_SetViasMinSizeCtrl;
		wxStaticText* m_ViaMinDrillTitle;
		wxTextCtrl* m_SetViasMinDrillCtrl;
		wxRadioBox* m_AllowMicroViaCtrl;
		wxStaticText* m_MicroViaMinSizeTitle;
		wxTextCtrl* m_SetMicroViasMinSizeCtrl;
		wxStaticText* m_MicroViaMinDrillTitle;
		wxTextCtrl* m_SetMicroViasMinDrillCtrl;
		wxStaticText* m_TrackMinWidthTitle;
		wxTextCtrl* m_SetTrackMinWidthCtrl;
		wxHtmlWindow* m_MessagesList;
		wxStdDialogButtonSizer* m_sdbButtonsSizer;
		wxButton* m_sdbButtonsSizerOK;
		wxButton* m_sdbButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnNetClassesNameLeftClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnNetClassesNameRightClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnAddNetclassClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveNetclassClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMoveUpSelectedNetClass( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftCBSelection( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightToLeftCopyButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftToRightCopyButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftSelectAllButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightSelectAllButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightCBSelection( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_DESIGN_RULES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Design Rules Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 792,692 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_DESIGN_RULES_BASE();
	
};

#endif //__dialog_design_rules_base__
