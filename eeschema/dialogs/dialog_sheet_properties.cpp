/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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

#include "dialog_sheet_properties.h"

#include <kiface_base.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/tooltip.h>
#include <common.h>
#include <confirm.h>
#include <kidialog.h>
#include <validators.h>
#include <wx_filename.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <kiplatform/ui.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_io/sch_io.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <bitmaps.h>
#include <eeschema_settings.h>
#include <settings/color_settings.h>
#include <trace_helpers.h>
#include "panel_eeschema_color_settings.h"
#include "wx/dcclient.h"
#include "string_utils.h"

DIALOG_SHEET_PROPERTIES::DIALOG_SHEET_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHEET* aSheet,
                                                  bool* aIsUndoable, bool* aClearAnnotationNewItems,
                                                  bool* aUpdateHierarchyNavigator,
                                                  wxString* aSourceSheetFilename ) :
        DIALOG_SHEET_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_isUndoable( aIsUndoable ),
        m_clearAnnotationNewItems( aClearAnnotationNewItems ),
        m_updateHierarchyNavigator( aUpdateHierarchyNavigator ),
        m_sourceSheetFilename( aSourceSheetFilename ),
        m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_dummySheet( *aSheet ),
        m_dummySheetNameField( &m_dummySheet, FIELD_T::SHEET_NAME )
{
    m_sheet = aSheet;
    m_fields = new FIELDS_GRID_TABLE( this, aParent, m_grid, m_sheet );
    m_delayedFocusRow = 0;
    m_delayedFocusColumn = FDC_VALUE;

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this, { &aParent->Schematic() },
                                                      [&]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddField( aEvent );
                                                      } ) );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_grid->ShowHideColumns( "0 1 2 3 4 5 6 7" );
    m_shownColumns = m_grid->GetShownColumns();

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    wxSize minSize = m_pageNumberTextCtrl->GetMinSize();
    int    minWidth = m_pageNumberTextCtrl->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_pageNumberTextCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    wxToolTip::Enable( true );
    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // Set font sizes
    m_hierarchicalPathLabel->SetFont( KIUI::GetSmallInfoFont( this ) );
    m_hierarchicalPath->SetFont( KIUI::GetSmallInfoFont( this ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_SHEET_PROPERTIES::OnGridCellChanging ),
                     nullptr, this );

    finishDialogSettings();
}


DIALOG_SHEET_PROPERTIES::~DIALOG_SHEET_PROPERTIES()
{
    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_SHEET_PROPERTIES::OnGridCellChanging ),
                        nullptr, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_SHEET_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();
    wxString variantName = m_frame->Schematic().GetCurrentVariant();

    // Push a copy of each field into m_updateFields
    for( SCH_FIELD& field : m_sheet->GetFields() )
    {
        SCH_FIELD field_copy( field );

#ifdef __WINDOWS__
        // Filenames are stored using unix notation, so convert to Windows notation
        if( field_copy.GetId() == FIELD_T::SHEET_FILENAME )
        {
            wxString filename = field_copy.GetText();
            filename.Replace( wxT( "/" ), wxT( "\\" ) );
            field_copy.SetText( filename );
        }
#endif

        if( !field_copy.IsMandatory() )
            field_copy.SetText( m_sheet->GetFieldText( field.GetName(), &instance, variantName ) );

        // change offset to be symbol-relative
        field_copy.Offset( -m_sheet->GetPosition() );

        m_fields->push_back( field_copy );
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_grid->ProcessTableMessage( msg );

    // border width
    m_borderWidth.SetValue( m_sheet->GetBorderWidth() );

    // set up color swatches
    KIGFX::COLOR4D borderColor     = m_sheet->GetBorderColor();
    KIGFX::COLOR4D backgroundColor = m_sheet->GetBackgroundColor();

    m_borderSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_backgroundSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    m_borderSwatch->SetSwatchColor( borderColor, false );
    m_backgroundSwatch->SetSwatchColor( backgroundColor, false );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderSwatch->SetSwatchBackground( canvas );
    m_backgroundSwatch->SetSwatchBackground( canvas );

    m_cbExcludeFromSim->SetValue( m_sheet->GetExcludedFromSim( &instance, variantName ) );
    m_cbExcludeFromBom->SetValue( m_sheet->GetExcludedFromBOM( &instance, variantName ) );
    m_cbExcludeFromBoard->SetValue( m_sheet->GetExcludedFromBoard() );
    m_cbDNP->SetValue( m_sheet->GetDNP( &instance, variantName ) );

    instance.push_back( m_sheet );
    m_pageNumberTextCtrl->ChangeValue( instance.GetPageNumber() );

    return true;
}


