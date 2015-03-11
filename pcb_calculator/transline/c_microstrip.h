/*
 * c_microstrip.h - coupled microstrip class definition
 *
 * Copyright (C) 2002 Claudio Girardi <in3otd@qsl.net>
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modifications for Kicad: 2015 Jean-Pierre Charras
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


#ifndef _C_MICROSTRIP_H_
#define _C_MICROSTRIP_H_

class C_MICROSTRIP : public TRANSLINE
{
public: C_MICROSTRIP();
    ~C_MICROSTRIP();

private:
    double h;                   // height of substrate
    double ht;                  // height to the top of box
    double t;                   // thickness of top metal
    double rough;               // Roughness of top metal
    double w;                   // width of lines
    double w_t_e;               // even-mode thickness-corrected line width
    double w_t_o;               // odd-mode thickness-corrected line width
    double l;                   // length of lines
    double s;                   // spacing of lines
    double Z0_e_0;              // static even-mode impedance
    double Z0_o_0;              // static odd-mode impedance
    double Z0e;                 // even-mode impedance
    double Z0o;                 // odd-mode impedance
    double c_e;                 // even-mode capacitance
    double c_o;                 // odd-mode capacitance
    double ang_l_e;             // even-mode electrical length in angle
    double ang_l_o;             // odd-mode electrical length in angle
    double er_eff_e;            // even-mode effective dielectric constant
    double er_eff_o;            // odd-mode effective dielectric constant
    double er_eff_e_0;          // static even-mode effective dielectric constant
    double er_eff_o_0;          // static odd-mode effective dielectric constant
    double er_eff;              // FIXME: dummy
    double w_eff;               // Effective width of line
    double atten_dielectric_e;  // even-mode dielectric losses (dB)
    double atten_cond_e;        // even-mode conductors losses (dB)
    double atten_dielectric_o;  // odd-mode dielectric losses (dB)
    double atten_cond_o;        // odd-mode conductors losses (dB)

public:
    void   analyze();
    void   synthesize();

private:
    double delta_u_thickness_single( double, double );
    void   delta_u_thickness();
    void   compute_single_line();
    double filling_factor_even( double, double, double );
    double filling_factor_odd( double, double, double );
    double delta_q_cover_even( double );
    double delta_q_cover_odd( double );
    void   er_eff_static();
    double delta_Z0_even_cover( double, double, double );
    double delta_Z0_odd_cover( double, double, double );
    void   Z0_even_odd();
    void   er_eff_freq();
    void   conductor_losses();
    void   dielectric_losses();
    void   attenuation();
    void   line_angle();
    void   syn_err_fun( double*, double*, double, double, double, double, double );
    void   synth_width();
    void   Z0_dispersion();
    void   calc();
    void   get_c_microstrip_sub();
    void   get_c_microstrip_comp();
    void   get_c_microstrip_elec();
    void   get_c_microstrip_phys();
    void   show_results();
    void   syn_fun( double*, double*, double, double, double, double );


private:
    MICROSTRIP* aux_ms;
};

#endif      // _C_MICROSTRIP_H_
