/*
 * coplanar.h - microstrip class definition
 *
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2015 jean-pierre.charras
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

class COPLANAR : public TRANSLINE
{
public:
    COPLANAR();

public:
    void calcSynthesize() override;

protected:
    bool backMetal;
    double unit_prop_delay;

private:
    void calcAnalyze() override;
    void showSynthesize() override;
    void showAnalyze() override;
    void show_results() override;
};


class GROUNDEDCOPLANAR : public COPLANAR
{
public:
    GROUNDEDCOPLANAR();
};

#endif // __COPLANAR_H
