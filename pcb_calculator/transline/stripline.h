/*
 * stripline.h - stripline class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2011 for Kicad: Jean-Pierre Charras
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
 *
 */

#ifndef STRIPLINE_UI_H
#define STRIPLINE_UI_H


#include "transline/transline.h"
#include <transline_calculations/stripline.h>

class STRIPLINE_UI : public TRANSLINE
{
public:
    STRIPLINE_UI();

private:
    void calcAnalyze() override;
    void calcSynthesize() override;
    void showSynthesize() override;
    void showAnalyze() override;
    void show_results() override;
    void getProperties() override;

    STRIPLINE m_calc;
};

#endif //STRIPLINE_UI_H
