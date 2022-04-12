/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_MODEL_SOURCE_H
#define SIM_MODEL_SOURCE_H

#include <sim/sim_model.h>


class SIM_MODEL_SOURCE : public SIM_MODEL
{
public:
    SIM_MODEL_SOURCE( TYPE aType );

    wxString GenerateSpiceModelLine( const wxString& aModelName ) const override;
    wxString GenerateSpiceItemLine( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aPinNetNames ) const override;

    bool SetParamValue( int aParamIndex, const wxString& aValue,
                        SIM_VALUE_GRAMMAR::NOTATION aNotation ) override;

    bool HasAutofill() const override { return true; }

private:
    std::vector<wxString> getPinNames() const override;

    static const std::vector<PARAM::INFO>& makeParams( TYPE aType );

    static std::vector<PARAM::INFO> makeDc( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeSin( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePulse( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeExp( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeSfam( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeSffm( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePwl( wxString aPrefix, wxString aQuantity, wxString aUnit );
    static std::vector<PARAM::INFO> makeWhiteNoise( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePinkNoise( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeBurstNoise( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomUniform( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomNormal( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomExp( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomPoisson( wxString aPrefix, wxString aUnit );
};

#endif // SIM_MODEL_SOURCE_H
