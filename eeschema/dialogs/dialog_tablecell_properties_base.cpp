///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"
#include "widgets/wx_infobar.h"

#include "dialog_tablecell_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TABLECELL_PROPERTIES_BASE::DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bMainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgTextStyleSizer;
	fgTextStyleSizer = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgTextStyleSizer->SetFlexibleDirection( wxBOTH );
	fgTextStyleSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* hAlignLabel;
	hAlignLabel = new wxStaticText( this, wxID_ANY, _("Horizontal alignment:"), wxDefaultPosition, wxDefaultSize, 0 );
	hAlignLabel->Wrap( -1 );
	hAlignLabel->SetToolTip( _("Horizontal alignment") );

	fgTextStyleSizer->Add( hAlignLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* hAlignButtons;
	hAlignButtons = new wxBoxSizer( wxHORIZONTAL );

	m_hAlignLeft = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignLeft->SetToolTip( _("Align left") );

	hAlignButtons->Add( m_hAlignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignCenter->SetToolTip( _("Align horizontal center") );

	hAlignButtons->Add( m_hAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignRight = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignRight->SetToolTip( _("Align right") );

	hAlignButtons->Add( m_hAlignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgTextStyleSizer->Add( hAlignButtons, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	vAlignLabel = new wxStaticText( this, wxID_ANY, _("Vertical alignment:"), wxDefaultPosition, wxDefaultSize, 0 );
	vAlignLabel->Wrap( -1 );
	vAlignLabel->SetToolTip( _("Vertical alignment") );

	fgTextStyleSizer->Add( vAlignLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* vAlignButtons;
	vAlignButtons = new wxBoxSizer( wxHORIZONTAL );

	m_vAlignTop = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignTop->SetToolTip( _("Align top") );

	vAlignButtons->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignCenter->SetToolTip( _("Align vertical center") );

	vAlignButtons->Add( m_vAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignBottom = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignBottom->SetToolTip( _("Align bottom") );

	vAlignButtons->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgTextStyleSizer->Add( vAlignButtons, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( fgTextStyleSizer, 0, wxEXPAND|wxBOTTOM, 6 );


	bMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbFontSizer;
	gbFontSizer = new wxGridBagSizer( 6, 5 );
	gbFontSizer->SetFlexibleDirection( wxBOTH );
	gbFontSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbFontSizer->SetEmptyCellSize( wxSize( -1,5 ) );

	m_styleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_styleLabel->Wrap( -1 );
	gbFontSizer->Add( m_styleLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_bold = new wxCheckBox( this, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer14->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new wxCheckBox( this, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer14->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 45 );


	gbFontSizer->Add( bSizer14, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbFontSizer->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbFontSizer->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbFontSizer->Add( m_textSizeLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer15->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizer15->Add( m_textSizeUnits, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );


	gbFontSizer->Add( bSizer15, wxGBPosition( 1, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbFontSizer->AddGrowableCol( 1 );

	bMargins->Add( gbFontSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgColorSizer;
	fgColorSizer = new wxFlexGridSizer( 0, 2, 4, 5 );
	fgColorSizer->SetFlexibleDirection( wxBOTH );
	fgColorSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textColorLabel = new wxStaticText( this, wxID_ANY, _("Text color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textColorLabel->Wrap( -1 );
	fgColorSizer->Add( m_textColorLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textColorBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* textColorPopupPanel;
	textColorPopupPanel = new wxPanel( m_textColorBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );

	wxString m_textColorPopupChoices[] = { _("-- mixed values --"), _("Set Color...") };
	int m_textColorPopupNChoices = sizeof( m_textColorPopupChoices ) / sizeof( wxString );
	m_textColorPopup = new wxChoice( textColorPopupPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_textColorPopupNChoices, m_textColorPopupChoices, 0 );
	m_textColorPopup->SetSelection( 0 );
	bSizer20->Add( m_textColorPopup, 0, 0, 5 );


	textColorPopupPanel->SetSizer( bSizer20 );
	textColorPopupPanel->Layout();
	bSizer20->Fit( textColorPopupPanel );
	m_textColorBook->AddPage( textColorPopupPanel, _("a page"), false );
	textColorSwatchPanel = new wxPanel( m_textColorBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer222;
	bSizer222 = new wxBoxSizer( wxHORIZONTAL );

	m_panelTextColor = new wxPanel( textColorSwatchPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer221;
	bSizer221 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelTextColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer221->Add( m_textColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelTextColor->SetSizer( bSizer221 );
	m_panelTextColor->Layout();
	bSizer221->Fit( m_panelTextColor );
	bSizer222->Add( m_panelTextColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	textColorSwatchPanel->SetSizer( bSizer222 );
	textColorSwatchPanel->Layout();
	bSizer222->Fit( textColorSwatchPanel );
	m_textColorBook->AddPage( textColorSwatchPanel, _("a page"), false );

	fgColorSizer->Add( m_textColorBook, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_fillColorLabel = new wxStaticText( this, wxID_ANY, _("Background fill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	fgColorSizer->Add( m_fillColorLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_fillColorBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* fillColorPopupPanel;
	fillColorPopupPanel = new wxPanel( m_fillColorBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer211;
	bSizer211 = new wxBoxSizer( wxVERTICAL );

	wxString m_fillColorPopupChoices[] = { _("-- mixed values --"), _("Set Color...") };
	int m_fillColorPopupNChoices = sizeof( m_fillColorPopupChoices ) / sizeof( wxString );
	m_fillColorPopup = new wxChoice( fillColorPopupPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fillColorPopupNChoices, m_fillColorPopupChoices, 0 );
	m_fillColorPopup->SetSelection( 0 );
	bSizer211->Add( m_fillColorPopup, 0, 0, 5 );


	fillColorPopupPanel->SetSizer( bSizer211 );
	fillColorPopupPanel->Layout();
	bSizer211->Fit( fillColorPopupPanel );
	m_fillColorBook->AddPage( fillColorPopupPanel, _("a page"), false );
	fillColorSwatchPanel = new wxPanel( m_fillColorBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxHORIZONTAL );

	m_panelFillColor = new wxPanel( fillColorSwatchPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panelFillColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelFillColor->SetSizer( bSizer22 );
	m_panelFillColor->Layout();
	bSizer22->Fit( m_panelFillColor );
	bSizer23->Add( m_panelFillColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fillColorSwatchPanel->SetSizer( bSizer23 );
	fillColorSwatchPanel->Layout();
	bSizer23->Fit( fillColorSwatchPanel );
	m_fillColorBook->AddPage( fillColorSwatchPanel, _("a page"), false );

	fgColorSizer->Add( m_fillColorBook, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( fgColorSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 20 );


	bMargins->Add( 0, 5, 0, wxEXPAND, 5 );

	wxGridSizer* gMarginsSizer;
	gMarginsSizer = new wxGridSizer( 0, 3, 4, 2 );

	wxStaticText* marginsLabel;
	marginsLabel = new wxStaticText( this, wxID_ANY, _("Cell margins:"), wxDefaultPosition, wxDefaultSize, 0 );
	marginsLabel->Wrap( -1 );
	gMarginsSizer->Add( marginsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_marginTopCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gMarginsSizer->Add( m_marginTopCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_marginTopUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginTopUnits->Wrap( -1 );
	gMarginsSizer->Add( m_marginTopUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 2 );

	m_marginLeftCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gMarginsSizer->Add( m_marginLeftCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gMarginsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_marginRightCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gMarginsSizer->Add( m_marginRightCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gMarginsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_marginBottomCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gMarginsSizer->Add( m_marginBottomCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( gMarginsSizer, 0, wxEXPAND, 5 );


	bSizer13->Add( bMargins, 1, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizer13, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );

	m_editTable = new wxButton( this, wxID_ANY, _("Edit Table..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_editTable->SetToolTip( _("Edit table properties and cell contents") );

	bButtons->Add( m_editTable, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );


	bButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bButtons->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bButtons, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_textColorPopup->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextColorPopup ), NULL, this );
	m_fillColorPopup->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onFillColorPopup ), NULL, this );
	m_editTable->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );
}

DIALOG_TABLECELL_PROPERTIES_BASE::~DIALOG_TABLECELL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_textColorPopup->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextColorPopup ), NULL, this );
	m_fillColorPopup->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onFillColorPopup ), NULL, this );
	m_editTable->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );

}
