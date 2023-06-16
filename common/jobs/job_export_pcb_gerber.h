/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_PCB_GERBER_H
#define JOB_EXPORT_PCB_GERBER_H

#include <layer_ids.h>
#include <wx/string.h>
#include "job.h"

class JOB_EXPORT_PCB_GERBER : public JOB
{
public:
    JOB_EXPORT_PCB_GERBER( const std::string& aType, bool aIsCli ) :
            JOB( aType, aIsCli ),
            m_filename(),
            m_outputFile(),
            m_plotFootprintValues( true ),
            m_plotRefDes( true ),
            m_plotBorderTitleBlocks( false ),
            m_subtractSolderMaskFromSilk( false ),
            m_includeNetlistAttributes( true ),
            m_useX2Format( true ),
            m_disableApertureMacros( false ),
            m_useAuxOrigin( false ),
            m_useProtelFileExtension( true ),
            m_precision( 5 ),
            m_printMaskLayer()
    {
    }

    JOB_EXPORT_PCB_GERBER( bool aIsCli ) :
            JOB_EXPORT_PCB_GERBER( "gerber", aIsCli )
    {
    }

    wxString m_filename;
    wxString m_outputFile;

    bool m_plotFootprintValues;
    bool m_plotRefDes;
    bool m_plotBorderTitleBlocks;
    bool m_subtractSolderMaskFromSilk;
    bool m_includeNetlistAttributes;
    bool m_useX2Format;
    bool m_disableApertureMacros;
    bool m_useAuxOrigin;
    bool m_useProtelFileExtension;

    int m_precision;


    LSET m_printMaskLayer;
};

#endif
