///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_3D_view_option_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_3D_VIEW_OPTIONS_BASE::DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizeShowOpts;
	fgSizeShowOpts = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizeShowOpts->SetFlexibleDirection( wxBOTH );
	fgSizeShowOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bitmapRealisticMode = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapRealisticMode, 0, wxALL, 5 );
	
	m_checkBoxRealisticMode = new wxCheckBox( this, wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxRealisticMode, 0, wxALL, 5 );
	
	m_bitmapCuThickness = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapCuThickness, 0, wxALL, 5 );
	
	m_checkBoxCuThickness = new wxCheckBox( this, wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxCuThickness, 0, wxALL, 5 );
	
	m_bitmap3Dshapes = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmap3Dshapes, 0, wxALL, 5 );
	
	m_checkBox3Dshapes = new wxCheckBox( this, wxID_ANY, _("Show component 3D shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBox3Dshapes, 0, wxALL, 5 );
	
	m_bitmapAreas = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapAreas, 0, wxALL, 5 );
	
	m_checkBoxAreas = new wxCheckBox( this, wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxAreas, 0, wxALL, 5 );
	
	m_bitmapSilkscreen = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapSilkscreen, 0, wxALL, 5 );
	
	m_checkBoxSilkscreen = new wxCheckBox( this, wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxSilkscreen, 0, wxALL, 5 );
	
	m_bitmapSolderMask = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapSolderMask, 0, wxALL, 5 );
	
	m_checkBoxSolderMask = new wxCheckBox( this, wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxSolderMask, 0, wxALL, 5 );
	
	m_bitmapSolderPaste = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapSolderPaste, 0, wxALL, 5 );
	
	m_checkBoxSolderpaste = new wxCheckBox( this, wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxSolderpaste, 0, wxALL, 5 );
	
	m_bitmapAdhesive = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapAdhesive, 0, wxALL, 5 );
	
	m_checkBoxAdhesive = new wxCheckBox( this, wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxAdhesive, 0, wxALL, 5 );
	
	m_bitmapComments = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapComments, 0, wxALL, 5 );
	
	m_checkBoxComments = new wxCheckBox( this, wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxComments, 0, wxALL, 5 );
	
	m_bitmapECO = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_bitmapECO, 0, wxALL, 5 );
	
	m_checkBoxECO = new wxCheckBox( this, wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeShowOpts->Add( m_checkBoxECO, 0, wxALL, 5 );
	
	
	bSizerLeft->Add( fgSizeShowOpts, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_buttonShowAll = new wxButton( this, wxID_ANY, _("Show All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonShowAll, 0, wxALL, 5 );
	
	m_buttonShowNone = new wxButton( this, wxID_ANY, _("Show None"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonShowNone, 0, wxALL, 5 );
	
	
	bSizerLeft->Add( bSizerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_checkBoxRealisticMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_buttonShowAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnShowAllClick ), NULL, this );
	m_buttonShowNone->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnShowNoneClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnOKClick ), NULL, this );
}

DIALOG_3D_VIEW_OPTIONS_BASE::~DIALOG_3D_VIEW_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_buttonShowAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnShowAllClick ), NULL, this );
	m_buttonShowNone->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnShowNoneClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnOKClick ), NULL, this );
	
}
