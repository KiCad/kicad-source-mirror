///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 29 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_layers_setup_base2__
#define __dialog_layers_setup_base2__

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/scrolwin.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LAYERS_SETUP_BASE2
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LAYERS_SETUP_BASE2 : public wxDialog 
{
	private:
	
	protected:
		wxChoice* m_PresetsChoice;
		wxChoice* m_CopperLayersChoice;
		wxStaticText* m_LayerNameCaption;
		wxStaticText* m_LayerEnabledCaption;
		wxStaticText* m_LayerTypeCaption;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayerListFlexGridSizer;
		wxStaticText* m_junkStaticText;
		wxCheckBox* m_junkCheckBox;
		wxChoice* m_junkChoice;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
	
	public:
		DIALOG_LAYERS_SETUP_BASE2( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Layer Setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 700,600 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LAYERS_SETUP_BASE2();
	
};

#endif //__dialog_layers_setup_base2__
