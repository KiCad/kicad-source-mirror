/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tools/sch_inspection_tool.h"
#include "dialog_change_symbols.h"

#include <sch_symbol.h>
#include <id.h>
#include <kiway.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <string_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_selection.h>
#include <sim/simulator_frame.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <eda_doc.h>
#include <sch_marker.h>
#include <project.h>
#include <project_sch.h>
#include <dialogs/html_message_box.h>
#include <dialogs/dialog_erc.h>
#include <dialogs/dialog_book_reporter.h>
#include <libraries/symbol_library_adapter.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/symbol_diff_widget.h>
#include <math/util.h>      // for KiROUND

#include <dialogs/dialog_kicad_diff.h>
#include <diff_merge/sch_diff_canvas_context.h>
#include <widgets/widget_diff_canvas.h>
#include <diff_merge/sch_geometry_extractor.h>
#include <diff_merge/sch_differ.h>
#include <eeschema_helpers.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <local_history.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>
#include <wx/filename.h>


SCH_INSPECTION_TOOL::SCH_INSPECTION_TOOL() :
        SCH_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" ), m_busSyntaxHelp( nullptr )
{
}


bool SCH_INSPECTION_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::excludeMarker, SCH_CONDITIONS::SingleNonExcludedMarker, 100 );

    selToolMenu.AddItem( ACTIONS::showDatasheet,
                         SCH_CONDITIONS::SingleSymbol && SCH_CONDITIONS::Idle, 220 );

    return true;
}


void SCH_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    SCH_TOOL_BASE::Reset( aReason );

    if( aReason == SUPERMODEL_RELOAD || aReason == RESET_REASON::SHUTDOWN )
    {
        wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_ERC_DIALOG, wxID_ANY );

        wxQueueEvent( m_frame, evt );
    }
}


int SCH_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    ShowERCDialog();
    return 0;
}


void SCH_INSPECTION_TOOL::ShowERCDialog()
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, /* void */ );

    // Needed at least on Windows. Raise() is not enough
    dlg->Show( true );

    // Bring it to the top if already open.  Dual monitor users need this.
    dlg->Raise();

    if( wxButton* okButton = dynamic_cast<wxButton*>( dlg->FindWindow( wxID_OK ) ) )
    {
        KIPLATFORM::UI::ForceFocus( okButton );
        okButton->SetDefault();
    }
}


int SCH_INSPECTION_TOOL::PrevMarker( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    if( dlg )
    {
        dlg->Show( true );
        dlg->Raise();
        dlg->PrevMarker();
    }

    return 0;
}


int SCH_INSPECTION_TOOL::NextMarker( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, 0 );

    dlg->Show( true );
    dlg->Raise();
    dlg->NextMarker();

    return 0;
}


int SCH_INSPECTION_TOOL::CrossProbe( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    wxCHECK( selectionTool, 0 );

    SCH_SELECTION&      selection = selectionTool->GetSelection();

    if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
    {
        SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
        DIALOG_ERC* dlg = frame ? frame->GetErcDialog() : nullptr;

        if( dlg && dlg->IsShownOnScreen() )
            dlg->SelectMarker( static_cast<SCH_MARKER*>( selection.Front() ) );
    }

    // Show the item info on a left click on this item
    UpdateMessagePanel( aEvent );

    return 0;
}


void SCH_INSPECTION_TOOL::CrossProbe( const SCH_MARKER* aMarker )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    if( dlg )
    {
        if( !dlg->IsShownOnScreen() )
        {
            dlg->Show( true );
            dlg->Raise();
        }

        dlg->SelectMarker( aMarker );
    }
}


wxString SCH_INSPECTION_TOOL::InspectERCErrorMenuText( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    if( aERCItem->GetErrorCode() == ERCE_BUS_TO_NET_CONFLICT )
    {
        return m_frame->GetRunMenuCommandDescription( SCH_ACTIONS::showBusSyntaxHelp );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        return m_frame->GetRunMenuCommandDescription( SCH_ACTIONS::diffSymbol );
    }

    return wxEmptyString;
}


void SCH_INSPECTION_TOOL::InspectERCError( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    EDA_ITEM* a = frame->ResolveItem( aERCItem->GetMainItemID() );

    if( aERCItem->GetErrorCode() == ERCE_BUS_TO_NET_CONFLICT )
    {
        m_toolMgr->RunAction( SCH_ACTIONS::showBusSyntaxHelp );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( a ) )
            DiffSymbol( symbol );
    }
}


int SCH_INSPECTION_TOOL::ExcludeMarker( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->GetSelection();
    SCH_MARKER*         marker = nullptr;

    if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
        marker = static_cast<SCH_MARKER*>( selection.Front() );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, 0 );

    // Let the ERC dialog handle it since it owns the marker provider's cached counts and view
    // updates. If marker is nullptr the dialog excludes whichever marker is selected in the
    // dialog itself.
    dlg->ExcludeMarker( marker );

    return 0;
}


extern void CheckLibSymbol( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                            int aGridForPins, UNITS_PROVIDER* aUnitsProvider );

int SCH_INSPECTION_TOOL::CheckSymbol( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

    if( !symbol )
        return 0;

    std::vector<wxString> messages;
    const int grid_size = KiROUND( getView()->GetGAL()->GetGridSize().x );

    CheckLibSymbol( symbol, messages, grid_size, m_frame );

    if( messages.empty() )
    {
        DisplayInfoMessage( m_frame, _( "No symbol issues found." ) );
    }
    else
    {
        HTML_MESSAGE_BOX dlg( m_frame, _( "Symbol Warnings" ) );

        for( const wxString& single_msg : messages )
            dlg.AddHTML_Text( single_msg );

        dlg.ShowModal();
    }

    return 0;
}


