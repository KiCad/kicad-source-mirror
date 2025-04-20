/*
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

#ifndef C_STRIPLINE_H_
#define C_STRIPLINE_H_


#include "transline/transline.h"
#include <transline_calculations/coupled_stripline.h>


class C_STRIPLINE : public TRANSLINE
{
public:
    C_STRIPLINE();

private:
    void calcAnalyze() override;
    void calcSynthesize() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void show_results() override;
    void getProperties() override;

    COUPLED_STRIPLINE m_calc;
};

#endif // C_STRIPLINE_H_
