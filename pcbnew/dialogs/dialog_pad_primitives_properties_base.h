///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class STD_BITMAP_BUTTON;
class TEXT_CTRL_EVAL;
class WX_GRID;

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
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>
#include <wx/statbmp.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPosStart;
		wxStaticText* m_startXLabel;
		TEXT_CTRL_EVAL* m_startXCtrl;
		wxStaticText* m_startXUnits;
		wxStaticText* m_startYLabel;
		TEXT_CTRL_EVAL* m_startYCtrl;
		wxStaticText* m_startYUnits;
		wxStaticText* m_staticTextPosCtrl1;
		wxStaticText* m_ctrl1XLabel;
		TEXT_CTRL_EVAL* m_ctrl1XCtrl;
		wxStaticText* m_ctrl1XUnits;
		wxStaticText* m_ctrl1YLabel;
		TEXT_CTRL_EVAL* m_ctrl1YCtrl;
		wxStaticText* m_ctrl1YUnits;
		wxStaticText* m_staticTextPosCtrl2;
		wxStaticText* m_ctrl2XLabel;
		TEXT_CTRL_EVAL* m_ctrl2XCtrl;
		wxStaticText* m_ctrl2XUnits;
		wxStaticText* m_ctrl2YLabel;
		TEXT_CTRL_EVAL* m_ctrl2YCtrl;
		wxStaticText* m_ctrl2YUnits;
		wxStaticText* m_staticTextPosEnd;
		wxStaticText* m_endXLabel;
		TEXT_CTRL_EVAL* m_endXCtrl;
		wxStaticText* m_endXUnits;
		wxStaticText* m_endYLabel;
		TEXT_CTRL_EVAL* m_endYCtrl;
		wxStaticText* m_endYUnits;
		wxStaticText* m_radiusLabel;
		TEXT_CTRL_EVAL* m_radiusCtrl;
		wxStaticText* m_radiusUnits;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxCheckBox* m_filledCtrl;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextMove;
		wxStaticText* m_xLabel;
		TEXT_CTRL_EVAL* m_xCtrl;
		wxStaticText* m_xUnits;
		wxStaticText* m_yLabel;
		TEXT_CTRL_EVAL* m_yCtrl;
		wxStaticText* m_yUnits;
		wxStaticText* m_rotationLabel;
		TEXT_CTRL_EVAL* m_rotationCtrl;
		wxStaticText* m_rotationUnits;
		wxStaticText* m_scaleLabel;
		TEXT_CTRL_EVAL* m_scaleCtrl;
		wxStaticText* m_staticTextDupCnt;
		wxSpinCtrl* m_spinCtrlDuplicateCount;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad Custom Shape Geometry Transform"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_GRID* m_gridCornersList;
		STD_BITMAP_BUTTON* m_addButton;
		STD_BITMAP_BUTTON* m_deleteButton;
		wxStaticText* m_thicknessLabel;
		TEXT_CTRL_EVAL* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxCheckBox* m_filledCtrl;
		wxPanel* m_panelPoly;
		wxStaticBitmap* m_warningIcon;
		wxStaticText* m_warningText;
		wxStaticText* m_statusLine1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onGridSelect( wxGridRangeSelectEvent& event ) { event.Skip(); }
		virtual void onCellSelect( wxGridEvent& event ) { event.Skip(); }
		virtual void OnButtonAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPaintPolyPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void onPolyPanelResize( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Basic Shape Polygon"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE();

};

