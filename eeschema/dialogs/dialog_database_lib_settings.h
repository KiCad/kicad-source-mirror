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

#ifndef KICAD_DIALOG_DATABASE_LIB_SETTINGS_H
#define KICAD_DIALOG_DATABASE_LIB_SETTINGS_H

#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <database/database_connection.h>
#include <database/database_lib_settings.h>
#include <database/odbc_backend.h>
#include <sch_io/database/sch_io_database.h>
#include <widgets/field_warning_icon.h>
#include <widgets/wx_debounced_action.h>
#include "dialog_database_lib_settings_base.h"

#include <wx/timer.h>

class DB_LIB_PREVIEW_PANEL;
class SYMBOL_LIBRARY_ADAPTER;


/**
 * Edits every setting of a .kicad_dbl file and previews the resulting symbols.
 *
 * The dialog works on its own copy of the settings and a connection of its own, so the library
 * in use is untouched until OK saves the file.
 */
class DIALOG_DATABASE_LIB_SETTINGS : public DIALOG_DATABASE_LIB_SETTINGS_BASE
{
public:
    /**
     * @param aDblPath is the settings file to edit; a missing file starts a new library, and an empty
     *                 path starts a new library whose file is chosen when the user clicks OK
     * @param aLibs resolves the symbols column for the preview; may be null
     * @param aDefaultDir is the folder the save dialog opens in for a library without a path
     */
    DIALOG_DATABASE_LIB_SETTINGS( wxWindow* aParent, const wxString& aDblPath, SYMBOL_LIBRARY_ADAPTER* aLibs,
                                  const wxString& aDefaultDir = wxEmptyString );

    ~DIALOG_DATABASE_LIB_SETTINGS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /// @return the settings file, which for a new library is known only once the dialog was accepted
    const wxString& GetSettingsPath() const { return m_path; }

protected:
    void OnSettingChanged( wxCommandEvent& aEvent ) override;
    void OnSettingSpinChanged( wxSpinEvent& aEvent ) override;
    void OnBackendChanged( wxCommandEvent& aEvent ) override;
    void OnFileTextChanged( wxCommandEvent& aEvent ) override;
    void OnBrowseFile( wxCommandEvent& aEvent ) override;
    void OnSourceTypeChanged( wxCommandEvent& aEvent ) override;
    void OnSourceChanged( wxCommandEvent& aEvent ) override;
    void OnSourceSpinChanged( wxSpinEvent& aEvent ) override;
    void OnInsertDriver( wxCommandEvent& aEvent ) override;
    void OnTestConnection( wxCommandEvent& aEvent ) override;
    void OnManageDataSources( wxCommandEvent& aEvent ) override;

    void OnLibrarySelected( wxCommandEvent& aEvent ) override;
    void OnAddLibrary( wxCommandEvent& aEvent ) override;
    void OnDuplicateLibrary( wxCommandEvent& aEvent ) override;
    void OnMoveLibraryUp( wxCommandEvent& aEvent ) override;
    void OnMoveLibraryDown( wxCommandEvent& aEvent ) override;
    void OnRemoveLibrary( wxCommandEvent& aEvent ) override;
    void OnLibraryChanged( wxCommandEvent& aEvent ) override;
    void OnTableChanged( wxCommandEvent& aEvent ) override;

    void OnFieldCellChanged( wxGridEvent& aEvent ) override;
    void OnAddField( wxCommandEvent& aEvent ) override;
    void OnMoveFieldUp( wxCommandEvent& aEvent ) override;
    void OnMoveFieldDown( wxCommandEvent& aEvent ) override;
    void OnRemoveField( wxCommandEvent& aEvent ) override;
    void OnAddUnmappedColumns( wxCommandEvent& aEvent ) override;

    void OnDataLabelRightClick( wxGridEvent& aEvent ) override;
    void OnDataCellSelected( wxGridEvent& aEvent ) override;

    void onSplitterIdle( wxIdleEvent& aEvent );

private:
    /// One column mapping on the Libraries page: its combo, its setting and its Data page label
    struct COLUMN_ROLE
    {
        wxComboBox*           combo;
        std::string&          ( *value )( DATABASE_LIB_TABLE& );
        bool                  optional;
        wxString              label;
        const char*           autofillId; ///< Key of the remembered column choices for this role
        std::vector<wxString> terms;      ///< English names to autofill from
    };

