///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_design_rules_base__
#define __dialog_design_rules_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/html/htmlwin.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_LAYERS_COUNT_SELECTION 1000
#define ID_LAYERS_PROPERTIES 1001
#define wxID_ADD_NETCLASS 1002
#define wxID_REMOVE_NETCLASS 1003
#define ID_LEFT_CHOICE_CLICK 1004
#define ID_LEFT_TO_RIGHT_COPY 1005
#define ID_RIGHT_TO_LEFT_COPY 1006
#define ID_RIGHT_CHOICE_CLICK 1007

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DESIGN_RULES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DESIGN_RULES_BASE : public wxDialog 
{
	private:
	
	protected:
		wxNotebook* m_notebook;
		wxPanel* m_panelLayers;
		wxRadioBox* m_LayersCountSelection;
		wxGrid* m_gridLayersProperties;
		wxPanel* m_panelNetClasses;
		wxGrid* m_gridNetClassesProperties;
		wxButton* m_buttonADD;
		wxButton* m_buttonRemove;
		wxChoice* m_CBoxLeftSelection;
		wxListBox* m_listBoxLeftNetSelect;
		wxButton* m_buttonRightToLeft;
		wxButton* m_buttonLeftToRight;
		wxButton* m_buttonLeftSelAll;
		wxButton* m_buttonRightSelAll;
		wxChoice* m_CBoxRightSelection;
		wxListBox* m_listBoxRightNetSelect;
		wxStaticText* m_staticTextMsg;
		wxHtmlWindow* m_MessagesList;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnLayerCountClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLayerGridLeftClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnLayerGridRighttClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnNetClassesGridLeftClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnNetClassesGridRightClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnAddNetclassClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveNetclassClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftCBSelection( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightToLeftCopyButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftToRightCopyButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftSelectAllButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightSelectAllButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightCBSelection( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_DESIGN_RULES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Design Rules Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 684,568 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_DESIGN_RULES_BASE();
	
};

#endif //__dialog_design_rules_base__
