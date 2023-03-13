///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NET_INSPECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NET_INSPECTOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFilter;
		wxTextCtrl* m_textCtrlFilter;
		wxCheckBox* m_cbShowZeroPad;
		wxCheckBox* m_groupBy;
		wxTextCtrl* m_groupByText;
		wxComboBox* m_groupByKind;
		wxDataViewCtrl* m_netsList;
		STD_BITMAP_BUTTON* m_addNet;
		STD_BITMAP_BUTTON* m_renameNet;
		STD_BITMAP_BUTTON* m_deleteNet;
		wxButton* m_ReportButt;

		// Virtual event handlers, override them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void onFilterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSortingChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onSelChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onListSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void onAddNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRenameNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onReport( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_NET_INSPECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Net Inspector"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_NET_INSPECTOR_BASE();

};

