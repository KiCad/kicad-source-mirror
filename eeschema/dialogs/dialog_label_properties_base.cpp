///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/filter_combobox.h"
#include "widgets/font_choice.h"
#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_label_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LABEL_PROPERTIES_BASE::DIALOG_LABEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_textEntrySizer = new wxFlexGridSizer( 5, 2, 1, 3 );
	m_textEntrySizer->AddGrowableCol( 1 );
	m_textEntrySizer->AddGrowableRow( 1 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelSingleLine = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSingleLine->Wrap( -1 );
	m_labelSingleLine->SetToolTip( _("Enter the text to be used within the schematic") );

	m_textEntrySizer->Add( m_labelSingleLine, 0, wxALIGN_CENTER_VERTICAL, 2 );

	m_valueSingleLine = new wxTextCtrl( this, wxID_VALUESINGLE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RICH );
	m_textEntrySizer->Add( m_valueSingleLine, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 2 );

	m_labelCombo = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCombo->Wrap( -1 );
	m_textEntrySizer->Add( m_labelCombo, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_valueCombo = new FILTER_COMBOBOX( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_textEntrySizer->Add( m_valueCombo, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_labelMultiLine = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMultiLine->Wrap( -1 );
	m_labelMultiLine->SetToolTip( _("Enter the text to be used within the schematic") );

	m_textEntrySizer->Add( m_labelMultiLine, 0, wxALIGN_CENTER_VERTICAL, 2 );

	m_valueMultiLine = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_textEntrySizer->Add( m_valueMultiLine, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );


	m_textEntrySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_cbMultiLine = new wxCheckBox( this, wxID_ANY, _("Multiple label input"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_cbMultiLine, 0, 0, 5 );


	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );

	m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_RIGHT|wxHL_CONTEXTMENU );
	m_syntaxHelp->SetToolTip( _("Show syntax help window") );

	bSizer7->Add( m_syntaxHelp, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_textEntrySizer->Add( bSizer7, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_textEntrySizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_grid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 4, 14 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 84 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 48 );
	m_grid->SetColSize( 4, 70 );
	m_grid->SetColSize( 5, 70 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 48 );
	m_grid->SetColSize( 8, 80 );
	m_grid->SetColSize( 9, 48 );
	m_grid->SetColSize( 10, 80 );
	m_grid->SetColSize( 11, 80 );
	m_grid->SetColSize( 12, 100 );
	m_grid->SetColSize( 13, 48 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelValue( 2, _("Show") );
	m_grid->SetColLabelValue( 3, _("Show Name") );
	m_grid->SetColLabelValue( 4, _("H Align") );
	m_grid->SetColLabelValue( 5, _("V Align") );
	m_grid->SetColLabelValue( 6, _("Italic") );
	m_grid->SetColLabelValue( 7, _("Bold") );
	m_grid->SetColLabelValue( 8, _("Text Size") );
	m_grid->SetColLabelValue( 9, _("Orientation") );
	m_grid->SetColLabelValue( 10, _("X Position") );
	m_grid->SetColLabelValue( 11, _("Y Position") );
	m_grid->SetColLabelValue( 12, _("Font") );
	m_grid->SetColLabelValue( 13, _("Color") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( -1,100 ) );

	sbFields->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );

	bButtonSize->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonSize->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonSize->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbFields->Add( bButtonSize, 0, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( sbFields, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* optionsSizer;
	optionsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_shapeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Shape") ), wxVERTICAL );

	m_input = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Input"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_input, 0, wxBOTTOM|wxRIGHT, 2 );

	m_output = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Output"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_output, 0, wxBOTTOM|wxRIGHT, 3 );

	m_bidirectional = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Bidirectional"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_bidirectional, 0, wxBOTTOM|wxRIGHT, 3 );

	m_triState = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Tri-state"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_triState, 0, wxBOTTOM|wxRIGHT, 3 );

	m_passive = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Passive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_passive, 0, wxBOTTOM|wxRIGHT, 3 );

	m_dot = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Dot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_dot, 0, wxBOTTOM|wxRIGHT, 3 );

	m_circle = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Circle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_circle, 0, wxBOTTOM|wxRIGHT, 3 );

	m_diamond = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Diamond"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_diamond, 0, wxBOTTOM|wxRIGHT, 3 );

	m_rectangle = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Rectangle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_rectangle, 0, wxBOTTOM|wxRIGHT, 3 );


	optionsSizer->Add( m_shapeSizer, 0, wxBOTTOM|wxEXPAND|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* formatting;
	formatting = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Formatting") ), wxVERTICAL );

	m_formattingGB = new wxGridBagSizer( 3, 1 );
	m_formattingGB->SetFlexibleDirection( wxBOTH );
	m_formattingGB->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_fontLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	m_formattingGB->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( formatting->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	m_formattingGB->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_iconBar = new wxBoxSizer( wxHORIZONTAL );

	m_separator1 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	m_iconBar->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bold = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	m_iconBar->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	m_iconBar->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	m_iconBar->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin0 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_iconBar->Add( m_spin0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin1 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_iconBar->Add( m_spin1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin2 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_iconBar->Add( m_spin2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin3 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_iconBar->Add( m_spin3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_autoRotate = new wxCheckBox( formatting->GetStaticBox(), wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_iconBar->Add( m_autoRotate, 0, wxALL, 5 );

	m_separator3 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	m_iconBar->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_formattingGB->Add( m_iconBar, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_textSizeLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	m_formattingGB->Add( m_textSizeLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 4 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( formatting->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizer71->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_textColorLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textColorLabel->Wrap( -1 );
	bSizer71->Add( m_textColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer71->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor1 = new wxPanel( formatting->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelBorderColor1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_textColorSwatch, 0, 0, 5 );


	m_panelBorderColor1->SetSizer( bSizer22 );
	m_panelBorderColor1->Layout();
	bSizer22->Fit( m_panelBorderColor1 );
	bSizer71->Add( m_panelBorderColor1, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_formattingGB->Add( bSizer71, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );


	formatting->Add( m_formattingGB, 1, wxBOTTOM|wxEXPAND, 5 );


	optionsSizer->Add( formatting, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( optionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnUpdateUI ) );
	m_valueSingleLine->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnValueCharHook ), NULL, this );
	m_valueSingleLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueCombo->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnCBValueCharHook ), NULL, this );
	m_valueCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueMultiLine->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnValueCharHook ), NULL, this );
	m_cbMultiLine->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::onMultiLabelCheck ), NULL, this );
	m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnDeleteField ), NULL, this );
}

DIALOG_LABEL_PROPERTIES_BASE::~DIALOG_LABEL_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnUpdateUI ) );
	m_valueSingleLine->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnValueCharHook ), NULL, this );
	m_valueSingleLine->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueCombo->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnCBValueCharHook ), NULL, this );
	m_valueCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueMultiLine->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnValueCharHook ), NULL, this );
	m_cbMultiLine->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::onMultiLabelCheck ), NULL, this );
	m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_PROPERTIES_BASE::OnDeleteField ), NULL, this );

}
