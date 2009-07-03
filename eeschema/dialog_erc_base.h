///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_erc_base__
#define __dialog_erc_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ERC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ERC_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnErcCmpClick( wxCommandEvent& event ){ OnErcCmpClick( event ); }
		void _wxFB_OnEraseDrcMarkersClick( wxCommandEvent& event ){ OnEraseDrcMarkersClick( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnResetMatrixClick( wxCommandEvent& event ){ OnResetMatrixClick( event ); }
		
	
	protected:
		enum
		{
			ID_ERC_CMP = 1000,
			ID_ERASE_DRC_MARKERS,
			ID_RESET_MATRIX,
		};
		
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelERC;
		wxStaticText* m_ErcTotalErrorsText;
		wxStaticText* m_TotalErrCount;
		wxStaticText* m_WarnErcErrorsText;
		wxStaticText* m_LastWarningCount;
		wxStaticText* m_LastErrCountText;
		wxStaticText* m_LastErrCount;
		
		wxButton* m_buttonERC;
		wxButton* m_buttondelmarkers;
		wxButton* m_buttonClose;
		wxCheckBox* m_WriteResultOpt;
		wxStaticLine* m_staticline2;
		wxStaticText* m_textMessage;
		wxTextCtrl* m_MessagesList;
		wxPanel* m_PanelERCOptions;
		wxBoxSizer* m_PanelMatrixSizer;
		wxButton* m_ResetOptButton;
		wxStaticLine* m_staticline1;
		wxBoxSizer* m_MatrixSizer;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnErcCmpClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEraseDrcMarkersClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnResetMatrixClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("EESchema Erc"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 438,407 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_ERC_BASE();
	
};

#endif //__dialog_erc_base__
