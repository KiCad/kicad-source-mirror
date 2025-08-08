/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#pragma once

#include <dialog_render_job_base.h>
#include <jobs/job_pcb_render.h>

class DIALOG_RENDER_JOB : public DIALOG_RENDER_JOB_BASE
{
public:
    DIALOG_RENDER_JOB( wxWindow* aParent, JOB_PCB_RENDER* aJob );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    JOB_PCB_RENDER::FORMAT getSelectedFormat();
    void                   setSelectedFormat( JOB_PCB_RENDER::FORMAT aFormat );

    JOB_PCB_RENDER::SIDE getSelectedSide();
    void                 setSelectedSide( JOB_PCB_RENDER::SIDE aSide );

    JOB_PCB_RENDER::BG_STYLE getSelectedBgStyle();
    void                     setSelectedBgStyle( JOB_PCB_RENDER::BG_STYLE aBgStyle );

protected:
    JOB_PCB_RENDER* m_job;
};