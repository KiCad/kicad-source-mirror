///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_BLOCK_OPTIONS_BASE_H__
#define __DIALOG_BLOCK_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BLOCK_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BLOCK_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_Include_Modules;
		wxCheckBox* m_Include_PcbTextes;
		wxCheckBox* m_IncludeLockedModules;
		wxCheckBox* m_Include_Draw_Items;
		wxCheckBox* m_Include_Tracks;
		wxCheckBox* m_Include_Edges_Items;
		wxCheckBox* m_Include_Zones;
		wxCheckBox* m_DrawBlockItems;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_checkBoxIncludeInvisible;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void checkBoxClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void ExecuteCommand( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_BLOCK_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,226 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_BLOCK_OPTIONS_BASE();
	
};

#endif //__DIALOG_BLOCK_OPTIONS_BASE_H__
