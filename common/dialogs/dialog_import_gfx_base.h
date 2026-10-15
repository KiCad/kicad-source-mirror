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
#include <wx/radiobut.h>
#include <wx/valtext.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/statbmp.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_GFX_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_GFX_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFile;
		wxTextCtrl* m_textCtrlFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxRadioButton* m_rbInteractivePlacement;
		wxRadioButton* m_rbAbsolutePlacement;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xCtrl;
		wxStaticText* m_xUnits;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yCtrl;
		wxStaticText* m_yUnits;
		wxStaticText* m_importScaleLabel;
		wxTextCtrl* m_importScaleCtrl;
		wxStaticText* m_importWidthLabel;
		wxTextCtrl* m_importWidthCtrl;
		wxStaticText* m_importWidthUnits;
		wxStaticText* m_importHeightLabel;
		wxTextCtrl* m_importHeightCtrl;
		wxStaticText* m_importHeightUnits;
		wxCheckBox* m_aspectRatioCheckbox;
		wxCheckBox* m_cbGroupItems;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_staticTextLineWidth1;
		wxChoice* m_choiceDxfUnits;
		wxPanel* m_imagePanel;
		wxStaticText* m_thresholdLabel;
		wxSlider* m_thresholdSlider;
		wxCheckBox* m_cbInvertColors;
		wxStaticBitmap* m_previewBitmap;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onFilenameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void onInteractivePlacement( wxCommandEvent& event ) { event.Skip(); }
		virtual void originOptionOnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onAbsolutePlacement( wxCommandEvent& event ) { event.Skip(); }
		virtual void onScaleChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onHeightChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLockAspectChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onThresholdChanged( wxScrollEvent& event ) { event.Skip(); }
		virtual void onInvertChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Graphics File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_GFX_BASE();

};