    void storeSource();

    /// Fills the backend choice with what this computer offers, plus the one the file already names
    void loadBackends();

    /// @return the backend chosen in the choice
    DATABASE_SOURCE_TYPE selectedBackend() const;

    void selectBackend( DATABASE_SOURCE_TYPE aType );

    /// @return true when the chosen backend has what it needs to try a connection
    bool sourceConfigured() const;

    /// Follows an edit of the data source by dropping the connection that no longer matches it
    void sourceEdited();

    /// Offers Access once the provider check, which does not block, has found the engine
    void onProviderTimer( wxTimerEvent& aEvent );

    /// Fills the data source list from the ODBC driver manager, keeping what is typed
    void loadDataSources();

    /// Shows which driver the chosen data source uses
    void updateDsnDriver();

    void updateSourceControls();

    /// Result of a connection attempt made on a worker thread
    struct CONNECT_JOB
    {
        /// Cleared on the UI thread when the attempt is abandoned; the worker never reads it
        DIALOG_DATABASE_LIB_SETTINGS*        owner = nullptr;
        std::unique_ptr<DATABASE_CONNECTION> conn;
        std::vector<std::string>             tables;
        bool                                 connected = false;

        /// Set by the worker before it opens anything, so a job that never ran can be destroyed anywhere
        std::atomic<bool>                    started = false;

        /// Lets the connection go on a pool thread, so a stalled close never blocks the UI thread while the app runs
        ~CONNECT_JOB();
    };

    /// Starts connecting with the current settings; the table list refreshes when it finishes
    void connect();

    /// Abandons a connection attempt in progress, whose result is then discarded
    void cancelConnect();

    void onConnectFinished( const std::shared_ptr<CONNECT_JOB>& aJob );

    /// Refreshes the table list and everything built on it after the connection changes
    void refreshTables();

    void loadLibraryList();
    void loadLibrary();
    void storeLibrary();
    void moveLibrary( int aDelta );
    void loadFields();
    void swapFields( DATABASE_LIB_TABLE& aLib, int aRow, int aOther );
    void loadDataGrid();

    /// Shows how the current library's symbols will be named
    void updateSymbolExample();

    /// Refreshes only the Data page column labels, which follow mapping edits
    void updateDataLabels();

    void loadColumnChoices();

    /// Records that the user chose aValue for aRole, so autofill leaves it alone
    void markUserRole( const COLUMN_ROLE& aRole, const std::string& aValue );

    /// Reads aTable's first rows unless they are already loaded; false if they could not be read
    bool ensureSample( const std::string& aTable );

    /// Marks the table field when it is required and missing or could not be read
    void updateTableWarning();

    /// Marks each column field for a missing key, a column the table does not have, keys that repeat in the sample
    /// or the problems the preview found in it
    void updateColumnWarnings();

    /// @return the messages of the preview issues found in aField
    wxArrayString previewIssues( SCH_IO_DATABASE::PREVIEW_ISSUE::FIELD aField ) const;

    /// Shows the first problem that stops the settings from being saved
    bool validateLibraries();

    /// @return the folder ${CWD} stands for, which is provisional until a new library is saved
    wxString cwdDir() const;

    /// Asks where to save a library that has no file yet; false if the user cancelled
    bool chooseSavePath();

    /// Italicizes the role combos that still hold an autofilled value
    void updateAutofillStyle();

    /// Shows the empty state of the library list only while it is empty
    void updateLibraryListHints();

    /// Every setting as the file would hold it, plus where the password is kept
    std::string settingsSnapshot();

    enum class CLOSE_ACTION
    {
        DISCARD, ///< Nothing to keep, or the user chose to discard it
        STAY,    ///< The user wants to keep editing
        SAVED    ///< The user chose to save, which ended the dialog with wxID_OK
    };

    /// Asks what to do with unsaved edits before the dialog closes without OK
    CLOSE_ACTION confirmClose();

    void onCancel( wxCommandEvent& aEvent );
    void onCloseWindow( wxCloseEvent& aEvent );

    /// Picks the table named like the library while the user has not chosen one
    void autofillTable();

