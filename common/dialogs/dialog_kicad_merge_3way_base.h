///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

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
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/splitter.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_KICAD_MERGE_3WAY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_KICAD_MERGE_3WAY_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_labelIntro;
		wxSplitterWindow* m_splitter;
		wxPanel* m_panelConflicts;
		wxStaticText* m_labelConflicts;
		wxListBox* m_listConflicts;
		wxPanel* m_panelResolution;
		wxStaticText* m_labelDetail;
		wxTextCtrl* m_textDetail;
		wxRadioButton* m_radioOurs;
		wxRadioButton* m_radioTheirs;
		wxRadioButton* m_radioAncestor;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnConflictSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResolutionChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		DIALOG_KICAD_MERGE_3WAY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Resolve Merge Conflicts"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 950,700 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_KICAD_MERGE_3WAY_BASE();

		void m_splitterOnIdle( wxIdleEvent& )
		{
			m_splitter->SetSashPosition( 300 );
			m_splitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::m_splitterOnIdle ), NULL, this );
		}

};

