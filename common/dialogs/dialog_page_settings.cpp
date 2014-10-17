/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Kicad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_page_settings.cpp
 */

#include <fctsys.h>
#include <macros.h>              // DIM()
#include <common.h>
#include <project.h>
#include <confirm.h>
#include <gr_basic.h>
#include <base_struct.h>
#include <class_drawpanel.h>
#include <class_title_block.h>
#include <draw_frame.h>
#include <worksheet_shape_builder.h>
#include <class_base_screen.h>
#include <wildcards_and_files_ext.h>

#include <wx/valgen.h>
#include <wx/tokenzr.h>

#ifdef EESCHEMA
#include <class_sch_screen.h>
#include <general.h>
#endif

#include <worksheet.h>
#include <dialog_page_settings.h>


// List of page formats.
// should be statically initialized, because we need both
// the translated and the not translated version.
// when displayed in dialog we should explicitely call wxGetTranslation()
// to show the translated version.
static const wxString pageFmts[] =
{
    _("A4 210x297mm"),
    _("A3 297x420mm"),
    _("A2 420x594mm"),
    _("A1 594x841mm"),
    _("A0 841x1189mm"),
    _("A 8.5x11in"),
    _("B 11x17in"),
    _("C 17x22in"),
    _("D 22x34in"),
    _("E 34x44in"),
    _("USLetter 8.5x11in"),     // USLetter without space is correct
    _("USLegal 8.5x14in"),      // USLegal without space is correct
    _("USLedger 11x17in"),      // USLedger without space is correct
    _("User (Custom)"),
};

void EDA_DRAW_FRAME::Process_PageSettings( wxCommandEvent& event )
{
    DIALOG_PAGES_SETTINGS dlg( this );
    dlg.SetWksFileName( BASE_SCREEN::m_PageLayoutDescrFileName );
    int diag = dlg.ShowModal();

    if( m_canvas && diag )
        m_canvas->Refresh();
}


