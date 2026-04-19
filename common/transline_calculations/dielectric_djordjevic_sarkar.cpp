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

#include <transline_calculations/dielectric_djordjevic_sarkar.h>

#include <stdexcept>


void DIELECTRIC_DJORDJEVIC_SARKAR::Fit( double aEpsRSpec, double aTanDSpec, double aFSpec,
                                        double aF1, double aF2 )
{
    if( aF1 <= 0.0 )
        throw std::invalid_argument( "DIELECTRIC_DJORDJEVIC_SARKAR: f1 must be positive" );

    if( aF2 <= aF1 )
        throw std::invalid_argument( "DIELECTRIC_DJORDJEVIC_SARKAR: f2 must exceed f1" );

    if( aFSpec < aF1 || aFSpec > aF2 )
        throw std::invalid_argument( "DIELECTRIC_DJORDJEVIC_SARKAR: f_spec outside [f1, f2]" );

    m_f1 = aF1;
    m_f2 = aF2;

    // Short-circuit makes TanDeltaAt return an exact 0.0 and skips the complex-log below.
    if( aTanDSpec == 0.0 )
    {
        m_lossless = true;
        m_epsInf   = aEpsRSpec;
        m_m        = 0.0;
        return;
    }

    // Djordjevic et al., IEEE Trans. EMC 43(4):662-667, Nov. 2001, eqs. (9)-(11).
    // Equivalent form in Svensson and Dermer, IEEE Trans. Adv. Packag. 24(2):191-196, May 2001.
    // Mirrored by scikit-rf skrf/media/definedAEpTandZ0.py (djordjevicsvensson).
    const std::complex<double> j{ 0.0, 1.0 };
    const std::complex<double> k = std::log( ( aF2 + j * aFSpec ) / ( aF1 + j * aFSpec ) );

    m_m        = -aTanDSpec * aEpsRSpec / k.imag();
    m_epsInf   = aEpsRSpec * ( 1.0 + aTanDSpec * k.real() / k.imag() );
    m_lossless = false;
}


std::complex<double> DIELECTRIC_DJORDJEVIC_SARKAR::ComplexEpsilonAt( double aF ) const
{
    if( m_lossless )
        return { m_epsInf, 0.0 };

    // Djordjevic et al. 2001, eq. (8).  Kramers-Kronig consistent by construction.
    const std::complex<double> j{ 0.0, 1.0 };

    return m_epsInf + m_m * std::log( ( m_f2 + j * aF ) / ( m_f1 + j * aF ) );
}


double DIELECTRIC_DJORDJEVIC_SARKAR::EpsilonRealAt( double aF ) const
{
    return ComplexEpsilonAt( aF ).real();
}


double DIELECTRIC_DJORDJEVIC_SARKAR::TanDeltaAt( double aF ) const
{
    if( m_lossless )
        return 0.0;

    const std::complex<double> eps = ComplexEpsilonAt( aF );

    return -eps.imag() / eps.real();
}
