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
#include <limits>

#include <transline_calculations/coplanar.h>
#include <transline_calculations/coupled_microstrip.h>
#include <transline_calculations/microstrip.h>
#include <transline_calculations/transline_calculation_base.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{

// Probe subclass used to exercise the shared helper without any geometry-specific math.
// The configurable m_fixedDeltaQ / m_useMicrostripDeltaQ flags let tests drive
// ApplySoldermaskCorrection through its early-out paths and through the active path.
class PROBE_TRANSLINE : public TRANSLINE_CALCULATION_BASE
{
public:
    PROBE_TRANSLINE() :
            TRANSLINE_CALCULATION_BASE( { TCP::H,
                                          TCP::SOLDERMASK_PRESENT,
                                          TCP::SOLDERMASK_THICKNESS,
                                          TCP::SOLDERMASK_EPSILONR,
                                          TCP::SOLDERMASK_TAND } )
    {
    }

    void Analyse() override {}
    bool Synthesize( SYNTHESIZE_OPTS ) override { return false; }

    double GetSoldermaskDeltaQ( double aWOverH, double aCOverH ) const override
    {
        if( m_useMicrostripDeltaQ )
        {
            const double q2Coated = WanHoorfarQ2( aWOverH, 1.0 + aCOverH );
            const double q2Base = WanHoorfarQ2( aWOverH, 1.0 );
            return std::max( 0.0, q2Coated - q2Base );
        }

        return m_fixedDeltaQ;
    }

    std::pair<double, double> CallPair( double aEpsEffUncoated, double aTanDSub, double aEpsRSub,
                                        double aWOverH ) const
    {
        return ApplySoldermaskCorrection( aEpsEffUncoated, aTanDSub, aEpsRSub, aWOverH, 1.0e9 );
    }

protected:
    void SetAnalysisResults() override {}
    void SetSynthesisResults() override {}

public:
    bool   m_useMicrostripDeltaQ{ false };
    double m_fixedDeltaQ{ 0.0 };
};


MICROSTRIP MakeFr4Microstrip()
{
    // FR-4 8 mil substrate, 14 mil trace, 1.4 mil copper, 1 oz.  Canonical 50-ohm
    // internal-layer reference geometry.
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::H, 8.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::H_T, 1.0e20 );
    calc.SetParameter( TCP::PHYS_WIDTH, 14.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::T, 1.4 * TC::UNIT_MIL );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::PHYS_LEN, 100.0e-3 );
    return calc;
}


COUPLED_MICROSTRIP MakeFr4CoupledMicrostrip()
{
    COUPLED_MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::H, 8.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::H_T, 1.0e20 );
    calc.SetParameter( TCP::PHYS_WIDTH, 10.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_S, 10.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::T, 1.4 * TC::UNIT_MIL );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::PHYS_LEN, 100.0e-3 );
    return calc;
}


COPLANAR MakeFr4Cpw( bool aBackMetal )
{
    COPLANAR calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, 10.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::T, 1.4 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_WIDTH, 10.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_S, 6.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::Z0, 0.0 );
    calc.SetParameter( TCP::ANG_L, 0.0 );
    calc.SetParameter( TCP::CPW_BACKMETAL, aBackMetal ? 1.0 : 0.0 );
    return calc;
}


void EnableMask( TRANSLINE_CALCULATION_BASE& aCalc, double aThicknessM, double aEpsR = 3.5,
                 double aTanD = 0.025 )
{
    aCalc.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    aCalc.SetParameter( TCP::SOLDERMASK_THICKNESS, aThicknessM );
    aCalc.SetParameter( TCP::SOLDERMASK_EPSILONR, aEpsR );
    aCalc.SetParameter( TCP::SOLDERMASK_TAND, aTanD );
}

} // namespace


BOOST_AUTO_TEST_SUITE( Soldermask )


