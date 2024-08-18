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

#include <jobs/job_export_pcb_pdf.h>


JOB_EXPORT_PCB_PDF::JOB_EXPORT_PCB_PDF( bool aIsCli ) :
        JOB( "pdf", aIsCli ),
        m_filename(),
        m_outputFile(),
        m_colorTheme(),
        m_drawingSheet(),
        m_mirror( false ),
        m_blackAndWhite( false ),
        m_negative( false ),
        m_plotFootprintValues( true ),
        m_plotRefDes( true ),
        m_plotBorderTitleBlocks( false ),
        m_printMaskLayer(),
        m_sketchPadsOnFabLayers( false ),
        m_hideDNPFPsOnFabLayers( false ),
        m_sketchDNPFPsOnFabLayers( true ),
        m_crossoutDNPFPsOnFabLayers( true ),
        m_drillShapeOption( 2 )
{
}