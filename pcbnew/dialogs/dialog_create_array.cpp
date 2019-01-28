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

#include <pcb_edit_frame.h>
#include <base_units.h>
#include <boost/algorithm/string/join.hpp>
#include <widgets/text_ctrl_eval.h>
#include <class_board.h>
#include <class_module.h>

#include "dialog_create_array.h"

/**
 * Struct containing the last-entered values for the dialog.
 */
struct CREATE_ARRAY_DIALOG_ENTRIES
{
    /**
     * Construct with some sensible defaults.
     * In future, this could be loaded from config?
     */
    CREATE_ARRAY_DIALOG_ENTRIES()
            : m_optionsSet( true ),
              m_gridNx( 5 ),
              m_gridNy( 5 ),
              m_gridDx( Millimeter2iu( 2.54 ) ),
              m_gridDy( Millimeter2iu( 2.54 ) ),
              m_gridOffsetX( 0 ),
              m_gridOffsetY( 0 ),
              m_gridStagger( 1 ),
              m_gridStaggerType( 0 ),   // rows
              m_gridNumberingAxis( 0 ), // h then v
              m_gridNumberingReverseAlternate( false ),
              m_gridNumberingStartSet( 1 ),    // use specified start
              m_grid2dArrayNumbering( 0 ),     // linear numbering
              m_gridPriAxisNumScheme( 0 ),     // numeric
              m_gridSecAxisNumScheme( 0 ),     // numeric
              m_gridPriNumberingOffset( "1" ), // numeric
              m_gridSecNumberingOffset( "1" ), // numeric
              m_circCentreX( 0 ),
              m_circCentreY( 0 ),
              m_circAngle( 0.0 ),
              m_circCount( 4 ),
              m_circNumberingStartSet( 1 ), // use specified start
              m_circNumberingOffset( "1" ),
              m_circRotate( false ),
              m_arrayTypeTab( 0 ) // start on grid view
    {
    }

    bool m_optionsSet;

    long m_gridNx, m_gridNy;
    long m_gridDx, m_gridDy;
    long m_gridOffsetX, m_gridOffsetY;
    long m_gridStagger;

    long     m_gridStaggerType, m_gridNumberingAxis;
    bool     m_gridNumberingReverseAlternate;
    long     m_gridNumberingStartSet;
    long     m_grid2dArrayNumbering;
    long     m_gridPriAxisNumScheme, m_gridSecAxisNumScheme;
    wxString m_gridPriNumberingOffset, m_gridSecNumberingOffset;

    long     m_circCentreX, m_circCentreY;
    long     m_circAngle;
    long     m_circCount;
    long     m_circNumberingStartSet;
    wxString m_circNumberingOffset;
    bool     m_circRotate;
    long     m_arrayTypeTab;
};

// Persistent options settings
static CREATE_ARRAY_DIALOG_ENTRIES saved_array_options;


