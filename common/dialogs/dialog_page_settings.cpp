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

#include <env_paths.h>
#include <pgm_base.h>
#include <bitmaps.h>
#include <base_screen.h>
#include <common.h>     // ExpandEnvVarSubstitutions
#include <core/arraydim.h>
#include <dialogs/dialog_page_settings.h>
#include <eda_draw_frame.h>
#include <eda_item.h>
#include <embedded_files.h>
#include <filename_resolver.h>
#include <gr_basic.h>
#include <kiface_base.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <project.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_painter.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/filedlg_hook_embed_file.h>
#include <wx/valgen.h>
#include <wx/tokenzr.h>
#include <wx/filedlg.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <confirm.h>

#define MAX_PAGE_EXAMPLE_SIZE 200


DIALOG_PAGES_SETTINGS::DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* aParent, EMBEDDED_FILES* aEmbeddedFiles,
                                              double aIuPerMils, const VECTOR2D& aMaxUserSizeMils ) :
        DIALOG_PAGES_SETTINGS_BASE( aParent ),
        m_parent( aParent ),
        m_screen( m_parent->GetScreen() ),
        m_initialized( false ),
        m_pageBitmap( nullptr ),
        m_iuPerMils( aIuPerMils ),
        m_embeddedFiles( aEmbeddedFiles ),
        m_customSizeX( aParent, m_userSizeXLabel, m_userSizeXCtrl, m_userSizeXUnits ),
        m_customSizeY( aParent, m_userSizeYLabel, m_userSizeYCtrl, m_userSizeYUnits )
{
    m_projectPath = Prj().GetProjectPath();
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_maxPageSizeMils = aMaxUserSizeMils;
    m_tb = m_parent->GetTitleBlock();
    m_customFmt = false;
    m_localPrjConfigChanged = false;

    m_drawingSheet = new DS_DATA_MODEL;
    wxString serialization;
    DS_DATA_MODEL::GetTheInstance().SaveInString( &serialization );
    m_drawingSheet->SetPageLayout( TO_UTF8( serialization ) );

    m_PickDate->SetValue( wxDateTime::Now() );

    if( m_parent->GetName() == PL_EDITOR_FRAME_NAME )
    {
        SetTitle( _( "Preview Settings" ) );
        m_staticTextPaper->SetLabel( _( "Preview Paper" ) );
        m_staticTextTitleBlock->SetLabel( _( "Preview Title Block Data" ) );
    }
    else
    {
        SetTitle( _( "Page Settings" ) );
        m_staticTextPaper->SetLabel( _( "Paper" ) );
        m_staticTextTitleBlock->SetLabel( _( "Title Block" ) );
    }

    m_filenameResolver = new FILENAME_RESOLVER;
    m_filenameResolver->SetProject( &Prj() );
    m_filenameResolver->SetProgramBase( &Pgm() );

    SetupStandardButtons();

    Centre();
}


DIALOG_PAGES_SETTINGS::~DIALOG_PAGES_SETTINGS()
{
    delete m_pageBitmap;
    delete m_drawingSheet;
}


