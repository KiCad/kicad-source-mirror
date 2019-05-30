/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <lib_pin.h>
#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <wildcards_and_files_ext.h>
#include <sch_legacy_plugin.h>
#include <properties.h>
#include <view/view.h>
#include <dialogs/dialog_display_info_HTML_base.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>

void LIB_EDIT_FRAME::LoadOneSymbol()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    LIB_PART*          part = GetCurPart();

    // Exit if no library entry is selected or a command is in progress.
    if( !part || !EE_CONDITIONS::Idle( selTool->GetSelection() ) )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Import Symbol" ), default_path,
                      wxEmptyString, SchematicSymbolFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = dlg.GetPath();

    prj.SetRString( PROJECT::SCH_LIB_PATH, filename );

    wxArrayString symbols;
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    wxString msg;

    try
    {
        pi->EnumerateSymbolLib( symbols, filename );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Cannot import symbol library \"%s\"." ), filename );
        DisplayErrorMessage( this, msg, ioe.What() );
        return;
    }

    if( symbols.empty() )
    {
        msg.Printf( _( "Symbol library file \"%s\" is empty." ), filename );
        DisplayError( this,  msg );
        return;
    }

    if( symbols.GetCount() > 1 )
    {
        msg.Printf( _( "More than one symbol found in symbol file \"%s\"." ), filename );
        wxMessageBox( msg, _( "Warning" ), wxOK | wxICON_EXCLAMATION, this );
    }

    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = pi->LoadSymbol( filename, symbols[0] );
    }
    catch( const IO_ERROR& )
    {
        return;
    }

    wxCHECK_RET( alias && alias->GetPart(), "Invalid symbol." );

    SaveCopyInUndoList( part );

    LIB_PART* first = alias->GetPart();
    LIB_ITEMS_CONTAINER& drawList = first->GetDrawItems();

    for( LIB_ITEM& item : drawList )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( item.GetUnit() )
            item.SetUnit( m_unit );

        if( item.GetConvert() )
            item.SetConvert( m_convert );

        item.SetFlags( IS_NEW | SELECTED );

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();

        newItem->SetParent( part );
        part->AddDrawItem( newItem );
        item.ClearSelected();
    }

    part->RemoveDuplicateDrawItems();

    OnModify();
}


void LIB_EDIT_FRAME::SaveOneSymbol()
{
    // Export the current part as a symbol (.sym file)
    // this is the current part without its aliases and doc file
    // because a .sym file is used to import graphics in a part being edited
    LIB_PART*       part = GetCurPart();

    if( !part || part->GetDrawItems().empty() )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Export Symbol" ), default_path,
                      part->GetName() + "." + SchematicSymbolFileExtension,
                      SchematicSymbolFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SchematicSymbolFileExtension );

    prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

    if( fn.FileExists() )
        wxRemove( fn.GetFullPath() );

    SetStatusText( wxString::Format( _( "Saving symbol in \"%s\"" ), fn.GetPath() ) );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    try
    {
        PROPERTIES nodoc_props;     // Doc file is useless for a .sym file
        nodoc_props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";
        plugin->CreateSymbolLib( fn.GetFullPath(), &nodoc_props );

        LIB_PART* saved_part = new LIB_PART( *part );
        saved_part->RemoveAllAliases();     // useless in a .sym file
        plugin->SaveSymbol( fn.GetFullPath(), saved_part, &nodoc_props );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "An error occurred saving symbol file \"%s\"" ),
                                         fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
    }
}


// helper function to sort pins by pin num
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    // Use number as primary key
    int test = ref->GetNumber().Cmp( tst->GetNumber() );

    // Use DeMorgan variant as secondary key
    if( test == 0 )
        test = ref->GetConvert() - tst->GetConvert();

    // Use unit as tertiary key
    if( test == 0 )
        test = ref->GetUnit() - tst->GetUnit();

    return test < 0;
}


void LIB_EDIT_FRAME::OnCheckComponent( wxCommandEvent& event )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    wxRealPoint curr_grid_size = GetScreen()->GetGridSize();
    const int min_grid_size = 25;
    const int grid_size = KiROUND( curr_grid_size.x );
    const int clamped_grid_size = ( grid_size < min_grid_size ) ? min_grid_size : grid_size;

    LIB_PINS pinList;

    part->GetPins( pinList );

    if( pinList.size() == 0 )
    {
        DisplayInfoMessage( this, _( "No pins!" ) );
        return;
    }

    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // Test for duplicates:
    DIALOG_DISPLAY_HTML_TEXT_BASE error_display( this, wxID_ANY,
                                                 _( "Marker Information" ),
                                                 wxDefaultPosition,
                                                 wxSize( 750, 600 ) );

    int dup_error = 0;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* curr_pin = pinList[ii];
        LIB_PIN* pin      = pinList[ii - 1];

        if( pin->GetNumber() != curr_pin->GetNumber()
            || pin->GetConvert() != curr_pin->GetConvert() )
            continue;

        dup_error++;

        /* TODO I dare someone to find a way to make happy translators on
           this thing! Lorenzo */

        wxString msg = wxString::Format( _(
                                                 "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>"
                                                 " conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                                         GetChars( curr_pin->GetNumber() ),
                                         GetChars( curr_pin->GetName() ),
                                         curr_pin->GetPosition().x / 1000.0,
                                         -curr_pin->GetPosition().y / 1000.0,
                                         GetChars( pin->GetNumber() ),
                                         GetChars( pin->GetName() ),
                                         pin->GetPosition().x / 1000.0,
                                         -pin->GetPosition().y / 1000.0
        );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in units %c and %c" ),
                                     'A' + curr_pin->GetUnit() - 1,
                                     'A' + pin->GetUnit() - 1 );
        }

        if( m_showDeMorgan )
        {
            if( curr_pin->GetConvert() )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    // Test for off grid pins:
    int offgrid_error = 0;

    for( unsigned ii = 0; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin = pinList[ii];

        if( ( (pin->GetPosition().x % clamped_grid_size) == 0 ) &&
            ( (pin->GetPosition().y % clamped_grid_size) == 0 ) )
            continue;

        // "pin" is off grid here.
        offgrid_error++;

        wxString msg = wxString::Format( _(
                                                 "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                                         GetChars( pin->GetNumber() ),
                                         GetChars( pin->GetName() ),
                                         pin->GetPosition().x / 1000.0,
                                         -pin->GetPosition().y / 1000.0
        );

        if( part->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in symbol %c" ), 'A' + pin->GetUnit() - 1 );
        }

        if( m_showDeMorgan )
        {
            if( pin->GetConvert() )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    if( !dup_error && !offgrid_error )
        DisplayInfoMessage( this, _( "No off grid or duplicate pins were found." ) );
    else
        error_display.ShowModal();
}
