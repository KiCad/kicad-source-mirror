///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
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


	bMargins->Add( fgTextStyleSizer, 0, wxEXPAND, 5 );


	bMargins->Add( 0, 20, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbFontSizer;
	gbFontSizer = new wxGridBagSizer( 7, 5 );
	gbFontSizer->SetFlexibleDirection( wxBOTH );
	gbFontSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbFontSizer->SetEmptyCellSize( wxSize( -1,5 ) );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbFontSizer->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbFontSizer->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_styleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_styleLabel->Wrap( -1 );
	gbFontSizer->Add( m_styleLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_bold = new wxCheckBox( this, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer14->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new wxCheckBox( this, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer14->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 45 );


	gbFontSizer->Add( bSizer14, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );


	gbFontSizer->AddGrowableCol( 1 );

	bMargins->Add( gbFontSizer, 0, wxEXPAND|wxBOTTOM, 1 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,8 ) );

	m_SizeXLabel = new wxStaticText( this, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXLabel->Wrap( -1 );
	m_SizeXLabel->SetToolTip( _("Text width") );

	gbSizer1->Add( m_SizeXLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

	m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeXCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeXUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SizeYLabel = new wxStaticText( this, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYLabel->Wrap( -1 );
	m_SizeYLabel->SetToolTip( _("Text height") );

	gbSizer1->Add( m_SizeYLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

	m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeYCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeYUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	m_ThicknessLabel->SetToolTip( _("Text thickness") );

	gbSizer1->Add( m_ThicknessLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_ThicknessCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ThicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessUnits->Wrap( -1 );
	gbSizer1->Add( m_ThicknessUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( gbSizer1, 0, wxEXPAND|wxTOP, 5 );


	bMargins->Add( 0, 20, 0, wxEXPAND, 5 );

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


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxALL, 10 );

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
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onThickness ), NULL, this );
	m_editTable->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );
}

DIALOG_TABLECELL_PROPERTIES_BASE::~DIALOG_TABLECELL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onThickness ), NULL, this );
	m_editTable->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );

}
