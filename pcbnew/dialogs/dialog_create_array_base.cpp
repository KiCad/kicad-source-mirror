///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_create_array_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CREATE_ARRAY_BASE::DIALOG_CREATE_ARRAY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_gridTypeNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_gridPanel = new wxPanel( m_gridTypeNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerGridArray;
	bSizerGridArray = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerGridLeft;
	bSizerGridLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerGridSize;
	sbSizerGridSize = new wxStaticBoxSizer( new wxStaticBox( m_gridPanel, wxID_ANY, _("Grid Array Size") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerGridSize;
	fgSizerGridSize = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizerGridSize->AddGrowableCol( 1 );
	fgSizerGridSize->SetFlexibleDirection( wxBOTH );
	fgSizerGridSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelNx = new wxStaticText( sbSizerGridSize->GetStaticBox(), wxID_ANY, _("Horizontal count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelNx->Wrap( -1 );
	m_labelNx->SetToolTip( _("Number of columns") );

	fgSizerGridSize->Add( m_labelNx, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_entryNx = new TEXT_CTRL_EVAL( sbSizerGridSize->GetStaticBox(), wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryNx->SetMinSize( wxSize( 60,-1 ) );

	fgSizerGridSize->Add( m_entryNx, 0, wxEXPAND, 5 );

	m_labelNy = new wxStaticText( sbSizerGridSize->GetStaticBox(), wxID_ANY, _("Vertical count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelNy->Wrap( -1 );
	m_labelNy->SetToolTip( _("Number of rows") );

	fgSizerGridSize->Add( m_labelNy, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_entryNy = new TEXT_CTRL_EVAL( sbSizerGridSize->GetStaticBox(), wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryNy->SetToolTip( _("Number of rows") );
	m_entryNy->SetMinSize( wxSize( 60,-1 ) );

	fgSizerGridSize->Add( m_entryNy, 0, wxEXPAND, 5 );


	sbSizerGridSize->Add( fgSizerGridSize, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerGridLeft->Add( sbSizerGridSize, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxStaticBoxSizer* sbSizerItemsSpacing;
	sbSizerItemsSpacing = new wxStaticBoxSizer( new wxStaticBox( m_gridPanel, wxID_ANY, _("Items Spacing") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerItemSpacing;
	fgSizerItemSpacing = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerItemSpacing->AddGrowableCol( 1 );
	fgSizerItemSpacing->SetFlexibleDirection( wxBOTH );
	fgSizerItemSpacing->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelDx = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("Horizontal spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDx->Wrap( -1 );
	m_labelDx->SetToolTip( _("Distance between columns") );

	fgSizerItemSpacing->Add( m_labelDx, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryDx = new wxTextCtrl( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("2.54"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerItemSpacing->Add( m_entryDx, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_unitLabelDx = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelDx->Wrap( -1 );
	fgSizerItemSpacing->Add( m_unitLabelDx, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelDy = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("Vertical spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDy->Wrap( -1 );
	m_labelDy->SetToolTip( _("Distance between rows") );

	fgSizerItemSpacing->Add( m_labelDy, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryDy = new wxTextCtrl( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("2.54"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerItemSpacing->Add( m_entryDy, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_unitLabelDy = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelDy->Wrap( -1 );
	fgSizerItemSpacing->Add( m_unitLabelDy, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerItemSpacing->Add( 0, 5, 1, wxEXPAND, 5 );


	fgSizerItemSpacing->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerItemSpacing->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelOffsetX = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("Horizontal offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOffsetX->Wrap( -1 );
	m_labelOffsetX->SetToolTip( _("Offset added to the next row position.") );

	fgSizerItemSpacing->Add( m_labelOffsetX, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryOffsetX = new wxTextCtrl( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerItemSpacing->Add( m_entryOffsetX, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_unitLabelOffsetX = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelOffsetX->Wrap( -1 );
	fgSizerItemSpacing->Add( m_unitLabelOffsetX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelOffsetY = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("Vertical offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOffsetY->Wrap( -1 );
	m_labelOffsetY->SetToolTip( _("Offset added to the next column position") );

	fgSizerItemSpacing->Add( m_labelOffsetY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryOffsetY = new wxTextCtrl( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerItemSpacing->Add( m_entryOffsetY, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_unitLabelOffsetY = new wxStaticText( sbSizerItemsSpacing->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelOffsetY->Wrap( -1 );
	fgSizerItemSpacing->Add( m_unitLabelOffsetY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerItemsSpacing->Add( fgSizerItemSpacing, 1, wxEXPAND|wxBOTTOM, 5 );


	bSizerGridLeft->Add( sbSizerItemsSpacing, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbSizerStagger;
	sbSizerStagger = new wxStaticBoxSizer( new wxStaticBox( m_gridPanel, wxID_ANY, _("Stagger Settings") ), wxVERTICAL );

	wxBoxSizer* bSizerStaggerset;
	bSizerStaggerset = new wxBoxSizer( wxHORIZONTAL );

	m_labelStagger = new wxStaticText( sbSizerStagger->GetStaticBox(), wxID_ANY, _("Stagger:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelStagger->Wrap( -1 );
	m_labelStagger->SetToolTip( _("Value -1,  0 or 1 disable this option.") );

	bSizerStaggerset->Add( m_labelStagger, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryStagger = new TEXT_CTRL_EVAL( sbSizerStagger->GetStaticBox(), wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerStaggerset->Add( m_entryStagger, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbSizerStagger->Add( bSizerStaggerset, 1, wxEXPAND, 5 );

	m_staggerRows = new wxRadioButton( sbSizerStagger->GetStaticBox(), wxID_ANY, _("Rows"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbSizerStagger->Add( m_staggerRows, 0, wxALL, 5 );

	m_staggerCols = new wxRadioButton( sbSizerStagger->GetStaticBox(), wxID_ANY, _("Columns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staggerCols->SetValue( true );
	sbSizerStagger->Add( m_staggerCols, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerGridLeft->Add( sbSizerStagger, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbGridPosition;
	sbGridPosition = new wxStaticBoxSizer( new wxStaticBox( m_gridPanel, wxID_ANY, _("Grid Position") ), wxVERTICAL );

	m_rbItemsRemainInPlace = new wxRadioButton( sbGridPosition->GetStaticBox(), wxID_ANY, _("Source items remain in place"), wxDefaultPosition, wxDefaultSize, 0 );
	sbGridPosition->Add( m_rbItemsRemainInPlace, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_rbCentreOnSource = new wxRadioButton( sbGridPosition->GetStaticBox(), wxID_ANY, _("Centre on source items"), wxDefaultPosition, wxDefaultSize, 0 );
	sbGridPosition->Add( m_rbCentreOnSource, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerGridLeft->Add( sbGridPosition, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bSizerGridArray->Add( bSizerGridLeft, 1, wxEXPAND, 5 );

	m_gridPadNumberingPanel = new wxPanel( m_gridPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_gridPadNumberingPanel->Hide();

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	m_gridPadNumberingSizer = new wxBoxSizer( wxVERTICAL );

	m_cbRenumberPads = new wxCheckBox( m_gridPadNumberingPanel, wxID_ANY, _("Renumber pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRenumberPads->SetValue(true);
	m_gridPadNumberingSizer->Add( m_cbRenumberPads, 0, wxALL, 5 );

	wxString m_radioBoxGridNumberingAxisChoices[] = { _("Horizontal, then vertical"), _("Vertical, then horizontal") };
	int m_radioBoxGridNumberingAxisNChoices = sizeof( m_radioBoxGridNumberingAxisChoices ) / sizeof( wxString );
	m_radioBoxGridNumberingAxis = new wxRadioBox( m_gridPadNumberingPanel, wxID_ANY, _("Numbering Direction"), wxDefaultPosition, wxDefaultSize, m_radioBoxGridNumberingAxisNChoices, m_radioBoxGridNumberingAxisChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxGridNumberingAxis->SetSelection( 0 );
	m_gridPadNumberingSizer->Add( m_radioBoxGridNumberingAxis, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxGridReverseNumbering = new wxCheckBox( m_gridPadNumberingPanel, wxID_ANY, _("Reverse numbering on alternate rows/columns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridPadNumberingSizer->Add( m_checkBoxGridReverseNumbering, 0, wxALL, 5 );

	wxString m_rbGridStartNumberingOptChoices[] = { _("Use first free number"), _("From start value") };
	int m_rbGridStartNumberingOptNChoices = sizeof( m_rbGridStartNumberingOptChoices ) / sizeof( wxString );
	m_rbGridStartNumberingOpt = new wxRadioBox( m_gridPadNumberingPanel, wxID_ANY, _("Initial Pad Number"), wxDefaultPosition, wxDefaultSize, m_rbGridStartNumberingOptNChoices, m_rbGridStartNumberingOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbGridStartNumberingOpt->SetSelection( 1 );
	m_gridPadNumberingSizer->Add( m_rbGridStartNumberingOpt, 0, wxALL|wxEXPAND, 5 );

	wxString m_radioBoxGridNumberingSchemeChoices[] = { _("Continuous (1, 2, 3...)"), _("Coordinate (A1, A2, ... B1, ...)") };
	int m_radioBoxGridNumberingSchemeNChoices = sizeof( m_radioBoxGridNumberingSchemeChoices ) / sizeof( wxString );
	m_radioBoxGridNumberingScheme = new wxRadioBox( m_gridPadNumberingPanel, wxID_ANY, _("Pad Numbering Scheme"), wxDefaultPosition, wxDefaultSize, m_radioBoxGridNumberingSchemeNChoices, m_radioBoxGridNumberingSchemeChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxGridNumberingScheme->SetSelection( 1 );
	m_gridPadNumberingSizer->Add( m_radioBoxGridNumberingScheme, 0, wxALL|wxEXPAND, 5 );

	m_labelPriAxisNumbering = new wxStaticText( m_gridPadNumberingPanel, wxID_ANY, _("Primary axis numbering:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelPriAxisNumbering->Wrap( -1 );
	m_gridPadNumberingSizer->Add( m_labelPriAxisNumbering, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_choicePriAxisNumberingChoices;
	m_choicePriAxisNumbering = new wxChoice( m_gridPadNumberingPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePriAxisNumberingChoices, 0 );
	m_choicePriAxisNumbering->SetSelection( 0 );
	m_gridPadNumberingSizer->Add( m_choicePriAxisNumbering, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_labelSecAxisNumbering = new wxStaticText( m_gridPadNumberingPanel, wxID_ANY, _("Secondary axis numbering:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSecAxisNumbering->Wrap( -1 );
	m_labelSecAxisNumbering->Enable( false );

	m_gridPadNumberingSizer->Add( m_labelSecAxisNumbering, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceSecAxisNumberingChoices;
	m_choiceSecAxisNumbering = new wxChoice( m_gridPadNumberingPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSecAxisNumberingChoices, 0 );
	m_choiceSecAxisNumbering->SetSelection( 0 );
	m_choiceSecAxisNumbering->Enable( false );

	m_gridPadNumberingSizer->Add( m_choiceSecAxisNumbering, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelGridNumberingOffset = new wxStaticText( m_gridPadNumberingPanel, wxID_ANY, _("Pad numbering start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelGridNumberingOffset->Wrap( -1 );
	fgSizer1->Add( m_labelGridNumberingOffset, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_entryGridPriNumberingOffset = new wxTextCtrl( m_gridPadNumberingPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryGridPriNumberingOffset->SetMinSize( wxSize( 72,-1 ) );

	fgSizer1->Add( m_entryGridPriNumberingOffset, 0, wxALL, 5 );

	m_entryGridSecNumberingOffset = new wxTextCtrl( m_gridPadNumberingPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryGridSecNumberingOffset->SetMinSize( wxSize( 72,-1 ) );

	fgSizer1->Add( m_entryGridSecNumberingOffset, 0, wxALL, 5 );

	m_labelGridNumberingStep = new wxStaticText( m_gridPadNumberingPanel, wxID_ANY, _("Pad numbering increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelGridNumberingStep->Wrap( -1 );
	fgSizer1->Add( m_labelGridNumberingStep, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_entryGridPriNumberingStep = new wxTextCtrl( m_gridPadNumberingPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryGridPriNumberingStep->SetMinSize( wxSize( 72,-1 ) );

	fgSizer1->Add( m_entryGridPriNumberingStep, 0, wxALL, 5 );

	m_entryGridSecNumberingStep = new wxTextCtrl( m_gridPadNumberingPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryGridSecNumberingStep->SetMinSize( wxSize( 72,-1 ) );

	fgSizer1->Add( m_entryGridSecNumberingStep, 0, wxALL, 5 );


	m_gridPadNumberingSizer->Add( fgSizer1, 0, wxEXPAND, 5 );


	bSizer15->Add( m_gridPadNumberingSizer, 1, wxEXPAND|wxLEFT, 10 );


	m_gridPadNumberingPanel->SetSizer( bSizer15 );
	m_gridPadNumberingPanel->Layout();
	bSizer15->Fit( m_gridPadNumberingPanel );
	bSizerGridArray->Add( m_gridPadNumberingPanel, 1, wxEXPAND, 5 );


	m_gridPanel->SetSizer( bSizerGridArray );
	m_gridPanel->Layout();
	bSizerGridArray->Fit( m_gridPanel );
	m_gridTypeNotebook->AddPage( m_gridPanel, _("Grid Array"), true );
	m_circularPanel = new wxPanel( m_gridTypeNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerCircLeft;
	bSizerCircLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( m_circularPanel, wxID_ANY, _("Center position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerArrayPrms;
	fgSizerArrayPrms = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerArrayPrms->AddGrowableCol( 1 );
	fgSizerArrayPrms->SetFlexibleDirection( wxBOTH );
	fgSizerArrayPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelCentreX = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Center pos X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCentreX->Wrap( -1 );
	fgSizerArrayPrms->Add( m_labelCentreX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_entryCentreX = new wxTextCtrl( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerArrayPrms->Add( m_entryCentreX, 0, wxEXPAND|wxTOP, 5 );

	m_unitLabelCentreX = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCentreX->Wrap( -1 );
	fgSizerArrayPrms->Add( m_unitLabelCentreX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelCentreY = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Center pos Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCentreY->Wrap( -1 );
	fgSizerArrayPrms->Add( m_labelCentreY, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_entryCentreY = new wxTextCtrl( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerArrayPrms->Add( m_entryCentreY, 0, wxEXPAND, 5 );

	m_unitLabelCentreY = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCentreY->Wrap( -1 );
	fgSizerArrayPrms->Add( m_unitLabelCentreY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerArrayPrms->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerInfo->Add( fgSizerArrayPrms, 0, wxEXPAND|wxLEFT, 25 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_btnSelectCenterPoint = new wxButton( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Select Point..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_btnSelectCenterPoint, 1, wxALL, 5 );

	m_btnSelectCenterItem = new wxButton( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Select Item..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_btnSelectCenterItem, 1, wxALL, 5 );


	sbSizerInfo->Add( bSizer12, 1, wxEXPAND, 5 );


	bSizerCircLeft->Add( sbSizerInfo, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxStaticBoxSizer* sbSizerDupPrms;
	sbSizerDupPrms = new wxStaticBoxSizer( new wxStaticBox( m_circularPanel, wxID_ANY, _("Duplication Settings") ), wxVERTICAL );

	m_checkBoxFullCircle = new wxCheckBox( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("Full circle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxFullCircle->SetValue(true);
	sbSizerDupPrms->Add( m_checkBoxFullCircle, 0, wxALL, 5 );

	wxString m_rbCircDirectionChoices[] = { _("Clockwise"), _("Anti-clockwise") };
	int m_rbCircDirectionNChoices = sizeof( m_rbCircDirectionChoices ) / sizeof( wxString );
	m_rbCircDirection = new wxRadioBox( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("Direction"), wxDefaultPosition, wxDefaultSize, m_rbCircDirectionNChoices, m_rbCircDirectionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbCircDirection->SetSelection( 0 );
	sbSizerDupPrms->Add( m_rbCircDirection, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizerDupPrms;
	fgSizerDupPrms = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerDupPrms->AddGrowableCol( 1 );
	fgSizerDupPrms->SetFlexibleDirection( wxBOTH );
	fgSizerDupPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelCircAngle = new wxStaticText( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("Angle between items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircAngle->Wrap( -1 );
	fgSizerDupPrms->Add( m_labelCircAngle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryCircAngle = new wxTextCtrl( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("90"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryCircAngle->SetToolTip( _("Positive angles represent an anti-clockwise rotation. An angle of 0 will produce a full circle divided evenly into \"Count\" portions.") );

	fgSizerDupPrms->Add( m_entryCircAngle, 0, wxEXPAND, 5 );

	m_unitLabelCircAngle = new wxStaticText( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCircAngle->Wrap( -1 );
	fgSizerDupPrms->Add( m_unitLabelCircAngle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelCircCount = new wxStaticText( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("Item count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircCount->Wrap( -1 );
	fgSizerDupPrms->Add( m_labelCircCount, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryCircCount = new TEXT_CTRL_EVAL( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryCircCount->SetToolTip( _("How many items in the array.") );

	fgSizerDupPrms->Add( m_entryCircCount, 0, wxEXPAND, 5 );


	fgSizerDupPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelCircOffset = new wxStaticText( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("First item angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircOffset->Wrap( -1 );
	fgSizerDupPrms->Add( m_labelCircOffset, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_entryCircOffset = new TEXT_CTRL_EVAL( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryCircOffset->SetToolTip( _("Angle offset of the first item in the array") );

	fgSizerDupPrms->Add( m_entryCircOffset, 0, wxEXPAND, 5 );

	m_unitLabelCircOffset = new wxStaticText( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCircOffset->Wrap( -1 );
	fgSizerDupPrms->Add( m_unitLabelCircOffset, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerDupPrms->Add( fgSizerDupPrms, 0, wxBOTTOM|wxEXPAND, 5 );

	m_entryRotateItemsCb = new wxCheckBox( sbSizerDupPrms->GetStaticBox(), wxID_ANY, _("Rotate items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryRotateItemsCb->SetToolTip( _("Rotate the item as well as move it - multi-selections will be rotated together") );

	sbSizerDupPrms->Add( m_entryRotateItemsCb, 0, wxALL, 5 );


	bSizerCircLeft->Add( sbSizerDupPrms, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );


	bSizer4->Add( bSizerCircLeft, 1, wxEXPAND, 5 );

	m_circularPadNumberingPanel = new wxPanel( m_circularPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_circularPadNumberingPanel->Hide();

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );


	bSizer13->Add( 10, 0, 0, wxEXPAND, 5 );

	m_circPadNumberingSizer = new wxStaticBoxSizer( new wxStaticBox( m_circularPadNumberingPanel, wxID_ANY, _("Numbering Options") ), wxVERTICAL );

	wxString m_rbCircStartNumberingOptChoices[] = { _("Use first free number"), _("From start value") };
	int m_rbCircStartNumberingOptNChoices = sizeof( m_rbCircStartNumberingOptChoices ) / sizeof( wxString );
	m_rbCircStartNumberingOpt = new wxRadioBox( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("Initial Pad Number:"), wxDefaultPosition, wxDefaultSize, m_rbCircStartNumberingOptNChoices, m_rbCircStartNumberingOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbCircStartNumberingOpt->SetSelection( 0 );
	m_circPadNumberingSizer->Add( m_rbCircStartNumberingOpt, 0, wxALL|wxEXPAND, 5 );

	m_labelCircNumbering = new wxStaticText( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("Pad Numbering:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircNumbering->Wrap( -1 );
	m_circPadNumberingSizer->Add( m_labelCircNumbering, 0, wxALL, 5 );

	wxArrayString m_choiceCircNumberingChoices;
	m_choiceCircNumbering = new wxChoice( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceCircNumberingChoices, 0 );
	m_choiceCircNumbering->SetSelection( 0 );
	m_circPadNumberingSizer->Add( m_choiceCircNumbering, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->AddGrowableCol( 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelCircNumStart = new wxStaticText( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("Pad numbering start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircNumStart->Wrap( -1 );
	fgSizer2->Add( m_labelCircNumStart, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_entryCircNumberingStart = new wxTextCtrl( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_entryCircNumberingStart, 1, wxALL, 5 );

	m_labelCircNumStep = new wxStaticText( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("Pad numbering increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircNumStep->Wrap( -1 );
	fgSizer2->Add( m_labelCircNumStep, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_entryCircNumberingStep = new wxTextCtrl( m_circPadNumberingSizer->GetStaticBox(), wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_entryCircNumberingStep, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_circPadNumberingSizer->Add( fgSizer2, 0, wxEXPAND, 5 );


	bSizer13->Add( m_circPadNumberingSizer, 1, wxEXPAND, 5 );


	m_circularPadNumberingPanel->SetSizer( bSizer13 );
	m_circularPadNumberingPanel->Layout();
	bSizer13->Fit( m_circularPadNumberingPanel );
	bSizer4->Add( m_circularPadNumberingPanel, 0, wxEXPAND | wxALL, 5 );


	m_circularPanel->SetSizer( bSizer4 );
	m_circularPanel->Layout();
	bSizer4->Fit( m_circularPanel );
	m_gridTypeNotebook->AddPage( m_circularPanel, _("Circular Array"), false );

	bSizer7->Add( m_gridTypeNotebook, 1, wxALL|wxEXPAND, 10 );

	m_optionsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_itemSourcePanel = new wxPanel( m_optionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizerDuplicateOrArrange;
	sbSizerDuplicateOrArrange = new wxStaticBoxSizer( new wxStaticBox( m_itemSourcePanel, wxID_ANY, _("Item Source") ), wxVERTICAL );

	m_radioBtnDuplicateSelection = new wxRadioButton( sbSizerDuplicateOrArrange->GetStaticBox(), wxID_ANY, _("Duplicate selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnDuplicateSelection->SetValue( true );
	sbSizerDuplicateOrArrange->Add( m_radioBtnDuplicateSelection, 0, wxALL, 5 );

	m_radioBtnArrangeSelection = new wxRadioButton( sbSizerDuplicateOrArrange->GetStaticBox(), wxID_ANY, _("Arrange selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnArrangeSelection->SetToolTip( _("This can conflict with reference designators in the schematic that have not yet been synchronized with the board.") );

	sbSizerDuplicateOrArrange->Add( m_radioBtnArrangeSelection, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_itemSourcePanel->SetSizer( sbSizerDuplicateOrArrange );
	m_itemSourcePanel->Layout();
	sbSizerDuplicateOrArrange->Fit( m_itemSourcePanel );
	bSizer8->Add( m_itemSourcePanel, 0, wxALL|wxEXPAND, 5 );

	m_footprintReannotatePanel = new wxPanel( m_optionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizerFootprintAnnotation;
	sbSizerFootprintAnnotation = new wxStaticBoxSizer( new wxStaticBox( m_footprintReannotatePanel, wxID_ANY, _("Footprint Annotation") ), wxVERTICAL );

	m_radioBtnKeepRefs = new wxRadioButton( sbSizerFootprintAnnotation->GetStaticBox(), wxID_ANY, _("Keep existing reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerFootprintAnnotation->Add( m_radioBtnKeepRefs, 0, wxALL, 5 );

	m_radioBtnUniqueRefs = new wxRadioButton( sbSizerFootprintAnnotation->GetStaticBox(), wxID_ANY, _("Assign unique reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnUniqueRefs->SetValue( true );
	m_radioBtnUniqueRefs->SetToolTip( _("This can conflict with reference designators in the schematic that have not yet been synchronized with the board.") );

	sbSizerFootprintAnnotation->Add( m_radioBtnUniqueRefs, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_footprintReannotatePanel->SetSizer( sbSizerFootprintAnnotation );
	m_footprintReannotatePanel->Layout();
	sbSizerFootprintAnnotation->Fit( m_footprintReannotatePanel );
	bSizer8->Add( m_footprintReannotatePanel, 0, wxEXPAND | wxALL, 5 );


	m_optionsPanel->SetSizer( bSizer8 );
	m_optionsPanel->Layout();
	bSizer8->Fit( m_optionsPanel );
	bSizer7->Add( m_optionsPanel, 0, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bSizer7, 1, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CREATE_ARRAY_BASE::OnClose ) );
	m_entryNx->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryNy->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDx->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDy->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryStagger->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_cbRenumberPads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_rbGridStartNumberingOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBoxGridNumberingScheme->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_choicePriAxisNumbering->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_choiceSecAxisNumbering->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_entryCentreX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_btnSelectCenterPoint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnSelectCenterButton ), NULL, this );
	m_btnSelectCenterItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnSelectCenterButton ), NULL, this );
	m_checkBoxFullCircle->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircAngle->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircCount->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircOffset->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_rbCircStartNumberingOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_choiceCircNumbering->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_radioBtnDuplicateSelection->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBtnArrangeSelection->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
}

DIALOG_CREATE_ARRAY_BASE::~DIALOG_CREATE_ARRAY_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CREATE_ARRAY_BASE::OnClose ) );
	m_entryNx->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryNy->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDx->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDy->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryStagger->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_cbRenumberPads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_rbGridStartNumberingOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBoxGridNumberingScheme->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_choicePriAxisNumbering->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_choiceSecAxisNumbering->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_entryCentreX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_btnSelectCenterPoint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnSelectCenterButton ), NULL, this );
	m_btnSelectCenterItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnSelectCenterButton ), NULL, this );
	m_checkBoxFullCircle->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircAngle->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircCount->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircOffset->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_rbCircStartNumberingOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_choiceCircNumbering->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnAxisNumberingChange ), NULL, this );
	m_radioBtnDuplicateSelection->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBtnArrangeSelection->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );

}
