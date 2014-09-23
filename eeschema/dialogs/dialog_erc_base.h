///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_ERC_BASE_H__
#define __DIALOG_ERC_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class ERC_HTML_LISTBOX;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/listbox.h>
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
class DIALOG_ERC_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnCloseErcDialog( wxCloseEvent& event ){ OnCloseErcDialog( event ); }
		void _wxFB_OnErcCmpClick( wxCommandEvent& event ){ OnErcCmpClick( event ); }
		void _wxFB_OnEraseDrcMarkersClick( wxCommandEvent& event ){ OnEraseDrcMarkersClick( event ); }
		void _wxFB_OnButtonCloseClick( wxCommandEvent& event ){ OnButtonCloseClick( event ); }
		void _wxFB_OnLeftClickMarkersList( wxCommandEvent& event ){ OnLeftClickMarkersList( event ); }
		void _wxFB_OnLeftDblClickMarkersList( wxCommandEvent& event ){ OnLeftDblClickMarkersList( event ); }
		void _wxFB_OnResetMatrixClick( wxCommandEvent& event ){ OnResetMatrixClick( event ); }
		
	
	protected:
		enum
		{
			ID_ERC_CMP = 1000,
			ID_ERASE_DRC_MARKERS,
			ID_MAKER_HTMLLISTBOX,
			ID_RESET_MATRIX
		};
		
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelERC;
		wxStaticText* m_ErcTotalErrorsText;
		wxTextCtrl* m_TotalErrCount;
		wxStaticText* m_WarnErcErrorsText;
		wxTextCtrl* m_LastWarningCount;
		wxStaticText* m_LastErrCountText;
		wxTextCtrl* m_LastErrCount;
		wxCheckBox* m_WriteResultOpt;
		wxStaticText* m_titleMessages;
		wxTextCtrl* m_MessagesList;
		wxButton* m_buttonERC;
		wxButton* m_buttondelmarkers;
		wxButton* m_buttonClose;
		wxStaticText* m_textMarkers;
		ERC_HTML_LISTBOX* m_MarkersList;
		wxPanel* m_PanelERCOptions;
		wxButton* m_ResetOptButton;
		wxPanel* m_matrixPanel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseErcDialog( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnErcCmpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEraseDrcMarkersClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCloseClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftClickMarkersList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftDblClickMarkersList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResetMatrixClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("EESchema Erc"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 519,392 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_ERC_BASE();
	
};

#endif //__DIALOG_ERC_BASE_H__
