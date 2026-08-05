/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dialogs/dialog_griditem_properties_base.h>

#include <algorithm>
#include <functional>

#include <board.h>
#include <board_commit.h>
#include <dialogs/geom_field_helpers.h>
#include <pcb_base_edit_frame.h>
#include <pcb_edit_frame.h>
#include <pcb_griditem.h>
#include <string_utils.h>
#include <trigo.h>
#include <widgets/unit_binder.h>

#include <wx/notebook.h>


class DIALOG_GRIDITEM_PROPERTIES : public DIALOG_GRIDITEM_PROPERTIES_BASE
{
public:
    DIALOG_GRIDITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_GRIDITEM* aItem );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    // The three notebook pages all edit the same extent rectangle; field order matches CTRL_IDX.
    enum CTRL_IDX
    {
        START_X = 0,
        START_Y,
        END_X,
        END_Y,
        CORNER_X,
        CORNER_Y,
        CORNER_W,
        CORNER_H,
        CENTER_X,
        CENTER_Y,
        CENTER_W,
        CENTER_H,

        POLAR_CENTER_X,
        POLAR_CENTER_Y,
        POLAR_RADIUS,
        POLAR_PHI,

        NUM_CTRLS,
    };

    int getInt( size_t aIndex ) const
    {
        return static_cast<int>( m_boundFields[aIndex].m_Binder->GetValue() );
    }

    void changeValue( size_t aIndex, int aValue )
    {
        m_boundFields[aIndex].m_Binder->ChangeValue( aValue );
    }

    EDA_ANGLE getAngle( size_t aIndex ) const
    {
        return m_boundFields[aIndex].m_Binder->GetAngleValue();
    }

    void changeAngle( size_t aIndex, const EDA_ANGLE& aValue )
    {
        m_boundFields[aIndex].m_Binder->ChangeAngleValue( aValue );
    }

    void bindFields( size_t aFrom, size_t aTo, std::function<void()> aCb )
    {
        for( size_t i = aFrom; i <= aTo; ++i )
        {
            m_boundFields[i].m_Ctrl->Bind( wxEVT_TEXT,
                                           [aCb]( wxCommandEvent& )
                                           {
                                               aCb();
                                           } );
        }
    }

    // The extent box is stored in the grid's local frame and drawn rotated by the
    // orientation, so corner fields must map through the rotation to stay on the
    // actual outline.
    VECTOR2I toWorld( VECTOR2I aLocal ) const
    {
        RotatePoint( aLocal, m_workingCopy.GetOrientation() );
        return aLocal;
    }

    VECTOR2I toLocal( VECTOR2I aWorld ) const
    {
        RotatePoint( aWorld, -m_workingCopy.GetOrientation() );
        return aWorld;
    }

    void onCornersChange();
    void onCornerSizeChange();
    void onCenterSizeChange();
    void onPolarChange();
    void updateCorners();
    void updateCornerSize();
    void updateCenterSize();
    void updatePolar();

    void onGridType( wxCommandEvent& aEvent );
    void saveSpacingToItem( PCB_GRIDITEM_TYPE aType );
    void updateSpacingRows();
    void updatePagesForType();

    PCB_BASE_EDIT_FRAME*       m_parent;
    PCB_GRIDITEM*              m_item;
    PCB_GRIDITEM               m_workingCopy;
    std::vector<BOUND_CONTROL> m_boundFields;
    UNIT_BINDER                m_orientation;
    UNIT_BINDER                m_spacingX;
    UNIT_BINDER                m_spacingY;
};


