///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/bmpcbox.h"

#include "dialog_lib_edit_pin_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LIB_EDIT_PIN_BASE, wxDialog )
	EVT_CLOSE( DIALOG_LIB_EDIT_PIN_BASE::_wxFB_OnCloseDialog )
	EVT_CHECKBOX( wxID_ANY, DIALOG_LIB_EDIT_PIN_BASE::_wxFB_OnCBpartSelection )
	EVT_BUTTON( wxID_CANCEL, DIALOG_LIB_EDIT_PIN_BASE::_wxFB_OnCancelButtonClick )
	EVT_BUTTON( wxID_OK, DIALOG_LIB_EDIT_PIN_BASE::_wxFB_OnOKButtonClick )
END_EVENT_TABLE()

DIALOG_LIB_EDIT_PIN_BASE::DIALOG_LIB_EDIT_PIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 5, 6, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableCol( 4 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_staticTextPinName = new wxStaticText( this, wxID_ANY, _("Pin &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinName->Wrap( -1 );
	fgSizer1->Add( m_staticTextPinName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPinName = new wxTextCtrl( this, ID_M_TEXTPINNAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textPinName, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticTextNameSize = new wxStaticText( this, wxID_ANY, _("N&ame text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNameSize->Wrap( -1 );
	fgSizer1->Add( m_staticTextNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPinNameTextSize = new wxTextCtrl( this, ID_M_TEXTPINNAMETEXTSIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textPinNameTextSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticNameTextSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNameTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticNameTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextPadName = new wxStaticText( this, ID_M_STATICTEXTPADNAME, _("Pin n&umber:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadName->Wrap( -1 );
	m_staticTextPadName->SetToolTip( _("Pin number: 1 to 4 ASCII letters and/or digits") );
	
	fgSizer1->Add( m_staticTextPadName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPadName = new wxTextCtrl( this, ID_M_TEXTPADNAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textPadName, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticTextPadNameSize = new wxStaticText( this, wxID_ANY, _("Number te&xt size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadNameSize->Wrap( -1 );
	fgSizer1->Add( m_staticTextPadNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPadNameTextSize = new wxTextCtrl( this, ID_M_TEXTPADNAMETEXTSIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textPadNameTextSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticNumberTextSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNumberTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticNumberTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextOrient = new wxStaticText( this, wxID_ANY, _("&Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrient->Wrap( -1 );
	fgSizer1->Add( m_staticTextOrient, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceOrientation = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( m_choiceOrientation, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer1->Add( 15, 0, 1, wxEXPAND, 3 );
	
	m_staticTextPinLen = new wxStaticText( this, ID_M_STATICTEXTPINLEN, _("&Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinLen->Wrap( -1 );
	fgSizer1->Add( m_staticTextPinLen, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textLength = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textLength, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticLengthUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLengthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextEType = new wxStaticText( this, wxID_ANY, _("&Electrical type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEType->Wrap( -1 );
	m_staticTextEType->SetToolTip( _("Used by the ERC.") );
	
	fgSizer1->Add( m_staticTextEType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceElectricalType = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( m_choiceElectricalType, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 0, wxEXPAND, 3 );
	
	m_staticTextGstyle = new wxStaticText( this, wxID_ANY, _("Graphic &Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGstyle->Wrap( -1 );
	fgSizer1->Add( m_staticTextGstyle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceStyle = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( m_choiceStyle, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	mainSizer->Add( fgSizer1, 1, wxALL|wxEXPAND, 12 );
	
	wxBoxSizer* boarderSizer;
	boarderSizer = new wxBoxSizer( wxVERTICAL );
	
	m_checkApplyToAllParts = new wxCheckBox( this, wxID_ANY, _("Add to all &parts in package"), wxDefaultPosition, wxDefaultSize, 0 );
	boarderSizer->Add( m_checkApplyToAllParts, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Add to all alternate &body styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	boarderSizer->Add( m_checkApplyToAllConversions, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_checkShow = new wxCheckBox( this, wxID_ANY, _("&Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShow->SetValue(true); 
	boarderSizer->Add( m_checkShow, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	boarderSizer->Add( 0, 5, 0, wxALL|wxEXPAND, 10 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	boarderSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	boarderSizer->Add( m_sdbSizerButtons, 0, wxALL|wxALIGN_RIGHT, 0 );
	
	mainSizer->Add( boarderSizer, 0, wxALL|wxEXPAND, 12 );
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_EDIT_PIN_BASE::~DIALOG_LIB_EDIT_PIN_BASE()
{
}
