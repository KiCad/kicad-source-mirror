///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;
class UP_DOWN_TREE;

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <widgets/split_button.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_TOOLBAR_CUSTOMIZATION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_TOOLBAR_CUSTOMIZATION_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_customToolbars;
		wxStaticLine* m_staticline1;
		wxStaticText* m_toolbarChoiceLabel;
		wxChoice* m_tbChoice;
		UP_DOWN_TREE* m_toolbarTree;
		SPLIT_BUTTON* m_insertButton;
		STD_BITMAP_BUTTON* m_btnToolMoveUp;
		STD_BITMAP_BUTTON* m_btnToolMoveDown;
		STD_BITMAP_BUTTON* m_btnToolDelete;
		STD_BITMAP_BUTTON* m_btnAddTool;
		wxListCtrl* m_actionsList;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onCustomizeTbCb( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTbChoiceSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTreeBeginLabelEdit( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeEndLabelEdit( wxTreeEvent& event ) { event.Skip(); }
		virtual void onToolMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void onToolMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void onToolDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnAddAction( wxCommandEvent& event ) { event.Skip(); }
		virtual void onListItemActivated( wxListEvent& event ) { event.Skip(); }


	public:

		PANEL_TOOLBAR_CUSTOMIZATION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_TOOLBAR_CUSTOMIZATION_BASE();

};

