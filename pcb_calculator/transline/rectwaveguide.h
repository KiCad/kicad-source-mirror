/*
 * rectwaveguide.h - rectangular waveguide UI wrapper
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2015 jean-pierre.charras
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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __RECTWAVEGUIDE_H
#define __RECTWAVEGUIDE_H

#include "transline/transline.h"
#include <transline_calculations/rectwaveguide.h>


#define PHYS_A_PRM PHYS_WIDTH_PRM
#define PHYS_B_PRM PHYS_S_PRM


/**
 * UI wrapper for the rectangular waveguide calculator.  Shuttles parameters between the
 * legacy pcb_calculator UI array and the shared RECTWAVEGUIDE math core, and surfaces the
 * TE / TM mode strings the core produces as rows 4 and 5 of the results panel.
 */
class RECTWAVEGUIDE_UI : public TRANSLINE
{
public:
    RECTWAVEGUIDE_UI();

private:
    RECTWAVEGUIDE m_calc;

    void getProperties() override;
    void show_results() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void calcAnalyze() override;
    void calcSynthesize() override;
};

#endif // __RECTWAVEGUIDE_H
