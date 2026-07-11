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

#ifndef DIALOG_CONSTRAINT_VALUE_H_
#define DIALOG_CONSTRAINT_VALUE_H_

#include <dialogs/dialog_constraint_value_base.h>

#include <constraints/pcb_constraint.h>

class BOARD_COMMIT;
class PCB_BASE_FRAME;
class PCB_CONSTRAINT;
class UNIT_BINDER;


/**
 * Show the value dialog for a valued constraint and stage the change in @p aCommit (the caller
 * pushes).  Returns true if the user accepted a change; false for a valueless constraint or cancel.
 * Shared by the constraint panel and the on-canvas double-click edit.
 */
bool EditConstraintValue( PCB_BASE_FRAME* aFrame, PCB_CONSTRAINT* aConstraint,
                          BOARD_COMMIT& aCommit );


/**
 * Value entry for a dimensional geometric constraint (issue #2329): a UNIT_BINDER for the length
 * (or angle, in degrees) plus a driving/reference toggle.  Pre-filled with the measured geometry.
 */
class DIALOG_CONSTRAINT_VALUE : public DIALOG_CONSTRAINT_VALUE_BASE
{
public:
    DIALOG_CONSTRAINT_VALUE( PCB_BASE_FRAME* aParent, PCB_CONSTRAINT_TYPE aType, double aValue,
                             bool aDriving );
    ~DIALOG_CONSTRAINT_VALUE() override;

    /// The entered value, in IU for a length/radius or in degrees for an angle.
    double GetConstraintValue();

    /// True if the constraint should drive (lock) the geometry; false for a reference dimension.
    bool GetDriving() const;

private:
    bool         m_isAngle;
    UNIT_BINDER* m_valueBinder;
};

#endif // DIALOG_CONSTRAINT_VALUE_H_
