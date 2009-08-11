///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_edit_module_for_BoardEditor_base__
#define __dialog_edit_module_for_BoardEditor_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NOTEBOOK 1000
#define ID_LISTBOX_ORIENT_SELECT 1001
#define ID_MODULE_PROPERTIES_EXCHANGE 1002
#define ID_GOTO_MODULE_EDITOR 1003
#define ID_BROWSE_3D_LIB 1004
#define ID_ADD_3D_SHAPE 1005
#define ID_REMOVE_3D_SHAPE 1006

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MODULE_BOARD_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MODULE_BOARD_EDITOR_BASE : public wxDialog 
{
	private:
		wxBoxSizer* m_GeneralBoxSizer;
		wxBoxSizer* m_PropRightSizer;
	
	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelProperties;
		wxTextCtrl* m_ReferenceCtrl;
		wxButton* m_button4;
		wxTextCtrl* m_ValueCtrl;
		wxButton* m_button5;
		wxRadioBox* m_LayerCtrl;
		wxRadioBox* m_OrientCtrl;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_OrientValue;
		wxStaticText* XPositionStatic;
		wxTextCtrl* m_ModPositionX;
		wxStaticText* YPositionStatic;
		wxTextCtrl* m_ModPositionY;
		wxButton* m_buttonExchange;
		wxButton* m_buttonModuleEditor;
		
		wxRadioBox* m_AttributsCtrl;
		wxRadioBox* m_AutoPlaceCtrl;
		wxStaticText* m_staticText11;
		wxSlider* m_CostRot90Ctrl;
		wxStaticText* m_staticText12;
		wxSlider* m_CostRot180Ctrl;
		wxPanel* m_Panel3D;
		wxStaticText* m_staticText3Dname;
		wxListBox* m_3D_ShapeNameListBox;
		wxButton* m_buttonBrowse;
		wxButton* m_buttonAdd;
		wxButton* m_buttonRemove;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnEditReference( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEditValue( wxCommandEvent& event ){ event.Skip(); }
		virtual void ModuleOrientEvent( wxCommandEvent& event ){ event.Skip(); }
		virtual void ExchangeModule( wxCommandEvent& event ){ event.Skip(); }
		virtual void GotoModuleEditor( wxCommandEvent& event ){ event.Skip(); }
		virtual void On3DShapeNameSelected( wxCommandEvent& event ){ event.Skip(); }
		virtual void Browse3DLib( wxCommandEvent& event ){ event.Skip(); }
		virtual void Add3DShape( wxCommandEvent& event ){ event.Skip(); }
		virtual void Remove3DShape( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxStaticBoxSizer* m_Sizer3DValues;
		DIALOG_MODULE_BOARD_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Module properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 422,583 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_MODULE_BOARD_EDITOR_BASE();
	
};

#endif //__dialog_edit_module_for_BoardEditor_base__
