/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2014-2021 KiCad Developers, see CHANGELOG.txt for contributors.
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

#include <dialog_sheet_properties.h>
#include <kiface_base.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/tooltip.h>
#include <confirm.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tab_traversal.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <bitmaps.h>
#include <eeschema_settings.h>
#include <settings/color_settings.h>
#include <trace_helpers.h>
#include "panel_eeschema_color_settings.h"

DIALOG_SHEET_PROPERTIES::DIALOG_SHEET_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHEET* aSheet,
                                                  bool* aClearAnnotationNewItems ) :
    DIALOG_SHEET_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_clearAnnotationNewItems( aClearAnnotationNewItems ),
    m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
    m_dummySheet( *aSheet ),
    m_dummySheetNameField( wxDefaultPosition, SHEETNAME, &m_dummySheet )
{
    m_sheet = aSheet;
    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_grid, m_sheet );
    m_width = 100;  // Will be later set to a better value
    m_delayedFocusRow = SHEETNAME;
    m_delayedFocusColumn = FDC_VALUE;

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to user's preference
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        m_shownColumns = cfg->m_Appearance.edit_sheet_visible_columns;
        m_grid->ShowHideColumns( m_shownColumns );
    }

    wxToolTip::Enable( true );
    m_stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmap( BITMAPS::small_down ) );

    // Set font sizes
    m_hierarchicalPathLabel->SetFont( KIUI::GetInfoFont( this ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_SHEET_PROPERTIES::OnGridCellChanging ),
                     nullptr, this );

    finishDialogSettings();
}


DIALOG_SHEET_PROPERTIES::~DIALOG_SHEET_PROPERTIES()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
        cfg->m_Appearance.edit_sheet_visible_columns = m_grid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_SHEET_PROPERTIES::OnGridCellChanging ),
                        nullptr, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_SHEET_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Push a copy of each field into m_updateFields
    for( SCH_FIELD& field : m_sheet->GetFields() )
    {
        SCH_FIELD field_copy( field );

#ifdef __WINDOWS__
        // Filenames are stored using unix notation
        if( field_copy.GetId() == SHEETFILENAME )
        {
            wxString filename = field_copy.GetText();
            filename.Replace( wxT( "/" ), wxT( "\\" ) );
            field_copy.SetText( filename );
        }
#endif

        // change offset to be symbol-relative
        field_copy.Offset( -m_sheet->GetPosition() );

        m_fields->push_back( field_copy );
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_grid->ProcessTableMessage( msg );
    AdjustGridColumns( m_grid->GetRect().GetWidth() );

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

    SCH_SHEET_LIST hierarchy = m_frame->Schematic().GetFullHierarchy();
    SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();

    instance.push_back( m_sheet );

    wxString nextPageNumber = m_sheet->GetPageNumber( instance );

    m_pageNumberTextCtrl->ChangeValue( nextPageNumber );

    Layout();

    return true;
}


