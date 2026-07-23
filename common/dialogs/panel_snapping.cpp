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

#include <algorithm>

#include <dialogs/panel_snapping.h>


std::vector<MAGNETIC_OPTIONS> MagneticSnapOptions( bool aRouting )
{
    if( aRouting )
    {
        return { MAGNETIC_OPTIONS::NO_EFFECT, MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL,
                 MAGNETIC_OPTIONS::CAPTURE_ALWAYS };
    }

    return { MAGNETIC_OPTIONS::NO_EFFECT, MAGNETIC_OPTIONS::CAPTURE_ALWAYS };
}


int MagneticSnapIndex( const std::vector<MAGNETIC_OPTIONS>& aOptions, MAGNETIC_OPTIONS aValue )
{
    auto it = std::find( aOptions.begin(), aOptions.end(), aValue );

    if( it == aOptions.end() )
        it = std::find( aOptions.begin(), aOptions.end(), MAGNETIC_OPTIONS::NO_EFFECT );

    return it == aOptions.end() ? 0 : static_cast<int>( std::distance( aOptions.begin(), it ) );
}


MAGNETIC_OPTIONS MagneticSnapValue( const std::vector<MAGNETIC_OPTIONS>& aOptions, int aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<int>( aOptions.size() ) )
        return MAGNETIC_OPTIONS::NO_EFFECT;

    return aOptions[aIndex];
}


static wxString magneticOptionLabel( MAGNETIC_OPTIONS aOption )
{
    switch( aOption )
    {
    case MAGNETIC_OPTIONS::NO_EFFECT:                    return _( "Never" );
    case MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL: return _( "When routing tracks" );
    case MAGNETIC_OPTIONS::CAPTURE_ALWAYS:               return _( "Always" );
    }

    return wxEmptyString;
}


static void fillMagneticChoice( wxChoice* aChoice, const std::vector<MAGNETIC_OPTIONS>& aOptions )
{
    for( MAGNETIC_OPTIONS option : aOptions )
        aChoice->Append( magneticOptionLabel( option ) );
}


PANEL_SNAPPING::PANEL_SNAPPING( wxWindow* aParent, FRAME_T aFrameType, int* aGridSnap,
                                SNAP_INFERENCE_SETTINGS* aInference, MAGNETIC_SETTINGS* aMagnetics,
                                std::function<VALUES()> aDefaults ) :
        PANEL_SNAPPING_BASE( aParent ),
        m_routing( aFrameType == FRAME_PCB_EDITOR ),
        m_gridSnap( aGridSnap ),
        m_inference( aInference ),
        m_magnetics( aMagnetics ),
        m_defaults( std::move( aDefaults ) )
{
    m_padOptions = MagneticSnapOptions( m_routing );
    m_trackOptions = MagneticSnapOptions( true );
    m_graphicsOptions = MagneticSnapOptions( false );

    fillMagneticChoice( m_padSnapChoice, m_padOptions );
    fillMagneticChoice( m_trackSnapChoice, m_trackOptions );
    fillMagneticChoice( m_graphicsSnapChoice, m_graphicsOptions );

    m_objectSnappingLabel->Show( m_magnetics != nullptr );
    m_objectSnappingLine->Show( m_magnetics != nullptr );
    m_sizerObjectSnapping->ShowItems( m_magnetics != nullptr );

    // Only the editors that route tracks have tracks to snap to.
    m_trackSnapLabel->Show( m_routing );
    m_trackSnapChoice->Show( m_routing );

    m_snapInferenceLabel->Show( m_inference != nullptr );
    m_snapInferenceLine->Show( m_inference != nullptr );
    m_sizerSnapInference->ShowItems( m_inference != nullptr );

    Layout();
}


void PANEL_SNAPPING::load( const VALUES& aValues )
{
    // The grid snapping mode is a bare int in the config, so a hand-edited file can name a mode
    // that doesn't exist.
    if( aValues.gridSnap >= 0 && aValues.gridSnap < static_cast<int>( m_gridSnapChoice->GetCount() ) )
        m_gridSnapChoice->SetSelection( aValues.gridSnap );
    else
        m_gridSnapChoice->SetSelection( 0 );

    if( m_magnetics )
    {
        m_padSnapChoice->SetSelection( MagneticSnapIndex( m_padOptions, aValues.magnetics.pads ) );

        if( m_routing )
            m_trackSnapChoice->SetSelection( MagneticSnapIndex( m_trackOptions, aValues.magnetics.tracks ) );

        MAGNETIC_OPTIONS graphics = aValues.magnetics.graphics ? MAGNETIC_OPTIONS::CAPTURE_ALWAYS
                                                               : MAGNETIC_OPTIONS::NO_EFFECT;
        m_graphicsSnapChoice->SetSelection( MagneticSnapIndex( m_graphicsOptions, graphics ) );

        m_snapAllLayers->SetValue( aValues.magnetics.allLayers );
    }

    if( m_inference )
    {
        m_snapObjectGeometry->SetValue( aValues.inference.objectGeometry );
        m_snapConstructionExtensions->SetValue( aValues.inference.constructionExtensions );
        m_snapTangentNormal->SetValue( aValues.inference.tangentNormal );
        m_snapAlignmentDistribution->SetValue( aValues.inference.alignmentDistribution );
    }
}


bool PANEL_SNAPPING::TransferDataToWindow()
{
    VALUES values;
    values.gridSnap = *m_gridSnap;

    if( m_magnetics )
        values.magnetics = *m_magnetics;

    if( m_inference )
        values.inference = *m_inference;

    load( values );

    return true;
}


bool PANEL_SNAPPING::TransferDataFromWindow()
{
    *m_gridSnap = m_gridSnapChoice->GetSelection();

    if( m_magnetics )
    {
        m_magnetics->pads = MagneticSnapValue( m_padOptions, m_padSnapChoice->GetSelection() );

        if( m_routing )
            m_magnetics->tracks = MagneticSnapValue( m_trackOptions, m_trackSnapChoice->GetSelection() );

        m_magnetics->graphics = MagneticSnapValue( m_graphicsOptions, m_graphicsSnapChoice->GetSelection() )
                                        == MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
        m_magnetics->allLayers = m_snapAllLayers->GetValue();
    }

    if( m_inference )
    {
        m_inference->objectGeometry = m_snapObjectGeometry->GetValue();
        m_inference->constructionExtensions = m_snapConstructionExtensions->GetValue();
        m_inference->tangentNormal = m_snapTangentNormal->GetValue();
        m_inference->alignmentDistribution = m_snapAlignmentDistribution->GetValue();
    }

    return true;
}


void PANEL_SNAPPING::ResetPanel()
{
    load( m_defaults() );
}