int SCH_INSPECTION_TOOL::ShowBusSyntaxHelp( const TOOL_EVENT& aEvent )
{
    if( m_busSyntaxHelp )
    {
        m_busSyntaxHelp->Raise();
        m_busSyntaxHelp->Show( true );
        return 0;
    }

    m_busSyntaxHelp = SCH_TEXT::ShowSyntaxHelp( m_frame );
    return 0;
}


int SCH_INSPECTION_TOOL::DiffSymbol( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, 0 );

    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( selection.Empty() )
    {
        m_frame->ShowInfoBarError( _( "Select a symbol to diff against its library equivalent." ) );
        return 0;
    }

    DiffSymbol( static_cast<SCH_SYMBOL*>( selection.Front() ) );
    return 0;
}


void SCH_INSPECTION_TOOL::DiffSymbol( SCH_SYMBOL* symbol )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, /* void */ );

    DIALOG_BOOK_REPORTER* dialog = schEditorFrame->GetSymbolDiffDialog();

    wxCHECK( dialog, /* void */ );

    dialog->DeleteAllPages();
    dialog->SetUserItemID( symbol->m_Uuid );

    wxString symbolDesc = wxString::Format( _( "Symbol %s" ),
                                            symbol->GetField( FIELD_T::REFERENCE )->GetText() );
    LIB_ID   libId = symbol->GetLibId();
    wxString libName = libId.GetLibNickname();
    wxString symbolName = libId.GetLibItemName();

    WX_HTML_REPORT_BOX* r = dialog->AddHTMLPage( _( "Summary" ) );

    r->Report( wxS( "<h7>" ) + _( "Schematic vs library diff for:" ) + wxS( "</h7>" ) );
    r->Report( wxS( "<ul><li>" ) + EscapeHTML( symbolDesc ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library: " ) + EscapeHTML( libName ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library item: " ) + EscapeHTML( symbolName )
             + wxS( "</li></ul>" ) );

    r->Report( "" );

    SYMBOL_LIBRARY_ADAPTER* libs = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );

    if( !libs->HasLibrary( libName, false ) )
    {
        r->Report( _( "The library is not included in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Symbol Libraries" ) + wxS( "</a>" ) );
    }
    else if( !libs->HasLibrary( libName, true ) )
    {
        r->Report( _( "The library is not enabled in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Symbol Libraries" ) + wxS( "</a>" ) );
    }
    else
    {
        std::unique_ptr<LIB_SYMBOL> flattenedLibSymbol;
        std::unique_ptr<LIB_SYMBOL> flattenedSchSymbol = symbol->GetLibSymbolRef()->Flatten();

        try
        {
            if( LIB_SYMBOL* libAlias = libs->LoadSymbol( libName, symbolName ) )
                flattenedLibSymbol = libAlias->Flatten();
        }
        catch( const IO_ERROR& )
        {
        }

        if( !flattenedLibSymbol )
        {
            r->Report( wxString::Format( _( "The library no longer contains the item %s." ),
                                         symbolName ) );
        }
        else
        {
            std::vector<SCH_FIELD> fields;

            for( SCH_FIELD& field : symbol->GetFields() )
            {
                fields.emplace_back( SCH_FIELD( flattenedLibSymbol.get(), field.GetId(),
                                                field.GetName( false ) ) );
                fields.back().CopyText( field );
                fields.back().SetAttributes( field );
                fields.back().Move( -symbol->GetPosition() );
            }

            flattenedSchSymbol->SetFields( fields );

            if( flattenedSchSymbol->Compare( *flattenedLibSymbol, SCH_ITEM::COMPARE_FLAGS::ERC,
                                             r ) == 0 )
            {
                r->Report( _( "No relevant differences detected." ) );
            }

            wxPanel*            panel = dialog->AddBlankPage( _( "Visual" ) );
            SYMBOL_DIFF_WIDGET* diff = constructDiffPanel( panel );

            diff->DisplayDiff( flattenedSchSymbol.release(), flattenedLibSymbol.release(),
                               symbol->GetUnit(), symbol->GetBodyStyle() );
        }
    }

    r->Flush();

    dialog->Raise();
    dialog->Show( true );
}


SYMBOL_DIFF_WIDGET* SCH_INSPECTION_TOOL::constructDiffPanel( wxPanel* aParentPanel )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = m_frame->GetCanvas()->GetBackend();
    SYMBOL_DIFF_WIDGET*          diffWidget = new SYMBOL_DIFF_WIDGET( aParentPanel, backend );

   	sizer->Add( diffWidget, 1, wxEXPAND | wxALL, 5 );
    aParentPanel->SetSizer( sizer );
    aParentPanel->Layout();

    return diffWidget;
}


