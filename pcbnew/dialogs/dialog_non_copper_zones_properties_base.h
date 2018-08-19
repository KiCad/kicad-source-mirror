///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__
#define __DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextLayerSelection;
		wxDataViewListCtrl* m_layers;
		wxCheckBox* m_ConstrainOpt;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineAppearanceCtrl;
		wxStaticText* m_MinWidthLabel;
		wxTextCtrl* m_MinWidthCtrl;
		wxStaticText* m_MinWidthUnits;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Non-copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxBORDER_SUNKEN ); 
		~DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE();
	
};

#endif //__DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__
