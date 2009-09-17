///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_copper_layers_setup_base__
#define __dialog_copper_layers_setup_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/html/htmlwin.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_LAYERS_COUNT_SELECTION 1000
#define ID_LAYERS_PROPERTIES 1001

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_LAYERS_SETUP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_LAYERS_SETUP_BASE : public wxDialog 
{
	private:
	
	protected:
		wxRadioBox* m_LayersCountSelection;
		wxGrid* m_gridLayersProperties;
		wxHtmlWindow* m_MessagesList;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnLayerCountClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLayerGridLeftClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnLayerGridRighttClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_COPPER_LAYERS_SETUP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Copper layers setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 558,479 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_COPPER_LAYERS_SETUP_BASE();
	
};

#endif //__dialog_copper_layers_setup_base__
