/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialog_import_symbol_select.h"

#include <bitmaps.h>
#include <confirm.h>
#include <kidialog.h>
#include <kiway.h>
#include <lib_symbol.h>
#include <lib_symbol_library_manager.h>
#include <symbol_edit_frame.h>
#include <widgets/symbol_preview_widget.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/dataview.h>


DIALOG_IMPORT_SYMBOL_SELECT::DIALOG_IMPORT_SYMBOL_SELECT( SYMBOL_EDIT_FRAME* aParent,
                                                          const wxString& aFilePath,
                                                          const wxString& aDestLibrary,
                                                          SCH_IO_MGR::SCH_FILE_T aPluginType ) :
        DIALOG_IMPORT_SYMBOL_SELECT_BASE( aParent ),
        m_frame( aParent ),
        m_filePath( aFilePath ),
        m_destLibrary( aDestLibrary ),
        m_pluginType( aPluginType ),
        m_plugin( SCH_IO_MGR::FindPlugin( aPluginType ) ),
        m_preview( nullptr )
{
    wxFileName fn( aFilePath );
    SetTitle( wxString::Format( _( "Import Symbols from %s" ), fn.GetFullName() ) );

    m_symbolList->AppendToggleColumn( wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, 30 );
    m_symbolList->AppendIconTextColumn( _( "Symbol" ), wxDATAVIEW_CELL_INERT, 250 );

    m_symbolList->Connect( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                           wxDataViewEventHandler( DIALOG_IMPORT_SYMBOL_SELECT::onItemChecked ),
                           nullptr, this );

    m_preview = new SYMBOL_PREVIEW_WIDGET( m_previewPanel, &Kiway(), false,
                                           EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
    m_previewSizer->Add( m_preview, 1, wxEXPAND, 0 );
    m_previewPanel->Layout();
    m_unitChoice->Enable( false );

    SetupStandardButtons( { { wxID_OK, _( "Import" ) } } );
    m_sdbSizerOK->Disable();

    SetInitialFocus( m_searchCtrl );
    finishDialogSettings();
}


DIALOG_IMPORT_SYMBOL_SELECT::~DIALOG_IMPORT_SYMBOL_SELECT()
{
    m_symbolList->Disconnect( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                              wxDataViewEventHandler( DIALOG_IMPORT_SYMBOL_SELECT::onItemChecked ),
                              nullptr, this );
}


bool DIALOG_IMPORT_SYMBOL_SELECT::TransferDataToWindow()
{
    if( !loadSymbols() )
        return false;

    m_manager.BuildDependencyMaps();
    refreshList();
    updateStatusLine();

    return true;
}


bool DIALOG_IMPORT_SYMBOL_SELECT::TransferDataFromWindow()
{
    return resolveConflicts();
}


bool DIALOG_IMPORT_SYMBOL_SELECT::loadSymbols()
{
    if( !m_plugin )
    {
        DisplayError( this, _( "Unable to find a plugin to read this library." ) );
        return false;
    }

    wxArrayString symbolNames;

    try
    {
        m_plugin->EnumerateSymbolLib( symbolNames, m_filePath );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayErrorMessage( this,
                             wxString::Format( _( "Cannot read symbol library '%s'." ), m_filePath ),
                             ioe.What() );
        return false;
    }

    if( symbolNames.empty() )
    {
        DisplayError( this, wxString::Format( _( "Symbol library '%s' is empty." ), m_filePath ) );
        return false;
    }

    // Get the library manager to check for existing symbols
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_frame->GetLibManager();
    m_manager.Clear();

    for( const wxString& name : symbolNames )
    {
        wxString parentName;
        bool isPower = false;
        LIB_SYMBOL* sym = nullptr;

        try
        {
            sym = m_plugin->LoadSymbol( m_filePath, name );

            if( sym )
            {
                parentName = sym->GetParentName();
                isPower = sym->IsPower();
            }
        }
        catch( const IO_ERROR& )
        {
            // Symbol failed to load - still add it to list but without full info
        }

        // Add to manager - don't pass the symbol pointer since LoadSymbol returns
        // a cached pointer owned by the plugin, not a new allocation
        m_manager.AddSymbol( name, parentName, isPower, nullptr );
    }

    m_manager.CheckExistingSymbols(
            [&libMgr, this]( const wxString& name ) {
                return libMgr.SymbolExists( name, m_destLibrary );
            } );

    return true;
}


void DIALOG_IMPORT_SYMBOL_SELECT::refreshList()
{
    m_symbolList->DeleteAllItems();
    m_listIndices.clear();

    int index = 0;

    for( const wxString& name : m_manager.GetSymbolNames() )
    {
        if( !matchesFilter( name ) )
        {
            m_listIndices[name] = -1;
            continue;
        }

        m_listIndices[name] = index;

        const SYMBOL_IMPORT_INFO* info = m_manager.GetSymbolInfo( name );

        if( !info )
            continue;

        wxVector<wxVariant> data;
        wxIcon              icon;

        // Checkbox column - show checked for both manual and auto-selected
        bool isChecked = info->m_checked || info->m_autoSelected;
        data.push_back( wxVariant( isChecked ) );

        if( info->m_isPower )
        {
            wxBitmap bmp = KiBitmap( BITMAPS::add_power );
            icon.CopyFromBitmap( bmp );
        }
        else if( info->m_existsInDest )
        {
            wxBitmap bmp = KiBitmap( BITMAPS::small_warning );
            icon.CopyFromBitmap( bmp );
        }
        else if( !info->m_parentName.IsEmpty() )
        {
            wxBitmap bmp = KiBitmap( BITMAPS::tree_nosel );
            icon.CopyFromBitmap( bmp );
        }

        wxDataViewIconText iconText( name, icon );
        data.push_back( wxVariant( iconText ) );

        m_symbolList->AppendItem( data );
        index++;
    }

    updateImportButton();
}


void DIALOG_IMPORT_SYMBOL_SELECT::updatePreview()
{
    if( m_selectedSymbol.IsEmpty() )
    {
        m_preview->DisplayPart( nullptr, 0 );
        m_unitChoice->Clear();
        m_unitChoice->Enable( false );
        return;
    }

    // Load symbol from plugin for preview (returns cached pointer)
    LIB_SYMBOL* sym = nullptr;

    try
    {
        sym = m_plugin->LoadSymbol( m_filePath, m_selectedSymbol );
    }
    catch( const IO_ERROR& )
    {
        m_preview->DisplayPart( nullptr, 0 );
        return;
    }

    if( !sym )
    {
        m_preview->DisplayPart( nullptr, 0 );
        return;
    }

    int unitCount = std::max( sym->GetUnitCount(), 1 );

    // Update unit choice if count changed
    m_unitChoice->Enable( unitCount > 1 );
    m_unitChoice->Clear();

    if( unitCount > 1 )
    {
        for( int ii = 0; ii < unitCount; ii++ )
            m_unitChoice->Append( sym->GetUnitDisplayName( ii + 1, true ) );

        m_unitChoice->SetSelection( 0 );
    }

    int selectedUnit = ( m_unitChoice->GetSelection() != wxNOT_FOUND )
                               ? m_unitChoice->GetSelection() + 1
                               : 1;

    // For derived symbols, we need to flatten to show properly
    std::unique_ptr<LIB_SYMBOL> flattenedSym;

    if( sym->IsDerived() || !sym->GetParentName().IsEmpty() )
    {
        try
        {
            flattenedSym = sym->Flatten();
            m_preview->DisplayPart( flattenedSym.get(), selectedUnit );
        }
        catch( const IO_ERROR& )
        {
            // show unflattened symbol as fallback
            m_preview->DisplayPart( sym, selectedUnit );
        }
    }
    else
    {
        m_preview->DisplayPart( sym, selectedUnit );
    }
}


void DIALOG_IMPORT_SYMBOL_SELECT::updateStatusLine()
{
    int manualCount = m_manager.GetManualSelectionCount();
    int autoCount = m_manager.GetAutoSelectionCount();

    wxString status;

    if( autoCount > 0 )
    {
        status = wxString::Format( _( "%d symbols selected, %d parents auto-included" ),
                                   manualCount, autoCount );
    }
    else
    {
        status = wxString::Format( _( "%d symbols selected" ), manualCount );
    }

    m_statusLine->SetLabel( status );
}


void DIALOG_IMPORT_SYMBOL_SELECT::updateImportButton()
{
    bool hasSelection = !m_manager.GetSymbolsToImport().empty();
    m_sdbSizerOK->Enable( hasSelection );
}


void DIALOG_IMPORT_SYMBOL_SELECT::OnFilterTextChanged( wxCommandEvent& event )
{
    m_filterString = m_searchCtrl->GetValue().Lower();
    refreshList();
}


void DIALOG_IMPORT_SYMBOL_SELECT::OnSymbolSelected( wxDataViewEvent& event )
{
    int row = m_symbolList->GetSelectedRow();

    if( row == wxNOT_FOUND )
    {
        m_selectedSymbol.clear();
        updatePreview();
        return;
    }

    for( const auto& [name, listIndex] : m_listIndices )
    {
        if( listIndex == row )
        {
            m_selectedSymbol = name;
            updatePreview();
            return;
        }
    }
}


void DIALOG_IMPORT_SYMBOL_SELECT::onItemChecked( wxDataViewEvent& event )
{
    if( event.GetColumn() != COL_CHECKBOX )
        return;

    int row = m_symbolList->ItemToRow( event.GetItem() );

    if( row == wxNOT_FOUND )
        return;

    for( const auto& [name, listIndex] : m_listIndices )
    {
        if( listIndex == row )
        {
            wxVariant value;
            m_symbolList->GetValue( value, row, COL_CHECKBOX );
            bool newState = value.GetBool();

            toggleSymbolSelection( name, newState );
            return;
        }
    }
}


bool DIALOG_IMPORT_SYMBOL_SELECT::toggleSymbolSelection( const wxString& aSymbolName, bool aChecked )
{
    if( aChecked )
    {
        std::vector<wxString> changed = m_manager.SetSymbolSelected( aSymbolName, true );

        for( const wxString& changedName : changed )
        {
            auto it = m_listIndices.find( changedName );

            if( it != m_listIndices.end() && it->second >= 0 )
            {
                m_symbolList->GetStore()->SetValueByRow( true, it->second, COL_CHECKBOX );
            }
        }
    }
    else
    {
        m_manager.DeselectWithDescendants( aSymbolName );

        for( const wxString& name : m_manager.GetSymbolNames() )
        {
            auto it = m_listIndices.find( name );

            if( it != m_listIndices.end() && it->second >= 0 )
            {
                const SYMBOL_IMPORT_INFO* info = m_manager.GetSymbolInfo( name );

                if( info )
                {
                    bool shouldBeChecked = info->m_checked || info->m_autoSelected;
                    m_symbolList->GetStore()->SetValueByRow( shouldBeChecked, it->second, COL_CHECKBOX );
                }
            }
        }
    }

    updateStatusLine();
    updateImportButton();
    return true;
}


bool DIALOG_IMPORT_SYMBOL_SELECT::matchesFilter( const wxString& aSymbolName ) const
{
    return SYMBOL_IMPORT_MANAGER::MatchesFilter( aSymbolName, m_filterString );
}


void DIALOG_IMPORT_SYMBOL_SELECT::OnSelectAll( wxCommandEvent& event )
{
    m_manager.SelectAll( [this]( const wxString& name ) { return matchesFilter( name ); } );

    refreshList();
    updateStatusLine();
    updateImportButton();
}


void DIALOG_IMPORT_SYMBOL_SELECT::OnSelectNone( wxCommandEvent& event )
{
    m_manager.DeselectAll( [this]( const wxString& name ) { return matchesFilter( name ); } );

    refreshList();
    updateStatusLine();
    updateImportButton();
}


void DIALOG_IMPORT_SYMBOL_SELECT::OnUnitChanged( wxCommandEvent& event )
{
    if( m_selectedSymbol.IsEmpty() )
        return;

    int selectedUnit = ( m_unitChoice->GetSelection() != wxNOT_FOUND )
                               ? m_unitChoice->GetSelection() + 1
                               : 1;

    // Load symbol from plugin for preview (returns cached pointer)
    LIB_SYMBOL* sym = nullptr;

    try
    {
        sym = m_plugin->LoadSymbol( m_filePath, m_selectedSymbol );
    }
    catch( const IO_ERROR& )
    {
        return;
    }

    if( !sym )
        return;

    // For derived symbols, we need to flatten to show properly
    std::unique_ptr<LIB_SYMBOL> flattenedSym;

    if( sym->IsDerived() || !sym->GetParentName().IsEmpty() )
    {
        try
        {
            flattenedSym = sym->Flatten();
            m_preview->DisplayPart( flattenedSym.get(), selectedUnit );
        }
        catch( const IO_ERROR& )
        {
            m_preview->DisplayPart( sym, selectedUnit );
        }
    }
    else
    {
        m_preview->DisplayPart( sym, selectedUnit );
    }
}


std::vector<wxString> DIALOG_IMPORT_SYMBOL_SELECT::GetSelectedSymbols() const
{
    return m_manager.GetSymbolsToImport();
}


bool DIALOG_IMPORT_SYMBOL_SELECT::resolveConflicts()
{
    m_conflictResolutions.clear();

    std::vector<wxString> conflicts = m_manager.GetConflicts();

    if( conflicts.empty() )
        return true;

    wxDialog dlg( this, wxID_ANY, _( "Resolve Import Conflicts" ),
                  wxDefaultPosition, wxSize( 500, 400 ),
                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* label = new wxStaticText( &dlg, wxID_ANY,
            _( "The following symbols already exist in the destination library. "
               "Choose how to handle each conflict:" ) );
    label->Wrap( 450 );
    mainSizer->Add( label, 0, wxALL | wxEXPAND, 10 );

    wxDataViewListCtrl* conflictList = new wxDataViewListCtrl( &dlg, wxID_ANY );
    conflictList->AppendTextColumn( _( "Symbol" ), wxDATAVIEW_CELL_INERT, 200 );
    conflictList->AppendTextColumn( _( "Action" ), wxDATAVIEW_CELL_EDITABLE, 100 );

    for( const wxString& name : conflicts )
    {
        wxVector<wxVariant> data;
        data.push_back( wxVariant( name ) );
        data.push_back( wxVariant( _( "Overwrite" ) ) );
        conflictList->AppendItem( data );

        m_conflictResolutions[name] = CONFLICT_RESOLUTION::OVERWRITE;
    }

    mainSizer->Add( conflictList, 1, wxALL | wxEXPAND, 10 );

    wxBoxSizer* actionSizer = new wxBoxSizer( wxHORIZONTAL );
    wxButton* skipAllBtn = new wxButton( &dlg, wxID_ANY, _( "Skip All" ) );
    wxButton* overwriteAllBtn = new wxButton( &dlg, wxID_ANY, _( "Overwrite All" ) );
    actionSizer->Add( skipAllBtn, 0, wxRIGHT, 5 );
    actionSizer->Add( overwriteAllBtn, 0 );
    mainSizer->Add( actionSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10 );

    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton( new wxButton( &dlg, wxID_OK, _( "Import" ) ) );
    btnSizer->AddButton( new wxButton( &dlg, wxID_CANCEL ) );
    btnSizer->Realize();
    mainSizer->Add( btnSizer, 0, wxALL | wxEXPAND, 10 );

    dlg.SetSizer( mainSizer );

    skipAllBtn->Bind( wxEVT_BUTTON, [&]( wxCommandEvent& ) {
        for( size_t i = 0; i < conflicts.size(); i++ )
        {
            conflictList->SetTextValue( _( "Skip" ), i, 1 );
            m_conflictResolutions[conflicts[i]] = CONFLICT_RESOLUTION::SKIP;
        }
    } );

    overwriteAllBtn->Bind( wxEVT_BUTTON, [&]( wxCommandEvent& ) {
        for( size_t i = 0; i < conflicts.size(); i++ )
        {
            conflictList->SetTextValue( _( "Overwrite" ), i, 1 );
            m_conflictResolutions[conflicts[i]] = CONFLICT_RESOLUTION::OVERWRITE;
        }
    } );

    conflictList->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, [&]( wxDataViewEvent& evt ) {
        int row = conflictList->ItemToRow( evt.GetItem() );

        if( row >= 0 && row < (int) conflicts.size() )
        {
            wxString action = conflictList->GetTextValue( row, 1 );
            m_conflictResolutions[conflicts[row]] =
                    ( action == _( "Skip" ) ) ? CONFLICT_RESOLUTION::SKIP
                                              : CONFLICT_RESOLUTION::OVERWRITE;
        }
    } );

    return dlg.ShowModal() == wxID_OK;
}
