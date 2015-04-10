///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PCB_TEXT_PROPERTIES_BASE_H__
#define __DIALOG_PCB_TEXT_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PCB_TEXT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PCB_TEXT_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_TextLabel;
		wxTextCtrl* m_TextContentCtrl;
		wxStaticText* m_SizeXLabel;
		wxStaticText* m_PositionXLabel;
		wxStaticText* m_LayerLabel;
		wxStaticText* m_staticText10;
		wxTextCtrl* m_SizeXCtrl;
		wxTextCtrl* m_PositionXCtrl;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxChoice* m_DisplayCtrl;
		wxStaticText* m_SizeYLabel;
		wxStaticText* m_PositionYLabel;
		wxStaticText* m_staticText9;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_SizeYCtrl;
		wxTextCtrl* m_PositionYCtrl;
		wxChoice* m_StyleCtrl;
		wxChoice* m_justifyChoice;
		wxStaticText* m_ThicknessLabel;
		wxStaticText* m_orientationLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxTextCtrl* m_OrientationCtrl;
		wxStdDialogButtonSizer* m_StandardSizer;
		wxButton* m_StandardSizerOK;
		wxButton* m_StandardSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PCB_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 483,450 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_PCB_TEXT_PROPERTIES_BASE();
	
};

#endif //__DIALOG_PCB_TEXT_PROPERTIES_BASE_H__
