/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_PCB_PDF_H
#define JOB_EXPORT_PCB_PDF_H

#include <layer_ids.h>
#include <wx/string.h>
#include "job.h"

class JOB_EXPORT_PCB_PDF : public JOB
{
public:
    JOB_EXPORT_PCB_PDF( bool aIsCli ) :
            JOB( "pdf", aIsCli ),
            m_filename(),
            m_outputFile(),
            m_colorTheme(),
            m_mirror( false ),
            m_blackAndWhite( false ),
            m_negative( false ),
            m_plotFootprintValues( true ),
            m_plotRefDes( true ),
            m_plotBorderTitleBlocks( false ),
            m_printMaskLayer()
    {
    }

    wxString m_filename;
    wxString m_outputFile;
    wxString m_colorTheme;

    bool m_mirror;
    bool m_blackAndWhite;
    bool m_negative;
    bool m_plotFootprintValues;
    bool m_plotRefDes;
    bool m_plotBorderTitleBlocks;

    LSET m_printMaskLayer;
};

#endif
