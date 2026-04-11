/*
 * c_microstrip.h - coupled microstrip class definition
 *
 * Copyright (C) 2002 Claudio Girardi <in3otd@qsl.net>
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
 * along with this package.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _C_MICROSTRIP_H_
#define _C_MICROSTRIP_H_


#include "transline/transline.h"
#include <transline_calculations/coupled_microstrip.h>


class C_MICROSTRIP : public TRANSLINE
{
public:
    C_MICROSTRIP();

private:
    void calcAnalyze() override;
    void calcSynthesize() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void show_results() override;
    void getProperties() override;

    COUPLED_MICROSTRIP m_calc;
};

#endif // _C_MICROSTRIP_H_
