///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_infobar.h"

#include "dialog_lib_new_symbol_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LIB_NEW_SYMBOL_BASE, DIALOG_SHIM )
	EVT_CHECKBOX( wxID_ANY, DIALOG_LIB_NEW_SYMBOL_BASE::_wxFB_onPowerCheckBox )
	EVT_CHECKBOX( wxID_ANY, DIALOG_LIB_NEW_SYMBOL_BASE::_wxFB_onCheckTransferUserFields )
END_EVENT_TABLE()

DIALOG_LIB_NEW_SYMBOL_BASE::DIALOG_LIB_NEW_SYMBOL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bSizerMain->Add( m_infoBar, 0, wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 2, 6, 6 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Symbol name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	m_staticTextName->SetToolTip( _("The symbol name in library and also the default\nsymbol value when loaded in the schematic.") );

	fgSizer31->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer31->Add( m_textName, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Derive from existing symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer31->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_comboInheritanceSelect = new SYMBOL_FILTER_COMBOBOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_comboInheritanceSelect, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextDes = new wxStaticText( this, wxID_ANY, _("Default reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDes->Wrap( -1 );
	fgSizer31->Add( m_staticTextDes, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textReference = new wxTextCtrl( this, wxID_ANY, _("U"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_textReference, 0, wxEXPAND, 5 );

	m_staticTextUnits = new wxStaticText( this, wxID_ANY, _("Number of units per package:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnits->Wrap( -1 );
	fgSizer31->Add( m_staticTextUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinPartCount = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64, 0 );
	fgSizer31->Add( m_spinPartCount, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bSizerTop->Add( fgSizer31, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );

	m_checkUnitsInterchangeable = new wxCheckBox( this, wxID_ANY, _("All units are interchangeable"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkUnitsInterchangeable->SetToolTip( _("Check this option when all symbol units are identical except\nfor pin numbers.") );

	bSizer17->Add( m_checkUnitsInterchangeable, 0, wxRIGHT|wxLEFT, 5 );

	m_checkHasAlternateBodyStyle = new wxCheckBox( this, wxID_ANY, _("Create symbol with alternate body style (De Morgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkHasAlternateBodyStyle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_checkIsPowerSymbol = new wxCheckBox( this, wxID_ANY, _("Create symbol as power symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkIsPowerSymbol, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_excludeFromBomCheckBox = new wxCheckBox( this, wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_excludeFromBomCheckBox, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_excludeFromBoardCheckBox = new wxCheckBox( this, wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_excludeFromBoardCheckBox, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_checkKeepDatasheet = new wxCheckBox( this, wxID_ANY, _("Keep linked datasheet"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkKeepDatasheet, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_checkKeepFootprint = new wxCheckBox( this, wxID_ANY, _("Keep linked footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkKeepFootprint, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_checkTransferUserFields = new wxCheckBox( this, wxID_ANY, _("Transfer user-defined fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkTransferUserFields, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_checkKeepContentUserFields = new wxCheckBox( this, wxID_ANY, _("Keep content of user-defined fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkKeepContentUserFields, 0, wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerTop->Add( bSizer17, 0, wxBOTTOM|wxEXPAND, 5 );


	bSizerMain->Add( bSizerTop, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 3, 6, 6 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticPinTextPositionLabel = new wxStaticText( this, wxID_ANY, _("Pin name position offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticPinTextPositionLabel->Wrap( -1 );
	fgSizer4->Add( m_staticPinTextPositionLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textPinTextPosition = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_textPinTextPosition, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticPinTextPositionUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticPinTextPositionUnits->Wrap( -1 );
	fgSizer4->Add( m_staticPinTextPositionUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerBottom->Add( fgSizer4, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );

	m_checkShowPinNumber = new wxCheckBox( this, wxID_ANY, _("Show pin number text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNumber->SetValue(true);
	bSizer19->Add( m_checkShowPinNumber, 0, wxRIGHT|wxLEFT, 5 );

	m_checkShowPinName = new wxCheckBox( this, wxID_ANY, _("Show pin name text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinName->SetValue(true);
	bSizer19->Add( m_checkShowPinName, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_checkShowPinNameInside = new wxCheckBox( this, wxID_ANY, _("Pin name inside"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNameInside->SetValue(true);
	bSizer19->Add( m_checkShowPinNameInside, 0, wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerBottom->Add( bSizer19, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 1, wxALL|wxEXPAND, 5 );

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
}

DIALOG_LIB_NEW_SYMBOL_BASE::~DIALOG_LIB_NEW_SYMBOL_BASE()
{
}
