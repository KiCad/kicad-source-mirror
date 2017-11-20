///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  4 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_COMPONENTS_LIBID_BASE_H__
#define __DIALOG_EDIT_COMPONENTS_LIBID_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_COMPONENTS_LIBID_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_COMPONENTS_LIBID_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxPanel* m_panelGrid;
		wxGrid* m_grid;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;
		wxButton* m_buttonUndo;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onCellBrowseLib( wxGridEvent& event ) { event.Skip(); }
		virtual void onApplyButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUndoChangesButton( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_COMPONENTS_LIBID_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Edit Components Links to Symbols in Libraries"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 797,311 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EDIT_COMPONENTS_LIBID_BASE();
	
};

#endif //__DIALOG_EDIT_COMPONENTS_LIBID_BASE_H__
