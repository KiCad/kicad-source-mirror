/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_DIALOG_DIMENSION_PROPERTIES_H
#define KICAD_DIALOG_DIMENSION_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include "dialog_dimension_properties_base.h"


class BOARD_ITEM;
class BOARD_COMMIT;
class PCB_CONSTRAINT;
class PCB_DIMENSION_BASE;
class PCB_BASE_EDIT_FRAME;

enum class DIM_VALUE_MODE : int;


class DIALOG_DIMENSION_PROPERTIES : public DIALOG_DIMENSION_PROPERTIES_BASE
{
public:
    DIALOG_DIMENSION_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem );

    ~DIALOG_DIMENSION_PROPERTIES();

protected:
    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

private:
    void onFontSelected( wxCommandEvent &aEvent ) override;
    void onBoldToggle( wxCommandEvent &aEvent ) override;
    void onAlignButton( wxCommandEvent &aEvent ) override;
    void onThickness( wxCommandEvent &aEvent ) override;

    void updateDimensionFromDialog( PCB_DIMENSION_BASE* aTarget );

    void updatePreviewText();

    /// True for value bearing dims aligned orthogonal or radial centre and leader have no value
    bool hasValueModeChoice() const;

    /// Populate the value-mode choice, offering the Driving entry only when DimensionCanDrive.
    void buildValueModeChoice();

    /// The value mode the current choice selection represents.
    DIM_VALUE_MODE selectedValueMode() const;

    /// Select the choice entry for @p aMode, falling back to Driven when Driving is not offered.
    void setValueModeSelection( DIM_VALUE_MODE aMode );

    /// True when field mirrors geometry in Driven mode refreshable by other edits
    bool valueFieldTracksMeasurement() const;

    /// Stages FIXED_LENGTH constraint for the mode on @p aCommit
    /// Returns live driving constraint to re solve or nullptr
    PCB_CONSTRAINT* stageValueModeConstraint( BOARD_COMMIT& aCommit );

    /// Parses value field as Driving length into @p aLengthIU in internal units
    /// Returns false for non positive values blocking stale or zero collapse
    bool parseDrivingLength( int& aLengthIU ) const;

    /// Re-solve so the geometry bound to a now-driving dimension follows its entered length.
    void resolveDrivingConstraint( PCB_CONSTRAINT* aConstraint );

private:
    PCB_BASE_EDIT_FRAME*    m_frame;

    PCB_DIMENSION_BASE*     m_dimension;
    PCB_DIMENSION_BASE*     m_previewDimension;

    PCB_LAYER_BOX_SELECTOR* m_cbLayerActual;       // The active layer box control
    wxTextCtrl*             m_txtValueActual;      // The active value control

    // Units the value field is shown in captured on open not reformatted when units dropdown changes
    EDA_UNITS               m_valueFieldUnits;

    // True when choice includes Driving entry with both endpoints bound indices shift when absent
    bool                    m_valueModeHasDriving = false;

    UNIT_BINDER             m_textWidth;
    UNIT_BINDER             m_textHeight;
    UNIT_BINDER             m_textThickness;
    UNIT_BINDER             m_textPosX;
    UNIT_BINDER             m_textPosY;
    UNIT_BINDER             m_orientation;         // rotation in degrees
    UNIT_BINDER             m_lineThickness;
    UNIT_BINDER             m_arrowLength;
    UNIT_BINDER             m_extensionOffset;
    UNIT_BINDER             m_extensionOvershoot;
};


#endif // KICAD_DIALOG_DIMENSION_PROPERTIES_H
