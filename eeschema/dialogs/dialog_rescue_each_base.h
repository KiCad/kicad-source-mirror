///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_RESCUE_EACH_BASE_H__
#define __DIALOG_RESCUE_EACH_BASE_H__

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
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RESCUE_EACH_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RESCUE_EACH_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_lblInfo;
		wxStaticText* m_staticText5;
		wxDataViewListCtrl* m_ListOfConflicts;
		wxStaticText* m_staticText4;
		wxDataViewListCtrl* m_ListOfInstances;
		wxStaticText* m_staticText2;
		wxPanel* m_componentViewOld;
		wxStaticText* m_staticText3;
		wxPanel* m_componentViewNew;
		wxButton* m_btnNeverShowAgain;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnDialogResize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnConflictSelect( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnHandleCachePreviewRepaint( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnHandleLibraryPreviewRepaint( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnNeverShowClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_RESCUE_EACH_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Conflicts Checking"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 529,593 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_RESCUE_EACH_BASE();
	
};

#endif //__DIALOG_RESCUE_EACH_BASE_H__
