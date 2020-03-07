/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2014-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <dialog_sch_sheet_props.h>
#include <kiface_i.h>
#include <wx/string.h>
#include <wx/tooltip.h>
#include <confirm.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tab_traversal.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <bitmaps.h>
#include <eeschema_settings.h>


DIALOG_SCH_SHEET_PROPS::DIALOG_SCH_SHEET_PROPS( SCH_EDIT_FRAME* aParent, SCH_SHEET* aSheet,
                                                bool* aClearAnnotationNewItems ) :
    DIALOG_SCH_SHEET_PROPS_BASE( aParent ),
    m_frame( aParent ),
    m_clearAnnotationNewItems( aClearAnnotationNewItems )
{
    m_sheet = aSheet;
    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_sheet );

    m_width = 0;
    m_delayedFocusRow = SHEETNAME;
    m_delayedFocusColumn = FDC_VALUE;

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to user's preference
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    m_shownColumns = cfg->m_Appearance.edit_sheet_visible_columns;
    m_grid->ShowHideColumns( m_shownColumns );

    wxToolTip::Enable( true );
    m_stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_SCH_SHEET_PROPS::OnGridCellChanging ),
                     NULL, this );

    FinishDialogSettings();
}


DIALOG_SCH_SHEET_PROPS::~DIALOG_SCH_SHEET_PROPS()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    cfg->m_Appearance.edit_sheet_visible_columns = m_grid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_SCH_SHEET_PROPS::OnGridCellChanging ),
                        NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_SCH_SHEET_PROPS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Push a copy of each field into m_fields
    for( SCH_FIELD& field : m_sheet->GetFields() )
    {
        SCH_FIELD field_copy( field );

#ifdef __WINDOWS__
        // Filenames are stored using unix notation
        if( field_copy.GetId() == SHEETFILENAME )
        {
            wxString filename = field_copy.GetText();
            filename.Replace( wxT("/"), wxT("\\") );
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

    m_heirarchyPath->SetValue( g_CurrentSheet->PathHumanReadable() );

    // Set the component's unique ID time stamp.
    m_textCtrlTimeStamp->SetValue( m_sheet->m_Uuid.AsString() );

    Layout();

    return true;
}


bool DIALOG_SCH_SHEET_PROPS::Validate()
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


bool DIALOG_SCH_SHEET_PROPS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )  // Calls our Validate() method.
        return false;

    wxString newRelativeNativeFilename = m_fields->at( SHEETFILENAME ).GetText();
    wxString newRelativeFilename = newRelativeNativeFilename;

    // Inside Eeschema, filenames are stored using unix notation
    newRelativeFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString newSheetname = m_fields->at( SHEETNAME ).GetText();

    if( newSheetname.IsEmpty() )
        newSheetname = _( "Untitled Sheet" );

    // Relative file names are relative to the path of the current sheet.  This allows for
    // nesting of schematic files in subfolders.
    wxFileName fileName( newRelativeNativeFilename );
    fileName.SetExt( SchematicFileExtension );

    if( !fileName.IsAbsolute() )
    {
        const SCH_SCREEN* currentScreen = g_CurrentSheet->LastScreen();

        wxCHECK_MSG( currentScreen, false, "Invalid sheet path object." );

        wxFileName currentSheetFileName = currentScreen->GetFileName();

        wxCHECK_MSG( fileName.Normalize( wxPATH_NORM_ALL, currentSheetFileName.GetPath() ), false,
                     "Cannot normalize new sheet schematic file path." );
    }

    wxString newAbsoluteFilename = fileName.GetFullPath();

    // Inside Eeschema, filenames are stored using unix notation
    newAbsoluteFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString msg;
    bool renameFile = false;
    bool loadFromFile = false;
    bool clearAnnotation = false;
    bool restoreSheet = false;
    bool isExistingSheet = false;
    SCH_SCREEN* useScreen = NULL;

    // Search for a schematic file having the same filename already in use in the hierarchy
    // or on disk, in order to reuse it.
    if( !g_RootSheet->SearchHierarchy( newAbsoluteFilename, &useScreen ) )
    {
        loadFromFile = wxFileExists( newAbsoluteFilename );
        wxLogDebug( "Sheet requested file \"%s\", %s",
                    newAbsoluteFilename,
                    ( loadFromFile ) ? "found" : "not found" );
    }

    if( m_sheet->GetScreen() == NULL )                 // New sheet.
    {
        if( !m_frame->AllowCaseSensitiveFileNameClashes( newAbsoluteFilename ) )
            return false;

        if( useScreen || loadFromFile )               // Load from existing file.
        {
            clearAnnotation = true;

            wxString existsMsg;
            wxString linkMsg;
            existsMsg.Printf( _( "\"%s\" already exists." ), fileName.GetFullName() );
            linkMsg.Printf( _( "Link \"%s\" to this file?" ), newSheetname );
            msg.Printf( wxT( "%s\n\n%s" ), existsMsg, linkMsg );

            if( !IsOK( this, msg ) )
                return false;

        }
        else                                          // New file.
        {
            m_frame->InitSheet( m_sheet, newAbsoluteFilename );
        }
    }
    else                                              // Existing sheet.
    {
        bool isUndoable = true;
        wxString replaceMsg;
        wxString newMsg;
        wxString noUndoMsg;

        isExistingSheet = true;

        if( !m_frame->AllowCaseSensitiveFileNameClashes( newAbsoluteFilename ) )
            return false;

        // Changing the filename of a sheet can modify the full hierarchy structure
        // and can be not always undoable.
        // So prepare messages for user notifications:
        replaceMsg.Printf( _( "Change \"%s\" link from \"%s\" to \"%s\"?" ),
                           newSheetname,
                           m_sheet->GetFileName(),
                           fileName.GetFullName() );
        newMsg.Printf( _( "Create new file \"%s\" with contents of \"%s\"?" ),
                       fileName.GetFullName(),
                       m_sheet->GetFileName() );
        noUndoMsg = _( "This action cannot be undone." );

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

                msg.Printf( wxT( "%s\n\n%s" ), replaceMsg, noUndoMsg );

                if( !IsOK( this, msg ) )
                    return false;

                if( loadFromFile )
                    m_sheet->SetScreen( NULL );
            }
            else                                      // Save to new file name.
            {
                if( m_sheet->GetScreenCount() > 1 )
                {
                    msg.Printf( wxT( "%s\n\n%s" ), newMsg, noUndoMsg );

                    if( !IsOK( this, msg ) )
                        return false;
                }

                renameFile = true;
            }
        }

        if( isUndoable )
            m_frame->SaveCopyInUndoList( m_sheet, UR_CHANGED );

        if( renameFile )
        {
            SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

            // If the the associated screen is shared by more than one sheet, do not
            // change the filename of the corresponding screen here.
            // (a new screen will be created later)
            // if it is not shared, update the filename
            if( m_sheet->GetScreenCount() <= 1 )
                m_sheet->GetScreen()->SetFileName( newAbsoluteFilename );

            try
            {
                pi->Save( newAbsoluteFilename, m_sheet->GetScreen(), &Kiway() );
            }
            catch( const IO_ERROR& ioe )
            {
                msg.Printf( _( "Error occurred saving schematic file \"%s\"." ), newAbsoluteFilename );
                DisplayErrorMessage( this, msg, ioe.What() );

                msg.Printf( _( "Failed to save schematic \"%s\"" ), newAbsoluteFilename );
                m_frame->AppendMsgPanel( wxEmptyString, msg, CYAN );

                return false;
            }

            // If the the associated screen is shared by more than one sheet, remove the
            // screen and reload the file to a new screen.  Failure to do this will trash
            // the screen reference counting in complex hierarchies.
            if( m_sheet->GetScreenCount() > 1 )
            {
                m_sheet->SetScreen( NULL );
                loadFromFile = true;
            }
        }
    }

    wxFileName nativeFileName( newRelativeNativeFilename );
    nativeFileName.SetExt( SchematicFileExtension );

    if( useScreen )
    {
        // Create a temporary sheet for recursion testing to prevent a possible recursion error.
        std::unique_ptr< SCH_SHEET> tmpSheet( new SCH_SHEET );
        tmpSheet->SetName( m_fields->at( SHEETNAME ).GetText() );
        tmpSheet->SetFileName( nativeFileName.GetFullPath() );
        tmpSheet->SetScreen( useScreen );

        // No need to check for valid library IDs if we are using an existing screen.
        if( m_frame->CheckSheetForRecursion( tmpSheet.get(), g_CurrentSheet ) )
        {
            if( restoreSheet )
                g_CurrentSheet->LastScreen()->Append( m_sheet );

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
            g_CurrentSheet->LastScreen()->Remove( m_sheet );
        }

        if( !m_frame->LoadSheetFromFile( m_sheet, g_CurrentSheet, newAbsoluteFilename )
                || m_frame->CheckSheetForRecursion( m_sheet, g_CurrentSheet ) )
        {
            if( restoreSheet )
                g_CurrentSheet->LastScreen()->Append( m_sheet );

            return false;
        }

        if( restoreSheet )
            g_CurrentSheet->LastScreen()->Append( m_sheet );
    }

    m_fields->at( SHEETFILENAME ).SetText( newRelativeFilename );
    m_fields->at( SHEETNAME ).SetText( newSheetname );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_sheet->GetPosition() );

    if( positioningChanged( m_fields, m_sheet->GetFields() ) )
        m_sheet->ClearFieldsAutoplaced();

    m_sheet->SetFields( *m_fields );

    if( m_clearAnnotationNewItems )
        *m_clearAnnotationNewItems = clearAnnotation;

    m_frame->TestDanglingEnds();
    m_frame->RefreshItem( m_sheet );
    m_frame->OnModify();

    return true;
}


