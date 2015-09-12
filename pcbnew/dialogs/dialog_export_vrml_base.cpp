///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_vrml_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_3DFILE_BASE::DIALOG_EXPORT_3DFILE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("File Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bUpperSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_filePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Save VRML Board File"), wxT("*.wrl"), wxDefaultPosition, wxDefaultSize, wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bUpperSizer->Add( m_filePicker, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Footprint 3D model path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bUpperSizer->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SubdirNameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SubdirNameCtrl->SetMaxLength( 0 ); 
	bUpperSizer->Add( m_SubdirNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer1->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( m_panel1, wxID_ANY, _("Grid Reference Point:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer9->Add( m_staticText6, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizerOptions;
	fgSizerOptions = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerOptions->AddGrowableCol( 1 );
	fgSizerOptions->SetFlexibleDirection( wxBOTH );
	fgSizerOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText61 = new wxStaticText( m_panel1, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText61->Wrap( -1 );
	fgSizerOptions->Add( m_staticText61, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_VRML_RefUnitChoiceChoices[] = { _("mm"), _("inch") };
	int m_VRML_RefUnitChoiceNChoices = sizeof( m_VRML_RefUnitChoiceChoices ) / sizeof( wxString );
	m_VRML_RefUnitChoice = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_VRML_RefUnitChoiceNChoices, m_VRML_RefUnitChoiceChoices, 0 );
	m_VRML_RefUnitChoice->SetSelection( 0 );
	fgSizerOptions->Add( m_VRML_RefUnitChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText4 = new wxStaticText( m_panel1, wxID_ANY, _("X Ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizerOptions->Add( m_staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_VRML_Xref = new wxTextCtrl( m_panel1, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_VRML_Xref->SetMaxLength( 8 ); 
	fgSizerOptions->Add( m_VRML_Xref, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText5 = new wxStaticText( m_panel1, wxID_ANY, _("Y Ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizerOptions->Add( m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_VRML_Yref = new wxTextCtrl( m_panel1, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_VRML_Yref->SetMaxLength( 8 ); 
	fgSizerOptions->Add( m_VRML_Yref, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer9->Add( fgSizerOptions, 1, wxEXPAND, 5 );
	
	
	m_panel1->SetSizer( bSizer9 );
	m_panel1->Layout();
	bSizer9->Fit( m_panel1 );
	bSizer5->Add( m_panel1, 1, wxEXPAND | wxALL, 5 );
	
	
	bSizer12->Add( bSizer5, 1, wxEXPAND, 5 );
	
	wxString m_rbSelectUnitsChoices[] = { _("mm"), _("meter"), _("0.1 Inch"), _("Inch") };
	int m_rbSelectUnitsNChoices = sizeof( m_rbSelectUnitsChoices ) / sizeof( wxString );
	m_rbSelectUnits = new wxRadioBox( this, wxID_ANY, _("Output Units:"), wxDefaultPosition, wxDefaultSize, m_rbSelectUnitsNChoices, m_rbSelectUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbSelectUnits->SetSelection( 0 );
	bSizer12->Add( m_rbSelectUnits, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer1->Add( bSizer12, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_cbCopyFiles = new wxCheckBox( this, wxID_ANY, _("Copy 3D model files to 3D model path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopyFiles->SetValue(true); 
	bSizer4->Add( m_cbCopyFiles, 0, wxALL, 5 );
	
	m_cbUseRelativePaths = new wxCheckBox( this, ID_USE_ABS_PATH, _("Use relative paths to model files in board VRML file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUseRelativePaths->SetToolTip( _("Use paths for model files in board VRML file relative to the vrml file") );
	
	bSizer4->Add( m_cbUseRelativePaths, 0, wxALL, 5 );
	
	m_cbPlainPCB = new wxCheckBox( this, wxID_ANY, _("Plain PCB (no copper or silk)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbPlainPCB, 0, wxALL, 5 );
	
	
	bLowerSizer->Add( bSizer4, 2, wxEXPAND, 5 );
	
	
	bSizer1->Add( bLowerSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer1->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );
	
	// Connect Events
	m_filePicker->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnFileChanged ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnOkClick ), NULL, this );
}

DIALOG_EXPORT_3DFILE_BASE::~DIALOG_EXPORT_3DFILE_BASE()
{
	// Disconnect Events
	m_filePicker->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnFileChanged ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnOkClick ), NULL, this );
	
}
