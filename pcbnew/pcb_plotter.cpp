/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <pcb_plotter.h>
#include <plotters/plotter.h>
#include <plotters/plotters_pslike.h>
#include <board.h>
#include <reporter.h>
#include <pcbplot.h>
#include <wx/filename.h>
#include <gerber_jobfile_writer.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_plot.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_svg.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>
#include <math/util.h> // for KiROUND


static int scaleToSelection( double scale )
{
    int selection = 1;

    if( scale == 0.0 )      selection = 0;
    else if( scale == 1.5 ) selection = 2;
    else if( scale == 2.0 ) selection = 3;
    else if( scale == 3.0 ) selection = 4;

    return selection;
}


PCB_PLOTTER::PCB_PLOTTER( BOARD* aBoard, REPORTER* aReporter, PCB_PLOT_PARAMS& aParams ) :
        m_board( aBoard ),
        m_plotOpts( aParams ),
        m_reporter( aReporter )
{
}


bool PCB_PLOTTER::Plot( const wxString& aOutputPath, const LSEQ& aLayersToPlot,
                        const LSEQ& aCommonLayers, bool aUseGerberFileExtensions,
                        bool aOutputPathIsSingle, std::optional<wxString> aLayerName,
                        std::optional<wxString> aSheetName, std::optional<wxString> aSheetPath )
{
    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        // Handles board->GetTitleBlock() *and* board->GetProject()
        return m_board->ResolveTextVar( token, 0 );
    };

    // sanity, ensure one layer to print
    if( aLayersToPlot.size() < 1 )
    {
        m_reporter->Report( _( "No layers selected for plotting." ), RPT_SEVERITY_ERROR );
        return false;
    }

    PAGE_INFO existingPageInfo = m_board->GetPageSettings();
    VECTOR2I  existingAuxOrigin = m_board->GetDesignSettings().GetAuxOrigin();

    if( m_plotOpts.GetFormat() == PLOT_FORMAT::SVG && m_plotOpts.GetSvgFitPagetoBoard() ) // Page is board boundary size
    {
        BOX2I     bbox = m_board->ComputeBoundingBox( false );
        PAGE_INFO currPageInfo = m_board->GetPageSettings();

        currPageInfo.SetWidthMils( bbox.GetWidth() / pcbIUScale.IU_PER_MILS );
        currPageInfo.SetHeightMils( bbox.GetHeight() / pcbIUScale.IU_PER_MILS );

        m_board->SetPageSettings( currPageInfo );
        m_plotOpts.SetUseAuxOrigin( true );

        VECTOR2I origin = bbox.GetOrigin();
        m_board->GetDesignSettings().SetAuxOrigin( origin );
    }

    // To reuse logic, in single plot mode, we want to kick any extra layers from the main list to commonLayers
    LSEQ layersToPlot;
    LSEQ commonLayers;

    const bool isPdfMultiPage =
            ( m_plotOpts.GetFormat() == PLOT_FORMAT::PDF && m_plotOpts.m_PDFSingle );

    if( aOutputPathIsSingle && !m_plotOpts.GetDXFMultiLayeredExportOption() && !isPdfMultiPage )
    {
        layersToPlot.push_back( aLayersToPlot[0] );

        if( aLayersToPlot.size() > 1 )
            commonLayers.insert( commonLayers.end(), aLayersToPlot.begin() + 1, aLayersToPlot.end() );
    }
    else
    {
        layersToPlot = aLayersToPlot;
        commonLayers = aCommonLayers;
    }

    int finalPageCount = 0;
    std::vector<std::pair<PCB_LAYER_ID, wxString>> layersToExport;

    // Skip the disabled copper layers and build the layer ID -> layer name mapping for plotter
    // DXF plotter will use this information to name its layers
    for( PCB_LAYER_ID layer : layersToPlot )
    {
        if( copperLayerShouldBeSkipped( layer ) )
            continue;

        finalPageCount++;
        layersToExport.emplace_back( layer, m_board->GetLayerName( layer ) );
    }

    std::unique_ptr<GERBER_JOBFILE_WRITER> jobfile_writer;

    if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER && !aOutputPathIsSingle )
        jobfile_writer = std::make_unique<GERBER_JOBFILE_WRITER>( m_board, m_reporter );

    PLOT_FORMAT plot_format = m_plotOpts.GetFormat();
    wxString fileExt( GetDefaultPlotExtension( m_plotOpts.GetFormat() ) );
    wxString sheetPath;
    wxString msg;
    bool     success = true;
    PLOTTER* plotter = nullptr;
    int      pageNum = 1;

    for( size_t i = 0; i < layersToPlot.size(); i++ )
    {
        PCB_LAYER_ID layer = layersToPlot[i];

        if( copperLayerShouldBeSkipped( layer ) )
            continue;

        LSEQ       plotSequence = getPlotSequence( layer, commonLayers );
        wxString   layerName = m_board->GetLayerName( layer );
        wxFileName fn;

        if( aOutputPathIsSingle )
        {
            fn = wxFileName( aOutputPath );
        }
        else
        {
            wxFileName brdFn = m_board->GetFileName();
            fn.Assign( aOutputPath, brdFn.GetName(), fileExt );

            // Use Gerber Extensions based on layer number
            // (See http://en.wikipedia.org/wiki/Gerber_File)
            if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER && aUseGerberFileExtensions )
                fileExt = GetGerberProtelExtension( layer );

            if( plot_format == PLOT_FORMAT::PDF && m_plotOpts.m_PDFSingle )
                fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );
            else if ( plot_format == PLOT_FORMAT::DXF && m_plotOpts.GetDXFMultiLayeredExportOption() )
                fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::DXF ) );
            else
                BuildPlotFileName( &fn, aOutputPath, layerName, fileExt );
        }

        if( jobfile_writer )
        {
            wxString fullname = fn.GetFullName();
            jobfile_writer->AddGbrFile( layer, fullname );
        }

        if( ( plot_format != PLOT_FORMAT::PDF && plot_format != PLOT_FORMAT::DXF )
            || ( !m_plotOpts.m_PDFSingle && !m_plotOpts.GetDXFMultiLayeredExportOption() )
            || ( pageNum == 1
                 && ( ( plot_format == PLOT_FORMAT::PDF && m_plotOpts.m_PDFSingle )
                      || ( plot_format == PLOT_FORMAT::DXF && m_plotOpts.GetDXFMultiLayeredExportOption() ) ) ) )
        {
            // this will only be used by pdf
            wxString pageNumber = wxString::Format( "%d", pageNum );
            wxString pageName = layerName;
            wxString sheetName = layerName;

            if( aLayerName.has_value() )
            {
                layerName = aLayerName.value();
                pageName = aLayerName.value();
            }

            if( aSheetName.has_value() )
                sheetName = aSheetName.value();

            if( aSheetPath.has_value() )
                sheetPath = aSheetPath.value();

            m_plotOpts.SetLayersToExport( layersToExport );
            plotter = StartPlotBoard( m_board, &m_plotOpts, layer, layerName, fn.GetFullPath(),
                                      sheetName, sheetPath, pageName, pageNumber, finalPageCount );
        }

        if( plotter )
        {
            plotter->SetLayer( layer );
            plotter->SetTitle( ExpandTextVars( m_board->GetTitleBlock().GetTitle(), &textResolver ) );

            if( m_plotOpts.m_PDFMetadata )
            {
                msg = wxS( "AUTHOR" );

                if( m_board->ResolveTextVar( &msg, 0 ) )
                    plotter->SetAuthor( msg );

                msg = wxS( "SUBJECT" );

                if( m_board->ResolveTextVar( &msg, 0 ) )
                    plotter->SetSubject( msg );
            }

            try
            {
                PlotBoardLayers( m_board, plotter, plotSequence, m_plotOpts );
                PlotInteractiveLayer( m_board, plotter, m_plotOpts );
            }
            catch( ... )
            {
                success = false;
                break;
            }

            if( m_plotOpts.GetFormat() == PLOT_FORMAT::PDF
                    && m_plotOpts.m_PDFSingle
                    && i != layersToPlot.size() - 1 )
            {
                wxString     pageNumber = wxString::Format( "%d", pageNum + 1 );
                size_t       nextI = i + 1;
                PCB_LAYER_ID nextLayer = layersToPlot[nextI];

                while( copperLayerShouldBeSkipped( nextLayer ) && nextI < layersToPlot.size() - 1 )
                {
                    ++nextI;
                    nextLayer = layersToPlot[nextI];
                }

                layerName = m_board->GetLayerName( nextLayer );

                wxString pageName = layerName;
                wxString sheetName = layerName;

                static_cast<PDF_PLOTTER*>( plotter )->ClosePage();
                static_cast<PDF_PLOTTER*>( plotter )->StartPage( pageNumber, pageName );
                setupPlotterNewPDFPage( plotter, m_board, &m_plotOpts, layerName, sheetName,
                                        sheetPath, pageNumber, finalPageCount );
            }

            // last page
            if( (plot_format != PLOT_FORMAT::PDF && plot_format != PLOT_FORMAT::DXF)
                    || (!m_plotOpts.m_PDFSingle && !m_plotOpts.GetDXFMultiLayeredExportOption())
                    || i == aLayersToPlot.size() - 1
                    || pageNum == finalPageCount )
            {
                try
                {
                    plotter->EndPlot();
                }
                catch( ... )
                {
                    success = false;
                }

                delete plotter->RenderSettings();
                delete plotter;
                plotter = nullptr;

                msg.Printf( _( "Plotted to '%s'." ), fn.GetFullPath() );
                m_reporter->Report( msg, RPT_SEVERITY_ACTION );
            }
        }
        else
        {
            msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );

            success = false;
        }

        pageNum++;

        wxSafeYield(); // displays report message.
    }

    if( jobfile_writer && m_plotOpts.GetCreateGerberJobFile() )
    {
        // Pick the basename from the board file
        wxFileName fn( m_board->GetFileName() );

        // Build gerber job file from basename
        BuildPlotFileName( &fn, aOutputPath, wxT( "job" ), FILEEXT::GerberJobFileExtension );
        jobfile_writer->CreateJobFile( fn.GetFullPath() );
    }

    m_reporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );

    if( m_plotOpts.GetFormat() == PLOT_FORMAT::SVG && m_plotOpts.GetSvgFitPagetoBoard() )
    {
        // restore the original page and aux origin
        m_board->SetPageSettings( existingPageInfo );
        m_board->GetDesignSettings().SetAuxOrigin( existingAuxOrigin );
    }

    return success;
}