// SOLDERMASK_PRESENT == 0 must leave the inputs bit-identical.  Callers rely on this to
// avoid tolerance widening in the large existing regression suite.
BOOST_AUTO_TEST_CASE( MaskAbsentReturnsInputsUnchanged )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 0.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 25.0e-6 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_fixedDeltaQ = 0.1;

    const auto result = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    BOOST_CHECK_EQUAL( result.first, 3.3 );
    BOOST_CHECK_EQUAL( result.second, 0.02 );
}


// Zero mask thickness takes the no-op path.  Required so flipping the present checkbox
// without filling the thickness field leaves pre-mask regressions unchanged.
BOOST_AUTO_TEST_CASE( ZeroThicknessReturnsInputsUnchanged )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 0.0 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_fixedDeltaQ = 0.1;

    const auto result = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    BOOST_CHECK_EQUAL( result.first, 3.3 );
    BOOST_CHECK_EQUAL( result.second, 0.02 );
}


// Subclasses that do not override GetSoldermaskDeltaQ (or return 0) must no-op so that
// stripline, coax, waveguide, twisted-pair are immune to mask parameters being set.
BOOST_AUTO_TEST_CASE( ZeroDeltaQReturnsInputsUnchanged )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 25.0e-6 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_fixedDeltaQ = 0.0;

    const auto result = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    BOOST_CHECK_EQUAL( result.first, 3.3 );
    BOOST_CHECK_EQUAL( result.second, 0.02 );
}


// Wan-Hoorfar 2000 hand computation for u = W/h = 1.75, C/h = 0.125, using eq (2) for
// q_1, eq (4) for the effective strip width, eq (5) for v-bar, and eq (12) for the
// mask filling factor q_2.
//
//   w-bar_e  = 1.75 + (2/pi) * ln(17.08 * (0.5 * 1.75 + 0.92))
//            = 1.75 + (2/pi) * ln(30.65)
//            = 1.75 + 2.179 = 3.929
//   v-bar_2  = (2/pi) * atan( 2*pi * (h-bar_2 - 1) / (pi * w-bar_e - 4) )
//            = (2/pi) * atan( 2*pi * 0.125 / 8.346 ) = 0.0589 at h-bar_2 = 1.125
//            = 0                                      at h-bar_2 = 1.0
//   q_1      = 1 - ln(pi * w-bar_e - 1) / (2 * w-bar_e)
//            = 1 - ln(11.344) / 7.858 = 0.691
//   inner(h-bar_2 = 1.125) = 2 * w-bar_e * cos(pi/2 * 0.0589) / (2 * 1.125 - 1 + 0.0589)
//                              + sin(pi/2 * 0.0589)
//                          = 7.858 * 0.99573 / 1.3089 + 0.09227 = 6.087
//   inner(h-bar_2 = 1.0)   = 2 * w-bar_e / 1 + 0 = 7.858
//   q_2(1.125) = 1 - 0.6909 - (0.9403 / 7.859) * ln(6.068) = 0.3091 - 0.2158 = 0.0933
//   q_2(1.000) = 1 - 0.6909 - 1 / 7.859 * ln(7.859)        = 0.3091 - 0.2624 = 0.0467
//   Delta q   = 0.0933 - 0.0467 = 0.0466
//   eps_eff_coated = 3.3 + 0.0466 * (3.5 - 1) = 3.3 + 0.1166 = 3.4166
BOOST_AUTO_TEST_CASE( WanHoorfarFormulaMatchesHandComputation )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 0.125e-3 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_useMicrostripDeltaQ = true;

    const auto result = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    const double q2Coated = TRANSLINE_CALCULATION_BASE::WanHoorfarQ2( 1.75, 1.125 );
    const double q2Base = TRANSLINE_CALCULATION_BASE::WanHoorfarQ2( 1.75, 1.0 );
    const double deltaQ = q2Coated - q2Base;
    const double expected = 3.3 + deltaQ * ( 3.5 - 1.0 );

    BOOST_CHECK_CLOSE_FRACTION( result.first, expected, 1.0e-6 );

    // The hand-computed value, pinned to four decimals, ensures no accidental refactor
    // drifts the formula coefficients.  Matches the derivation in the function header.
    BOOST_CHECK_CLOSE_FRACTION( result.first, 3.4166, 1.0e-3 );
    BOOST_CHECK_CLOSE_FRACTION( deltaQ, 0.0466, 2.0e-3 );
}


