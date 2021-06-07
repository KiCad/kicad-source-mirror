/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <common.h>     // For ExpandEnvVarSubstitutions
#include <dialogs/wx_html_report_panel.h>
#include <dialog_plot_schematic.h>
#include <eeschema_settings.h>
#include <kiface_i.h>
#include <locale_io.h>
#include <plotters_specific.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <settings/settings_manager.h>
#include <drawing_sheet/ds_painter.h>

#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <schematic.h>
#include <sch_screen.h>

#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/log.h>


// static members (static to remember last state):
int DIALOG_PLOT_SCHEMATIC::m_pageSizeSelect = PAGE_SIZE_AUTO;
int DIALOG_PLOT_SCHEMATIC::m_HPGLPaperSizeSelect = PAGE_SIZE_AUTO;


enum HPGL_PAGEZ_T {
    PAGE_DEFAULT = 0,
    HPGL_PAGE_SIZE_A5,
    HPGL_PAGE_SIZE_A4,
    HPGL_PAGE_SIZE_A3,
    HPGL_PAGE_SIZE_A2,
    HPGL_PAGE_SIZE_A1,
    HPGL_PAGE_SIZE_A0,
    HPGL_PAGE_SIZE_A,
    HPGL_PAGE_SIZE_B,
    HPGL_PAGE_SIZE_C,
    HPGL_PAGE_SIZE_D,
    HPGL_PAGE_SIZE_E,
};


static const wxChar* plot_sheet_list( int aSize )
{
    switch( aSize )
    {
    default:
    case PAGE_DEFAULT:      return nullptr;
    case HPGL_PAGE_SIZE_A5: return wxT( "A5" );
    case HPGL_PAGE_SIZE_A4: return wxT( "A4" );
    case HPGL_PAGE_SIZE_A3: return wxT( "A3" );
    case HPGL_PAGE_SIZE_A2: return wxT( "A2" );
    case HPGL_PAGE_SIZE_A1: return wxT( "A1" );
    case HPGL_PAGE_SIZE_A0: return wxT( "A0" );
    case HPGL_PAGE_SIZE_A:  return wxT( "A" );
    case HPGL_PAGE_SIZE_B:  return wxT( "B" );
    case HPGL_PAGE_SIZE_C:  return wxT( "C" );
    case HPGL_PAGE_SIZE_D:  return wxT( "D" );
    case HPGL_PAGE_SIZE_E:  return wxT( "E" );
    }
}