    /// Maps columns of the chosen table to roles and fields the user has not set
    void autofillColumns();

    const std::vector<std::string>& columnsFor( const std::string& aTable );

    DATABASE_LIB_TABLE* currentLibrary();

    /// @return the field mapped to aColumn in the current library, or nullptr
    const DATABASE_FIELD_MAPPING* fieldForColumn( const std::string& aColumn );

    /// Restarts the debounce before the preview is rebuilt
    void markDirty();

    /// Replaces the info bar message, which can be dismissed
    void showInfo( const wxString& aMessage, int aIcon );
    void updatePreview();

    /// Shows a one-line status; the full driver report goes in the tooltip
    void setConnectionStatus( const wxString& aText, bool aError, const wxString& aDetails = wxEmptyString );

    wxString                               m_path;
    wxString                               m_defaultDir;
    SYMBOL_LIBRARY_ADAPTER*                m_libs;
    std::unique_ptr<DATABASE_LIB_SETTINGS> m_settings;
    std::unique_ptr<DATABASE_CONNECTION>   m_conn;
    std::shared_ptr<CONNECT_JOB>           m_connectJob;
    DB_LIB_PREVIEW_PANEL*                  m_preview;
    std::vector<COLUMN_ROLE>               m_roles;

    /// Index into m_settings->m_Tables of the library shown on the Libraries page, or -1
    int  m_currentLibrary;

    /// Set while controls are filled from the settings, so their change events are ignored
    bool m_loading;

    /// True until a new library has been given a file
    bool m_unnamed;

    /// Set once OK found a missing table or key, which is then flagged inline
    bool m_flagMissing;

    /// False when the file is unreadable or from a newer KiCad, since saving would lose its contents
    bool m_canSave;

    /// Coalesces edits into one preview refresh
    WX_DEBOUNCED_ACTION m_previewDebounce;

    /// Connects once typing in the file field pauses
    WX_DEBOUNCED_ACTION m_connectDebounce;

    /// Why the table could not be read, shown on the table warning icon
    wxString m_tableError;

    /// Problems found while building the preview symbols, shown on the icons of the fields they came from
    SCH_IO_DATABASE::PREVIEW_ISSUES m_previewIssues;

    FIELD_WARNING_ICON*              m_tableIcon;
    std::vector<FIELD_WARNING_ICON*> m_roleIcons; ///< One per entry of m_roles

    /// DSN login and keyring password at load, so an untouched login never rewrites the keyring
    DATABASE_SOURCE m_keyringLogin;

    /// The type each entry of the backend choice stands for
    std::vector<DATABASE_SOURCE_TYPE> m_backendTypes;

    /// The database file as picked, which stays absolute so the stored path can follow the save location
    wxString m_dbFile;

    /// The source as loaded, which an unknown type is restored from and source.path is kept from until a file is picked
    DATABASE_SOURCE m_loadedSource;

    bool m_pathEdited = false;

    wxTimer m_providerTimer;
    int     m_providerPolls = 0;

    /// Autofilled values for the current library, which a later autofill may replace
    std::string                        m_autoTable;
    std::map<wxComboBox*, std::string> m_autoRoles;
    std::map<std::string, std::string> m_autoFields;

    /// Role combos the user set for the current library, which autofill leaves alone
    std::set<wxComboBox*> m_userRoles;

    /// Columns the user chose this session, keyed like EESCHEMA_SETTINGS::m_DatabaseColumnChoices
    std::map<std::string, std::string> m_newChoices;

    std::vector<ODBC_BACKEND::DATA_SOURCE>          m_dataSources;
    std::vector<std::string>                        m_tables;
    std::map<std::string, std::vector<std::string>> m_columns;

    /// Rows of m_sampleTable shared by the Data page and the preview
    std::string                           m_sampleTable;
    std::vector<std::string>              m_sampleColumns;
    std::vector<DATABASE_CONNECTION::ROW> m_sampleRows;
    std::map<std::string, size_t>         m_sampleRepeats;

    /// settingsSnapshot() when the dialog opened, to tell whether closing loses anything
    std::string m_cleanSnapshot;
};

#endif //KICAD_DIALOG_DATABASE_LIB_SETTINGS_H
