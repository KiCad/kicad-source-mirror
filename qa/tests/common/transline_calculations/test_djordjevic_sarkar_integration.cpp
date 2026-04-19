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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cmath>

#include <transline_calculations/coax.h>
#include <transline_calculations/microstrip.h>
#include <transline_calculations/rectwaveguide.h>
#include <transline_calculations/transline_calculation_base.h>


using TCP = TRANSLINE_PARAMETERS;


BOOST_AUTO_TEST_SUITE( DjordjevicSarkarIntegration )


// Populate a realistic 50 ohm FR-4 microstrip driven at 10 GHz.  W / h / T are chosen
// so the static impedance lands near 50 ohm (typical 8 mil trace on 8 mil FR-4 core
// with 1 oz copper).  Loss tangent 0.02 at the 1 GHz spec frequency.
static void configureFR4Microstrip( MICROSTRIP& aCalc )
{
    aCalc.SetParameter( TCP::EPSILONR, 4.4 );
    aCalc.SetParameter( TCP::TAND, 0.02 );
    aCalc.SetParameter( TCP::H, 200.0e-6 );          // 200 um substrate
    aCalc.SetParameter( TCP::H_T, 1.0e12 );          // effectively unbounded cover
    aCalc.SetParameter( TCP::T, 35.0e-6 );           // 1 oz copper
    aCalc.SetParameter( TCP::PHYS_WIDTH, 380.0e-6 ); // ~50 ohm trace on 200 um FR-4
    aCalc.SetParameter( TCP::PHYS_LEN, 25.4e-3 );    // 1 inch
    aCalc.SetParameter( TCP::FREQUENCY, 10.0e9 );    // 10 GHz operating
    aCalc.SetParameter( TCP::MUR, 1.0 );
    aCalc.SetParameter( TCP::MURC, 1.0 );
    aCalc.SetParameter( TCP::SIGMA, 5.8e7 );         // copper
    aCalc.SetParameter( TCP::ROUGH, 0.0 );
}


// FR-4 microstrip at 10 GHz.  CONSTANT uses epsR=4.4/tan d=0.02 everywhere.  DJORDJEVIC_SARKAR
// fit at 1 GHz yields epsR(10 GHz)~4.27 and tan d(10 GHz)~0.0205, so Z0 shifts by a small but
// observable amount (positive, because lower epsR at operating frequency raises Z0).
BOOST_AUTO_TEST_CASE( MicrostripFR4Z0Shift )
{
    MICROSTRIP calc;
    configureFR4Microstrip( calc );

    // CONSTANT mode baseline
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::CONSTANT ) );
    calc.Analyse();
    const double z0_const = calc.GetParameter( TCP::Z0 );

    // DJORDJEVIC_SARKAR mode
    calc.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );
    calc.Analyse();
    const double z0_ds = calc.GetParameter( TCP::Z0 );

    BOOST_TEST_MESSAGE( "FR-4 microstrip 10 GHz: Z0_const=" << z0_const << " ohm, Z0_ds="
                                                             << z0_ds << " ohm" );

    const double shift = std::fabs( z0_ds - z0_const );
    BOOST_TEST( shift > 0.3 );
    BOOST_TEST( shift < 3.0 );

    // Raw EPSILONR / TAND must survive the swap-and-restore in MICROSTRIP::Analyse
    BOOST_TEST( calc.GetParameter( TCP::EPSILONR ) == 4.4 );
    BOOST_TEST( calc.GetParameter( TCP::TAND ) == 0.02 );
}