// Thick-mask limit: at C >> h the Wan-Hoorfar q_2 saturates at (1 - q_1), so the coated
// effective permittivity approaches the Bahl-Stuchly 1980 eq (22) limit
//   eps_eff -> q_sub * eps_sub + (1 - q_sub) * eps_mask
// where q_sub = (eps_eff_uncoated - 1) / (eps_sub - 1).
BOOST_AUTO_TEST_CASE( ThickMaskApproachesBahlStuchlyLimit )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 100.0e-3 ); // C = 100 * h
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_useMicrostripDeltaQ = true;

    const double epsEffUncoated = 3.3;
    const double epsRSub = 4.4;
    const auto result = probe.CallPair( epsEffUncoated, 0.02, epsRSub, 1.75 );

    const double qSub = ( epsEffUncoated - 1.0 ) / ( epsRSub - 1.0 );
    const double limit = qSub * epsRSub + ( 1.0 - qSub ) * 3.5;

    // Formula converges to the limit within ~5% because the anchoring subtraction shifts
    // the asymptote by q_2(h-bar=1), which is non-zero in Svacina's formula.  The
    // correct direction and monotonic approach are the essential physics.
    BOOST_TEST( result.first > epsEffUncoated );
    BOOST_TEST( result.first < 3.5 + qSub * ( epsRSub - 1.0 ) );
    BOOST_CHECK_CLOSE_FRACTION( result.first, limit, 5.0e-2 );
}


// Mask disabled on a microstrip must match the pre-mask baseline bit-identically.  This
// is the only regression guard the existing microstrip_cover suite does not already
// provide; failing it means the mask path leaked into the no-mask case.
BOOST_AUTO_TEST_CASE( MicrostripUncoatedIsBitIdenticalWithMaskDisabled )
{
    MICROSTRIP baseline = MakeFr4Microstrip();
    baseline.Analyse();
    const double z0Baseline = baseline.GetParameter( TCP::Z0 );
    const double eEffBaseline = baseline.GetParameter( TCP::EPSILON_EFF );
    const double attenBaseline = baseline.GetParameter( TCP::ATTEN_DILECTRIC );

    MICROSTRIP withMaskOff = MakeFr4Microstrip();
    EnableMask( withMaskOff, 25.0e-6 );
    withMaskOff.SetParameter( TCP::SOLDERMASK_PRESENT, 0.0 );
    withMaskOff.Analyse();

    BOOST_CHECK_CLOSE_FRACTION( withMaskOff.GetParameter( TCP::Z0 ), z0Baseline, 1.0e-9 );
    BOOST_CHECK_CLOSE_FRACTION( withMaskOff.GetParameter( TCP::EPSILON_EFF ), eEffBaseline, 1.0e-9 );
    BOOST_CHECK_CLOSE_FRACTION( withMaskOff.GetParameter( TCP::ATTEN_DILECTRIC ), attenBaseline, 1.0e-9 );
}