bool DIALOG_SHEET_PROPERTIES::Validate()
{
    wxString msg;
    LIB_ID   id;

    if( !m_grid->CommitPendingChanges() || !m_grid->Validate() )
        return false;

    // Check for missing field names.
    for( size_t i = SHEET_MANDATORY_FIELDS;  i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Fields must have a name." ) );

            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = i;

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


static bool positioningChanged( FIELDS_GRID_TABLE<SCH_FIELD>* a, std::vector<SCH_FIELD>& b )
{
    for( size_t i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
    {
        if( positioningChanged( a->at( i ), b.at( i ) ) )
            return true;
    }

    return false;
}


bool DIALOG_SHEET_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )  // Calls our Validate() method.
        return false;

    // Sheet file names can be relative or absolute.
    wxString sheetFileName = m_fields->at( SHEETFILENAME ).GetText();

    // Ensure filepath is not empty.  (In normal use will be caught by grid validators,
    // but unedited data from existing files can be bad.)
    if( sheetFileName.IsEmpty() )
    {
        DisplayError( this, _( "A sheet must have a valid file name." ) );
        return false;
    }

    // Ensure the filename extension is OK.  In normal use will be caught by grid validators,
    // but unedited data from existing files can be bad.
    wxFileName fn( sheetFileName );

    if( fn.GetExt().CmpNoCase( KiCadSchematicFileExtension ) != 0 )
    {
        DisplayError( this, _( "Sheet file must have a '.kicad_sch' extension." ) );
        return false;
    }

    wxString newRelativeFilename = fn.GetFullPath();

    // Inside Eeschema, filenames are stored using unix notation
    newRelativeFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString oldFilename = m_sheet->GetFields()[ SHEETFILENAME ].GetText();
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
            wxMessageDialog makeRelDlg( this, _( "Use relative path for sheet file?" ),
                                        _( "Sheet File Path" ),
                                        wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION | wxCENTER );

            makeRelDlg.SetExtendedMessage( _( "Using relative hierarchical sheet file name paths "
                                              "improves schematic portability across systems and "
                                              "platforms.  Using absolute paths can result in "
                                              "portability issues." ) );
            makeRelDlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Use Relative Path" ) ),
                                       wxMessageDialog::ButtonLabel( _( "Use Absolute Path" ) ) );

            if( makeRelDlg.ShowModal() == wxID_YES )
            {
                wxLogTrace( tracePathsAndFiles, "\n    Converted absolute path: '%s'"
                                                "\n    to relative path: '%s'",
                                                tmp.GetPath(),
                                                fn.GetPath() );
                m_fields->at( SHEETFILENAME ).SetText( fn.GetFullPath() );
                newRelativeFilename = fn.GetFullPath();
            }
        }

        if( !onSheetFilenameChanged( newRelativeFilename ) )
        {
            if( clearFileName )
                currentScreen->SetFileName( wxEmptyString );

            return false;
        }

        if( clearFileName )
            currentScreen->SetFileName( wxEmptyString );

        // One last validity check (and potential repair) just to be sure to be sure
        SCH_SHEET_LIST repairedList( &m_frame->Schematic().Root(), true );
    }

    wxString newSheetname = m_fields->at( SHEETNAME ).GetText();

    if( newSheetname.IsEmpty() )
        newSheetname = _( "Untitled Sheet" );

    m_fields->at( SHEETNAME ).SetText( newSheetname );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_sheet->GetPosition() );

    if( positioningChanged( m_fields, m_sheet->GetFields() ) )
        m_sheet->ClearFieldsAutoplaced();

    m_sheet->SetFields( *m_fields );

    m_sheet->SetBorderWidth( m_borderWidth.GetValue() );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();

    if( colorSettings->GetOverrideSchItemColors()
            && ( m_sheet->GetBorderColor()     != m_borderSwatch->GetSwatchColor() ||
                 m_sheet->GetBackgroundColor() != m_backgroundSwatch->GetSwatchColor() ) )
    {
        wxPanel temp( this );
        temp.Hide();
        PANEL_EESCHEMA_COLOR_SETTINGS prefs( m_frame, &temp );
        wxString checkboxLabel = prefs.m_optOverrideColors->GetLabel();

        KIDIALOG dlg( this, _( "Note: item colors are overridden in the current color theme." ),
                      KIDIALOG::KD_WARNING );
        dlg.ShowDetailedText( wxString::Format( _( "To see individual item colors uncheck '%s'\n"
                                                   "in Preferences > Eeschema > Colors." ),
                                                checkboxLabel ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );
        dlg.ShowModal();
    }

    m_sheet->SetBorderColor( m_borderSwatch->GetSwatchColor() );
    m_sheet->SetBackgroundColor( m_backgroundSwatch->GetSwatchColor() );

    SCH_SHEET_LIST hierarchy = m_frame->Schematic().GetFullHierarchy();
    SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();

    instance.push_back( m_sheet );

    if( m_sheet->IsNew() )
        m_sheet->AddInstance( instance.Path() );

    m_sheet->SetPageNumber( instance, m_pageNumberTextCtrl->GetValue() );

    m_frame->TestDanglingEnds();

    // Refresh all sheets in case ordering changed.
    for( SCH_ITEM* item : m_frame->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        m_frame->UpdateItem( item );

    m_frame->OnModify();

    return true;
}


bool DIALOG_SHEET_PROPERTIES::onSheetFilenameChanged( const wxString& aNewFilename )
{
    wxString msg;

    // Sheet file names are relative to the path of the current sheet.  This allows for
    // nesting of schematic files in subfolders.  Screen file names are always absolute.
    wxFileName sheetFileName( aNewFilename );

    if( sheetFileName.GetExt().IsEmpty() )
    {
        sheetFileName.SetExt( KiCadSchematicFileExtension );
    }
    else if( sheetFileName.GetExt().CmpNoCase( KiCadSchematicFileExtension ) != 0 )
    {
        msg = wxString::Format( _( "The file '%s' does not appear to be a valid schematic file." ),
                                sheetFileName.GetFullName() );
        wxMessageDialog badSchFileDialog( this, msg, _( "Invalid Schematic File" ),
                                          wxOK | wxCENTRE | wxICON_EXCLAMATION );
        badSchFileDialog.ShowModal();
        return false;
    }

    wxFileName screenFileName( sheetFileName );
    wxFileName tmp( sheetFileName );

    SCH_SCREEN* currentScreen = m_frame->GetCurrentSheet().LastScreen();

    wxCHECK( currentScreen, false );

    // SCH_SCREEN file names are always absolute.
    wxFileName currentScreenFileName = currentScreen->GetFileName();

    if( !screenFileName.Normalize( wxPATH_NORM_ALL, currentScreenFileName.GetPath() ) )
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
    bool restoreSheet = false;
    bool isExistingSheet = false;
    SCH_SCREEN* useScreen = nullptr;

    // Search for a schematic file having the same filename already in use in the hierarchy
    // or on disk, in order to reuse it.
    if( !m_frame->Schematic().Root().SearchHierarchy( newAbsoluteFilename, &useScreen ) )
    {
        loadFromFile = wxFileExists( newAbsoluteFilename );

        wxLogTrace( tracePathsAndFiles, "\n    Sheet requested file '%s', %s",
                                        newAbsoluteFilename,
                                        loadFromFile ? "found" : "not found" );
    }

    if( m_sheet->GetScreen() == nullptr )      // New just created sheet.
    {
        if( !m_frame->AllowCaseSensitiveFileNameClashes( newAbsoluteFilename ) )
            return false;

        if( useScreen || loadFromFile )     // Load from existing file.
        {
            clearAnnotation = true;

            if( !IsOK( this, wxString::Format( _( "'%s' already exists." ),
                                               sheetFileName.GetFullName() )
                             + wxT( "\n\n" )
                             + wxString::Format( _( "Link '%s' to this file?" ),
                                                 newAbsoluteFilename ) ) )
            {
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
        bool isUndoable = true;
        isExistingSheet = true;

        if( !m_frame->AllowCaseSensitiveFileNameClashes( newAbsoluteFilename ) )
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
            isUndoable = false;

            if( useScreen || loadFromFile )           // Load from existing file.
            {
                clearAnnotation = true;

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

        if( isUndoable )
            m_frame->SaveCopyInUndoList( m_frame->GetScreen(), m_sheet, UNDO_REDO::CHANGED, false );

        if( renameFile )
        {
            SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

            // If the associated screen is shared by more than one sheet, do not
            // change the filename of the corresponding screen here.
            // (a new screen will be created later)
            // if it is not shared, update the filename
            if( m_sheet->GetScreenCount() <= 1 )
                m_sheet->GetScreen()->SetFileName( newAbsoluteFilename );

            try
            {
                pi->Save( newAbsoluteFilename, m_sheet, &m_frame->Schematic() );
            }
            catch( const IO_ERROR& ioe )
            {
                msg = wxString::Format( _( "Error occurred saving schematic file '%s'." ),
                                        newAbsoluteFilename );
                DisplayErrorMessage( this, msg, ioe.What() );

                msg = wxString::Format( _( "Failed to save schematic '%s'" ),
                                        newAbsoluteFilename );
                m_frame->SetMsgPanel( wxEmptyString, msg );
                return false;
            }

            // If the associated screen is shared by more than one sheet, remove the
            // screen and reload the file to a new screen.  Failure to do this will trash
            // the screen reference counting in complex hierarchies.
            if( m_sheet->GetScreenCount() > 1 )
            {
                m_sheet->SetScreen( nullptr );
                loadFromFile = true;
            }
        }
    }

    SCH_SHEET_PATH& currentSheet = m_frame->GetCurrentSheet();

    if( useScreen )
    {
        // Create a temporary sheet for recursion testing to prevent a possible recursion error.
        std::unique_ptr< SCH_SHEET> tmpSheet = std::make_unique<SCH_SHEET>();
        tmpSheet->GetFields()[SHEETNAME] = m_fields->at( SHEETNAME );
        tmpSheet->GetFields()[SHEETFILENAME].SetText( sheetFileName.GetFullPath() );
        tmpSheet->SetScreen( useScreen );

        // No need to check for valid library IDs if we are using an existing screen.
        if( m_frame->CheckSheetForRecursion( tmpSheet.get(), &currentSheet ) )
        {
            if( restoreSheet )
                currentSheet.LastScreen()->Append( m_sheet );

            return false;
        }

        // It's safe to set the sheet screen now.
        m_sheet->SetScreen( useScreen );
    }
    else if( loadFromFile )
    {
        if( isExistingSheet )
        {
            // Temporarily remove the sheet from the current schematic page so that recursion
            // and symbol library link tests can be performed with the modified sheet settings.
            restoreSheet = true;
            currentSheet.LastScreen()->Remove( m_sheet );
        }

        if( !m_frame->LoadSheetFromFile( m_sheet, &currentSheet, newAbsoluteFilename )
          || m_frame->CheckSheetForRecursion( m_sheet, &currentSheet ) )
        {
            if( restoreSheet )
                currentSheet.LastScreen()->Append( m_sheet );

            return false;
        }

        if( restoreSheet )
            currentSheet.LastScreen()->Append( m_sheet );
    }

    if( m_clearAnnotationNewItems )
        *m_clearAnnotationNewItems = clearAnnotation;

    return true;
}


void DIALOG_SHEET_PROPERTIES::OnGridCellChanging( wxGridEvent& event )
{
    bool              success = true;
    wxGridCellEditor* editor = m_grid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl*        control = editor->GetControl();
    wxTextEntry*      textControl = dynamic_cast<wxTextEntry*>( control );

    // Short-circuit the validator's more generic "can't be empty" message for the
    // two mandatory fields:
    if( event.GetRow() == SHEETNAME && event.GetCol() == FDC_VALUE )
    {
        if( textControl && textControl->IsEmpty() )
        {
            wxMessageBox( _( "A sheet must have a name." ) );
            success = false;
        }
    }
    else if( event.GetRow() == SHEETFILENAME && event.GetCol() == FDC_VALUE && textControl )
    {
        if( textControl->IsEmpty() )
        {
            wxMessageBox( _( "A sheet must have a file specified." ) );
            success = false;
        }
    }

    if( success && control && control->GetValidator() )
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
    if( !m_grid->CommitPendingChanges() )
        return;

    int       fieldID = m_fields->size();
    SCH_FIELD newField( wxPoint( 0, 0 ), fieldID, m_sheet,
                        SCH_SHEET::GetDefaultFieldName( fieldID ) );

    newField.SetTextAngle( m_fields->at( SHEETNAME ).GetTextAngle() );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();
}


void DIALOG_SHEET_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
        return;
    else if( curRow < SHEET_MANDATORY_FIELDS )
    {
        DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                              SHEET_MANDATORY_FIELDS ) );
        return;
    }

    m_grid->CommitPendingChanges( true /* quiet mode */ );

    m_fields->erase( m_fields->begin() + curRow );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_grid->ProcessTableMessage( msg );

    if( m_grid->GetNumberRows() > 0 )
    {
        m_grid->MakeCellVisible( std::max( 0, curRow-1 ), m_grid->GetGridCursorCol() );
        m_grid->SetGridCursor( std::max( 0, curRow-1 ), m_grid->GetGridCursorCol() );
    }
}


void DIALOG_SHEET_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i > SHEET_MANDATORY_FIELDS )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i - 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
        wxBell();
}


