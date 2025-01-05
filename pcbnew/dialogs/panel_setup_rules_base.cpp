///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_box.h"

#include "panel_setup_rules_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_RULES_BASE::PANEL_SETUP_RULES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_title = new wxStaticText( this, wxID_ANY, _("DRC Rules"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	bSizer4->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );


	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );

	m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_syntaxHelp->SetToolTip( _("Show syntax help window") );

	bSizer4->Add( m_syntaxHelp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bSizer4, 0, wxEXPAND, 5 );

	m_topMargin = new wxBoxSizer( wxVERTICAL );

	m_textEditor = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_textEditor->SetUseTabs( true );
	m_textEditor->SetTabWidth( 4 );
	m_textEditor->SetIndent( 4 );
	m_textEditor->SetTabIndents( true );
	m_textEditor->SetBackSpaceUnIndents( true );
	m_textEditor->SetViewEOL( false );
	m_textEditor->SetViewWhiteSpace( false );
	m_textEditor->SetMarginWidth( 2, 0 );
	m_textEditor->SetIndentationGuides( true );
	m_textEditor->SetReadOnly( false );
	m_textEditor->SetMarginWidth( 1, 0 );
	m_textEditor->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_textEditor->SetMarginWidth( 0, m_textEditor->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_textEditor->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_textEditor->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_textEditor->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_textEditor->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_textEditor->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_textEditor->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_textEditor->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_textEditor->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_textEditor->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_textEditor->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_textEditor->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_topMargin->Add( m_textEditor, 3, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_compileButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_compileButton->SetToolTip( _("Check rule syntax") );

	bSizer5->Add( m_compileButton, 0, wxTOP|wxBOTTOM, 5 );

	m_errorsReport = new WX_HTML_REPORT_BOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_errorsReport->SetMinSize( wxSize( 400,60 ) );

	bSizer5->Add( m_errorsReport, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_topMargin->Add( bSizer5, 1, wxEXPAND, 5 );


	bPanelSizer->Add( m_topMargin, 1, wxEXPAND, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_SETUP_RULES_BASE::OnSyntaxHelp ), NULL, this );
	m_textEditor->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_SETUP_RULES_BASE::OnContextMenu ), NULL, this );
	m_compileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_RULES_BASE::OnCompile ), NULL, this );
	m_errorsReport->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_SETUP_RULES_BASE::OnErrorLinkClicked ), NULL, this );
}

PANEL_SETUP_RULES_BASE::~PANEL_SETUP_RULES_BASE()
{
	// Disconnect Events
	m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_SETUP_RULES_BASE::OnSyntaxHelp ), NULL, this );
	m_textEditor->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_SETUP_RULES_BASE::OnContextMenu ), NULL, this );
	m_compileButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_RULES_BASE::OnCompile ), NULL, this );
	m_errorsReport->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_SETUP_RULES_BASE::OnErrorLinkClicked ), NULL, this );

}
