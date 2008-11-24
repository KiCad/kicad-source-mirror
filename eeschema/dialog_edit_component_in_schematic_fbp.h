///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  7 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_edit_component_in_schematic_fbp__
#define __dialog_edit_component_in_schematic_fbp__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP : public wxDialog 
{
	private:
	
	protected:
		wxChoice* unitChoice;
		wxRadioBox* orientationRadioBox;
		wxRadioBox* mirrorRadioBox;
		wxTextCtrl* chipnameTxtControl;
		wxCheckBox* convertCheckBox;
		wxGrid* fieldGrid;
		wxButton* addFieldButton;
		wxButton* deleteFieldButton;
		wxButton* moveUpButton;
		wxCheckBox* showCheckBox;
		wxCheckBox* rotateCheckBox;
		wxTextCtrl* fieldNameTextCtrl;
		wxTextCtrl* m_textCtrl3;
		wxTextCtrl* textSizeTextCtrl;
		wxTextCtrl* posXTextCtrl;
		wxTextCtrl* posYTextCtrl;
		wxButton* defaultsButton;
		wxStdDialogButtonSizer* stdDialogButtonSizer;
		wxButton* stdDialogButtonSizerOK;
		wxButton* stdDialogButtonSizerCancel;
	
	public:
		
		DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Component Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 864,540 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP();
	
};

#endif //__dialog_edit_component_in_schematic_fbp__
