///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_component_in_lib_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelBasic = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerBasicPanel;
	bSizerBasicPanel = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelBasic, wxID_ANY, _("General :") ), wxVERTICAL );
	
	m_AsConvertButt = new wxCheckBox( m_PanelBasic, wxID_ANY, _("As Convert"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_AsConvertButt->SetToolTip( _("Check this option for components that have a De Morgan representation.\nThis is usual for gates.") );
	
	m_OptionsBoxSizer->Add( m_AsConvertButt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ShowPinNumButt = new wxCheckBox( m_PanelBasic, wxID_ANY, _("Show Pin Num"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_ShowPinNumButt->SetToolTip( _("Show or hide pin numbers") );
	
	m_OptionsBoxSizer->Add( m_ShowPinNumButt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ShowPinNameButt = new wxCheckBox( m_PanelBasic, wxID_ANY, _("Show Pin Name"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_ShowPinNameButt->SetToolTip( _("Show or hide pin names") );
	
	m_OptionsBoxSizer->Add( m_ShowPinNameButt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PinsNameInsideButt = new wxCheckBox( m_PanelBasic, wxID_ANY, _("Pin Name Inside"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_PinsNameInsideButt->SetToolTip( _("Check this option to have pin names inside the body and pin number outside.\nIf not checked pins names and pins numbers are outside.") );
	
	m_OptionsBoxSizer->Add( m_PinsNameInsideButt, 0, wxALL, 5 );
	
	bSizerBasicPanel->Add( m_OptionsBoxSizer, 0, 0, 5 );
	
	m_staticline3 = new wxStaticLine( m_PanelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerBasicPanel->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerMidBasicPanel;
	bSizerMidBasicPanel = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizernbunits;
	bSizernbunits = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextNbUnits = new wxStaticText( m_PanelBasic, wxID_ANY, _("Number of Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNbUnits->Wrap( -1 );
	bSizernbunits->Add( m_staticTextNbUnits, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SelNumberOfUnits = new wxSpinCtrl( m_PanelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 16, 1 );
	bSizernbunits->Add( m_SelNumberOfUnits, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizerMidBasicPanel->Add( bSizernbunits, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextskew = new wxStaticText( m_PanelBasic, wxID_ANY, _("Skew:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextskew->Wrap( -1 );
	bSizer17->Add( m_staticTextskew, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SetSkew = new wxSpinCtrl( m_PanelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
	bSizer17->Add( m_SetSkew, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizerMidBasicPanel->Add( bSizer17, 1, wxEXPAND, 5 );
	
	bSizerBasicPanel->Add( bSizerMidBasicPanel, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( m_PanelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerBasicPanel->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_OptionPower = new wxCheckBox( m_PanelBasic, wxID_ANY, _("Power Symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_OptionPower->SetToolTip( _("Check this option for power symbols.\nPower symbols have specific properties") );
	
	bSizerBasicPanel->Add( m_OptionPower, 0, wxALL, 5 );
	
	m_OptionPartsLocked = new wxCheckBox( m_PanelBasic, wxID_ANY, _("Parts are locked"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_OptionPartsLocked->SetToolTip( _("Check this option if Eeschema cannot change parts selections inside a given package\nThis happens when parts are differents in this package.") );
	
	bSizerBasicPanel->Add( m_OptionPartsLocked, 0, wxALL, 5 );
	
	m_PanelBasic->SetSizer( bSizerBasicPanel );
	m_PanelBasic->Layout();
	bSizerBasicPanel->Fit( m_PanelBasic );
	m_NoteBook->AddPage( m_PanelBasic, _("Options"), true );
	m_PanelDoc = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelDocBoxSizer;
	m_PanelDocBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDescription = new wxStaticText( m_PanelDoc, wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDescription->Wrap( -1 );
	m_PanelDocBoxSizer->Add( m_staticTextDescription, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Doc = new wxTextCtrl( m_PanelDoc, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelDocBoxSizer->Add( m_Doc, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextKeywords = new wxStaticText( m_PanelDoc, wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKeywords->Wrap( -1 );
	m_PanelDocBoxSizer->Add( m_staticTextKeywords, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Keywords = new wxTextCtrl( m_PanelDoc, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelDocBoxSizer->Add( m_Keywords, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextDocFileName = new wxStaticText( m_PanelDoc, wxID_ANY, _("DocFileName:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDocFileName->Wrap( -1 );
	m_PanelDocBoxSizer->Add( m_staticTextDocFileName, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Docfile = new wxTextCtrl( m_PanelDoc, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 400,-1 ), 0 );
	m_PanelDocBoxSizer->Add( m_Docfile, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerPaneldocbutts;
	bSizerPaneldocbutts = new wxBoxSizer( wxHORIZONTAL );
	
	m_ButtonCopyDoc = new wxButton( m_PanelDoc, ID_COPY_DOC_TO_ALIAS, _("Copy Doc"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPaneldocbutts->Add( m_ButtonCopyDoc, 0, wxALL, 5 );
	
	m_buttonBrowseDocFiles = new wxButton( m_PanelDoc, ID_BROWSE_DOC_FILES, _("Browse DocFiles"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPaneldocbutts->Add( m_buttonBrowseDocFiles, 0, wxALL, 5 );
	
	m_PanelDocBoxSizer->Add( bSizerPaneldocbutts, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_PanelDoc->SetSizer( m_PanelDocBoxSizer );
	m_PanelDoc->Layout();
	m_PanelDocBoxSizer->Fit( m_PanelDoc );
	m_NoteBook->AddPage( m_PanelDoc, _("Description"), false );
	m_PanelAlias = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerMainPanelAlias;
	bSizerMainPanelAlias = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftBoxSizerPanelAlias;
	bLeftBoxSizerPanelAlias = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextAlias = new wxStaticText( m_PanelAlias, wxID_ANY, _("Alias List:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAlias->Wrap( -1 );
	bLeftBoxSizerPanelAlias->Add( m_staticTextAlias, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PartAliasList = new wxListBox( m_PanelAlias, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bLeftBoxSizerPanelAlias->Add( m_PartAliasList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizerMainPanelAlias->Add( bLeftBoxSizerPanelAlias, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightBoxSizerPanelAlias;
	bRightBoxSizerPanelAlias = new wxBoxSizer( wxVERTICAL );
	
	m_ButtonAddeAlias = new wxButton( m_PanelAlias, ID_ADD_ALIAS, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightBoxSizerPanelAlias->Add( m_ButtonAddeAlias, 0, wxALL|wxEXPAND, 5 );
	
	m_ButtonDeleteOneAlias = new wxButton( m_PanelAlias, ID_DELETE_ONE_ALIAS, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightBoxSizerPanelAlias->Add( m_ButtonDeleteOneAlias, 0, wxALL|wxEXPAND, 5 );
	
	m_ButtonDeleteAllAlias = new wxButton( m_PanelAlias, ID_DELETE_ALL_ALIAS, _("Delete All"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightBoxSizerPanelAlias->Add( m_ButtonDeleteAllAlias, 0, wxALL, 5 );
	
	bSizerMainPanelAlias->Add( bRightBoxSizerPanelAlias, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_PanelAlias->SetSizer( bSizerMainPanelAlias );
	m_PanelAlias->Layout();
	bSizerMainPanelAlias->Fit( m_PanelAlias );
	m_NoteBook->AddPage( m_PanelAlias, _("Alias"), false );
	m_PanelFootprintFilter = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bPanelFpFilterBoxSizer;
	bPanelFpFilterBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bFpFilterLeftBoxSizer;
	bFpFilterLeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextFootprints = new wxStaticText( m_PanelFootprintFilter, wxID_ANY, _("Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFootprints->Wrap( -1 );
	bFpFilterLeftBoxSizer->Add( m_staticTextFootprints, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FootprintFilterListBox = new wxListBox( m_PanelFootprintFilter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bFpFilterLeftBoxSizer->Add( m_FootprintFilterListBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bPanelFpFilterBoxSizer->Add( bFpFilterLeftBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bFpFilterRightBoxSizer;
	bFpFilterRightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddFpF = new wxButton( m_PanelFootprintFilter, ID_ADD_FOOTPRINT_FILTER, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bFpFilterRightBoxSizer->Add( m_buttonAddFpF, 0, wxALL|wxEXPAND, 5 );
	
	m_ButtonDeleteOneFootprintFilter = new wxButton( m_PanelFootprintFilter, ID_DELETE_ONE_FOOTPRINT_FILTER, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bFpFilterRightBoxSizer->Add( m_ButtonDeleteOneFootprintFilter, 0, wxALL|wxEXPAND, 5 );
	
	m_ButtonDeleteAllFootprintFilter = new wxButton( m_PanelFootprintFilter, ID_DELETE_ALL_FOOTPRINT_FILTER, _("Delete All"), wxDefaultPosition, wxDefaultSize, 0 );
	bFpFilterRightBoxSizer->Add( m_ButtonDeleteAllFootprintFilter, 0, wxALL|wxEXPAND, 5 );
	
	bPanelFpFilterBoxSizer->Add( bFpFilterRightBoxSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_PanelFootprintFilter->SetSizer( bPanelFpFilterBoxSizer );
	m_PanelFootprintFilter->Layout();
	bPanelFpFilterBoxSizer->Fit( m_PanelFootprintFilter );
	m_NoteBook->AddPage( m_PanelFootprintFilter, _("Footprint Filter"), false );
	
	bMainSizer->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();
	bMainSizer->Add( m_sdbSizer2, 0, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_ButtonCopyDoc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::CopyDocToAlias ), NULL, this );
	m_buttonBrowseDocFiles->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::BrowseAndSelectDocFile ), NULL, this );
	m_ButtonAddeAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::AddAliasOfPart ), NULL, this );
	m_ButtonDeleteOneAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAliasOfPart ), NULL, this );
	m_ButtonDeleteAllAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAllAliasOfPart ), NULL, this );
	m_buttonAddFpF->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::AddFootprintFilter ), NULL, this );
	m_ButtonDeleteOneFootprintFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteOneFootprintFilter ), NULL, this );
	m_ButtonDeleteAllFootprintFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAllFootprintFilter ), NULL, this );
	m_sdbSizer2Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::OnOkClick ), NULL, this );
}

DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::~DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE()
{
	// Disconnect Events
	m_ButtonCopyDoc->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::CopyDocToAlias ), NULL, this );
	m_buttonBrowseDocFiles->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::BrowseAndSelectDocFile ), NULL, this );
	m_ButtonAddeAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::AddAliasOfPart ), NULL, this );
	m_ButtonDeleteOneAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAliasOfPart ), NULL, this );
	m_ButtonDeleteAllAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAllAliasOfPart ), NULL, this );
	m_buttonAddFpF->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::AddFootprintFilter ), NULL, this );
	m_ButtonDeleteOneFootprintFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteOneFootprintFilter ), NULL, this );
	m_ButtonDeleteAllFootprintFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::DeleteAllFootprintFilter ), NULL, this );
	m_sdbSizer2Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE::OnOkClick ), NULL, this );
}
