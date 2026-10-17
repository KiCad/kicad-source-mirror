///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "dialog_database_lib_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DATABASE_LIB_SETTINGS_BASE::DIALOG_DATABASE_LIB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	bMainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter->SetSashGravity( 0.5 );
	m_splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::m_splitterOnIdle ), NULL, this );
	m_splitter->SetMinimumPaneSize( 300 );

	m_editPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bEdit;
	bEdit = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( m_editPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionPage = new wxScrolledWindow( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxVSCROLL );
	m_connectionPage->SetScrollRate( 5, 5 );
	wxBoxSizer* bConnection;
	bConnection = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbMetadata;
	sbMetadata = new wxStaticBoxSizer( new wxStaticBox( m_connectionPage, wxID_ANY, _("Library") ), wxVERTICAL );

	wxFlexGridSizer* fgMetadata;
	fgMetadata = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgMetadata->AddGrowableCol( 1 );
	fgMetadata->SetFlexibleDirection( wxBOTH );
	fgMetadata->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_nameLabel;
	m_nameLabel = new wxStaticText( sbMetadata->GetStaticBox(), wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameLabel->Wrap( -1 );
	m_nameLabel->SetToolTip( _("Names the contents of this file. The nickname shown in the symbol library table is set in the library table, not here.") );

	fgMetadata->Add( m_nameLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_txtName = new wxTextCtrl( sbMetadata->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtName->SetToolTip( _("Names the contents of this file. The nickname shown in the symbol library table is set in the library table, not here.") );

	fgMetadata->Add( m_txtName, 1, wxEXPAND, 5 );

	wxStaticText* m_descriptionLabel;
	m_descriptionLabel = new wxStaticText( sbMetadata->GetStaticBox(), wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_descriptionLabel->Wrap( -1 );
	m_descriptionLabel->SetToolTip( _("Describes the contents of this file. The nickname shown in the symbol library table is set in the library table, not here.") );

	fgMetadata->Add( m_descriptionLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_txtDescription = new wxTextCtrl( sbMetadata->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtDescription->SetToolTip( _("Describes the contents of this file. The nickname shown in the symbol library table is set in the library table, not here.") );

	fgMetadata->Add( m_txtDescription, 1, wxEXPAND, 5 );


	sbMetadata->Add( fgMetadata, 0, wxEXPAND|wxALL, 5 );


	bConnection->Add( sbMetadata, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSource;
	sbSource = new wxStaticBoxSizer( new wxStaticBox( m_connectionPage, wxID_ANY, _("Data Source") ), wxVERTICAL );

	m_rbDSN = new wxRadioButton( sbSource->GetStaticBox(), wxID_ANY, _("ODBC data source name (DSN)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbSource->Add( m_rbDSN, 0, wxALL, 5 );

	wxFlexGridSizer* fgDSN;
	fgDSN = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgDSN->AddGrowableCol( 1 );
	fgDSN->SetFlexibleDirection( wxBOTH );
	fgDSN->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_dsnLabel;
	m_dsnLabel = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("Data source:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dsnLabel->Wrap( -1 );
	fgDSN->Add( m_dsnLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbDSN = new wxComboBox( sbSource->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgDSN->Add( m_cbDSN, 1, wxEXPAND, 5 );


	fgDSN->Add( 0, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bDsnInfo;
	bDsnInfo = new wxBoxSizer( wxHORIZONTAL );

	m_stDsnDriver = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_stDsnDriver->Wrap( -1 );
	bDsnInfo->Add( m_stDsnDriver, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_btnManageDsn = new wxButton( sbSource->GetStaticBox(), wxID_ANY, _("Manage Data Sources..."), wxDefaultPosition, wxDefaultSize, 0 );
	bDsnInfo->Add( m_btnManageDsn, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgDSN->Add( bDsnInfo, 0, wxEXPAND, 0 );

	wxStaticText* m_userLabel;
	m_userLabel = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("Username:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userLabel->Wrap( -1 );
	fgDSN->Add( m_userLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_txtUser = new wxTextCtrl( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgDSN->Add( m_txtUser, 1, wxEXPAND, 5 );

	wxStaticText* m_passwordLabel;
	m_passwordLabel = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_passwordLabel->Wrap( -1 );
	fgDSN->Add( m_passwordLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_txtPassword = new wxTextCtrl( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
	fgDSN->Add( m_txtPassword, 1, wxEXPAND, 5 );


	sbSource->Add( fgDSN, 0, wxEXPAND|wxLEFT|wxRIGHT, 25 );

	m_cbPasswordInFile = new wxCheckBox( sbSource->GetStaticBox(), wxID_ANY, _("Save password in the library file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPasswordInFile->SetToolTip( _("The library file is stored in plain text and is often shared.  When unchecked, the password is kept in this computer's system keyring.") );

	sbSource->Add( m_cbPasswordInFile, 0, wxLEFT|wxTOP, 25 );

	m_rbConnectionString = new wxRadioButton( sbSource->GetStaticBox(), wxID_ANY, _("Connection string"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSource->Add( m_rbConnectionString, 0, wxTOP|wxLEFT|wxRIGHT, 10 );

	m_txtConnectionString = new wxTextCtrl( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_txtConnectionString->SetMinSize( wxSize( -1,50 ) );

	sbSource->Add( m_txtConnectionString, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 25 );

	wxBoxSizer* bDriver;
	bDriver = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_driverLabel;
	m_driverLabel = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("Insert driver:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_driverLabel->Wrap( -1 );
	bDriver->Add( m_driverLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_choiceDriverChoices;
	m_choiceDriver = new wxChoice( sbSource->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDriverChoices, 0 );
	m_choiceDriver->SetSelection( 0 );
	bDriver->Add( m_choiceDriver, 1, wxALIGN_CENTER_VERTICAL, 5 );


	sbSource->Add( bDriver, 0, wxEXPAND|wxLEFT|wxTOP, 25 );

	m_stCwdHelp = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("${CWD}"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCwdHelp->Wrap( -1 );
	sbSource->Add( m_stCwdHelp, 0, wxLEFT|wxTOP, 25 );

	wxBoxSizer* bTimeout;
	bTimeout = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_timeoutLabel;
	m_timeoutLabel = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("Connection timeout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeoutLabel->Wrap( -1 );
	m_timeoutLabel->SetToolTip( _("How long to wait for the database to answer before a connection attempt is given up.") );

	bTimeout->Add( m_timeoutLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spinTimeout = new wxSpinCtrl( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, 2 );
	m_spinTimeout->SetToolTip( _("How long to wait for the database to answer before a connection attempt is given up.") );

	bTimeout->Add( m_spinTimeout, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_timeoutUnits;
	m_timeoutUnits = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeoutUnits->Wrap( -1 );
	bTimeout->Add( m_timeoutUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSource->Add( bTimeout, 0, wxTOP|wxLEFT, 10 );

	wxBoxSizer* bTest;
	bTest = new wxBoxSizer( wxHORIZONTAL );

	m_btnTest = new wxButton( sbSource->GetStaticBox(), wxID_ANY, _("Test Connection"), wxDefaultPosition, wxDefaultSize, 0 );
	bTest->Add( m_btnTest, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_stConnectionStatus = new wxStaticText( sbSource->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_stConnectionStatus->Wrap( -1 );
	bTest->Add( m_stConnectionStatus, 1, wxALIGN_CENTER_VERTICAL, 5 );


	sbSource->Add( bTest, 0, wxEXPAND|wxALL, 5 );


	bConnection->Add( sbSource, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbOptions;
	sbOptions = new wxStaticBoxSizer( new wxStaticBox( m_connectionPage, wxID_ANY, _("Advanced") ), wxVERTICAL );

	wxFlexGridSizer* fgCache;
	fgCache = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgCache->SetFlexibleDirection( wxBOTH );
	fgCache->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_cacheSizeLabel;
	m_cacheSizeLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Cache size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cacheSizeLabel->Wrap( -1 );
	m_cacheSizeLabel->SetToolTip( _("Number of tables kept in memory; each holds the rows read from it.") );

	fgCache->Add( m_cacheSizeLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCacheSize = new wxSpinCtrl( sbOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 256 );
	m_spinCacheSize->SetToolTip( _("Number of tables kept in memory; each holds the rows read from it.") );

	fgCache->Add( m_spinCacheSize, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_cacheSizeUnits;
	m_cacheSizeUnits = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("tables"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cacheSizeUnits->Wrap( -1 );
	fgCache->Add( m_cacheSizeUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_cacheAgeLabel;
	m_cacheAgeLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Cache lifetime:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cacheAgeLabel->Wrap( -1 );
	m_cacheAgeLabel->SetToolTip( _("How long a cached row is trusted before it is read from the database again.") );

	fgCache->Add( m_cacheAgeLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCacheAge = new wxSpinCtrl( sbOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 60 );
	m_spinCacheAge->SetToolTip( _("How long a cached row is trusted before it is read from the database again.") );

	fgCache->Add( m_spinCacheAge, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* m_cacheAgeUnits;
	m_cacheAgeUnits = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cacheAgeUnits->Wrap( -1 );
	fgCache->Add( m_cacheAgeUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbOptions->Add( fgCache, 0, wxALL, 5 );

	m_cbGloballyUniqueKeys = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Name symbols by key only, without the library nickname"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbGloballyUniqueKeys->SetToolTip( _("Use only when every key is unique across all libraries in this file. Symbols are then named by key alone, for example C14996, instead of Diodes/C14996.") );

	sbOptions->Add( m_cbGloballyUniqueKeys, 0, wxALL, 5 );


	bConnection->Add( sbOptions, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );


	m_connectionPage->SetSizer( bConnection );
	m_connectionPage->Layout();
	bConnection->Fit( m_connectionPage );
	m_notebook->AddPage( m_connectionPage, _("Connection"), true );
	m_librariesPage = new wxScrolledWindow( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxVSCROLL );
	m_librariesPage->SetScrollRate( 5, 5 );
	wxBoxSizer* bLibrariesPage;
	bLibrariesPage = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bLibraries;
	bLibraries = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLibList;
	bLibList = new wxBoxSizer( wxVERTICAL );

	m_stNoLibraries = new wxStaticText( m_librariesPage, wxID_ANY, _("No libraries yet. Add one to map a database table to symbols."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stNoLibraries->Wrap( -1 );
	bLibList->Add( m_stNoLibraries, 0, wxEXPAND|wxBOTTOM, 5 );

	m_btnAddFirstLibrary = new wxButton( m_librariesPage, wxID_ANY, _("Add Library"), wxDefaultPosition, wxDefaultSize, 0 );
	bLibList->Add( m_btnAddFirstLibrary, 0, wxBOTTOM, 5 );

	m_libList = new wxListBox( m_librariesPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE );
	m_libList->SetMinSize( wxSize( 160,-1 ) );

	bLibList->Add( m_libList, 1, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bLibButtons;
	bLibButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddLibrary = new STD_BITMAP_BUTTON( m_librariesPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnAddLibrary->SetToolTip( _("Add library") );

	bLibButtons->Add( m_btnAddLibrary, 0, wxRIGHT, 5 );

	m_btnDuplicateLibrary = new STD_BITMAP_BUTTON( m_librariesPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnDuplicateLibrary->SetToolTip( _("Duplicate library") );

	bLibButtons->Add( m_btnDuplicateLibrary, 0, wxRIGHT, 5 );

	m_btnMoveLibraryUp = new STD_BITMAP_BUTTON( m_librariesPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnMoveLibraryUp->SetToolTip( _("Move up") );

	bLibButtons->Add( m_btnMoveLibraryUp, 0, wxRIGHT, 5 );

	m_btnMoveLibraryDown = new STD_BITMAP_BUTTON( m_librariesPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnMoveLibraryDown->SetToolTip( _("Move down") );

	bLibButtons->Add( m_btnMoveLibraryDown, 0, wxRIGHT, 5 );


	bLibButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnRemoveLibrary = new STD_BITMAP_BUTTON( m_librariesPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnRemoveLibrary->SetToolTip( _("Remove library") );

	bLibButtons->Add( m_btnRemoveLibrary, 0, wxRIGHT, 5 );


	bLibList->Add( bLibButtons, 0, wxEXPAND, 0 );


	bLibraries->Add( bLibList, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLibDetails;
	bLibDetails = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgLibrary;
	fgLibrary = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgLibrary->AddGrowableCol( 1 );
	fgLibrary->SetFlexibleDirection( wxBOTH );
	fgLibrary->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_libNameLabel;
	m_libNameLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Nickname:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libNameLabel->Wrap( -1 );
	fgLibrary->Add( m_libNameLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_txtLibName = new wxTextCtrl( m_librariesPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgLibrary->Add( m_txtLibName, 1, wxEXPAND, 5 );


	fgLibrary->Add( 0, 0, 0, wxEXPAND, 5 );

	m_stSymbolExample = new wxStaticText( m_librariesPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_stSymbolExample->Wrap( -1 );
	fgLibrary->Add( m_stSymbolExample, 0, wxEXPAND, 5 );

	wxStaticText* m_tableLabel;
	m_tableLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Table: *"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tableLabel->Wrap( -1 );
	fgLibrary->Add( m_tableLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTable = new wxComboBox( m_librariesPage, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgLibrary->Add( m_cbTable, 1, wxEXPAND, 5 );

	wxStaticText* m_keyLabel;
	m_keyLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Key column: *"), wxDefaultPosition, wxDefaultSize, 0 );
	m_keyLabel->Wrap( -1 );
	fgLibrary->Add( m_keyLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbKeyCol = new wxComboBox( m_librariesPage, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgLibrary->Add( m_cbKeyCol, 1, wxEXPAND, 5 );

	wxStaticText* m_symbolsLabel;
	m_symbolsLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Symbols column:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolsLabel->Wrap( -1 );
	fgLibrary->Add( m_symbolsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbSymbolsCol = new wxComboBox( m_librariesPage, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgLibrary->Add( m_cbSymbolsCol, 1, wxEXPAND, 5 );

	wxStaticText* m_footprintsLabel;
	m_footprintsLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Footprints column:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_footprintsLabel->Wrap( -1 );
	fgLibrary->Add( m_footprintsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbFootprintsCol = new wxComboBox( m_librariesPage, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgLibrary->Add( m_cbFootprintsCol, 1, wxEXPAND, 5 );

	wxStaticText* m_pinsLabel;
	m_pinsLabel = new wxStaticText( m_librariesPage, wxID_ANY, _("Pin map column:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinsLabel->Wrap( -1 );
	m_pinsLabel->SetToolTip( _("Optional column holding JSON that maps symbol pin numbers to footprint pads, for example [{\"sym\":\"1\",\"fp\":\"A\"},{\"sym\":\"2\",\"fp\":\"K\"}]") );

	fgLibrary->Add( m_pinsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPinsCol = new wxComboBox( m_librariesPage, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_cbPinsCol->SetToolTip( _("Optional column holding JSON that maps symbol pin numbers to footprint pads, for example [{\"sym\":\"1\",\"fp\":\"A\"},{\"sym\":\"2\",\"fp\":\"K\"}]") );

	fgLibrary->Add( m_cbPinsCol, 1, wxEXPAND, 5 );


	bLibDetails->Add( fgLibrary, 0, wxEXPAND|wxBOTTOM, 5 );

	m_stRequiredNote = new wxStaticText( m_librariesPage, wxID_ANY, _("* required"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stRequiredNote->Wrap( -1 );
	bLibDetails->Add( m_stRequiredNote, 0, wxBOTTOM, 5 );

	wxStaticBoxSizer* sbProperties;
	sbProperties = new wxStaticBoxSizer( new wxStaticBox( m_librariesPage, wxID_ANY, _("Symbol Properties (optional)") ), wxVERTICAL );

	wxFlexGridSizer* fgProperties;
	fgProperties = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgProperties->AddGrowableCol( 1 );
	fgProperties->SetFlexibleDirection( wxBOTH );
	fgProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_propDescLabel;
	m_propDescLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propDescLabel->Wrap( -1 );
	fgProperties->Add( m_propDescLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbDescriptionCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbDescriptionCol, 1, wxEXPAND, 5 );

	wxStaticText* m_propKeywordsLabel;
	m_propKeywordsLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propKeywordsLabel->Wrap( -1 );
	fgProperties->Add( m_propKeywordsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbKeywordsCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbKeywordsCol, 1, wxEXPAND, 5 );

	wxStaticText* m_propFpFiltersLabel;
	m_propFpFiltersLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Footprint filters:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propFpFiltersLabel->Wrap( -1 );
	fgProperties->Add( m_propFpFiltersLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbFpFiltersCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbFpFiltersCol, 1, wxEXPAND, 5 );

	wxStaticText* m_propExclSimLabel;
	m_propExclSimLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Exclude from simulation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propExclSimLabel->Wrap( -1 );
	fgProperties->Add( m_propExclSimLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbExclSimCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbExclSimCol, 1, wxEXPAND, 5 );

	wxStaticText* m_propExclBomLabel;
	m_propExclBomLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Exclude from BOM:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propExclBomLabel->Wrap( -1 );
	fgProperties->Add( m_propExclBomLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbExclBomCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbExclBomCol, 1, wxEXPAND, 5 );

	wxStaticText* m_propExclBoardLabel;
	m_propExclBoardLabel = new wxStaticText( sbProperties->GetStaticBox(), wxID_ANY, _("Exclude from board:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_propExclBoardLabel->Wrap( -1 );
	fgProperties->Add( m_propExclBoardLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_cbExclBoardCol = new wxComboBox( sbProperties->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgProperties->Add( m_cbExclBoardCol, 1, wxEXPAND, 5 );


	sbProperties->Add( fgProperties, 0, wxEXPAND|wxALL, 5 );


	bLibDetails->Add( sbProperties, 0, wxEXPAND|wxBOTTOM, 5 );


	bLibraries->Add( bLibDetails, 1, wxEXPAND|wxTOP|wxRIGHT|wxBOTTOM, 5 );


	bLibrariesPage->Add( bLibraries, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( m_librariesPage, wxID_ANY, _("Fields") ), wxVERTICAL );

	wxBoxSizer* bFields;
	bFields = new wxBoxSizer( wxVERTICAL );

	m_fieldsGrid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_fieldsGrid->CreateGrid( 0, 6 );
	m_fieldsGrid->EnableEditing( true );
	m_fieldsGrid->EnableGridLines( true );
	m_fieldsGrid->EnableDragGridSize( false );
	m_fieldsGrid->SetMargins( 0, 0 );

	// Columns
	m_fieldsGrid->EnableDragColMove( false );
	m_fieldsGrid->EnableDragColSize( true );
	m_fieldsGrid->SetColLabelValue( 0, _("Column") );
	m_fieldsGrid->SetColLabelValue( 1, _("Field") );
	m_fieldsGrid->SetColLabelValue( 2, _("On add") );
	m_fieldsGrid->SetColLabelValue( 3, _("In chooser") );
	m_fieldsGrid->SetColLabelValue( 4, _("Show name") );
	m_fieldsGrid->SetColLabelValue( 5, _("Inherit") );
	m_fieldsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_fieldsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_fieldsGrid->EnableDragRowSize( true );
	m_fieldsGrid->SetRowLabelSize( 0 );
	m_fieldsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_fieldsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_fieldsGrid->SetMinSize( wxSize( -1,140 ) );

	bFields->Add( m_fieldsGrid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bFieldButtons;
	bFieldButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddField = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnAddField->SetToolTip( _("Add field") );

	bFieldButtons->Add( m_btnAddField, 0, wxRIGHT, 5 );

	m_btnMoveFieldUp = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnMoveFieldUp->SetToolTip( _("Move up") );

	bFieldButtons->Add( m_btnMoveFieldUp, 0, wxRIGHT, 5 );

	m_btnMoveFieldDown = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnMoveFieldDown->SetToolTip( _("Move down") );

	bFieldButtons->Add( m_btnMoveFieldDown, 0, wxRIGHT, 5 );

	m_btnRemoveField = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_btnRemoveField->SetToolTip( _("Remove field") );

	bFieldButtons->Add( m_btnRemoveField, 0, wxRIGHT, 5 );


	bFieldButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnAddUnmapped = new wxButton( sbFields->GetStaticBox(), wxID_ANY, _("Add Unmapped Columns"), wxDefaultPosition, wxDefaultSize, 0 );
	bFieldButtons->Add( m_btnAddUnmapped, 0, wxRIGHT, 5 );


	bFields->Add( bFieldButtons, 0, wxEXPAND|wxLEFT|wxBOTTOM, 5 );


	sbFields->Add( bFields, 1, wxEXPAND, 5 );


	bLibrariesPage->Add( sbFields, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );


	m_librariesPage->SetSizer( bLibrariesPage );
	m_librariesPage->Layout();
	bLibrariesPage->Fit( m_librariesPage );
	m_notebook->AddPage( m_librariesPage, _("Libraries"), false );
	m_dataPage = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bData;
	bData = new wxBoxSizer( wxVERTICAL );

	m_dataGrid = new WX_GRID( m_dataPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_dataGrid->CreateGrid( 0, 0 );
	m_dataGrid->EnableEditing( false );
	m_dataGrid->EnableGridLines( true );
	m_dataGrid->EnableDragGridSize( false );
	m_dataGrid->SetMargins( 0, 0 );

	// Columns
	m_dataGrid->EnableDragColMove( false );
	m_dataGrid->EnableDragColSize( true );
	m_dataGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_dataGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_dataGrid->EnableDragRowSize( true );
	m_dataGrid->SetRowLabelSize( 0 );
	m_dataGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_dataGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_dataGrid->SetMinSize( wxSize( -1,120 ) );

	bData->Add( m_dataGrid, 1, wxEXPAND|wxALL, 5 );

	m_stDataStatus = new wxStaticText( m_dataPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stDataStatus->Wrap( -1 );
	bData->Add( m_stDataStatus, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );


	m_dataPage->SetSizer( bData );
	m_dataPage->Layout();
	bData->Fit( m_dataPage );
	m_notebook->AddPage( m_dataPage, _("Data"), false );

	bEdit->Add( m_notebook, 1, wxEXPAND|wxALL, 5 );


	m_editPanel->SetSizer( bEdit );
	m_editPanel->Layout();
	bEdit->Fit( m_editPanel );
	m_previewPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_previewSizer = new wxBoxSizer( wxVERTICAL );


	m_previewPanel->SetSizer( m_previewSizer );
	m_previewPanel->Layout();
	m_previewSizer->Fit( m_previewPanel );
	m_splitter->SplitVertically( m_editPanel, m_previewPanel, 620 );
	bMainSizer->Add( m_splitter, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_txtName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_txtDescription->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_rbDSN->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceTypeChanged ), NULL, this );
	m_cbDSN->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_cbDSN->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_btnManageDsn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnManageDataSources ), NULL, this );
	m_txtUser->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_txtPassword->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_rbConnectionString->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceTypeChanged ), NULL, this );
	m_txtConnectionString->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_choiceDriver->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnInsertDriver ), NULL, this );
	m_spinTimeout->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceSpinChanged ), NULL, this );
	m_btnTest->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTestConnection ), NULL, this );
	m_spinCacheSize->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingSpinChanged ), NULL, this );
	m_spinCacheAge->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingSpinChanged ), NULL, this );
	m_cbGloballyUniqueKeys->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_btnAddFirstLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddLibrary ), NULL, this );
	m_libList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibrarySelected ), NULL, this );
	m_btnAddLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddLibrary ), NULL, this );
	m_btnDuplicateLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDuplicateLibrary ), NULL, this );
	m_btnMoveLibraryUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveLibraryUp ), NULL, this );
	m_btnMoveLibraryDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveLibraryDown ), NULL, this );
	m_btnRemoveLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnRemoveLibrary ), NULL, this );
	m_txtLibName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbTable->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTableChanged ), NULL, this );
	m_cbTable->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTableChanged ), NULL, this );
	m_cbKeyCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeyCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbSymbolsCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbSymbolsCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFootprintsCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFootprintsCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbPinsCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbPinsCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbDescriptionCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbDescriptionCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeywordsCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeywordsCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFpFiltersCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFpFiltersCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclSimCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclSimCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBomCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBomCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBoardCol->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBoardCol->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_fieldsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnFieldCellChanged ), NULL, this );
	m_btnAddField->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddField ), NULL, this );
	m_btnMoveFieldUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveFieldUp ), NULL, this );
	m_btnMoveFieldDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveFieldDown ), NULL, this );
	m_btnRemoveField->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnRemoveField ), NULL, this );
	m_btnAddUnmapped->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddUnmappedColumns ), NULL, this );
	m_dataGrid->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDataLabelRightClick ), NULL, this );
	m_dataGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDataCellSelected ), NULL, this );
}

DIALOG_DATABASE_LIB_SETTINGS_BASE::~DIALOG_DATABASE_LIB_SETTINGS_BASE()
{
	// Disconnect Events
	m_txtName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_txtDescription->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_rbDSN->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceTypeChanged ), NULL, this );
	m_cbDSN->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_cbDSN->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_btnManageDsn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnManageDataSources ), NULL, this );
	m_txtUser->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_txtPassword->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_rbConnectionString->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceTypeChanged ), NULL, this );
	m_txtConnectionString->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceChanged ), NULL, this );
	m_choiceDriver->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnInsertDriver ), NULL, this );
	m_spinTimeout->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSourceSpinChanged ), NULL, this );
	m_btnTest->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTestConnection ), NULL, this );
	m_spinCacheSize->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingSpinChanged ), NULL, this );
	m_spinCacheAge->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingSpinChanged ), NULL, this );
	m_cbGloballyUniqueKeys->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnSettingChanged ), NULL, this );
	m_btnAddFirstLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddLibrary ), NULL, this );
	m_libList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibrarySelected ), NULL, this );
	m_btnAddLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddLibrary ), NULL, this );
	m_btnDuplicateLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDuplicateLibrary ), NULL, this );
	m_btnMoveLibraryUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveLibraryUp ), NULL, this );
	m_btnMoveLibraryDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveLibraryDown ), NULL, this );
	m_btnRemoveLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnRemoveLibrary ), NULL, this );
	m_txtLibName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbTable->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTableChanged ), NULL, this );
	m_cbTable->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnTableChanged ), NULL, this );
	m_cbKeyCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeyCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbSymbolsCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbSymbolsCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFootprintsCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFootprintsCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbPinsCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbPinsCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbDescriptionCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbDescriptionCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeywordsCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbKeywordsCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFpFiltersCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbFpFiltersCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclSimCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclSimCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBomCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBomCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBoardCol->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_cbExclBoardCol->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnLibraryChanged ), NULL, this );
	m_fieldsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnFieldCellChanged ), NULL, this );
	m_btnAddField->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddField ), NULL, this );
	m_btnMoveFieldUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveFieldUp ), NULL, this );
	m_btnMoveFieldDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnMoveFieldDown ), NULL, this );
	m_btnRemoveField->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnRemoveField ), NULL, this );
	m_btnAddUnmapped->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnAddUnmappedColumns ), NULL, this );
	m_dataGrid->Disconnect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDataLabelRightClick ), NULL, this );
	m_dataGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDataCellSelected ), NULL, this );

}
