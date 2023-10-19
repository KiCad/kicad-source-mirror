///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_page_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAGES_SETTINGS_BASE::DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizerH;
	bUpperSizerH = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextPaper = new wxStaticText( this, wxID_ANY, _("Paper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPaper->Wrap( -1 );
	bleftSizer->Add( m_staticTextPaper, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bleftSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP, 1 );


	bleftSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_staticTextSize = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSize->Wrap( -1 );
	bleftSizer->Add( m_staticTextSize, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_paperSizeComboBoxChoices[] = { _("dummy text") };
	int m_paperSizeComboBoxNChoices = sizeof( m_paperSizeComboBoxChoices ) / sizeof( wxString );
	m_paperSizeComboBox = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_paperSizeComboBoxNChoices, m_paperSizeComboBoxChoices, 0 );
	m_paperSizeComboBox->SetSelection( 0 );
	bleftSizer->Add( m_paperSizeComboBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bleftSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_staticTextOrient = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrient->Wrap( -1 );
	bleftSizer->Add( m_staticTextOrient, 0, wxRIGHT|wxLEFT, 5 );

	wxString m_orientationComboBoxChoices[] = { _("Landscape"), _("Portrait") };
	int m_orientationComboBoxNChoices = sizeof( m_orientationComboBoxChoices ) / sizeof( wxString );
	m_orientationComboBox = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_orientationComboBoxNChoices, m_orientationComboBoxChoices, 0 );
	m_orientationComboBox->SetSelection( 0 );
	bleftSizer->Add( m_orientationComboBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextCustSize = new wxStaticText( this, wxID_ANY, _("Custom paper size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCustSize->Wrap( -1 );
	bleftSizer->Add( m_staticTextCustSize, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bleftSizer->Add( 0, 2, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_userSizeYLabel = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userSizeYLabel->Wrap( -1 );
	fgSizer1->Add( m_userSizeYLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_userSizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT );
	#ifdef __WXGTK__
	if ( !m_userSizeYCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_userSizeYCtrl->SetMaxLength( -1 );
	}
	#else
	m_userSizeYCtrl->SetMaxLength( -1 );
	#endif
	m_userSizeYCtrl->SetToolTip( _("Custom paper height.") );

	fgSizer1->Add( m_userSizeYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_userSizeYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userSizeYUnits->Wrap( -1 );
	fgSizer1->Add( m_userSizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_userSizeXLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userSizeXLabel->Wrap( -1 );
	fgSizer1->Add( m_userSizeXLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_userSizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT );
	#ifdef __WXGTK__
	if ( !m_userSizeXCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_userSizeXCtrl->SetMaxLength( -1 );
	}
	#else
	m_userSizeXCtrl->SetMaxLength( -1 );
	#endif
	m_userSizeXCtrl->SetToolTip( _("Custom paper width.") );

	fgSizer1->Add( m_userSizeXCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_userSizeXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userSizeXUnits->Wrap( -1 );
	fgSizer1->Add( m_userSizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bleftSizer->Add( fgSizer1, 0, wxEXPAND|wxBOTTOM, 5 );

	m_PaperExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	bleftSizer->Add( m_PaperExport, 0, wxALL, 4 );


	bleftSizer->Add( 0, 20, 0, wxEXPAND, 5 );

	m_staticTextPreview = new wxStaticText( this, wxID_ANY, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPreview->Wrap( -1 );
	bleftSizer->Add( m_staticTextPreview, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline31 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bleftSizer->Add( m_staticline31, 0, wxEXPAND|wxTOP, 1 );


	bleftSizer->Add( 0, 12, 0, wxEXPAND, 5 );

	m_PageLayoutExampleBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxBORDER_SIMPLE );
	m_PageLayoutExampleBitmap->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_PageLayoutExampleBitmap->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bleftSizer->Add( m_PageLayoutExampleBitmap, 1, wxALL|wxEXPAND, 5 );


	bUpperSizerH->Add( bleftSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizerH->Add( 15, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextDrawingSheet = new wxStaticText( this, wxID_ANY, _("Drawing Sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDrawingSheet->Wrap( -1 );
	bSizerRight->Add( m_staticTextDrawingSheet, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRight->Add( m_staticline4, 0, wxEXPAND|wxTOP, 1 );


	bSizerRight->Add( 0, 10, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerFilename;
	bSizerFilename = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerfileSelection;
	bSizerfileSelection = new wxBoxSizer( wxHORIZONTAL );

	m_staticText30 = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	bSizerfileSelection->Add( m_staticText30, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_textCtrlFilePicker = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerfileSelection->Add( m_textCtrlFilePicker, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerfileSelection->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );


	bSizerFilename->Add( bSizerfileSelection, 1, wxBOTTOM|wxEXPAND, 5 );


	bSizerRight->Add( bSizerFilename, 0, wxEXPAND|wxALL, 5 );


	bSizerRight->Add( 0, 10, 0, wxEXPAND, 5 );

	m_staticTextTitleBlock = new wxStaticText( this, wxID_ANY, _("Title Block Parameters"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTitleBlock->Wrap( -1 );
	bSizerRight->Add( m_staticTextTitleBlock, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRight->Add( m_staticline3, 0, wxEXPAND|wxTOP, 1 );


	bSizerRight->Add( 0, 10, 0, wxEXPAND, 5 );

	wxBoxSizer* SheetInfoSizer;
	SheetInfoSizer = new wxBoxSizer( wxHORIZONTAL );

	m_TextSheetCount = new wxStaticText( this, wxID_ANY, _("Number of sheets: %d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSheetCount->Wrap( -1 );
	SheetInfoSizer->Add( m_TextSheetCount, 0, wxALL, 5 );


	SheetInfoSizer->Add( 5, 5, 1, wxEXPAND, 5 );

	m_TextSheetNumber = new wxStaticText( this, wxID_ANY, _("Sheet number: %d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSheetNumber->Wrap( -1 );
	SheetInfoSizer->Add( m_TextSheetNumber, 0, wxALL, 5 );


	bSizerRight->Add( SheetInfoSizer, 0, wxLEFT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextDate = new wxStaticText( this, wxID_ANY, _("Issue Date:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDate->Wrap( -1 );
	fgSizer2->Add( m_staticTextDate, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerDate;
	bSizerDate = new wxBoxSizer( wxHORIZONTAL );

	m_TextDate = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDate->SetMinSize( wxSize( 100,-1 ) );

	bSizerDate->Add( m_TextDate, 3, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ApplyDate = new wxButton( this, wxID_ANY, _("<<<"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerDate->Add( m_ApplyDate, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_PickDate = new wxDatePickerCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT );
	bSizerDate->Add( m_PickDate, 2, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	fgSizer2->Add( bSizerDate, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_DateExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_DateExport, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextRev = new wxStaticText( this, wxID_ANY, _("Revision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRev->Wrap( -1 );
	fgSizer2->Add( m_staticTextRev, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextRevision = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextRevision->SetMinSize( wxSize( 100,-1 ) );

	fgSizer2->Add( m_TextRevision, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_RevisionExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_RevisionExport, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextTitle = new wxStaticText( this, wxID_ANY, _("Title:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTitle->Wrap( -1 );
	fgSizer2->Add( m_staticTextTitle, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 4 );

	m_TextTitle = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextTitle->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextTitle, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_TitleExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_TitleExport, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextCompany = new wxStaticText( this, wxID_ANY, _("Company:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCompany->Wrap( -1 );
	fgSizer2->Add( m_staticTextCompany, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextCompany = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextCompany->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextCompany, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_CompanyExport = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_CompanyExport, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextComment1 = new wxStaticText( this, wxID_ANY, _("Comment1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment1->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment1, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment1->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment1, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_Comment1Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment1Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextComment2 = new wxStaticText( this, wxID_ANY, _("Comment2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment2->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment2, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment2->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment2, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_Comment2Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment2Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextComment3 = new wxStaticText( this, wxID_ANY, _("Comment3:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment3->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment3 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment3->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_Comment3Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment3Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextComment4 = new wxStaticText( this, wxID_ANY, _("Comment4:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment4->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment4, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment4 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment4->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment4, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_Comment4Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment4Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextComment5 = new wxStaticText( this, wxID_ANY, _("Comment5:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment5->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment5, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment5 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment5->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment5, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Comment5Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment5Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextComment6 = new wxStaticText( this, wxID_ANY, _("Comment6:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment6->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment6, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment6 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment6->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment6, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Comment6Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment6Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextComment7 = new wxStaticText( this, wxID_ANY, _("Comment7:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment7->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment7, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment7 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment7->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment7, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Comment7Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment7Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextComment8 = new wxStaticText( this, wxID_ANY, _("Comment8:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment8->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment8, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment8 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment8->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment8, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Comment8Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment8Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextComment9 = new wxStaticText( this, wxID_ANY, _("Comment9:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment9->Wrap( -1 );
	fgSizer2->Add( m_staticTextComment9, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextComment9 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextComment9->SetMinSize( wxSize( 360,-1 ) );

	fgSizer2->Add( m_TextComment9, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Comment9Export = new wxCheckBox( this, wxID_ANY, _("Export to other sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Comment9Export, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( fgSizer2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bUpperSizerH->Add( bSizerRight, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bUpperSizerH, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_paperSizeComboBox->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPaperSizeChoice ), NULL, this );
	m_orientationComboBox->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPageOrientationChoice ), NULL, this );
	m_userSizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeYTextUpdated ), NULL, this );
	m_userSizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeXTextUpdated ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnWksFileSelection ), NULL, this );
	m_TextDate->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnDateTextUpdated ), NULL, this );
	m_ApplyDate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnDateApplyClick ), NULL, this );
	m_TextRevision->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnRevisionTextUpdated ), NULL, this );
	m_TextTitle->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnTitleTextUpdated ), NULL, this );
	m_TitleExport->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCheckboxTitleClick ), NULL, this );
	m_TextCompany->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCompanyTextUpdated ), NULL, this );
	m_TextComment1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment1TextUpdated ), NULL, this );
	m_TextComment2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment2TextUpdated ), NULL, this );
	m_TextComment3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment3TextUpdated ), NULL, this );
	m_TextComment4->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment4TextUpdated ), NULL, this );
	m_TextComment5->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment5TextUpdated ), NULL, this );
	m_TextComment6->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment6TextUpdated ), NULL, this );
	m_TextComment7->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment7TextUpdated ), NULL, this );
	m_TextComment8->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment8TextUpdated ), NULL, this );
	m_TextComment9->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment9TextUpdated ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PAGES_SETTINGS_BASE::~DIALOG_PAGES_SETTINGS_BASE()
{
	// Disconnect Events
	m_paperSizeComboBox->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPaperSizeChoice ), NULL, this );
	m_orientationComboBox->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnPageOrientationChoice ), NULL, this );
	m_userSizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeYTextUpdated ), NULL, this );
	m_userSizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnUserPageSizeXTextUpdated ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnWksFileSelection ), NULL, this );
	m_TextDate->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnDateTextUpdated ), NULL, this );
	m_ApplyDate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnDateApplyClick ), NULL, this );
	m_TextRevision->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnRevisionTextUpdated ), NULL, this );
	m_TextTitle->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnTitleTextUpdated ), NULL, this );
	m_TitleExport->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCheckboxTitleClick ), NULL, this );
	m_TextCompany->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnCompanyTextUpdated ), NULL, this );
	m_TextComment1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment1TextUpdated ), NULL, this );
	m_TextComment2->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment2TextUpdated ), NULL, this );
	m_TextComment3->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment3TextUpdated ), NULL, this );
	m_TextComment4->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment4TextUpdated ), NULL, this );
	m_TextComment5->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment5TextUpdated ), NULL, this );
	m_TextComment6->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment6TextUpdated ), NULL, this );
	m_TextComment7->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment7TextUpdated ), NULL, this );
	m_TextComment8->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment8TextUpdated ), NULL, this );
	m_TextComment9->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnComment9TextUpdated ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGES_SETTINGS_BASE::OnOkClick ), NULL, this );

}
