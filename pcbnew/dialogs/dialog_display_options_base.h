///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  9 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_DISPLAY_OPTIONS_BASE_H__
#define __DIALOG_DISPLAY_OPTIONS_BASE_H__

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
#include <wx/choice.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinbutt.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DISPLAY_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_SHOW_CLEARANCE = 1000
		};
		
		wxCheckBox* m_OptDisplayTracks;
		wxCheckBox* m_OptDisplayVias;
		wxChoice* m_choiceAntialiasing;
		wxRadioBox* m_gridStyle;
		wxStaticText* l_gridLineWidth;
		wxTextCtrl* m_gridLineWidth;
		wxSpinButton* m_gridLineWidthSpinBtn;
		wxStaticText* l_gridLineWidthUnits;
		wxStaticText* l_gridMinSpacing;
		wxTextCtrl* m_gridMinSpacing;
		wxSpinButton* m_gridMinSpacingSpinBtn;
		wxStaticText* l_gridMinSpacingUnits;
		wxRadioBox* m_ShowNetNamesOption;
		wxRadioBox* m_OptDisplayTracksClearance;
		wxCheckBox* m_OptDisplayModOutlines;
		wxCheckBox* m_OptDisplayModTexts
		;
		wxCheckBox* m_OptDisplayPads;
		wxCheckBox* m_OptDisplayPadClearence;
		wxCheckBox* m_OptDisplayPadNumber;
		wxCheckBox* m_OptDisplayPadNoConn;
		wxCheckBox* m_OptDisplayDrawings;
		wxCheckBox* m_Show_Page_Limits;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_DISPLAY_OPTIONS_BASE();
	
};

#endif //__DIALOG_DISPLAY_OPTIONS_BASE_H__