bool DIALOG_PAGES_SETTINGS::TransferDataToWindow()
{
    // initialize page format choice box and page format list.
    // The first shows translated strings, the second contains not translated strings
    m_paperSizeComboBox->Clear();

    int selectedIdx = -1;
    m_pageInfo = m_parent->GetPageSettings();

    for( const PAGE_INFO& pageFmt : PAGE_INFO::GetPageFormatsList() )
    {
        int idx = m_paperSizeComboBox->Append( wxGetTranslation( pageFmt.GetPageFormatDescription() ),
                                               reinterpret_cast<void*>( static_cast<intptr_t>( pageFmt.GetType() ) ) );

        if( pageFmt.GetType() == m_pageInfo.GetType() )
        {
            selectedIdx = idx;
        }
    }

    m_paperSizeComboBox->SetSelection( selectedIdx );

    // initialize the drawing sheet filename
    SetWksFileName( BASE_SCREEN::m_DrawingSheetFileName );

    m_orientationComboBox->SetSelection( m_pageInfo.IsPortrait() );

    // only a click fires the "selection changed" event, so have to fabricate this check
    wxCommandEvent dummy;
    OnPaperSizeChoice( dummy );

    if( m_customFmt )
    {
        m_customSizeX.SetDoubleValue( m_pageInfo.GetWidthMils() * m_iuPerMils );
        m_customSizeY.SetDoubleValue( m_pageInfo.GetHeightMils() * m_iuPerMils );
    }
    else
    {
        m_customSizeX.SetDoubleValue( m_pageInfo.GetCustomWidthMils() * m_iuPerMils );
        m_customSizeY.SetDoubleValue( m_pageInfo.GetCustomHeightMils() * m_iuPerMils );
    }

    m_TextRevision->SetValue( m_tb.GetRevision() );
    m_TextDate->SetValue( m_tb.GetDate() );
    m_TextTitle->SetValue( m_tb.GetTitle() );
    m_TextCompany->SetValue( m_tb.GetCompany() );
    m_TextComment1->SetValue( m_tb.GetComment( 0 ) );
    m_TextComment2->SetValue( m_tb.GetComment( 1 ) );
    m_TextComment3->SetValue( m_tb.GetComment( 2 ) );
    m_TextComment4->SetValue( m_tb.GetComment( 3 ) );
    m_TextComment5->SetValue( m_tb.GetComment( 4 ) );
    m_TextComment6->SetValue( m_tb.GetComment( 5 ) );
    m_TextComment7->SetValue( m_tb.GetComment( 6 ) );
    m_TextComment8->SetValue( m_tb.GetComment( 7 ) );
    m_TextComment9->SetValue( m_tb.GetComment( 8 ) );

    // The default is to disable aall these fields for the "generic" dialog
    m_TextSheetCount->Show( false );
    m_TextSheetNumber->Show( false );
    m_PaperExport->Show( false );
    m_RevisionExport->Show( false );
    m_DateExport->Show( false );
    m_TitleExport->Show( false );
    m_CompanyExport->Show( false );
    m_Comment1Export->Show( false );
    m_Comment2Export->Show( false );
    m_Comment3Export->Show( false );
    m_Comment4Export->Show( false );
    m_Comment5Export->Show( false );
    m_Comment6Export->Show( false );
    m_Comment7Export->Show( false );
    m_Comment8Export->Show( false );
    m_Comment9Export->Show( false );

    onTransferDataToWindow();

    GetPageLayoutInfoFromDialog();
    UpdateDrawingSheetExample();

    GetSizer()->SetSizeHints( this );

    m_initialized = true;

    return true;
}


bool DIALOG_PAGES_SETTINGS::TransferDataFromWindow()
{
    int idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    void*          clientData = m_paperSizeComboBox->GetClientData( idx );
    PAGE_SIZE_TYPE pageType = static_cast<PAGE_SIZE_TYPE>( reinterpret_cast<intptr_t>( clientData ) );

    if( pageType == PAGE_SIZE_TYPE::User )
    {
        if( !m_customSizeX.Validate( MIN_PAGE_SIZE_MILS, m_maxPageSizeMils.x, EDA_UNITS::MILS ) )
            return false;

        if( !m_customSizeY.Validate( MIN_PAGE_SIZE_MILS, m_maxPageSizeMils.y, EDA_UNITS::MILS ) )
            return false;
    }

    if( SavePageSettings() )
    {
        m_screen->SetContentModified();

        if( LocalPrjConfigChanged() )
            m_parent->OnModify();

        // Call the post processing (if any) after changes
        m_parent->OnPageSettingsChange();
    }

    return true;
}