bool DIALOG_SHEET_PROPERTIES::Validate()
{
    if( !m_grid->CommitPendingChanges() || !m_grid->Validate() )
        return false;

    // Check for missing field names.
    for( size_t i = 0;  i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );

        if( field.IsMandatory() )
            continue;

        if( field.GetName( false ).empty() && !field.GetText().empty() )
        {
            DisplayErrorMessage( this, _( "Fields must have a name." ) );

            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = (int) i;

            return false;
        }
    }

    return true;
}


static bool positioningChanged( const SCH_FIELD& a, const SCH_FIELD& b )
{
    if( a.GetPosition() != b.GetPosition() )
        return true;

    if( a.GetHorizJustify() != b.GetHorizJustify() )
        return true;

    if( a.GetVertJustify() != b.GetVertJustify() )
        return true;

    if( a.GetTextAngle() != b.GetTextAngle() )
        return true;

    return false;
}


static bool positioningChanged( FIELDS_GRID_TABLE* a, SCH_SHEET* b )
{
    if( positioningChanged( a->GetField( FIELD_T::SHEET_NAME ), b->GetField( FIELD_T::SHEET_NAME ) ) )
        return true;

    if( positioningChanged( a->GetField( FIELD_T::SHEET_FILENAME ), b->GetField( FIELD_T::SHEET_FILENAME ) ) )
        return true;

    return false;
}


