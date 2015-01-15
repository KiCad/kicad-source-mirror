///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CONFIG_EQUFILES_BASE_H__
#define __DIALOG_CONFIG_EQUFILES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/listbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CONFIG_EQUFILES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CONFIG_EQUFILES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_ADD_EQU = 1000,
			ID_REMOVE_EQU,
			ID_EQU_UP,
			ID_EQU_DOWN
		};
		
		wxListBox* m_ListEquiv;
		wxButton* m_buttonAddEqu;
		wxButton* m_buttonRemoveEqu;
		wxButton* m_buttonMoveUp;
		wxButton* m_buttonMoveDown;
		wxButton* m_buttonEdit;
		wxStaticText* m_staticText2;
		wxGrid* m_gridEnvVars;
		wxRadioBox* m_rbPathOptionChoice;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnAddFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditEquFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CONFIG_EQUFILES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 454,338 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CONFIG_EQUFILES_BASE();
	
};

#endif //__DIALOG_CONFIG_EQUFILES_BASE_H__