void DIALOG_SHEET_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i >= SHEET_MANDATORY_FIELDS && i < m_grid->GetNumberRows() - 1 )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i + 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
        wxBell();
}


void DIALOG_SHEET_PROPERTIES::AdjustGridColumns( int aWidth )
{
    m_width = aWidth;
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->AutoSizeColumn( 0 );

    int fixedColsWidth = m_grid->GetColSize( 0 );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( 1, aWidth - fixedColsWidth );
}


void DIALOG_SHEET_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            AdjustGridColumns( m_grid->GetRect().GetWidth() );
    }

    // Propagate changes in sheetname to displayed hierarchical path
    wxString hierarchicalPath = _( "Hierarchical path: " );

    hierarchicalPath += m_frame->GetCurrentSheet().PathHumanReadable( false );

    if( hierarchicalPath.Last() != '/' )
        hierarchicalPath.Append( '/' );

    wxGridCellEditor* editor = m_grid->GetCellEditor( SHEETNAME, FDC_VALUE );
    wxControl*        control = editor->GetControl();
    wxTextEntry*      textControl = dynamic_cast<wxTextEntry*>( control );
    wxString          sheetName;

    if( textControl )
        sheetName = textControl->GetValue();
    else
        sheetName = m_grid->GetCellValue( SHEETNAME, FDC_VALUE );

    m_dummySheet.SetFields( *m_fields );
    m_dummySheetNameField.SetText( sheetName );
    hierarchicalPath += m_dummySheetNameField.GetShownText();

    editor->DecRef();

    if( m_hierarchicalPathLabel->GetLabel() != hierarchicalPath )
        m_hierarchicalPathLabel->SetLabel( hierarchicalPath );

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


void DIALOG_SHEET_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_width != new_size )
    {
        AdjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SHEET_PROPERTIES::OnInitDlg( wxInitDialogEvent& event )
{
    TransferDataToWindow();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}
