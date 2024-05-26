///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class FONT_CHOICE;
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TABLECELL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TABLECELL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		BITMAP_BUTTON* m_hAlignLeft;
		BITMAP_BUTTON* m_hAlignCenter;
		BITMAP_BUTTON* m_hAlignRight;
		wxStaticText* vAlignLabel;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignCenter;
		BITMAP_BUTTON* m_vAlignBottom;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		wxStaticText* m_styleLabel;
		wxCheckBox* m_bold;
		wxCheckBox* m_italic;
		wxStaticText* m_SizeXLabel;
		wxTextCtrl* m_SizeXCtrl;
		wxStaticText* m_SizeXUnits;
		wxStaticText* m_SizeYLabel;
		wxTextCtrl* m_SizeYCtrl;
		wxStaticText* m_SizeYUnits;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_ThicknessUnits;
		wxTextCtrl* m_marginTopCtrl;
		wxStaticText* m_marginTopUnits;
		wxTextCtrl* m_marginLeftCtrl;
		wxTextCtrl* m_marginRightCtrl;
		wxTextCtrl* m_marginBottomCtrl;
		wxStaticLine* m_staticline1;
		wxButton* m_editTable;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onThickness( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditTable( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Table Cell Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_TABLECELL_PROPERTIES_BASE();

};