// Direction and magnitude.  Adding a mask lowers Z0 and raises eps_eff monotonically in
// the mask thickness.  Target Z0 drop for 1 mil mask on 14/8 mil FR-4 is ~1-3 ohms per
// Bogatin 2018 Ch. 7 Table 7-2 measured data.
BOOST_AUTO_TEST_CASE( MicrostripMaskDirectionAndMagnitude )
{
    MICROSTRIP open = MakeFr4Microstrip();
    open.Analyse();
    const double z0Open = open.GetParameter( TCP::Z0 );
    const double eEffOpen = open.GetParameter( TCP::EPSILON_EFF );

    MICROSTRIP oneMil = MakeFr4Microstrip();
    EnableMask( oneMil, 1.0 * TC::UNIT_MIL );
    oneMil.Analyse();
    const double z0One = oneMil.GetParameter( TCP::Z0 );
    const double eEffOne = oneMil.GetParameter( TCP::EPSILON_EFF );

    MICROSTRIP twoMil = MakeFr4Microstrip();
    EnableMask( twoMil, 2.0 * TC::UNIT_MIL );
    twoMil.Analyse();
    const double z0Two = twoMil.GetParameter( TCP::Z0 );
    const double eEffTwo = twoMil.GetParameter( TCP::EPSILON_EFF );

    BOOST_TEST( z0One < z0Open );
    BOOST_TEST( z0Two < z0One );
    BOOST_TEST( eEffOne > eEffOpen );
    BOOST_TEST( eEffTwo > eEffOne );

    const double drop1 = z0Open - z0One;
    const double drop2 = z0Open - z0Two;

    BOOST_TEST( drop1 > 0.3 );
    BOOST_TEST( drop1 < 4.0 );
    BOOST_TEST( drop2 > drop1 );
    BOOST_TEST( drop2 < 8.0 );
}


// Coupled microstrip mask raises both mode eps_eff.  No strict ordering constraint on
// (odd delta vs even delta) in the current formula because Wan-Hoorfar uses the same u
// for both modes; a rigorous model would extend Kirschning-Jansen 1984 to the overlay
// case, which is not attempted here.
BOOST_AUTO_TEST_CASE( CoupledMicrostripMaskRaisesBothModes )
{
    COUPLED_MICROSTRIP open = MakeFr4CoupledMicrostrip();
    open.Analyse();
    const double z0eOpen = open.GetParameter( TCP::Z0_E );
    const double z0oOpen = open.GetParameter( TCP::Z0_O );
    const double eEven = open.GetAnalysisResults()[TCP::EPSILON_EFF_EVEN].first;
    const double eOdd = open.GetAnalysisResults()[TCP::EPSILON_EFF_ODD].first;

    COUPLED_MICROSTRIP coated = MakeFr4CoupledMicrostrip();
    EnableMask( coated, 1.0 * TC::UNIT_MIL );
    coated.Analyse();
    const double eEvenC = coated.GetAnalysisResults()[TCP::EPSILON_EFF_EVEN].first;
    const double eOddC = coated.GetAnalysisResults()[TCP::EPSILON_EFF_ODD].first;

    BOOST_TEST( coated.GetParameter( TCP::Z0_E ) < z0eOpen );
    BOOST_TEST( coated.GetParameter( TCP::Z0_O ) < z0oOpen );
    BOOST_TEST( eEvenC > eEven );
    BOOST_TEST( eOddC > eOdd );
}


// CPW sanity check.  Gaps-filled produces a larger Z0 drop than gaps-unfilled because
// the mask occupies the high-field-density coplanar slots.
BOOST_AUTO_TEST_CASE( CpwMaskGapsFilledExceedsMaskTracesOnly )
{
    COPLANAR open = MakeFr4Cpw( false );
    open.Analyse();
    const double z0Open = open.GetParameter( TCP::Z0 );

    COPLANAR tracesOnly = MakeFr4Cpw( false );
    EnableMask( tracesOnly, 1.0 * TC::UNIT_MIL );
    tracesOnly.SetParameter( TCP::SOLDERMASK_FILLS_GAPS, 0.0 );
    tracesOnly.Analyse();
    const double dropTraces = z0Open - tracesOnly.GetParameter( TCP::Z0 );

    COPLANAR gapsFilled = MakeFr4Cpw( false );
    EnableMask( gapsFilled, 1.0 * TC::UNIT_MIL );
    gapsFilled.SetParameter( TCP::SOLDERMASK_FILLS_GAPS, 1.0 );
    gapsFilled.Analyse();
    const double dropGaps = z0Open - gapsFilled.GetParameter( TCP::Z0 );

    BOOST_TEST( dropTraces > 0.0 );
    BOOST_TEST( dropGaps > dropTraces );
}


