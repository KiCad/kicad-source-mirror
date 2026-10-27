/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <set>

#include <wx/app.h>
#include <wx/display.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/menu.h>
#include <wx/regex.h>
#include <wx/settings.h>
#include <wx/stdpaths.h>
#include <wx/thread.h>
#include <wx/utils.h>

#include <bitmaps.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <database/database_autofill.h>
#include <database/database_lib_settings.h>
#include <dialogs/dialog_database_lib_settings.h>
#include <eda_draw_frame.h>
#include <eeschema_settings.h>
#include <grid_tricks.h>
#include <i18n_utility.h>
#include <kiplatform/ui.h>
#include <lib_symbol.h>
#include <pgm_base.h>
#include <reporter.h>
#include <sch_io/database/sch_io_database.h>
#include <thread_pool.h>
#include <scoped_set_reset.h>
#include <settings/json_settings_internals.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/db_lib_preview_panel.h>
#include <widgets/grid_combobox.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/wx_infobar.h>
#include <wildcards_and_files_ext.h>

#ifdef _WIN32
#include <database/ace_backend.h>
#endif


/// Rows read for the Data page and preview; enough to judge a mapping without loading a whole table
static constexpr size_t PREVIEW_ROW_LIMIT = 200;

/// Narrowest the preview next to the settings can be before its tree columns are cut off
static constexpr int PREVIEW_MIN_WIDTH_DIP = 450;

/// Delay after the last edit before the preview is rebuilt
static constexpr int PREVIEW_DEBOUNCE_MS = 750;

/// How often the Access provider check is asked whether it has finished, and for how long
static constexpr int PROVIDER_POLL_MS = 250;
static constexpr int PROVIDER_POLL_LIMIT = 80;

/// Pause in typing a database file path before the connection is tried
static constexpr int CONNECT_DEBOUNCE_MS = 500;


enum class ACCESS_PROVIDER
{
    MISSING,
    PENDING,
    AVAILABLE
};


/// Whether this computer can read Access files, which never blocks
static ACCESS_PROVIDER accessProvider()
{
#ifdef _WIN32
    switch( ACE_BACKEND::ProviderState() )
    {
    case ACE_BACKEND::PROVIDER_STATE::AVAILABLE: return ACCESS_PROVIDER::AVAILABLE;
    case ACE_BACKEND::PROVIDER_STATE::PENDING:
    case ACE_BACKEND::PROVIDER_STATE::BUSY: return ACCESS_PROVIDER::PENDING;
    case ACE_BACKEND::PROVIDER_STATE::MISSING: break;
    }
#endif

    return ACCESS_PROVIDER::MISSING;
}


/// Re-lays out a scrolled page and refits its scroll area after its contents change size
static void relayoutPage( wxScrolledWindow* aPage )
{
    aPage->Layout();
    aPage->FitInside();
}


/// True if autofill wrote aValue for aKey and the user has not changed it since
template <typename MAP>
static bool isAutofilled( const MAP& aAutofilled, const typename MAP::key_type& aKey, const std::string& aValue )
{
    auto it = aAutofilled.find( aKey );

    return it != aAutofilled.end() && it->second == aValue;
}


enum FIELD_COLUMNS
{
    FIELD_COL_COLUMN = 0,
    FIELD_COL_NAME,
    FIELD_COL_VISIBLE_ON_ADD,
    FIELD_COL_VISIBLE_IN_CHOOSER,
    FIELD_COL_SHOW_NAME,
    FIELD_COL_INHERIT
};


static wxString displayName( const DATABASE_LIB_TABLE& aTable )
{
    return aTable.name.empty() ? _( "(no nickname)" ) : From_UTF8( aTable.name );
}


static wxString describeFailure( DB_FAILURE aFailure, const DATABASE_SOURCE& aSource )
{
    using FAILURE = DB_FAILURE;

    switch( aFailure )
    {
    case FAILURE::LOGIN:
        if( aSource.type == DATABASE_SOURCE_TYPE::ACCESS )
            return _( "The database password is missing or wrong." );

        return _( "The server rejected the user name or password." );

    case FAILURE::UNREACHABLE:
        if( aSource.type != DATABASE_SOURCE_TYPE::ODBC )
            return _( "Could not reach the database file; check that its drive or share is available." );

        return _( "Could not reach the database server; check that it is running." );

    case FAILURE::TIMEOUT:
        if( aSource.type != DATABASE_SOURCE_TYPE::ODBC )
        {
            const int seconds = DatabaseFileWaitSeconds( aSource.timeout );

            return wxString::Format( wxPLURAL( "The database file '%s' did not respond within %d second.",
                                               "The database file '%s' did not respond within %d seconds.",
                                               static_cast<unsigned>( seconds ) ),
                                     wxFileName( From_UTF8( aSource.path ) ).GetFullName(), seconds );
        }

        if( aSource.timeout <= 0 )
            return _( "The server did not answer in time." );

        return wxString::Format( wxPLURAL( "The server did not answer within %d second.",
                                           "The server did not answer within %d seconds.",
                                           static_cast<unsigned>( aSource.timeout ) ),
                                 aSource.timeout );

    case FAILURE::NO_DSN:
        if( !aSource.connection_string.empty() )
            return _( "The connection string names a data source or driver that is not configured." );

        return wxString::Format( _( "There is no ODBC data source named '%s'." ), From_UTF8( aSource.dsn ) );

    case FAILURE::NO_DRIVER:
        return _( "The ODBC driver for this data source is not installed." );

    case FAILURE::FILE_NOT_FOUND: return _( "The database file was not found." );

    case FAILURE::FILE_IN_USE: return _( "The database file is locked by another program." );

    case FAILURE::PROVIDER_MISSING: return _( "The Microsoft Access Database Engine is not installed." );

    case FAILURE::NOT_SUPPORTED_ON_PLATFORM: return _( "Microsoft Access databases can only be read on Windows." );

    case FAILURE::INVALID_TYPE:
        return wxString::Format( _( "This KiCad does not know the database type '%s'." ),
                                 From_UTF8( aSource.type_name ) );

    case FAILURE::NETWORK_WAL:
        return _( "A database in write-ahead log mode cannot be read safely from a network drive." );

    case FAILURE::UNKNOWN: break;
    }

    return _( "Could not connect to the database." );
}


/// A symbol field that autofill maps from a like-named column
struct AUTOFILL_FIELD
{
    const char*           name;
    std::vector<wxString> terms;
    bool                  visibleOnAdd;
};


static const std::vector<AUTOFILL_FIELD>& autofillFields()
{
    static const std::vector<AUTOFILL_FIELD> fields = {
        { "Value", { _HKI( "Value" ) }, true },
        { "Datasheet", { _HKI( "Datasheet" ), _HKI( "data sheet" ) }, false }
    };

    return fields;
}


static std::string fieldChoiceId( const char* aFieldName )
{
    return std::string( "field." ) + aFieldName;
}


static DB_AUTOFILL::ROLE autofillRole( const std::string& aChoiceId, const std::vector<wxString>& aTerms )
{
    DB_AUTOFILL::ROLE role;

    for( const wxString& term : aTerms )
    {
        role.terms.push_back( term );

        if( wxString translated = wxGetTranslation( term ); translated != term )
            role.terms.push_back( translated );
    }

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        if( auto it = cfg->m_DatabaseColumnChoices.find( aChoiceId ); it != cfg->m_DatabaseColumnChoices.end() )
            role.remembered = it->second;
    }

    return role;
}


static DATABASE_FIELD_MAPPING* findField( DATABASE_LIB_TABLE& aLib, const char* aName )
{
    for( DATABASE_FIELD_MAPPING& field : aLib.fields )
    {
        if( wxString::FromUTF8( field.name ).IsSameAs( aName, false ) )
            return &field;
    }

    return nullptr;
}