void DIALOG_PAGES_SETTINGS::OnPaperSizeChoice( wxCommandEvent& event )
{
    int idx = m_paperSizeComboBox->GetSelection();

    if( idx < 0 )
        idx = 0;

    void* clientData = m_paperSizeComboBox->GetClientData( idx );
    PAGE_SIZE_TYPE pageType = static_cast<PAGE_SIZE_TYPE>( reinterpret_cast<intptr_t>( clientData ) );

    if( pageType == PAGE_SIZE_TYPE::User )
    {
        m_staticTextOrient->Enable( false );
        m_orientationComboBox->Enable( false );

        m_staticTextCustSize->Enable( true );
        m_customSizeX.Enable( true );
        m_customSizeY.Enable( true );
        m_customFmt = true;
    }
    else
    {
        m_staticTextOrient->Enable( true );
        m_orientationComboBox->Enable( true );

        m_staticTextCustSize->Enable( false );
        m_customSizeX.Enable( false );
        m_customSizeY.Enable( false );
        m_customFmt = false;
    }

    GetPageLayoutInfoFromDialog();
    UpdateDrawingSheetExample();
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeXTextUpdated( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeYTextUpdated( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnPageOrientationChoice( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnRevisionTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextRevision->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetRevision( m_TextRevision->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnDateTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextDate->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetDate( m_TextDate->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnTitleTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextTitle->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetTitle( m_TextTitle->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnCompanyTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextCompany->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetCompany( m_TextCompany->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment1TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment1->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 0, m_TextComment1->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment2TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment2->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 1, m_TextComment2->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment3TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment3->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 2, m_TextComment3->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment4TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment4->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 3, m_TextComment4->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment5TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment5->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 4, m_TextComment5->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment6TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment6->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 5, m_TextComment6->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment7TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment7->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 6, m_TextComment7->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment8TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment8->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 7, m_TextComment8->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment9TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment9->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 8, m_TextComment9->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnDateApplyClick( wxCommandEvent& event )
{
    wxDateTime datetime = m_PickDate->GetValue();
    wxString date =
    // We can choose different formats. Should probably be kept in sync with CURRENT_DATE
    // formatting in TITLE_BLOCK.
    //
    //  datetime.Format( wxLocale::GetInfo( wxLOCALE_SHORT_DATE_FMT ) );
    //  datetime.Format( wxLocale::GetInfo( wxLOCALE_LONG_DATE_FMT ) );
    //  datetime.Format( wxT("%Y-%b-%d") );
        datetime.FormatISODate();

    m_TextDate->SetValue( date );
}


bool DIALOG_PAGES_SETTINGS::SavePageSettings()
{
    bool     success = false;
    wxString msg;
    wxString fileName = GetWksFileName();

    wxString fullFileName = m_filenameResolver->ResolvePath( fileName, m_projectPath, { m_embeddedFiles } );

    BASE_SCREEN::m_DrawingSheetFileName = fileName;

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( fullFileName, &msg ) )
        DisplayErrorMessage( this, wxString::Format( _( "Error loading drawing sheet '%s'." ), fullFileName ), msg );

    m_localPrjConfigChanged = true;

    int            idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    void*          clientData = m_paperSizeComboBox->GetClientData( idx );
    PAGE_SIZE_TYPE pageType = static_cast<PAGE_SIZE_TYPE>( reinterpret_cast<intptr_t>( clientData ) );

    if( pageType == PAGE_SIZE_TYPE::User )
    {
        GetCustomSizeMilsFromDialog();

        success = m_pageInfo.SetType( PAGE_SIZE_TYPE::User );

        if( success )
        {
            PAGE_INFO::SetCustomWidthMils( m_layout_size.x );
            PAGE_INFO::SetCustomHeightMils( m_layout_size.y );

            m_pageInfo.SetWidthMils( m_layout_size.x );
            m_pageInfo.SetHeightMils( m_layout_size.y );
        }
    }
    else
    {
        success = m_pageInfo.SetType( pageType );

        if( success )
        {
            int choice = m_orientationComboBox->GetSelection();
            m_pageInfo.SetPortrait( choice != 0 );
        }
    }

    if( !success )
    {
        wxFAIL_MSG( "The translation for paper size must preserve original spellings" );
        m_pageInfo.SetType( PAGE_SIZE_TYPE::A4 );
    }

    m_parent->SetPageSettings( m_pageInfo );

    m_tb.SetRevision( m_TextRevision->GetValue() );
    m_tb.SetDate(     m_TextDate->GetValue() );
    m_tb.SetCompany(  m_TextCompany->GetValue() );
    m_tb.SetTitle(    m_TextTitle->GetValue() );
    m_tb.SetComment( 0, m_TextComment1->GetValue() );
    m_tb.SetComment( 1, m_TextComment2->GetValue() );
    m_tb.SetComment( 2, m_TextComment3->GetValue() );
    m_tb.SetComment( 3, m_TextComment4->GetValue() );
    m_tb.SetComment( 4, m_TextComment5->GetValue() );
    m_tb.SetComment( 5, m_TextComment6->GetValue() );
    m_tb.SetComment( 6, m_TextComment7->GetValue() );
    m_tb.SetComment( 7, m_TextComment8->GetValue() );
    m_tb.SetComment( 8, m_TextComment9->GetValue() );

    m_parent->SetTitleBlock( m_tb );

    return onSavePageSettings();
}


void DIALOG_PAGES_SETTINGS::UpdateDrawingSheetExample()
{
    int lyWidth, lyHeight;

    VECTOR2D clamped_layout_size( std::clamp( m_layout_size.x, (double) MIN_PAGE_SIZE_MILS,
                                              m_maxPageSizeMils.x ),
                                  std::clamp( m_layout_size.y, (double) MIN_PAGE_SIZE_MILS,
                                              m_maxPageSizeMils.y ) );

    double lyRatio = clamped_layout_size.x < clamped_layout_size.y ?
                        (double) clamped_layout_size.y / clamped_layout_size.x :
                        (double) clamped_layout_size.x / clamped_layout_size.y;

    if( clamped_layout_size.x < clamped_layout_size.y )
    {
        lyHeight = MAX_PAGE_EXAMPLE_SIZE;
        lyWidth = KiROUND( (double) lyHeight / lyRatio );
    }
    else
    {
        lyWidth = MAX_PAGE_EXAMPLE_SIZE;
        lyHeight = KiROUND( (double) lyWidth / lyRatio );
    }

    if( m_pageBitmap )
    {
        m_PageLayoutExampleBitmap->SetBitmap( wxNullBitmap );
        delete m_pageBitmap;
    }

    m_pageBitmap = new wxBitmap( lyWidth + 1, lyHeight + 1 );

    if( m_pageBitmap->IsOk() )
    {
        double scaleW = (double) lyWidth  / clamped_layout_size.x;
        double scaleH = (double) lyHeight / clamped_layout_size.y;
        double scale = std::min( scaleW, scaleH );

        // Prepare DC.
        wxSize example_size( lyWidth + 1, lyHeight + 1 );
        wxMemoryDC memDC;
        memDC.SelectObject( *m_pageBitmap );
        memDC.SetClippingRegion( wxPoint( 0, 0 ), example_size );
        memDC.Clear();
        memDC.SetUserScale( scale, scale );

        // Get logical page size and margins.
        PAGE_INFO pageDUMMY;

        // Get page type
        int idx = m_paperSizeComboBox->GetSelection();

        if( idx < 0 )
            idx = 0;

        void*          clientData = m_paperSizeComboBox->GetClientData( idx );
        PAGE_SIZE_TYPE pageType = static_cast<PAGE_SIZE_TYPE>( reinterpret_cast<intptr_t>( clientData ) );

        bool portrait = clamped_layout_size.x < clamped_layout_size.y;
        pageDUMMY.SetType( pageType, portrait );

        if( m_customFmt )
        {
            pageDUMMY.SetWidthMils( clamped_layout_size.x );
            pageDUMMY.SetHeightMils( clamped_layout_size.y );
        }

        // Draw layout preview.
        KIGFX::DS_RENDER_SETTINGS renderSettings;
        COLOR_SETTINGS*           colorSettings = m_parent->GetColorSettings();
        COLOR4D                   bgColor = m_parent->GetDrawBgColor();
        wxString                  emptyString;

        DS_DATA_MODEL::SetAltInstance( m_drawingSheet );
        {
            GRResetPenAndBrush( &memDC );
            renderSettings.SetDefaultPenWidth( 1 );
            renderSettings.LoadColors( colorSettings );
            renderSettings.SetPrintDC( &memDC );

            if( m_parent->IsType( FRAME_SCH )
                || m_parent->IsType( FRAME_SCH_SYMBOL_EDITOR )
                || m_parent->IsType( FRAME_SCH_VIEWER ) )
            {
                COLOR4D color = renderSettings.GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );
                renderSettings.SetLayerColor( LAYER_DRAWINGSHEET, color );
            }

            GRFilledRect( &memDC, VECTOR2I( 0, 0 ), m_layout_size, 0, bgColor, bgColor );

            PrintDrawingSheet( &renderSettings, pageDUMMY, emptyString, emptyString, emptyString,
                               m_tb, nullptr, m_screen->GetPageCount(), m_screen->GetPageNumber(),
                               1, &Prj(), wxEmptyString, m_screen->GetVirtualPageNumber() == 1 );

            memDC.SelectObject( wxNullBitmap );
            m_PageLayoutExampleBitmap->SetBitmap( *m_pageBitmap );
        }

        DS_DATA_MODEL::SetAltInstance( nullptr );

        // Refresh the dialog.
        Layout();
        Refresh();
    }
}


void DIALOG_PAGES_SETTINGS::GetPageLayoutInfoFromDialog()
{
    int            idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    void*          clientData = m_paperSizeComboBox->GetClientData( idx );
    PAGE_SIZE_TYPE pageType = static_cast<PAGE_SIZE_TYPE>( reinterpret_cast<intptr_t>( clientData ) );

    if( pageType == PAGE_SIZE_TYPE::User )
    {
        GetCustomSizeMilsFromDialog();

        if( m_layout_size.x && m_layout_size.y )
        {
            if( m_layout_size.x < m_layout_size.y )
                m_orientationComboBox->SetStringSelection( _( "Portrait" ) );
            else
                m_orientationComboBox->SetStringSelection( _( "Landscape" ) );
        }
    }
    else
    {
        PAGE_INFO       pageInfo;   // SetType() later to lookup size

        pageInfo.SetType( pageType );

        VECTOR2D sz = pageInfo.GetSizeMils();
        m_layout_size = VECTOR2D( sz.x, sz.y );

        // swap sizes to match orientation
        bool isPortrait = (bool) m_orientationComboBox->GetSelection();

        if( ( isPortrait  && m_layout_size.x >= m_layout_size.y ) ||
            ( !isPortrait && m_layout_size.x <  m_layout_size.y ) )
        {
            std::swap( m_layout_size.x, m_layout_size.y );
        }
    }
}


void DIALOG_PAGES_SETTINGS::GetCustomSizeMilsFromDialog()
{
    double customSizeX = (double) m_customSizeX.GetDoubleValue() / m_iuPerMils;
    double customSizeY = (double) m_customSizeY.GetDoubleValue() / m_iuPerMils;

    // Ensure layout size can be converted to int coordinates later
    customSizeX = std::clamp( customSizeX, double( INT_MIN ), double( INT_MAX ) );
    customSizeY = std::clamp( customSizeY, double( INT_MIN ), double( INT_MAX ) );
    m_layout_size = VECTOR2D( customSizeX, customSizeY );
}


void DIALOG_PAGES_SETTINGS::OnWksFileSelection( wxCommandEvent& event )
{
    wxFileName fn = GetWksFileName();
    wxString   name = fn.GetFullName();
    wxString   path;
    wxString   msg;

    if( fn.IsAbsolute() )
    {
        path = fn.GetPath();
    }
    else
    {
        wxFileName expanded( ExpandEnvVarSubstitutions( GetWksFileName(), &m_parentFrame->Prj() ) );

        if( expanded.IsAbsolute() )
            path = expanded.GetPath();
        else
            path = m_projectPath;
    }

    // Display a file picker dialog
    FILEDLG_HOOK_EMBED_FILE customize;
    wxFileDialog fileDialog( this, _( "Drawing Sheet File" ), path, name, FILEEXT::DrawingSheetFileWildcard(),
                             wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( m_embeddedFiles )
        fileDialog.SetCustomizeHook( customize );

    if( fileDialog.ShowModal() != wxID_OK )
        return;

    wxString fileName = fileDialog.GetPath();
    wxString shortFileName;

    if( m_embeddedFiles && customize.GetEmbed() )
    {
        fn.Assign( fileName );
        EMBEDDED_FILES::EMBEDDED_FILE* result = m_embeddedFiles->AddFile( fn, true );
        shortFileName = result->GetLink();
        fileName = m_embeddedFiles->GetTemporaryFileName( result->name ).GetFullPath();
    }
    else if( !m_projectPath.IsEmpty() && fileName.StartsWith( m_projectPath ) )
    {
        // Try to use a project-relative path
        fn = wxFileName( fileName );
        fn.MakeRelativeTo( m_projectPath );
        shortFileName = fn.GetFullPath();
    }
    else
    {
        // Failing that see if we can shorten it with env vars:
        shortFileName = NormalizePath( fileName, &Pgm().GetLocalEnvVariables(), nullptr );
    }

    std::unique_ptr<DS_DATA_MODEL> ws = std::make_unique<DS_DATA_MODEL>();

    if( !ws->LoadDrawingSheet( fileName, &msg ) )
    {
        DisplayErrorMessage( this,
                             wxString::Format( _( "Error loading drawing sheet '%s'.\n%s" ),
                                               fileName ),
                             msg );
        return;
    }

    delete m_drawingSheet;

    m_drawingSheet = ws.release();

    SetWksFileName( shortFileName );

    GetPageLayoutInfoFromDialog();
    UpdateDrawingSheetExample();
}