namespace
{
void buildSchOverrides( const KICAD_DIFF::DOCUMENT_DIFF& aDiff, const KICAD_DIFF::DIFF_COLOR_THEME& aTheme,
                        std::map<KIID, KIGFX::COLOR4D>& aRefO, std::map<KIID, KIGFX::COLOR4D>& aCompO,
                        std::map<KIID, KICAD_DIFF::CATEGORY>& aCats )
{
    aRefO.clear();
    aCompO.clear();
    aCats.clear();

    std::function<void( const KICAD_DIFF::ITEM_CHANGE& )> visit = [&]( const KICAD_DIFF::ITEM_CHANGE& aChange )
    {
        if( !aChange.id.empty() )
        {
            const KIID& kiid = aChange.id.back();
            aCats[kiid] = KICAD_DIFF::CategoryFor( aChange.kind );

            switch( aChange.kind )
            {
            case KICAD_DIFF::CHANGE_KIND::ADDED: aCompO[kiid] = aTheme.added; break;
            case KICAD_DIFF::CHANGE_KIND::REMOVED:
                aRefO[kiid] = aTheme.removed;
                aCompO[kiid] = aTheme.removed;
                break;
            case KICAD_DIFF::CHANGE_KIND::MODIFIED:
                aRefO[kiid] = aTheme.modified;
                aCompO[kiid] = aTheme.modified;
                break;
            default:
                aRefO[kiid] = aTheme.conflict;
                aCompO[kiid] = aTheme.conflict;
                break;
            }
        }

        for( const KICAD_DIFF::ITEM_CHANGE& child : aChange.children )
            visit( child );
    };

    for( const KICAD_DIFF::ITEM_CHANGE& change : aDiff.changes )
        visit( change );
}


DIALOG_KICAD_DIFF::SHEET_SWITCHER makeSchSwitcher( SCHEMATIC* aRef, SCHEMATIC* aComp,
                                                   std::map<KIID, KIGFX::COLOR4D>       aRefOverrides,
                                                   std::map<KIID, KIGFX::COLOR4D>       aCompOverrides,
                                                   std::map<KIID, KICAD_DIFF::CATEGORY> aCategories,
                                                   const KICAD_DIFF::DIFF_COLOR_THEME&  aTheme )
{
    auto holder = std::make_shared<std::vector<std::unique_ptr<SCH_ITEM>>>();

    return [aRef, aComp, modifiedColor = aTheme.modified, removedColor = aTheme.removed,
            refO = std::move( aRefOverrides ), compO = std::move( aCompOverrides ), cats = std::move( aCategories ),
            holder]( WIDGET_DIFF_CANVAS& aCanvas, const KIID_PATH& aSheetPath )
    {
        SCH_SCREEN* compScreen = aComp->RootScreen();
        SCH_SCREEN* refScreen = aRef->RootScreen();

        if( !aSheetPath.empty() )
        {
            if( auto sp = aComp->Hierarchy().GetSheetPathByKIIDPath( aSheetPath, true ) )
                compScreen = sp->LastScreen();

            if( auto sp = aRef->Hierarchy().GetSheetPathByKIIDPath( aSheetPath, true ) )
                refScreen = sp->LastScreen();
        }

        holder->clear();

        if( refScreen )
        {
            for( const auto& [kiid, c] : refO )
            {
                if( c != removedColor )
                    continue;

                for( SCH_ITEM* item : refScreen->Items() )
                {
                    if( item && item->m_Uuid == kiid )
                    {
                        if( auto* clone = dynamic_cast<SCH_ITEM*>( item->Clone() ) )
                            holder->emplace_back( clone );

                        break;
                    }
                }
            }
        }

        std::vector<KIGFX::VIEW_ITEM*> extras;

        for( const auto& clone : *holder )
            extras.push_back( clone.get() );

        KICAD_DIFF::ConfigureSchDiffCanvasContext( aCanvas, nullptr, aComp, modifiedColor, compO, extras, cats, nullptr,
                                                   compScreen );
    };
}
} // namespace


int SCH_INSPECTION_TOOL::CompareSchematicWithFile( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, 0 );

    wxFileDialog dlg( schEditorFrame, _( "Choose Schematic to Compare With" ), wxEmptyString,
                      wxEmptyString, FILEEXT::KiCadSchematicFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    wxFileName otherFn( dlg.GetPath() );
    otherFn.MakeAbsolute();

    if( !otherFn.GetExt().IsSameAs( FILEEXT::KiCadSchematicFileExtension, false ) )
    {
        schEditorFrame->ShowInfoBarError(
                _( "Select a KiCad s-expression schematic file (.kicad_sch)." ) );
        return 0;
    }

    const wxString otherPath = otherFn.GetFullPath();

    wxFileName projectFn = otherFn;
    projectFn.SetExt( FILEEXT::ProjectFileExtension );
    const wxString projectPath = projectFn.GetFullPath();

    wxFileName activeProjectFn( schEditorFrame->Prj().GetProjectFullName() );
    activeProjectFn.MakeAbsolute();

    // Refuse the self-compare; attaching the active PROJECT to a second
    // SCHEMATIC would clobber its ERC/schematic settings.
    if( projectFn.SameAs( activeProjectFn ) )
    {
        schEditorFrame->ShowInfoBarError(
                _( "Select a schematic file from another project to compare." ) );
        return 0;
    }

    return showSchematicComparison( otherPath, projectPath, otherPath );
}


