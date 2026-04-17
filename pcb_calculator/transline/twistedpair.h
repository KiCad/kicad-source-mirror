/*
 * twistedpair.h - twisted pair UI wrapper
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
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


#ifndef __TWISTEDPAIR_H
#define __TWISTEDPAIR_H

#include "transline/transline.h"
#include <transline_calculations/twistedpair.h>


/**
 * UI wrapper for the twisted-pair calculator.  Shuttles parameters between the legacy
 * pcb_calculator UI array and the shared TWISTEDPAIR math core, and surfaces the εeff /
 * loss / skin-depth results in the pcb_calculator result panel rows.
 */
class TWISTEDPAIR_UI : public TRANSLINE
{
public:
    TWISTEDPAIR_UI();

private:
    TWISTEDPAIR m_calc;

    void getProperties() override;
    void show_results() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void calcAnalyze() override;
    void calcSynthesize() override;
};

#endif // __TWISTEDPAIR_H
