///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_image_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMAGE_PROPERTIES_BASE::DIALOG_IMAGE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* m_GeneralBoxSizer;
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelGeneral = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerProperties;
	sbSizerProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, wxT("Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPos;
	fgSizerPos = new wxFlexGridSizer( 2, 3, 1, 0 );
	fgSizerPos->AddGrowableCol( 1 );
	fgSizerPos->AddGrowableRow( 0 );
	fgSizerPos->SetFlexibleDirection( wxBOTH );
	fgSizerPos->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_XPosLabel = new wxStaticText( sbSizerProperties->GetStaticBox(), wxID_ANY, wxT("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_XPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionX = new wxTextCtrl( sbSizerProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionX, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_XPosUnit = new wxStaticText( sbSizerProperties->GetStaticBox(), wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_XPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_YPosLabel = new wxStaticText( sbSizerProperties->GetStaticBox(), wxID_ANY, wxT("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_YPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionY = new wxTextCtrl( sbSizerProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionY, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 1 );

	m_YPosUnit = new wxStaticText( sbSizerProperties->GetStaticBox(), wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_YPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbSizerProperties->Add( fgSizerPos, 0, wxBOTTOM|wxEXPAND, 3 );


	bSizerProperties->Add( sbSizerProperties, 1, wxALL|wxEXPAND, 5 );


	m_PanelGeneral->SetSizer( bSizerProperties );
	m_PanelGeneral->Layout();
	bSizerProperties->Fit( m_PanelGeneral );
	m_Notebook->AddPage( m_PanelGeneral, wxT("General"), false );

	m_GeneralBoxSizer->Add( m_Notebook, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	m_GeneralBoxSizer->Add( m_sdbSizerStdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();

	this->Centre( wxBOTH );
}

DIALOG_IMAGE_PROPERTIES_BASE::~DIALOG_IMAGE_PROPERTIES_BASE()
{
}
