/*
 * stripline.h - stripline class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2011 for Kicad: Jean-Pierre Charras
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

#ifndef __STRIPLINE_H
#define __STRIPLINE_H

class STRIPLINE : public TRANSLINE
{
public: STRIPLINE();

private:
    double h;                   // height of substrate
    double a;                   // distance of strip to top metal
    double t;                   // thickness of top metal
    double w;                   // width of line
    double len;                 // length of line
    double Z0;                  // characteristic impedance
    double ang_l;               // electrical length in angle
    double er_eff;              // effective dielectric constant
    double atten_dielectric;    // loss in dielectric (dB)
    double atten_cond;          // loss in conductors (dB)

public:
    void   analyze() override;
    void   synthesize() override;

private:
    double lineImpedance( double, double& );
    void   calc();
    void   show_results();
    void   getProperties();
};

#endif