DIALOG_CREATE_ARRAY::DIALOG_CREATE_ARRAY(
        PCB_BASE_FRAME* aParent, bool enableNumbering, wxPoint aOrigPos )
        : DIALOG_CREATE_ARRAY_BASE( aParent ),
          m_settings( NULL ),
          m_hSpacing( aParent, m_labelDx, m_entryDx, m_unitLabelDx ),
          m_vSpacing( aParent, m_labelDy, m_entryDy, m_unitLabelDy ),
          m_hOffset( aParent, m_labelOffsetX, m_entryOffsetX, m_unitLabelOffsetX ),
          m_vOffset( aParent, m_labelOffsetY, m_entryOffsetY, m_unitLabelOffsetY ),
          m_hCentre( aParent, m_labelCentreX, m_entryCentreX, m_unitLabelCentreX ),
          m_vCentre( aParent, m_labelCentreY, m_entryCentreY, m_unitLabelCentreY ),
          m_circRadius( aParent, m_labelCircRadius, m_valueCircRadius, m_unitLabelCircRadius ),
          m_circAngle( aParent, m_labelCircAngle, m_entryCircAngle, m_unitLabelCircAngle ),
          m_cfg_persister( saved_array_options.m_optionsSet ),
          m_originalItemPosition( aOrigPos ),
          m_numberingEnabled( enableNumbering )
{
    // Set up numbering scheme drop downs
    //
    // character set
    // NOTE: do not change the order of this relative to the NUMBERING_TYPE_T enum
    const wxString charSetDescriptions[] =
    {
        _( "Numerals (0,1,2,...,9,10)" ),
        _( "Hexadecimal (0,1,...,F,10,...)" ),
        _( "Alphabet, minus IOSQXZ" ),
        _( "Alphabet, full 26 characters" )
    };
    m_choicePriAxisNumbering->Set( arrayDim( charSetDescriptions ), charSetDescriptions );
    m_choiceSecAxisNumbering->Set( arrayDim( charSetDescriptions ), charSetDescriptions );

    m_choicePriAxisNumbering->SetSelection( 0 );
    m_choiceSecAxisNumbering->SetSelection( 0 );

    m_circAngle.SetUnits( EDA_UNITS_T::DEGREES );

    // bind grid options to persister
    m_cfg_persister.Add( *m_entryNx, saved_array_options.m_gridNx );
    m_cfg_persister.Add( *m_entryNy, saved_array_options.m_gridNy );
    m_cfg_persister.Add( m_hSpacing, saved_array_options.m_gridDx );
    m_cfg_persister.Add( m_vSpacing, saved_array_options.m_gridDy );

    m_cfg_persister.Add( m_hOffset, saved_array_options.m_gridOffsetX );
    m_cfg_persister.Add( m_vOffset, saved_array_options.m_gridOffsetY );
    m_cfg_persister.Add( *m_entryStagger, saved_array_options.m_gridStagger );

    m_cfg_persister.Add( *m_radioBoxGridStaggerType, saved_array_options.m_gridStaggerType );

    m_cfg_persister.Add( *m_radioBoxGridNumberingAxis, saved_array_options.m_gridNumberingAxis );
    m_cfg_persister.Add(
            *m_checkBoxGridReverseNumbering, saved_array_options.m_gridNumberingReverseAlternate );

    m_cfg_persister.Add( *m_rbGridStartNumberingOpt, saved_array_options.m_gridNumberingStartSet );
    m_cfg_persister.Add(
            *m_radioBoxGridNumberingScheme, saved_array_options.m_grid2dArrayNumbering );
    m_cfg_persister.Add( *m_choicePriAxisNumbering, saved_array_options.m_gridPriAxisNumScheme );
    m_cfg_persister.Add( *m_choiceSecAxisNumbering, saved_array_options.m_gridSecAxisNumScheme );

    m_cfg_persister.Add(
            *m_entryGridPriNumberingOffset, saved_array_options.m_gridPriNumberingOffset );
    m_cfg_persister.Add(
            *m_entryGridSecNumberingOffset, saved_array_options.m_gridSecNumberingOffset );

    // bind circular options to persister
    m_cfg_persister.Add( m_hCentre, saved_array_options.m_circCentreX );
    m_cfg_persister.Add( m_vCentre, saved_array_options.m_circCentreY );
    m_cfg_persister.Add( m_circAngle, saved_array_options.m_circAngle );
    m_cfg_persister.Add( *m_entryCircCount, saved_array_options.m_circCount );
    m_cfg_persister.Add( *m_entryRotateItemsCb, saved_array_options.m_circRotate );

    m_cfg_persister.Add( *m_rbCircStartNumberingOpt, saved_array_options.m_circNumberingStartSet );
    m_cfg_persister.Add( *m_entryCircNumberingStart, saved_array_options.m_circNumberingOffset );

    m_cfg_persister.Add( *m_gridTypeNotebook, saved_array_options.m_arrayTypeTab );

    m_cfg_persister.RestoreConfigToControls();

    // Run the callbacks once to process the dialog contents
    setControlEnablement();
    calculateCircularArrayProperties();

    m_stdButtonsOK->SetDefault();
    Fit();
    SetMinSize( GetSize() );
}


DIALOG_CREATE_ARRAY::~DIALOG_CREATE_ARRAY()
{
    if( m_settings != NULL )
        delete m_settings;
}


void DIALOG_CREATE_ARRAY::OnParameterChanged( wxCommandEvent& event )
{
    setControlEnablement();
    calculateCircularArrayProperties();
}


