/*
 * microstrip.h - microstrip class definition
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
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


#ifndef __MICROSTRIP_H
#define __MICROSTRIP_H

class MICROSTRIP : public TRANSLINE
{
public: MICROSTRIP();

    friend class C_MICROSTRIP;

private:
    double h;                   // height of substrate
    double ht;                  // height to the top of box
    double t;                   // thickness of top metal
    double rough;               // Roughness of top metal
    double mur;                 // magnetic permeability of substrate
    double w;                   // width of line
    double l;                   // length of line
    double Z0_0;                // static characteristic impedance
    double Z0;                  // characteristic impedance
    double ang_l;               // Electrical length in angle
    double er_eff_0;            // Static effective dielectric constant
    double er_eff;              // Effective dielectric constant
    double mur_eff;             // Effective mag. permeability
    double w_eff;               // Effective width of line
    double atten_dielectric;    // Loss in dielectric (dB)
    double atten_cond;          // Loss in conductors (dB)

    // private params
    double Z0_h_1;      // homogeneous stripline impedance

public:
    void   analyze();
    void   synthesize();

private:
    double er_eff_freq();
    double alpha_c();
    double alpha_c_roughness();
    double alpha_dielectric();
    double char_impedance_ht();
    double synth_width();
    double ereff_dispersion();
    double Z0_dispersion();
    double Z0_homogeneous( double );
    double delta_Z0_cover( double, double );
    double filling_factor( double, double );
    double delta_q_cover( double );
    double delta_q_thickness( double, double );
    double e_r_effective( double, double );
    double delta_u_thickness( double, double, double );
    double e_r_dispersion( double, double, double );
    double Z0_dispersion( double, double, double, double, double );
    double conductor_losses();
    double dielectric_losses();
    void   microstrip_Z0();
    void   dispersion();
    void   attenuation();
    void   mur_eff_ms();
    void   line_angle();
    void   calc();
    void   get_microstrip_sub();
    void   get_microstrip_comp();
    void   get_microstrip_elec();
    void   get_microstrip_phys();
    void   show_results();
};

#endif      // __MICROSTRIP_H