int SCH_INSPECTION_TOOL::showSchematicComparison( const wxString& aOtherPath, const wxString& aProjectPath,
                                                  const wxString& aComparisonLabel )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, 0 );

    SETTINGS_MANAGER* mgr = schEditorFrame->GetSettingsManager();

    wxCHECK( mgr, 0 );

    // A missing .kicad_pro is fine (LoadProject returns false but still
    // inserts a defaults-only slot); a present-but-malformed .kicad_pro is
    // treated as failure.
    bool     projectLoadOk = mgr->LoadProject( aProjectPath, false );
    PROJECT* otherPrj = mgr->GetProject( aProjectPath );

    if( !otherPrj )
    {
        schEditorFrame->ShowInfoBarError( wxString::Format( _( "Failed to load project for %s" ), aOtherPath ) );
        return 0;
    }

    if( !projectLoadOk && wxFileName( aProjectPath ).FileExists() )
    {
        mgr->UnloadProject( otherPrj, false );
        schEditorFrame->ShowInfoBarError( wxString::Format( _( "Failed to load project for %s" ), aOtherPath ) );
        return 0;
    }

    // SCH_DIFFER walks the sheet hierarchy and properties; connectivity build
    // is unnecessary and slow for a read-only compare.
    SCHEMATIC* loadedSchematic = nullptr;

    try
    {
        loadedSchematic = EESCHEMA_HELPERS::LoadSchematic( aOtherPath,
                                                           /*aSetActive=*/false,
                                                           /*aForceDefaultProject=*/false, otherPrj,
                                                           /*aCalculateConnectivity=*/false );
    }
    catch( ... )
    {
        schEditorFrame->ShowInfoBarError( wxString::Format( _( "Failed to load %s" ), aOtherPath ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }

    SCHEMATIC* mySch = &schEditorFrame->Schematic();

    // Belt-and-suspenders: EESCHEMA_HELPERS::LoadSchematic has a fallback
    // branch that returns the active editor's schematic when the project
    // happens to be the active one. The path guard above should have caught
    // this, but a path-alias miss would leave the unique_ptr owning the live
    // schematic and SetProject(nullptr) would destroy editor state.
    if( loadedSchematic == mySch )
    {
        schEditorFrame->ShowInfoBarError(
                _( "Select a schematic file from another project to compare." ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }

    std::unique_ptr<SCHEMATIC> otherSchematic{ loadedSchematic };

    if( !otherSchematic )
    {
        schEditorFrame->ShowInfoBarError( wxString::Format( _( "Failed to load %s" ), aOtherPath ) );
        mgr->UnloadProject( otherPrj, false );
        return 0;
    }

    KICAD_DIFF::SCH_DIFFER differ( mySch, otherSchematic.get(), aOtherPath );

    // Scope to the editor's current sheet. Comparison resolves the matching
    // sheet by KIID when present, else falls back to its root.
    const KIID_PATH      scopeBefore = schEditorFrame->GetCurrentSheet().Path();
    const SCH_SHEET_LIST otherSheets = otherSchematic->BuildSheetListSortedByPageNumbers();
    KIID_PATH            scopeAfter;

    if( auto sp = otherSchematic->Hierarchy().GetSheetPathByKIIDPath( scopeBefore, true ) )
        scopeAfter = sp->Path();
    else if( !otherSheets.empty() )
        scopeAfter = otherSheets.front().Path();

    if( !scopeBefore.empty() && !scopeAfter.empty() )
        differ.SetScope( scopeBefore, scopeAfter );

    KICAD_DIFF::DOCUMENT_DIFF result = differ.Diff();

    const KICAD_DIFF::DIFF_COLOR_THEME theme;

    std::map<KIID, KIGFX::COLOR4D>       refOverrides;
    std::map<KIID, KIGFX::COLOR4D>       compOverrides;
    std::map<KIID, KICAD_DIFF::CATEGORY> kiidCategories;

    buildSchOverrides( result, theme, refOverrides, compOverrides, kiidCategories );

    KICAD_DIFF::DOCUMENT_GEOMETRY refGeometry =
            KICAD_DIFF::ExtractSchematicGeometry( *mySch, theme.reference, refOverrides );
    KICAD_DIFF::DOCUMENT_GEOMETRY compGeometry =
            KICAD_DIFF::ExtractSchematicGeometry( *otherSchematic, theme.comparison, compOverrides );

    auto initialSwitcher =
            makeSchSwitcher( mySch, otherSchematic.get(), refOverrides, compOverrides, kiidCategories, theme );

    KIID_PATH   initialSheet = schEditorFrame->GetCurrentSheet().Path();
    SCH_SCREEN* currentSheetScreen = schEditorFrame->GetCurrentSheet().LastScreen();
    wxString    referenceLabel = currentSheetScreen ? currentSheetScreen->GetFileName() : mySch->GetFileName();

    DIALOG_KICAD_DIFF dlgDiff( schEditorFrame, referenceLabel, aComparisonLabel, result, std::move( refGeometry ),
                               std::move( compGeometry ), std::move( initialSwitcher ), std::move( initialSheet ) );

    // Drill state owns the comparison schematics loaded across double-click
    // drills. The editor schematic stays the reference on all drills.
    struct DRILL_STATE
    {
        SCH_SHEET_PATH                          editorPath;
        SCHEMATIC*                              compSch;
        wxString                                compFile;
        std::vector<std::unique_ptr<SCHEMATIC>> ownedSchs;
    };

    DRILL_STATE drillState;
    drillState.editorPath = schEditorFrame->GetCurrentSheet();
    drillState.compSch = otherSchematic.get();
    drillState.compFile = aOtherPath;
    drillState.ownedSchs.push_back( std::move( otherSchematic ) );

    if( WIDGET_DIFF_CANVAS* canvas = dlgDiff.DiffCanvas() )
    {
        canvas->SetDoubleClickHandler(
                [&dlgDiff, &drillState, mySch, otherPrj, theme, schEditorFrame]( KIGFX::VIEW_ITEM* aItem )
                {
                    auto* sheet = dynamic_cast<SCH_SHEET*>( aItem );

                    if( !sheet )
                        return;

                    const wxString sheetFile = sheet->GetFileName();

                    if( sheetFile.IsEmpty() )
                        return;

                    KIID_PATH newEditorKiid = drillState.editorPath.Path();
                    newEditorKiid.push_back( sheet->m_Uuid );

                    auto newEditorSheet = mySch->Hierarchy().GetSheetPathByKIIDPath( newEditorKiid, true );

                    if( !newEditorSheet )
                        return;

                    // Prefer resolving inside the current comparison schematic so
                    // shared-sheet instance context is preserved on both sides.
                    SCHEMATIC* compSch = drillState.compSch;
                    wxString   compFile = drillState.compFile;
                    KIID_PATH  scopeBefore = newEditorKiid;
                    KIID_PATH  scopeAfter;

                    if( auto compMatch = compSch->Hierarchy().GetSheetPathByKIIDPath( newEditorKiid, true ) )
                    {
                        scopeAfter = compMatch->Path();

                        if( SCH_SCREEN* compMatchScreen = compMatch->LastScreen() )
                            compFile = compMatchScreen->GetFileName();
                    }
                    else
                    {
                        wxFileName newCompFn( wxFileName( drillState.compFile ).GetPath(), sheetFile );
                        newCompFn.MakeAbsolute();

                        SCHEMATIC* loaded =
                                EESCHEMA_HELPERS::LoadSchematic( newCompFn.GetFullPath(), /*aSetActive=*/false,
                                                                 /*aForceDefaultProject=*/false, otherPrj,
                                                                 /*aCalculateConnectivity=*/false );

                        if( !loaded )
                        {
                            schEditorFrame->ShowInfoBarError(
                                    wxString::Format( _( "Failed to load %s" ), newCompFn.GetFullPath() ) );
                            return;
                        }

                        const SCH_SHEET_LIST loadedSheets = loaded->BuildSheetListSortedByPageNumbers();

                        if( !loadedSheets.empty() )
                            scopeAfter = loadedSheets.front().Path();

                        drillState.ownedSchs.emplace_back( loaded );
                        compSch = loaded;
                        compFile = newCompFn.GetFullPath();
                    }

                    KICAD_DIFF::SCH_DIFFER newDiffer( mySch, compSch, compFile );

                    if( !scopeBefore.empty() && !scopeAfter.empty() )
                        newDiffer.SetScope( scopeBefore, scopeAfter );

                    KICAD_DIFF::DOCUMENT_DIFF newDiff = newDiffer.Diff();

                    std::map<KIID, KIGFX::COLOR4D>       newRefO;
                    std::map<KIID, KIGFX::COLOR4D>       newCompO;
                    std::map<KIID, KICAD_DIFF::CATEGORY> newCats;
                    buildSchOverrides( newDiff, theme, newRefO, newCompO, newCats );

                    auto newSwitcher = makeSchSwitcher( mySch, compSch, newRefO, newCompO, newCats, theme );

                    drillState.editorPath = *newEditorSheet;
                    drillState.compSch = compSch;
                    drillState.compFile = compFile;

                    SCH_SCREEN* newRefScreen = newEditorSheet->LastScreen();
                    wxString    newRefLabel = newRefScreen ? newRefScreen->GetFileName() : wxString();

                    dlgDiff.Reload( newRefLabel, compFile, std::move( newDiff ),
                                    /*aReferenceGeometry=*/{}, /*aComparisonGeometry=*/{}, std::move( newSwitcher ),
                                    scopeBefore );
                } );
    }

    dlgDiff.ShowModal();

    // Detach schematics from the project before unloading so the project's
    // ERC and schematic settings release cleanly.
    for( auto& sch : drillState.ownedSchs )
    {
        if( sch )
            sch->SetProject( nullptr );
    }

    drillState.ownedSchs.clear();

    mgr->UnloadProject( otherPrj, false );

    return 0;
}


int SCH_INSPECTION_TOOL::CompareSchematicWithHistory( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, 0 );

    SCHEMATIC* mySch = &schEditorFrame->Schematic();

    if( mySch->GetFileName().IsEmpty() )
    {
        schEditorFrame->ShowInfoBarError( _( "Save the schematic before comparing against local history." ) );
        return 0;
    }

    const wxString projectPath = schEditorFrame->Prj().GetProjectPath();
    LOCAL_HISTORY& history = schEditorFrame->Kiway().LocalHistory();

    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> snapshots = history.GetSnapshots( projectPath );

    if( snapshots.empty() )
    {
        schEditorFrame->ShowInfoBarError( _( "No local history snapshots for this project." ) );
        return 0;
    }

    SETTINGS_MANAGER* mgr = schEditorFrame->GetSettingsManager();

    wxCHECK( mgr, 0 );

    auto relTo = [&]( const wxString& aFull )
    {
        wxFileName fn( aFull );
        fn.MakeRelativeTo( projectPath );
        return fn.GetFullPath( wxPATH_UNIX );
    };

    const wxString rootRel = relTo( mySch->GetFileName() );
    const wxString projRel = relTo( schEditorFrame->Prj().GetProjectFullName() );

    // One entry per distinct schematic version: newest-first, skipping commits
    // with no schematic or whose schematic content (any .kicad_sch sheet) matches
    // the previously kept one. This drops board-only saves like "PCB Save".
    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> filtered;
    wxString                                 prevFingerprint;

    for( const LOCAL_HISTORY_SNAPSHOT_INFO& s : snapshots )
    {
        wxString fingerprint = history.TreeFingerprint( projectPath, s.hash, wxS( ".kicad_sch" ) );

        if( fingerprint.IsEmpty() || fingerprint == prevFingerprint )
            continue;

        prevFingerprint = fingerprint;
        filtered.push_back( s );
    }

    if( filtered.empty() )
    {
        schEditorFrame->ShowInfoBarError( _( "No local history snapshots change this schematic." ) );
        return 0;
    }

    snapshots = std::move( filtered );

    std::vector<wxString> labels;

    for( const LOCAL_HISTORY_SNAPSHOT_INFO& s : snapshots )
    {
        wxString summary = s.summary.IsEmpty() ? s.message.BeforeFirst( '\n' ) : s.summary;
        labels.push_back( wxString::Format( wxS( "%s (%s)" ), summary, s.hash.Left( 8 ) ) );
    }

    const KICAD_DIFF::DIFF_COLOR_THEME theme;
    const KIID_PATH                    scopeBefore = schEditorFrame->GetCurrentSheet().Path();

    struct SCH_DIFF_VIEW
    {
        KICAD_DIFF::DOCUMENT_DIFF         result;
        KICAD_DIFF::DOCUMENT_GEOMETRY     refGeom;
        KICAD_DIFF::DOCUMENT_GEOMETRY     compGeom;
        DIALOG_KICAD_DIFF::SHEET_SWITCHER switcher;
    };

    auto buildView = [&]( SCHEMATIC* aComp, const wxString& aPath ) -> SCH_DIFF_VIEW
    {
        SCH_DIFF_VIEW          view;
        KICAD_DIFF::SCH_DIFFER differ( mySch, aComp, aPath );

        KIID_PATH scopeAfter;

        if( auto sp = aComp->Hierarchy().GetSheetPathByKIIDPath( scopeBefore, true ) )
        {
            scopeAfter = sp->Path();
        }
        else
        {
            SCH_SHEET_LIST sheets = aComp->BuildSheetListSortedByPageNumbers();

            if( !sheets.empty() )
                scopeAfter = sheets.front().Path();
        }

        if( !scopeBefore.empty() && !scopeAfter.empty() )
            differ.SetScope( scopeBefore, scopeAfter );

        view.result = differ.Diff();

        std::map<KIID, KIGFX::COLOR4D>       refO;
        std::map<KIID, KIGFX::COLOR4D>       compO;
        std::map<KIID, KICAD_DIFF::CATEGORY> cats;
        buildSchOverrides( view.result, theme, refO, compO, cats );

        view.refGeom = KICAD_DIFF::ExtractSchematicGeometry( *mySch, theme.reference, refO );
        view.compGeom = KICAD_DIFF::ExtractSchematicGeometry( *aComp, theme.comparison, compO );
        view.switcher = makeSchSwitcher( mySch, aComp, refO, compO, cats, theme );

        return view;
    };

    // State for the revision currently shown. Swapped on each dropdown change.
    std::unique_ptr<SCHEMATIC> curSch;
    PROJECT*                   curPrj = nullptr;
    wxString                   curTempDir;

    // Drill state: which comparison sheet is shown and sub-schematics loaded on
    // double-click. Reset when the revision changes.
    SCH_SHEET_PATH                          drillEditorPath = schEditorFrame->GetCurrentSheet();
    SCHEMATIC*                              drillCompSch = nullptr;
    wxString                                drillCompFile;
    std::vector<std::unique_ptr<SCHEMATIC>> drilledSchs;

    auto cleanupCurrent = [&]()
    {
        for( auto& sch : drilledSchs )
        {
            if( sch )
                sch->SetProject( nullptr );
        }

        drilledSchs.clear();

        if( curSch )
        {
            curSch->SetProject( nullptr );
            curSch.reset();
        }

        // Skip if the project was already evicted from the manager.
        if( curPrj && mgr->IsProjectLoaded( curPrj ) )
            mgr->UnloadProject( curPrj, false );

        curPrj = nullptr;

        if( !curTempDir.IsEmpty() )
        {
            wxFileName::Rmdir( curTempDir, wxPATH_RMDIR_RECURSIVE );
            curTempDir.Clear();
        }
    };

    // Extract the hierarchy at snapshot aIndex and load its root sheet, cleaning
    // up the temp dir on any failure.
    auto loadRevision = [&]( int aIndex, std::unique_ptr<SCHEMATIC>& aSch, PROJECT*& aPrj, wxString& aTempDir ) -> bool
    {
        const wxString hash = snapshots[aIndex].hash;
        wxFileName     dirFn;
        dirFn.AssignDir( wxFileName::GetTempDir() );
        dirFn.AppendDir( wxS( "kicad-history-" ) + hash.Left( 8 ) );
        const wxString tempDir = dirFn.GetPath();

        // Extract just the schematic sheets and project file, skipping the
        // board, 3D models, gerbers, etc.
        if( !history.ExtractAllFilesAtCommit( projectPath, hash, tempDir,
                                              { wxS( ".kicad_sch" ), wxS( ".kicad_pro" ) } ) )
        {
            wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
            return false;
        }

        const wxString root = tempDir + wxS( "/" ) + rootRel;
        const wxString proj = tempDir + wxS( "/" ) + projRel;

        mgr->LoadProject( proj, false );
        PROJECT* prj = mgr->GetProject( proj );

        if( !prj )
        {
            wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
            return false;
        }

        SCHEMATIC* loaded = nullptr;

        try
        {
            loaded = EESCHEMA_HELPERS::LoadSchematic( root, /*aSetActive=*/false, /*aForceDefaultProject=*/false, prj,
                                                      /*aCalculateConnectivity=*/false );
        }
        catch( ... )
        {
            mgr->UnloadProject( prj, false );
            wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
            return false;
        }

        if( !loaded || loaded == mySch )
        {
            mgr->UnloadProject( prj, false );
            wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
            return false;
        }

        aSch.reset( loaded );
        aPrj = prj;
        aTempDir = tempDir;
        return true;
    };

    auto loadView = [&]( int aIndex, std::unique_ptr<SCHEMATIC>& aSch, PROJECT*& aPrj, wxString& aTempDir,
                         SCH_DIFF_VIEW& aView ) -> bool
    {
        if( !loadRevision( aIndex, aSch, aPrj, aTempDir ) )
            return false;

        try
        {
            aView = buildView( aSch.get(), aTempDir + wxS( "/" ) + rootRel );
        }
        catch( ... )
        {
            aSch->SetProject( nullptr );
            aSch.reset();
            mgr->UnloadProject( aPrj, false );
            aPrj = nullptr;
            wxFileName::Rmdir( aTempDir, wxPATH_RMDIR_RECURSIVE );
            aTempDir.Clear();
            return false;
        }

        return true;
    };

    SCH_DIFF_VIEW view;
    int           startIndex = 0;

    if( !loadView( 0, curSch, curPrj, curTempDir, view ) )
    {
        schEditorFrame->ShowInfoBarError( _( "Could not compare against the selected snapshot." ) );
        return 0;
    }

    // Default to the first commit back from HEAD that actually differs from the
    // current schematic, so opening lands on real changes rather than an in-sync HEAD.
    while( view.result.Empty() && startIndex + 1 < static_cast<int>( snapshots.size() ) )
    {
        std::unique_ptr<SCHEMATIC> nextSch;
        PROJECT*                   nextPrj = nullptr;
        wxString                   nextTempDir;
        SCH_DIFF_VIEW              nextView;

        if( !loadView( startIndex + 1, nextSch, nextPrj, nextTempDir, nextView ) )
            break;

        cleanupCurrent();
        view = std::move( nextView );
        curSch = std::move( nextSch );
        curPrj = nextPrj;
        curTempDir = nextTempDir;
        startIndex++;
    }

    drillCompSch = curSch.get();
    drillCompFile = curTempDir + wxS( "/" ) + rootRel;

    SCH_SCREEN* curScreen = schEditorFrame->GetCurrentSheet().LastScreen();
    wxString    referenceLabel = curScreen ? curScreen->GetFileName() : mySch->GetFileName();

    auto dlgDiff = std::make_unique<DIALOG_KICAD_DIFF>( schEditorFrame, referenceLabel, labels[startIndex], view.result,
                                                        view.refGeom, view.compGeom, view.switcher, scopeBefore );

    // Double-click a sheet to drill into its sub-schematic, within the current
    // revision's extracted hierarchy.
    if( WIDGET_DIFF_CANVAS* canvas = dlgDiff->DiffCanvas() )
    {
        canvas->SetDoubleClickHandler(
                [&]( KIGFX::VIEW_ITEM* aItem )
                {
                    auto* sheet = dynamic_cast<SCH_SHEET*>( aItem );

                    if( !sheet || sheet->GetFileName().IsEmpty() )
                        return;

                    KIID_PATH newEditorKiid = drillEditorPath.Path();
                    newEditorKiid.push_back( sheet->m_Uuid );

                    auto newEditorSheet = mySch->Hierarchy().GetSheetPathByKIIDPath( newEditorKiid, true );

                    if( !newEditorSheet )
                        return;

                    SCHEMATIC* compSch = drillCompSch;
                    wxString   compFile = drillCompFile;
                    KIID_PATH  drillScopeBefore = newEditorKiid;
                    KIID_PATH  drillScopeAfter;

                    if( auto compMatch = compSch->Hierarchy().GetSheetPathByKIIDPath( newEditorKiid, true ) )
                    {
                        drillScopeAfter = compMatch->Path();

                        if( SCH_SCREEN* matchScreen = compMatch->LastScreen() )
                            compFile = matchScreen->GetFileName();
                    }
                    else
                    {
                        wxFileName subFn( wxFileName( drillCompFile ).GetPath(), sheet->GetFileName() );
                        subFn.MakeAbsolute();

                        SCHEMATIC* loaded = nullptr;

                        try
                        {
                            loaded = EESCHEMA_HELPERS::LoadSchematic( subFn.GetFullPath(), /*aSetActive=*/false,
                                                                      /*aForceDefaultProject=*/false, curPrj,
                                                                      /*aCalculateConnectivity=*/false );
                        }
                        catch( ... )
                        {
                            loaded = nullptr;
                        }

                        if( !loaded || loaded == mySch )
                        {
                            schEditorFrame->ShowInfoBarError(
                                    wxString::Format( _( "Failed to load %s" ), subFn.GetFullPath() ) );
                            return;
                        }

                        SCH_SHEET_LIST loadedSheets = loaded->BuildSheetListSortedByPageNumbers();

                        if( !loadedSheets.empty() )
                            drillScopeAfter = loadedSheets.front().Path();

                        drilledSchs.emplace_back( loaded );
                        compSch = loaded;
                        compFile = subFn.GetFullPath();
                    }

                    try
                    {
                        KICAD_DIFF::SCH_DIFFER newDiffer( mySch, compSch, compFile );

                        if( !drillScopeBefore.empty() && !drillScopeAfter.empty() )
                            newDiffer.SetScope( drillScopeBefore, drillScopeAfter );

                        KICAD_DIFF::DOCUMENT_DIFF newDiff = newDiffer.Diff();

                        std::map<KIID, KIGFX::COLOR4D>       newRefO;
                        std::map<KIID, KIGFX::COLOR4D>       newCompO;
                        std::map<KIID, KICAD_DIFF::CATEGORY> newCats;
                        buildSchOverrides( newDiff, theme, newRefO, newCompO, newCats );

                        auto newSwitcher = makeSchSwitcher( mySch, compSch, newRefO, newCompO, newCats, theme );

                        SCH_SCREEN* newRefScreen = newEditorSheet->LastScreen();
                        wxString    newRefLabel = newRefScreen ? newRefScreen->GetFileName() : wxString();

                        dlgDiff->Reload( newRefLabel, compFile, std::move( newDiff ), /*aReferenceGeometry=*/{},
                                         /*aComparisonGeometry=*/{}, std::move( newSwitcher ), drillScopeBefore );
                    }
                    catch( ... )
                    {
                        schEditorFrame->ShowInfoBarError( _( "Could not open this sheet for comparison." ) );
                        return;
                    }

                    drillEditorPath = *newEditorSheet;
                    drillCompSch = compSch;
                    drillCompFile = compFile;
                } );
    }

    dlgDiff->SetRevisionChooser( labels, startIndex,
                                 [&]( int aIndex )
                                 {
                                     std::unique_ptr<SCHEMATIC> newSch;
                                     PROJECT*                   newPrj = nullptr;
                                     wxString                   newTempDir;
                                     SCH_DIFF_VIEW              newView;

                                     if( !loadView( aIndex, newSch, newPrj, newTempDir, newView ) )
                                     {
                                         schEditorFrame->ShowInfoBarError(
                                                 _( "Could not compare against the selected snapshot." ) );
                                         return;
                                     }

                                     dlgDiff->Reload( referenceLabel, labels[aIndex], newView.result, newView.refGeom,
                                                      newView.compGeom, newView.switcher, scopeBefore );

                                     cleanupCurrent();
                                     view = std::move( newView );
                                     curSch = std::move( newSch );
                                     curPrj = newPrj;
                                     curTempDir = newTempDir;

                                     // Restart drilling from the new revision's root.
                                     drillEditorPath = schEditorFrame->GetCurrentSheet();
                                     drillCompSch = curSch.get();
                                     drillCompFile = curTempDir + wxS( "/" ) + rootRel;
                                 } );

    dlgDiff->ShowModal();

    // Destroy the dialog before the schematics it references are freed.
    dlgDiff.reset();

    cleanupCurrent();
    return 0;
}


int SCH_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
    SIMULATOR_FRAME* simFrame = (SIMULATOR_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );

    if( !simFrame )
        return -1;

    if( wxWindow* blocking_win = simFrame->Kiway().GetBlockingDialog() )
        blocking_win->Close( true );

    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();

    return 0;
}


int SCH_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    wxString datasheet;
    std::vector<EMBEDDED_FILES*> filesStack;

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return 0;

        datasheet = symbol->GetDatasheetField().GetText();
        filesStack.push_back( symbol );
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) )
    {
        LIB_SYMBOL* entry = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame )->GetSelectedSymbol();

        if( !entry )
            return 0;

        datasheet = entry->GetDatasheetField().GetText();
        filesStack.push_back( entry );
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

        if( selection.Empty() )
            return 0;

        SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();
        SCH_FIELD*  field = symbol->GetField( FIELD_T::DATASHEET );

        // Use GetShownText() to resolve any text variables, but don't allow adding extra text
        // (ie: the field name)
        datasheet = field->GetShownText( &symbol->Schematic()->CurrentSheet(), false );
        filesStack.push_back( symbol->Schematic() );

        if( symbol->GetLibSymbolRef() )
            filesStack.push_back( symbol->GetLibSymbolRef().get() );
    }

    if( datasheet.IsEmpty() || datasheet == wxS( "~" ) )
    {
        m_frame->ShowInfoBarError( _( "No datasheet defined." ) );
    }
    else
    {
        GetAssociatedDocument( m_frame, datasheet, &m_frame->Prj(),
                               PROJECT_SCH::SchSearchS( &m_frame->Prj() ), filesStack );
    }

    return 0;
}


