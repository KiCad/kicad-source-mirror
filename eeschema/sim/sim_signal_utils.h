/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_SIGNAL_UTILS_H
#define SIM_SIGNAL_UTILS_H

#include <map>
#include <string>
#include <vector>
#include <wx/arrstr.h> // wxbase exports std::vector<wxString>, so MSVC needs this to avoid LNK2005
#include <wx/string.h>

/**
 * Text handling for simulator signal expressions and measurement commands.
 *
 * These work on the strings the simulator frame hands to ngspice and hold no frame state.
 */
namespace SIM_SIGNAL
{

/// Which part of a complex signal a plotted row shows.
enum class PART
{
    VALUE,
    GAIN,       ///< AC magnitude, plotted in dB
    PHASE,
    AMPLITUDE   ///< S-parameter magnitude, plotted linear
};

/// Vector name prefix for the voltage across a component, "kicad_vdev3".
inline const wxString DEVICE_VECTOR_PREFIX = wxS( "kicad_vdev" );

/// @return true for a vector KiCad derives with a let command, "user3" or "kicad_vdev3", in any case.
bool IsDerivedSignalVector( const wxString& aVectorName );

/// @return true for a single V(...) reference, not a compound expression.
bool IsSingleVoltageRef( const wxString& aExpression );

/// @return true if a .meas function takes \a aSignal as it is: a vector name or a V(), VDB(), ..., I() reference.
bool IsMeasVector( const wxString& aSignal );

/**
 * Build the signal a measurement should read so it matches what is plotted.
 *
 * @return false if the part cannot be measured (phase).
 */
bool MeasurableSignal( const wxString& aBase, PART aPart, wxString& aResult );

/**
 * Split "MAX V(out) from=1m to=2m" into its function, signal and trailing parameters.
 *
 * Parameters start at the first whitespace-delimited "name=" outside quotes and parentheses, so comparison
 * operators such as ">=" stay part of the signal.
 */
void SplitMeasurement( const wxString& aCmd, wxString& aFunc, wxString& aSignal, wxString& aParams );

/**
 * Replace V(R1), VDB(R1), VM(R1), ... with the terminal voltage expression of the part.
 *
 * @param aDeviceExprs maps a reference ("R1") to its expression ("V(a) - V(b)").  Matching ignores case, like
 *                     ngspice.  Inserted text is never rescanned.
 */
wxString ExpandDeviceVoltages( const wxString& aExpression, const std::map<wxString, wxString>& aDeviceExprs );

/**
 * Quote every whole occurrence of a net name that ngspice would otherwise parse as an expression.
 *
 * Matching ignores case because ngspice node names do; the text keeps the user's spelling.  Names already in
 * quotes are left alone.
 */
wxString QuoteNetNames( const wxString& aExpression, const std::vector<wxString>& aNetnames );

/// @return true if every parenthesis outside quotes is closed in order.
bool ParensBalanced( const wxString& aExpression );

/**
 * Extract the V() and I() and P() node or device references from an expression, unquoted.
 *
 * A two-node V(a,b) yields one reference per node; ground is omitted.  VDB(), VM(), VP(), VR() and VI() count
 * as voltage references.
 */
std::vector<wxString> ExtractSignalRefs( const wxString& aExpression );

/**
 * Build the voltage across a two-terminal part in its model's pin order, matching the sign of I() and P().
 *
 * @param aModelPins    symbol pin numbers in the order the SPICE model declares its terminals.
 * @param aPinNumbers   symbol pin numbers of the netlist item.
 * @param aPinNetNames  SPICE net names, parallel to \a aPinNumbers.
 * @return the expression, or empty if the part has no voltage across it.
 */
wxString DeviceVoltageExpression( const std::vector<std::string>& aModelPins,
                                  const std::vector<std::string>& aPinNumbers,
                                  const std::vector<std::string>& aPinNetNames );

} // namespace SIM_SIGNAL

#endif // SIM_SIGNAL_UTILS_H