DIALOG_DATABASE_LIB_SETTINGS::DIALOG_DATABASE_LIB_SETTINGS( wxWindow* aParent, const wxString& aDblPath,
                                                            SYMBOL_LIBRARY_ADAPTER* aLibs,
                                                            const wxString& aDefaultDir ) :
        DIALOG_DATABASE_LIB_SETTINGS_BASE( aParent ),
        m_defaultDir( aDefaultDir ),
        m_libs( aLibs ),
        m_preview( nullptr ),
        m_currentLibrary( -1 ),
        m_loading( false ),
        m_unnamed( aDblPath.IsEmpty() ),
        m_flagMissing( false ),
        m_canSave( true ),
        m_previewDebounce( [this]()
                           {
                               updatePreview();
                           },
                           PREVIEW_DEBOUNCE_MS ),
        m_connectDebounce( [this]()
                           {
                               storeSource();
                               connect();
                           },
                           CONNECT_DEBOUNCE_MS )
{
    m_providerTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &DIALOG_DATABASE_LIB_SETTINGS::onProviderTimer, this, m_providerTimer.GetId() );

    // Search terms are English; autofill also tries their translations
    m_roles = {
        { m_cbKeyCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.key_col; }, false, _( "key" ),
          "key", { _HKI( "id" ), _HKI( "key" ) } },
        { m_cbSymbolsCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.symbols_col; }, false,
          _( "symbols" ), "symbols", { _HKI( "symbol" ), _HKI( "sym" ) } },
        { m_cbFootprintsCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.footprints_col; }, false,
          _( "footprints" ), "footprints", { _HKI( "footprint" ), _HKI( "fp" ) } },
        { m_cbPinsCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.pins_col; }, true, _( "pin map" ),
          "pins", { _HKI( "pin map" ), _HKI( "pins" ) } },
        { m_cbDescriptionCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.description; },
          true, _( "description" ), "description", { _HKI( "description" ), _HKI( "desc" ) } },
        { m_cbKeywordsCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.keywords; }, true,
          _( "keywords" ), "keywords", { _HKI( "keywords" ), _HKI( "tags" ) } },
        { m_cbFpFiltersCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.footprint_filters; },
          true, _( "footprint filters" ), "footprint_filters",
          { _HKI( "footprint filters" ), _HKI( "fp filters" ) } },
        { m_cbExclSimCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.exclude_from_sim; },
          true, _( "exclude from simulation" ), "exclude_from_sim",
          { _HKI( "exclude from simulation" ), _HKI( "exclude from sim" ) } },
        { m_cbExclBomCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.exclude_from_bom; },
          true, _( "exclude from BOM" ), "exclude_from_bom", { _HKI( "exclude from BOM" ) } },
        { m_cbExclBoardCol, []( DATABASE_LIB_TABLE& t ) -> std::string& { return t.properties.exclude_from_board; },
          true, _( "exclude from board" ), "exclude_from_board", { _HKI( "exclude from board" ) } }
    };

    wxFileName fn;
    bool       isNew = true;
    bool       loaded = true;

    if( m_unnamed )
    {
        // The file is chosen on OK, so there is nothing to load yet
        m_settings = std::make_unique<DATABASE_LIB_SETTINGS>( std::string() );
        m_settings->SetReadOnly( true );
        m_settings->ResetToDefaults();
    }
    else
    {
        // The settings file always carries the extension, whatever the library table row says
        fn.Assign( aDblPath );
        fn.SetExt( FILEEXT::DatabaseLibraryFileExtension );
        m_path = fn.GetFullPath();

        isNew = !wxFileExists( m_path );

        m_settings = LoadDatabaseLibSettingsReadOnly( m_path, &loaded );
        m_canSave = loaded && !m_settings->IsFutureFormat();
    }

    EDA_BASE_FRAME* frame = nullptr;

    for( wxWindow* window = aParent; window && !frame; window = window->GetParent() )
        frame = dynamic_cast<EDA_BASE_FRAME*>( window );

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    EDA_DRAW_FRAME*              drawFrame = dynamic_cast<EDA_DRAW_FRAME*>( frame );

    if( drawFrame && drawFrame->GetCanvas() )
        canvasType = drawFrame->GetCanvas()->GetBackend();
    else if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        canvasType = EDA_DRAW_PANEL_GAL::ResolveStoredCanvasType( cfg->m_Graphics.canvas_type );

    m_preview = new DB_LIB_PREVIEW_PANEL( m_previewPanel, frame, HasKiway() ? &Kiway() : nullptr, canvasType );
    m_previewSizer->Add( m_preview, 1, wxEXPAND | wxALL, 5 );

    m_btnBrowse->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_btnAddLibrary->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_btnDuplicateLibrary->SetBitmap( KiBitmapBundle( BITMAPS::copy ) );
    m_btnMoveLibraryUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_btnMoveLibraryDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_btnRemoveLibrary->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_btnAddField->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_btnMoveFieldUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_btnMoveFieldDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_btnRemoveField->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    for( int col : { FIELD_COL_VISIBLE_ON_ADD, FIELD_COL_VISIBLE_IN_CHOOSER, FIELD_COL_SHOW_NAME, FIELD_COL_INHERIT } )
    {
        wxGridCellAttr* attr = new wxGridCellAttr;
        attr->SetRenderer( new wxGridCellBoolRenderer() );
        attr->SetEditor( new wxGridCellBoolEditor() );
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        m_fieldsGrid->SetColAttr( col, attr );
    }

    m_fieldsGrid->PushEventHandler( new GRID_TRICKS( m_fieldsGrid ) );
    m_dataGrid->PushEventHandler( new GRID_TRICKS( m_dataGrid ) );

    // The button turns into Stop while connecting and must not shift the status beside it
    m_btnTest->SetMinSize( m_btnTest->GetBestSize() );

    // Lets the label shrink below its text so long messages ellipsize instead of widening the page
    m_stConnectionStatus->SetMinSize( wxSize( FromDIP( 40 ), -1 ) );
    m_stDsnDriver->SetMinSize( wxSize( FromDIP( 40 ), -1 ) );
    m_stSymbolExample->SetMinSize( wxSize( FromDIP( 40 ), -1 ) );

    if( m_unnamed && m_defaultDir.IsEmpty() )
        m_defaultDir = wxStandardPaths::Get().GetDocumentsDir();

    m_stCwdHelp->SetLabel( m_unnamed ? wxString::Format( _( "${CWD} is replaced by %s (the folder the file will "
                                                            "be saved in)" ),
                                                         m_defaultDir )
                                     : wxString::Format( _( "${CWD} is replaced by %s" ), fn.GetPath() ) );

    // The hint sits in the narrow library list column
    m_stNoLibraries->Wrap( FromDIP( 160 ) );

    if( !loaded )
    {
        m_infoBar->ShowMessage( _( "This file could not be read and is shown read-only." ), wxICON_ERROR );
    }
    else if( m_settings->IsFutureFormat() )
    {
        m_infoBar->ShowMessage( _( "This file was written by a newer version of KiCad and is shown read-only." ),
                                wxICON_WARNING );
    }
    else if( isNew )
    {
        m_infoBar->ShowMessage( _( "Choose a data source and click Test Connection, then add a library on the "
                                   "Libraries page." ),
                                wxICON_INFORMATION );
    }

    // Every control mirrors the file being edited, so restoring values from a previous session
    // would silently overwrite it
    OptOut( this );

    // Child windows of the combo boxes' parents, which destroy them
    m_tableIcon = new FIELD_WARNING_ICON( m_cbTable );

    for( const COLUMN_ROLE& role : m_roles )
    {
        m_roleIcons.push_back( new FIELD_WARNING_ICON( role.combo ) );

        role.combo->Bind( wxEVT_KILL_FOCUS,
                          [this]( wxFocusEvent& aEvent )
                          {
                              aEvent.Skip();
                              CallAfter( &DIALOG_DATABASE_LIB_SETTINGS::updateAutofillStyle );
                          } );
    }

    // Bound after GRID_TRICKS so the mapping menu runs first; a left click opens it as well
    m_dataGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &DIALOG_DATABASE_LIB_SETTINGS::OnDataLabelRightClick, this );
    m_dataGrid->Bind( wxEVT_GRID_LABEL_LEFT_CLICK, &DIALOG_DATABASE_LIB_SETTINGS::OnDataLabelRightClick, this );

    m_fieldsGrid->GetGridColLabelWindow()->Bind( wxEVT_MOTION,
            [this]( wxMouseEvent& aEvent )
            {
                const wxString tips[] = {
                    _( "Database column the field reads its value from" ),
                    _( "Name of the symbol field" ),
                    _( "Show the field on the schematic when the symbol is placed" ),
                    _( "Show the field as a column in the symbol chooser" ),
                    _( "Show the field name before its value" ),
                    _( "Take position and style from the symbol's own field of the same name" )
                };

                int x = m_fieldsGrid->CalcUnscrolledPosition( aEvent.GetPosition() ).x;
                int col = m_fieldsGrid->XToCol( x );

                bool validCol = col >= 0 && col < static_cast<int>( std::size( tips ) );

                m_fieldsGrid->GetGridColLabelWindow()->SetToolTip( validCol ? tips[col] : wxString() );
                aEvent.Skip();
            } );

    // Escape arrives as the Cancel button too
    Bind( wxEVT_BUTTON, &DIALOG_DATABASE_LIB_SETTINGS::onCancel, this, wxID_CANCEL );
    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_DATABASE_LIB_SETTINGS::onCloseWindow, this );

    SetTitle( m_unnamed ? _( "New Database Library" )
                        : wxString::Format( _( "Database Library Settings (%s)" ), fn.GetFullName() ) );

    SetupStandardButtons();

    if( !m_canSave )
        m_sdbSizerOK->Disable();

    // The pages scroll, so their full height must not make the dialog taller than the screen
    wxSize fullSize = GetBestSize();

    // wxFormBuilder's Fit() leaves each page as tall as its content, which GTK will not shrink
    for( wxScrolledWindow* page : { m_connectionPage, m_librariesPage } )
    {
        page->SetMinSize( wxSize( -1, FromDIP( 200 ) ) );
        page->SetSize( wxSize( page->GetSize().x, FromDIP( 200 ) ) );
    }

    finishDialogSettings();

    // DIALOG_SHIM opens at this size, clamped to the monitor the dialog lands on
    m_initialSize.IncTo( fullSize );
    m_connectionPage->FitInside();
    m_librariesPage->FitInside();

    // The base class restores a fixed sash position on the first idle event, which cuts the pages off wherever
    // the platform's fonts and controls need more room
    m_splitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::m_splitterOnIdle ),
                            nullptr, this );
    m_splitter->Bind( wxEVT_IDLE, &DIALOG_DATABASE_LIB_SETTINGS::onSplitterIdle, this );
}


/// A connection closes with a bounded wait that a dead share can use up, so the UI thread hands it to the pool
/// while a pool thread closes it inline, as detaching from inside purge() would lock the queue twice
static void releaseConnection( std::unique_ptr<DATABASE_CONNECTION> aConn )
{
    if( !aConn )
        return;

    // Once the app is gone the pool may be deleted too
    if( !wxTheApp || !wxIsMainThread() )
    {
        aConn.reset();
        return;
    }

    GetKiCadThreadPool().detach_task(
            [holder = std::shared_ptr<DATABASE_CONNECTION>( std::move( aConn ) )]() mutable
            {
                holder.reset();
            } );
}


DIALOG_DATABASE_LIB_SETTINGS::CONNECT_JOB::~CONNECT_JOB()
{
    // A job purged from the pool's queue is destroyed under its lock and never opened anything
    if( !started )
    {
        conn.reset();
        return;
    }

    releaseConnection( std::move( conn ) );
}


DIALOG_DATABASE_LIB_SETTINGS::~DIALOG_DATABASE_LIB_SETTINGS()
{
    m_providerTimer.Stop();

    cancelConnect();
    m_previewDebounce.Cancel();
    m_connectDebounce.Cancel();
    releaseConnection( std::move( m_conn ) );

    m_fieldsGrid->PopEventHandler( true );
    m_dataGrid->PopEventHandler( true );
}