bool DIALOG_SHEET_PROPERTIES::TransferDataFromWindow()
{
    wxCHECK( m_sheet && m_frame, false );

    if( !wxDialog::TransferDataFromWindow() )  // Calls our Validate() method.
        return false;

    if( m_isUndoable )
        *m_isUndoable = true;

    // Sheet file names can be relative or absolute.
    wxString sheetFileName = m_fields->GetField( FIELD_T::SHEET_FILENAME )->GetText();

    // Ensure filepath is not empty.  (In normal use will be caught by grid validators,
    // but unedited data from existing files can be bad.)
    if( sheetFileName.IsEmpty() )
    {
        DisplayError( this, _( "A sheet must have a valid file name." ) );
        return false;
    }

    // Ensure the filename extension is OK.  (In normal use will be caught by grid validators,
    // but unedited data from existing files can be bad.)
    sheetFileName = EnsureFileExtension( sheetFileName, FILEEXT::KiCadSchematicFileExtension );

    // Ensure sheetFileName is legal
    if( !IsFullFileNameValid( sheetFileName ) )
    {
        DisplayError( this, _( "A sheet must have a valid file name." ) );
        return false;
    }

    wxFileName fn( sheetFileName );

    wxString newRelativeFilename = fn.GetFullPath();

    // Inside Eeschema, filenames are stored using unix notation
    newRelativeFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString oldFilename = m_sheet->GetField( FIELD_T::SHEET_FILENAME )->GetText();
    oldFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    bool filename_changed = oldFilename != newRelativeFilename;

    if( filename_changed || m_sheet->IsNew() )
    {
        SCH_SCREEN* currentScreen = m_frame->GetCurrentSheet().LastScreen();

        wxCHECK( currentScreen, false );

        bool clearFileName = false;

        // This can happen for the root sheet when opening Eeschema in the stand alone mode.
        if( currentScreen->GetFileName().IsEmpty() )
        {
            clearFileName = true;
            currentScreen->SetFileName( m_frame->Prj().AbsolutePath( wxT( "noname.kicad_sch" ) ) );
        }

        wxFileName tmp( fn );
        wxFileName screenFileName = currentScreen->GetFileName();

        if( fn.IsAbsolute() && fn.MakeRelativeTo( screenFileName.GetPath() ) )
        {
            wxMessageDialog makeRelDlg( this, _( "Use relative path for sheet file?" ), _( "Sheet File Path" ),
                                        wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION | wxCENTER );

            makeRelDlg.SetExtendedMessage( _( "Using relative hierarchical sheet file name paths improves "
                                              "schematic portability across systems and platforms.  Using "
                                              "absolute paths can result in portability issues." ) );
            makeRelDlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Use Relative Path" ) ),
                                       wxMessageDialog::ButtonLabel( _( "Use Absolute Path" ) ) );

            if( makeRelDlg.ShowModal() == wxID_YES )
            {
                wxLogTrace( tracePathsAndFiles, "\n    Converted absolute path: '%s'"
                                                "\n    to relative path: '%s'",
                                                tmp.GetPath(),
                                                fn.GetPath() );
                m_fields->GetField( FIELD_T::SHEET_FILENAME )->SetText( fn.GetFullPath() );
                newRelativeFilename = fn.GetFullPath();
            }
        }

        if( !onSheetFilenameChanged( newRelativeFilename ) )
        {
            if( clearFileName )
                currentScreen->SetFileName( wxEmptyString );
            else
                FindField( *m_fields, FIELD_T::SHEET_FILENAME )->SetText( oldFilename );

            return false;
        }
        else if( m_updateHierarchyNavigator )
        {
            *m_updateHierarchyNavigator = true;
        }

        if( clearFileName )
            currentScreen->SetFileName( wxEmptyString );

        // One last validity check (and potential repair) just to be sure to be sure
        SCH_SHEET_LIST repairedList;
        repairedList.BuildSheetList( &m_frame->Schematic().Root(), true );
    }

    wxString newSheetname = m_fields->GetField( FIELD_T::SHEET_NAME )->GetText();

    if( ( newSheetname != m_sheet->GetName() ) && m_updateHierarchyNavigator )
        *m_updateHierarchyNavigator = true;

    if( newSheetname.IsEmpty() )
        newSheetname = _( "Untitled Sheet" );

    m_fields->GetField( FIELD_T::SHEET_NAME )->SetText( newSheetname );

    m_sheet->SetName( newSheetname );
    m_sheet->SetFileName( newRelativeFilename );

    // change all field positions from relative to absolute
    for( SCH_FIELD& m_field : *m_fields)
        m_field.Offset( m_sheet->GetPosition() );

    if( positioningChanged( m_fields, m_sheet ) )
        m_sheet->SetFieldsAutoplaced( AUTOPLACE_NONE );

    SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();
    wxString variantName = m_frame->Schematic().GetCurrentVariant();

    for( int ii = m_fields->GetNumberRows() - 1; ii >= 0; ii-- )
    {
        SCH_FIELD& field = m_fields->at( ii );

        if( field.IsMandatory() )
            continue;

        const wxString& fieldName = field.GetCanonicalName();

        if( field.IsEmpty() )
            m_fields->erase( m_fields->begin() + ii );
        else if( fieldName.IsEmpty() )
            field.SetName( _( "untitled" ) );

        SCH_FIELD* existingField = m_sheet->GetField( fieldName );
        SCH_FIELD* tmp;

        if( !existingField )
        {
            m_sheet->AddOptionalField( field );
        }
        else
        {
            wxString defaultText = m_sheet->Schematic()->ConvertRefsToKIIDs( existingField->GetText() );
            tmp = const_cast<SCH_FIELD*>( existingField );

            *tmp = field;

            if( !variantName.IsEmpty() )
            {
                // Restore the default field text for existing fields.
                tmp->SetText( defaultText, &instance );

                tmp->SetText( m_sheet->Schematic()->ConvertRefsToKIIDs( field.GetText() ),
                              &instance, variantName );
            }
        }
    }

    m_sheet->SetBorderWidth( m_borderWidth.GetIntValue() );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();

    if( colorSettings->GetOverrideSchItemColors()
            && ( m_sheet->GetBorderColor()     != m_borderSwatch->GetSwatchColor() ||
                 m_sheet->GetBackgroundColor() != m_backgroundSwatch->GetSwatchColor() ) )
    {
        wxPanel temp( this );
        temp.Hide();
        PANEL_EESCHEMA_COLOR_SETTINGS prefs( &temp );
        wxString checkboxLabel = prefs.m_optOverrideColors->GetLabel();

        KIDIALOG dlg( this, _( "Note: item colors are overridden in the current color theme." ),
                      KIDIALOG::KD_WARNING );
        dlg.ShowDetailedText( wxString::Format( _( "To see individual item colors uncheck '%s'\n"
                                                   "in Preferences > Schematic Editor > Colors." ),
                                                checkboxLabel ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );
        dlg.ShowModal();
    }

    m_sheet->SetBorderColor( m_borderSwatch->GetSwatchColor() );
    m_sheet->SetBackgroundColor( m_backgroundSwatch->GetSwatchColor() );

    m_sheet->SetExcludedFromSim( m_cbExcludeFromSim->GetValue(), &instance, variantName );
    m_sheet->SetExcludedFromBOM( m_cbExcludeFromBom->GetValue(), &instance, variantName );
    m_sheet->SetExcludedFromBoard( m_cbExcludeFromBoard->GetValue() );
    m_sheet->SetDNP( m_cbDNP->GetValue(), &instance, variantName );

    instance.push_back( m_sheet );

    instance.SetPageNumber( m_pageNumberTextCtrl->GetValue() );

    m_frame->TestDanglingEnds();

    // Refresh all sheets in case ordering changed.
    for( SCH_ITEM* item : m_frame->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        m_frame->UpdateItem( item );

    return true;
}


bool DIALOG_SHEET_PROPERTIES::onSheetFilenameChanged( const wxString& aNewFilename )
{
    wxString       msg;
    wxFileName     sheetFileName( EnsureFileExtension( aNewFilename, FILEEXT::KiCadSchematicFileExtension ) );

    // Sheet file names are relative to the path of the current sheet.  This allows for
    // nesting of schematic files in subfolders.  Screen file names are always absolute.
    SCHEMATIC&     schematic = m_frame->Schematic();
    SCH_SHEET_LIST fullHierarchy = schematic.Hierarchy();
    wxFileName     screenFileName( sheetFileName );
    wxFileName     tmp( sheetFileName );
    SCH_SCREEN*    currentScreen = m_frame->GetCurrentSheet().LastScreen();

    wxCHECK( currentScreen, false );

    // SCH_SCREEN file names are always absolute.
    wxFileName currentScreenFileName = currentScreen->GetFileName();

    if( !screenFileName.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS, currentScreenFileName.GetPath() ) )
    {
        msg = wxString::Format( _( "Cannot normalize new sheet schematic file path:\n"
                                   "'%s'\n"
                                   "against parent sheet schematic file path:\n"
                                   "'%s'." ),
                                sheetFileName.GetPath(),
                                currentScreenFileName.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    wxString newAbsoluteFilename = screenFileName.GetFullPath();

    // Inside Eeschema, filenames are stored using unix notation
    newAbsoluteFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    bool renameFile = false;
    bool loadFromFile = false;
    bool clearAnnotation = false;
    bool isExistingSheet = false;
    SCH_SCREEN* useScreen = nullptr;
    SCH_SCREEN* oldScreen = nullptr;

    // Search for a schematic file having the same filename already in use in the hierarchy
    // or on disk, in order to reuse it.
    if( !schematic.Root().SearchHierarchy( newAbsoluteFilename, &useScreen ) )
    {
        loadFromFile = wxFileExists( newAbsoluteFilename );

        wxLogTrace( tracePathsAndFiles, "\n    Sheet requested file '%s', %s",
                                        newAbsoluteFilename,
                                        loadFromFile ? "found" : "not found" );
    }

    if( m_sheet->GetScreen() == nullptr )      // New just created sheet.
    {
        if( !m_frame->AllowCaseSensitiveFileNameClashes( m_sheet->GetFileName(), newAbsoluteFilename ) )
            return false;

        if( useScreen || loadFromFile )     // Load from existing file.
        {
            clearAnnotation = true;

            if( !IsOK( this, wxString::Format( _( "'%s' already exists." ), sheetFileName.GetFullName() )
                             + wxT( "\n\n" )
                             + wxString::Format( _( "Link '%s' to this file?" ), newAbsoluteFilename ) ) )
            {
                return false;
            }
        }
        // If we are drawing a sheet from a design block/sheet import, we need to copy the
        // sheet to the current directory.
        else if( m_sourceSheetFilename && !m_sourceSheetFilename->IsEmpty() )
        {
            loadFromFile = true;

            if( !wxCopyFile( *m_sourceSheetFilename, newAbsoluteFilename, false ) )
            {
                msg.Printf( _( "Failed to copy schematic file '%s' to destination '%s'." ),
                            currentScreenFileName.GetFullPath(),
                            newAbsoluteFilename );

                DisplayError( m_frame, msg );

                return false;
            }
        }
        else                                // New file.
        {
            m_frame->InitSheet( m_sheet, newAbsoluteFilename );
        }
    }
    else                                    // Existing sheet.
    {
        isExistingSheet = true;

        if( !m_frame->AllowCaseSensitiveFileNameClashes( m_sheet->GetFileName(), newAbsoluteFilename ) )
            return false;

        // We are always using here a case insensitive comparison to avoid issues
        // under Windows, although under Unix filenames are case sensitive.
        // But many users create schematic under both Unix and Windows
        // **
        // N.B. 1: aSheet->GetFileName() will return a relative path
        //         aSheet->GetScreen()->GetFileName() returns a full path
        //
        // N.B. 2: newFilename uses the unix notation for separator.
        //         so we must use it also to compare the old and new filenames
        wxString oldAbsoluteFilename = m_sheet->GetScreen()->GetFileName();
        oldAbsoluteFilename.Replace( wxT( "\\" ), wxT( "/" ) );

        if( newAbsoluteFilename.Cmp( oldAbsoluteFilename ) != 0 )
        {
            // Sheet file name changes cannot be undone.
            if( m_isUndoable )
                *m_isUndoable = false;

            if( useScreen || loadFromFile )           // Load from existing file.
            {
                clearAnnotation = true;
                oldScreen = m_sheet->GetScreen();

                if( !IsOK( this, wxString::Format( _( "Change '%s' link from '%s' to '%s'?" ),
                                                   newAbsoluteFilename,
                                                   m_sheet->GetFileName(),
                                                   sheetFileName.GetFullName() )
                                 + wxT( "\n\n" )
                                 + _( "This action cannot be undone." ) ) )
                {
                    return false;
                }

                if( loadFromFile )
                    m_sheet->SetScreen( nullptr );
            }
            else                                      // Save to new file name.
            {
                if( m_sheet->GetScreenCount() > 1 )
                {
                    if( !IsOK( this, wxString::Format( _( "Create new file '%s' with contents of '%s'?" ),
                                                       sheetFileName.GetFullName(),
                                                       m_sheet->GetFileName() )
                                     + wxT( "\n\n" )
                                     + _( "This action cannot be undone." ) ) )
                    {
                        return false;
                    }
                }

                renameFile = true;
            }
        }

        if( renameFile )
        {
            IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

            // If the associated screen is shared by more than one sheet, do not
            // change the filename of the corresponding screen here.
            // (a new screen will be created later)
            // if it is not shared, update the filename
            if( m_sheet->GetScreenCount() <= 1 )
                m_sheet->GetScreen()->SetFileName( newAbsoluteFilename );

            try
            {
                pi->SaveSchematicFile( newAbsoluteFilename, m_sheet, &schematic );
            }
            catch( const IO_ERROR& ioe )
            {
                msg = wxString::Format( _( "Error occurred saving schematic file '%s'." ), newAbsoluteFilename );
                DisplayErrorMessage( this, msg, ioe.What() );

                msg = wxString::Format( _( "Failed to save schematic '%s'" ), newAbsoluteFilename );
                m_frame->SetMsgPanel( wxEmptyString, msg );
                return false;
            }

            // If the associated screen is shared by more than one sheet, remove the
            // screen and reload the file to a new screen.  Failure to do this will trash
            // the screen reference counting in complex hierarchies.
            if( m_sheet->GetScreenCount() > 1 )
            {
                oldScreen = m_sheet->GetScreen();
                m_sheet->SetScreen( nullptr );
                loadFromFile = true;
            }
        }
    }

    SCH_SHEET_PATH& currentSheet = m_frame->GetCurrentSheet();

    if( useScreen )
    {
        // Create a temporary sheet for recursion testing to prevent a possible recursion error.
        std::unique_ptr< SCH_SHEET> tmpSheet = std::make_unique<SCH_SHEET>( &schematic );

        if( SCH_FIELD* srcField = m_fields->GetField( FIELD_T::SHEET_NAME ) )
            *tmpSheet->GetField( FIELD_T::SHEET_NAME ) = *srcField;

        tmpSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( sheetFileName.GetFullPath() );
        tmpSheet->SetScreen( useScreen );

        // No need to check for valid library IDs if we are using an existing screen.
        if( m_frame->CheckSheetForRecursion( tmpSheet.get(), &currentSheet ) )
            return false;

        // It's safe to set the sheet screen now.
        m_sheet->SetScreen( useScreen );

        SCH_SHEET_LIST sheetHierarchy( m_sheet );  // The hierarchy of the loaded file.

        sheetHierarchy.AddNewSymbolInstances( currentSheet, m_frame->Prj().GetProjectName() );
        sheetHierarchy.AddNewSheetInstances( currentSheet, fullHierarchy.GetLastVirtualPageNumber() );
    }
    else if( loadFromFile )
    {
        bool restoreSheet = false;

        if( isExistingSheet )
        {
            // Temporarily remove the sheet from the current schematic page so that recursion
            // and symbol library link tests can be performed with the modified sheet settings.
            restoreSheet = true;
            currentSheet.LastScreen()->Remove( m_sheet );
        }

        if( !m_frame->LoadSheetFromFile( m_sheet, &currentSheet, newAbsoluteFilename, false, true )
          || m_frame->CheckSheetForRecursion( m_sheet, &currentSheet ) )
        {
            if( restoreSheet )
            {
                // If we cleared the previous screen, restore it before returning to the user
                if( oldScreen )
                    m_sheet->SetScreen( oldScreen );

                currentSheet.LastScreen()->Append( m_sheet );
            }

            return false;
        }

        if( restoreSheet )
            currentSheet.LastScreen()->Append( m_sheet );
    }

    if( m_clearAnnotationNewItems )
        *m_clearAnnotationNewItems = clearAnnotation;

    // Rebuild the entire connection graph.
    m_frame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );

    return true;
}


void DIALOG_SHEET_PROPERTIES::OnGridCellChanging( wxGridEvent& event )
{
    bool              success = true;
    wxGridCellEditor* editor = m_grid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl*        control = editor->GetControl();

    if( control && control->GetValidator() )
        success = control->GetValidator()->Validate( control );

    if( !success )
    {
        event.Veto();
        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
    }

    editor->DecRef();
}


void DIALOG_SHEET_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                SCH_FIELD newField( m_sheet, FIELD_T::SHEET_USER, GetUserFieldName( m_fields->size(), DO_TRANSLATE ) );

                newField.SetTextAngle( m_fields->GetField( FIELD_T::SHEET_NAME )->GetTextAngle() );
                newField.SetVisible( false );
                m_fields->push_back( newField );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
                m_grid->ProcessTableMessage( msg );
                return { m_fields->size() - 1, FDC_NAME };
            } );
}


