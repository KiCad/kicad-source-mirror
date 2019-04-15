///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  1 2018)
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
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SELECT_NET_FROM_LIST_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SELECT_NET_FROM_LIST_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFilter;
		wxTextCtrl* m_textCtrlFilter;
		wxCheckBox* m_cbShowZeroPad;
		wxDataViewListCtrl* m_netsList;
		wxButton* m_ReportButt;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onFilterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSelChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onListSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void onReport( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SELECT_NET_FROM_LIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Nets"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 584,397 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SELECT_NET_FROM_LIST_BASE();

};