DIALOG_PAGES_SETTINGS::DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* parent ) :
    DIALOG_PAGES_SETTINGS_BASE( parent ),
    m_initialized( false )
{
    m_parent   = parent;
    m_screen   = m_parent->GetScreen();
    m_page_bitmap = NULL;
    m_tb = m_parent->GetTitleBlock();
    m_customFmt = false;
    m_localPrjConfigChanged = false;

    initDialog();

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_PAGES_SETTINGS::~DIALOG_PAGES_SETTINGS()
{
    if( m_page_bitmap )
        delete m_page_bitmap;
}


void DIALOG_PAGES_SETTINGS::initDialog()
{
    wxString    msg;
    double      customSizeX;
    double      customSizeY;

    // initalize page format choice box and page format list.
    // The first shows translated strings, the second contains not translated strings
    m_paperSizeComboBox->Clear();

    for( unsigned ii = 0; ii < DIM(pageFmts); ii++ )
    {
        m_pageFmt.Add( pageFmts[ii] );
        m_paperSizeComboBox->Append( wxGetTranslation( pageFmts[ii] ) );
    }

    // initialize the page layout descr filename
    SetWksFileName( BASE_SCREEN::m_PageLayoutDescrFileName );


#ifdef EESCHEMA
    // Init display value for schematic sub-sheet number
    wxString format = m_TextSheetCount->GetLabel();
    msg.Printf( format, m_screen->m_NumberOfScreens );
    m_TextSheetCount->SetLabel( msg );

    format = m_TextSheetNumber->GetLabel();
    msg.Printf( format, m_screen->m_ScreenNumber );
    m_TextSheetNumber->SetLabel( msg );
#else
    m_TextSheetCount->Show( false );
    m_TextSheetNumber->Show( false );
#endif

    m_pageInfo = m_parent->GetPageSettings();
    SetCurrentPageSizeSelection( m_pageInfo.GetType() );
    m_orientationComboBox->SetSelection( m_pageInfo.IsPortrait() );

    // only a click fires the "selection changed" event, so have to fabricate this check
    wxCommandEvent dummy;
    OnPaperSizeChoice( dummy );

    if( m_customFmt)    // The custom value is defined by the page size
    {
        customSizeX = m_pageInfo.GetWidthMils();
        customSizeY = m_pageInfo.GetHeightMils();
    }
    else    // The custom value is set to a default value, or the last defined value
    {
        customSizeX = m_pageInfo.GetCustomWidthMils();
        customSizeY = m_pageInfo.GetCustomHeightMils();
    }

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        customSizeX *= 25.4e-3;
        customSizeY *= 25.4e-3;

        msg.Printf( wxT( "%.2f" ), customSizeX );
        m_TextUserSizeX->SetValue( msg );

        msg.Printf( wxT( "%.2f" ), customSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;

    default:
    case INCHES:
        customSizeX /= 1000.0;
        customSizeY /= 1000.0;

        msg.Printf( wxT( "%.3f" ), customSizeX );
        m_TextUserSizeX->SetValue( msg );

        msg.Printf( wxT( "%.3f" ), customSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;
    }

    m_TextRevision->SetValue( m_tb.GetRevision() );
    m_TextDate->SetValue( m_tb.GetDate() );
    m_TextTitle->SetValue( m_tb.GetTitle() );
    m_TextCompany->SetValue( m_tb.GetCompany() );
    m_TextComment1->SetValue( m_tb.GetComment1() );
    m_TextComment2->SetValue( m_tb.GetComment2() );
    m_TextComment3->SetValue( m_tb.GetComment3() );
    m_TextComment4->SetValue( m_tb.GetComment4() );

#ifndef EESCHEMA
    m_RevisionExport->Show( false );
    m_DateExport->Show( false );
    m_TitleExport->Show( false );
    m_CompanyExport->Show( false );
    m_Comment1Export->Show( false );
    m_Comment2Export->Show( false );
    m_Comment3Export->Show( false );
    m_Comment4Export->Show( false );
#endif

    GetPageLayoutInfoFromDialog();
    UpdatePageLayoutExample();

    // Make the OK button the default.
    m_sdbSizer1OK->SetDefault();
    m_initialized = true;
}


void DIALOG_PAGES_SETTINGS::OnOkClick( wxCommandEvent& event )
{
    if( SavePageSettings() )
    {
        m_screen->SetModify();
        m_parent->GetCanvas()->Refresh();

        if( m_localPrjConfigChanged )
            m_parent->SaveProjectSettings( true );

        EndModal( true );
    }
}


void DIALOG_PAGES_SETTINGS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( false );
}


void DIALOG_PAGES_SETTINGS::OnPaperSizeChoice( wxCommandEvent& event )
{
    int idx = m_paperSizeComboBox->GetSelection();

    if( idx < 0 )
        idx = 0;

    const wxString paperType = m_pageFmt[idx];

    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        m_orientationComboBox->Enable( false );
        m_TextUserSizeX->Enable( true );
        m_TextUserSizeY->Enable( true );
        m_customFmt = true;
    }
    else
    {
        m_orientationComboBox->Enable( true );

#if 0
        // ForcePortrait() does not exist, but could be useful.
        // so I leave these lines, which could be seen as a todo feature
        if( paperType.ForcePortrait() )
        {
            m_orientationComboBox->SetStringSelection( _( "Portrait" ) );
            m_orientationComboBox->Enable( false );
        }
#endif
        m_TextUserSizeX->Enable( false );
        m_TextUserSizeY->Enable( false );
        m_customFmt = false;
    }

    GetPageLayoutInfoFromDialog();
    UpdatePageLayoutExample();
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeXTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextUserSizeX->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeYTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextUserSizeY->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnPageOrientationChoice( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnRevisionTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextRevision->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetRevision( m_TextRevision->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnDateTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextDate->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetDate( m_TextDate->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnTitleTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextTitle->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetTitle( m_TextTitle->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnCompanyTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextCompany->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetCompany( m_TextCompany->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment1TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment1->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment1( m_TextComment1->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment2TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment2->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment2( m_TextComment2->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment3TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment3->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment3( m_TextComment3->GetValue() );
        UpdatePageLayoutExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment4TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment4->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment4( m_TextComment4->GetValue() );
        UpdatePageLayoutExample();
    }
}

void DIALOG_PAGES_SETTINGS::OnDateApplyClick( wxCommandEvent& event )
{
    m_TextDate->SetValue( FormatDateLong( m_PickDate->GetValue() ) );
}


bool DIALOG_PAGES_SETTINGS::SavePageSettings()
{
    bool retSuccess = false;

    wxString fileName = GetWksFileName();

    if( fileName != BASE_SCREEN::m_PageLayoutDescrFileName )
    {
        if( !fileName.IsEmpty() )
        {
            wxString fullFileName = WORKSHEET_LAYOUT::MakeFullFileName( fileName );
            if( !wxFileExists( fullFileName ) )
            {
                wxString msg;
                msg.Printf( _("Page layout description file <%s> not found. Abort"),
                            GetChars( fullFileName ) );
                wxMessageBox( msg );
                return false;
            }
        }

        BASE_SCREEN::m_PageLayoutDescrFileName = fileName;
        WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
        pglayout.SetPageLayout( fileName );
        m_localPrjConfigChanged = true;
    }

    int idx = m_paperSizeComboBox->GetSelection();

    if( idx < 0 )
        idx = 0;

    const wxString paperType = m_pageFmt[idx];

    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        GetCustomSizeMilsFromDialog();

        retSuccess = m_pageInfo.SetType( PAGE_INFO::Custom );

        if( retSuccess )
        {
            if( m_layout_size.x < MIN_PAGE_SIZE || m_layout_size.y < MIN_PAGE_SIZE ||
                m_layout_size.x > MAX_PAGE_SIZE || m_layout_size.y > MAX_PAGE_SIZE )
            {
                wxString msg = wxString::Format( _( "Selected custom paper size\nis out of the permissible \
limits\n%.1f - %.1f %s!\nSelect another custom paper size?" ),
                        g_UserUnit == INCHES ? MIN_PAGE_SIZE / 1000. : MIN_PAGE_SIZE * 25.4 / 1000,
                        g_UserUnit == INCHES ? MAX_PAGE_SIZE / 1000. : MAX_PAGE_SIZE * 25.4 / 1000,
                        g_UserUnit == INCHES ? _( "inches" ) : _( "mm" ) );

                if( wxMessageBox( msg, _( "Warning!" ), wxYES_NO | wxICON_EXCLAMATION, this ) == wxYES )
                {
                    return false;
                }

                m_layout_size.x = Clamp( MIN_PAGE_SIZE, m_layout_size.x, MAX_PAGE_SIZE );
                m_layout_size.y = Clamp( MIN_PAGE_SIZE, m_layout_size.y, MAX_PAGE_SIZE );
            }

            PAGE_INFO::SetCustomWidthMils( m_layout_size.x );
            PAGE_INFO::SetCustomHeightMils( m_layout_size.y );

            m_pageInfo.SetWidthMils( m_layout_size.x );
            m_pageInfo.SetHeightMils( m_layout_size.y );
        }
    }
    else
    {
        // search for longest common string first, e.g. A4 before A
        if( paperType.Contains( PAGE_INFO::USLetter ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::USLetter );
        else if( paperType.Contains( PAGE_INFO::USLegal ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::USLegal );
        else if( paperType.Contains( PAGE_INFO::USLedger ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::USLedger );
        else if( paperType.Contains( PAGE_INFO::GERBER ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::GERBER );
        else if( paperType.Contains( PAGE_INFO::A4 ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A4 );
        else if( paperType.Contains( PAGE_INFO::A3 ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A3 );
        else if( paperType.Contains( PAGE_INFO::A2 ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A2 );
        else if( paperType.Contains( PAGE_INFO::A1 ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A1 );
        else if( paperType.Contains( PAGE_INFO::A0 ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A0 );
        else if( paperType.Contains( PAGE_INFO::A ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::A );
        else if( paperType.Contains( PAGE_INFO::B ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::B );
        else if( paperType.Contains( PAGE_INFO::C ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::C );
        else if( paperType.Contains( PAGE_INFO::D ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::D );
        else if( paperType.Contains( PAGE_INFO::E ) )
            retSuccess = m_pageInfo.SetType( PAGE_INFO::E );

        if( retSuccess )
        {
            int choice = m_orientationComboBox->GetSelection();
            m_pageInfo.SetPortrait( choice != 0 );
        }
    }

    if( !retSuccess )
    {
        wxASSERT_MSG( false, wxT( "the translation for paper size must preserve original spellings" ) );
        m_pageInfo.SetType( PAGE_INFO::A4 );
    }

    m_parent->SetPageSettings( m_pageInfo );

    m_tb.SetRevision( m_TextRevision->GetValue() );
    m_tb.SetDate(     m_TextDate->GetValue() );
    m_tb.SetCompany(  m_TextCompany->GetValue() );
    m_tb.SetTitle(    m_TextTitle->GetValue() );
    m_tb.SetComment1( m_TextComment1->GetValue() );
    m_tb.SetComment2( m_TextComment2->GetValue() );
    m_tb.SetComment3( m_TextComment3->GetValue() );
    m_tb.SetComment4( m_TextComment4->GetValue() );

    m_parent->SetTitleBlock( m_tb );


#ifdef EESCHEMA
    // Exports settings to other sheets if requested:
    SCH_SCREEN* screen;

    // Build the screen list
    SCH_SCREENS ScreenList;

    // Update title blocks for all screens
    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        if( screen == m_screen )
            continue;

        TITLE_BLOCK tb2 = screen->GetTitleBlock();

        if( m_RevisionExport->IsChecked() )
            tb2.SetRevision( m_tb.GetRevision() );

        if( m_DateExport->IsChecked() )
            tb2.SetDate( m_tb.GetDate() );

        if( m_TitleExport->IsChecked() )
            tb2.SetTitle( m_tb.GetTitle() );

        if( m_CompanyExport->IsChecked() )
            tb2.SetCompany( m_tb.GetCompany() );

        if( m_Comment1Export->IsChecked() )
            tb2.SetComment1( m_tb.GetComment1() );

        if( m_Comment2Export->IsChecked() )
            tb2.SetComment2( m_tb.GetComment2() );

        if( m_Comment3Export->IsChecked() )
            tb2.SetComment3( m_tb.GetComment3() );

        if( m_Comment4Export->IsChecked() )
            tb2.SetComment4( m_tb.GetComment4() );

        screen->SetTitleBlock( tb2 );
    }

#endif

    return true;
}


void DIALOG_PAGES_SETTINGS::SetCurrentPageSizeSelection( const wxString& aPaperSize )
{
    // search all the not translated label list containing our paper type
    for( unsigned i = 0; i < m_pageFmt.GetCount(); ++i )
    {
        // parse each label looking for aPaperSize within it
        wxStringTokenizer st( m_pageFmt[i] );

        while( st.HasMoreTokens() )
        {
            if( st.GetNextToken() == aPaperSize )
            {
                m_paperSizeComboBox->SetSelection( i );
                return;
            }
        }
    }
}


void DIALOG_PAGES_SETTINGS::UpdatePageLayoutExample()
{
    int lyWidth, lyHeight;

    wxSize clamped_layout_size( Clamp( MIN_PAGE_SIZE, m_layout_size.x, MAX_PAGE_SIZE ),
                                Clamp( MIN_PAGE_SIZE, m_layout_size.y, MAX_PAGE_SIZE ) );

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

    if( m_page_bitmap )
    {
        m_PageLayoutExampleBitmap->SetBitmap( wxNullBitmap );
        delete m_page_bitmap;
    }

    m_page_bitmap = new wxBitmap( lyWidth + 1, lyHeight + 1 );

    if( m_page_bitmap->IsOk() )
    {
        // Calculate layout preview scale.
        int appScale = m_screen->MilsToIuScalar();

        double scaleW = (double) lyWidth  / clamped_layout_size.x / appScale;
        double scaleH = (double) lyHeight / clamped_layout_size.y / appScale;

        // Prepare DC.
        wxSize example_size( lyWidth + 1, lyHeight + 1 );
        wxMemoryDC memDC;
        memDC.SelectObject( *m_page_bitmap );
        memDC.SetClippingRegion( wxPoint( 0, 0 ), example_size );
        memDC.Clear();
        memDC.SetUserScale( scaleW, scaleH );

        // Get logical page size and margins.
        PAGE_INFO pageDUMMY;

        // Get page type
        int idx = m_paperSizeComboBox->GetSelection();

        if( idx < 0 )
            idx = 0;

        wxString pageFmtName = m_pageFmt[idx].BeforeFirst( ' ' );
        bool portrait = clamped_layout_size.x < clamped_layout_size.y;
        pageDUMMY.SetType( pageFmtName, portrait );
        if( m_customFmt )
        {
            pageDUMMY.SetWidthMils( clamped_layout_size.x );
            pageDUMMY.SetHeightMils( clamped_layout_size.y );
        }

        // Draw layout preview.
        wxString emptyString;
        GRResetPenAndBrush( &memDC );

        DrawPageLayout( &memDC, NULL, pageDUMMY,
                        emptyString, emptyString,
                        m_tb, m_screen->m_NumberOfScreens,
                        m_screen->m_ScreenNumber, 1, appScale, DARKGRAY, RED );

        memDC.SelectObject( wxNullBitmap );
        m_PageLayoutExampleBitmap->SetBitmap( *m_page_bitmap );

        // Refresh the dialog.
        Layout();
        Refresh();
    }
}


void DIALOG_PAGES_SETTINGS::GetPageLayoutInfoFromDialog()
{
    int idx = m_paperSizeComboBox->GetSelection();

    if( idx < 0 )
        idx = 0;

    const wxString paperType = m_pageFmt[idx];

    // here we assume translators will keep original paper size spellings
    if( paperType.Contains( PAGE_INFO::Custom ) )
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

        static const wxChar* papers[] = {
            // longest common string first, since sequential search below
            PAGE_INFO::A4,
            PAGE_INFO::A3,
            PAGE_INFO::A2,
            PAGE_INFO::A1,
            PAGE_INFO::A0,
            PAGE_INFO::A,
            PAGE_INFO::B,
            PAGE_INFO::C,
            PAGE_INFO::D,
            PAGE_INFO::E,
            PAGE_INFO::USLetter,
            PAGE_INFO::USLegal,
            PAGE_INFO::USLedger,
        };

        unsigned i;

        for( i=0;  i < DIM( papers );  ++i )
        {
            if( paperType.Contains( papers[i] ) )
            {
                pageInfo.SetType( papers[i] );
                break;
            }
        }

        wxASSERT( i != DIM(papers) );   // dialog UI match the above list?

        m_layout_size = pageInfo.GetSizeMils();

        // swap sizes to match orientation
        bool isPortrait = (bool) m_orientationComboBox->GetSelection();

        if( ( isPortrait  && m_layout_size.x >= m_layout_size.y ) ||
            ( !isPortrait && m_layout_size.x <  m_layout_size.y ) )
        {
            m_layout_size.Set( m_layout_size.y, m_layout_size.x );
        }
    }
}


void DIALOG_PAGES_SETTINGS::GetCustomSizeMilsFromDialog()
{
    double      customSizeX;
    double      customSizeY;
    wxString    msg;

    msg = m_TextUserSizeX->GetValue();
    msg.ToDouble( &customSizeX );

    msg = m_TextUserSizeY->GetValue();
    msg.ToDouble( &customSizeY );

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        customSizeX *= 1000. / 25.4;
        customSizeY *= 1000. / 25.4;
        break;

    default:
    case INCHES:
        customSizeX *= 1000.;
        customSizeY *= 1000.;
    }

    // Prepare to painless double -> int conversion.
    customSizeX = Clamp( double( INT_MIN ), customSizeX, double( INT_MAX ) );
    customSizeY = Clamp( double( INT_MIN ), customSizeY, double( INT_MAX ) );
    m_layout_size = wxSize( KiROUND( customSizeX ), KiROUND( customSizeY ) );
}

// Called on .kicad_wks file description selection change
void DIALOG_PAGES_SETTINGS::OnWksFileSelection( wxCommandEvent& event )
{
    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    // Display a file picker dialog
    wxFileDialog fileDialog( this, _( "Select Page Layout Descr File" ),
                             pro_dir, GetWksFileName(),
                             PageLayoutDescrFileWildcard,
                             wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( fileDialog.ShowModal() != wxID_OK )
        return;

    wxString fileName = fileDialog.GetPath();

    // Try to remove the path, if the path is the current working dir,
    // or the dir of kicad.pro (template)
    wxString shortFileName = WORKSHEET_LAYOUT::MakeShortFileName( fileName );
    wxFileName fn = shortFileName;

    // For Win/Linux/macOS compatibility, a relative path is a good idea
    if( fn.IsAbsolute() && fileName != GetWksFileName() )
    {
        fn.MakeRelativeTo( pro_dir );

        wxString msg = wxString::Format( _(
                "The page layout descr filename has changed.\n"
                "Do you want to use the relative path:\n"
                "'%s'" ),
                GetChars( fn.GetFullPath() )
                );
        if( IsOK( this, msg ) )
            shortFileName = fn.GetFullPath();
    }

    SetWksFileName( shortFileName );
}
