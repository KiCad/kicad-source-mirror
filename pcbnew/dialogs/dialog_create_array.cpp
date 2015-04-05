/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wxPcbStruct.h>
#include <base_units.h>
#include <macros.h>

#include <class_drawpanel.h>
#include <class_board.h>
#include <class_module.h>

#include "dialog_create_array.h"


// initialise statics
DIALOG_CREATE_ARRAY::CREATE_ARRAY_DIALOG_ENTRIES DIALOG_CREATE_ARRAY::m_options;


DIALOG_CREATE_ARRAY::DIALOG_CREATE_ARRAY( PCB_BASE_FRAME* aParent, wxPoint aOrigPos,
                                          ARRAY_OPTIONS** aSettings ) :
    DIALOG_CREATE_ARRAY_BASE( aParent ),
    CONFIG_SAVE_RESTORE_WINDOW( m_options.m_optionsSet ),
    m_settings( aSettings ),
    m_originalItemPosition( aOrigPos )
{
    // Set up numbering scheme drop downs
    //
    // character set
    // NOTE: do not change the order of this relative to the ARRAY_NUMBERING_TYPE_T enum
    const wxString charSetDescriptions[] =
    {
        _( "Numerals (0,1,2,...,9,10)" ),
        _( "Hexadecimal (0,1,...,F,10,...)" ),
        _( "Alphabet, minus IOSQXZ" ),
        _( "Alphabet, full 26 characters" )
    };
    m_choicePriAxisNumbering->Set( DIM( charSetDescriptions ), charSetDescriptions );
    m_choiceSecAxisNumbering->Set( DIM( charSetDescriptions ), charSetDescriptions );
    m_choiceCircNumberingType->Set( DIM( charSetDescriptions ), charSetDescriptions );;

    m_choicePriAxisNumbering->SetSelection( 0 );
    m_choiceSecAxisNumbering->SetSelection( 0 );
    m_choiceCircNumberingType->SetSelection( 0 );

    Add( m_entryNx, m_options.m_gridNx );
    Add( m_entryNy, m_options.m_gridNy );
    Add( m_entryDx, m_options.m_gridDx );
    Add( m_entryDy, m_options.m_gridDy );

    Add( m_entryOffsetX, m_options.m_gridOffsetX );
    Add( m_entryOffsetY, m_options.m_gridOffsetY );
    Add( m_entryStagger, m_options.m_gridStagger );

    Add( m_radioBoxGridStaggerType, m_options.m_gridStaggerType );

    Add( m_radioBoxGridNumberingAxis, m_options.m_gridNumberingAxis );
    Add( m_checkBoxGridReverseNumbering, m_options.m_gridNumberingReverseAlternate );

    Add( m_entryCentreX, m_options.m_circCentreX );
    Add( m_entryCentreY, m_options.m_circCentreY );
    Add( m_entryCircAngle, m_options.m_circAngle );
    Add( m_entryCircCount, m_options.m_circCount );
    Add( m_entryRotateItemsCb, m_options.m_circRotate );
    Add( m_entryCircNumberingStart, m_options.m_circNumberingOffset );

    Add( m_gridTypeNotebook, m_options.m_arrayTypeTab );

    Add( m_radioBoxGridNumberingScheme, m_options.m_grid2dArrayNumbering );
    Add( m_choicePriAxisNumbering, m_options.m_gridPriAxisNumScheme );
    Add( m_choiceSecAxisNumbering, m_options.m_gridSecAxisNumScheme );

    Add( m_entryGridPriNumberingOffset, m_options.m_gridPriNumberingOffset );
    Add( m_entryGridSecNumberingOffset, m_options.m_gridSecNumberingOffset );


    RestoreConfigToControls();

    // Load units into labels
    {
        const wxString lengthUnit = GetAbbreviatedUnitsLabel( g_UserUnit );

        m_unitLabelCentreX->SetLabelText( lengthUnit );
        m_unitLabelCentreY->SetLabelText( lengthUnit );
        m_unitLabelDx->SetLabelText( lengthUnit );
        m_unitLabelDy->SetLabelText( lengthUnit );
        m_unitLabelOffsetX->SetLabelText( lengthUnit );
        m_unitLabelOffsetY->SetLabelText( lengthUnit );
    }

    // Run the callbacks once to process the dialog contents
    setControlEnablement();
    calculateCircularArrayProperties();

    Fit();
}


void DIALOG_CREATE_ARRAY::OnParameterChanged( wxCommandEvent& event )
{
    const wxObject* evObj = event.GetEventObject();

    // some controls result in a change of enablement
    if( evObj == m_radioBoxGridNumberingScheme
        || evObj == m_checkBoxGridRestartNumbering )
    {
        setControlEnablement();
    }
    if( evObj == m_entryCentreX || evObj == m_entryCentreY )
    {
        calculateCircularArrayProperties();
    }
}