bool PCB_PLOTTER::copperLayerShouldBeSkipped( PCB_LAYER_ID aLayerToPlot )
{
    return ( LSET::AllCuMask() & ~m_board->GetEnabledLayers() )[aLayerToPlot];
}


void PCB_PLOTTER::BuildPlotFileName( wxFileName* aFilename, const wxString& aOutputDir,
                                     const wxString& aSuffix, const wxString& aExtension )
{
    // aFilename contains the base filename only (without path and extension)
    // when calling this function.
    // It is expected to be a valid filename (this is usually the board filename)
    aFilename->SetPath( aOutputDir );

    // Set the file extension
    aFilename->SetExt( aExtension );

    // remove leading and trailing spaces if any from the suffix, if
    // something survives add it to the name;
    // also the suffix can contain some not allowed chars in filename (/ \ . : and some others),
    // so change them to underscore
    // Remember it can be called from a python script, so the illegal chars
    // have to be filtered here.
    wxString suffix = aSuffix;
    suffix.Trim( true );
    suffix.Trim( false );

    wxString badchars = wxFileName::GetForbiddenChars( wxPATH_DOS );
    badchars.Append( "%." );

    for( unsigned ii = 0; ii < badchars.Len(); ii++ )
        suffix.Replace( badchars[ii], wxT( "_" ) );

    if( !suffix.IsEmpty() )
        aFilename->SetName( aFilename->GetName() + wxT( "-" ) + suffix );
}


