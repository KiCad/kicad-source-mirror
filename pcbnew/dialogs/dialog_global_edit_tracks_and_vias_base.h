///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_H__
#define __DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_H__

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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CURRENT_VALUES_TO_CURRENT_NET 1000
#define ID_NETCLASS_VALUES_TO_CURRENT_NET 1001
#define ID_ALL_TRACKS_VIAS 1002
#define ID_ALL_VIAS 1003
#define ID_ALL_TRACKS 1004

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText12;
		wxStaticText* m_CurrentNetText;
		wxChoice* m_choiceNetName;
		wxStaticText* m_CurrentNetclassText;
		wxStaticText* m_CurrentNetclassName;
		wxGrid* m_gridDisplayCurrentSettings;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText11;
		wxRadioButton* m_Net2CurrValueButton;
		wxRadioButton* m_NetUseNetclassValueButton;
		wxRadioButton* m_radioBtnAll;
		wxRadioButton* m_radioAllVias;
		wxRadioButton* m_radioAllTracks;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onNetSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectionClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global Edition of Tracks and Vias"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 706,412 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE();
	
};

#endif //__DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_H__