void DIALOG_CREATE_ARRAY::OnCancelClick( wxCommandEvent& event )
{
    EndModal( CREATE_ARRAY_ABORT );
}


static const std::string& alphabetFromNumberingScheme(
        DIALOG_CREATE_ARRAY::ARRAY_NUMBERING_TYPE_T type )
{
    static const std::string    alphaNumeric = "0123456789";
    static const std::string    alphaHex = "0123456789ABCDEF";
    static const std::string    alphaFull = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const std::string    alphaNoIOSQXZ   = "ABCDEFGHJKLNMPRTUVWY";
    static const std::string    alphaEmpty      = "";

    switch( type )
    {
    case DIALOG_CREATE_ARRAY::NUMBERING_NUMERIC:
        return alphaNumeric;

    case DIALOG_CREATE_ARRAY::NUMBERING_HEX:
        return alphaHex;

    case DIALOG_CREATE_ARRAY::NUMBERING_ALPHA_NO_IOSQXZ:
        return alphaNoIOSQXZ;

    case DIALOG_CREATE_ARRAY::NUMBERING_ALPHA_FULL:
        return alphaFull;

    default:
        wxASSERT_MSG( false, wxString( "Un-handled numbering scheme: " ) << type );
    }

    return alphaEmpty;
}


/**
 * @return False for schemes like 0,1...9,10
 *         True for schemes like A,B..Z,AA (where the tens column starts with char 0)
 */
static bool schemeNonUnitColsStartAt0( DIALOG_CREATE_ARRAY::ARRAY_NUMBERING_TYPE_T type )
{
    return type == DIALOG_CREATE_ARRAY::NUMBERING_ALPHA_FULL
           || type == DIALOG_CREATE_ARRAY::NUMBERING_ALPHA_NO_IOSQXZ;
}


static bool getNumberingOffset( const std::string& str,
        DIALOG_CREATE_ARRAY::ARRAY_NUMBERING_TYPE_T type,
        int& offsetToFill )
{
    const std::string alphabet = alphabetFromNumberingScheme( type );

    wxASSERT_MSG( !alphabet.empty(), wxString(
                    "Unable to determine alphabet for numbering scheme: " ) << type );

    int offset = 0;
    const int radix = alphabet.length();

    for( unsigned i = 0; i < str.length(); i++ )
    {
        int chIndex = alphabet.find( str[i], 0 );

        if( chIndex == wxNOT_FOUND )
            return false;

        const bool start0 = schemeNonUnitColsStartAt0( type );

        // eg "AA" is actually index 27, not 26
        if( start0 && i < str.length() - 1 )
            chIndex++;

        offset  *= radix;
        offset  += chIndex;
    }

    offsetToFill = offset;
    return true;
}