LSEQ PCB_PLOTTER::getPlotSequence( PCB_LAYER_ID aLayerToPlot, LSEQ aPlotWithAllLayersSeq )
{
    LSEQ plotSequence;

    // Base layer always gets plotted first.
    plotSequence.push_back( aLayerToPlot );

    for( PCB_LAYER_ID layer : aPlotWithAllLayersSeq )
    {
        // Don't plot the same layer more than once;
        if( find( plotSequence.begin(), plotSequence.end(), layer ) != plotSequence.end() )
            continue;

        plotSequence.push_back( layer );
    }

    return plotSequence;
}


void PCB_PLOTTER::PlotJobToPlotOpts( PCB_PLOT_PARAMS& aOpts, JOB_EXPORT_PCB_PLOT* aJob,
                                     REPORTER& aReporter )
{
    if( aJob->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::GERBER )
    {
        JOB_EXPORT_PCB_GERBERS* gJob = static_cast<JOB_EXPORT_PCB_GERBERS*>( aJob );
        aOpts.SetDisableGerberMacros( gJob->m_disableApertureMacros );
        aOpts.SetUseGerberProtelExtensions( gJob->m_useProtelFileExtension );
        aOpts.SetUseGerberX2format( gJob->m_useX2Format );
        aOpts.SetIncludeGerberNetlistInfo( gJob->m_includeNetlistAttributes );
        aOpts.SetCreateGerberJobFile( gJob->m_createJobsFile );
        aOpts.SetGerberPrecision( gJob->m_precision );
        // Always disable plot pad holes
        aOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );
    }
    else
    {
        // Scale, doesn't apply to GERBER
        aOpts.SetScale( aJob->m_scale );
        aOpts.SetAutoScale( !aJob->m_scale );
        aOpts.SetScaleSelection( scaleToSelection( aJob->m_scale ) );
        // Drill marks doesn't apply to GERBER
        switch( aJob->m_drillShapeOption )
        {
        case DRILL_MARKS::NO_DRILL_SHAPE:    aOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );    break;
        case DRILL_MARKS::SMALL_DRILL_SHAPE: aOpts.SetDrillMarksType( DRILL_MARKS::SMALL_DRILL_SHAPE ); break;
        default:
        case DRILL_MARKS::FULL_DRILL_SHAPE:  aOpts.SetDrillMarksType( DRILL_MARKS::FULL_DRILL_SHAPE );  break;
        }
    }

    if( aJob->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG )
    {
        JOB_EXPORT_PCB_SVG* svgJob = static_cast<JOB_EXPORT_PCB_SVG*>( aJob );
        aOpts.SetSvgPrecision( svgJob->m_precision );
        aOpts.SetSvgFitPageToBoard( svgJob->m_fitPageToBoard );
    }

    if( aJob->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::DXF )
    {
        JOB_EXPORT_PCB_DXF* dxfJob = static_cast<JOB_EXPORT_PCB_DXF*>( aJob );
        aOpts.SetDXFPlotUnits( dxfJob->m_dxfUnits == JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH ? DXF_UNITS::INCH
                                                                                         : DXF_UNITS::MM );
        aOpts.SetDXFPlotMode( dxfJob->m_plotGraphicItemsUsingContours ? DXF_OUTLINE_MODE::SKETCH
                                                                      : DXF_OUTLINE_MODE::FILLED );
        aOpts.SetDXFPlotPolygonMode( dxfJob->m_polygonMode );
        aOpts.SetDXFMultiLayeredExportOption( dxfJob->m_genMode == JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE );
    }

    if( aJob->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF )
    {
        JOB_EXPORT_PCB_PDF* pdfJob = static_cast<JOB_EXPORT_PCB_PDF*>( aJob );
        aOpts.m_PDFFrontFPPropertyPopups = pdfJob->m_pdfFrontFPPropertyPopups;
        aOpts.m_PDFBackFPPropertyPopups = pdfJob->m_pdfBackFPPropertyPopups;
        aOpts.m_PDFMetadata = pdfJob->m_pdfMetadata;
        aOpts.m_PDFSingle = pdfJob->m_pdfSingle;
        aOpts.m_PDFBackgroundColor = COLOR4D( pdfJob->m_pdfBackgroundColor );
    }

    if( aJob->m_plotFormat == JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::POST )
    {
        JOB_EXPORT_PCB_PS* psJob = static_cast<JOB_EXPORT_PCB_PS*>( aJob );
        aOpts.SetWidthAdjust( KiROUND( psJob->m_trackWidthCorrection * pcbIUScale.IU_PER_MM ) );
        aOpts.SetFineScaleAdjustX( psJob->m_XScaleAdjust );
        aOpts.SetFineScaleAdjustY( psJob->m_YScaleAdjust );
        aOpts.SetA4Output( psJob->m_forceA4 );
    }

    aOpts.SetUseAuxOrigin( aJob->m_useDrillOrigin );
    aOpts.SetPlotFrameRef( aJob->m_plotDrawingSheet );
    aOpts.SetSubtractMaskFromSilk( aJob->m_subtractSolderMaskFromSilk );
    aOpts.SetPlotReference( aJob->m_plotRefDes );
    aOpts.SetPlotValue( aJob->m_plotFootprintValues );
    aOpts.SetSketchPadsOnFabLayers( aJob->m_sketchPadsOnFabLayers );
    aOpts.SetHideDNPFPsOnFabLayers( aJob->m_hideDNPFPsOnFabLayers );
    aOpts.SetSketchDNPFPsOnFabLayers( aJob->m_sketchDNPFPsOnFabLayers );
    aOpts.SetCrossoutDNPFPsOnFabLayers( aJob->m_crossoutDNPFPsOnFabLayers );
    aOpts.SetPlotPadNumbers( aJob->m_plotPadNumbers );

    aOpts.SetBlackAndWhite( aJob->m_blackAndWhite );
    aOpts.SetMirror( aJob->m_mirror );
    aOpts.SetNegative( aJob->m_negative );

    aOpts.SetLayerSelection( aJob->m_plotLayerSequence );
    aOpts.SetPlotOnAllLayersSequence( aJob->m_plotOnAllLayersSequence );

    switch( aJob->m_plotFormat )
    {
    default:
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::GERBER: aOpts.SetFormat( PLOT_FORMAT::GERBER ); break;
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::POST:   aOpts.SetFormat( PLOT_FORMAT::POST );   break;
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG:    aOpts.SetFormat( PLOT_FORMAT::SVG );    break;
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::DXF:    aOpts.SetFormat( PLOT_FORMAT::DXF );    break;
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::HPGL:   /* no longer supported */               break;
    case JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF:    aOpts.SetFormat( PLOT_FORMAT::PDF );    break;
    }

    wxString theme = aJob->m_colorTheme;

    // Theme may be empty when running from a job in GUI context, so use the GUI settings.
    if( theme.IsEmpty() )
    {
        if( PCBNEW_SETTINGS* pcbSettings = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
            theme = pcbSettings->m_ColorTheme;
    }

    COLOR_SETTINGS* colors = ::GetColorSettings( aJob->m_colorTheme );

    if( colors->GetFilename() != theme && !aOpts.GetBlackAndWhite() )
    {
        aReporter.Report( wxString::Format( _( "Color theme '%s' not found, will use theme from PCB Editor.\n" ),
                                            theme ),
                          RPT_SEVERITY_WARNING );
    }

    aOpts.SetColorSettings( colors );
    aOpts.SetOutputDirectory( aJob->GetConfiguredOutputPath() );
}