// FR-4 microstrip dielectric attenuation scales linearly with tan delta.  At 10 GHz the DS
// model raises tan d from 0.02 (spec @ 1 GHz) to ~0.020486, so alpha_d ratio should be
// ~1.0243 within ~1%.
BOOST_AUTO_TEST_CASE( MicrostripFR4DielectricLossShift )
{
    MICROSTRIP calc;
    configureFR4Microstrip( calc );

    // CONSTANT baseline alpha_d
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::CONSTANT ) );
    calc.Analyse();
    const double atten_d_const = calc.GetParameter( TCP::ATTEN_DILECTRIC );

    // DS mode alpha_d
    calc.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );
    calc.Analyse();
    const double atten_d_ds = calc.GetParameter( TCP::ATTEN_DILECTRIC );

    BOOST_TEST_MESSAGE( "FR-4 microstrip 10 GHz: atten_d_const=" << atten_d_const
                                                                  << " dB, atten_d_ds="
                                                                  << atten_d_ds << " dB" );

    BOOST_TEST( atten_d_const > 0.0 );
    BOOST_TEST( atten_d_ds > 0.0 );

    const double ratio = atten_d_ds / atten_d_const;

    // DS raises tan d at 10 GHz (0.02 -> ~0.02049) but simultaneously lowers e_r (4.4 -> ~4.27).
    // The Hammerstad / Kirschning-Jansen microstrip dielectric-loss formula combines both through
    // e_r * (e_r_eff - 1) / ((e_r - 1) * sqrt(e_r_eff)), which partially compensates the tan-delta
    // bump.  Assert a small but clearly positive ratio (observed ~1.008 vs 1.000 CONSTANT) and
    // bound it below the naive tan-delta-only ratio of 1.0243 that would apply to coax.
    BOOST_TEST( ratio > 1.005 );
    BOOST_TEST( ratio < 1.030 );
}


// Coax LOSS_DIELECTRIC is proportional to sqrt(epsR) * tan d.  DS shifts both, so the loss
// ratio between CONSTANT and DS at 10 GHz is sqrt(4.2709 / 4.4) * (0.020486 / 0.02) ~ 1.0094.
BOOST_AUTO_TEST_CASE( CoaxFR4DispersionShift )
{
    COAX calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 0.5e-3 );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 1.8e-3 );
    calc.SetParameter( TCP::PHYS_LEN, 100.0e-3 );
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );

    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::CONSTANT ) );
    calc.Analyse();
    const double z0_const = calc.GetParameter( TCP::Z0 );
    const double loss_d_const = calc.GetParameter( TCP::LOSS_DIELECTRIC );

    calc.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );
    calc.Analyse();
    const double z0_ds = calc.GetParameter( TCP::Z0 );
    const double loss_d_ds = calc.GetParameter( TCP::LOSS_DIELECTRIC );

    BOOST_TEST_MESSAGE( "Coax 10 GHz: Z0_const=" << z0_const << " Z0_ds=" << z0_ds
                                                  << " loss_d_const=" << loss_d_const
                                                  << " loss_d_ds=" << loss_d_ds );

    // Z0 rises because eps_r drops at 10 GHz.  Z0 = ZF0 / (2 pi sqrt(eps_r)) * ln(Dout/Din).
    BOOST_TEST( z0_ds > z0_const );
    const double z0_ratio_expected = std::sqrt( 4.4 / 4.270924 );
    BOOST_TEST( z0_ds / z0_const == z0_ratio_expected, boost::test_tools::tolerance( 0.002 ) );

    // Loss is proportional to sqrt(eps_r) * tan d.  Expected ratio 1.0094.
    BOOST_TEST( loss_d_ds > loss_d_const );
    const double loss_ratio_expected = std::sqrt( 4.270924 / 4.4 ) * ( 0.020486 / 0.02 );
    BOOST_TEST( loss_d_ds / loss_d_const == loss_ratio_expected,
                boost::test_tools::tolerance( 0.005 ) );

    // Raw parameters must survive the swap-and-restore
    BOOST_TEST( calc.GetParameter( TCP::EPSILONR ) == 4.4 );
    BOOST_TEST( calc.GetParameter( TCP::TAND ) == 0.02 );
}


