/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialogs/dialog_configure_paths.h>

#include <algorithm>

#include <bitmaps.h>
#include <confirm.h>
#include <kidialog.h>
#include <validators.h>
#include <dialogs/html_message_box.h>
#include <settings/common_settings.h>
#include <filename_resolver.h>
#include <env_vars.h>
#include <grid_tricks.h>
#include <pgm_base.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <wx/dirdlg.h>


enum TEXT_VAR_GRID_COLUMNS
{
    TV_NAME_COL = 0,
    TV_VALUE_COL,
    TV_FLAG_COL
};

enum SEARCH_PATH_GRID_COLUMNS
{
    SP_ALIAS_COL = 0,
    SP_PATH_COL,
    SP_DESC_COL
};


DIALOG_CONFIGURE_PATHS::DIALOG_CONFIGURE_PATHS( wxWindow* aParent ) :
    DIALOG_CONFIGURE_PATHS_BASE( aParent ),
    m_errorGrid( nullptr ),
    m_errorRow( -1 ),
    m_errorCol( -1 ),
    m_helpBox( nullptr )
{
    m_heightBeforeHelp = FromDIP ( 400 );
    m_btnAddEnvVar->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_btnDeleteEnvVar->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_EnvVars->SetMinSize( FromDIP( m_EnvVars->GetMinSize() ) );

    m_EnvVars->ClearRows();
    m_EnvVars->AppendCols( 1 );     // for the isExternal flags
    m_EnvVars->HideCol( TV_FLAG_COL );
    m_EnvVars->UseNativeColHeader( true );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_PATH_EDITOR( this, m_EnvVars, &m_curdir, wxEmptyString ) );
    m_EnvVars->SetColAttr( TV_VALUE_COL, attr );

    m_EnvVars->PushEventHandler( new GRID_TRICKS( m_EnvVars,
                                                  [this]( wxCommandEvent& aEvent )
                                                  {
                                                      OnAddEnvVar( aEvent );
                                                  } ) );

    m_EnvVars->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );

    SetInitialFocus( m_EnvVars );
    SetupStandardButtons();

    // wxFormBuilder doesn't include this event...
    m_EnvVars->Connect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_CONFIGURE_PATHS::OnGridCellChanging ),
                        nullptr, this );

    m_EnvVars->SetupColumnAutosizer( TV_VALUE_COL );

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_CONFIGURE_PATHS::~DIALOG_CONFIGURE_PATHS()
{
    // Delete the GRID_TRICKS.
    m_EnvVars->PopEventHandler( true );

    m_EnvVars->Disconnect( wxEVT_GRID_CELL_CHANGING,
                           wxGridEventHandler( DIALOG_CONFIGURE_PATHS::OnGridCellChanging ),
                           nullptr, this );
}


bool DIALOG_CONFIGURE_PATHS::TransferDataToWindow()
{
   if( !wxDialog::TransferDataToWindow() )
       return false;

    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

    for( auto it = envVars.begin(); it != envVars.end(); ++it )
    {
        const wxString& path = it->second.GetValue();
        AppendEnvVar( it->first, path, it->second.GetDefinedExternally() );

        if( m_curdir.IsEmpty() && !path.StartsWith( "${" ) && !path.StartsWith( "$(" ) )
            m_curdir = path;
    }

    return true;
}


void DIALOG_CONFIGURE_PATHS::AppendEnvVar( const wxString& aName, const wxString& aPath,
                                           bool isExternal )
{
    int i = m_EnvVars->GetNumberRows();

    m_EnvVars->AppendRows( 1 );

    m_EnvVars->SetCellValue( i, TV_NAME_COL, aName );

    wxGridCellAttr* nameCellAttr = m_EnvVars->GetOrCreateCellAttr( i, TV_NAME_COL );
    wxGridCellTextEditor* nameTextEditor = new GRID_CELL_TEXT_EDITOR();
    nameTextEditor->SetValidator( ENV_VAR_NAME_VALIDATOR() );
    nameCellAttr->SetEditor( nameTextEditor );
    nameCellAttr->SetReadOnly( ENV_VAR::IsEnvVarImmutable( aName ) );
    nameCellAttr->DecRef();

    m_EnvVars->SetCellValue( i, TV_VALUE_COL, aPath );

    wxGridCellAttr* pathCellAttr = m_EnvVars->GetOrCreateCellAttr( i, TV_VALUE_COL );
    wxSystemColour c = isExternal ? wxSYS_COLOUR_MENU : wxSYS_COLOUR_LISTBOX;
    pathCellAttr->SetBackgroundColour( wxSystemSettings::GetColour( c ) );
    pathCellAttr->DecRef();

    m_EnvVars->SetCellValue( i, TV_FLAG_COL, isExternal ? wxT( "external" ) : wxEmptyString );
}


