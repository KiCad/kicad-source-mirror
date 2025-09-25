/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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

#include "panel_template_fieldnames.h"

#include <wx/msgdlg.h>

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <widgets/std_bitmap_button.h>
#include <template_fieldnames.h>
#include <grid_tricks.h>
#include <bitmaps.h>
#include <richio.h>
#include <string_utils.h>

PANEL_TEMPLATE_FIELDNAMES::PANEL_TEMPLATE_FIELDNAMES( wxWindow* aWindow,
                                                      TEMPLATES* aProjectTemplateMgr ) :
        PANEL_TEMPLATE_FIELDNAMES_BASE( aWindow )
{
    if( aProjectTemplateMgr )
    {
        m_title->SetLabel( _( "Project Field Name Templates" ) );
        m_global = false;
        m_templateMgr = aProjectTemplateMgr;
    }
    else
    {
        m_title->SetLabel( _( "Global Field Name Templates" ) );
        m_global = true;
        m_templateMgr = &m_templateMgrInstance;

        if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            if( !cfg->m_Drawing.field_names.IsEmpty() )
                m_templateMgr->AddTemplateFieldNames( cfg->m_Drawing.field_names );
        }
    }

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    m_grid->SetUseNativeColLabels();

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid, [this]( wxCommandEvent& aEvent )
                                                       {
                                                           OnAddButtonClick( aEvent );
                                                       } ) );
    m_grid->SetupColumnAutosizer( 0 );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
}


PANEL_TEMPLATE_FIELDNAMES::~PANEL_TEMPLATE_FIELDNAMES()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool PANEL_TEMPLATE_FIELDNAMES::TransferDataToWindow()
{
    m_fields = m_templateMgr->GetTemplateFieldNames( m_global );

    return TransferDataToGrid();
}


void PANEL_TEMPLATE_FIELDNAMES::OnAddButtonClick( wxCommandEvent& event )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                int row = m_grid->GetNumberRows();
                TransferDataFromGrid();

                TEMPLATE_FIELDNAME newFieldname = TEMPLATE_FIELDNAME( _( "Untitled Field" ) );
                newFieldname.m_Visible = false;
                m_fields.insert( m_fields.end(), newFieldname );

                TransferDataToGrid();
                return { row, 0 };
            } );
}


void PANEL_TEMPLATE_FIELDNAMES::OnDeleteButtonClick( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                m_fields.erase( m_fields.begin() + row );
                m_grid->DeleteRows( row );
            } );
}


void PANEL_TEMPLATE_FIELDNAMES::OnMoveUp( wxCommandEvent& event )
{
    m_grid->OnMoveRowUp(
            [&]( int row )
            {
                m_grid->SwapRows( row, row - 1 );
            } );
}


void PANEL_TEMPLATE_FIELDNAMES::OnMoveDown( wxCommandEvent& event )
{
    m_grid->OnMoveRowDown(
            [&]( int row )
            {
                m_grid->SwapRows( row, row + 1 );
            } );
}


bool PANEL_TEMPLATE_FIELDNAMES::TransferDataToGrid()
{
    m_grid->Freeze();

    m_grid->ClearRows();
    m_grid->AppendRows( m_fields.size() );

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        m_grid->SetCellValue( row, 0, m_fields[row].m_Name );

        // columns 1 and 2 show a boolean value (in a check box):
        m_grid->SetCellValue( row, 1, m_fields[row].m_Visible ? wxS( "1" ) : wxS( "0" ) );
        m_grid->SetCellValue( row, 2, m_fields[row].m_URL     ? wxS( "1" ) : wxS( "0" ) );

        // Set cell properties
        m_grid->SetCellAlignment( row, 0, wxALIGN_LEFT, wxALIGN_CENTRE );

        // Render the Visible and URL columns as check boxes
        m_grid->SetCellRenderer( row, 1, new wxGridCellBoolRenderer() );
        m_grid->SetReadOnly( row, 1 );  // Not really; we delegate interactivity to GRID_TRICKS
        m_grid->SetCellAlignment( row, 1, wxALIGN_CENTRE, wxALIGN_CENTRE );

        m_grid->SetCellRenderer( row, 2, new wxGridCellBoolRenderer() );
        m_grid->SetReadOnly( row, 2 );  // Not really; we delegate interactivity to GRID_TRICKS
        m_grid->SetCellAlignment( row, 2, wxALIGN_CENTRE, wxALIGN_CENTRE );
    }

    m_grid->Thaw();

    return true;
}


bool PANEL_TEMPLATE_FIELDNAMES::TransferDataFromGrid()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        m_fields[row].m_Name    = m_grid->GetCellValue( row, 0 );
        m_fields[row].m_Visible = m_grid->GetCellValue( row, 1 ) == wxS( "1" );
        m_fields[row].m_URL     = m_grid->GetCellValue( row, 2 ) == wxS( "1" );
    }

    return true;
}


bool PANEL_TEMPLATE_FIELDNAMES::TransferDataFromWindow()
{
    if( !TransferDataFromGrid() )
        return false;

    m_templateMgr->DeleteAllFieldNameTemplates( m_global );

    for( TEMPLATE_FIELDNAME& field : m_fields )
    {
        if( !field.m_Name.IsEmpty() )
        {
            wxString trimmedName = field.m_Name;

            trimmedName.Trim();
            trimmedName.Trim( false );

            // Check if the field name contains leading and/or trailing white space.
            if( field.m_Name != trimmedName )
            {
                wxString msg;

                msg.Printf( _( "The field name '%s' contains trailing and/or leading white space." ),
                            field.m_Name );

                wxMessageDialog dlg( this, msg, _( "Warning" ), wxOK|wxCANCEL|wxCENTER|wxICON_WARNING );

                dlg.SetExtendedMessage( _( "This may result in what appears to be duplicate field "
                                           "names but are actually unique names differing only by "
                                           "white space characters.  Removing the white space "
                                           "characters will have no effect on existing symbol "
                                           "field names." ) );

                dlg.SetOKCancelLabels( wxMessageDialog::ButtonLabel( _( "Remove White Space" ) ),
                                       wxMessageDialog::ButtonLabel( _( "Keep White Space" ) ) );

                if( dlg.ShowModal() == wxID_OK )
                    field.m_Name = trimmedName;
            }

            m_templateMgr->AddTemplateFieldName( field, m_global );
        }
    }

    if( m_global )
    {
        if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            // Save global fieldname templates
            STRING_FORMATTER sf;
            m_templateMgr->Format( &sf, true );

            wxString record = From_UTF8( sf.GetString().c_str() );
            record.Replace( wxT( "  " ), wxT( " " ), true );  // double space to single

            cfg->m_Drawing.field_names = record.ToStdString();
        }
    }

    return true;
}


void PANEL_TEMPLATE_FIELDNAMES::ImportSettingsFrom( TEMPLATES* templateMgr )
{
    m_fields = templateMgr->GetTemplateFieldNames( m_global );
    TransferDataToGrid();
}


