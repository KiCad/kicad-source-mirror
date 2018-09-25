///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SWAP_LAYERS_BASE_H__
#define __DIALOG_SWAP_LAYERS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SWAP_LAYERS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SWAP_LAYERS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		WX_GRID* m_grid;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SWAP_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Swap Layers"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SWAP_LAYERS_BASE();
	
};

#endif //__DIALOG_SWAP_LAYERS_BASE_H__