bool DIALOG_CONFIGURE_PATHS::TransferDataFromWindow()
{
    if( !m_EnvVars->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Update environment variables
    ENV_VAR_MAP& envVarMap = Pgm().GetLocalEnvVariables();

    for( int row = 0; row < m_EnvVars->GetNumberRows(); ++row )
    {
        wxString name     = m_EnvVars->GetCellValue( row, TV_NAME_COL );
        wxString path     = m_EnvVars->GetCellValue( row, TV_VALUE_COL );
        bool     external = !m_EnvVars->GetCellValue( row, TV_FLAG_COL ).IsEmpty();

        if( external )
        {
            // Don't check for consistency on external variables, just use them as-is
        }
        else if( name.empty() && path.empty() )
        {
            // Skip empty rows altogether
            continue;
        }
        else if( name.IsEmpty() )
        {
            m_errorGrid = m_EnvVars;
            m_errorRow = row;
            m_errorCol = TV_NAME_COL;
            m_errorMsg = _( "Environment variable name cannot be empty." );
            return false;
        }
        else if( path.IsEmpty() )
        {
            m_errorGrid = m_EnvVars;
            m_errorRow = row;
            m_errorCol = TV_VALUE_COL;
            m_errorMsg = _( "Environment variable path cannot be empty." );
            return false;
        }

        if( envVarMap.count( name ) )
            envVarMap.at( name ).SetValue( path );
        else
            envVarMap[ name ] = ENV_VAR_ITEM( name, path );
    }

    // Remove deleted env vars
    for( auto it = envVarMap.begin(); it != envVarMap.end();  )
    {
        bool found = false;

        for( int row = 0; row < m_EnvVars->GetNumberRows(); ++row )
        {
            wxString name = m_EnvVars->GetCellValue( row, TV_NAME_COL );

            if( it->first == name )
            {
                found = true;
                break;
            }
        }

        if( found )
            ++it;
        else
            it = envVarMap.erase( it );
    }

    Pgm().SetLocalEnvVariables();

    return true;
}


void DIALOG_CONFIGURE_PATHS::OnGridCellChanging( wxGridEvent& event )
{
    wxGrid*  grid = dynamic_cast<wxGrid*>( event.GetEventObject() );
    int      row = event.GetRow();
    int      col = event.GetCol();
    wxString text = event.GetString();

    text.Trim( true ).Trim( false ); // Trim from both sides
    grid->SetCellValue( row, col, text ); // Update the grid with trimmed value

    if( text.IsEmpty() )
    {
        if( grid == m_EnvVars )
        {
            if( col == TV_NAME_COL )
                m_errorMsg = _( "Environment variable name cannot be empty." );
            else
                m_errorMsg = _( "Environment variable path cannot be empty." );
        }
        else
        {
            if( col == SP_ALIAS_COL )
                m_errorMsg = _( "3D search path alias cannot be empty." );
            else
                m_errorMsg = _( "3D search path cannot be empty." );
        }

        m_errorGrid = dynamic_cast<wxGrid*>( event.GetEventObject() );
        m_errorRow = row;
        m_errorCol = col;

        event.Veto();
    }

    if( grid == m_EnvVars )
    {
        if( col == TV_VALUE_COL && m_EnvVars->GetCellValue( row, TV_FLAG_COL ).Length()
                && !Pgm().GetCommonSettings()->m_DoNotShowAgain.env_var_overwrite_warning )
        {
            wxString msg1 = _( "This path was defined  externally to the running process and\n"
                               "will only be temporarily overwritten." );
            wxString msg2 = _( "The next time KiCad is launched, any paths that have already\n"
                               "been defined are honored and any settings defined in the path\n"
                               "configuration dialog are ignored.  If you did not intend for\n"
                               "this behavior, either rename any conflicting entries or remove\n"
                               "the external environment variable(s) from your system." );
            KIDIALOG dlg( this, msg1, KIDIALOG::KD_WARNING );
            dlg.ShowDetailedText( msg2 );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );
            dlg.ShowModal();

            if( dlg.DoNotShowAgain() )
                Pgm().GetCommonSettings()->m_DoNotShowAgain.env_var_overwrite_warning = true;
        }
        else if( col == TV_NAME_COL && m_EnvVars->GetCellValue( row, TV_NAME_COL ) != text )
        {
            // This env var name is reserved and cannot be added here.
            if( text == PROJECT_VAR_NAME )
            {
                wxMessageBox( wxString::Format( _( "The name %s is reserved, and cannot be used." ),
                              PROJECT_VAR_NAME ) );
                event.Veto();
            }
            else    // Changing name; clear external flag
            {
                m_EnvVars->SetCellValue( row, TV_FLAG_COL, wxEmptyString );
            }
        }
    }
}


