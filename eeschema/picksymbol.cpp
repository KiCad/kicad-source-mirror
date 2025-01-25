/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <symbol_library_common.h>
#include <confirm.h>
#include <sch_tool_utils.h>
#include <eeschema_id.h>
#include <general.h>
#include <kidialog.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <algorithm>
#include <sch_symbol.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
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

    bool aCancelled = false;

    DIALOG_SYMBOL_CHOOSER dlg( this, aHighlight, aFilter, aHistoryList, aAlreadyPlaced,
                               aAllowFields, aShowFootprints, aCancelled );

    if( aCancelled || dlg.ShowModal() == wxID_CANCEL )
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
        std::erase_if( aHistoryList, [&sel]( PICKED_SYMBOL const& i )
                                      {
                                          return i.LibId == sel.LibId;
                                      } );

        aHistoryList.insert( aHistoryList.begin(), sel );
    }

    sel.KeepSymbol = dlg.GetKeepSymbol();
    sel.PlaceAllUnits = dlg.GetPlaceAllUnits();
    return sel;
}


void SCH_EDIT_FRAME::SelectUnit( SCH_SYMBOL* aSymbol, int aUnit )
{
    SCH_COMMIT  commit( m_toolManager );
    LIB_SYMBOL* symbol = GetLibSymbol( aSymbol->GetLibId() );

    if( !symbol )
        return;

    const int unitCount = symbol->GetUnitCount();
    const int currentUnit = aSymbol->GetUnit();

    if( unitCount <= 1 || currentUnit == aUnit )
        return;

    if( aUnit > unitCount )
        aUnit = unitCount;

    const SCH_SHEET_PATH&        sheetPath = GetCurrentSheet();
    bool                         swapWithOther = false;
    std::optional<SCH_REFERENCE> otherSymbolRef = FindSymbolByRefAndUnit( *aSymbol->Schematic(),
                                                                          aSymbol->GetRef( &sheetPath, false ),
                                                                          aUnit );

    if( otherSymbolRef )
    {
        const wxString targetUnitName = symbol->GetUnitDisplayName( aUnit, false );
        const wxString currUnitName = symbol->GetUnitDisplayName( currentUnit, false );
        wxString otherSheetName = otherSymbolRef->GetSheetPath().PathHumanReadable( true, true );

        if( otherSheetName.IsEmpty() )
            otherSheetName = _( "Root" );

        const wxString msg = wxString::Format( _( "Symbol unit '%s' is already placed (on sheet '%s')" ),
                                               targetUnitName, otherSheetName );

        KIDIALOG dlg( this, msg, _( "Unit Already Placed" ), wxYES_NO | wxCANCEL | wxICON_WARNING );
        dlg.SetYesNoLabels( wxString::Format( _( "&Swap '%s' and '%s'" ), targetUnitName, currUnitName ),
                            wxString::Format( _( "&Duplicate '%s'" ), targetUnitName ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
            return;

        if( ret == wxID_YES )
            swapWithOther = true;
    }

    if( swapWithOther )
    {
        // We were reliably informed this would exist.
        wxASSERT( otherSymbolRef );

        SCH_SYMBOL* otherSymbol = otherSymbolRef->GetSymbol();

        if( !otherSymbol->GetEditFlags() )
            commit.Modify( otherSymbol, otherSymbolRef->GetSheetPath().LastScreen() );

        // Give that symbol the unit we used to have
        otherSymbol->SetUnitSelection( &otherSymbolRef->GetSheetPath(), currentUnit );
        otherSymbol->SetUnit( currentUnit );
    }

    if( !aSymbol->GetEditFlags() ) // No command in progress: save in undo list
        commit.Modify( aSymbol, GetScreen() );

    // Update the unit number.
    aSymbol->SetUnit( aUnit );
    aSymbol->SetUnitSelection( &sheetPath, aUnit );

    if( !commit.Empty() )
    {
        if( eeconfig()->m_AutoplaceFields.enable )
        {
            AUTOPLACE_ALGO fieldsAutoplaced = aSymbol->GetFieldsAutoplaced();

            if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
                aSymbol->AutoplaceFields( GetScreen(), fieldsAutoplaced );
        }

        if( swapWithOther )
            commit.Push( _( "Swap Units" ) );
        else
            commit.Push( _( "Change Unit" ) );
    }
}


void SCH_EDIT_FRAME::SelectBodyStyle( SCH_SYMBOL* aSymbol, int aBodyStyle )
{
    if( !aSymbol || !aSymbol->GetLibSymbolRef() )
        return;

    const int bodyStyleCount = aSymbol->GetLibSymbolRef()->GetBodyStyleCount();
    const int currentBodyStyle = aSymbol->GetBodyStyle();

    if( bodyStyleCount <= 1 || currentBodyStyle == aBodyStyle )
        return;

    if( aBodyStyle > bodyStyleCount )
        aBodyStyle = bodyStyleCount;

    SCH_COMMIT commit( m_toolManager );

    commit.Modify( aSymbol, GetScreen() );

    aSymbol->SetBodyStyle( aBodyStyle );

    // If selected make sure all the now-included pins are selected
    if( aSymbol->IsSelected() )
        m_toolManager->RunAction<EDA_ITEM*>( ACTIONS::selectItem, aSymbol );

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