void DIALOG_SCH_SHEET_PROPS::OnGridCellChanging( wxGridEvent& event )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl* control = editor->GetControl();

    if( control && control->GetValidator() && !control->GetValidator()->Validate( control ) )
    {
        event.Veto();
        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
    }

    editor->DecRef();
}


void DIALOG_SCH_SHEET_PROPS::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int       fieldID = m_fields->size();
    SCH_FIELD newField( wxPoint( 0, 0 ), fieldID, m_sheet );

    newField.SetName( SCH_SHEET::GetDefaultFieldName( fieldID ) );
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


void DIALOG_SCH_SHEET_PROPS::OnDeleteField( wxCommandEvent& event )
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


void DIALOG_SCH_SHEET_PROPS::OnMoveUp( wxCommandEvent& event )
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


void DIALOG_SCH_SHEET_PROPS::OnMoveDown( wxCommandEvent& event )
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


void DIALOG_SCH_SHEET_PROPS::AdjustGridColumns( int aWidth )
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


void DIALOG_SCH_SHEET_PROPS::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            AdjustGridColumns( m_grid->GetRect().GetWidth() );
    }

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


void DIALOG_SCH_SHEET_PROPS::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_width != new_size )
    {
        AdjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SCH_SHEET_PROPS::OnInitDlg( wxInitDialogEvent& event )
{
    TransferDataToWindow();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}
