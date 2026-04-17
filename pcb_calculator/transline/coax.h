/*
 * coax.h - coaxial class definition
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modifications for Kicad: 2015 Jean-Pierre Charras
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


#ifndef __COAX_H
#define __COAX_H

#include "transline/transline.h"
#include <transline_calculations/coax.h>


class COAX_UI : public TRANSLINE
{
public:
    COAX_UI();

private:
    COAX m_calc;

    void getProperties() override;
    void show_results() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void calcAnalyze() override;
    void calcSynthesize() override;
};

#endif // __COAX_H