/**
 * Validates and saves (if valid) the type and offset of an array axis numbering
 *
 * @param offsetEntry the entry of the offset (text)
 * @param typeEntry the entry of the axis nmbering scheme (choice)
 * @param type the destination of the type if valid
 * @param offset the destination of the offset if valid
 * @param errors error string accumulator
 * @return if all valid
 */
static bool validateNumberingTypeAndOffset( const wxTextCtrl& offsetEntry,
                                            const wxChoice& typeEntry,
                                            ARRAY_OPTIONS::NUMBERING_TYPE_T& type,
                                            int& offset, wxArrayString& errors )
{
    const int typeVal = typeEntry.GetSelection();
    // mind undefined casts to enums (should not be able to happen)
    bool ok = typeVal <= ARRAY_OPTIONS::NUMBERING_TYPE_MAX;

    if( ok )
    {
        type = (ARRAY_OPTIONS::NUMBERING_TYPE_T) typeVal;
    }
    else
    {
        wxString err;
        err.Printf( _("Unrecognized numbering scheme: %d"), typeVal );
        errors.Add( err );
        // we can't proceed - we don't know the numbering type
        return false;
    }

    const wxString text = offsetEntry.GetValue();
    ok = ARRAY_OPTIONS::GetNumberingOffset( text, type, offset );

    if( !ok )
    {
        const wxString& alphabet = ARRAY_OPTIONS::AlphabetFromNumberingScheme( type );

        wxString err;
        err.Printf( _( "Could not determine numbering start from \"%s\": "
                       "expected value consistent with alphabet \"%s\"" ),
                    text, alphabet );
        errors.Add(err);
    }

    return ok;
}


/**
 * Validate and save a long integer entry
 *
 * @param entry the text entry to read from
 * @param dest the value destination
 * @param description description of the field (used if the value is not OK)
 * @param errors a list of errors to add any error to
 * @return valid
 */
static bool validateLongEntry( const wxTextEntry& entry, long& dest, const wxString& description,
                               wxArrayString& errors )
{
    bool ok = true;

    if( !entry.GetValue().ToLong( &dest ) )
    {
        wxString err;
        err.Printf( _("Bad numeric value for %s: %s"), description, entry.GetValue() );
        errors.Add( err );
        ok = false;
     }

    return ok;
}