void DIALOG_CREATE_ARRAY::OnOkClick( wxCommandEvent& event )
{
    ARRAY_OPTIONS* newSettings = NULL;

    const wxWindow* page = m_gridTypeNotebook->GetCurrentPage();

    if( page == m_gridPanel )
    {
        ARRAY_GRID_OPTIONS* newGrid = new ARRAY_GRID_OPTIONS();
        bool ok = true;

        // ints
        ok  = ok && m_entryNx->GetValue().ToLong( &newGrid->m_nx );
        ok  = ok && m_entryNy->GetValue().ToLong( &newGrid->m_ny );

        newGrid->m_delta.x = DoubleValueFromString( g_UserUnit, m_entryDx->GetValue() );
        newGrid->m_delta.y = DoubleValueFromString( g_UserUnit, m_entryDy->GetValue() );

        newGrid->m_offset.x = DoubleValueFromString( g_UserUnit, m_entryOffsetX->GetValue() );
        newGrid->m_offset.y = DoubleValueFromString( g_UserUnit, m_entryOffsetY->GetValue() );

        ok = ok && m_entryStagger->GetValue().ToLong( &newGrid->m_stagger );

        newGrid->m_stagger_rows = m_radioBoxGridStaggerType->GetSelection() == 0;

        newGrid->m_horizontalThenVertical = m_radioBoxGridNumberingAxis->GetSelection() == 0;
        newGrid->m_reverseNumberingAlternate = m_checkBoxGridReverseNumbering->GetValue();

        newGrid->m_2dArrayNumbering = m_radioBoxGridNumberingScheme->GetSelection() != 0;

        // this is only correct if you set the choice up according to the enum size and order
        ok = ok && m_choicePriAxisNumbering->GetSelection() < NUMBERING_TYPE_Max
             && m_choiceSecAxisNumbering->GetSelection() < NUMBERING_TYPE_Max;

        // mind undefined casts to enums (should not be able to happen)
        if( ok )
        {
            newGrid->m_priAxisNumType =
                (ARRAY_NUMBERING_TYPE_T) m_choicePriAxisNumbering->GetSelection();
            newGrid->m_secAxisNumType =
                (ARRAY_NUMBERING_TYPE_T) m_choiceSecAxisNumbering->GetSelection();
        }

        // Work out the offsets for the numbering
        ok = ok && getNumberingOffset(
                m_entryGridPriNumberingOffset->GetValue().ToStdString(),
                newGrid->m_priAxisNumType, newGrid->m_numberingOffsetX );

        if( newGrid->m_2dArrayNumbering )
            ok = ok && getNumberingOffset(
                    m_entryGridSecNumberingOffset->GetValue().ToStdString(),
                    newGrid->m_secAxisNumType, newGrid->m_numberingOffsetY );

        newGrid->m_shouldRenumber = m_checkBoxGridRestartNumbering->GetValue();

        // Only use settings if all values are good
        if( ok )
            newSettings = newGrid;
        else
            delete newGrid;
    }
    else if( page == m_circularPanel )
    {
        ARRAY_CIRCULAR_OPTIONS* newCirc = new ARRAY_CIRCULAR_OPTIONS();
        bool ok = true;

        newCirc->m_centre.x = DoubleValueFromString( g_UserUnit, m_entryCentreX->GetValue() );
        newCirc->m_centre.y = DoubleValueFromString( g_UserUnit, m_entryCentreY->GetValue() );

        newCirc->m_angle = DoubleValueFromString( DEGREES, m_entryCircAngle->GetValue() );
        ok = ok && m_entryCircCount->GetValue().ToLong( &newCirc->m_nPts );

        newCirc->m_rotateItems = m_entryRotateItemsCb->GetValue();

        newCirc->m_shouldRenumber = m_checkBoxCircRestartNumbering->GetValue();

        // This is only correct if you set the choice up according to the enum size and order
        ok = ok && m_choiceCircNumberingType->GetSelection() < NUMBERING_TYPE_Max;

        // Mind undefined casts to enums (should not be able to happen)
        if( ok )
            newCirc->m_numberingType =
                (ARRAY_NUMBERING_TYPE_T) m_choiceCircNumberingType->GetSelection();

        ok = ok && m_entryCircNumberingStart->GetValue().ToLong( &newCirc->m_numberingOffset );

        // Only use settings if all values are good
        if( ok )
            newSettings = newCirc;
        else
            delete newCirc;
    }

    // If we got good settings, send them out and finish
    if( newSettings )
    {
        delete *m_settings;

        // assign pointer and ownership here
        *m_settings = newSettings;

        ReadConfigFromControls();

        EndModal( CREATE_ARRAY_OK );
    }
}


void DIALOG_CREATE_ARRAY::setControlEnablement()
{
    const bool renumber = m_checkBoxGridRestartNumbering->GetValue();

    // If we're not renumbering, we can't set the numbering scheme
    // or axis numbering types
    m_radioBoxGridNumberingScheme->Enable( renumber );
    m_labelPriAxisNumbering->Enable( renumber );
    m_choicePriAxisNumbering->Enable( renumber );

    // Disable the secondary axis numbering option if the
    // numbering scheme doesn't have two axes
    const bool num2d = m_radioBoxGridNumberingScheme->GetSelection() != 0;

    m_labelSecAxisNumbering->Enable( renumber && num2d );
    m_choiceSecAxisNumbering->Enable( renumber && num2d );

    // We can only set an offset if we renumber
    m_labelGridNumberingOffset->Enable( renumber );
    m_entryGridPriNumberingOffset->Enable( renumber );
    m_entryGridSecNumberingOffset->Enable( renumber && num2d );


    // Circular array options
    const bool circRenumber = m_checkBoxCircRestartNumbering->GetValue();
    m_choiceCircNumberingType->Enable( circRenumber );
}


void DIALOG_CREATE_ARRAY::calculateCircularArrayProperties()
{
    wxPoint centre;

    centre.x = DoubleValueFromString( g_UserUnit, m_entryCentreX->GetValue() );
    centre.y = DoubleValueFromString( g_UserUnit, m_entryCentreY->GetValue() );

    // FInd the radius, etc of the circle
    centre -= m_originalItemPosition;

    const double radius = VECTOR2I(centre.x, centre.y).EuclideanNorm();

    m_labelCircRadiusValue->SetLabelText( StringFromValue( g_UserUnit, int(radius), true ) );
}