// Synthesize must size the cable against the dispersed permittivity, not eps_r_spec.
// At 10 GHz on FR-4 spec'd at 1 GHz, eps_r drops from 4.4 to ~4.2709, which shrinks the
// diameter ratio needed to hit a given Z0.  Verify the DS-mode synth yields a smaller
// outer diameter than the CONSTANT-mode synth for identical targets.
BOOST_AUTO_TEST_CASE( CoaxFR4SynthesizeDispersion )
{
    auto configure = []( COAX& calc )
    {
        calc.SetParameter( TCP::EPSILONR, 4.4 );
        calc.SetParameter( TCP::TAND, 0.02 );
        calc.SetParameter( TCP::MUR, 1.0 );
        calc.SetParameter( TCP::MURC, 1.0 );
        calc.SetParameter( TCP::SIGMA, 5.8e7 );
        calc.SetParameter( TCP::PHYS_DIAM_IN, 0.5e-3 );
        calc.SetParameter( TCP::Z0, 50.0 );
        calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
        calc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    };

    COAX calcConst;
    configure( calcConst );
    calcConst.SetSynthesizeTarget( TCP::PHYS_DIAM_OUT );
    calcConst.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                            static_cast<double>( DIELECTRIC_MODEL::CONSTANT ) );
    BOOST_TEST( calcConst.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );
    const double dOut_const = calcConst.GetParameter( TCP::PHYS_DIAM_OUT );
    const double len_const = calcConst.GetParameter( TCP::PHYS_LEN );

    COAX calcDs;
    configure( calcDs );
    calcDs.SetSynthesizeTarget( TCP::PHYS_DIAM_OUT );
    calcDs.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calcDs.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                         static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );
    BOOST_TEST( calcDs.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );
    const double dOut_ds = calcDs.GetParameter( TCP::PHYS_DIAM_OUT );
    const double len_ds = calcDs.GetParameter( TCP::PHYS_LEN );

    BOOST_TEST_MESSAGE( "Coax synth 10 GHz: Dout_const=" << dOut_const
                                                         << " Dout_ds=" << dOut_ds
                                                         << " len_const=" << len_const
                                                         << " len_ds=" << len_ds );

    // Dout = Din * exp(Z0 * sqrt(eps_r) / ZF0 * 2 pi).  Lower eps_r at 10 GHz shrinks
    // the exponent, so DS-mode Dout sits below the CONSTANT-mode Dout for identical
    // targets (Din, Z0, f).
    BOOST_TEST( dOut_ds < dOut_const );
    // Lower eps_r raises phase velocity => longer guided wavelength => longer physical
    // length for the same ANG_L.
    BOOST_TEST( len_ds > len_const );

    // Raw EPSILONR / TAND must survive the synth pipeline.
    BOOST_TEST( calcDs.GetParameter( TCP::EPSILONR ) == 4.4 );
    BOOST_TEST( calcDs.GetParameter( TCP::TAND ) == 0.02 );
}


// Rectangular waveguide synthesis must also use dispersed eps_r.  At low dielectric
// fill (eps_r ~ 1.0006 for PTFE foam) this matters little, but for an eps_r=4.4 FR-4
// fill the dispersion shift is comparable to the microstrip case.  Verify the synthesized
// broad-wall width differs between CONSTANT and DS modes and that raw params are restored.
BOOST_AUTO_TEST_CASE( RectWaveguideFR4SynthesizeDispersion )
{
    auto configure = []( RECTWAVEGUIDE& calc )
    {
        calc.SetParameter( TCP::EPSILONR, 4.4 );
        calc.SetParameter( TCP::TAND, 0.02 );
        calc.SetParameter( TCP::MUR, 1.0 );
        calc.SetParameter( TCP::MURC, 1.0 );
        calc.SetParameter( TCP::SIGMA, 5.8e7 );
        calc.SetParameter( TCP::PHYS_S, 5.0e-3 );
        calc.SetParameter( TCP::Z0, 400.0 );
        calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
        calc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    };

    RECTWAVEGUIDE calcConst;
    configure( calcConst );
    calcConst.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                            static_cast<double>( DIELECTRIC_MODEL::CONSTANT ) );
    BOOST_TEST( calcConst.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );
    const double a_const = calcConst.GetParameter( TCP::PHYS_WIDTH );
    const double len_const = calcConst.GetParameter( TCP::PHYS_LEN );

    RECTWAVEGUIDE calcDs;
    configure( calcDs );
    calcDs.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calcDs.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                         static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );
    BOOST_TEST( calcDs.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );
    const double a_ds = calcDs.GetParameter( TCP::PHYS_WIDTH );
    const double len_ds = calcDs.GetParameter( TCP::PHYS_LEN );

    BOOST_TEST_MESSAGE( "Rectwg synth 10 GHz: a_const=" << a_const << " a_ds=" << a_ds
                                                         << " len_const=" << len_const
                                                         << " len_ds=" << len_ds );

    BOOST_TEST( std::isfinite( a_const ) );
    BOOST_TEST( a_const > 0.0 );
    BOOST_TEST( std::isfinite( a_ds ) );
    BOOST_TEST( a_ds > 0.0 );
    BOOST_TEST( a_ds != a_const );
    BOOST_TEST( len_ds != len_const );

    BOOST_TEST( calcDs.GetParameter( TCP::EPSILONR ) == 4.4 );
    BOOST_TEST( calcDs.GetParameter( TCP::TAND ) == 0.02 );
}


BOOST_AUTO_TEST_SUITE_END()