DIALOG_PLOT_SCHEMATIC::DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent )
        : DIALOG_PLOT_SCHEMATIC_BASE( parent ),
          m_parent( parent ),
          m_plotFormat( PLOT_FORMAT::UNDEFINED ),
          m_HPGLPenSize( 1.0 ),
          m_defaultLineWidth( parent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
          m_penWidth( parent, m_penWidthLabel, m_penWidthCtrl, m_penWidthUnits )
{
    m_configChanged = false;

    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    m_MessagesBox->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Plot All Pages" ) );
    m_sdbSizer1Apply->SetLabel( _( "Plot Current Page" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();
    initDlg();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


// Initialize the dialog options:
void DIALOG_PLOT_SCHEMATIC::initDlg()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( COLOR_SETTINGS* settings : m_parent->GetSettingsManager()->GetColorSettingsList() )
        {
            int idx = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

            if( settings->GetFilename() == cfg->m_PlotPanel.color_theme )
                m_colorTheme->SetSelection( idx );
        }

        m_colorTheme->Enable( cfg->m_PlotPanel.color );

        m_plotBackgroundColor->Enable( cfg->m_PlotPanel.color );
        m_plotBackgroundColor->SetValue( cfg->m_PlotPanel.background_color );

        // Set color or B&W plot option
        setModeColor( cfg->m_PlotPanel.color );

        // Set plot or not frame reference option
        setPlotDrawingSheet( cfg->m_PlotPanel.frame_reference );

        // HPGL plot origin and unit system configuration
        m_plotOriginOpt->SetSelection( cfg->m_PlotPanel.hpgl_origin );

        m_HPGLPaperSizeSelect = cfg->m_PlotPanel.hpgl_paper_size;

        // HPGL Pen Size is stored in mm in config
        m_HPGLPenSize = cfg->m_PlotPanel.hpgl_pen_size * IU_PER_MM;

        // Switch to the last save plot format
        PLOT_FORMAT fmt = static_cast<PLOT_FORMAT>( cfg->m_PlotPanel.format );

        switch( fmt  )
        {
        default:
        case PLOT_FORMAT::POST: m_plotFormatOpt->SetSelection( 0 ); break;
        case PLOT_FORMAT::PDF:  m_plotFormatOpt->SetSelection( 1 ); break;
        case PLOT_FORMAT::SVG:  m_plotFormatOpt->SetSelection( 2 ); break;
        case PLOT_FORMAT::DXF:  m_plotFormatOpt->SetSelection( 3 ); break;
        case PLOT_FORMAT::HPGL: m_plotFormatOpt->SetSelection( 4 ); break;
        }

        if( fmt == PLOT_FORMAT::DXF || fmt == PLOT_FORMAT::HPGL )
            m_plotBackgroundColor->Disable();

        // Set the default line width (pen width which should be used for
        // items that do not have a pen size defined (like frame ref)
        // the default line width is stored in mils in config
        m_defaultLineWidth.SetValue( Mils2iu( cfg->m_Drawing.default_line_thickness ) );
    }

    // Initialize HPGL specific widgets
    m_penWidth.SetValue( m_HPGLPenSize );

    // Plot directory
    SCHEMATIC_SETTINGS& settings = m_parent->Schematic().Settings();
    wxString            path     = settings.m_PlotDirectoryName;
#ifdef __WINDOWS__
    path.Replace( '/', '\\' );
#endif
    m_outputDirectoryName->SetValue( path );
}


/**
 * @todo Copy of DIALOG_PLOT::OnOutputDirectoryBrowseClicked in dialog_plot.cpp, maybe merge to
 *       a common method.
 */
void DIALOG_PLOT_SCHEMATIC::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );

    // When editing a schematic that is not part of a project in the stand alone mode, the
    // project path is not defined so point to the users document path to save the plot files.
    if( Prj().IsNullProject() )
    {
        path = wxStandardPaths::Get().GetDocumentsDir();
    }
    else
    {
        // Build the absolute path of current output directory to preselect it in the file browser.
        path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );
        path = Prj().AbsolutePath( path );
    }

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    wxFileName fn( Prj().AbsolutePath( m_parent->Schematic().Root().GetFileName() ) );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    wxFileName relPathTest; // Used to test if we can make the path relative

    relPathTest.Assign( dirDialog.GetPath() );

    // Test if making the path relative is possible before asking the user if they want to do it
    if( relPathTest.MakeRelativeTo( defaultPath ) )
    {
        msg.Printf( _( "Do you want to use a path relative to\n\"%s\"" ), defaultPath );

        wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                                wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

        if( dialog.ShowModal() == wxID_YES )
            dirName.MakeRelativeTo( defaultPath );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


PLOT_FORMAT DIALOG_PLOT_SCHEMATIC::GetPlotFileFormat()
{
    switch( m_plotFormatOpt->GetSelection() )
    {
    default:
    case 0:
        return PLOT_FORMAT::POST;
    case 1:
        return PLOT_FORMAT::PDF;
    case 2:
        return PLOT_FORMAT::SVG;
    case 3:
        return PLOT_FORMAT::DXF;
    case 4:
        return PLOT_FORMAT::HPGL;
    }
}


void DIALOG_PLOT_SCHEMATIC::OnPageSizeSelected( wxCommandEvent& event )
{
    if( GetPlotFileFormat() == PLOT_FORMAT::HPGL )
        m_HPGLPaperSizeSelect = m_paperSizeOption->GetSelection();
    else
        m_pageSizeSelect = m_paperSizeOption->GetSelection();
}


void DIALOG_PLOT_SCHEMATIC::OnUpdateUI( wxUpdateUIEvent& event )
{
    PLOT_FORMAT fmt = GetPlotFileFormat();

    if( fmt != m_plotFormat )
    {
        m_plotFormat = fmt;

        wxArrayString paperSizes;
        paperSizes.push_back( _( "Schematic size" ) );

        int selection;

        if( fmt == PLOT_FORMAT::HPGL )
        {
            paperSizes.push_back( _( "A5" ) );
            paperSizes.push_back( _( "A4" ) );
            paperSizes.push_back( _( "A3" ) );
            paperSizes.push_back( _( "A2" ) );
            paperSizes.push_back( _( "A1" ) );
            paperSizes.push_back( _( "A0" ) );
            paperSizes.push_back( _( "A" ) );
            paperSizes.push_back( _( "B" ) );
            paperSizes.push_back( _( "C" ) );
            paperSizes.push_back( _( "D" ) );
            paperSizes.push_back( _( "E" ) );

            selection = m_HPGLPaperSizeSelect;
        }
        else
        {
            paperSizes.push_back( _( "A4" ) );
            paperSizes.push_back( _( "A" ) );

            selection = m_pageSizeSelect;
        }

        m_paperSizeOption->Set( paperSizes );
        m_paperSizeOption->SetSelection( selection );

        m_defaultLineWidth.Enable(
                fmt == PLOT_FORMAT::POST || fmt == PLOT_FORMAT::PDF || fmt == PLOT_FORMAT::SVG );

        m_plotOriginTitle->Enable( fmt == PLOT_FORMAT::HPGL );
        m_plotOriginOpt->Enable( fmt == PLOT_FORMAT::HPGL );
        m_penWidth.Enable( fmt == PLOT_FORMAT::HPGL );

        m_plotBackgroundColor->Enable(
                fmt == PLOT_FORMAT::POST || fmt == PLOT_FORMAT::PDF || fmt == PLOT_FORMAT::SVG );

        m_colorTheme->Enable( fmt != PLOT_FORMAT::HPGL );
        m_ModeColorOption->Enable( fmt != PLOT_FORMAT::HPGL );
    }
}


void DIALOG_PLOT_SCHEMATIC::getPlotOptions( RENDER_SETTINGS* aSettings )
{
    m_HPGLPenSize = m_penWidth.GetValue();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    COLOR_SETTINGS* colors = getColorSettings();

    if( cfg )
    {
        cfg->m_PlotPanel.background_color = m_plotBackgroundColor->GetValue();
        cfg->m_PlotPanel.color            = getModeColor();
        cfg->m_PlotPanel.color_theme      = colors->GetFilename();
        cfg->m_PlotPanel.frame_reference  = getPlotDrawingSheet();
        cfg->m_PlotPanel.format           = static_cast<int>( GetPlotFileFormat() );
        cfg->m_PlotPanel.hpgl_origin      = m_plotOriginOpt->GetSelection();
        cfg->m_PlotPanel.hpgl_paper_size  = m_HPGLPaperSizeSelect;

        // HPGL Pen Size is stored in mm in config
        cfg->m_PlotPanel.hpgl_pen_size = m_HPGLPenSize / IU_PER_MM;
    }

    aSettings->LoadColors( colors );
    aSettings->SetDefaultPenWidth( (int) m_defaultLineWidth.GetValue() );

    if( m_plotBackgroundColor->GetValue() )
        aSettings->SetBackgroundColor( colors->GetColor( LAYER_SCHEMATIC_BACKGROUND ) );
    else
        aSettings->SetBackgroundColor( COLOR4D::UNSPECIFIED );

    // Plot directory
    wxString path = m_outputDirectoryName->GetValue();
    path.Replace( '\\', '/' );

    SCHEMATIC_SETTINGS& settings = m_parent->Schematic().Settings();

    if( settings.m_PlotDirectoryName != path )
        m_configChanged = true;

    settings.m_PlotDirectoryName = path;
}


COLOR_SETTINGS* DIALOG_PLOT_SCHEMATIC::getColorSettings()
{
    int selection = m_colorTheme->GetSelection();

    if( selection < 0 )
        return m_parent->GetSettingsManager()->GetColorSettings( "_builtin_default" );

    return static_cast<COLOR_SETTINGS*>( m_colorTheme->GetClientData( selection ) );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotCurrent( wxCommandEvent& event )
{
    plotSchematic( false );
}


void DIALOG_PLOT_SCHEMATIC::OnPlotAll( wxCommandEvent& event )
{
    plotSchematic( true );
}


void DIALOG_PLOT_SCHEMATIC::plotSchematic( bool aPlotAll )
{
    KIGFX::SCH_RENDER_SETTINGS renderSettings( *m_parent->GetRenderSettings() );

    getPlotOptions( &renderSettings );

    switch( GetPlotFileFormat() )
    {
    default:
    case PLOT_FORMAT::POST:
        createPSFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::DXF:
        createDxfFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::PDF:
        createPDFFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::SVG:
        createSVGFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    case PLOT_FORMAT::HPGL:
        createHPGLFile( aPlotAll, getPlotDrawingSheet(), &renderSettings );
        break;
    }
}


wxFileName DIALOG_PLOT_SCHEMATIC::createPlotFileName( const wxString& aPlotFileName,
                                                      const wxString& aExtension,
                                                      REPORTER* aReporter )
{
    wxFileName retv;
    wxFileName tmp;

    tmp.SetPath( getOutputPath() );
    retv.SetPath( tmp.GetPath() );

    if( !aPlotFileName.IsEmpty() )
        retv.SetName( aPlotFileName );
    else
        retv.SetName( _( "Schematic" ) );

    retv.SetExt( aExtension );

    if( !EnsureFileDirectoryExists( &tmp, retv.GetFullName(), aReporter )
      || !tmp.IsDirWritable() )
    {
        wxString msg = wxString::Format( _( "Could not write plot files to folder \"%s\"." ),
                                         tmp.GetPath() );
        aReporter->Report( msg, RPT_SEVERITY_ERROR );
        retv.Clear();

        SCHEMATIC_SETTINGS& settings = m_parent->Schematic().Settings();
        settings.m_PlotDirectoryName.Clear();

        m_configChanged = true;
    }
    else
    {
        retv.SetPath( tmp.GetPath() );
    }

    wxLogTrace( tracePathsAndFiles, "Writing plot file '%s'.", retv.GetFullPath() );

    return retv;
}


void DIALOG_PLOT_SCHEMATIC::createDxfFile( bool aPlotAll, bool aPlotDrawingSheet,
                                           RENDER_SETTINGS*  aRenderSettings )
{
    SCH_EDIT_FRAME* schframe  = m_parent;
    SCH_SHEET_PATH  oldsheetpath = schframe->GetCurrentSheet();

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters
     * in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &schframe->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( schframe->GetCurrentSheet() );
    }

    REPORTER& reporter = m_MessagesBox->Reporter();

    for( unsigned i = 0; i < sheetList.size();  i++ )
    {
        schframe->SetCurrentSheet( sheetList[i] );
        schframe->GetCurrentSheet().UpdateAllScreenReferences();
        schframe->SetSheetNumberAndCount();

        SCH_SCREEN* screen = schframe->GetCurrentSheet().LastScreen();
        wxPoint     plot_offset;
        wxString    msg;

        try
        {
            wxString fname = schframe->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString ext = DXF_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( !plotFileName.IsOk() )
                return;

            if( plotOneSheetDxf( plotFileName.GetFullPath(), screen, aRenderSettings,
                                 plot_offset, 1.0, aPlotDrawingSheet ) )
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
            else    // Error
            {
                msg.Printf( _( "Unable to create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }
        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "DXF Plotter exception: %s"), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
            schframe->SetCurrentSheet( oldsheetpath );
            schframe->GetCurrentSheet().UpdateAllScreenReferences();
            schframe->SetSheetNumberAndCount();
            return;
        }
    }

    schframe->SetCurrentSheet( oldsheetpath );
    schframe->GetCurrentSheet().UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetDxf( const wxString&  aFileName,
                                             SCH_SCREEN*      aScreen,
                                             RENDER_SETTINGS* aRenderSettings,
                                             wxPoint          aPlotOffset,
                                             double           aScale,
                                             bool             aPlotFrameRef )
{
    aRenderSettings->LoadColors( getColorSettings() );
    aRenderSettings->SetDefaultPenWidth( 0 );

    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();
    DXF_PLOTTER*     plotter = new DXF_PLOTTER();

    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( getModeColor() );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlotOffset, IU_PER_MILS/10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-DXF" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &m_parent->Prj(), m_parent->GetTitleBlock(), pageInfo,
                          aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(),
                          plotter->GetColorMode() ?
                          plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) :
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    // finish
    plotter->EndPlot();
    delete plotter;

    return true;
}


void DIALOG_PLOT_SCHEMATIC::setHpglPenWidth()
{
    m_HPGLPenSize = m_penWidth.GetValue();

    if( m_HPGLPenSize > Millimeter2iu( 2 ) )
        m_HPGLPenSize = Millimeter2iu( 2 );

    if( m_HPGLPenSize < Millimeter2iu( 0.01 ) )
        m_HPGLPenSize = Millimeter2iu( 0.01 );
}


void DIALOG_PLOT_SCHEMATIC::createHPGLFile( bool aPlotAll, bool aPlotFrameRef,
                                            RENDER_SETTINGS* aRenderSettings )
{
    SCH_SCREEN*     screen = m_parent->GetScreen();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and other parameters
     *  in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    SCH_SHEET_LIST  sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &m_parent->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_parent->GetCurrentSheet() );
    }

    REPORTER& reporter = m_MessagesBox->Reporter();

    setHpglPenWidth();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();

        screen = m_parent->GetCurrentSheet().LastScreen();

        if( !screen ) // LastScreen() may return NULL
            screen = m_parent->GetScreen();

        const PAGE_INFO&    curPage = screen->GetPageSettings();

        PAGE_INFO           plotPage = curPage;

        // if plotting on a page size other than curPage
        if( m_paperSizeOption->GetSelection() != PAGE_DEFAULT )
            plotPage.SetType( plot_sheet_list( m_paperSizeOption->GetSelection() ) );

        // Calculation of conversion scales.
        double  plot_scale = (double) plotPage.GetWidthMils() / curPage.GetWidthMils();

        // Calculate offsets
        wxPoint plotOffset;
        wxString msg;

        if( getPlotOriginAndUnits() == HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER )
        {
            plotOffset.x    = plotPage.GetWidthIU() / 2;
            plotOffset.y    = -plotPage.GetHeightIU() / 2;
        }

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();
            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString ext = HPGL_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( !plotFileName.IsOk() )
                return;

            LOCALE_IO toggle;

            if( plotOneSheetHpgl( plotFileName.GetFullPath(), screen, plotPage, aRenderSettings,
                                  plotOffset, plot_scale, aPlotFrameRef, getPlotOriginAndUnits() ) )
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
            else
            {
                msg.Printf( _( "Unable to create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }
        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "HPGL Plotter exception: %s"), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
        }

    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetHpgl( const wxString&   aFileName,
                                              SCH_SCREEN*       aScreen,
                                              const PAGE_INFO&  aPageInfo,
                                              RENDER_SETTINGS*  aRenderSettings,
                                              wxPoint           aPlot0ffset,
                                              double            aScale,
                                              bool              aPlotFrameRef,
                                              HPGL_PLOT_ORIGIN_AND_UNITS aOriginAndUnits )
{
    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();
    // Currently, plot units are in decimil

    plotter->SetPageSettings( aPageInfo );
    plotter->SetRenderSettings( aRenderSettings );
    plotter->RenderSettings()->LoadColors( getColorSettings() );
    plotter->SetColorMode( getModeColor() );
    plotter->SetViewport( aPlot0ffset, IU_PER_MILS/10, aScale, false );

    // TODO this could be configurable
    plotter->SetTargetChordLength( Millimeter2iu( 0.6 ) );

    switch( aOriginAndUnits )
    {
    case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT:
    case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER:
    default:
        plotter->SetUserCoords( false );
        break;
    case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE:
        plotter->SetUserCoords( true );
        plotter->SetUserCoordsFit( false );
        break;
    case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT:
        plotter->SetUserCoords( true );
        plotter->SetUserCoordsFit( true );
        break;
    }

    // Init :
    plotter->SetCreator( wxT( "Eeschema-HPGL" ) );

    if( !plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;

    // Pen num and pen speed are not initialized here.
    // Default HPGL driver values are used
    plotter->SetPenDiameter( m_HPGLPenSize );
    plotter->StartPlot();

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &m_parent->Prj(), m_parent->GetTitleBlock(), aPageInfo,
                          aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(), COLOR4D::BLACK,
                          aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void DIALOG_PLOT_SCHEMATIC::createPDFFile( bool aPlotAll, bool aPlotDrawingSheet,
                                           RENDER_SETTINGS* aRenderSettings )
{
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();     // sheetpath is saved here

    /* When printing all pages, the printed page is not the current page.  In
     * complex hierarchies, we must update component references and others
     * parameters in the given printed SCH_SCREEN, accordant to the sheet path
     * because in complex hierarchies a SCH_SCREEN (a drawing ) is shared
     * between many sheets and component references depend on the actual sheet
     * path used
     */
    SCH_SHEET_LIST sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &m_parent->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_parent->GetCurrentSheet() );
    }

    // Allocate the plotter and set the job level parameter
    PDF_PLOTTER* plotter = new PDF_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetColorMode( getModeColor() );
    plotter->SetCreator( wxT( "Eeschema-PDF" ) );
    plotter->SetTitle( m_parent->GetTitleBlock().GetTitle() );

    wxString msg;
    wxFileName plotFileName;
    REPORTER& reporter = m_MessagesBox->Reporter();
    LOCALE_IO toggle;       // Switch the locale to standard C

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();
        SCH_SCREEN* screen = m_parent->GetCurrentSheet().LastScreen();

        if( i == 0 )
        {

            try
            {
                wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();

                // The sub sheet can be in a sub_hierarchy, but we plot the file in the
                // main project folder (or the folder specified by the caller),
                // so replace separators to create a unique filename:
                fname.Replace( "/", "_" );
                fname.Replace( "\\", "_" );
                wxString ext = PDF_PLOTTER::GetDefaultFileExtension();
                plotFileName = createPlotFileName( fname, ext, &reporter );

                if( !plotFileName.IsOk() )
                    return;

                if( !plotter->OpenFile( plotFileName.GetFullPath() ) )
                {
                    msg.Printf( _( "Unable to create file \"%s\".\n" ),
                                plotFileName.GetFullPath() );
                    reporter.Report( msg, RPT_SEVERITY_ERROR );
                    delete plotter;
                    return;
                }

                // Open the plotter and do the first page
                setupPlotPagePDF( plotter, screen );
                plotter->StartPlot();
            }
            catch( const IO_ERROR& e )
            {
                // Cannot plot PDF file
                msg.Printf( wxT( "PDF Plotter exception: %s" ), e.What() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );

                restoreEnvironment( plotter, oldsheetpath );
                return;
            }

        }
        else
        {
            /* For the following pages you need to close the (finished) page,
             *  reconfigure, and then start a new one */
            plotter->ClosePage();
            setupPlotPagePDF( plotter, screen );
            plotter->StartPage();
        }

        plotOneSheetPDF( plotter, screen, aPlotDrawingSheet );
    }

    // Everything done, close the plot and restore the environment
    msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
    reporter.Report( msg, RPT_SEVERITY_ACTION );

    restoreEnvironment( plotter, oldsheetpath );
}


void DIALOG_PLOT_SCHEMATIC::restoreEnvironment( PDF_PLOTTER* aPlotter,
                                                SCH_SHEET_PATH& aOldsheetpath )
{
    aPlotter->EndPlot();
    delete aPlotter;

    // Restore the previous sheet
    m_parent->SetCurrentSheet( aOldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


void DIALOG_PLOT_SCHEMATIC::plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                                             bool aPlotDrawingSheet )
{
    if( m_plotBackgroundColor->GetValue() )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetBackgroundColor() );
        wxPoint end( aPlotter->PageSettings().GetWidthIU(),
                     aPlotter->PageSettings().GetHeightIU() );
        aPlotter->Rect( wxPoint( 0, 0 ), end, FILL_TYPE::FILLED_SHAPE, 1.0 );
    }

    if( aPlotDrawingSheet )
    {
        COLOR4D color = COLOR4D::BLACK;

        if( aPlotter->GetColorMode() )
            color = aPlotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );

        PlotDrawingSheet( aPlotter, &aScreen->Schematic()->Prj(), m_parent->GetTitleBlock(),
                          m_parent->GetPageSettings(), aScreen->GetPageNumber(),
                          aScreen->GetPageCount(), m_parent->GetScreenDesc(),
                          aScreen->GetFileName(), color, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( aPlotter );
}


void DIALOG_PLOT_SCHEMATIC::setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen )
{
    PAGE_INFO   plotPage;                               // page size selected to plot

    // Considerations on page size and scaling requests
    const PAGE_INFO& actualPage = aScreen->GetPageSettings(); // page size selected in schematic

    switch( m_pageSizeSelect )
    {
    case PAGE_SIZE_A:
        plotPage.SetType( wxT( "A" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_A4:
        plotPage.SetType( wxT( "A4" ) );
        plotPage.SetPortrait( actualPage.IsPortrait() );
        break;

    case PAGE_SIZE_AUTO:
    default:
        plotPage = actualPage;
        break;
    }

    double  scalex  = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
    double  scaley  = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();
    double  scale   = std::min( scalex, scaley );
    aPlotter->SetPageSettings( plotPage );

    // Currently, plot units are in decimil
    aPlotter->SetViewport( wxPoint( 0, 0 ), IU_PER_MILS/10, scale, false );
}


void DIALOG_PLOT_SCHEMATIC::createPSFile( bool aPlotAll, bool aPlotFrameRef,
                                          RENDER_SETTINGS* aRenderSettings )
{
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();  // sheetpath is saved here
    PAGE_INFO       plotPage;                                    // page size selected to plot

    /* When printing all pages, the printed page is not the current page.
     * In complex hierarchies, we must update component references
     *  and others parameters in the given printed SCH_SCREEN, accordant to the sheet path
     *  because in complex hierarchies a SCH_SCREEN (a drawing )
     *  is shared between many sheets and component references depend on the actual sheet path used
     */
    SCH_SHEET_LIST  sheetList;

    if( aPlotAll )
    {
        sheetList.BuildSheetList( &m_parent->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_parent->GetCurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();

        SCH_SCREEN* screen = m_parent->GetCurrentSheet().LastScreen();
        PAGE_INFO   actualPage = screen->GetPageSettings();

        switch( m_pageSizeSelect )
        {
        case PAGE_SIZE_A:
            plotPage.SetType( wxT( "A" ) );
            plotPage.SetPortrait( actualPage.IsPortrait() );
            break;

        case PAGE_SIZE_A4:
            plotPage.SetType( wxT( "A4" ) );
            plotPage.SetPortrait( actualPage.IsPortrait() );
            break;

        case PAGE_SIZE_AUTO:
        default:
            plotPage = actualPage;
            break;
        }

        double  scalex  = (double) plotPage.GetWidthMils() / actualPage.GetWidthMils();
        double  scaley  = (double) plotPage.GetHeightMils() / actualPage.GetHeightMils();

        double  scale = std::min( scalex, scaley );

        wxPoint plot_offset;

        wxString msg;
        REPORTER& reporter = m_MessagesBox->Reporter();

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace ("\\", "_" );
            wxString ext = PS_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( !plotFileName.IsOk() )
                return;

            if( plotOneSheetPS( plotFileName.GetFullPath(), screen, aRenderSettings, plotPage,
                                plot_offset, scale, aPlotFrameRef ) )
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
            else
            {
                // Error
                msg.Printf( _( "Unable to create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }

        }
        catch( IO_ERROR& e )
        {
            msg.Printf( wxT( "PS Plotter exception: %s"), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
        }
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetPS( const wxString&     aFileName,
                                            SCH_SCREEN*         aScreen,
                                            RENDER_SETTINGS*    aRenderSettings,
                                            const PAGE_INFO&    aPageInfo,
                                            wxPoint             aPlot0ffset,
                                            double              aScale,
                                            bool                aPlotFrameRef )
{
    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( aPageInfo );
    plotter->SetColorMode( getModeColor() );

    // Currently, plot units are in decimil
    plotter->SetViewport( aPlot0ffset, IU_PER_MILS/10, aScale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-PS" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO toggle;       // Switch the locale to standard C

    plotter->StartPlot();

    if( m_plotBackgroundColor->GetValue() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
        wxPoint end( plotter->PageSettings().GetWidthIU(),
                     plotter->PageSettings().GetHeightIU() );
        plotter->Rect( wxPoint( 0, 0 ), end, FILL_TYPE::FILLED_SHAPE, 1.0 );
    }

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &aScreen->Schematic()->Prj(), m_parent->GetTitleBlock(),
                          aPageInfo, aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(),
                          plotter->GetColorMode() ?
                          plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) :
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}


void DIALOG_PLOT_SCHEMATIC::createSVGFile( bool aPrintAll, bool aPrintFrameRef,
                                           RENDER_SETTINGS* aRenderSettings )
{
    wxString        msg;
    REPORTER&       reporter = m_MessagesBox->Reporter();
    SCH_SHEET_PATH  oldsheetpath = m_parent->GetCurrentSheet();
    SCH_SHEET_LIST  sheetList;

    if( aPrintAll )
    {
        sheetList.BuildSheetList( &m_parent->Schematic().Root(), true );
        sheetList.SortByPageNumbers();
    }
    else
    {
        sheetList.push_back( m_parent->GetCurrentSheet() );
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        SCH_SCREEN*  screen;
        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
        m_parent->SetSheetNumberAndCount();
        screen = m_parent->GetCurrentSheet().LastScreen();

        try
        {
            wxString fname = m_parent->GetUniqueFilenameForCurrentSheet();

            // The sub sheet can be in a sub_hierarchy, but we plot the file in the
            // main project folder (or the folder specified by the caller),
            // so replace separators to create a unique filename:
            fname.Replace( "/", "_" );
            fname.Replace( "\\", "_" );
            wxString ext = SVG_PLOTTER::GetDefaultFileExtension();
            wxFileName plotFileName = createPlotFileName( fname, ext, &reporter );

            if( !plotFileName.IsOk() )
                return;

            bool success = plotOneSheetSVG( plotFileName.GetFullPath(), screen, aRenderSettings,
                                            getModeColor() ? false : true, aPrintFrameRef );

            if( !success )
            {
                msg.Printf( _( "Cannot create file \"%s\".\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ERROR );
            }
            else
            {
                msg.Printf( _( "Plot: \"%s\" OK.\n" ), plotFileName.GetFullPath() );
                reporter.Report( msg, RPT_SEVERITY_ACTION );
            }
        }
        catch( const IO_ERROR& e )
        {
            // Cannot plot SVG file
            msg.Printf( wxT( "SVG Plotter exception: %s" ), e.What() );
            reporter.Report( msg, RPT_SEVERITY_ERROR );
            break;
        }
    }

    m_parent->SetCurrentSheet( oldsheetpath );
    m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    m_parent->SetSheetNumberAndCount();
}


bool DIALOG_PLOT_SCHEMATIC::plotOneSheetSVG( const wxString&  aFileName,
                                             SCH_SCREEN*      aScreen,
                                             RENDER_SETTINGS* aRenderSettings,
                                             bool             aPlotBlackAndWhite,
                                             bool             aPlotFrameRef )
{
    const PAGE_INFO& pageInfo = aScreen->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetRenderSettings( aRenderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( aPlotBlackAndWhite ? false : true );
    wxPoint plot_offset;
    double scale = 1.0;

    // Currently, plot units are in decimil
    plotter->SetViewport( plot_offset, IU_PER_MILS/10, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( ! plotter->OpenFile( aFileName ) )
    {
        delete plotter;
        return false;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( m_plotBackgroundColor->GetValue() )
    {
        plotter->SetColor( plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
        wxPoint end( plotter->PageSettings().GetWidthIU(),
                     plotter->PageSettings().GetHeightIU() );
        plotter->Rect( wxPoint( 0, 0 ), end, FILL_TYPE::FILLED_SHAPE, 1.0 );
    }

    if( aPlotFrameRef )
    {
        PlotDrawingSheet( plotter, &aScreen->Schematic()->Prj(), m_parent->GetTitleBlock(),
                          pageInfo, aScreen->GetPageNumber(), aScreen->GetPageCount(),
                          m_parent->GetScreenDesc(), aScreen->GetFileName(),
                          plotter->GetColorMode() ?
                          plotter->RenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) :
                          COLOR4D::BLACK, aScreen->GetVirtualPageNumber() == 1 );
    }

    aScreen->Plot( plotter );

    plotter->EndPlot();
    delete plotter;

    return true;
}


wxString DIALOG_PLOT_SCHEMATIC::getOutputPath()
{
    wxString msg;
    wxString extMsg;
    wxFileName fn;

    extMsg.Printf( _( "Falling back to user path '%s'." ),
                   wxStandardPaths::Get().GetDocumentsDir() );

    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputDirectoryName->GetValue(), &Prj() );

    fn.SetPath( path );

    // If the contents of the path edit control results in an absolute path, return it as is.
    if( fn.IsAbsolute() )
        return path;

    // When editing a schematic that is not part of a project in the stand alone mode, the
    // project path is not defined.
    if( Prj().IsNullProject() )
    {
        SCH_SCREEN* screen = m_parent->Schematic().RootScreen();

        if( screen && !screen->GetFileName().IsEmpty() )
        {
            fn = screen->GetFileName();
            msg.Printf( _( "Cannot normalize path '%s%s'." ), fn.GetPathWithSep(), path );
            fn.SetPath( fn.GetPathWithSep() + path );

            // Normalize always returns true for a non-empty file name so clear the file name
            // and extension so that only the path is normalized.
            fn.SetName( wxEmptyString );
            fn.SetExt( wxEmptyString );

            if( fn.Normalize() )
            {
                path = fn.GetPath();
            }
            else
            {
                wxMessageDialog dlg( this, msg, _( "Warning" ),
                                     wxOK | wxCENTER | wxRESIZE_BORDER | wxICON_EXCLAMATION |
                                     wxSTAY_ON_TOP );

                dlg.SetExtendedMessage( extMsg );
                dlg.ShowModal();

                path = wxStandardPaths::Get().GetDocumentsDir();
            }
        }
        else
        {
            msg = _( "No project or path defined for the current schematic." );

            wxMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxOK | wxCENTER | wxRESIZE_BORDER | wxICON_EXCLAMATION |
                                 wxSTAY_ON_TOP );
            dlg.SetExtendedMessage( extMsg );
            dlg.ShowModal();

            // Always fall back to user's document path if no other absolute path can be normalized.
            path = wxStandardPaths::Get().GetDocumentsDir();
        }
    }
    else
    {
        msg.Printf( _( "Cannot normalize path '%s%s'." ), Prj().GetProjectPath(), path );

        // Build the absolute path of current output directory and the project path.
        fn.SetPath( Prj().GetProjectPath() + path );

        if( fn.Normalize() )
        {
            path = fn.GetPath();
        }
        else
        {
            wxMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxOK | wxCENTER | wxRESIZE_BORDER | wxICON_EXCLAMATION |
                                 wxSTAY_ON_TOP );

            dlg.SetExtendedMessage( extMsg );
            dlg.ShowModal();

            path = wxStandardPaths::Get().GetDocumentsDir();
        }
    }

    return path;
}
