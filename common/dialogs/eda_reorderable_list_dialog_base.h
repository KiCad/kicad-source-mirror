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
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class EDA_REORDERABLE_LIST_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class EDA_REORDERABLE_LIST_DIALOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_availableListLabel;
		wxStaticText* m_enabledListLabel;
		wxListCtrl* m_availableListBox;
		wxButton* m_btnAdd;
		wxButton* m_btnRemove;
		wxListCtrl* m_enabledListBox;
		STD_BITMAP_BUTTON* m_btnUp;
		STD_BITMAP_BUTTON* m_btnDown;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onAvailableListItemSelected( wxListEvent& event ) = 0;
		virtual void onAddItem( wxCommandEvent& event ) = 0;
		virtual void onRemoveItem( wxCommandEvent& event ) = 0;
		virtual void onEnabledListItemSelected( wxListEvent& event ) = 0;
		virtual void onMoveUp( wxCommandEvent& event ) = 0;
		virtual void onMoveDown( wxCommandEvent& event ) = 0;


	public:
		wxBoxSizer* m_ButtonsSizer;

		EDA_REORDERABLE_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 580,260 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~EDA_REORDERABLE_LIST_DIALOG_BASE();

};

