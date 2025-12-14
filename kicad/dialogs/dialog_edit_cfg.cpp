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

#include "dialog_edit_cfg.h"

#include <advanced_config.h>
#include <config_params.h>
#include <paths.h>
#include <wx/button.h>
#include <wx/display.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <widgets/wx_grid.h>
#include <functional>
#include <memory>


static wxString paramValueString( const PARAM_CFG& aParam )
{
    wxString s;

    try
    {
        switch( aParam.m_Type )
        {
        case paramcfg_id::PARAM_INT:
            s << *static_cast<const PARAM_CFG_INT&>( aParam ).m_Pt_param;
            break;

        case paramcfg_id::PARAM_DOUBLE:
            s = wxString::FromCDouble( *static_cast<const PARAM_CFG_DOUBLE&>( aParam ).m_Pt_param );
            break;

        case paramcfg_id::PARAM_WXSTRING:
            s = *static_cast<const PARAM_CFG_WXSTRING&>( aParam ).m_Pt_param;
            break;

        case paramcfg_id::PARAM_BOOL:
            s << ( *static_cast<const PARAM_CFG_BOOL&>( aParam ).m_Pt_param ? wxS( "true" ) : wxS( "false" ) );
            break;

        default:
            wxLogError( wxS( "Unsupported PARAM_CFG variant: " ) + wxString::Format( wxS( "%d" ), aParam.m_Type ) );
        }
    }
    catch( ... )
    {
        wxLogError( wxS( "Error converting parameter value to string." ) );
    }
    return s;
}


static wxString paramDefaultString( const PARAM_CFG& aParam )
{
    wxString s;

    try
    {
        switch( aParam.m_Type )
        {
        case paramcfg_id::PARAM_INT:
            s << static_cast<const PARAM_CFG_INT&>( aParam ).m_Default;
            break;

        case paramcfg_id::PARAM_DOUBLE:
            s = wxString::FromCDouble( static_cast<const PARAM_CFG_DOUBLE&>( aParam ).m_Default );
            break;

        case paramcfg_id::PARAM_WXSTRING:
            s << static_cast<const PARAM_CFG_WXSTRING&>( aParam ).m_default;
            break;

        case paramcfg_id::PARAM_BOOL:
            s << ( static_cast<const PARAM_CFG_BOOL&>( aParam ).m_Default ? wxS( "true" ) : wxS( "false" ) );
            break;

        default:
            break;
        }
    }
    catch( ... )
    {
        wxLogError( wxS( "Error converting parameter default value to string." ) );
    }

    return s;
}


static void writeParam( PARAM_CFG& aParam, const wxString& aValue )
{
    switch( aParam.m_Type )
    {
    case paramcfg_id::PARAM_INT:
    {
        PARAM_CFG_INT& param = static_cast<PARAM_CFG_INT&>( aParam );
        *param.m_Pt_param = wxAtoi( aValue );
        break;
    }

    case paramcfg_id::PARAM_BOOL:
    {
        PARAM_CFG_BOOL& param = static_cast<PARAM_CFG_BOOL&>( aParam );

        if( aValue.CmpNoCase( wxS( "true" ) ) == 0 || aValue.CmpNoCase( wxS( "yes" ) ) == 0
            || aValue.CmpNoCase( wxS( "1" ) ) == 0 )
        {
            *param.m_Pt_param = true;
        }
        else
        {
            *param.m_Pt_param = false;
        }
        break;
    }

    case paramcfg_id::PARAM_DOUBLE:
    {
        PARAM_CFG_DOUBLE& param = static_cast<PARAM_CFG_DOUBLE&>( aParam );
        aValue.ToCDouble( param.m_Pt_param );
        break;
    }

    case paramcfg_id::PARAM_WXSTRING:
    {
        PARAM_CFG_WXSTRING& param = static_cast<PARAM_CFG_WXSTRING&>( aParam );
        *param.m_Pt_param = aValue;
        break;
    }

    default:
        wxASSERT_MSG( false, wxS( "Unsupported PARAM_CFG variant: " )
                                + wxString::Format( wxS( "%d" ), aParam.m_Type ) );
    }
}


