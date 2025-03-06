///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/bitmap_button.h"
#include "widgets/font_choice.h"

#include "dialog_dimension_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DIMENSION_PROPERTIES_BASE::DIALOG_DIMENSION_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_sizerLeader = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Leader Format") ), wxVERTICAL );

	wxGridBagSizer* gbSizerLeader;
	gbSizerLeader = new wxGridBagSizer( 0, 0 );
	gbSizerLeader->SetFlexibleDirection( wxBOTH );
	gbSizerLeader->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblLeaderValue = new wxStaticText( m_sizerLeader->GetStaticBox(), wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblLeaderValue->Wrap( -1 );
	gbSizerLeader->Add( m_lblLeaderValue, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_txtLeaderValue = new wxTextCtrl( m_sizerLeader->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerLeader->Add( m_txtLeaderValue, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbSizerLeader->Add( 60, 0, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_lblTextFrame = new wxStaticText( m_sizerLeader->GetStaticBox(), wxID_ANY, _("Text frame:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextFrame->Wrap( -1 );
	m_lblTextFrame->SetToolTip( _("Draw a shape around the leader text") );

	gbSizerLeader->Add( m_lblTextFrame, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_cbTextFrameChoices[] = { _("None"), _("Rectangle"), _("Circle") };
	int m_cbTextFrameNChoices = sizeof( m_cbTextFrameChoices ) / sizeof( wxString );
	m_cbTextFrame = new wxChoice( m_sizerLeader->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbTextFrameNChoices, m_cbTextFrameChoices, 0 );
	m_cbTextFrame->SetSelection( 0 );
	m_cbTextFrame->SetToolTip( _("Draw a shape around the leader text") );

	gbSizerLeader->Add( m_cbTextFrame, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_lblLeaderLayer = new wxStaticText( m_sizerLeader->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblLeaderLayer->Wrap( -1 );
	gbSizerLeader->Add( m_lblLeaderLayer, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbLeaderLayer = new PCB_LAYER_BOX_SELECTOR( m_sizerLeader->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizerLeader->Add( m_cbLeaderLayer, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbSizerLeader->AddGrowableCol( 1 );
	gbSizerLeader->AddGrowableCol( 4 );

	m_sizerLeader->Add( gbSizerLeader, 0, wxEXPAND, 5 );


	m_mainSizer->Add( m_sizerLeader, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sizerCenter = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimension Format") ), wxVERTICAL );

	wxGridBagSizer* gbSizerCenter;
	gbSizerCenter = new wxGridBagSizer( 0, 0 );
	gbSizerCenter->SetFlexibleDirection( wxBOTH );
	gbSizerCenter->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblCenterLayer = new wxStaticText( m_sizerCenter->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblCenterLayer->Wrap( -1 );
	gbSizerCenter->Add( m_lblCenterLayer, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbCenterLayer = new PCB_LAYER_BOX_SELECTOR( m_sizerCenter->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizerCenter->Add( m_cbCenterLayer, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	m_sizerCenter->Add( gbSizerCenter, 0, wxEXPAND, 5 );


	m_mainSizer->Add( m_sizerCenter, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sizerFormat = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimension Format") ), wxVERTICAL );

	wxGridBagSizer* gbSizerFormat;
	gbSizerFormat = new wxGridBagSizer( 3, 0 );
	gbSizerFormat->SetFlexibleDirection( wxBOTH );
	gbSizerFormat->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblValue = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblValue->Wrap( -1 );
	m_lblValue->SetToolTip( _("Measured value of this dimension") );

	gbSizerFormat->Add( m_lblValue, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtValue = new wxTextCtrl( m_sizerFormat->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtValue->Enable( false );
	m_txtValue->SetToolTip( _("Measured value of this dimension") );

	gbSizerFormat->Add( m_txtValue, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbOverrideValue = new wxCheckBox( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Override value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOverrideValue->SetToolTip( _("When checked, the actual measurement is ignored and any value can be entered") );

	gbSizerFormat->Add( m_cbOverrideValue, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	gbSizerFormat->Add( 40, 0, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_lblUnits = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUnits->Wrap( -1 );
	m_lblUnits->SetToolTip( _("Units of this dimension (\"automatic\" to follow the units selected in the editor)") );

	gbSizerFormat->Add( m_lblUnits, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_cbUnitsChoices[] = { _("Inches"), _("Mils"), _("Millimeters"), _("Automatic") };
	int m_cbUnitsNChoices = sizeof( m_cbUnitsChoices ) / sizeof( wxString );
	m_cbUnits = new wxChoice( m_sizerFormat->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbUnitsNChoices, m_cbUnitsChoices, 0 );
	m_cbUnits->SetSelection( 0 );
	m_cbUnits->SetToolTip( _("Units of this dimension (\"automatic\" to follow the units selected in the editor)") );

	gbSizerFormat->Add( m_cbUnits, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblPrefix = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPrefix->Wrap( -1 );
	m_lblPrefix->SetToolTip( _("Text to print before the dimension value") );

	gbSizerFormat->Add( m_lblPrefix, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtPrefix = new wxTextCtrl( m_sizerFormat->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtPrefix->SetToolTip( _("Text to print before the dimension value") );

	gbSizerFormat->Add( m_txtPrefix, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_txtUnitsFormat = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Units format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtUnitsFormat->Wrap( -1 );
	m_txtUnitsFormat->SetToolTip( _("Choose how to display the units") );

	gbSizerFormat->Add( m_txtUnitsFormat, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_cbUnitsFormatChoices[] = { _("1234"), _("1234 mm"), _("1234 (mm)") };
	int m_cbUnitsFormatNChoices = sizeof( m_cbUnitsFormatChoices ) / sizeof( wxString );
	m_cbUnitsFormat = new wxChoice( m_sizerFormat->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbUnitsFormatNChoices, m_cbUnitsFormatChoices, 0 );
	m_cbUnitsFormat->SetSelection( 1 );
	m_cbUnitsFormat->SetToolTip( _("Choose how to display the units") );

	gbSizerFormat->Add( m_cbUnitsFormat, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblSuffix = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Suffix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblSuffix->Wrap( -1 );
	m_lblSuffix->SetToolTip( _("Text to print after the dimension value") );

	gbSizerFormat->Add( m_lblSuffix, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtSuffix = new wxTextCtrl( m_sizerFormat->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtSuffix->SetToolTip( _("Text to print after the dimension value") );

	gbSizerFormat->Add( m_txtSuffix, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblPrecision = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPrecision->Wrap( -1 );
	m_lblPrecision->SetToolTip( _("Choose how many digits of precision to display") );

	gbSizerFormat->Add( m_lblPrecision, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_cbPrecisionChoices[] = { _("0"), _("0.0"), _("0.00"), _("0.000"), _("0.0000"), _("0.00000"), _("0.00 in / 0 mils / 0.0 mm"), _("0.000 / 0 / 0.00"), _("0.0000 / 0.0 / 0.000"), _("0.00000 / 0.00 / 0.0000") };
	int m_cbPrecisionNChoices = sizeof( m_cbPrecisionChoices ) / sizeof( wxString );
	m_cbPrecision = new wxChoice( m_sizerFormat->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbPrecisionNChoices, m_cbPrecisionChoices, 0 );
	m_cbPrecision->SetSelection( 0 );
	m_cbPrecision->SetToolTip( _("Choose how many digits of precision to display") );

	gbSizerFormat->Add( m_cbPrecision, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbLayer = new PCB_LAYER_BOX_SELECTOR( m_sizerFormat->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizerFormat->Add( m_cbLayer, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_lblLayer = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblLayer->Wrap( -1 );
	gbSizerFormat->Add( m_lblLayer, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_cbSuppressZeroes = new wxCheckBox( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Suppress trailing zeroes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSuppressZeroes->SetToolTip( _("When checked, \"0.100\" will be shown as \"0.1\" even if the precision setting is higher") );

	gbSizerFormat->Add( m_cbSuppressZeroes, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_lblPreview = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("Preview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPreview->Wrap( -1 );
	gbSizerFormat->Add( m_lblPreview, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_staticTextPreview = new wxStaticText( m_sizerFormat->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPreview->Wrap( -1 );
	gbSizerFormat->Add( m_staticTextPreview, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxEXPAND, 5 );


	gbSizerFormat->AddGrowableCol( 1 );
	gbSizerFormat->AddGrowableCol( 5 );

	m_sizerFormat->Add( gbSizerFormat, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_mainSizer->Add( m_sizerFormat, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sizerText = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimension Text") ), wxVERTICAL );

	wxGridBagSizer* gbSizerText;
	gbSizerText = new wxGridBagSizer( 3, 0 );
	gbSizerText->SetFlexibleDirection( wxBOTH );
	gbSizerText->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_fontLabel = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizerText->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_fontCtrlChoices[] = { _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( m_sizerText->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizerText->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bSizerButtonBar;
	bSizerButtonBar = new wxBoxSizer( wxHORIZONTAL );

	m_separator0 = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator0->Enable( false );

	bSizerButtonBar->Add( m_separator0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bold = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator1 = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	bSizerButtonBar->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignLeft = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignCenter = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignRight = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	bSizerButtonBar->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_mirrored = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_mirrored, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( m_sizerText->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	bSizerButtonBar->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizerText->Add( bSizerButtonBar, wxGBPosition( 0, 4 ), wxGBSpan( 1, 3 ), wxEXPAND|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextWidth = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextWidth->Wrap( -1 );
	m_lblTextWidth->SetToolTip( _("Text width") );

	gbSizerText->Add( m_lblTextWidth, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtTextWidth = new wxTextCtrl( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerText->Add( m_txtTextWidth, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextWidthUnits = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextWidthUnits->Wrap( -1 );
	gbSizerText->Add( m_lblTextWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	gbSizerText->Add( 40, 0, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_lblTextPosX = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPosX->Wrap( -1 );
	m_lblTextPosX->SetToolTip( _("Text pos X") );

	gbSizerText->Add( m_lblTextPosX, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtTextPosX = new wxTextCtrl( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerText->Add( m_txtTextPosX, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextPosXUnits = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPosXUnits->Wrap( -1 );
	gbSizerText->Add( m_lblTextPosXUnits, wxGBPosition( 1, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_lblTextHeight = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextHeight->Wrap( -1 );
	m_lblTextHeight->SetToolTip( _("Text height") );

	gbSizerText->Add( m_lblTextHeight, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtTextHeight = new wxTextCtrl( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerText->Add( m_txtTextHeight, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextHeightUnits = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextHeightUnits->Wrap( -1 );
	gbSizerText->Add( m_lblTextHeightUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_lblTextPosY = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPosY->Wrap( -1 );
	m_lblTextPosY->SetToolTip( _("Text pos Y") );

	gbSizerText->Add( m_lblTextPosY, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtTextPosY = new wxTextCtrl( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerText->Add( m_txtTextPosY, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextPosYUnits = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPosYUnits->Wrap( -1 );
	gbSizerText->Add( m_lblTextPosYUnits, wxGBPosition( 2, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_lblTextThickness = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextThickness->Wrap( -1 );
	m_lblTextThickness->SetToolTip( _("Text thickness") );

	gbSizerText->Add( m_lblTextThickness, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtTextThickness = new wxTextCtrl( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerText->Add( m_txtTextThickness, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextThicknessUnits = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextThicknessUnits->Wrap( -1 );
	gbSizerText->Add( m_lblTextThicknessUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lblTextOrientation = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextOrientation->Wrap( -1 );
	m_lblTextOrientation->SetToolTip( _("Text orientation") );

	gbSizerText->Add( m_lblTextOrientation, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_cbTextOrientation = new wxComboBox( m_sizerText->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_cbTextOrientation->Append( _("0.0") );
	m_cbTextOrientation->Append( _("90.0") );
	m_cbTextOrientation->Append( _("-90.0") );
	m_cbTextOrientation->Append( _("180.0") );
	gbSizerText->Add( m_cbTextOrientation, wxGBPosition( 3, 5 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_lblTextPositionMode = new wxStaticText( m_sizerText->GetStaticBox(), wxID_ANY, _("Position mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPositionMode->Wrap( -1 );
	gbSizerText->Add( m_lblTextPositionMode, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	wxString m_cbTextPositionModeChoices[] = { _("Outside"), _("Inline"), _("Manual") };
	int m_cbTextPositionModeNChoices = sizeof( m_cbTextPositionModeChoices ) / sizeof( wxString );
	m_cbTextPositionMode = new wxChoice( m_sizerText->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbTextPositionModeNChoices, m_cbTextPositionModeChoices, 0 );
	m_cbTextPositionMode->SetSelection( 0 );
	m_cbTextPositionMode->SetToolTip( _("Choose how to position the text relative to the dimension line") );

	gbSizerText->Add( m_cbTextPositionMode, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_cbKeepAligned = new wxCheckBox( m_sizerText->GetStaticBox(), wxID_ANY, _("Keep aligned with dimension"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbKeepAligned->SetToolTip( _("Automatically set the text orientation to match the dimension lines") );

	gbSizerText->Add( m_cbKeepAligned, wxGBPosition( 4, 4 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxTOP, 5 );


	gbSizerText->AddGrowableCol( 1 );
	gbSizerText->AddGrowableCol( 5 );

	m_sizerText->Add( gbSizerText, 0, wxBOTTOM|wxEXPAND|wxLEFT, 5 );


	m_mainSizer->Add( m_sizerText, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbSizerLine;
	sbSizerLine = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimension Line") ), wxVERTICAL );

	wxGridBagSizer* gbSizerLine;
	gbSizerLine = new wxGridBagSizer( 3, 0 );
	gbSizerLine->SetFlexibleDirection( wxBOTH );
	gbSizerLine->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblLineThickness = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblLineThickness->Wrap( -1 );
	m_lblLineThickness->SetToolTip( _("Thickness of the dimension lines") );

	gbSizerLine->Add( m_lblLineThickness, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtLineThickness = new wxTextCtrl( sbSizerLine->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerLine->Add( m_txtLineThickness, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lblLineThicknessUnits = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblLineThicknessUnits->Wrap( -1 );
	gbSizerLine->Add( m_lblLineThicknessUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	gbSizerLine->Add( 40, 0, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_lblArrowLength = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("Arrow length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblArrowLength->Wrap( -1 );
	gbSizerLine->Add( m_lblArrowLength, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtArrowLength = new wxTextCtrl( sbSizerLine->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizerLine->Add( m_txtArrowLength, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblArrowLengthUnits = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblArrowLengthUnits->Wrap( -1 );
	gbSizerLine->Add( m_lblArrowLengthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lblExtensionOffset = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("Extension line offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOffset->Wrap( -1 );
	m_lblExtensionOffset->SetToolTip( _("Gap between the measured points and the start of the extension lines") );

	gbSizerLine->Add( m_lblExtensionOffset, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_txtExtensionOffset = new wxTextCtrl( sbSizerLine->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtExtensionOffset->SetToolTip( _("Gap between the measured points and the start of the extension lines") );

	gbSizerLine->Add( m_txtExtensionOffset, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lblExtensionOffsetUnits = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOffsetUnits->Wrap( -1 );
	gbSizerLine->Add( m_lblExtensionOffsetUnits, wxGBPosition( 0, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lblExtensionOvershoot = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("Extension line overshoot:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOvershoot->Wrap( -1 );
	gbSizerLine->Add( m_lblExtensionOvershoot, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_txtExtensionOvershoot = new wxTextCtrl( sbSizerLine->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerLine->Add( m_txtExtensionOvershoot, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lblExtensionOvershootUnits = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOvershootUnits->Wrap( -1 );
	gbSizerLine->Add( m_lblExtensionOvershootUnits, wxGBPosition( 1, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lblArrowDirection = new wxStaticText( sbSizerLine->GetStaticBox(), wxID_ANY, _("Arrow direction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblArrowDirection->Wrap( -1 );
	gbSizerLine->Add( m_lblArrowDirection, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	wxString m_cbArrowDirectionChoices[] = { _("Inward"), _("Outward") };
	int m_cbArrowDirectionNChoices = sizeof( m_cbArrowDirectionChoices ) / sizeof( wxString );
	m_cbArrowDirection = new wxChoice( sbSizerLine->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbArrowDirectionNChoices, m_cbArrowDirectionChoices, 0 );
	m_cbArrowDirection->SetSelection( 0 );
	m_cbArrowDirection->SetToolTip( _("Chose Dimension Arrow Direction\nAutomatic: Determined based on text position.\nInward:    >-----<\nOutward: <----->") );

	gbSizerLine->Add( m_cbArrowDirection, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 5 );


	gbSizerLine->AddGrowableCol( 1 );
	gbSizerLine->AddGrowableCol( 5 );

	sbSizerLine->Add( gbSizerLine, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_mainSizer->Add( sbSizerLine, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnInitDlg ) );
	m_cbUnits->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnDimensionUnitsChange ), NULL, this );
	m_fontCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_alignLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignCenter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_txtTextWidth->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextPosX->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextHeight->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextPosY->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextThickness->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onThickness ), NULL, this );
	m_txtTextThickness->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_cbTextOrientation->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtLineThickness->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
}

DIALOG_DIMENSION_PROPERTIES_BASE::~DIALOG_DIMENSION_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnInitDlg ) );
	m_cbUnits->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnDimensionUnitsChange ), NULL, this );
	m_fontCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_alignLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignCenter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_txtTextWidth->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextPosX->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextHeight->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextPosY->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtTextThickness->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::onThickness ), NULL, this );
	m_txtTextThickness->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_cbTextOrientation->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_txtLineThickness->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_PROPERTIES_BASE::OnOkClick ), NULL, this );

}
