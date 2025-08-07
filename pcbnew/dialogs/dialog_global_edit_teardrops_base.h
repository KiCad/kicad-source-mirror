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

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <widgets/net_selector.h>
#include <wx/choice.h>
#include <wx/bmpcbox.h>
#include <wx/radiobut.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TEARDROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TEARDROPS_BASE : public DIALOG_SHIM
{
	private:
		wxStaticText* m_lengthUnitsPrefix;
		wxStaticText* m_lengthUnitsHint;
		wxStaticText* m_lengthUnitsSuffix;
		wxStaticText* m_widthUnitsPrefix;
		wxStaticText* m_widthUnitsHint;
		wxStaticText* m_widthUnitsSuffix;

	protected:
		wxCheckBox* m_pthPads;
		wxCheckBox* m_smdPads;
		wxCheckBox* m_vias;
		wxCheckBox* m_trackToTrack;
		wxCheckBox* m_netFilterOpt;
		NET_SELECTOR* m_netFilter;
		wxCheckBox* m_netclassFilterOpt;
		wxChoice* m_netclassFilter;
		wxCheckBox* m_layerFilterOpt;
		PCB_LAYER_BOX_SELECTOR* m_layerFilter;
		wxCheckBox* m_roundPadsFilter;
		wxCheckBox* m_existingFilter;
		wxCheckBox* m_selectedItemsFilter;
		wxRadioButton* m_removeTeardrops;
		wxRadioButton* m_removeAllTeardrops;
		wxRadioButton* m_addTeardrops;
		wxHyperlinkCtrl* m_boardSetupLink;
		wxRadioButton* m_specifiedValues;
		wxCheckBox* m_cbPreferZoneConnection;
		wxCheckBox* m_cbTeardropsUseNextTrack;
		wxStaticText* m_stHDRatio;
		wxTextCtrl* m_tcHDRatio;
		wxStaticText* m_stHDRatioUnits;
		wxStaticText* m_minTrackWidthHint;
		wxStaticBitmap* m_bitmapTeardrop;
		wxStaticText* m_stLenPercentLabel;
		wxTextCtrl* m_tcLenPercent;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stMaxLenUnits;
		wxStaticText* m_stHeightPercentLabel;
		wxTextCtrl* m_tcHeightPercent;
		wxStaticText* m_stMaxHeight;
		wxTextCtrl* m_tcMaxHeight;
		wxStaticText* m_stMaxHeightUnits;
		wxCheckBox* m_curvedEdges;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onTrackToTrack( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFilterUpdateUi( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnNetclassFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLayerFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExistingFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onShowBoardSetup( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void onSpecifiedValuesUpdateUi( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_GLOBAL_EDIT_TEARDROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Set Teardrops"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GLOBAL_EDIT_TEARDROPS_BASE();

};

