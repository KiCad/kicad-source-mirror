///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/valtext.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
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
		wxStaticText* m_txtBrdFile;
		wxTextCtrl* m_outputFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxRadioButton* m_rbDrillAndPlotOrigin;
		wxRadioButton* m_rbGridOrigin;
		wxRadioButton* m_rbUserDefinedOrigin;
		wxRadioButton* m_rbBoardCenterOrigin;
		wxStaticText* m_staticTextUnits;
		wxChoice* m_STEP_OrgUnitChoice;
		wxStaticText* m_staticTextXpos;
		TEXT_CTRL_EVAL* m_STEP_Xorg;
		wxStaticText* m_staticTextYpos;
		TEXT_CTRL_EVAL* m_STEP_Yorg;
		wxCheckBox* m_cbRemoveDNP;
		wxCheckBox* m_cbRemoveUnspecified;
		wxCheckBox* m_cbSubstModels;
		wxCheckBox* m_cbOverwriteFile;
		wxStaticLine* m_staticline11_hidden;
		wxCheckBox* m_cbOptimizeStep;
		wxCheckBox* m_cbExportCompound_hidden;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_cbExportBody;
		wxCheckBox* m_cbExportComponents;
		wxCheckBox* m_cbExportTracks;
		wxCheckBox* m_cbExportZones;
		wxCheckBox* m_cbExportInnerCopper;
		wxCheckBox* m_cbFuseShapes;
		wxCheckBox* m_cbExportSilkscreen_hidden;
		wxCheckBox* m_cbExportSoldermask_hidden;
		wxCheckBox* m_cbExportSolderpaste_hidden;
		wxStaticText* m_staticTextNetFilter;
		wxTextCtrl* m_txtNetFilter;
		wxStaticText* m_staticTextTolerance;
		wxChoice* m_choiceTolerance;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateUnits( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateXPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateYPos( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onExportButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export STEP / BREP / GLTF"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_STEP_BASE();

};

