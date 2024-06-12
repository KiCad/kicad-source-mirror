/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <symbol_library.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <core/kicad_algo.h>
#include <symbol_library_common.h>
#include <confirm.h>
#include <eeschema_id.h>
#include <general.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_symbol.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <project_sch.h>

#include <dialog_symbol_chooser.h>

PICKED_SYMBOL SCH_BASE_FRAME::PickSymbolFromLibrary( const SYMBOL_LIBRARY_FILTER* aFilter,
                                                     std::vector<PICKED_SYMBOL>&  aHistoryList,
                                                     std::vector<PICKED_SYMBOL>&  aAlreadyPlaced,
                                                     bool aShowFootprints, const LIB_ID* aHighlight,
                                                     bool aAllowFields )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_SYMBOL_CHOOSER::g_Mutex, std::defer_lock );

    // One DIALOG_SYMBOL_CHOOSER dialog at a time.  User probably can't handle more anyway.
    if( !dialogLock.try_lock() )
        return PICKED_SYMBOL();

    DIALOG_SYMBOL_CHOOSER dlg( this, aHighlight, aFilter, aHistoryList, aAlreadyPlaced,
                               aAllowFields, aShowFootprints );

    if( dlg.ShowModal() == wxID_CANCEL )
        return PICKED_SYMBOL();

    PICKED_SYMBOL sel;
    LIB_ID id = dlg.GetSelectedLibId( &sel.Unit );

    if( !id.IsValid() )
        return PICKED_SYMBOL();

    if( sel.Unit == 0 )
        sel.Unit = 1;

    sel.Fields = dlg.GetFields();
    sel.LibId = id;

    if( sel.LibId.IsValid() )
    {
        alg::delete_if( aHistoryList, [&sel]( PICKED_SYMBOL const& i )
                                      {
                                          return i.LibId == sel.LibId;
                                      } );

        aHistoryList.insert( aHistoryList.begin(), sel );
    }

    return sel;
}


void SCH_EDIT_FRAME::SelectUnit( SCH_SYMBOL* aSymbol, int aUnit )
{
    SCH_COMMIT  commit( m_toolManager );
    LIB_SYMBOL* symbol = GetLibSymbol( aSymbol->GetLibId() );

    if( !symbol )
        return;

    int unitCount = symbol->GetUnitCount();

    if( unitCount <= 1 || aSymbol->GetUnit() == aUnit )
        return;

    if( aUnit > unitCount )
        aUnit = unitCount;

    if( !aSymbol->GetEditFlags() )    // No command in progress: save in undo list
        commit.Modify( aSymbol, GetScreen() );

    /* Update the unit number. */
    aSymbol->SetUnitSelection( &GetCurrentSheet(), aUnit );
    aSymbol->SetUnit( aUnit );

    if( !commit.Empty() )
    {
        if( eeconfig()->m_AutoplaceFields.enable )
            aSymbol->AutoAutoplaceFields( GetScreen() );

        commit.Push( _( "Change Unit" ) );
    }
}


void SCH_EDIT_FRAME::FlipBodyStyle( SCH_SYMBOL* aSymbol )
{
    if( !aSymbol || !aSymbol->GetLibSymbolRef() )
        return;

    SCH_COMMIT commit( m_toolManager );
    wxString   msg;

    if( !aSymbol->GetLibSymbolRef()->HasAlternateBodyStyle() )
    {
        LIB_ID id = aSymbol->GetLibSymbolRef()->GetLibId();

        msg.Printf( _( "No alternate body style found for symbol '%s' in library '%s'." ),
                    id.GetLibItemName().wx_str(),
                    id.GetLibNickname().wx_str() );
        DisplayError( this,  msg );
        return;
    }

    commit.Modify( aSymbol, GetScreen() );

    aSymbol->SetBodyStyle( aSymbol->GetBodyStyle() + 1 );

    // ensure m_bodyStyle = 1 or 2
    // 1 = shape 1 = first (base DeMorgan) alternate body style
    // 2 = shape 2 = second (DeMorgan conversion) alternate body style
    // > 2 is not currently supported
    // When m_bodyStyle = val max, return to the first shape
    if( aSymbol->GetBodyStyle() > BODY_STYLE::DEMORGAN )
        aSymbol->SetBodyStyle( BODY_STYLE::BASE );

    // If selected make sure all the now-included pins are selected
    if( aSymbol->IsSelected() )
        m_toolManager->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, aSymbol );

    commit.Push( _( "Change Body Style" ) );
}


void SCH_EDIT_FRAME::SetAltPinFunction( SCH_PIN* aPin, const wxString& aFunction )
{
    if( !aPin )
        return;

    SCH_COMMIT commit( m_toolManager );
    commit.Modify( aPin, GetScreen() );

    if( aFunction == aPin->GetName() )
        aPin->SetAlt( wxEmptyString );
    else
        aPin->SetAlt( aFunction );

    commit.Push( _( "Set Pin Function" ) );
}
