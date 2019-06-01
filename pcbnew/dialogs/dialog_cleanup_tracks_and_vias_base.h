///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 23 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DRCLISTBOX;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CLEANUP_ITEMS_LIST 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_cleanShortCircuitOpt;
		wxCheckBox* m_cleanViasOpt;
		wxCheckBox* m_mergeSegmOpt;
		wxCheckBox* m_deleteUnconnectedOpt;
		wxCheckBox* m_deleteTracksInPadsOpt;
		wxStaticText* staticChangesLabel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftDClickItem( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSelectItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRightUpItem( wxMouseEvent& event ) { event.Skip(); }


	public:
		DRCLISTBOX* m_ItemsListBox;

		DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleanup Tracks and Vias"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE();

};

