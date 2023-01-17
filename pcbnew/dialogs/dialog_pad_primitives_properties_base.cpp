///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/text_ctrl_eval.h"
#include "widgets/wx_grid.h"

#include "dialog_pad_primitives_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerShapeProperties;
	fgSizerShapeProperties = new wxFlexGridSizer( 0, 7, 5, 0 );
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

	m_staticTextPosCtrl1 = new wxStaticText( this, wxID_ANY, _("Control point 1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl1->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ctrl1XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl1XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1XCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl1XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ctrl1YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl1YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1YCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl1YUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextPosCtrl2 = new wxStaticText( this, wxID_ANY, _("Control point 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl2->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ctrl2XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl2XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2XCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl2XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ctrl2YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl2YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2YCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

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

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_filledCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizermain->Add( fgSizerShapeProperties, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizermain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


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

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizermain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


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
	m_gridCornersList->SetColSize( 0, 124 );
	m_gridCornersList->SetColSize( 1, 124 );
	m_gridCornersList->EnableDragColMove( false );
	m_gridCornersList->EnableDragColSize( true );
	m_gridCornersList->SetColLabelValue( 0, _("Pos X") );
	m_gridCornersList->SetColLabelValue( 1, _("Pos Y") );
	m_gridCornersList->SetColLabelSize( 22 );
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

	m_addButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addButton->SetMinSize( wxSize( 30,30 ) );

	bSizerRightButts->Add( m_addButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerRightButts->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_deleteButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteButton->SetMinSize( wxSize( 30,30 ) );

	bSizerRightButts->Add( m_deleteButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bLeftSizer->Add( bSizerRightButts, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerThickness;
	fgSizerThickness = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerThickness->AddGrowableCol( 1 );
	fgSizerThickness->SetFlexibleDirection( wxBOTH );
	fgSizerThickness->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_thicknessCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_filledCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );


	bLeftSizer->Add( fgSizerThickness, 0, wxALL|wxEXPAND, 10 );


	bSizerUpper->Add( bLeftSizer, 1, wxEXPAND|wxRIGHT, 5 );

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


	bSizerUpper->Add( bRightSizer, 1, wxEXPAND|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxHORIZONTAL );

	m_statusLine1 = new wxStaticText( this, wxID_ANY, _("Coordinates are relative to anchor pad, rotated 0.0 deg."), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine1->Wrap( -1 );
	bSizer24->Add( m_statusLine1, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer24->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


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
