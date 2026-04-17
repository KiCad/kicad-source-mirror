/*
 * coplanar.h - coplanar class definition
 *
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
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


#ifndef __COPLANAR_H
#define __COPLANAR_H

#include "transline/transline.h"
#include <transline_calculations/coplanar.h>


/**
 * UI wrapper for an ungrounded coplanar waveguide.  The math lives in the shared
 * ::COPLANAR class; this class only shuttles parameters between the legacy UI array
 * and the calculator.
 */
class COPLANAR_UI : public TRANSLINE
{
public:
    COPLANAR_UI();

protected:
    /// Pushes back-metal flag and common UI parameters into the calculator
    void getProperties() override;

    COPLANAR m_calc;

private:
    void show_results() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void calcAnalyze() override;
    void calcSynthesize() override;
};


/**
 * UI wrapper for a conductor-backed coplanar waveguide.  Differs from COPLANAR_UI only in
 * setting the CPW_BACKMETAL parameter to 1.0 so the shared math core picks the grounded
 * branch of the conformal-mapping solution.
 */
class GROUNDEDCOPLANAR_UI : public COPLANAR_UI
{
public:
    GROUNDEDCOPLANAR_UI();

protected:
    void getProperties() override;
};

#endif // __COPLANAR_H