void DIALOG_CONFIGURE_PATHS::OnAddEnvVar( wxCommandEvent& event )
{
    m_EnvVars->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                AppendEnvVar( wxEmptyString, wxEmptyString, false );
                return { m_EnvVars->GetNumberRows() - 1, TV_NAME_COL };
            } );
}


void DIALOG_CONFIGURE_PATHS::OnRemoveEnvVar( wxCommandEvent& event )
{
    m_EnvVars->OnDeleteRows(
            [&]( int row )
            {
                if( ENV_VAR::IsEnvVarImmutable( m_EnvVars->GetCellValue( row, TV_NAME_COL ) ) )
                {
                    wxBell();
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                m_EnvVars->DeleteRows( row, 1 );
            } );
}


void DIALOG_CONFIGURE_PATHS::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Handle a grid error.  This is delayed to OnUpdateUI so that we can change focus even when
    // the original validation was triggered from a killFocus event.
    if( m_errorGrid )
    {
        // We will re-enter this routine when the error dialog is displayed, so make sure we don't
        // keep putting up more dialogs.
        wxGrid* grid = m_errorGrid;
        m_errorGrid = nullptr;

        DisplayErrorMessage( this, m_errorMsg );

        grid->SetFocus();
        grid->MakeCellVisible( m_errorRow, m_errorCol );
        grid->SetGridCursor( m_errorRow, m_errorCol );

        grid->EnableCellEditControl( true );
        grid->ShowCellEditControl();
    }
}


void DIALOG_CONFIGURE_PATHS::OnHelp( wxCommandEvent& event )
{
    wxSizer* sizerMain = GetSizer();

    if( !m_helpBox )
    {
        m_helpBox = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxHW_SCROLLBAR_AUTO );

        wxString msg = _( "Enter the name and value for each environment variable.  Grey entries "
                          "are names that have been defined externally at the system or user "
                          "level.  Environment variables defined at the system or user level "
                          "take precedence over the ones defined in this table.  This means the "
                          "values in this table are ignored." );
        msg << "<br><br><b>";
        msg << _( "To ensure environment variable names are valid on all platforms, the name field "
                  "will only accept upper case letters, digits, and the underscore characters." );
        msg << "</b>";

        for( const wxString& var : ENV_VAR::GetPredefinedEnvVars() )
        {
            msg << "<br><br><b>" << var << "</b>";

            const auto desc = ENV_VAR::LookUpEnvVarHelp( var );

            if( desc.size() > 0 )
                msg << ": " << desc;

        }

        m_helpBox->SetPage( msg );
        m_helpBox->Show( false );

        sizerMain->Insert( sizerMain->GetItemCount() - 1, m_helpBox, 1, wxALL|wxEXPAND, 10 );
    }

    if( m_helpBox->IsShown() )
    {
        m_helpBox->Show( false );
        SetClientSize( wxSize( GetClientSize().x, m_heightBeforeHelp ) );
    }
    else
    {
        m_helpBox->Show( true );
        m_heightBeforeHelp = GetClientSize().y;

        int minHelpBoxHeight = GetTextExtent( wxT( "T" ) ).y * 20;

        if( GetClientSize().y < minHelpBoxHeight * 2 )
            SetClientSize( wxSize( GetClientSize().x, GetClientSize().y + minHelpBoxHeight ) );
    }

    Layout();
}