// Mask tan delta participates in dielectric loss.  The air-replacement decomposition
// means even for eps_mask < eps_sub the mask adds loss because it is being compared
// against lossless air, not against substrate.
BOOST_AUTO_TEST_CASE( SoldermaskTanDeltaAffectsDielectricLoss )
{
    MICROSTRIP lossless = MakeFr4Microstrip();
    EnableMask( lossless, 1.0 * TC::UNIT_MIL, 3.5, 0.0 );
    lossless.Analyse();

    MICROSTRIP lossy = MakeFr4Microstrip();
    EnableMask( lossy, 1.0 * TC::UNIT_MIL, 3.5, 0.025 );
    lossy.Analyse();

    const double aLossless = lossless.GetParameter( TCP::ATTEN_DILECTRIC );
    const double aLossy = lossy.GetParameter( TCP::ATTEN_DILECTRIC );

    BOOST_TEST( std::isfinite( aLossless ) );
    BOOST_TEST( std::isfinite( aLossy ) );
    BOOST_TEST( aLossy > aLossless );
    BOOST_TEST( ( aLossy - aLossless ) > 1.0e-6 );
}


// Hand-pinned tan-delta blend on the air-replacement decomposition.  Substrate q_sub is
// computed from q_sub = (eps_eff_uncoated - 1) / (eps_sub - 1), the mask Delta q from
// Wan-Hoorfar, and the blended tan delta is the eps-weighted air-excluded average
// normalised by eps_eff_coated.
BOOST_AUTO_TEST_CASE( TanDeltaBlendAirReplacement )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 0.125e-3 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_useMicrostripDeltaQ = true;

    const double epsEffUncoated = 3.3;
    const double epsRSub = 4.4;
    const double tanDSub = 0.02;

    const auto result = probe.CallPair( epsEffUncoated, tanDSub, epsRSub, 1.75 );

    const double q2Coated = TRANSLINE_CALCULATION_BASE::WanHoorfarQ2( 1.75, 1.125 );
    const double q2Base = TRANSLINE_CALCULATION_BASE::WanHoorfarQ2( 1.75, 1.0 );
    const double deltaQ = q2Coated - q2Base;
    const double epsEffCoated = epsEffUncoated + deltaQ * ( 3.5 - 1.0 );
    const double qSub = ( epsEffUncoated - 1.0 ) / ( epsRSub - 1.0 );
    const double expectedTanD =
            ( qSub * epsRSub * tanDSub + deltaQ * 3.5 * 0.025 ) / epsEffCoated;

    BOOST_CHECK_CLOSE_FRACTION( result.first, epsEffCoated, 1.0e-9 );
    BOOST_CHECK_CLOSE_FRACTION( result.second, expectedTanD, 1.0e-9 );

    BOOST_TEST( result.second > 0.0 );
    BOOST_TEST( result.second < std::max( tanDSub, 0.025 ) );
}


// Lossless mask still yields non-zero dielectric loss because the substrate retains its
// own loss contribution.  Conversely a lossless substrate with a lossy mask must
// produce some dielectric loss.
BOOST_AUTO_TEST_CASE( TanDeltaBlendBothEndsContribute )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 0.125e-3 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.m_useMicrostripDeltaQ = true;

    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.0 );
    const auto losslessMask = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    const auto lossyMask = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );

    BOOST_TEST( losslessMask.second > 0.0 );
    BOOST_TEST( lossyMask.second > losslessMask.second );

    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    const auto losslessSub = probe.CallPair( 3.3, 0.0, 4.4, 1.75 );

    BOOST_TEST( losslessSub.second > 0.0 );
}


