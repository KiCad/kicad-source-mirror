///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"
#include "widgets/wx_grid.h"

#include "dialog_pad_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAD_PROPERTIES_BASE::DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_notebook->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	m_panelGeneral = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGeneralSizer;
	bGeneralSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerShapeType;
	fgSizerShapeType = new wxFlexGridSizer( 0, 3, 2, 0 );
	fgSizerShapeType->AddGrowableCol( 1 );
	fgSizerShapeType->SetFlexibleDirection( wxBOTH );
	fgSizerShapeType->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_PadNumText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNumText->Wrap( -1 );
	fgSizerShapeType->Add( m_PadNumText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );
	
	m_PadNumCtrl = new wxTextCtrl( m_panelGeneral, wxID_PADNUMCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_PadNumCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_PadNameText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Net name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNameText->Wrap( -1 );
	fgSizerShapeType->Add( m_PadNameText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3 );
	
	m_PadNetSelector = new NET_SELECTOR( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 ); 
	fgSizerShapeType->Add( m_PadNetSelector, 0, wxTOP|wxLEFT|wxEXPAND, 3 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText44 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText44, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	wxString m_PadTypeChoices[] = { _("Through-hole"), _("SMD"), _("Connector"), _("NPTH, Mechanical"), _("Aperture") };
	int m_PadTypeNChoices = sizeof( m_PadTypeChoices ) / sizeof( wxString );
	m_PadType = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadTypeNChoices, m_PadTypeChoices, 0 );
	m_PadType->SetSelection( 0 );
	fgSizerShapeType->Add( m_PadType, 0, wxEXPAND|wxALL, 3 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText45 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText45, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	wxString m_PadShapeChoices[] = { _("Circular"), _("Oval"), _("Rectangular"), _("Trapezoidal"), _("Rounded Rectangle"), _("Chamfered Rectangle"), _("Custom (Circ. Anchor)"), _("Custom (Rect. Anchor)") };
	int m_PadShapeNChoices = sizeof( m_PadShapeChoices ) / sizeof( wxString );
	m_PadShape = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadShapeNChoices, m_PadShapeChoices, 0 );
	m_PadShape->SetSelection( 0 );
	fgSizerShapeType->Add( m_PadShape, 0, wxEXPAND|wxALL, 3 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_posXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_posXLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 3 );
	
	m_posXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_posXCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_posXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_posXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_posYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_posYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );
	
	m_posYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_posYCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_posYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_posYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_sizeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_sizeXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );
	
	m_sizeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_sizeXCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_sizeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_sizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_sizeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_sizeYLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	m_sizeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_sizeYCtrl, 0, wxEXPAND|wxALL, 3 );
	
	m_sizeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_sizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	m_PadOrientText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadOrientText->Wrap( -1 );
	fgSizerShapeType->Add( m_PadOrientText, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	m_orientation = new wxComboBox( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_orientation->Append( _("0") );
	m_orientation->Append( _("90") );
	m_orientation->Append( _("-90") );
	m_orientation->Append( _("180") );
	fgSizerShapeType->Add( m_orientation, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText491 = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText491->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText491, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	m_offsetXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_offsetXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );
	
	m_offsetXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_offsetXCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_offsetXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_offsetXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_offsetYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_offsetYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );
	
	m_offsetYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_offsetYCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_offsetYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_offsetYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_padToDieLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad to die length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieLabel->Wrap( -1 );
	m_padToDieLabel->SetToolTip( _("Wire length from pad to die on chip ( used to calculate actual track length)") );
	
	fgSizerShapeType->Add( m_padToDieLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 3 );
	
	m_padToDieCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_padToDieCtrl, 0, wxEXPAND|wxALL, 3 );
	
	m_padToDieUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_padToDieUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	m_trapDeltaLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trapezoid delta:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapDeltaLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_trapDeltaLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	m_trapDeltaCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_trapDeltaCtrl, 0, wxEXPAND|wxALL, 3 );
	
	m_trapDeltaUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapDeltaUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_trapDeltaUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	m_trapAxisLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trapezoid axis:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapAxisLabel->Wrap( -1 );
	fgSizerShapeType->Add( m_trapAxisLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );
	
	wxString m_trapAxisCtrlChoices[] = { _("Horizontal"), _("Vertical") };
	int m_trapAxisCtrlNChoices = sizeof( m_trapAxisCtrlChoices ) / sizeof( wxString );
	m_trapAxisCtrl = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trapAxisCtrlNChoices, m_trapAxisCtrlChoices, 0 );
	m_trapAxisCtrl->SetSelection( 0 );
	fgSizerShapeType->Add( m_trapAxisCtrl, 0, wxEXPAND|wxALL, 3 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND|wxTOP, 15 );
	
	m_staticTextCornerSizeRatio = new wxStaticText( m_panelGeneral, wxID_ANY, _("Corner size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerSizeRatio->Wrap( -1 );
	m_staticTextCornerSizeRatio->SetToolTip( _("Corner radius in percent  of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );
	
	fgSizerShapeType->Add( m_staticTextCornerSizeRatio, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_tcCornerSizeRatio = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_tcCornerSizeRatio, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextCornerSizeRatioUnit = new wxStaticText( m_panelGeneral, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerSizeRatioUnit->Wrap( -1 );
	fgSizerShapeType->Add( m_staticTextCornerSizeRatioUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	m_cornerRadiusLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Corner radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	m_cornerRadiusLabel->SetToolTip( _("Corner radius.\nCan be no more than half pad width.\nThe width is the smaller value between size X and size Y.\nNote: IPC norm gives a max value = 0.25mm.") );
	
	fgSizerShapeType->Add( m_cornerRadiusLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );
	
	m_tcCornerRadius = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_tcCornerRadius, 0, wxALL|wxEXPAND, 3 );
	
	m_cornerRadiusUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_cornerRadiusUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	m_staticTextChamferRatio = new wxStaticText( m_panelGeneral, wxID_ANY, _("Chamfer size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferRatio->Wrap( -1 );
	m_staticTextChamferRatio->SetToolTip( _("Chamfer size in percent of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );
	
	fgSizerShapeType->Add( m_staticTextChamferRatio, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_tcChamferRatio = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_tcChamferRatio, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_staticTextChamferRatioUnit = new wxStaticText( m_panelGeneral, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferRatioUnit->Wrap( -1 );
	fgSizerShapeType->Add( m_staticTextChamferRatioUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextChamferCorner = new wxStaticText( m_panelGeneral, wxID_ANY, _("Chamfered corner:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferCorner->Wrap( -1 );
	m_staticTextChamferCorner->SetToolTip( _("Chamfered corners. The position is relative to a pad orientation 0 degree.") );
	
	fgSizerShapeType->Add( m_staticTextChamferCorner, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerChamferedCorners;
	bSizerChamferedCorners = new wxBoxSizer( wxVERTICAL );
	
	m_cbTopLeft = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Top left"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTopLeft->SetValue(true); 
	bSizerChamferedCorners->Add( m_cbTopLeft, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_cbTopRight = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Top right"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChamferedCorners->Add( m_cbTopRight, 0, wxRIGHT|wxLEFT, 5 );
	
	m_cbBottomLeft = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Bottom left"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChamferedCorners->Add( m_cbBottomLeft, 0, wxRIGHT|wxLEFT, 5 );
	
	m_cbBottomRight = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Bottom right"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChamferedCorners->Add( m_cbBottomRight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizerShapeType->Add( bSizerChamferedCorners, 0, wxEXPAND, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizerShapeType, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bGeneralSizer->Add( m_LeftBoxSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerGeometry;
	fgSizerGeometry = new wxFlexGridSizer( 14, 3, 0, 0 );
	fgSizerGeometry->AddGrowableCol( 1 );
	fgSizerGeometry->SetFlexibleDirection( wxBOTH );
	fgSizerGeometry->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_holeShapeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeShapeLabel->Wrap( -1 );
	fgSizerGeometry->Add( m_holeShapeLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );
	
	wxString m_holeShapeCtrlChoices[] = { _("Circular"), _("Oval") };
	int m_holeShapeCtrlNChoices = sizeof( m_holeShapeCtrlChoices ) / sizeof( wxString );
	m_holeShapeCtrl = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_holeShapeCtrlNChoices, m_holeShapeCtrlChoices, 0 );
	m_holeShapeCtrl->SetSelection( 0 );
	fgSizerGeometry->Add( m_holeShapeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_staticText51 = new wxStaticText( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizerGeometry->Add( m_staticText51, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_holeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXLabel->Wrap( -1 );
	fgSizerGeometry->Add( m_holeXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_holeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGeometry->Add( m_holeXCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_holeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXUnits->Wrap( -1 );
	fgSizerGeometry->Add( m_holeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_holeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYLabel->Wrap( -1 );
	fgSizerGeometry->Add( m_holeYLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_holeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGeometry->Add( m_holeYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_holeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYUnits->Wrap( -1 );
	fgSizerGeometry->Add( m_holeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bSizer10->Add( fgSizerGeometry, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_LayersSizer;
	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelGeneral, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_FlippedWarningSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_FlippedWarningIcon = new wxStaticBitmap( m_LayersSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_FlippedWarningIcon->SetMinSize( wxSize( 48,48 ) );
	
	m_FlippedWarningSizer->Add( m_FlippedWarningIcon, 0, wxALIGN_TOP|wxBOTTOM|wxTOP, 4 );
	
	m_staticText86 = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Parent footprint on board is flipped.\nLayers will be reversed."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText86->Wrap( 150 );
	m_FlippedWarningSizer->Add( m_staticText86, 1, wxALIGN_TOP|wxBOTTOM|wxLEFT|wxRIGHT, 8 );
	
	
	m_LayersSizer->Add( m_FlippedWarningSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText511 = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Copper:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText511->Wrap( -1 );
	bSizer11->Add( m_staticText511, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxTOP, 4 );
	
	wxString m_rbCopperLayersSelChoices[] = { _("Front layer"), _("Back layer"), _("All copper layers"), _("None") };
	int m_rbCopperLayersSelNChoices = sizeof( m_rbCopperLayersSelChoices ) / sizeof( wxString );
	m_rbCopperLayersSel = new wxChoice( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rbCopperLayersSelNChoices, m_rbCopperLayersSelChoices, 0 );
	m_rbCopperLayersSel->SetSelection( 0 );
	bSizer11->Add( m_rbCopperLayersSel, 1, wxALL|wxEXPAND|wxTOP, 4 );
	
	
	m_LayersSizer->Add( bSizer11, 0, wxEXPAND, 5 );
	
	m_techLayersLabel = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Technical layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	m_techLayersLabel->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	m_LayersSizer->Add( m_techLayersLabel, 0, wxALL, 5 );
	
	m_PadLayerAdhCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerAdhCmp, 0, wxLEFT|wxRIGHT, 4 );
	
	m_PadLayerAdhCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerAdhCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerPateCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerPateCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerPateCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerPateCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerSilkCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerSilkCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerSilkCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerSilkCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerMaskCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerMaskCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerMaskCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerMaskCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerDraft = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Drafting notes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerDraft, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerECO1 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerECO1, 0, wxTOP|wxRIGHT|wxLEFT, 4 );
	
	m_PadLayerECO2 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerECO2, 0, wxALL, 4 );
	
	
	bSizer10->Add( m_LayersSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bGeneralSizer->Add( bSizer10, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panelGeneral->SetSizer( bGeneralSizer );
	m_panelGeneral->Layout();
	bGeneralSizer->Fit( m_panelGeneral );
	m_notebook->AddPage( m_panelGeneral, _("General"), true );
	m_localSettingsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearance;
	bSizerPanelClearance = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerClearance;
	bSizerClearance = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbClearancesSizer;
	sbClearancesSizer = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Clearances") ), wxVERTICAL );
	
	wxStaticText* m_staticTextHint;
	m_staticTextHint = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Set values to 0 to use parent footprint or netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHint->Wrap( -1 );
	m_staticTextHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	sbClearancesSizer->Add( m_staticTextHint, 0, wxRIGHT, 10 );
	
	m_staticTextInfoPosValue = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPosValue->Wrap( -1 );
	m_staticTextInfoPosValue->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	sbClearancesSizer->Add( m_staticTextInfoPosValue, 0, wxTOP|wxRIGHT, 10 );
	
	m_staticTextInfoNegVal = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoNegVal->Wrap( -1 );
	m_staticTextInfoNegVal->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	sbClearancesSizer->Add( m_staticTextInfoNegVal, 0, wxBOTTOM|wxRIGHT, 10 );
	
	wxFlexGridSizer* fgClearancesGridSizer;
	fgClearancesGridSizer = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgClearancesGridSizer->AddGrowableCol( 1 );
	fgClearancesGridSizer->SetFlexibleDirection( wxBOTH );
	fgClearancesGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_clearanceLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	m_clearanceLabel->SetToolTip( _("This is the local net clearance for this pad.\nIf 0, the footprint local value or the Netclass value is used.") );
	
	fgClearancesGridSizer->Add( m_clearanceLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_clearanceCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_clearanceCtrl, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_clearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_maskClearanceLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskClearanceLabel->Wrap( -1 );
	m_maskClearanceLabel->SetToolTip( _("This is the local clearance between this pad and the solder mask.\nIf 0, the footprint local value or the global value is used.") );
	
	fgClearancesGridSizer->Add( m_maskClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_maskClearanceCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_maskClearanceCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_maskClearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskClearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_maskClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_pasteClearanceLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteClearanceLabel->Wrap( -1 );
	m_pasteClearanceLabel->SetToolTip( _("This is the local clearance between this pad and the solder paste.\nIf 0, the footprint value or the global value is used.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller mask size than pad size.") );
	
	fgClearancesGridSizer->Add( m_pasteClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_pasteClearanceCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_pasteClearanceCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_pasteClearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteClearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_pasteClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextRatio = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in percent between this pad and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nIf 0, the footprint value or the global value is used.\nThe final clearance value is the sum of this value and the clearance value.\nA negative value means a smaller mask size than pad size.") );
	
	fgClearancesGridSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );
	
	m_SolderPasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbClearancesSizer->Add( fgClearancesGridSizer, 0, wxEXPAND, 5 );
	
	m_nonCopperWarningBook = new wxSimplebook( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* notePanel;
	notePanel = new wxPanel( m_nonCopperWarningBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bNoteSizer;
	bNoteSizer = new wxBoxSizer( wxVERTICAL );
	
	m_nonCopperNote = new wxStaticText( notePanel, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_nonCopperNote->Wrap( -1 );
	m_nonCopperNote->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bNoteSizer->Add( m_nonCopperNote, 0, wxTOP|wxRIGHT, 15 );
	
	
	notePanel->SetSizer( bNoteSizer );
	notePanel->Layout();
	bNoteSizer->Fit( notePanel );
	m_nonCopperWarningBook->AddPage( notePanel, _("a page"), false );
	wxPanel* warningPanel;
	warningPanel = new wxPanel( m_nonCopperWarningBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bWarningSizer;
	bWarningSizer = new wxBoxSizer( wxHORIZONTAL );
	
	bWarningSizer->SetMinSize( wxSize( -1,50 ) ); 
	m_nonCopperWarningIcon = new wxStaticBitmap( warningPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_nonCopperWarningIcon->SetMinSize( wxSize( 48,48 ) );
	
	bWarningSizer->Add( m_nonCopperWarningIcon, 0, wxALL, 5 );
	
	m_nonCopperWarningText = new wxStaticText( warningPanel, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_nonCopperWarningText->Wrap( -1 );
	m_nonCopperWarningText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bWarningSizer->Add( m_nonCopperWarningText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	warningPanel->SetSizer( bWarningSizer );
	warningPanel->Layout();
	bWarningSizer->Fit( warningPanel );
	m_nonCopperWarningBook->AddPage( warningPanel, _("a page"), false );
	
	sbClearancesSizer->Add( m_nonCopperWarningBook, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bSizerClearance->Add( sbClearancesSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_sbSizerZonesSettings = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Connection to Copper Zones") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerCopperZonesOpts;
	fgSizerCopperZonesOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerCopperZonesOpts->AddGrowableCol( 1 );
	fgSizerCopperZonesOpts->SetFlexibleDirection( wxBOTH );
	fgSizerCopperZonesOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText40 = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticText40, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_ZoneConnectionChoiceChoices[] = { _("From parent footprint"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneConnectionChoice, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerCopperZonesOpts->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_spokeWidthLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_spokeWidthCtrl = new wxTextCtrl( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthCtrl, 0, wxEXPAND|wxLEFT|wxTOP|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_spokeWidthUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_thermalGapLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_thermalGapLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_thermalGapCtrl = new wxTextCtrl( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_thermalGapCtrl, 0, wxEXPAND|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_thermalGapUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_thermalGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextcps = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Custom pad shape in zone:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextcps->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticTextcps, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	wxString m_ZoneCustomPadShapeChoices[] = { _("Use pad shape"), _("Use pad convex hull") };
	int m_ZoneCustomPadShapeNChoices = sizeof( m_ZoneCustomPadShapeChoices ) / sizeof( wxString );
	m_ZoneCustomPadShape = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneCustomPadShapeNChoices, m_ZoneCustomPadShapeChoices, 0 );
	m_ZoneCustomPadShape->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneCustomPadShape, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_sbSizerZonesSettings->Add( fgSizerCopperZonesOpts, 0, wxEXPAND, 5 );
	
	
	bSizerClearance->Add( m_sbSizerZonesSettings, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerPanelClearance->Add( bSizerClearance, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_localSettingsPanel->SetSizer( bSizerPanelClearance );
	m_localSettingsPanel->Layout();
	bSizerPanelClearance->Fit( m_localSettingsPanel );
	m_notebook->AddPage( m_localSettingsPanel, _("Local Clearance and Settings"), false );
	m_panelCustomShapePrimitives = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_bSizerPanelPrimitives = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPrimitivesList = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Primitives list"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitivesList->Wrap( -1 );
	m_staticTextPrimitivesList->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	m_bSizerPanelPrimitives->Add( m_staticTextPrimitivesList, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextPrimitiveListWarning = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Coordinates are relative to anchor pad, orientation 0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitiveListWarning->Wrap( -1 );
	m_staticTextPrimitiveListWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	m_bSizerPanelPrimitives->Add( m_staticTextPrimitiveListWarning, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_listCtrlPrimitives = new wxListView( m_panelCustomShapePrimitives, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_NO_HEADER|wxLC_REPORT );
	m_bSizerPanelPrimitives->Add( m_listCtrlPrimitives, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerButtonsUpper;
	bSizerButtonsUpper = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonDel = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Delete Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDel, 0, wxALL, 5 );
	
	m_buttonEditShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Edit Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonEditShape, 0, wxALL, 5 );
	
	m_buttonAddShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Add Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonAddShape, 0, wxALL, 5 );
	
	m_buttonDup = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Duplicate Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDup, 0, wxALL, 5 );
	
	m_buttonGeometry = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Transform Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonGeometry, 0, wxALL, 5 );
	
	
	bSizerButtons->Add( bSizerButtonsUpper, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_bSizerPanelPrimitives->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_panelCustomShapePrimitives->SetSizer( m_bSizerPanelPrimitives );
	m_panelCustomShapePrimitives->Layout();
	m_bSizerPanelPrimitives->Fit( m_panelCustomShapePrimitives );
	m_notebook->AddPage( m_panelCustomShapePrimitives, _("Custom Shape Primitives"), false );
	
	bSizerUpper->Add( m_notebook, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );
	
	
	bSizerDisplayPad->Add( 0, 0, 0, wxBOTTOM|wxEXPAND|wxTOP, 3 );
	
	m_parentInfoLine1 = new wxStaticText( this, wxID_ANY, _("Footprint name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_parentInfoLine1->Wrap( -1 );
	m_parentInfoLine1->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizerDisplayPad->Add( m_parentInfoLine1, 0, wxTOP, 8 );
	
	m_parentInfoLine2 = new wxStaticText( this, wxID_ANY, _("side and rotation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_parentInfoLine2->Wrap( -1 );
	m_parentInfoLine2->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizerDisplayPad->Add( m_parentInfoLine2, 0, wxRIGHT, 3 );
	
	
	bSizerDisplayPad->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_panelShowPad = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxBORDER_SIMPLE );
	m_panelShowPad->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	m_panelShowPad->SetMinSize( wxSize( 280,-1 ) );
	
	bSizerDisplayPad->Add( m_panelShowPad, 12, wxEXPAND|wxALL, 5 );
	
	m_panelShowPadGal = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, m_galOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO);
	m_panelShowPadGal->SetMinSize( wxSize( 280,-1 ) );
	
	bSizerDisplayPad->Add( m_panelShowPadGal, 12, wxEXPAND|wxALL, 5 );
	
	m_cbShowPadOutline = new wxCheckBox( this, wxID_ANY, _("Show pad in outline mode"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDisplayPad->Add( m_cbShowPadOutline, 0, wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	
	bSizerDisplayPad->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline13 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerDisplayPad->Add( m_staticline13, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	
	bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );
	
	
	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	m_MainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_panelGeneral->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUI ), NULL, this );
	m_PadNumCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_PadShape->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_sizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_orientation->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_offsetXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapDeltaCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapAxisCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_tcCornerSizeRatio->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_tcCornerRadius->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerRadiusChange ), NULL, this );
	m_tcChamferRatio->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeShapeCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_rbCopperLayersSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerDraft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO2->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_clearanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonDel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
	m_buttonEditShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonAddShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonDup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_panelShowPad->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_cbShowPadOutline->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onChangePadMode ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_PAD_PROPERTIES_BASE::~DIALOG_PAD_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_panelGeneral->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUI ), NULL, this );
	m_PadNumCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_PadShape->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_sizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_orientation->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_offsetXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapDeltaCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapAxisCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_tcCornerSizeRatio->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_tcCornerRadius->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerRadiusChange ), NULL, this );
	m_tcChamferRatio->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeShapeCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_rbCopperLayersSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerDraft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO2->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_clearanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonDel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
	m_buttonEditShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonAddShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonDup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_panelShowPad->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_cbShowPadOutline->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onChangePadMode ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );
	
}

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerShapeProperties;
	fgSizerShapeProperties = new wxFlexGridSizer( 0, 7, 3, 0 );
	fgSizerShapeProperties->AddGrowableCol( 2 );
	fgSizerShapeProperties->AddGrowableCol( 4 );
	fgSizerShapeProperties->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextPosStart = new wxStaticText( this, wxID_ANY, _("Start point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosStart->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosStart, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_startXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startXLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_startXCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_startXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_startXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );
	
	m_startYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startYLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_startYCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_startYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_startYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextPosCtrl1 = new wxStaticText( this, wxID_ANY, _("Control Point 1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl1->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_ctrl1XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_ctrl1XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1XCtrl, 0, wxALL, 5 );
	
	m_ctrl1XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_ctrl1YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_ctrl1YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1YCtrl, 0, wxALL, 5 );
	
	m_ctrl1YUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextPosCtrl2 = new wxStaticText( this, wxID_ANY, _("Control Point 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl2->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_ctrl2XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_ctrl2XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2XCtrl, 0, wxALL, 5 );
	
	m_ctrl2XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_ctrl2YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_ctrl2YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2YCtrl, 0, wxALL, 5 );
	
	m_ctrl2YUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2YUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2YUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextPosEnd = new wxStaticText( this, wxID_ANY, _("End point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosEnd->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosEnd, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_endXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endXLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_endXCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_endXCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_endXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_endYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endYLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_endYCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_endYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_endYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_radiusLabel = new wxStaticText( this, wxID_ANY, _("Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_radiusLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_radiusCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_radiusCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_radiusUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_radiusUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizermain->Add( fgSizerShapeProperties, 1, wxEXPAND|wxALL, 10 );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Set thickness to 0 for a filled circle."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizermain->Add( m_staticTextInfo, 0, wxRIGHT|wxLEFT, 15 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizermain->Add( m_sdbSizer, 0, wxALL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizermain );
	this->Layout();
	bSizermain->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::~DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE()
{
}

DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE::DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerShapeProperties1;
	fgSizerShapeProperties1 = new wxFlexGridSizer( 0, 7, 3, 0 );
	fgSizerShapeProperties1->AddGrowableCol( 2 );
	fgSizerShapeProperties1->AddGrowableCol( 4 );
	fgSizerShapeProperties1->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextMove = new wxStaticText( this, wxID_ANY, _("Move vector"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMove->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_staticTextMove, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_xLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );
	
	m_xCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_xCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_xUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_xUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );
	
	m_yLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_yLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_yCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_yCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_yUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_yUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_rotationLabel = new wxStaticText( this, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_rotationLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_rotationCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_rotationCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_rotationUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_rotationUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_scaleLabel = new wxStaticText( this, wxID_ANY, _("Scaling factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_scaleLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_scaleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_scaleCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_scaleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextDupCnt = new wxStaticText( this, wxID_ANY, _("Duplicate:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDupCnt->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_staticTextDupCnt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_spinCtrlDuplicateCount = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1 );
	fgSizerShapeProperties1->Add( m_spinCtrlDuplicateCount, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizermain->Add( fgSizerShapeProperties1, 1, wxALL|wxEXPAND, 10 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizermain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizermain );
	this->Layout();
	bSizermain->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE::~DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE()
{
}

DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_gridCornersList = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );
	
	// Grid
	m_gridCornersList->CreateGrid( 1, 2 );
	m_gridCornersList->EnableEditing( true );
	m_gridCornersList->EnableGridLines( true );
	m_gridCornersList->EnableDragGridSize( false );
	m_gridCornersList->SetMargins( 0, 0 );
	
	// Columns
	m_gridCornersList->SetColSize( 0, 100 );
	m_gridCornersList->SetColSize( 1, 100 );
	m_gridCornersList->EnableDragColMove( false );
	m_gridCornersList->EnableDragColSize( true );
	m_gridCornersList->SetColLabelSize( 22 );
	m_gridCornersList->SetColLabelValue( 0, _("Pos X") );
	m_gridCornersList->SetColLabelValue( 1, _("Pos Y") );
	m_gridCornersList->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	
	// Rows
	m_gridCornersList->AutoSizeRows();
	m_gridCornersList->EnableDragRowSize( false );
	m_gridCornersList->SetRowLabelSize( 80 );
	m_gridCornersList->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridCornersList->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bLeftSizer->Add( m_gridCornersList, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bSizerRightButts;
	bSizerRightButts = new wxBoxSizer( wxHORIZONTAL );
	
	m_addButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_addButton->SetMinSize( wxSize( 30,30 ) );
	
	bSizerRightButts->Add( m_addButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerRightButts->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_deleteButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteButton->SetMinSize( wxSize( 30,30 ) );
	
	bSizerRightButts->Add( m_deleteButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bLeftSizer->Add( bSizerRightButts, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerThickness;
	fgSizerThickness = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerThickness->AddGrowableCol( 1 );
	fgSizerThickness->SetFlexibleDirection( wxBOTH );
	fgSizerThickness->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Outline thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_thicknessCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_thicknessCtrl, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bLeftSizer->Add( fgSizerThickness, 0, wxALL|wxEXPAND, 10 );
	
	
	bSizerUpper->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_panelPoly = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelPoly->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	m_panelPoly->SetMinSize( wxSize( 290,290 ) );
	
	bRightSizer->Add( m_panelPoly, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );
	
	wxBoxSizer* m_warningSizer;
	m_warningSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_warningIcon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_warningIcon->SetMinSize( wxSize( 50,50 ) );
	
	m_warningSizer->Add( m_warningIcon, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_warningText = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_warningText->Wrap( -1 );
	m_warningSizer->Add( m_warningText, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );
	
	
	m_warningSizer->Add( 5, 88, 0, 0, 5 );
	
	
	bRightSizer->Add( m_warningSizer, 0, wxEXPAND|wxRIGHT, 10 );
	
	
	bSizerUpper->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer25->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 4 );
	
	m_statusLine1 = new wxStaticText( this, wxID_ANY, _("Coordinates are relative to anchor pad, rotated 0.0 deg."), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine1->Wrap( -1 );
	m_statusLine1->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer25->Add( m_statusLine1, 0, 0, 5 );
	
	m_statusLine2 = new wxStaticText( this, wxID_ANY, _("Set thickness to 0 for a filled polygon."), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_statusLine2->Wrap( -1 );
	m_statusLine2->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer25->Add( m_statusLine2, 0, wxTOP, 2 );
	
	
	bSizer24->Add( bSizer25, 1, wxEXPAND|wxLEFT, 10 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizer24->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	bSizerMain->Add( bSizer24, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_gridCornersList->Connect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonAdd ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );
}

DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE()
{
	// Disconnect Events
	m_gridCornersList->Disconnect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonAdd ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );
	
}