void DIALOG_SHEET_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                if( row < m_fields->GetMandatoryRowCount() )
                {
                    DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                          m_fields->GetMandatoryRowCount() ) );
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                m_fields->erase( m_fields->begin() + row );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
                m_grid->ProcessTableMessage( msg );
            } );
}


void DIALOG_SHEET_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    m_grid->OnMoveRowUp(
            [&]( int row )
            {
                return row > m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                std::swap( *( m_fields->begin() + row ), *( m_fields->begin() + row - 1 ) );
                m_grid->ForceRefresh();
            } );
}


void DIALOG_SHEET_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    m_grid->OnMoveRowUp(
            [&]( int row )
            {
                return row >= m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                std::swap( *( m_fields->begin() + row ), *( m_fields->begin() + row + 1 ) );
                m_grid->ForceRefresh();
            } );
}


void DIALOG_SHEET_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            m_grid->SetGridWidthsDirty();
    }

    // Propagate changes in sheetname to displayed hierarchical path
    int       sheetnameRow = m_fields->GetFieldRow( FIELD_T::SHEET_NAME );
    wxString  path = m_frame->GetCurrentSheet().PathHumanReadable( false );

    if( path.Last() != '/' )
        path.Append( '/' );

    wxGridCellEditor* editor = m_grid->GetCellEditor( sheetnameRow, FDC_VALUE );
    wxControl*        control = editor->GetControl();
    wxTextEntry*      textControl = dynamic_cast<wxTextEntry*>( control );
    wxString          sheetName;

    if( textControl )
        sheetName = textControl->GetValue();
    else
        sheetName = m_grid->GetCellValue( sheetnameRow, FDC_VALUE );

    m_dummySheet.SetFields( *m_fields );
    m_dummySheetNameField.SetText( sheetName );
    path += m_dummySheetNameField.GetShownText( false );

    editor->DecRef();

    wxClientDC dc( m_hierarchicalPathLabel );
    int        width = m_sizerBottom->GetSize().x - m_stdDialogButtonSizer->GetSize().x
                                                  - m_hierarchicalPathLabel->GetSize().x
                                                  - 30;

    path = wxControl::Ellipsize( path, dc, wxELLIPSIZE_START, width, wxELLIPSIZE_FLAGS_NONE );

    if( m_hierarchicalPath->GetLabel() != path )
        m_hierarchicalPath->SetLabel( path );

    // Handle a delayed focus
    if( m_delayedFocusRow >= 0 )
    {
        m_grid->SetFocus();
        m_grid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_grid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );

        m_grid->EnableCellEditControl( true );
        m_grid->ShowCellEditControl();

        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }
}