bool DIALOG_DATABASE_LIB_SETTINGS::TransferDataToWindow()
{
    SCOPED_SET_RESET<bool> loading( m_loading, true );

    DATABASE_SOURCE& source = m_settings->m_Source;
    bool             useConnectionString = !source.connection_string.empty();
    bool             passwordInFile = !source.password.empty();

    m_loadedSource = source;

    m_keyringLogin = source;
    m_keyringLogin.password.clear();

    if( !passwordInFile && GetDatabasePassword( source, m_keyringLogin.password ) )
        source.password = m_keyringLogin.password;

    m_txtName->ChangeValue( From_UTF8( m_settings->m_Name ) );
    m_txtDescription->ChangeValue( From_UTF8( m_settings->m_Description ) );

    m_rbDSN->SetValue( !useConnectionString );
    m_rbConnectionString->SetValue( useConnectionString );

    loadBackends();

    m_dbFile = From_UTF8( source.Expanded( cwdDir() ).path );
    m_pathEdited = false;
    m_txtDbFile->ChangeValue( m_dbFile );
    m_txtFilePassword->ChangeValue( source.type == DATABASE_SOURCE_TYPE::ACCESS ? From_UTF8( source.password )
                                                                                : wxString() );

    for( const std::string& driver : ODBC_BACKEND::ListDrivers() )
        m_choiceDriver->Append( From_UTF8( driver ) );

    m_cbDSN->ChangeValue( From_UTF8( source.dsn ) );
    loadDataSources();
    m_txtUser->ChangeValue( From_UTF8( source.username ) );
    m_txtPassword->ChangeValue( source.type == DATABASE_SOURCE_TYPE::ODBC ? From_UTF8( source.password ) : wxString() );
    m_cbPasswordInFile->SetValue( passwordInFile );
    m_txtConnectionString->ChangeValue( From_UTF8( source.connection_string ) );
    m_spinTimeout->SetValue( source.timeout );

    m_spinCacheSize->SetValue( m_settings->m_Cache.max_size );
    m_spinCacheAge->SetValue( m_settings->m_Cache.max_age );
    m_cbGloballyUniqueKeys->SetValue( m_settings->m_GloballyUniqueKeys );

    updateSourceControls();

    m_currentLibrary = m_settings->m_Tables.empty() ? -1 : 0;
    loadLibraryList();
    loadLibrary();

    m_cleanSnapshot = settingsSnapshot();

    // Connect once the dialog is visible; a slow server should not delay it appearing
    CallAfter( [this]()
               {
                   connect();
               } );

    return true;
}


bool DIALOG_DATABASE_LIB_SETTINGS::TransferDataFromWindow()
{
    m_connectDebounce.Cancel();

    // Ctrl+Enter reaches here even with OK disabled
    if( !m_canSave || !m_fieldsGrid->CommitPendingChanges() )
        return false;

    storeSource();
    storeLibrary();

    if( !validateLibraries() )
        return false;

    // Asked last so that cancelling it leaves the keyring and the dialog as they were
    if( m_unnamed && m_path.IsEmpty() && !chooseSavePath() )
        return false;

    DATABASE_SOURCE& source = m_settings->m_Source;
    std::string      password = source.password;
    bool             useKeyring = source.type == DATABASE_SOURCE_TYPE::ODBC && source.connection_string.empty()
                      && !m_cbPasswordInFile->GetValue();

    if( useKeyring )
    {
        bool sameLogin = source.dsn == m_keyringLogin.dsn && source.username == m_keyringLogin.username;

        if( !password.empty() && ( !sameLogin || password != m_keyringLogin.password ) )
        {
            if( !StoreDatabasePassword( source, password ) )
            {
                DisplayError( this, _( "The password could not be saved in the system keyring, which needs a "
                                       "user name and a running keyring service.\n\n"
                                       "Check 'Save password in the library file' to keep it there instead." ) );
                return false;
            }
        }
        else if( password.empty() && sameLogin && !m_keyringLogin.password.empty() )
        {
            // A failure here is usually an entry that is already gone
            StoreDatabasePassword( source, password );
        }

        m_keyringLogin = source;
        source.password.clear();
    }

    // A file chosen in the save dialog is replaced, however much it already holds
    m_settings->SetReadOnly( false );
    bool saved = m_settings->SaveToFile( wxEmptyString, m_unnamed || !wxFileExists( m_path ) );
    m_settings->SetReadOnly( true );

    // The dialog stays open on failure and keeps connecting with the entered password
    source.password = password;

    // An unchanged file is not rewritten, which is not a failure
    if( !saved && m_settings->LastSaveFailed() )
    {
        DisplayError( this, wxString::Format( _( "Could not save '%s'." ), m_path ) );

        // The next OK asks for a location again, starting in the same folder
        if( m_unnamed )
        {
            m_defaultDir = cwdDir();
            m_path.Clear();
        }

        return false;
    }

    m_unnamed = false;

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        for( const auto& [id, column] : m_newChoices )
            DB_AUTOFILL::RememberChoice( cfg->m_DatabaseColumnChoices[id], column );
    }

    return true;
}


bool DIALOG_DATABASE_LIB_SETTINGS::validateLibraries()
{
    std::set<std::string> nicknames;

    for( size_t ii = 0; ii < m_settings->m_Tables.size(); ++ii )
    {
        const DATABASE_LIB_TABLE& table = m_settings->m_Tables[ii];
        wxString                  msg;
        wxWindow*                 focus = nullptr;
        wxString                  name = From_UTF8( table.name );

        if( table.table.empty() && table.key_col.empty() )
        {
            msg = wxString::Format( _( "Library '%s' needs a table and a key column." ), name );
            focus = m_cbTable;
        }
        else if( table.table.empty() )
        {
            msg = wxString::Format( _( "Library '%s' needs a table." ), name );
            focus = m_cbTable;
        }
        else if( table.key_col.empty() )
        {
            msg = wxString::Format( _( "Library '%s' needs a key column." ), name );
            focus = m_cbKeyCol;
        }
        else if( !nicknames.insert( table.name ).second )
        {
            msg = wxString::Format( _( "More than one library is named '%s'." ), name );
            focus = m_txtLibName;
        }

        if( !focus )
            continue;

        m_flagMissing = true;
        m_notebook->SetSelection( 1 );

        // Reloading would drop the autofill state of a library the user is already looking at
        if( m_currentLibrary != static_cast<int>( ii ) )
        {
            m_currentLibrary = static_cast<int>( ii );
            loadLibraryList();
            loadLibrary();
        }
        else
        {
            loadColumnChoices();
            loadDataGrid();
        }

        showInfo( msg, wxICON_ERROR );
        focus->SetFocus();
        return false;
    }

    return true;
}


wxString DIALOG_DATABASE_LIB_SETTINGS::cwdDir() const
{
    return m_path.IsEmpty() ? m_defaultDir : wxFileName( m_path ).GetPath();
}


