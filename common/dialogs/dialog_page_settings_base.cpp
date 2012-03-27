///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 17 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_page_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAGES_SETTINGS_BASE::DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizerH;
	bUpperSizerH = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* LeftColumnSizer;
	LeftColumnSizer = new wxFlexGridSizer( 3, 1, 0, 0 );
	LeftColumnSizer->AddGrowableRow( 0 );
	LeftColumnSizer->AddGrowableRow( 1 );
	LeftColumnSizer->AddGrowableRow( 2 );
	LeftColumnSizer->SetFlexibleDirection( wxBOTH );
	LeftColumnSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticBoxSizer* PaperSizer;
	PaperSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Paper") ), wxVERTICAL );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	PaperSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_paperSizeComboBoxChoices[] = { _("dummy text") };
	int m_paperSizeComboBoxNChoices = sizeof( m_paperSizeComboBoxChoices ) / sizeof( wxString );
	m_paperSizeComboBox = new wxChoice( this, ID_CHICE_PAGE_SIZE, wxDefaultPosition, wxDefaultSize, m_paperSizeComboBoxNChoices, m_paperSizeComboBoxChoices, 0 );
	m_paperSizeComboBox->SetSelection( 0 );
	PaperSizer->Add( m_paperSizeComboBox, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	PaperSizer->Add( m_staticText6, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_orientationComboBoxChoices[] = { _("Landscape"), _("Portrait") };
	int m_orientationComboBoxNChoices = sizeof( m_orientationComboBoxChoices ) / sizeof( wxString );
	m_orientationComboBox = new wxChoice( this, ID_CHOICE_PAGE_ORIENTATION, wxDefaultPosition, wxDefaultSize, m_orientationComboBoxNChoices, m_orientationComboBoxChoices, 0 );
	m_orientationComboBox->SetSelection( 0 );
	PaperSizer->Add( m_orientationComboBox, 0, wxEXPAND|wxALL, 5 );
	
	
	PaperSizer->Add( 0, 10, 0, 0, 5 );
	
	wxStaticBoxSizer* CustomPaperSizer;
	CustomPaperSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Custom Size") ), wxHORIZONTAL );
	
	
	CustomPaperSizer->Add( 5, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* CustomPaperWidth;
	CustomPaperWidth = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Width:") ), wxVERTICAL );
	
	m_TextUserSizeX = new wxTextCtrl( this, ID_TEXTCTRL_USER_PAGE_SIZE_X, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT );
	m_TextUserSizeX->SetMaxLength( 6 ); 
	m_TextUserSizeX->SetToolTip( _("Custom paper width.") );
	
	CustomPaperWidth->Add( m_TextUserSizeX, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxEXPAND, 5 );
	
	
	CustomPaperSizer->Add( CustomPaperWidth, 0, wxEXPAND, 5 );
	
	
	CustomPaperSizer->Add( 10, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* CustomPaperHeight;
	CustomPaperHeight = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Height:") ), wxVERTICAL );
	
	m_TextUserSizeY = new wxTextCtrl( this, ID_TEXTCTRL_USER_PAGE_SIZE_Y, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT );
	m_TextUserSizeY->SetMaxLength( 6 ); 
	m_TextUserSizeY->SetToolTip( _("Custom paper height.") );
	
	CustomPaperHeight->Add( m_TextUserSizeY, 0, wxALIGN_TOP|wxALL|wxEXPAND, 5 );
	
	
	CustomPaperSizer->Add( CustomPaperHeight, 0, wxEXPAND, 5 );
	
	
	CustomPaperSizer->Add( 5, 50, 0, 0, 5 );
	
	
	PaperSizer->Add( CustomPaperSizer, 1, wxEXPAND, 5 );
	
	
	LeftColumnSizer->Add( PaperSizer, 1, wxALL, 5 );
	
	wxStaticBoxSizer* PageLayoutExampleSizer;
	PageLayoutExampleSizer = new wxStaticBoxSizer( new wxStaticBox( this, ID_PAGE_LAYOUT_EXAMPLE_SIZER, _("Layout Preview") ), wxVERTICAL );
	
	PageLayoutExampleSizer->SetMinSize( wxSize( 240,-1 ) ); 
	m_PageLayoutExampleBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxSIMPLE_BORDER );
	m_PageLayoutExampleBitmap->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_PageLayoutExampleBitmap->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	PageLayoutExampleSizer->Add( m_PageLayoutExampleBitmap, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	LeftColumnSizer->Add( PageLayoutExampleSizer, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
	
	
	LeftColumnSizer->Add( 0, 1, 1, wxEXPAND, 5 );
	
	
	bUpperSizerH->Add( LeftColumnSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* RightColumnSizer;
	RightColumnSizer = new wxFlexGridSizer( 8, 1, 0, 0 );
	RightColumnSizer->AddGrowableCol( 0 );
	RightColumnSizer->AddGrowableRow( 0 );
	RightColumnSizer->AddGrowableRow( 1 );
	RightColumnSizer->AddGrowableRow( 2 );
	RightColumnSizer->AddGrowableRow( 3 );
	RightColumnSizer->AddGrowableRow( 4 );
	RightColumnSizer->AddGrowableRow( 5 );
	RightColumnSizer->AddGrowableRow( 6 );
	RightColumnSizer->AddGrowableRow( 7 );
	RightColumnSizer->SetFlexibleDirection( wxBOTH );
	RightColumnSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticBoxSizer* BasicInscriptionsSizer;
	BasicInscriptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Basic Inscriptions") ), wxVERTICAL );
	
	BasicInscriptionsSizer->SetMinSize( wxSize( -1,452 ) ); 
	wxBoxSizer* SheetInfoSizer;
	SheetInfoSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_TextSheetCount = new wxStaticText( this, wxID_ANY, _("Number of sheets: %d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSheetCount->Wrap( -1 );
	SheetInfoSizer->Add( m_TextSheetCount, 0, wxALL, 5 );
	
	
	SheetInfoSizer->Add( 5, 5, 1, wxEXPAND, 5 );
	
	m_TextSheetNumber = new wxStaticText( this, wxID_ANY, _("Sheet number: %d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSheetNumber->Wrap( -1 );
	SheetInfoSizer->Add( m_TextSheetNumber, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( SheetInfoSizer, 0, 0, 5 );
	
	wxStaticBoxSizer* RevisionSizer;
	RevisionSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Revision") ), wxHORIZONTAL );
	
	m_TextRevision = new wxTextCtrl( this, ID_TEXTCTRL_REVISION, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextRevision->SetMinSize( wxSize( 100,-1 ) );
	
	RevisionSizer->Add( m_TextRevision, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_RevisionExport = new wxCheckBox( this, ID_CHECKBOX_REVISION, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	RevisionSizer->Add( m_RevisionExport, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	BasicInscriptionsSizer->Add( RevisionSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* TitleSizer;
	TitleSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Title") ), wxHORIZONTAL );
	
	m_TextTitle = new wxTextCtrl( this, ID_TEXTCTRL_TITLE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextTitle->SetMinSize( wxSize( 360,-1 ) );
	
	TitleSizer->Add( m_TextTitle, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TitleExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	TitleSizer->Add( m_TitleExport, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( TitleSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* CompanySizer;
	CompanySizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Company") ), wxHORIZONTAL );
	
	m_TextCompany = new wxTextCtrl( this, ID_TEXTCTRL_COMPANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextCompany->SetMinSize( wxSize( 360,-1 ) );
	
	CompanySizer->Add( m_TextCompany, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_CompanyExport = new wxCheckBox( this, ID_CHECKBOX_COMPANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	CompanySizer->Add( m_CompanyExport, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( CompanySizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* Comment1Sizer;
	Comment1Sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Comment1") ), wxHORIZONTAL );
	
	m_TextComment1 = new wxTextCtrl( this, ID_TEXTCTRL_COMMENT1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment1->SetMinSize( wxSize( 360,-1 ) );
	
	Comment1Sizer->Add( m_TextComment1, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Comment1Export = new wxCheckBox( this, ID_CHECKBOX_COMMENT1, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	Comment1Sizer->Add( m_Comment1Export, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( Comment1Sizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* Comment2Sizer;
	Comment2Sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Comment2") ), wxHORIZONTAL );
	
	m_TextComment2 = new wxTextCtrl( this, ID_TEXTCTRL_COMMENT2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment2->SetMinSize( wxSize( 360,-1 ) );
	
	Comment2Sizer->Add( m_TextComment2, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Comment2Export = new wxCheckBox( this, ID_CHECKBOX_COMMENT2, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	Comment2Sizer->Add( m_Comment2Export, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( Comment2Sizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* Comment3Sizer;
	Comment3Sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Comment3") ), wxHORIZONTAL );
	
	m_TextComment3 = new wxTextCtrl( this, ID_TEXTCTRL_COMMENT3, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment3->SetMinSize( wxSize( 360,-1 ) );
	
	Comment3Sizer->Add( m_TextComment3, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Comment3Export = new wxCheckBox( this, ID_CHECKBOX_COMMENT3, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	Comment3Sizer->Add( m_Comment3Export, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( Comment3Sizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* Comment4Sizer;
	Comment4Sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Comment4") ), wxHORIZONTAL );
	
	m_TextComment4 = new wxTextCtrl( this, ID_TEXTCTRL_COMMENT4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment4->SetMinSize( wxSize( 360,-1 ) );
	
	Comment4Sizer->Add( m_TextComment4, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Comment4Export = new wxCheckBox( this, ID_CHECKBOX_COMMENT4, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	Comment4Sizer->Add( m_Comment4Export, 0, wxALL, 5 );
	
	
	BasicInscriptionsSizer->Add( Comment4Sizer, 1, wxEXPAND, 5 );
	
	
	RightColumnSizer->Add( BasicInscriptionsSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizerH->Add( RightColumnSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bUpperSizerH, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bMainSizer->Add( m_sdbSizer1, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCloseWindow ) );
	m_paperSizeComboBox->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPaperSizeChoice ), NULL, this );
	m_orientationComboBox->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPageOrientationChoice ), NULL, this );
	m_TextUserSizeX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeXTextUpdated ), NULL, this );
	m_TextUserSizeY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeYTextUpdated ), NULL, this );
	m_TextRevision->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnRevisionTextUpdated ), NULL, this );
	m_TextTitle->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnTitleTextUpdated ), NULL, this );
	m_TitleExport->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCheckboxTitleClick ), NULL, this );
	m_TextCompany->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCompanyTextUpdated ), NULL, this );
	m_TextComment1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment1TextUpdated ), NULL, this );
	m_TextComment2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment2TextUpdated ), NULL, this );
	m_TextComment3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment3TextUpdated ), NULL, this );
	m_TextComment4->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment4TextUpdated ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PAGES_SETTINGS_BASE::~DIALOG_PAGES_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCloseWindow ) );
	m_paperSizeComboBox->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPaperSizeChoice ), NULL, this );
	m_orientationComboBox->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPageOrientationChoice ), NULL, this );
	m_TextUserSizeX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeXTextUpdated ), NULL, this );
	m_TextUserSizeY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeYTextUpdated ), NULL, this );
	m_TextRevision->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnRevisionTextUpdated ), NULL, this );
	m_TextTitle->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnTitleTextUpdated ), NULL, this );
	m_TitleExport->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCheckboxTitleClick ), NULL, this );
	m_TextCompany->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCompanyTextUpdated ), NULL, this );
	m_TextComment1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment1TextUpdated ), NULL, this );
	m_TextComment2->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment2TextUpdated ), NULL, this );
	m_TextComment3->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment3TextUpdated ), NULL, this );
	m_TextComment4->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment4TextUpdated ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnOkClick ), NULL, this );
	
}
