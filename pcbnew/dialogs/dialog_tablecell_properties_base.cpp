///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/font_choice.h"
#include "widgets/wx_infobar.h"

#include "dialog_tablecell_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TABLECELL_PROPERTIES_BASE::DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id,
                                                                    const wxString& title, const wxPoint& pos,
                                                                    const wxSize& size, long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bMainSizer;
    bMainSizer = new wxBoxSizer( wxVERTICAL );

    m_infoBar = new WX_INFOBAR( this );
    m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
    m_infoBar->SetEffectDuration( 500 );
    m_infoBar->Hide();

    bMainSizer->Add( m_infoBar, 0, wxEXPAND | wxBOTTOM, 5 );

    wxBoxSizer* bSizer7;
    bSizer7 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer8;
    bSizer8 = new wxBoxSizer( wxVERTICAL );

    m_cellTextLabel = new wxStaticText( this, wxID_ANY, _( "Cell contents:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_cellTextLabel->Wrap( -1 );
    bSizer8->Add( m_cellTextLabel, 0, wxTOP | wxRIGHT | wxLEFT, 5 );

    m_cellTextCtrl =
            new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN, wxEmptyString );
    m_cellTextCtrl->SetUseTabs( false );
    m_cellTextCtrl->SetTabWidth( 4 );
    m_cellTextCtrl->SetIndent( 4 );
    m_cellTextCtrl->SetTabIndents( false );
    m_cellTextCtrl->SetBackSpaceUnIndents( false );
    m_cellTextCtrl->SetViewEOL( false );
    m_cellTextCtrl->SetViewWhiteSpace( false );
    m_cellTextCtrl->SetMarginWidth( 2, 0 );
    m_cellTextCtrl->SetIndentationGuides( false );
    m_cellTextCtrl->SetReadOnly( false );
    m_cellTextCtrl->SetMarginWidth( 1, 0 );
    m_cellTextCtrl->SetMarginWidth( 0, 0 );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
    m_cellTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT( "BLACK" ) ) );
    m_cellTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT( "WHITE" ) ) );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
    m_cellTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT( "BLACK" ) ) );
    m_cellTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT( "WHITE" ) ) );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
    m_cellTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT( "BLACK" ) ) );
    m_cellTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT( "WHITE" ) ) );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
    m_cellTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT( "BLACK" ) ) );
    m_cellTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT( "WHITE" ) ) );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
    m_cellTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
    m_cellTextCtrl->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
    m_cellTextCtrl->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    bSizer8->Add( m_cellTextCtrl, 1, wxEXPAND | wxALL, 2 );


    bSizer7->Add( bSizer8, 1, wxEXPAND | wxTOP | wxRIGHT, 5 );

    wxBoxSizer* bSizer9;
    bSizer9 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bMargins;
    bMargins = new wxBoxSizer( wxVERTICAL );

    wxGridBagSizer* gbFontSizer;
    gbFontSizer = new wxGridBagSizer( 3, 5 );
    gbFontSizer->SetFlexibleDirection( wxBOTH );
    gbFontSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    gbFontSizer->SetEmptyCellSize( wxSize( -1, 5 ) );

    m_fontLabel = new wxStaticText( this, wxID_ANY, _( "Font:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_fontLabel->Wrap( -1 );
    gbFontSizer->Add( m_fontLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | wxLEFT, 1 );

    wxString m_fontCtrlChoices[] = { _( "KiCad Font" ) };
    int      m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
    m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices,
                                  m_fontCtrlChoices, 0 );
    m_fontCtrl->SetSelection( 0 );
    gbFontSizer->Add( m_fontCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL | wxEXPAND, 5 );


    gbFontSizer->AddGrowableCol( 1 );

    bMargins->Add( gbFontSizer, 0, wxEXPAND | wxBOTTOM, 5 );

    wxFlexGridSizer* fgTextStyleSizer;
    fgTextStyleSizer = new wxFlexGridSizer( 0, 2, 5, 5 );
    fgTextStyleSizer->SetFlexibleDirection( wxBOTH );
    fgTextStyleSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    wxBoxSizer* hAlignButtons;
    hAlignButtons = new wxBoxSizer( wxHORIZONTAL );

    m_bold = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                wxBU_AUTODRAW | wxBORDER_NONE );
    hAlignButtons->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_italic = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                  wxBU_AUTODRAW | wxBORDER_NONE );
    hAlignButtons->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_separator0 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                      wxBU_AUTODRAW | wxBORDER_NONE );
    m_separator0->Enable( false );

    hAlignButtons->Add( m_separator0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_hAlignLeft = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                      wxBU_AUTODRAW | wxBORDER_NONE );
    m_hAlignLeft->SetToolTip( _( "Align left" ) );

    hAlignButtons->Add( m_hAlignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_hAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                        wxBU_AUTODRAW | wxBORDER_NONE );
    m_hAlignCenter->SetToolTip( _( "Align horizontal center" ) );

    hAlignButtons->Add( m_hAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_hAlignRight = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                       wxBU_AUTODRAW | wxBORDER_NONE );
    m_hAlignRight->SetToolTip( _( "Align right" ) );

    hAlignButtons->Add( m_hAlignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_separator1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                      wxBU_AUTODRAW | wxBORDER_NONE );
    m_separator1->Enable( false );

    hAlignButtons->Add( m_separator1, 0, wxALL, 5 );

    m_vAlignTop = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                     wxBU_AUTODRAW | wxBORDER_NONE );
    m_vAlignTop->SetToolTip( _( "Align top" ) );

    hAlignButtons->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_vAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                        wxBU_AUTODRAW | wxBORDER_NONE );
    m_vAlignCenter->SetToolTip( _( "Align vertical center" ) );

    hAlignButtons->Add( m_vAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_vAlignBottom = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                        wxBU_AUTODRAW | wxBORDER_NONE );
    m_vAlignBottom->SetToolTip( _( "Align bottom" ) );

    hAlignButtons->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );


    fgTextStyleSizer->Add( hAlignButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bMargins->Add( fgTextStyleSizer, 1, wxEXPAND, 5 );

    wxGridBagSizer* gbSizer1;
    gbSizer1 = new wxGridBagSizer( 3, 5 );
    gbSizer1->SetFlexibleDirection( wxBOTH );
    gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    gbSizer1->SetEmptyCellSize( wxSize( -1, 8 ) );

    m_SizeXLabel = new wxStaticText( this, wxID_ANY, _( "Text width:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_SizeXLabel->Wrap( -1 );
    m_SizeXLabel->SetToolTip( _( "Text width" ) );

    gbSizer1->Add( m_SizeXLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

    m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    gbSizer1->Add( m_SizeXCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | wxEXPAND, 5 );

    m_SizeXUnits = new wxStaticText( this, wxID_ANY, _( "unit" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_SizeXUnits->Wrap( -1 );
    gbSizer1->Add( m_SizeXUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

    m_SizeYLabel = new wxStaticText( this, wxID_ANY, _( "Text height:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_SizeYLabel->Wrap( -1 );
    m_SizeYLabel->SetToolTip( _( "Text height" ) );

    gbSizer1->Add( m_SizeYLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

    m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    gbSizer1->Add( m_SizeYCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | wxEXPAND, 5 );

    m_SizeYUnits = new wxStaticText( this, wxID_ANY, _( "unit" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_SizeYUnits->Wrap( -1 );
    gbSizer1->Add( m_SizeYUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

    m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _( "Thickness:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_ThicknessLabel->Wrap( -1 );
    m_ThicknessLabel->SetToolTip( _( "Text thickness" ) );

    gbSizer1->Add( m_ThicknessLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

    m_ThicknessCtrl =
            new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    gbSizer1->Add( m_ThicknessCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL | wxEXPAND, 5 );

    m_ThicknessUnits = new wxStaticText( this, wxID_ANY, _( "unit" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_ThicknessUnits->Wrap( -1 );
    gbSizer1->Add( m_ThicknessUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

    m_autoTextThickness = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                             wxBU_AUTODRAW | wxBORDER_NONE );
    m_autoTextThickness->SetToolTip( _( "Adjust the text thickness" ) );

    gbSizer1->Add( m_autoTextThickness, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


    bMargins->Add( gbSizer1, 0, wxEXPAND | wxTOP, 5 );


    bMargins->Add( 0, 20, 0, wxEXPAND, 5 );

    wxGridSizer* gMarginsSizer;
    gMarginsSizer = new wxGridSizer( 0, 3, 4, 2 );

    wxStaticText* marginsLabel;
    marginsLabel = new wxStaticText( this, wxID_ANY, _( "Cell margins:" ), wxDefaultPosition, wxDefaultSize, 0 );
    marginsLabel->Wrap( -1 );
    gMarginsSizer->Add( marginsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_marginTopCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    gMarginsSizer->Add( m_marginTopCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_marginTopUnits = new wxStaticText( this, wxID_ANY, _( "mm" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_marginTopUnits->Wrap( -1 );
    gMarginsSizer->Add( m_marginTopUnits, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2 );

    m_marginLeftCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    gMarginsSizer->Add( m_marginLeftCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


    gMarginsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

    m_marginRightCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    gMarginsSizer->Add( m_marginRightCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


    gMarginsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

    m_marginBottomCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    gMarginsSizer->Add( m_marginBottomCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bMargins->Add( gMarginsSizer, 0, wxEXPAND, 5 );


    bSizer9->Add( bMargins, 1, wxEXPAND | wxALL, 10 );


    bSizer7->Add( bSizer9, 1, wxEXPAND, 5 );


    bMainSizer->Add( bSizer7, 1, wxEXPAND | wxLEFT, 5 );

    wxBoxSizer* bButtons;
    bButtons = new wxBoxSizer( wxHORIZONTAL );

    m_editTable = new wxButton( this, wxID_ANY, _( "Edit Table..." ), wxDefaultPosition, wxDefaultSize, 0 );
    m_editTable->SetToolTip( _( "Edit table properties and cell contents" ) );

    bButtons->Add( m_editTable, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10 );

    m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _( "Syntax help" ), wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_syntaxHelp->SetToolTip( _( "Show syntax help window" ) );

    bButtons->Add( m_syntaxHelp, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10 );


    bButtons->Add( 0, 0, 1, wxEXPAND, 5 );

    m_sdbSizer1 = new wxStdDialogButtonSizer();
    m_sdbSizer1OK = new wxButton( this, wxID_OK );
    m_sdbSizer1->AddButton( m_sdbSizer1OK );
    m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
    m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
    m_sdbSizer1->Realize();

    bButtons->Add( m_sdbSizer1, 0, wxEXPAND | wxALL, 5 );


    bMainSizer->Add( bButtons, 0, wxEXPAND, 5 );


    this->SetSizer( bMainSizer );
    this->Layout();
    bMainSizer->Fit( this );

    // Connect Events
    m_bold->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                     wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBoldToggle ), NULL, this );
    m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                          wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextSize ), NULL, this );
    m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER,
                          wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
    m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                          wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextSize ), NULL, this );
    m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER,
                          wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
    m_ThicknessCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                              wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onThickness ), NULL, this );
    m_autoTextThickness->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onAutoTextThickness ), NULL,
                                  this );
    m_editTable->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                          wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );
    m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK,
                           wxHyperlinkEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onSyntaxHelp ), NULL, this );
}

DIALOG_TABLECELL_PROPERTIES_BASE::~DIALOG_TABLECELL_PROPERTIES_BASE()
{
    // Disconnect Events
    m_bold->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                        wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBoldToggle ), NULL, this );
    m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                             wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextSize ), NULL, this );
    m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER,
                             wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
    m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                             wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onTextSize ), NULL, this );
    m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER,
                             wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnOkClick ), NULL, this );
    m_ThicknessCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                 wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onThickness ), NULL, this );
    m_autoTextThickness->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                     wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onAutoTextThickness ),
                                     NULL, this );
    m_editTable->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                             wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onEditTable ), NULL, this );
    m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK,
                              wxHyperlinkEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onSyntaxHelp ), NULL, this );
}
