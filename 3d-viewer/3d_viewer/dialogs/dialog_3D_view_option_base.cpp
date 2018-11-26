///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 11 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_3D_view_option_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_3D_VIEW_OPTIONS_BASE::DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticText3DRenderOpts = new wxStaticText( this, wxID_ANY, _("Render options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3DRenderOpts->Wrap( -1 );
	m_staticText3DRenderOpts->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizeLeft->Add( m_staticText3DRenderOpts, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizerRenderOptions;
	fgSizerRenderOptions = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRenderOptions->SetFlexibleDirection( wxBOTH );
	fgSizerRenderOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizerRenderOptions->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );
	
	m_bitmapRealisticMode = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxRealisticMode = new wxCheckBox( this, wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapBoardBody = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxBoardBody = new wxCheckBox( this, wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerRenderOptions->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapCuThickness = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapCuThickness, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxCuThickness = new wxCheckBox( this, wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxCuThickness, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapBoundingBoxes = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapBoundingBoxes, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxBoundingBoxes = new wxCheckBox( this, wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxBoundingBoxes, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapAreas = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxAreas = new wxCheckBox( this, wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizerRenderOptions->Add( fgSizer3, 1, wxEXPAND, 5 );
	
	
	bSizeLeft->Add( fgSizerRenderOptions, 0, wxEXPAND|wxBOTTOM, 5 );
	
	
	bSizeLeft->Add( 0, 10, 0, 0, 5 );
	
	m_staticText3DmodelVisibility = new wxStaticText( this, wxID_ANY, _("3D model visibility:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3DmodelVisibility->Wrap( -1 );
	m_staticText3DmodelVisibility->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizeLeft->Add( m_staticText3DmodelVisibility, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer3DVisibility;
	fgSizer3DVisibility = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3DVisibility->SetFlexibleDirection( wxBOTH );
	fgSizer3DVisibility->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer3DVisibility->Add( 0, 0, 1, wxRIGHT|wxLEFT, 10 );
	
	m_bitmap3DshapesTH = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBox3DshapesTH = new wxCheckBox( this, wxID_ANY, _("Show 3D through hole models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmap3DshapesSMD = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBox3DshapesSMD = new wxCheckBox( this, wxID_ANY, _("Show 3D SMD models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmap3DshapesVirtual = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBox3DshapesVirtual = new wxCheckBox( this, wxID_ANY, _("Show 3D virtual models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizeLeft->Add( fgSizer3DVisibility, 0, wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizeLeft, 1, wxEXPAND, 5 );
	
	m_staticlineVertical = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizerUpper->Add( m_staticlineVertical, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizeLayer;
	bSizeLayer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextBoardLayers = new wxStaticText( this, wxID_ANY, _("Board layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBoardLayers->Wrap( -1 );
	m_staticTextBoardLayers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizeLayer->Add( m_staticTextBoardLayers, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizerShowBrdLayersOpts;
	fgSizerShowBrdLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowBrdLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowBrdLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapSilkscreen = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSilkscreen, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxSilkscreen = new wxCheckBox( this, wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSilkscreen, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapSolderMask = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxSolderMask = new wxCheckBox( this, wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapSolderPaste = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderPaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxSolderpaste = new wxCheckBox( this, wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderpaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapAdhesive = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxAdhesive = new wxCheckBox( this, wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizeLayer->Add( fgSizerShowBrdLayersOpts, 0, wxEXPAND, 5 );
	
	
	bSizeLayer->Add( 0, 10, 0, 0, 5 );
	
	m_staticTextUserLayers = new wxStaticText( this, wxID_ANY, _("User layers (not shown in realistic mode):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUserLayers->Wrap( -1 );
	m_staticTextUserLayers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizeLayer->Add( m_staticTextUserLayers, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizerShowUserLayersOpts;
	fgSizerShowUserLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowUserLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowUserLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapComments = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxComments = new wxCheckBox( this, wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_bitmapECO = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxECO = new wxCheckBox( this, wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizeLayer->Add( fgSizerShowUserLayersOpts, 0, wxEXPAND, 5 );
	
	
	bSizerRight->Add( bSizeLayer, 1, wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizerRight, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticlineH = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticlineH, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_checkBoxRealisticMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
}

DIALOG_3D_VIEW_OPTIONS_BASE::~DIALOG_3D_VIEW_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	
}
