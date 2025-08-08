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
class PCB_LAYER_BOX_SELECTOR;
class STD_BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/valtext.h>
#include <wx/bmpcbox.h>
#include <wx/gbsizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_GRAPHICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFile;
		wxTextCtrl* m_textCtrlFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_importScaleLabel;
		wxTextCtrl* m_importScaleCtrl;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_dxfUnitsLabel;
		wxChoice* m_dxfUnitsChoice;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_placeAtCheckbox;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xCtrl;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yCtrl;
		wxStaticText* m_yUnits;
		wxCheckBox* m_setLayerCheckbox;
		PCB_LAYER_BOX_SELECTOR* m_SelLayerBox;
		wxCheckBox* m_cbGroupItems;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_rbFixDiscontinuities;
		wxStaticText* m_toleranceLabel;
		wxTextCtrl* m_toleranceCtrl;
		wxStaticText* m_toleranceUnits;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onBrowseFiles( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Vector Graphics File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_GRAPHICS_BASE();

};

