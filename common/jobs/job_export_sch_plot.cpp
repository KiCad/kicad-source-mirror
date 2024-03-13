/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_sch_plot.h>


JOB_EXPORT_SCH_PLOT::JOB_EXPORT_SCH_PLOT( bool aIsCli, SCH_PLOT_FORMAT aPlotFormat, wxString aFilename ) :
        JOB( "plot", aIsCli ),
        m_plotFormat( aPlotFormat ),
        m_filename( aFilename ),
        m_drawingSheet(),
        m_plotAll( true ),
        m_plotDrawingSheet( true ),
        m_blackAndWhite( false ),
        m_pageSizeSelect( JOB_PAGE_SIZE::PAGE_SIZE_AUTO ),
        m_useBackgroundColor( true ),
        m_HPGLPenSize( 1.0 ),
        m_HPGLPaperSizeSelect( JOB_HPGL_PAGE_SIZE::DEFAULT ),
        m_PDFPropertyPopups( true ),
        m_PDFMetadata( true ),
        m_theme(),
        m_outputDirectory(),
        m_outputFile(),
        m_HPGLPlotOrigin( JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT )
{
}