bool DIALOG_CREATE_ARRAY::TransferDataFromWindow()
{
    ARRAY_OPTIONS*  newSettings = NULL;
    wxArrayString   errors;
    const wxWindow* page = m_gridTypeNotebook->GetCurrentPage();

    if( page == m_gridPanel )
    {
        ARRAY_GRID_OPTIONS* newGrid = new ARRAY_GRID_OPTIONS();
        bool ok = true;

        // ints
        ok = ok && validateLongEntry(*m_entryNx, newGrid->m_nx, _("horizontal count"), errors);
        ok = ok && validateLongEntry(*m_entryNy, newGrid->m_ny, _("vertical count"), errors);

        newGrid->m_delta.x = m_hSpacing.GetValue();
        newGrid->m_delta.y = m_vSpacing.GetValue();

        newGrid->m_offset.x = m_hOffset.GetValue();
        newGrid->m_offset.y = m_vOffset.GetValue();

        ok = ok && validateLongEntry(*m_entryStagger, newGrid->m_stagger, _("stagger"), errors);

        newGrid->m_stagger_rows = m_radioBoxGridStaggerType->GetSelection() == 0;

        newGrid->m_horizontalThenVertical = m_radioBoxGridNumberingAxis->GetSelection() == 0;
        newGrid->m_reverseNumberingAlternate = m_checkBoxGridReverseNumbering->GetValue();

        newGrid->SetShouldNumber( m_numberingEnabled );

        if ( m_numberingEnabled )
        {
            newGrid->m_2dArrayNumbering = m_radioBoxGridNumberingScheme->GetSelection() != 0;

            bool numOk = validateNumberingTypeAndOffset(
                                    *m_entryGridPriNumberingOffset, *m_choicePriAxisNumbering,
                                    newGrid->m_priAxisNumType, newGrid->m_numberingOffsetX,
                                    errors );

            if( newGrid->m_2dArrayNumbering )
            {
                numOk = validateNumberingTypeAndOffset(
                                    *m_entryGridSecNumberingOffset, *m_choiceSecAxisNumbering,
                                    newGrid->m_secAxisNumType, newGrid->m_numberingOffsetY,
                                    errors ) && numOk;
            }

            ok = ok && numOk;

            newGrid->SetNumberingStartIsSpecified( m_rbGridStartNumberingOpt->GetSelection() == 1 );
        }

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

        newCirc->m_centre.x = m_hCentre.GetValue();
        newCirc->m_centre.y = m_vCentre.GetValue();
        newCirc->m_angle = DoubleValueFromString( DEGREES, m_entryCircAngle->GetValue() );

        ok = ok && validateLongEntry(*m_entryCircCount, newCirc->m_nPts, _("point count"), errors);

        newCirc->m_rotateItems = m_entryRotateItemsCb->GetValue();
        newCirc->SetShouldNumber( m_numberingEnabled );

        if ( m_numberingEnabled )
        {
            newCirc->SetNumberingStartIsSpecified( m_rbCircStartNumberingOpt->GetSelection() == 1 );
            newCirc->m_numberingType = ARRAY_OPTIONS::NUMBERING_NUMERIC;

            ok = ok && validateLongEntry(*m_entryCircNumberingStart, newCirc->m_numberingOffset,
                                         _("numbering start"), errors);
        }

        // Only use settings if all values are good
        if( ok )
            newSettings = newCirc;
        else
            delete newCirc;
    }

    // If we got good settings, send them out and finish
    if( newSettings )
    {
        delete m_settings;

        // assign pointer and ownership here
        m_settings = newSettings;
        m_cfg_persister.ReadConfigFromControls();

        return true;
    }
    else
    {
        wxString errorStr;

        if( errors.IsEmpty() )
            errorStr = _("Bad parameters");
        else
            errorStr = boost::algorithm::join( errors, "\n" );

        wxMessageBox( errorStr );
        return false;
    }
}


void DIALOG_CREATE_ARRAY::setControlEnablement()
{
    if ( m_numberingEnabled )
    {
        const bool use_set_start_grid = m_rbGridStartNumberingOpt->GetSelection() == 1;

        m_radioBoxGridNumberingScheme->Enable( true );
        m_labelPriAxisNumbering->Enable( true );
        m_choicePriAxisNumbering->Enable( true );

        // Disable the secondary axis numbering option if the
        // numbering scheme doesn't have two axes
        const bool num2d = m_radioBoxGridNumberingScheme->GetSelection() != 0;

        m_labelSecAxisNumbering->Enable( true && num2d );
        m_choiceSecAxisNumbering->Enable( true && num2d );

        // We can only set an offset if we're setting the start number
        m_labelGridNumberingOffset->Enable( use_set_start_grid );
        m_entryGridPriNumberingOffset->Enable( use_set_start_grid );
        m_entryGridSecNumberingOffset->Enable( use_set_start_grid && num2d );

        // disable the circular number offset in the same way
        const bool use_set_start_circ = m_rbCircStartNumberingOpt->GetSelection() == 1;
        m_entryCircNumberingStart->Enable( use_set_start_circ );
    }
    else
    {
        // grid
        m_rbGridStartNumberingOpt->Enable( false );
        m_radioBoxGridNumberingScheme->Enable( false );
        m_choiceSecAxisNumbering->Enable( false );
        m_choicePriAxisNumbering->Enable( false );
        m_entryGridPriNumberingOffset->Enable( false );
        m_entryGridSecNumberingOffset->Enable( false );

        // circular
        m_rbCircStartNumberingOpt->Enable( false );
        m_entryCircNumberingStart->Enable( false );
    }
}


void DIALOG_CREATE_ARRAY::calculateCircularArrayProperties()
{
    VECTOR2I centre( m_hCentre.GetValue(), m_vCentre.GetValue() );

    // Find the radius, etc of the circle
    centre -= m_originalItemPosition;

    m_circRadius.SetValue( int( centre.EuclideanNorm() ) );
}