int SCH_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDIT_FRAME*  symbolEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );
    SCH_EDIT_FRAME*     schEditFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->GetSelection();

    // Note: the symbol viewer manages its own message panel

    if( symbolEditFrame || schEditFrame )
    {
        if( selection.GetSize() == 1 )
        {
            EDA_ITEM*                   item = (EDA_ITEM*) selection.Front();
            std::vector<MSG_PANEL_ITEM> msgItems;

            if( std::optional<wxString> uuid = GetMsgPanelDisplayUuid( item->m_Uuid ) )
                msgItems.emplace_back( _( "UUID" ), *uuid );

            item->GetMsgPanelInfo( m_frame, msgItems );
            m_frame->SetMsgPanel( msgItems );
        }
        else
        {
            m_frame->ClearMsgPanel();
        }
    }

    if( schEditFrame )
    {
        schEditFrame->UpdateNetHighlightStatus();
        schEditFrame->UpdateHierarchySelection();
    }

    return 0;
}


void SCH_INSPECTION_TOOL::setTransitions()
{
    Go( &SCH_INSPECTION_TOOL::RunERC,                SCH_ACTIONS::runERC.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::PrevMarker,            SCH_ACTIONS::prevMarker.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::NextMarker,            SCH_ACTIONS::nextMarker.MakeEvent() );
    // See note 1:
    Go( &SCH_INSPECTION_TOOL::CrossProbe,            EVENTS::PointSelectedEvent );
    Go( &SCH_INSPECTION_TOOL::CrossProbe,            EVENTS::SelectedEvent );
    Go( &SCH_INSPECTION_TOOL::ExcludeMarker,         SCH_ACTIONS::excludeMarker.MakeEvent() );

    Go( &SCH_INSPECTION_TOOL::CheckSymbol,           SCH_ACTIONS::checkSymbol.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::DiffSymbol,            SCH_ACTIONS::diffSymbol.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::CompareSchematicWithFile,
        SCH_ACTIONS::compareSchematicWithFile.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::CompareSchematicWithHistory, SCH_ACTIONS::compareSchematicWithHistory.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::RunSimulation,         SCH_ACTIONS::showSimulator.MakeEvent() );
    Go( &SCH_INSPECTION_TOOL::ShowBusSyntaxHelp,     SCH_ACTIONS::showBusSyntaxHelp.MakeEvent() );

    Go( &SCH_INSPECTION_TOOL::ShowDatasheet,         ACTIONS::showDatasheet.MakeEvent() );

    // Note 1: tUpdateMessagePanel is called by CrossProbe. So uncomment this line if
    // call to CrossProbe is modifiied
    // Go( &SCH_INSPECTION_TOOL::UpdateMessagePanel, EVENTS::SelectedEvent );
    Go( &SCH_INSPECTION_TOOL::UpdateMessagePanel,    EVENTS::UnselectedEvent );
    Go( &SCH_INSPECTION_TOOL::UpdateMessagePanel,    EVENTS::ClearedEvent );
    Go( &SCH_INSPECTION_TOOL::UpdateMessagePanel,    EVENTS::SelectedItemsModified );
}

