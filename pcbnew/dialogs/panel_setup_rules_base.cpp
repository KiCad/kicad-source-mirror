///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_rules_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_RULES_BASE::PANEL_SETUP_RULES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	m_leftMargin = new wxBoxSizer( wxHORIZONTAL );

	m_topMargin = new wxBoxSizer( wxVERTICAL );

	m_title = new wxStaticText( this, wxID_ANY, _("DRC rules:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_topMargin->Add( m_title, 0, wxTOP|wxBOTTOM, 5 );

	m_textEditor = new wxStyledTextCtrl( this, ID_RULES_EDITOR, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_textEditor->SetUseTabs( true );
	m_textEditor->SetTabWidth( 4 );
	m_textEditor->SetIndent( 4 );
	m_textEditor->SetTabIndents( true );
	m_textEditor->SetBackSpaceUnIndents( true );
	m_textEditor->SetViewEOL( false );
	m_textEditor->SetViewWhiteSpace( false );
	m_textEditor->SetMarginWidth( 2, 0 );
	m_textEditor->SetIndentationGuides( true );
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
	m_topMargin->Add( m_textEditor, 1, wxEXPAND | wxALL, 5 );


	m_leftMargin->Add( m_topMargin, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bPanelSizer->Add( m_leftMargin, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SETUP_RULES_BASE::~PANEL_SETUP_RULES_BASE()
{
}
