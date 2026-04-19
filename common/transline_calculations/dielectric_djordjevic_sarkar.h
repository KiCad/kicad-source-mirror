/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef TRANSLINE_CALCULATIONS_DIELECTRIC_DJORDJEVIC_SARKAR_H
#define TRANSLINE_CALCULATIONS_DIELECTRIC_DJORDJEVIC_SARKAR_H

#include <complex>


/**
 * Kramers-Kronig-consistent wideband dielectric model after Djordjevic et al.
 * ("Wideband frequency-domain characterization of FR-4 and time-domain
 * causality", IEEE Trans. EMC 43(4), 2001).  Equivalent closed form to the
 * Svensson-Djordjevic expression used by scikit-rf
 * (skrf/media/definedAEpTandZ0.py : djordjevicsvensson).
 *
 * The caller supplies a single measurement triple (epsR, tan delta, f_spec)
 * and a bandwidth [f1, f2] over which the loss tangent is approximately
 * constant.  Fit() computes the two model parameters (eps_inf, m) once;
 * subsequent queries are a single complex-log evaluation with no iteration.
 */
class DIELECTRIC_DJORDJEVIC_SARKAR
{
public:
    /**
     * Fit the model from a single (epsR, tan delta) datapoint at f_spec.
     * The default bandwidth [1 kHz, 1 THz] matches Djordjevic 2001.
     *
     * @throws std::invalid_argument if f1 <= 0, f2 <= f1, or f_spec is
     *         outside [f1, f2].
     */
    void Fit( double aEpsRSpec, double aTanDSpec, double aFSpec, double aF1 = 1.0e3,
              double aF2 = 1.0e12 );

    /// Complex relative permittivity at aF.  Imag part is non-positive (loss).
    std::complex<double> ComplexEpsilonAt( double aF ) const;

    /// Real part of relative permittivity at aF.
    double EpsilonRealAt( double aF ) const;

    /// Loss tangent tan delta = -Im(eps) / Re(eps) at aF.
    double TanDeltaAt( double aF ) const;

    double GetEpsilonInf() const { return m_epsInf; }
    double GetM() const { return m_m; }
    bool   IsLossless() const { return m_lossless; }

private:
    double m_epsInf   = 1.0;
    double m_m        = 0.0;
    double m_f1       = 1.0e3;
    double m_f2       = 1.0e12;

    // Avoids a complex-log evaluation on every query when tan delta is zero.
    bool   m_lossless = true;
};

#endif