DIALOG_GRIDITEM_PROPERTIES::DIALOG_GRIDITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                        PCB_GRIDITEM* aItem ) :
        DIALOG_GRIDITEM_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_item( aItem ),
        m_workingCopy( *aItem ),
        m_orientation( aParent, m_orientationLabel, m_orientationCtrl, m_orientationUnits ),
        m_spacingX( aParent, m_spacingXLabel, m_spacingXCtrl, m_spacingXUnits ),
        m_spacingY( aParent, m_spacingYLabel, m_spacingYCtrl, m_spacingYUnits )
{
    SetTitle( wxString::Format( GetTitle(), m_item->GetFriendlyName() ) );
    m_hash_key = TO_UTF8( GetTitle() );

    m_orientation.SetPrecision( 4 );
    m_orientation.SetUnits( EDA_UNITS::DEGREES );

    // Field order must match CTRL_IDX.
    AddXYPointToSizer( *aParent, *m_gbsRectangleByCorners, 0, 0, _( "Start Point" ), false, m_boundFields );
    AddXYPointToSizer( *aParent, *m_gbsRectangleByCorners, 0, 3, _( "End Point" ), false, m_boundFields );

    AddXYPointToSizer( *aParent, *m_gbsRectangleByCornerSize, 0, 0, _( "Start Point" ), false, m_boundFields );
    AddXYPointToSizer( *aParent, *m_gbsRectangleByCornerSize, 0, 3, _( "Size" ), true, m_boundFields );

    AddXYPointToSizer( *aParent, *m_gbsRectangleByCenterSize, 0, 0, _( "Center" ), false, m_boundFields );
    AddXYPointToSizer( *aParent, *m_gbsRectangleByCenterSize, 0, 3, _( "Size" ), true, m_boundFields );

    AddXYPointToSizer( *aParent, *m_gbsPolarCenterRadius, 0, 0, _( "Center" ), false, m_boundFields );
    AddFieldToSizer( *aParent, *m_gbsPolarCenterRadius, 1, 3, _( "Radius" ), ORIGIN_TRANSFORMS::NOT_A_COORD, false, m_boundFields );
    AddFieldToSizer( *aParent, *m_gbsPolarCenterRadius, 2, 3, _( "Angular extent" ), ORIGIN_TRANSFORMS::NOT_A_COORD, true, m_boundFields );

    bindFields( START_X, END_Y, [this]() { onCornersChange(); } );
    bindFields( CORNER_X, CORNER_H, [this]() { onCornerSizeChange(); } );
    bindFields( CENTER_X, CENTER_H, [this]() { onCenterSizeChange(); } );
    bindFields( POLAR_CENTER_X, POLAR_PHI, [this]() { onPolarChange(); } );

    for( wxWindow* page : { m_rectangleByCorners, m_rectangleByCornerSize, m_rectangleByCenterSize,
                            m_polarCenterRadius } )
        page->Layout();

    m_gridTypeCtrl->Bind( wxEVT_CHOICE, &DIALOG_GRIDITEM_PROPERTIES::onGridType, this );

    // Corner fields are world coordinates on the rotated outline, so they track the angle.
    m_orientationCtrl->Bind( wxEVT_TEXT,
                             [this]( wxCommandEvent& )
                             {
                                 m_workingCopy.SetOrientation( m_orientation.GetAngleValue() );
                                 updateCorners();
                                 updateCornerSize();
                             } );

    // Do not allow locking items in the footprint editor
    m_locked->Show( dynamic_cast<PCB_EDIT_FRAME*>( aParent ) != nullptr );

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_GRIDITEM_PROPERTIES::onCornersChange()
{
    const VECTOR2I p0( getInt( START_X ), getInt( START_Y ) );
    const VECTOR2I p1( getInt( END_X ), getInt( END_Y ) );

    m_workingCopy.SetPosition( ( p0 + p1 ) / 2 );
    m_workingCopy.SetExtent( toLocal( p1 - p0 ) / 2 );

    updateCornerSize();
    updateCenterSize();
    updatePolar();
}


void DIALOG_GRIDITEM_PROPERTIES::onCornerSizeChange()
{
    const VECTOR2I p0( getInt( CORNER_X ), getInt( CORNER_Y ) );
    const VECTOR2I size( getInt( CORNER_W ), getInt( CORNER_H ) );

    m_workingCopy.SetPosition( p0 + toWorld( size / 2 ) );
    m_workingCopy.SetExtent( size / 2 );

    updateCorners();
    updateCenterSize();
    updatePolar();
}


void DIALOG_GRIDITEM_PROPERTIES::onCenterSizeChange()
{
    const VECTOR2I center( getInt( CENTER_X ), getInt( CENTER_Y ) );
    const VECTOR2I size( getInt( CENTER_W ), getInt( CENTER_H ) );

    m_workingCopy.SetPosition( center );
    m_workingCopy.SetExtent( size / 2 );

    updateCorners();
    updateCornerSize();
    updatePolar();
}


void DIALOG_GRIDITEM_PROPERTIES::onPolarChange()
{
    m_workingCopy.SetPosition( VECTOR2I( getInt( POLAR_CENTER_X ), getInt( POLAR_CENTER_Y ) ) );
    m_workingCopy.SetRadiusExtent( getInt( POLAR_RADIUS ) );
    m_workingCopy.SetPhiExtentDegrees( getAngle( POLAR_PHI ).AsDegrees() );

    updateCorners();
    updateCornerSize();
    updateCenterSize();
}


void DIALOG_GRIDITEM_PROPERTIES::updateCorners()
{
    const VECTOR2I half = toWorld( m_workingCopy.GetExtent() );
    const VECTOR2I p0 = m_workingCopy.GetPosition() - half;
    const VECTOR2I p1 = m_workingCopy.GetPosition() + half;

    changeValue( START_X, p0.x );
    changeValue( START_Y, p0.y );
    changeValue( END_X, p1.x );
    changeValue( END_Y, p1.y );
}


void DIALOG_GRIDITEM_PROPERTIES::updateCornerSize()
{
    const VECTOR2I p0 = m_workingCopy.GetPosition() - toWorld( m_workingCopy.GetExtent() );

    changeValue( CORNER_X, p0.x );
    changeValue( CORNER_Y, p0.y );
    changeValue( CORNER_W, m_workingCopy.GetExtentX() );
    changeValue( CORNER_H, m_workingCopy.GetExtentY() );
}


void DIALOG_GRIDITEM_PROPERTIES::updateCenterSize()
{
    changeValue( CENTER_X, m_workingCopy.GetPosition().x );
    changeValue( CENTER_Y, m_workingCopy.GetPosition().y );
    changeValue( CENTER_W, m_workingCopy.GetExtentX() );
    changeValue( CENTER_H, m_workingCopy.GetExtentY() );
}


void DIALOG_GRIDITEM_PROPERTIES::updatePolar()
{
    changeValue( POLAR_CENTER_X, m_workingCopy.GetPosition().x );
    changeValue( POLAR_CENTER_Y, m_workingCopy.GetPosition().y );
    changeValue( POLAR_RADIUS, m_workingCopy.GetRadiusExtent() );
    changeAngle( POLAR_PHI, m_workingCopy.GetPhiExtent() );
}


void DIALOG_GRIDITEM_PROPERTIES::updatePagesForType()
{
    const bool polar = m_workingCopy.GetGridItemType() == PCB_GRIDITEM_TYPE::POLAR;

    // RemovePage() keeps the window alive as a (visible) child, so hide it explicitly.
    while( m_notebookGridDefs->GetPageCount() > 0 )
    {
        m_notebookGridDefs->GetPage( m_notebookGridDefs->GetPageCount() - 1 )->Hide();
        m_notebookGridDefs->RemovePage( m_notebookGridDefs->GetPageCount() - 1 );
    }

    if( polar )
    {
        m_notebookGridDefs->AddPage( m_polarCenterRadius, _( "Center and Radius" ), true );
    }
    else
    {
        m_notebookGridDefs->AddPage( m_rectangleByCorners, _( "By Corners" ), false );
        m_notebookGridDefs->AddPage( m_rectangleByCornerSize, _( "By Corner and Size" ), false );
        m_notebookGridDefs->AddPage( m_rectangleByCenterSize, _( "By Center and Size" ), true );
    }

    Layout();
}


void DIALOG_GRIDITEM_PROPERTIES::onGridType( wxCommandEvent& aEvent )
{
    // Capture the field values under their old meaning before relabelling.
    saveSpacingToItem( m_workingCopy.GetGridItemType() );
    m_workingCopy.SetGridItemType( static_cast<PCB_GRIDITEM_TYPE>( m_gridTypeCtrl->GetSelection() ) );
    updateSpacingRows();
    updatePagesForType();
}


void DIALOG_GRIDITEM_PROPERTIES::saveSpacingToItem( PCB_GRIDITEM_TYPE aType )
{
    if( aType == PCB_GRIDITEM_TYPE::POLAR )
    {
        m_workingCopy.SetRadiusSpacing( m_spacingX.GetIntValue() );
        m_workingCopy.SetPhiSpacingDegrees( m_spacingY.GetAngleValue().AsDegrees() );
    }
    else
    {
        m_workingCopy.SetSpacingX( m_spacingX.GetIntValue() );
        m_workingCopy.SetSpacingY( m_spacingY.GetIntValue() );
    }
}


void DIALOG_GRIDITEM_PROPERTIES::updateSpacingRows()
{
    if( m_workingCopy.GetGridItemType() == PCB_GRIDITEM_TYPE::POLAR )
    {
        m_spacingX.SetLabel( _( "Radius spacing:" ) );
        m_spacingY.SetLabel( _( "Angular spacing:" ) );
        m_spacingY.SetPrecision( 4 );
        m_spacingY.SetUnits( EDA_UNITS::DEGREES );
        m_spacingX.ChangeValue( m_workingCopy.GetRadiusSpacing() );
        m_spacingY.ChangeAngleValue( m_workingCopy.GetPhiSpacing() );
    }
    else
    {
        m_spacingX.SetLabel( _( "Spacing X:" ) );
        m_spacingY.SetLabel( _( "Spacing Y:" ) );
        m_spacingY.SetPrecision( 0 );
        m_spacingY.SetUnits( m_parent->GetUserUnits() );
        m_spacingX.ChangeValue( m_workingCopy.GetSpacingX() );
        m_spacingY.ChangeValue( m_workingCopy.GetSpacingY() );
    }
}


bool DIALOG_GRIDITEM_PROPERTIES::TransferDataToWindow()
{
    updateCorners();
    updateCornerSize();
    updateCenterSize();
    updatePolar();

    m_orientation.ChangeAngleValue( m_workingCopy.GetOrientation() );

    m_gridTypeCtrl->SetSelection( static_cast<int>( m_workingCopy.GetGridItemType() ) );
    updateSpacingRows();
    updatePagesForType();

    m_tickIntervalCtrl->ChangeValue( wxString::Format( wxT( "%u" ), m_workingCopy.GetTickInterval() ) );
    m_priorityCtrl->ChangeValue( wxString::Format( wxT( "%u" ), m_workingCopy.GetAssignedPriority() ) );

    m_affectsCursor->SetValue( m_workingCopy.Affects().cursor );
    m_affectsRouting->SetValue( m_workingCopy.Affects().routing );
    m_affectsPlacement->SetValue( m_workingCopy.Affects().placement );

    m_locked->SetValue( m_item->IsLocked() );

    return true;
}


bool DIALOG_GRIDITEM_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_GRIDITEM_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    saveSpacingToItem( m_workingCopy.GetGridItemType() );

    m_workingCopy.SetOrientation( m_orientation.GetAngleValue() );

    long tickInterval = 0;
    m_tickIntervalCtrl->GetValue().ToLong( &tickInterval );
    m_workingCopy.SetTickInterval( std::max( 0L, tickInterval ) );

    long priority = 0;
    m_priorityCtrl->GetValue().ToLong( &priority );
    m_workingCopy.SetAssignedPriority( std::max( 0L, priority ) );

    m_workingCopy.Affects().cursor = m_affectsCursor->GetValue();
    m_workingCopy.Affects().routing = m_affectsRouting->GetValue();
    m_workingCopy.Affects().placement = m_affectsPlacement->GetValue();

    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_item );

    bool pushCommit = ( m_item->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same item if it is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_item->SetFlags( IN_EDIT );

    *m_item = m_workingCopy;
    m_item->SetLocked( m_locked->GetValue() );

    if( pushCommit )
        commit.Push( _( "Edit Grid Item Properties" ) );

    return true;
}


void PCB_BASE_EDIT_FRAME::ShowGridItemPropertiesDialog( PCB_GRIDITEM* aGridItem )
{
    wxCHECK_RET( aGridItem, wxT( "ShowGridItemPropertiesDialog() error: NULL item" ) );

    DIALOG_GRIDITEM_PROPERTIES dlg( this, aGridItem );
    dlg.ShowModal();
}
