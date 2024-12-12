///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_box.h"

#include "panel_drc_rule_editor_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DRC_RULE_EDITOR_BASE::PANEL_DRC_RULE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );

	bContentSizer = new wxBoxSizer( wxVERTICAL );

	bBasicDetailSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 1, 4, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->AddGrowableCol( 3 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_nameLabel = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameLabel->Wrap( -1 );
	fgSizer2->Add( m_nameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_nameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer2->Add( m_nameCtrl, 0, wxALL|wxEXPAND, 5 );

	m_commentLabel = new wxStaticText( this, wxID_ANY, _("Comment"), wxDefaultPosition, wxDefaultSize, 0 );
	m_commentLabel->Wrap( -1 );
	fgSizer2->Add( m_commentLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_commentCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer2->Add( m_commentCtrl, 0, wxALL|wxEXPAND, 5 );


	bBasicDetailSizer->Add( fgSizer2, 0, wxEXPAND|wxLEFT|wxTOP, 5 );


	bContentSizer->Add( bBasicDetailSizer, 0, wxEXPAND, 20 );

	m_constraintSizer = new wxBoxSizer( wxVERTICAL );

	m_constraintHeaderTitle = new wxStaticText( this, wxID_ANY, _("Constraint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_constraintHeaderTitle->Wrap( -1 );
	m_constraintSizer->Add( m_constraintHeaderTitle, 0, wxALL, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_constraintSizer->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	m_constraintContentSizer = new wxBoxSizer( wxVERTICAL );


	m_constraintSizer->Add( m_constraintContentSizer, 0, wxEXPAND, 5 );


	bContentSizer->Add( m_constraintSizer, 0, wxEXPAND|wxTOP, 15 );

	wxBoxSizer* bConditionSizer;
	bConditionSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	m_conditionHeaderTitle = new wxStaticText( this, wxID_ANY, _("Conditions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_conditionHeaderTitle->Wrap( -1 );
	bSizer13->Add( m_conditionHeaderTitle, 0, wxALL, 5 );


	bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );

	m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer13->Add( m_syntaxHelp, 0, wxALL, 5 );


	bConditionSizer->Add( bSizer13, 0, wxEXPAND, 5 );

	m_staticline8 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bConditionSizer->Add( m_staticline8, 0, wxEXPAND | wxALL, 5 );

        m_conditionControlsSizer = new wxBoxSizer( wxVERTICAL );

	m_textConditionCtrl = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0, wxEmptyString );
	m_textConditionCtrl->SetUseTabs( true );
	m_textConditionCtrl->SetTabWidth( 4 );
	m_textConditionCtrl->SetIndent( 4 );
	m_textConditionCtrl->SetTabIndents( true );
	m_textConditionCtrl->SetBackSpaceUnIndents( true );
	m_textConditionCtrl->SetViewEOL( false );
	m_textConditionCtrl->SetViewWhiteSpace( false );
	m_textConditionCtrl->SetMarginWidth( 2, 0 );
	m_textConditionCtrl->SetIndentationGuides( true );
	m_textConditionCtrl->SetReadOnly( false );
	m_textConditionCtrl->SetMarginWidth( 1, 0 );
	m_textConditionCtrl->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_textConditionCtrl->SetMarginWidth( 0, m_textConditionCtrl->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_textConditionCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_textConditionCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_textConditionCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_textConditionCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_textConditionCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_textConditionCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_textConditionCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_textConditionCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_textConditionCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_textConditionCtrl->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_textConditionCtrl->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_textConditionCtrl->SetMaxSize( wxSize( -1,60 ) );

        m_conditionControlsSizer->Add( m_textConditionCtrl, 0, wxBOTTOM|wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_checkSyntaxBtnCtrl = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_checkSyntaxBtnCtrl->SetToolTip( _("Check rule syntax") );

	bSizer16->Add( m_checkSyntaxBtnCtrl, 0, wxALL, 5 );

	m_syntaxErrorReport = new WX_HTML_REPORT_BOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_syntaxErrorReport->SetMinSize( wxSize( -1,50 ) );

	bSizer16->Add( m_syntaxErrorReport, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


        m_conditionControlsSizer->Add( bSizer16, 1, wxEXPAND, 5 );


        bConditionSizer->Add( m_conditionControlsSizer, 1, wxEXPAND|wxTOP, 5 );


	bContentSizer->Add( bConditionSizer, 0, wxEXPAND, 15 );

	wxBoxSizer* bLayerSizer;
	bLayerSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText711 = new wxStaticText( this, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText711->Wrap( -1 );
	bLayerSizer->Add( m_staticText711, 0, wxALL|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticline111 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLayerSizer->Add( m_staticline111, 0, wxEXPAND | wxALL, 5 );

	m_LayersComboBoxSizer = new wxBoxSizer( wxVERTICAL );


	bLayerSizer->Add( m_LayersComboBoxSizer, 1, wxALL|wxEXPAND, 5 );


	bContentSizer->Add( bLayerSizer, 0, wxEXPAND|wxTOP, 15 );


	mainSizer->Add( bContentSizer, 1, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	// Connect Events
	m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onSyntaxHelp ), NULL, this );
	m_textConditionCtrl->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onContextMenu ), NULL, this );
	m_checkSyntaxBtnCtrl->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onCheckSyntax ), NULL, this );
	m_syntaxErrorReport->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onErrorLinkClicked ), NULL, this );
}

PANEL_DRC_RULE_EDITOR_BASE::~PANEL_DRC_RULE_EDITOR_BASE()
{
	// Disconnect Events
	m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onSyntaxHelp ), NULL, this );
	m_textConditionCtrl->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onContextMenu ), NULL, this );
	m_checkSyntaxBtnCtrl->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onCheckSyntax ), NULL, this );
	m_syntaxErrorReport->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_DRC_RULE_EDITOR_BASE::onErrorLinkClicked ), NULL, this );

}
