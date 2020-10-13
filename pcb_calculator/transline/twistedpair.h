/*
 * twistedpair.h - twisted pair class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
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


#ifndef __TWISTEDPAIR_H
#define __TWISTEDPAIR_H

#include "transline/transline.h"

class TWISTEDPAIR : public TRANSLINE
{
public:
    TWISTEDPAIR();

private:
    void calcAnalyze() override;
    void calcSynthesize() override;
    void showAnalyze() override;
    void showSynthesize() override;
    void show_results() override;
};

#endif
