///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_gen_footprint_position_file_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GEN_FOOTPRINT_POSITION_BASE::DIALOG_GEN_FOOTPRINT_POSITION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerdirBrowse;
	bSizerdirBrowse = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bSizerdirBrowse->Add( m_staticTextDir, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_outputDirectoryName->SetMinSize( wxSize( 350,-1 ) );

	bSizerdirBrowse->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerdirBrowse->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bUpperSizer->Add( bSizerdirBrowse, 1, wxEXPAND|wxALL, 10 );


	m_MainSizer->Add( bUpperSizer, 0, wxEXPAND, 2 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_formatLabel = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_formatLabel->Wrap( -1 );
	fgSizer1->Add( m_formatLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_formatCtrlChoices[] = { _("Plain text"), _("CSV"), _("Gerber X3") };
	int m_formatCtrlNChoices = sizeof( m_formatCtrlChoices ) / sizeof( wxString );
	m_formatCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_formatCtrlNChoices, m_formatCtrlChoices, 0 );
	m_formatCtrl->SetSelection( 0 );
	fgSizer1->Add( m_formatCtrl, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_unitsLabel = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsLabel->Wrap( -1 );
	fgSizer1->Add( m_unitsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_unitsCtrlChoices[] = { _("Inches"), _("Millimeters") };
	int m_unitsCtrlNChoices = sizeof( m_unitsCtrlChoices ) / sizeof( wxString );
	m_unitsCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitsCtrlNChoices, m_unitsCtrlChoices, 0 );
	m_unitsCtrl->SetSelection( 1 );
	fgSizer1->Add( m_unitsCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMiddle->Add( fgSizer1, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerMiddle->Add( bSizer6, 1, wxEXPAND, 5 );


	m_MainSizer->Add( bSizerMiddle, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxVERTICAL );

	m_onlySMD = new wxCheckBox( this, wxID_ANY, _("Include only SMD footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_onlySMD, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeTH = new wxCheckBox( this, wxID_ANY, _("Exclude all footprints with through hole pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_excludeTH, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeDNP = new wxCheckBox( this, wxID_ANY, _("Exclude all footprints with the Do Not Populate flag set"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_excludeDNP, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeBOM = new wxCheckBox( this, wxID_ANY, _("Exclude all footprints with the Exclude from BOM flag set"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_excludeBOM, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbIncludeBoardEdge = new wxCheckBox( this, wxID_ANY, _("Include board edge layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_cbIncludeBoardEdge, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_useDrillPlaceOrigin = new wxCheckBox( this, wxID_ANY, _("Use drill/place file origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useDrillPlaceOrigin->SetValue(true);
	bSizerLower->Add( m_useDrillPlaceOrigin, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_negateXcb = new wxCheckBox( this, wxID_ANY, _("Use negative X coordinates for footprints on bottom layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_negateXcb, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_singleFile = new wxCheckBox( this, wxID_ANY, _("Generate single file with both front and back positions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_singleFile->SetValue(true);
	bSizerLower->Add( m_singleFile, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerLower->Add( 0, 5, 0, wxEXPAND, 5 );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( 350,120 ) );

	bSizerLower->Add( m_messagesPanel, 1, wxEXPAND|wxTOP, 10 );


	m_MainSizer->Add( bSizerLower, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_MainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_formatCtrl->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIFileOpt ), NULL, this );
	m_unitsCtrl->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIUnits ), NULL, this );
	m_onlySMD->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIOnlySMD ), NULL, this );
	m_excludeTH->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_excludeDNP->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_excludeBOM->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_cbIncludeBoardEdge->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIincludeBoardEdge ), NULL, this );
	m_negateXcb->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUInegXcoord ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onGenerate ), NULL, this );
}

DIALOG_GEN_FOOTPRINT_POSITION_BASE::~DIALOG_GEN_FOOTPRINT_POSITION_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_formatCtrl->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIFileOpt ), NULL, this );
	m_unitsCtrl->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIUnits ), NULL, this );
	m_onlySMD->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIOnlySMD ), NULL, this );
	m_excludeTH->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_excludeDNP->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_excludeBOM->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_cbIncludeBoardEdge->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIincludeBoardEdge ), NULL, this );
	m_negateXcb->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUInegXcoord ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onGenerate ), NULL, this );

}
