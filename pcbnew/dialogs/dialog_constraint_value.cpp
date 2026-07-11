/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_constraint_value.h>

#include <pcb_base_frame.h>
#include <board.h>
#include <board_commit.h>
#include <pcb_shape.h>
#include <constraints/board_constraint_adapter.h>
#include <widgets/unit_binder.h>


DIALOG_CONSTRAINT_VALUE::DIALOG_CONSTRAINT_VALUE( PCB_BASE_FRAME* aParent, PCB_CONSTRAINT_TYPE aType,
                                                  double aValue, bool aDriving ) :
        DIALOG_CONSTRAINT_VALUE_BASE( aParent ),
        m_isAngle( aType == PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION )
{
    if( m_isAngle )
        m_valueLabel->SetLabel( _( "Angle:" ) );

    m_drivingCtrl->SetValue( aDriving );

    m_valueBinder = new UNIT_BINDER( aParent, m_valueLabel, m_valueCtrl, m_valueUnits );

    if( m_isAngle )
    {
        m_valueBinder->SetUnits( EDA_UNITS::DEGREES );
        m_valueBinder->SetAngleValue( EDA_ANGLE( aValue, DEGREES_T ) );
    }
    else
    {
        m_valueBinder->SetValue( static_cast<long long int>( aValue ) );
    }

    finishDialogSettings();
}


DIALOG_CONSTRAINT_VALUE::~DIALOG_CONSTRAINT_VALUE()
{
    delete m_valueBinder;
}


double DIALOG_CONSTRAINT_VALUE::GetConstraintValue()
{
    if( m_isAngle )
        return m_valueBinder->GetAngleValue().AsDegrees();

    return static_cast<double>( m_valueBinder->GetValue() );
}


bool DIALOG_CONSTRAINT_VALUE::GetDriving() const
{
    return m_drivingCtrl->GetValue();
}


bool EditConstraintValue( PCB_BASE_FRAME* aFrame, PCB_CONSTRAINT* aConstraint, BOARD_COMMIT& aCommit )
{
    if( !aConstraint || !aConstraint->HasValue() )
        return false;

    DIALOG_CONSTRAINT_VALUE dlg( aFrame, aConstraint->GetConstraintType(), *aConstraint->GetValue(),
                                 aConstraint->IsDriving() );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aCommit.Modify( aConstraint );
    aConstraint->SetValue( dlg.GetConstraintValue() );
    aConstraint->SetDriving( dlg.GetDriving() );

    // Snap the geometry to the new value in the same commit, the way creation does -- otherwise a
    // driving length/radius/angle change is stored but the shapes keep their old size.
    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( aFrame->GetBoard(), aConstraint, &modified,
                                [&]( PCB_SHAPE* aShape ) { aCommit.Modify( aShape ); } );

    aCommit.Push( _( "Edit Geometric Constraint" ) );
    return true;
}