// ARRAY OPTION implementation functions --------------------------------------

std::string DIALOG_CREATE_ARRAY::ARRAY_OPTIONS::getCoordinateNumber( int n,
        ARRAY_NUMBERING_TYPE_T type )
{
    std::string itemNum;
    const std::string& alphabet = alphabetFromNumberingScheme( type );

    if( !alphabet.empty() )
    {
        const bool nonUnitColsStartAt0 = schemeNonUnitColsStartAt0( type );

        bool    firstRound = true;
        int     radix = alphabet.length();

        do {
            int modN = n % radix;

            if( nonUnitColsStartAt0 && !firstRound )
                modN--;    // Start the "tens/hundreds/etc column" at "Ax", not "Bx"

            itemNum.insert( 0, 1, alphabet[modN] );

            n /= radix;
            firstRound = false;
        } while( n );
    }

    return itemNum;
}


wxString DIALOG_CREATE_ARRAY::ARRAY_OPTIONS::InterpolateNumberIntoString(
        int aN, const wxString& aPattern ) const
{
    wxString newStr( aPattern );
    newStr.Replace( "%s", GetItemNumber( aN ), false );

    return newStr;
}


int DIALOG_CREATE_ARRAY::ARRAY_GRID_OPTIONS::GetArraySize() const
{
    return m_nx * m_ny;
}


wxPoint DIALOG_CREATE_ARRAY::ARRAY_GRID_OPTIONS::getGridCoords( int n ) const
{
    const int axisSize = m_horizontalThenVertical ? m_nx : m_ny;

    int x   = n % axisSize;
    int y   = n / axisSize;

    // reverse on this row/col?
    if( m_reverseNumberingAlternate && ( y % 2 ) )
        x = axisSize - x - 1;

    wxPoint coords( x, y );

    return coords;
}


void DIALOG_CREATE_ARRAY::ARRAY_GRID_OPTIONS::TransformItem( int n, BOARD_ITEM* item,
        const wxPoint& rotPoint ) const
{
    wxPoint point;

    wxPoint coords = getGridCoords( n );

    // swap axes if needed
    if( !m_horizontalThenVertical )
        std::swap( coords.x, coords.y );

    point.x = coords.x * m_delta.x + coords.y * m_offset.x;
    point.y = coords.y * m_delta.y + coords.x * m_offset.y;

    if( std::abs( m_stagger ) > 1 )
    {
        const int stagger = std::abs( m_stagger );
        const bool  sr = m_stagger_rows;
        const int   stagger_idx = ( ( sr ? coords.y : coords.x ) % stagger );

        wxPoint stagger_delta( ( sr ? m_delta.x : m_offset.x ),
                ( sr ? m_offset.y : m_delta.y ) );

        // Stagger to the left/up if the sign of the stagger is negative
        point += stagger_delta * copysign( stagger_idx, m_stagger ) / stagger;
    }

    // this is already relative to the first array entry
    item->Move( point );
}


wxString DIALOG_CREATE_ARRAY::ARRAY_GRID_OPTIONS::GetItemNumber( int n ) const
{
    wxString itemNum;

    if( m_2dArrayNumbering )
    {
        wxPoint coords = getGridCoords( n );

        itemNum += getCoordinateNumber( coords.x + m_numberingOffsetX, m_priAxisNumType );
        itemNum += getCoordinateNumber( coords.y + m_numberingOffsetY, m_secAxisNumType );
    }
    else
    {
        itemNum += getCoordinateNumber( n + m_numberingOffsetX, m_priAxisNumType );
    }

    return itemNum;
}


int DIALOG_CREATE_ARRAY::ARRAY_CIRCULAR_OPTIONS::GetArraySize() const
{
    return m_nPts;
}


void DIALOG_CREATE_ARRAY::ARRAY_CIRCULAR_OPTIONS::TransformItem( int n, BOARD_ITEM* item,
        const wxPoint& rotPoint ) const
{
    double angle;

    if( m_angle == 0 )
        // angle is zero, divide evenly into m_nPts
        angle = 3600.0 * n / float(m_nPts);
    else
        // n'th step
        angle = m_angle * n;

    item->Rotate( m_centre, angle );

    // take off the rotation (but not the translation) if needed
    if( !m_rotateItems )
        item->Rotate( item->GetCenter(), -angle );
}


wxString DIALOG_CREATE_ARRAY::ARRAY_CIRCULAR_OPTIONS::GetItemNumber( int aN ) const
{
    return getCoordinateNumber( aN + m_numberingOffset, m_numberingType );
}
