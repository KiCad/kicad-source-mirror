///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GLOBAL_DELETION_BASE_H__
#define __DIALOG_GLOBAL_DELETION_BASE_H__

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
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_DELETION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_DELETION_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_DelZones;
		wxCheckBox* m_DelTexts;
		wxCheckBox* m_DelBoardEdges;
		wxCheckBox* m_DelDrawings;
		wxCheckBox* m_DelModules;
		wxCheckBox* m_DelTracks;
		wxCheckBox* m_DelMarkers;
		wxCheckBox* m_DelAlls;
		wxStaticBoxSizer* sbFilter;
		wxCheckBox* m_TrackFilterAR;
		wxCheckBox* m_TrackFilterLocked;
		wxCheckBox* m_TrackFilterNormal;
		wxCheckBox* m_TrackFilterVias;
		wxCheckBox* m_ModuleFilterLocked;
		wxCheckBox* m_ModuleFilterNormal;
		wxRadioBox* m_rbLayersOption;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textCtrlCurrLayer;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckDeleteModules( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckDeleteTracks( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GLOBAL_DELETION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Delete Items"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 339,365 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GLOBAL_DELETION_BASE();
	
};

#endif //__DIALOG_GLOBAL_DELETION_BASE_H__
