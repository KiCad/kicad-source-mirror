/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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


PCB_PLOTTER::PCB_PLOTTER( BOARD* aBoard, REPORTER* aReporter, PCB_PLOT_PARAMS& aParams ) :
        m_board( aBoard ), m_plotOpts( aParams ), m_reporter( aReporter )
{
}


bool PCB_PLOTTER::Plot( const wxString& aOutputPath, const LSEQ& aLayersToPlot,
                        const LSEQ& aCommonLayers, bool aUseGerberFileExtensions )
{
    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        // Handles board->GetTitleBlock() *and* board->GetProject()
        return m_board->ResolveTextVar( token, 0 );
    };

    size_t finalPageCount = 0;
    for( size_t i = 0; i < aLayersToPlot.size(); i++ )
    {
        PCB_LAYER_ID layer = aLayersToPlot[i];
        if( copperLayerShouldBeSkipped( layer ) )
            continue;

        finalPageCount++;
    }

    std::unique_ptr<GERBER_JOBFILE_WRITER> jobfile_writer;

    if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER )
    {
        jobfile_writer = std::make_unique<GERBER_JOBFILE_WRITER>( m_board, m_reporter );
    }

    wxString fileExt( GetDefaultPlotExtension( m_plotOpts.GetFormat() ) );
    wxString sheetPath;
    PLOTTER* plotter = nullptr;
    for( size_t i = 0, pageNum = 1; i < aLayersToPlot.size(); i++ )
    {
        PCB_LAYER_ID layer = aLayersToPlot[i];

        if( copperLayerShouldBeSkipped( layer ) )
            continue;

        LSEQ plotSequence = getPlotSequence( layer, aCommonLayers );

        wxString layerName = m_board->GetLayerName( layer );

        wxFileName fn( aOutputPath );
        wxFileName brdFn = m_board->GetFileName();
        wxString   msg;
        fn.SetName( brdFn.GetName() );

        // Use Gerber Extensions based on layer number
        // (See http://en.wikipedia.org/wiki/Gerber_File)
        if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER && aUseGerberFileExtensions )
            fileExt = GetGerberProtelExtension( layer );

        if( m_plotOpts.GetFormat() == PLOT_FORMAT::PDF && m_plotOpts.m_PDFSingle )
        {
            fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );
        }
        else
        {
            BuildPlotFileName( &fn, aOutputPath, layerName, fileExt );
        }

        if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER )
        {
            wxString fullname = fn.GetFullName();
            jobfile_writer->AddGbrFile( layer, fullname );
        }

        if( m_plotOpts.GetFormat() != PLOT_FORMAT::PDF || !m_plotOpts.m_PDFSingle
            || ( i == 0 && m_plotOpts.GetFormat() == PLOT_FORMAT::PDF
                    && m_plotOpts.m_PDFSingle ) )
        {
            // this will only be used by pdf
            wxString pageNumber = wxString::Format( "%zu", pageNum );
            wxString pageName = layerName;
            wxString sheetName = layerName;

            plotter = StartPlotBoard( m_board, &m_plotOpts, layer, layerName, fn.GetFullPath(),
                                      sheetName,
                                      sheetPath, pageName, pageNumber, finalPageCount );
        }

        if( plotter )
        {
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

            PlotBoardLayers( m_board, plotter, plotSequence, m_plotOpts );
            PlotInteractiveLayer( m_board, plotter, m_plotOpts );


            if( m_plotOpts.GetFormat() == PLOT_FORMAT::PDF && m_plotOpts.m_PDFSingle
                && i != aLayersToPlot.size() - 1 )
            {
                wxString     pageNumber = wxString::Format( "%zu", pageNum + 1 );
                PCB_LAYER_ID nextLayer = aLayersToPlot[i + 1];
                wxString     pageName = m_board->GetLayerName( nextLayer );
                wxString     sheetName = layerName;

                static_cast<PDF_PLOTTER*>( plotter )->ClosePage();
                static_cast<PDF_PLOTTER*>( plotter )->StartPage( pageNumber, pageName );
                setupPlotterNewPDFPage( plotter, m_board, &m_plotOpts, sheetName, sheetPath,
                                        pageNumber,
                                        finalPageCount );
            }


            // last page
            if( m_plotOpts.GetFormat() != PLOT_FORMAT::PDF || !m_plotOpts.m_PDFSingle
                || i == aLayersToPlot.size() - 1 )
            {
                plotter->EndPlot();
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

            if( m_plotOpts.m_PDFSingle )
            {
                return false;
            }
        }

        pageNum++;

        wxSafeYield(); // displays report message.
    }

    if( m_plotOpts.GetFormat() == PLOT_FORMAT::GERBER && m_plotOpts.GetCreateGerberJobFile() )
    {
        // Pick the basename from the board file
        wxFileName fn( m_board->GetFileName() );

        // Build gerber job file from basename
        BuildPlotFileName( &fn, aOutputPath, wxT( "job" ),
                           FILEEXT::GerberJobFileExtension );
        jobfile_writer->CreateJobFile( fn.GetFullPath() );
    }

    m_reporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );

    return true;
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

    for( size_t i = 0; i < aPlotWithAllLayersSeq.size(); i++ )
    {
        PCB_LAYER_ID layer = aPlotWithAllLayersSeq[i];

        // Don't plot the same layer more than once;
        if( find( plotSequence.begin(), plotSequence.end(), layer ) != plotSequence.end() )
            continue;

        plotSequence.push_back( layer );
    }

    return plotSequence;
}