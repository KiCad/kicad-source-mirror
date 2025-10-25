/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Reece R. Pollack <reece@his.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/debug.h>     // for wxASSERT
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <pcb_origin_transforms.h>

using COORD_TYPE = ORIGIN_TRANSFORMS::COORD_TYPES_T;

PCB_ORIGIN_TRANSFORMS::PCB_ORIGIN_TRANSFORMS( PCB_BASE_FRAME& aPcbBaseFrame ) :
        m_pcbBaseFrame( aPcbBaseFrame )
{}


long long int PCB_ORIGIN_TRANSFORMS::ToDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const
{
    long long int value = aValue;

    switch( aCoordType )
    {
    case COORD_TYPE::ABS_X_COORD: value = ToDisplayAbsX( value ); break;
    case COORD_TYPE::ABS_Y_COORD: value = ToDisplayAbsY( value ); break;
    case COORD_TYPE::REL_X_COORD: value = ToDisplayRelX( value ); break;
    case COORD_TYPE::REL_Y_COORD: value = ToDisplayRelY( value ); break;
    case COORD_TYPE::NOT_A_COORD: /* do nothing */ ; break;
    default:                      wxASSERT( false ); break;
    };

    return value;
}


double PCB_ORIGIN_TRANSFORMS::ToDisplay( double aValue, COORD_TYPES_T aCoordType ) const
{
    double value = aValue;

    switch( aCoordType )
    {
    case COORD_TYPE::ABS_X_COORD: value = ToDisplayAbsX( value ); break;
    case COORD_TYPE::ABS_Y_COORD: value = ToDisplayAbsY( value ); break;
    case COORD_TYPE::REL_X_COORD: value = ToDisplayRelX( value ); break;
    case COORD_TYPE::REL_Y_COORD: value = ToDisplayRelY( value ); break;
    case COORD_TYPE::NOT_A_COORD: /* do nothing */ ; break;
    default:                      wxASSERT( false ); break;
    };

    return value;
}


double PCB_ORIGIN_TRANSFORMS::ToDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const
{
    if( !invertYAxis() && ( aCoordType == REL_X_COORD || aCoordType == REL_Y_COORD ) )
        return -aValue.AsDegrees();

    return aValue.AsDegrees();
}


long long int PCB_ORIGIN_TRANSFORMS::FromDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const
{
    long long value = aValue;

    switch( aCoordType )
    {
    case COORD_TYPE::ABS_X_COORD: value = FromDisplayAbsX( value ); break;
    case COORD_TYPE::ABS_Y_COORD: value = FromDisplayAbsY( value ); break;
    case COORD_TYPE::REL_X_COORD: value = FromDisplayRelX( value ); break;
    case COORD_TYPE::REL_Y_COORD: value = FromDisplayRelY( value ); break;
    case COORD_TYPE::NOT_A_COORD: /* do nothing */ ; break;
    default:                      wxASSERT( false ); break;
    };

    return value;
}


double PCB_ORIGIN_TRANSFORMS::FromDisplay( double aValue, COORD_TYPES_T aCoordType ) const
{
    double value = aValue;

    switch( aCoordType )
    {
    case COORD_TYPE::ABS_X_COORD: value = FromDisplayAbsX( value ); break;
    case COORD_TYPE::ABS_Y_COORD: value = FromDisplayAbsY( value ); break;
    case COORD_TYPE::REL_X_COORD: value = FromDisplayRelX( value ); break;
    case COORD_TYPE::REL_Y_COORD: value = FromDisplayRelY( value ); break;
    case COORD_TYPE::NOT_A_COORD: /* do nothing */ ; break;
    default:                      wxASSERT( false ); break;
    };

    return value;
}


EDA_ANGLE PCB_ORIGIN_TRANSFORMS::FromDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const
{
    if( !invertYAxis() && ( aCoordType == REL_X_COORD || aCoordType == REL_Y_COORD ) )
        return -aValue;

    return aValue;
}


int PCB_ORIGIN_TRANSFORMS::getUserXOrigin() const
{
    return m_pcbBaseFrame.GetUserOrigin().x;
}


int PCB_ORIGIN_TRANSFORMS::getUserYOrigin() const
{
    return m_pcbBaseFrame.GetUserOrigin().y;
}


bool PCB_ORIGIN_TRANSFORMS::invertXAxis() const
{
    if( m_pcbBaseFrame.GetFrameType() == FRAME_PCB_EDITOR )
        return m_pcbBaseFrame.GetPcbNewSettings()->m_Display.m_DisplayInvertXAxis;
    else
        return m_pcbBaseFrame.GetFootprintEditorSettings()->m_DisplayInvertXAxis;
}


bool PCB_ORIGIN_TRANSFORMS::invertYAxis() const
{
    if( m_pcbBaseFrame.GetFrameType() == FRAME_PCB_EDITOR )
        return m_pcbBaseFrame.GetPcbNewSettings()->m_Display.m_DisplayInvertYAxis;
    else
        return m_pcbBaseFrame.GetFootprintEditorSettings()->m_DisplayInvertYAxis;
}