// Input-validation guards: each non-physical mask material parameter must short-circuit
// the correction and return the un-coated values bit-identical so downstream Z0 and
// ATTEN_DILECTRIC do not absorb NaN / inf or develop negative ("dielectric gain") loss.
BOOST_AUTO_TEST_CASE( BadMaskEpsilonRejectsCorrection )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 25.0e-6 );
    probe.SetParameter( TCP::SOLDERMASK_TAND, 0.025 );
    probe.m_useMicrostripDeltaQ = true;

    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 1.0 );
    auto r1 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r1.first, 3.3 );
    BOOST_CHECK_EQUAL( r1.second, 0.02 );

    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 0.5 );
    auto r2 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r2.first, 3.3 );
    BOOST_CHECK_EQUAL( r2.second, 0.02 );

    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, std::nan( "" ) );
    auto r3 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r3.first, 3.3 );
    BOOST_CHECK_EQUAL( r3.second, 0.02 );

    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, std::numeric_limits<double>::infinity() );
    auto r4 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r4.first, 3.3 );
    BOOST_CHECK_EQUAL( r4.second, 0.02 );
}


BOOST_AUTO_TEST_CASE( BadMaskTanDeltaRejectsCorrection )
{
    PROBE_TRANSLINE probe;
    probe.SetParameter( TCP::H, 1.0e-3 );
    probe.SetParameter( TCP::SOLDERMASK_PRESENT, 1.0 );
    probe.SetParameter( TCP::SOLDERMASK_THICKNESS, 25.0e-6 );
    probe.SetParameter( TCP::SOLDERMASK_EPSILONR, 3.5 );
    probe.m_useMicrostripDeltaQ = true;

    probe.SetParameter( TCP::SOLDERMASK_TAND, -1.0 );
    auto r1 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r1.first, 3.3 );
    BOOST_CHECK_EQUAL( r1.second, 0.02 );

    probe.SetParameter( TCP::SOLDERMASK_TAND, std::nan( "" ) );
    auto r2 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r2.first, 3.3 );
    BOOST_CHECK_EQUAL( r2.second, 0.02 );

    probe.SetParameter( TCP::SOLDERMASK_TAND, std::numeric_limits<double>::infinity() );
    auto r3 = probe.CallPair( 3.3, 0.02, 4.4, 1.75 );
    BOOST_CHECK_EQUAL( r3.first, 3.3 );
    BOOST_CHECK_EQUAL( r3.second, 0.02 );
}


// CBCPW.  Mask present produces a smaller Z0 drop than the CPW case at the same geometry
// because the back-metal already confines a larger share of the field to the substrate.
BOOST_AUTO_TEST_CASE( CbcpwMaskDropsLessThanPureCpw )
{
    COPLANAR cpwOpen = MakeFr4Cpw( false );
    cpwOpen.Analyse();
    COPLANAR cpwCoated = MakeFr4Cpw( false );
    EnableMask( cpwCoated, 1.0 * TC::UNIT_MIL );
    cpwCoated.SetParameter( TCP::SOLDERMASK_FILLS_GAPS, 1.0 );
    cpwCoated.Analyse();
    const double cpwDrop = cpwOpen.GetParameter( TCP::Z0 ) - cpwCoated.GetParameter( TCP::Z0 );

    COPLANAR cbcpwOpen = MakeFr4Cpw( true );
    cbcpwOpen.Analyse();
    COPLANAR cbcpwCoated = MakeFr4Cpw( true );
    EnableMask( cbcpwCoated, 1.0 * TC::UNIT_MIL );
    cbcpwCoated.SetParameter( TCP::SOLDERMASK_FILLS_GAPS, 1.0 );
    cbcpwCoated.Analyse();
    const double cbcpwDrop = cbcpwOpen.GetParameter( TCP::Z0 ) - cbcpwCoated.GetParameter( TCP::Z0 );

    BOOST_TEST( cpwDrop > 0.0 );
    BOOST_TEST( cbcpwDrop > 0.0 );
    BOOST_TEST( cbcpwDrop < cpwDrop );
}


BOOST_AUTO_TEST_SUITE_END()