bool DIALOG_DATABASE_LIB_SETTINGS::chooseSavePath()
{
    wxString ext = From_UTF8( FILEEXT::DatabaseLibraryFileExtension.c_str() );
    wxString filter = _( "KiCad database library files" )
                      + AddFileExtListToFilter( { FILEEXT::DatabaseLibraryFileExtension } );

    wxString dir = m_defaultDir;
    wxString name = DefaultDatabaseLibFileName( From_UTF8( m_settings->m_Name ) );

    wxFileDialog dlg( this, _( "Save Database Library" ), dir, name + wxS( "." ) + ext, filter,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxFileName fn( dlg.GetPath() );
    fn.SetExt( FILEEXT::DatabaseLibraryFileExtension );

    // The file dialog only confirmed the name as typed, not the one with the extension added
    if( fn.GetFullPath() != dlg.GetPath() && fn.FileExists()
        && !IsOK( this, wxString::Format( _( "Overwrite '%s'?" ), fn.GetFullPath() ) ) )
    {
        return false;
    }

    wxString oldBase = cwdDir();
    m_path = fn.GetFullPath();

    m_settings->SetFilename( m_path );
    m_settings->Internals()->SetFromString( "meta.filename", std::string( m_settings->GetFullFilename().ToUTF8() ) );
    m_stCwdHelp->SetLabel( wxString::Format( _( "${CWD} is replaced by %s" ), cwdDir() ) );

    // The picked file stays absolute so its stored form follows the save folder
    storeSource();

    // The connection was tested against another folder than the one it will resolve in once saved
    const DATABASE_SOURCE& stored = m_settings->m_Source;

    if( oldBase != cwdDir()
        && ( stored.connection_string.find( "${CWD}" ) != std::string::npos
             || stored.path.find( "${CWD}" ) != std::string::npos ) )
    {
        connect();
        showInfo( _( "The data source uses ${CWD} and was tested again in the folder you chose. "
                     "Check the result, then click OK to save." ),
                  wxICON_INFORMATION );
        return false;
    }

    return true;
}


std::string DIALOG_DATABASE_LIB_SETTINGS::settingsSnapshot()
{
    storeSource();
    storeLibrary();

    return m_settings->FormatAsString() + ( m_cbPasswordInFile->GetValue() ? "1" : "0" );
}


DIALOG_DATABASE_LIB_SETTINGS::CLOSE_ACTION DIALOG_DATABASE_LIB_SETTINGS::confirmClose()
{
    // A grid edit in progress only reaches the settings once committed
    if( !m_fieldsGrid->CommitPendingChanges() )
        return CLOSE_ACTION::STAY;

    if( !m_canSave || settingsSnapshot() == m_cleanSnapshot )
        return CLOSE_ACTION::DISCARD;

    bool     saved = false;
    wxString prompt = m_unnamed ? _( "Save the new database library before closing?" )
                                : wxString::Format( _( "Save changes to '%s' before closing?" ),
                                                    wxFileName( m_path ).GetFullName() );

    if( !HandleUnsavedChanges( this, prompt,
                               [&]() -> bool
                               {
                                   saved = TransferDataFromWindow();
                                   return saved;
                               } ) )
    {
        return CLOSE_ACTION::STAY;
    }

    if( saved )
    {
        EndDialogShim( wxID_OK );
        return CLOSE_ACTION::SAVED;
    }

    return CLOSE_ACTION::DISCARD;
}


void DIALOG_DATABASE_LIB_SETTINGS::onCancel( wxCommandEvent& aEvent )
{
    m_connectDebounce.Cancel();

    if( confirmClose() == CLOSE_ACTION::DISCARD )
        aEvent.Skip();
}


void DIALOG_DATABASE_LIB_SETTINGS::onCloseWindow( wxCloseEvent& aEvent )
{
    m_connectDebounce.Cancel();

    if( !aEvent.CanVeto() )
    {
        aEvent.Skip();
        return;
    }

    // The default close handler sends a Cancel, which would ask a second time
    if( confirmClose() == CLOSE_ACTION::DISCARD )
        EndDialogShim( wxID_CANCEL );
    else
        aEvent.Veto();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnSettingChanged( wxCommandEvent& aEvent )
{
    if( m_loading )
        return;

    m_settings->m_Name = TO_UTF8( m_txtName->GetValue() );
    m_settings->m_Description = TO_UTF8( m_txtDescription->GetValue() );
    m_settings->m_Cache.max_size = m_spinCacheSize->GetValue();
    m_settings->m_Cache.max_age = m_spinCacheAge->GetValue();
    m_settings->m_GloballyUniqueKeys = m_cbGloballyUniqueKeys->GetValue();

    updateSymbolExample();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnSettingSpinChanged( wxSpinEvent& aEvent )
{
    OnSettingChanged( aEvent );
}


void DIALOG_DATABASE_LIB_SETTINGS::loadDataSources()
{
    // Clearing the list raises text events that would store an empty data source
    SCOPED_SET_RESET<bool> loading( m_loading, true );

    wxString current = m_cbDSN->GetValue();

    m_dataSources = ODBC_BACKEND::ListDataSourceDetails();
    m_cbDSN->Clear();

    for( const ODBC_BACKEND::DATA_SOURCE& source : m_dataSources )
        m_cbDSN->Append( From_UTF8( source.name ) );

    m_cbDSN->ChangeValue( current );
    updateDsnDriver();
}


void DIALOG_DATABASE_LIB_SETTINGS::updateDsnDriver()
{
    std::string name = TO_UTF8( m_cbDSN->GetValue() );
    wxString    text;

    if( !name.empty() )
    {
        text = _( "Not configured on this computer" );

        for( const ODBC_BACKEND::DATA_SOURCE& source : m_dataSources )
        {
            if( source.name == name )
                text = wxString::Format( _( "Driver: %s" ), From_UTF8( source.driver ) );
        }
    }

    m_stDsnDriver->SetLabel( text );
    m_stDsnDriver->SetToolTip( text );

    // SetLabel() resizes the control to its text; layout shrinks it back so the end ellipsizes
    relayoutPage( m_connectionPage );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnManageDataSources( wxCommandEvent& aEvent )
{
#ifdef __WXMSW__
    // Waits so that the list can show what was added
    wxExecute( wxS( "odbcad32.exe" ), wxEXEC_SYNC );
    loadDataSources();
#endif
}


static wxString accessLabel( ACCESS_PROVIDER aProvider )
{
    return aProvider == ACCESS_PROVIDER::MISSING ? _( "Access (unavailable)" ) : _( "Access" );
}


void DIALOG_DATABASE_LIB_SETTINGS::loadBackends()
{
    const DATABASE_SOURCE& source = m_settings->m_Source;

    m_backendTypes = { DATABASE_SOURCE_TYPE::ODBC, DATABASE_SOURCE_TYPE::SQLITE };
    m_choiceBackend->Clear();
    m_choiceBackend->Append( _( "ODBC" ) );
    m_choiceBackend->Append( _( "SQLite" ) );

    const ACCESS_PROVIDER provider = accessProvider();

    if( provider == ACCESS_PROVIDER::PENDING )
        m_providerTimer.Start( PROVIDER_POLL_MS );

    // A file naming Access keeps its entry, marked when this computer cannot read it, so opening it never changes it
    if( provider == ACCESS_PROVIDER::AVAILABLE || source.type == DATABASE_SOURCE_TYPE::ACCESS )
    {
        m_backendTypes.push_back( DATABASE_SOURCE_TYPE::ACCESS );
        m_choiceBackend->Append( accessLabel( provider ) );
    }

    if( source.type == DATABASE_SOURCE_TYPE::INVALID )
    {
        m_backendTypes.push_back( DATABASE_SOURCE_TYPE::INVALID );
        m_choiceBackend->Append( wxString::Format( _( "Unknown (%s)" ), From_UTF8( source.type_name ) ) );
    }

    selectBackend( source.type );
}


DATABASE_SOURCE_TYPE DIALOG_DATABASE_LIB_SETTINGS::selectedBackend() const
{
    const int selection = m_choiceBackend->GetSelection();

    if( selection < 0 || selection >= static_cast<int>( m_backendTypes.size() ) )
        return DATABASE_SOURCE_TYPE::ODBC;

    return m_backendTypes[selection];
}


void DIALOG_DATABASE_LIB_SETTINGS::selectBackend( DATABASE_SOURCE_TYPE aType )
{
    auto it = std::find( m_backendTypes.begin(), m_backendTypes.end(), aType );

    m_choiceBackend->SetSelection( it == m_backendTypes.end() ? 0 : static_cast<int>( it - m_backendTypes.begin() ) );
}


void DIALOG_DATABASE_LIB_SETTINGS::onProviderTimer( wxTimerEvent& aEvent )
{
    const ACCESS_PROVIDER provider = accessProvider();

    if( provider == ACCESS_PROVIDER::PENDING && ++m_providerPolls < PROVIDER_POLL_LIMIT )
        return;

    m_providerTimer.Stop();

    // A check that never finished counts as unavailable
    const ACCESS_PROVIDER shown =
            provider == ACCESS_PROVIDER::AVAILABLE ? ACCESS_PROVIDER::AVAILABLE : ACCESS_PROVIDER::MISSING;
    const DATABASE_SOURCE_TYPE current = selectedBackend();
    auto access = std::find( m_backendTypes.begin(), m_backendTypes.end(), DATABASE_SOURCE_TYPE::ACCESS );

    if( access != m_backendTypes.end() )
    {
        m_choiceBackend->SetString( access - m_backendTypes.begin(), accessLabel( shown ) );
    }
    else if( shown == ACCESS_PROVIDER::AVAILABLE )
    {
        // An unknown type always comes last
        m_backendTypes.insert( m_backendTypes.begin() + 2, DATABASE_SOURCE_TYPE::ACCESS );
        m_choiceBackend->Insert( accessLabel( shown ), 2 );
    }

    selectBackend( current );
}


bool DIALOG_DATABASE_LIB_SETTINGS::sourceConfigured() const
{
    const DATABASE_SOURCE& source = m_settings->m_Source;

    switch( source.type )
    {
    case DATABASE_SOURCE_TYPE::ODBC: return !source.dsn.empty() || !source.connection_string.empty();
    case DATABASE_SOURCE_TYPE::SQLITE:
    case DATABASE_SOURCE_TYPE::ACCESS: return !source.path.empty();
    case DATABASE_SOURCE_TYPE::INVALID: return true;
    }

    return false;
}


void DIALOG_DATABASE_LIB_SETTINGS::updateSourceControls()
{
    const DATABASE_SOURCE_TYPE type = selectedBackend();
    const bool                 odbc = type == DATABASE_SOURCE_TYPE::ODBC;
    const bool                 access = type == DATABASE_SOURCE_TYPE::ACCESS;
    const bool                 file = access || type == DATABASE_SOURCE_TYPE::SQLITE;
    const bool                 useDSN = m_rbDSN->GetValue();
    wxSizer*                   sbSource = m_rbDSN->GetContainingSizer();
    wxSizer*                   fgBackend = m_choiceBackend->GetContainingSizer();

    sbSource->Show( m_rbDSN, odbc );
    sbSource->Show( m_rbConnectionString, odbc );
    sbSource->Show( m_cbDSN->GetContainingSizer(), odbc && useDSN );
    sbSource->Show( m_cbPasswordInFile, odbc && useDSN );
    sbSource->Show( m_txtConnectionString, odbc && !useDSN );
    sbSource->Show( m_choiceDriver->GetContainingSizer(), odbc && !useDSN );
    sbSource->Show( m_stCwdHelp, odbc && !useDSN );

    fgBackend->Show( m_fileLabel, file );
    fgBackend->Show( m_txtDbFile->GetContainingSizer(), file );
    fgBackend->Show( m_filePasswordLabel, access );
    fgBackend->Show( m_txtFilePassword, access );

    // Showing the data source rows shows every control in them
#ifdef __WXMSW__
    m_btnManageDsn->Show( odbc && useDSN );
#else
    m_btnManageDsn->Hide();
#endif

    relayoutPage( m_connectionPage );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnBackendChanged( wxCommandEvent& aEvent )
{
    updateSourceControls();
    sourceEdited();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnFileTextChanged( wxCommandEvent& aEvent )
{
    if( m_loading )
        return;

    m_dbFile = DatabasePathFromInput( m_txtDbFile->GetValue(), cwdDir() );
    m_pathEdited = true;
    sourceEdited();

    // Typing reports every keystroke, and a file on an unreachable share must not stall this thread
    m_connectDebounce.Restart();
    setConnectionStatus( _( "The file is tried when typing pauses." ), false );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnBrowseFile( wxCommandEvent& aEvent )
{
    wxString dir = wxPathOnly( m_dbFile );

    if( dir.IsEmpty() )
        dir = cwdDir();

    wxFileDialog dlg( this, _( "Select the database file" ), dir, wxEmptyString,
                      _( "SQLite databases (*.sqlite;*.sqlite3;*.db)|*.sqlite;*.sqlite3;*.db|"
                         "Access databases (*.accdb;*.mdb)|*.accdb;*.mdb|All files|*" ),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() != wxID_OK )
        return;

    m_txtDbFile->ChangeValue( dlg.GetPath() );
    m_dbFile = DatabasePathFromInput( dlg.GetPath(), cwdDir() );
    m_pathEdited = true;
    sourceEdited();
    m_connectDebounce.Cancel();
    storeSource();
    connect();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnSourceTypeChanged( wxCommandEvent& aEvent )
{
    updateSourceControls();
    OnSourceChanged( aEvent );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnSourceChanged( wxCommandEvent& aEvent )
{
    sourceEdited();
}


void DIALOG_DATABASE_LIB_SETTINGS::sourceEdited()
{
    if( m_loading )
        return;

    m_connectDebounce.Cancel();
    updateDsnDriver();
    storeSource();
    cancelConnect();
    markDirty();

    // Connecting on every keystroke would hammer the server, so it waits for an explicit test
    setConnectionStatus( _( "Data source changed; click Test Connection to apply it." ), false );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnSourceSpinChanged( wxSpinEvent& aEvent )
{
    OnSourceChanged( aEvent );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnInsertDriver( wxCommandEvent& aEvent )
{
    wxString driver = wxString::Format( wxS( "Driver={%s};" ), m_choiceDriver->GetStringSelection() );
    wxString text = m_txtConnectionString->GetValue();
    wxRegEx  existing( wxS( "Driver=\\{[^}]*\\};?" ), wxRE_ICASE | wxRE_ADVANCED );

    if( !existing.Replace( &text, driver, 1 ) )
        text = driver + text;

    m_txtConnectionString->SetValue( text );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnTestConnection( wxCommandEvent& aEvent )
{
    if( m_connectJob )
    {
        cancelConnect();
        setConnectionStatus( _( "Connection attempt stopped." ), false );
        refreshTables();
        markDirty();
        return;
    }

    m_connectDebounce.Cancel();
    storeSource();
    connect();
}


void DIALOG_DATABASE_LIB_SETTINGS::storeSource()
{
    DATABASE_SOURCE&           source = m_settings->m_Source;
    const DATABASE_SOURCE_TYPE type = selectedBackend();

    source.timeout = m_spinTimeout->GetValue();

    // An unknown type is restored as the file has it, whatever was chosen in between
    if( type == DATABASE_SOURCE_TYPE::INVALID )
    {
        source = m_loadedSource;
        source.timeout = m_spinTimeout->GetValue();
        return;
    }

    if( source.type != type )
    {
        source.type = type;
        source.type_name.clear();
        source.path.clear();
    }

    if( type == DATABASE_SOURCE_TYPE::ODBC )
    {
        if( m_rbConnectionString->GetValue() )
        {
            source.connection_string = TO_UTF8( m_txtConnectionString->GetValue() );
            source.dsn.clear();
            source.username.clear();
            source.password.clear();
        }
        else
        {
            source.connection_string.clear();
            source.dsn = TO_UTF8( m_cbDSN->GetValue() );
            source.username = TO_UTF8( m_txtUser->GetValue() );
            source.password = TO_UTF8( m_txtPassword->GetValue() );
        }

        return;
    }

    source.connection_string.clear();
    source.dsn.clear();
    source.username.clear();
    source.path = m_pathEdited ? DatabasePathForStorage( m_dbFile, cwdDir() ) : m_loadedSource.path;

    // The keyring is keyed by DSN login, so Access keeps its password in the file
    source.password = type == DATABASE_SOURCE_TYPE::ACCESS ? TO_UTF8( m_txtFilePassword->GetValue() ) : std::string();
}


void DIALOG_DATABASE_LIB_SETTINGS::connect()
{
    cancelConnect();

    releaseConnection( std::move( m_conn ) );
    m_tables.clear();
    m_columns.clear();
    m_sampleTable.clear();
    m_sampleColumns.clear();
    m_sampleRows.clear();
    m_sampleRepeats.clear();
    loadDataGrid();
    markDirty();

    const DATABASE_SOURCE& source = m_settings->m_Source;

    if( !sourceConfigured() )
    {
        setConnectionStatus( source.type == DATABASE_SOURCE_TYPE::ODBC ? _( "No data source configured." )
                                                                       : _( "No database file chosen." ),
                             false );
        refreshTables();
        return;
    }

    auto job = std::make_shared<CONNECT_JOB>();
    job->owner = this;
    SCH_IO_DATABASE::CONNECTION_RESULT created = SCH_IO_DATABASE::CreateConnection( *m_settings, false, cwdDir() );
    job->conn = std::move( created.connection );

    if( !job->conn )
    {
        setConnectionStatus( describeFailure( created.failure, source ), true, From_UTF8( created.error ) );
        refreshTables();
        return;
    }

    m_connectJob = job;

    const std::string sourceName = m_settings->ExpandedSource( cwdDir() ).Describe();

    setConnectionStatus( sourceName.empty() ? _( "Connecting..." )
                                            : wxString::Format( _( "Connecting to %s..." ), From_UTF8( sourceName ) ),
                         false );
    m_btnTest->SetLabel( _( "Stop" ) );
    refreshTables();

    // A slow or unreachable server would otherwise freeze the dialog for the whole login timeout
    GetKiCadThreadPool().detach_task(
            [job]()
            {
                job->started = true;
                job->connected = job->conn->Connect();

                if( job->connected )
                    job->conn->GetTables( job->tables );

                // A callback still pending at exit is destroyed after the pool, so it must not own the job
                if( wxTheApp )
                {
                    wxTheApp->CallAfter(
                            [weak = std::weak_ptr<CONNECT_JOB>( job )]()
                            {
                                std::shared_ptr<CONNECT_JOB> finished = weak.lock();

                                if( finished && finished->owner )
                                    finished->owner->onConnectFinished( finished );
                            } );
                }
            } );
}


void DIALOG_DATABASE_LIB_SETTINGS::cancelConnect()
{
    if( !m_connectJob )
        return;

    m_connectJob->owner = nullptr;
    m_connectJob.reset();
    m_btnTest->SetLabel( _( "Test Connection" ) );
}


void DIALOG_DATABASE_LIB_SETTINGS::onConnectFinished( const std::shared_ptr<CONNECT_JOB>& aJob )
{
    if( aJob != m_connectJob )
        return;

    m_connectJob.reset();
    m_btnTest->SetLabel( _( "Test Connection" ) );

    if( aJob->connected )
    {
        m_conn = std::move( aJob->conn );
        m_tables = std::move( aJob->tables );
        setConnectionStatus( wxString::Format( wxPLURAL( "Connected; %zu table or view found.",
                                                         "Connected; %zu tables and views found.",
                                                         static_cast<unsigned>( m_tables.size() ) ),
                                               m_tables.size() ),
                             false );
    }
    else
    {
        setConnectionStatus( describeFailure( aJob->conn->GetLastFailure(), m_settings->m_Source ), true,
                             From_UTF8( aJob->conn->GetLastError() ) );
    }

    refreshTables();
    autofillTable();
    loadDataGrid();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::refreshTables()
{
    SCOPED_SET_RESET<bool> loading( m_loading, true );

    wxArrayString tables;

    for( const std::string& name : m_tables )
        tables.Add( From_UTF8( name ) );

    wxString table = m_cbTable->GetValue();
    m_cbTable->Set( tables );
    m_cbTable->ChangeValue( table );
    loadColumnChoices();
}


void DIALOG_DATABASE_LIB_SETTINGS::setConnectionStatus( const wxString& aText, bool aError,
                                                        const wxString& aDetails )
{
    m_stConnectionStatus->SetLabel( aText );
    m_stConnectionStatus->SetToolTip( aDetails.IsEmpty() ? aText : aText + wxS( "\n\n" ) + aDetails );
    m_stConnectionStatus->SetForegroundColour( aError ? *wxRED : wxNullColour );

    // SetLabel() resizes the control to its text; layout shrinks it back so the end ellipsizes
    relayoutPage( m_connectionPage );
}


DATABASE_LIB_TABLE* DIALOG_DATABASE_LIB_SETTINGS::currentLibrary()
{
    if( m_currentLibrary < 0 || m_currentLibrary >= static_cast<int>( m_settings->m_Tables.size() ) )
        return nullptr;

    return &m_settings->m_Tables[m_currentLibrary];
}


const std::vector<std::string>& DIALOG_DATABASE_LIB_SETTINGS::columnsFor( const std::string& aTable )
{
    static const std::vector<std::string> empty;

    if( !m_conn || aTable.empty() )
        return empty;

    auto it = m_columns.find( aTable );

    if( it == m_columns.end() )
    {
        it = m_columns.emplace( aTable, std::vector<std::string>() ).first;
        m_conn->GetColumns( aTable, it->second );
    }

    return it->second;
}


void DIALOG_DATABASE_LIB_SETTINGS::loadLibraryList()
{
    wxArrayString names;

    for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
        names.Add( displayName( table ) );

    m_libList->Set( names );

    if( m_currentLibrary >= 0 )
        m_libList->SetSelection( m_currentLibrary );

    updateLibraryListHints();
}


void DIALOG_DATABASE_LIB_SETTINGS::updateLibraryListHints()
{
    bool empty = m_settings->m_Tables.empty();

    if( m_stNoLibraries->IsShown() == empty && m_btnAddFirstLibrary->IsShown() == empty )
        return;

    m_stNoLibraries->Show( empty );
    m_btnAddFirstLibrary->Show( empty );
    relayoutPage( m_librariesPage );
}


void DIALOG_DATABASE_LIB_SETTINGS::loadLibrary()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    m_autoTable.clear();
    m_autoRoles.clear();
    m_autoFields.clear();
    m_userRoles.clear();

    {
        SCOPED_SET_RESET<bool> loading( m_loading, true );

        for( wxWindow* control : std::initializer_list<wxWindow*>{ m_txtLibName, m_cbTable, m_fieldsGrid,
                                                                   m_btnAddField, m_btnMoveFieldUp,
                                                                   m_btnMoveFieldDown, m_btnRemoveField,
                                                                   m_btnAddUnmapped, m_btnDuplicateLibrary,
                                                                   m_btnRemoveLibrary, m_btnMoveLibraryUp,
                                                                   m_btnMoveLibraryDown } )
        {
            control->Enable( lib != nullptr );
        }

        for( const COLUMN_ROLE& role : m_roles )
            role.combo->Enable( lib != nullptr );

        m_txtLibName->ChangeValue( lib ? From_UTF8( lib->name ) : wxString() );
        m_cbTable->ChangeValue( lib ? From_UTF8( lib->table ) : wxString() );

        loadColumnChoices();
        loadFields();
    }

    relayoutPage( m_librariesPage );

    // Another library may sample the same table with different column roles
    loadDataGrid();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::loadColumnChoices()
{
    DATABASE_LIB_TABLE  empty;
    DATABASE_LIB_TABLE* lib = currentLibrary();
    DATABASE_LIB_TABLE& shown = lib ? *lib : empty;

    wxArrayString columns;

    for( const std::string& column : columnsFor( shown.table ) )
        columns.Add( From_UTF8( column ) );

    // Optional mappings offer a blank entry to clear them
    wxArrayString optionalColumns = columns;
    optionalColumns.Insert( wxEmptyString, 0 );

    SCOPED_SET_RESET<bool> loading( m_loading, true );

    for( const COLUMN_ROLE& role : m_roles )
    {
        role.combo->Set( role.optional ? optionalColumns : columns );
        role.combo->ChangeValue( From_UTF8( role.value( shown ) ) );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_COMBOBOX( columns ) );
    m_fieldsGrid->SetColAttr( FIELD_COL_COLUMN, attr );

    updateAutofillStyle();
    updateTableWarning();
    updateColumnWarnings();
}


void DIALOG_DATABASE_LIB_SETTINGS::markUserRole( const COLUMN_ROLE& aRole, const std::string& aValue )
{
    m_userRoles.insert( aRole.combo );
    m_autoRoles.erase( aRole.combo );
    m_newChoices[aRole.autofillId] = aValue;
}


void DIALOG_DATABASE_LIB_SETTINGS::updateAutofillStyle()
{
    for( const COLUMN_ROLE& role : m_roles )
    {
        std::string value = TO_UTF8( role.combo->GetValue() );
        bool        autofilled = !value.empty() && isAutofilled( m_autoRoles, role.combo, value );

        wxFont font = role.combo->GetFont();
        wxFontStyle style = autofilled ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;

        // A new font on a combo that is being typed in makes Windows expand the typed character to a matching entry
        // and select it, so the next key replaces it.  The style follows when focus leaves.
        wxWindow* focus = wxWindow::FindFocus();
        bool      typing = focus && ( focus == role.combo || focus->GetParent() == role.combo );

        if( font.GetStyle() != style && !typing )
        {
            font.SetStyle( style );
            role.combo->SetFont( font );
        }
    }
}


void DIALOG_DATABASE_LIB_SETTINGS::storeLibrary()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib )
        return;

    lib->name = TO_UTF8( m_txtLibName->GetValue() );

    // The loader strips '/' because it separates the nickname from the symbol name
    std::erase( lib->name, '/' );

    lib->table = TO_UTF8( m_cbTable->GetValue() );

    for( const COLUMN_ROLE& role : m_roles )
        role.value( *lib ) = TO_UTF8( role.combo->GetValue() );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnLibrarySelected( wxCommandEvent& aEvent )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    storeLibrary();
    m_currentLibrary = m_libList->GetSelection();
    loadLibrary();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnAddLibrary( wxCommandEvent& aEvent )
{
    storeLibrary();
    m_flagMissing = false;
    m_settings->m_Tables.emplace_back();
    m_currentLibrary = static_cast<int>( m_settings->m_Tables.size() ) - 1;
    loadLibraryList();
    loadLibrary();
    m_txtLibName->SetFocus();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnDuplicateLibrary( wxCommandEvent& aEvent )
{
    storeLibrary();

    if( DATABASE_LIB_TABLE* lib = currentLibrary() )
    {
        DATABASE_LIB_TABLE copy = *lib;
        copy.name += "-copy";
        m_settings->m_Tables.insert( m_settings->m_Tables.begin() + m_currentLibrary + 1, std::move( copy ) );
        m_currentLibrary++;
        loadLibraryList();
        loadLibrary();
    }
}


void DIALOG_DATABASE_LIB_SETTINGS::moveLibrary( int aDelta )
{
    storeLibrary();

    int target = m_currentLibrary + aDelta;

    if( m_currentLibrary < 0 || target < 0 || target >= static_cast<int>( m_settings->m_Tables.size() ) )
    {
        wxBell();
        return;
    }

    std::swap( m_settings->m_Tables[m_currentLibrary], m_settings->m_Tables[target] );
    m_currentLibrary = target;
    loadLibraryList();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnMoveLibraryUp( wxCommandEvent& aEvent )
{
    moveLibrary( -1 );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnMoveLibraryDown( wxCommandEvent& aEvent )
{
    moveLibrary( 1 );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnRemoveLibrary( wxCommandEvent& aEvent )
{
    if( !currentLibrary() )
        return;

    m_settings->m_Tables.erase( m_settings->m_Tables.begin() + m_currentLibrary );
    m_currentLibrary = std::min( m_currentLibrary, static_cast<int>( m_settings->m_Tables.size() ) - 1 );
    loadLibraryList();
    loadLibrary();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnLibraryChanged( wxCommandEvent& aEvent )
{
    if( m_loading )
        return;

    storeLibrary();

    if( DATABASE_LIB_TABLE* lib = currentLibrary() )
        m_libList->SetString( m_currentLibrary, displayName( *lib ) );

    for( const COLUMN_ROLE& role : m_roles )
    {
        if( aEvent.GetEventObject() == role.combo )
        {
            markUserRole( role, TO_UTF8( role.combo->GetValue() ) );
        }
    }

    if( aEvent.GetEventObject() == m_txtLibName )
        autofillTable();

    updateAutofillStyle();
    updateTableWarning();
    updateColumnWarnings();
    updateDataLabels();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnTableChanged( wxCommandEvent& aEvent )
{
    if( m_loading )
        return;

    storeLibrary();
    m_autoTable.clear();

    DATABASE_LIB_TABLE* lib = currentLibrary();

    // Typing a partial name would otherwise query the catalog for every keystroke
    if( lib && ( m_tables.empty() || alg::contains( m_tables, lib->table ) ) )
    {
        loadColumnChoices();
        autofillColumns();
    }

    updateTableWarning();
    updateColumnWarnings();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::autofillTable()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib || m_tables.empty() || ( !lib->table.empty() && lib->table != m_autoTable ) )
        return;

    std::string table = DB_AUTOFILL::MatchTable( m_tables, lib->name );

    if( table.empty() || table == lib->table )
        return;

    lib->table = table;
    m_autoTable = table;

    {
        SCOPED_SET_RESET<bool> loading( m_loading, true );
        m_cbTable->ChangeValue( From_UTF8( table ) );
    }

    // Reloads the column choices and, by sampling the table, the data grid
    autofillColumns();
}


void DIALOG_DATABASE_LIB_SETTINGS::autofillColumns()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib || !alg::contains( m_tables, lib->table ) )
        return;

    // The sample shows which columns repeat, and a repeating column cannot be the key
    ensureSample( lib->table );

    // Mappings the user made keep their columns, which are then not offered to other roles
    std::set<std::string>              taken;
    std::vector<const COLUMN_ROLE*>    roles;
    std::vector<const AUTOFILL_FIELD*> fields;
    std::vector<DB_AUTOFILL::ROLE>     requests;

    for( const COLUMN_ROLE& role : m_roles )
    {
        const std::string& value = role.value( *lib );

        if( m_userRoles.contains( role.combo )
            || ( !value.empty() && !isAutofilled( m_autoRoles, role.combo, value ) ) )
        {
            taken.insert( value );
            continue;
        }

        roles.push_back( &role );
        requests.push_back( autofillRole( role.autofillId, role.terms ) );

        if( role.combo == m_cbKeyCol && m_sampleTable == lib->table )
        {
            for( const auto& [column, repeats] : m_sampleRepeats )
            {
                if( repeats > 0 )
                    requests.back().excluded.insert( column );
            }
        }
    }

    for( const AUTOFILL_FIELD& def : autofillFields() )
    {
        const DATABASE_FIELD_MAPPING* field = findField( *lib, def.name );

        if( field && !isAutofilled( m_autoFields, def.name, field->column ) )
            continue;

        fields.push_back( &def );
        requests.push_back( autofillRole( fieldChoiceId( def.name ), def.terms ) );
    }

    for( const DATABASE_FIELD_MAPPING& field : lib->fields )
    {
        if( !isAutofilled( m_autoFields, field.name, field.column ) )
            taken.insert( field.column );
    }

    std::vector<std::string> columns;

    for( const std::string& column : columnsFor( lib->table ) )
    {
        if( !taken.contains( column ) )
            columns.push_back( column );
    }

    std::vector<std::string> chosen = DB_AUTOFILL::AssignColumns( columns, requests );

    for( size_t ii = 0; ii < roles.size(); ++ii )
    {
        roles[ii]->value( *lib ) = chosen[ii];
        m_autoRoles[roles[ii]->combo] = chosen[ii];
    }

    for( size_t ii = 0; ii < fields.size(); ++ii )
    {
        const AUTOFILL_FIELD&   def = *fields[ii];
        const std::string&      column = chosen[roles.size() + ii];
        DATABASE_FIELD_MAPPING* field = findField( *lib, def.name );

        if( column.empty() )
        {
            if( field )
                lib->fields.erase( lib->fields.begin() + ( field - lib->fields.data() ) );

            m_autoFields.erase( def.name );
        }
        else
        {
            if( field )
                field->column = column;
            else
                lib->fields.emplace_back( column, def.name, def.visibleOnAdd, def.visibleOnAdd, false, false );

            m_autoFields[def.name] = column;
        }
    }

    loadColumnChoices();
    loadFields();
    updateDataLabels();
}


void DIALOG_DATABASE_LIB_SETTINGS::loadFields()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    m_fieldsGrid->ClearRows();

    if( !lib )
        return;

    m_fieldsGrid->AppendRows( static_cast<int>( lib->fields.size() ) );

    auto boolCell =
            []( bool aValue )
            {
                return aValue ? wxS( "1" ) : wxS( "" );
            };

    for( int row = 0; row < static_cast<int>( lib->fields.size() ); ++row )
    {
        const DATABASE_FIELD_MAPPING& field = lib->fields[row];

        m_fieldsGrid->SetCellValue( row, FIELD_COL_COLUMN, From_UTF8( field.column ) );
        m_fieldsGrid->SetCellValue( row, FIELD_COL_NAME, From_UTF8( field.name ) );
        m_fieldsGrid->SetCellValue( row, FIELD_COL_VISIBLE_ON_ADD, boolCell( field.visible_on_add ) );
        m_fieldsGrid->SetCellValue( row, FIELD_COL_VISIBLE_IN_CHOOSER, boolCell( field.visible_in_chooser ) );
        m_fieldsGrid->SetCellValue( row, FIELD_COL_SHOW_NAME, boolCell( field.show_name ) );
        m_fieldsGrid->SetCellValue( row, FIELD_COL_INHERIT, boolCell( field.inherit_properties ) );
    }

    // Counts the header too, since empty rows give no width and the labels would overlap
    for( int col = 0; col < m_fieldsGrid->GetNumberCols(); ++col )
        m_fieldsGrid->SetColSize( col, m_fieldsGrid->GetVisibleWidth( col ) );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnFieldCellChanged( wxGridEvent& aEvent )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();
    int                 row = aEvent.GetRow();

    if( !lib || row < 0 || row >= static_cast<int>( lib->fields.size() ) )
        return;

    // Edit in place so each mapping keeps the raw JSON it was loaded from
    DATABASE_FIELD_MAPPING& field = lib->fields[row];
    wxString                value = m_fieldsGrid->GetCellValue( row, aEvent.GetCol() );
    bool                    checked = value == wxS( "1" );

    switch( aEvent.GetCol() )
    {
    case FIELD_COL_COLUMN:             field.column = TO_UTF8( value );     break;
    case FIELD_COL_NAME:               field.SetName( TO_UTF8( value ) );   break;
    case FIELD_COL_VISIBLE_ON_ADD:     field.visible_on_add = checked;      break;
    case FIELD_COL_VISIBLE_IN_CHOOSER: field.visible_in_chooser = checked;  break;
    case FIELD_COL_SHOW_NAME:          field.show_name = checked;           break;
    case FIELD_COL_INHERIT:            field.inherit_properties = checked;  break;
    default:                                                                break;
    }

    if( aEvent.GetCol() == FIELD_COL_COLUMN || aEvent.GetCol() == FIELD_COL_NAME )
    {
        for( const AUTOFILL_FIELD& def : autofillFields() )
        {
            if( wxString::FromUTF8( field.name ).IsSameAs( def.name, false ) )
            {
                m_autoFields.erase( def.name );
                m_newChoices[fieldChoiceId( def.name )] = field.column;
            }
        }

        updateDataLabels();
    }

    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnAddField( wxCommandEvent& aEvent )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib )
        return;

    m_fieldsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                lib->fields.emplace_back();
                m_fieldsGrid->AppendRows( 1 );
                return { m_fieldsGrid->GetNumberRows() - 1, FIELD_COL_COLUMN };
            } );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnMoveFieldUp( wxCommandEvent& aEvent )
{
    if( DATABASE_LIB_TABLE* lib = currentLibrary() )
        m_fieldsGrid->OnMoveRowUp( [&]( int row ) { swapFields( *lib, row, row - 1 ); } );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnMoveFieldDown( wxCommandEvent& aEvent )
{
    if( DATABASE_LIB_TABLE* lib = currentLibrary() )
        m_fieldsGrid->OnMoveRowDown( [&]( int row ) { swapFields( *lib, row, row + 1 ); } );
}


void DIALOG_DATABASE_LIB_SETTINGS::swapFields( DATABASE_LIB_TABLE& aLib, int aRow, int aOther )
{
    std::swap( aLib.fields[aRow], aLib.fields[aOther] );
    m_fieldsGrid->SwapRows( aRow, aOther );
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnRemoveField( wxCommandEvent& aEvent )
{
    if( DATABASE_LIB_TABLE* lib = currentLibrary() )
    {
        m_fieldsGrid->OnDeleteRows(
                [&]( int row )
                {
                    lib->fields.erase( lib->fields.begin() + row );
                    m_fieldsGrid->DeleteRows( row, 1 );
                } );

        updateDataLabels();
        markDirty();
    }
}


void DIALOG_DATABASE_LIB_SETTINGS::OnAddUnmappedColumns( wxCommandEvent& aEvent )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib || !m_fieldsGrid->CommitPendingChanges() )
        return;

    std::set<std::string> mapped;

    for( const COLUMN_ROLE& role : m_roles )
        mapped.insert( role.value( *lib ) );

    for( const DATABASE_FIELD_MAPPING& field : lib->fields )
        mapped.insert( field.column );

    for( const std::string& column : columnsFor( lib->table ) )
    {
        if( !mapped.contains( column ) )
            lib->fields.emplace_back( column, column, false, false, false, false );
    }

    loadFields();
    updateDataLabels();
    markDirty();
}


const DATABASE_FIELD_MAPPING* DIALOG_DATABASE_LIB_SETTINGS::fieldForColumn( const std::string& aColumn )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    if( !lib )
        return nullptr;

    for( const DATABASE_FIELD_MAPPING& field : lib->fields )
    {
        if( field.column == aColumn )
            return &field;
    }

    return nullptr;
}


void DIALOG_DATABASE_LIB_SETTINGS::loadDataGrid()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    // Rebuilding moves the grid cursor, which must not select a symbol in the preview
    SCOPED_SET_RESET<bool> loading( m_loading, true );

    m_dataGrid->BeginBatch();
    m_dataGrid->ClearRows();

    if( m_dataGrid->GetNumberCols() > 0 )
        m_dataGrid->DeleteCols( 0, m_dataGrid->GetNumberCols() );

    if( lib && !m_sampleTable.empty() && m_sampleTable == lib->table )
    {
        m_dataGrid->AppendCols( static_cast<int>( m_sampleColumns.size() ) );
        m_dataGrid->AppendRows( static_cast<int>( m_sampleRows.size() ) );

        for( int col = 0; col < static_cast<int>( m_sampleColumns.size() ); ++col )
        {
            for( int row = 0; row < static_cast<int>( m_sampleRows.size() ); ++row )
            {
                auto it = m_sampleRows[row].find( m_sampleColumns[col] );

                if( it == m_sampleRows[row].end() )
                    continue;

                if( const std::string* value = std::any_cast<std::string>( &it->second ) )
                    m_dataGrid->SetCellValue( row, col, From_UTF8( *value ) );
            }
        }

        // Labels first, so the sizing accounts for the role lines under each column name
        updateDataLabels();
        m_dataGrid->AutoSizeColumns( false );
    }
    else
    {
        updateDataLabels();
    }

    m_dataGrid->EndBatch();
}


void DIALOG_DATABASE_LIB_SETTINGS::updateSymbolExample()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();
    wxString            text;

    if( lib )
    {
        wxString symbol;

        if( m_sampleTable == lib->table && !m_sampleRows.empty() )
            symbol = SCH_IO_DATABASE::SymbolNameForRow( *lib, m_settings->m_GloballyUniqueKeys, m_sampleRows[0] );

        if( symbol.IsEmpty() )
        {
            wxString key = _( "<key>" );

            if( m_settings->m_GloballyUniqueKeys || lib->name.empty() )
                symbol = key;
            else
                symbol = From_UTF8( lib->name ) + wxS( "/" ) + key;
        }

        text = wxString::Format( _( "Symbols appear as %s" ), symbol );
    }

    m_stSymbolExample->SetLabel( text );
    m_stSymbolExample->SetToolTip( text );

    // SetLabel() resizes the control to its text; layout shrinks it back so the end ellipsizes
    relayoutPage( m_librariesPage );
}


void DIALOG_DATABASE_LIB_SETTINGS::updateDataLabels()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    updateSymbolExample();

    if( !lib )
    {
        m_stDataStatus->SetLabel( _( "Choose a library on the Libraries page to see its table." ) );
        return;
    }

    if( m_sampleTable.empty() || m_sampleTable != lib->table )
    {
        m_stDataStatus->SetLabel( m_conn ? _( "No rows loaded." ) : _( "Not connected." ) );
        return;
    }

    std::map<std::string, wxString> roles;

    for( const COLUMN_ROLE& role : m_roles )
    {
        const std::string& column = role.value( *lib );

        if( !column.empty() )
            roles.emplace( column, role.label );
    }

    for( int col = 0; col < m_dataGrid->GetNumberCols(); ++col )
    {
        const std::string& column = m_sampleColumns[col];
        wxString           label = From_UTF8( column );

        if( auto role = roles.find( column ); role != roles.end() )
            label += wxS( "\n" ) + role->second;
        else if( const DATABASE_FIELD_MAPPING* field = fieldForColumn( column ) )
            label += wxS( "\n" ) + wxString::Format( _( "field '%s'" ), field->name_wx );

        m_dataGrid->SetColLabelValue( col, label );
    }

    const unsigned sampled = static_cast<unsigned>( m_sampleRows.size() );
    wxString       rows;

    if( m_sampleRows.size() < PREVIEW_ROW_LIMIT )
        rows = wxString::Format( wxPLURAL( "%zu row", "%zu rows", sampled ), m_sampleRows.size() );
    else
        rows = wxString::Format( wxPLURAL( "first %zu row", "first %zu rows", sampled ), m_sampleRows.size() );

    m_stDataStatus->SetLabel( wxString::Format( _( "Library '%s', table '%s', %s. Click a column header to map it." ),
                                                displayName( *lib ), From_UTF8( lib->table ), rows ) );
}


void DIALOG_DATABASE_LIB_SETTINGS::OnDataLabelRightClick( wxGridEvent& aEvent )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();
    int                 col = aEvent.GetCol();

    if( !lib || m_sampleTable != lib->table || col < 0 || col >= static_cast<int>( m_sampleColumns.size() )
        || !m_fieldsGrid->CommitPendingChanges() )
    {
        return;
    }

    // A copy, since the preview timer can replace the sample while the popup menu runs its event loop
    const std::string column = m_sampleColumns[col];
    bool              mapped = fieldForColumn( column ) != nullptr;
    wxMenu            menu;
    int               fieldId = wxWindow::NewControlId();

    menu.Append( fieldId, mapped ? _( "Remove Field Mapping" ) : _( "Map as Field" ) );
    menu.AppendSeparator();

    std::map<int, const COLUMN_ROLE*> roleIds;

    for( const COLUMN_ROLE& role : m_roles )
    {
        int id = wxWindow::NewControlId();
        roleIds[id] = &role;
        menu.AppendCheckItem( id, wxString::Format( _( "Use for %s" ), role.label ) )
                ->Check( role.value( *lib ) == column );
    }

    int choice = GetPopupMenuSelectionFromUser( menu );

    if( auto it = roleIds.find( choice ); it != roleIds.end() )
    {
        const COLUMN_ROLE& role = *it->second;

        role.value( *lib ) = role.value( *lib ) == column ? std::string() : column;
        markUserRole( role, role.value( *lib ) );
        loadColumnChoices();
        updateDataLabels();
        markDirty();
        return;
    }

    if( choice != fieldId )
        return;

    if( mapped )
    {
        std::erase_if( lib->fields,
                       [&]( const DATABASE_FIELD_MAPPING& aField )
                       {
                           return aField.column == column;
                       } );
    }
    else
    {
        lib->fields.emplace_back( column, column, false, false, false, false );
    }

    loadFields();
    updateDataLabels();
    markDirty();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnDataCellSelected( wxGridEvent& aEvent )
{
    DATABASE_LIB_TABLE* lib = currentLibrary();
    int                 row = aEvent.GetRow();

    if( !m_loading && lib && row >= 0 && row < static_cast<int>( m_sampleRows.size() ) )
    {
        m_preview->SelectSymbol( SCH_IO_DATABASE::SymbolNameForRow( *lib, m_settings->m_GloballyUniqueKeys,
                                                                    m_sampleRows[row] ) );
    }

    aEvent.Skip();
}


void DIALOG_DATABASE_LIB_SETTINGS::showInfo( const wxString& aMessage, int aIcon )
{
    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();
    m_infoBar->ShowMessage( aMessage, aIcon );
}


void DIALOG_DATABASE_LIB_SETTINGS::onSplitterIdle( wxIdleEvent& aEvent )
{
    m_splitter->Unbind( wxEVT_IDLE, &DIALOG_DATABASE_LIB_SETTINGS::onSplitterIdle, this );

    // What the pages need in the pane around them.  A scrolled window accepts being narrower than its content, so
    // its own size says nothing, and the sizer's minimum does.
    int needed = 0;

    for( wxScrolledWindow* page : { m_connectionPage, m_librariesPage } )
    {
        int overhead = m_editPanel->GetSize().x - page->GetClientSize().x;

#ifndef __WXOSX__
        // Classic scrollbars take room from the page once the page grows taller than its pane
        if( !page->HasScrollbar( wxVERTICAL ) )
            overhead += wxSystemSettings::GetMetric( wxSYS_VSCROLL_X, page );
#endif

        needed = std::max( needed, page->GetSizer()->CalcMin().x + overhead );
    }

    // The preview is only useful with room for its tree columns and symbol, so the dialog grows to give it that
    const int previewWidth = FromDIP( PREVIEW_MIN_WIDTH_DIP );
    const int deficit = needed + m_splitter->GetSashSize() + previewWidth - m_splitter->GetClientSize().x;

    if( deficit > 0 )
    {
        wxRect display = wxDisplay( this ).GetClientArea();
        wxSize size = GetSize();

        SetSize( std::min( size.x + deficit, display.width ), size.y );
    }

    // Resizing then only changes the preview, so the pages keep the width they need
    m_splitter->SetSashGravity( 0.0 );
    m_splitter->SetSashPosition( std::clamp( needed, m_splitter->GetMinimumPaneSize(),
                                             std::max( m_splitter->GetClientSize().x - previewWidth, 0 ) ) );

    // Narrower than this the pages are cut off, and where scrollbars are overlays nothing says they scroll
    wxSize minSize = GetMinSize();

    minSize.x = std::max( minSize.x, GetSize().x - m_splitter->GetClientSize().x + needed + m_splitter->GetSashSize()
                                             + m_splitter->GetMinimumPaneSize() );
    SetMinSize( minSize );
}


void DIALOG_DATABASE_LIB_SETTINGS::markDirty()
{
    m_previewDebounce.Restart();
}


bool DIALOG_DATABASE_LIB_SETTINGS::ensureSample( const std::string& aTable )
{
    if( !m_conn || aTable.empty() )
        return false;

    if( m_sampleTable == aTable )
        return true;

    wxBusyCursor busy;

    // Only a successful sample is cached, so an error stays visible until the table reads
    m_sampleTable.clear();
    m_sampleRepeats.clear();

    bool ok = m_conn->SelectSample( aTable, PREVIEW_ROW_LIMIT, m_sampleColumns, m_sampleRows );

    if( ok )
    {
        m_sampleTable = aTable;
        m_sampleRepeats = DB_AUTOFILL::RepeatCounts( m_sampleRows );
    }

    loadDataGrid();
    return ok;
}


void DIALOG_DATABASE_LIB_SETTINGS::updateTableWarning()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    wxArrayString warnings;

    if( m_flagMissing && lib && lib->table.empty() )
        warnings.Add( _( "Choose a table." ) );
    else if( !m_tableError.IsEmpty() )
        warnings.Add( m_tableError );

    m_tableIcon->SetWarnings( warnings );
}


void DIALOG_DATABASE_LIB_SETTINGS::updateColumnWarnings()
{
    using FIELD = SCH_IO_DATABASE::PREVIEW_ISSUE::FIELD;

    DATABASE_LIB_TABLE* lib = currentLibrary();

    // A table name still being typed must not query the catalog for its columns
    const std::vector<std::string>* columns =
            lib && alg::contains( m_tables, lib->table ) ? &columnsFor( lib->table ) : nullptr;

    // The sample counts rows with the same key, which is only meaningful for the table it was read from
    size_t repeats = 0;

    if( lib && m_sampleTable == lib->table )
    {
        if( auto it = m_sampleRepeats.find( lib->key_col ); it != m_sampleRepeats.end() )
            repeats = it->second;
    }

    for( size_t ii = 0; ii < m_roles.size(); ++ii )
    {
        const COLUMN_ROLE& role = m_roles[ii];
        const bool         isKey = role.combo == m_cbKeyCol;
        wxArrayString      warnings;

        if( lib )
        {
            const std::string& value = role.value( *lib );

            if( value.empty() )
            {
                if( isKey && m_flagMissing )
                    warnings.Add( _( "Choose a key column." ) );
            }
            else if( columns && !columns->empty() && !alg::contains( *columns, value ) )
            {
                warnings.Add( wxString::Format( _( "Column '%s' is not in table '%s'." ), From_UTF8( value ),
                                                From_UTF8( lib->table ) ) );
            }

            if( isKey && repeats > 0 )
            {
                warnings.Add( wxString::Format(
                        wxPLURAL( "'%s' repeats in %zu of the first %zu row, and parts that share a key hide each "
                                  "other. Choose a column with a unique value per part.",
                                  "'%s' repeats in %zu of the first %zu rows, and parts that share a key hide each "
                                  "other. Choose a column with a unique value per part.",
                                  static_cast<unsigned>( m_sampleRows.size() ) ),
                        From_UTF8( lib->key_col ), repeats, m_sampleRows.size() ) );
            }
        }

        // The repeat warning already covers every key the preview found repeated
        if( isKey )
        {
            for( const wxString& message : previewIssues( FIELD::KEY ) )
                warnings.Add( message );

            if( repeats == 0 )
            {
                for( const wxString& message : previewIssues( FIELD::REPEATED_KEY ) )
                    warnings.Add( message );
            }
        }
        else if( role.combo == m_cbSymbolsCol )
        {
            for( const wxString& message : previewIssues( FIELD::SYMBOLS ) )
                warnings.Add( message );
        }
        else if( role.combo == m_cbPinsCol )
        {
            for( const wxString& message : previewIssues( FIELD::PIN_MAP ) )
                warnings.Add( message );
        }

        m_roleIcons[ii]->SetWarnings( warnings );
    }
}


wxArrayString DIALOG_DATABASE_LIB_SETTINGS::previewIssues( SCH_IO_DATABASE::PREVIEW_ISSUE::FIELD aField ) const
{
    wxArrayString messages;

    for( const SCH_IO_DATABASE::PREVIEW_ISSUE& issue : m_previewIssues )
    {
        if( issue.field == aField )
            messages.Add( issue.message );
    }

    return messages;
}


void DIALOG_DATABASE_LIB_SETTINGS::updatePreview()
{
    DATABASE_LIB_TABLE* lib = currentLibrary();

    auto clear =
            [&]( const wxString& aStatus, const wxString& aDetails = wxEmptyString )
            {
                m_tableError = aDetails.IsEmpty() ? wxString() : aStatus + wxS( "\n" ) + aDetails;
                m_previewIssues.clear();
                updateTableWarning();
                updateColumnWarnings();
                m_preview->SetSymbols( wxEmptyString, {} );
                m_preview->SetStatus( aStatus );
            };

    if( !lib )
        return clear( _( "Add a library on the Libraries page to preview it." ) );

    if( m_connectJob )
        return clear( _( "Connecting..." ) );

    if( !m_conn )
        return clear( _( "Not connected to a database." ) );

    if( lib->table.empty() )
        return clear( _( "Choose a table." ) );

    if( !ensureSample( lib->table ) )
    {
        return clear( wxString::Format( _( "Could not read table '%s'." ), From_UTF8( lib->table ) ),
                      From_UTF8( m_conn->GetLastError() ) );
    }

    m_tableError.Clear();
    updateTableWarning();

    m_previewIssues.clear();
    std::vector<std::unique_ptr<LIB_SYMBOL>> symbols = SCH_IO_DATABASE::BuildPreviewSymbols(
            m_libs, *lib, m_settings->m_GloballyUniqueKeys, m_sampleRows, &m_previewIssues );

    size_t   count = symbols.size();
    wxString status;

    if( lib->key_col.empty() )
        status = _( "Choose a key column." );
    else if( !m_sampleRows.empty() && count == 0 && !alg::contains( m_sampleColumns, lib->key_col ) )
        status = wxString::Format( _( "Key column '%s' is not in table '%s'." ), From_UTF8( lib->key_col ),
                                   From_UTF8( lib->table ) );
    else if( m_sampleRows.size() < PREVIEW_ROW_LIMIT )
        status = wxString::Format( wxPLURAL( "%zu symbol.", "%zu symbols.", static_cast<unsigned>( count ) ), count );
    else
        status = wxString::Format( wxPLURAL( "%zu symbol from the first %zu rows.",
                                             "%zu symbols from the first %zu rows.", static_cast<unsigned>( count ) ),
                                   count, m_sampleRows.size() );

    updateColumnWarnings();

    m_preview->SetSymbols( displayName( *lib ), std::move( symbols ) );
    m_preview->SetStatus( status );
}
