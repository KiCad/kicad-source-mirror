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
class TEXT_CTRL_EVAL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/valtext.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_STEP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_STEP_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerSTEPFile;
		wxBoxSizer* bSizerTop;
		wxStaticText* m_txtFormat;
		wxChoice* m_choiceFormat;
		wxStaticText* m_txtBrdFile;
		wxTextCtrl* m_outputFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxCheckBox* m_cbExportCompound_hidden;
		wxCheckBox* m_cbExportBody;
		wxCheckBox* m_cbCutViasInBody;
		wxCheckBox* m_cbExportSilkscreen;
		wxCheckBox* m_cbExportSoldermask;
		wxCheckBox* m_cbExportSolderpaste_hidden;
		wxCheckBox* m_cbExportComponents;
		wxRadioButton* m_rbAllComponents;
		wxRadioButton* m_rbOnlySelected;
		wxRadioButton* m_rbFilteredComponents;
		wxTextCtrl* m_txtComponentFilter;
		wxCheckBox* m_cbExportTracks;
		wxCheckBox* m_cbExportPads;
		wxCheckBox* m_cbExportZones;
		wxCheckBox* m_cbExportInnerCopper;
		wxCheckBox* m_cbFuseShapes;
		wxCheckBox* m_cbFillAllVias;
		wxStaticText* m_staticTextNetFilter;
		wxTextCtrl* m_txtNetFilter;
		wxRadioButton* m_rbDrillAndPlotOrigin;
		wxRadioButton* m_rbGridOrigin;
		wxRadioButton* m_rbUserDefinedOrigin;
		wxRadioButton* m_rbBoardCenterOrigin;
		wxStaticText* m_originXLabel;
		TEXT_CTRL_EVAL* m_originXCtrl;
		wxStaticText* m_originXUnits;
		wxStaticText* m_originYLabel;
		TEXT_CTRL_EVAL* m_originYCtrl;
		wxStaticText* m_originYUnits;
		wxCheckBox* m_cbRemoveDNP;
		wxCheckBox* m_cbRemoveUnspecified;
		wxCheckBox* m_cbSubstModels;
		wxCheckBox* m_cbOverwriteFile;
		wxCheckBox* m_cbOptimizeStep;
		wxStaticText* m_staticTextTolerance;
		wxChoice* m_choiceTolerance;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onFormatChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCbExportComponents( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComponentModeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateXPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateYPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onExportButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export 3D Model"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_STEP_BASE();

};