DIALOG_EDIT_CFG::DIALOG_EDIT_CFG( wxWindow* aParent ) :
        wxDialog( aParent, wxID_ANY, _( "Edit Advanced Configuration" ), wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    m_cfgFile = wxFileName( PATHS::GetUserSettingsPath(), wxS( "kicad_advanced" ) );

    m_grid = new WX_GRID( this, wxID_ANY );
    m_grid->CreateGrid( 0, 3 );
    m_grid->SetColSize( 0, 100 );   // SetColumnAutosizer() will use these for minimum size
    m_grid->SetColSize( 1, 80 );
    m_grid->SetColLabelValue( 0, _( "Key" ) );
    m_grid->SetColLabelValue( 1, _( "Value" ) );
    m_grid->SetColLabelValue( 2, _( "Extant" ) );
    m_grid->HideCol( 2 );
    m_grid->SetRowLabelSize( 0 );
    m_grid->SetupColumnAutosizer( 1 );

    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK, _( "OK" ) );
    okButton->SetDefault();
    buttonSizer->AddButton( okButton );
    buttonSizer->Realize();

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( m_grid, 1, wxEXPAND | wxALL, 5 );
    sizer->Add( buttonSizer, 0, wxEXPAND | wxALL, 5 );
    SetSizer( sizer );

    wxDisplay display( wxDisplay::GetFromWindow( this ) );
    wxRect    displayRect = display.GetClientArea();

    SetMinSize( wxSize( 500, 300 ) );
    SetMaxSize( wxSize( displayRect.GetWidth() - 100, displayRect.GetHeight() - 100 ) );
    SetSizeHints( wxSize( 700, 500 ) );

    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_EDIT_CFG::OnCellChange, this );
    m_grid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &DIALOG_EDIT_CFG::OnCellRightClick, this );
    Bind( wxEVT_SIZE, &DIALOG_EDIT_CFG::OnSize, this );

    m_contextRow = -1;

    Layout();
    Centre();
}


bool DIALOG_EDIT_CFG::TransferDataToWindow()
{
    for( const std::unique_ptr<PARAM_CFG>& entry : ADVANCED_CFG::GetCfg().GetEntries() )
    {
        wxString value = paramValueString( *entry );
        wxString def = paramDefaultString( *entry );
        int      row = m_grid->GetNumberRows();
        m_grid->AppendRows( 1 );
        m_grid->SetCellValue( row, 0, entry->m_Ident );
        m_grid->SetCellValue( row, 1, value );
        m_grid->SetCellValue( row, 2, def == value ? wxS( "0" ) : wxS( "1" ) );
        m_grid->SetReadOnly( row, 2 );
        updateRowAppearance( row );
    }

    return true;
}


bool DIALOG_EDIT_CFG::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    saveSettings();
    return true;
}


void DIALOG_EDIT_CFG::saveSettings()
{
    int rows = m_grid->GetNumberRows();

    for( int row = 0; row < rows; ++row )
    {
        wxString key = m_grid->GetCellValue( row, 0 );
        wxString val = m_grid->GetCellValue( row, 1 );
        wxString ext = m_grid->GetCellValue( row, 2 );

        if( key.IsEmpty() || ext != wxS( "1" ) )
            continue;

        for( const std::unique_ptr<PARAM_CFG>& entry : ADVANCED_CFG::GetCfg().GetEntries() )
        {
            if( entry->m_Ident == key )
            {
                writeParam( *entry, val );
                break;
            }
        }
    }

    ADVANCED_CFG& adv = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    adv.Save();
}


void DIALOG_EDIT_CFG::OnCellChange( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( col == 0 || col == 1 )
    {
        m_grid->SetCellValue( row, 2, wxS( "1" ) );
        updateRowAppearance( row );
    }

    saveSettings();

    int lastRow = m_grid->GetNumberRows() - 1;

    if( !m_grid->GetCellValue( lastRow, 0 ).IsEmpty() || !m_grid->GetCellValue( lastRow, 1 ).IsEmpty() )
    {
        m_grid->AppendRows( 1 );
        m_grid->SetCellValue( m_grid->GetNumberRows() - 1, 2, wxS( "0" ) );
        m_grid->SetReadOnly( m_grid->GetNumberRows() - 1, 2 );
        updateRowAppearance( m_grid->GetNumberRows() - 1 );
    }

    aEvent.Skip();
}


void DIALOG_EDIT_CFG::OnCellRightClick( wxGridEvent& aEvent )
{
    m_contextRow = aEvent.GetRow();
    wxMenu menu;
    menu.Append( wxID_ANY, _( "Reset to default" ) );
    menu.Bind( wxEVT_MENU, &DIALOG_EDIT_CFG::OnResetDefault, this );
    PopupMenu( &menu );
    menu.Unbind( wxEVT_MENU, &DIALOG_EDIT_CFG::OnResetDefault, this );
}


void DIALOG_EDIT_CFG::OnResetDefault( wxCommandEvent& aEvent )
{
    if( m_contextRow < 0 )
        return;

    wxString key = m_grid->GetCellValue( m_contextRow, 0 );
    wxString def;

    for( const std::unique_ptr<PARAM_CFG>& entry : ADVANCED_CFG::GetCfg().GetEntries() )
    {
        if( entry->m_Ident == key )
        {
            def = paramDefaultString( *entry );
            break;
        }
    }

    m_grid->SetCellValue( m_contextRow, 1, def );
    m_grid->SetCellValue( m_contextRow, 2, wxS( "0" ) );
    updateRowAppearance( m_contextRow );
    saveSettings();
}


void DIALOG_EDIT_CFG::updateRowAppearance( int aRow )
{
    bool   ext = m_grid->GetCellValue( aRow, 2 ) == wxS( "1" );
    wxFont font = m_grid->GetCellFont( aRow, 0 );
    font.SetWeight( ext ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL );
    m_grid->SetCellFont( aRow, 0, font );
    m_grid->SetCellFont( aRow, 1, font );
}