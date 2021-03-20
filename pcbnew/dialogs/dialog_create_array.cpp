/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialogs/dialog_create_array.h"

#include <base_units.h>
#include <widgets/text_ctrl_eval.h>
#include <board.h>
#include <footprint.h>
#include <pcb_edit_frame.h>

#include <boost/algorithm/string/join.hpp>

/**
 * Struct containing the last-entered values for the dialog.
 */
struct CREATE_ARRAY_DIALOG_ENTRIES
{
    /**
     * Construct with some sensible defaults.
     * In future, this could be loaded from config?
     */
    CREATE_ARRAY_DIALOG_ENTRIES() :
            m_OptionsSet( true ),
            m_GridNx( 5 ),
            m_GridNy( 5 ),
            m_GridDx( Millimeter2iu( 2.54 ) ),
            m_GridDy( Millimeter2iu( 2.54 ) ),
            m_GridOffsetX( 0 ),
            m_GridOffsetY( 0 ),
            m_GridStagger( 1 ),
            m_GridStaggerType( 0 ),           // rows
            m_GridNumberingAxis( 0 ),         // h then v
            m_GridNumReverseAlt( false ),
            m_GridNumStartSet( 1 ),           // use specified start
            m_Grid2dArrayNumbering( 0 ),      // linear numbering
            m_GridPrimaryAxisScheme( 0 ),     // numeric
            m_GridSecondaryAxisScheme( 0 ),   // numeric
            m_GridPrimaryNumOffset( "1" ),    // numeric
            m_GridSecondaryNumOffset( "1" ),  // numeric
            m_GridPrimaryAxisStep( 1 ),
            m_GridSecondaryAxisStep( 1 ),
            m_CircCentreX( 0 ),
            m_CircCentreY( 0 ),
            m_CircAngle( 0.0 ),
            m_CircCount( 4 ),
            m_CircNumStartSet( 1 ),           // use specified start
            m_GridCircNumScheme( 0 ),
            m_CircNumberingOffset( "1" ),
            m_CircNumberingStep( 1 ),
            m_CircRotatationStep( false ),
            m_ArrayTypeTab( 0 )               // start on grid view
    {
    }

    bool     m_OptionsSet;

    long     m_GridNx;
    long     m_GridNy;
    long     m_GridDx;
    long     m_GridDy;
    long     m_GridOffsetX;
    long     m_GridOffsetY;
    long     m_GridStagger;

    long     m_GridStaggerType;
    long     m_GridNumberingAxis;
    bool     m_GridNumReverseAlt;
    long     m_GridNumStartSet;
    long     m_Grid2dArrayNumbering;
    long     m_GridPrimaryAxisScheme;
    long     m_GridSecondaryAxisScheme;
    wxString m_GridPrimaryNumOffset;
    wxString m_GridSecondaryNumOffset;
    long     m_GridPrimaryAxisStep;
    long     m_GridSecondaryAxisStep;

    long     m_CircCentreX;
    long     m_CircCentreY;
    long     m_CircAngle;
    long     m_CircCount;
    long     m_CircNumStartSet;
    long     m_GridCircNumScheme;
    wxString m_CircNumberingOffset;
    long     m_CircNumberingStep;
    bool     m_CircRotatationStep;
    long     m_ArrayTypeTab;
};

// Persistent options settings
static CREATE_ARRAY_DIALOG_ENTRIES s_arrayOptions;

/**
 * Local mapping for list-box <-> numbering type
 */
struct NUMBERING_LIST_DATA
{
    ARRAY_AXIS::NUMBERING_TYPE m_numbering_type;
    wxString                   m_label;
};

/**
 * List of type <--> name mappings (in order) for the numbering type
 * list boxes
 */
static const std::vector<NUMBERING_LIST_DATA> numberingTypeData {
    {
        ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
        _( "Numerals (0,1,2,...,9,10)" ),
    },
    {
        ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_HEX,
        _( "Hexadecimal (0,1,...,F,10,...)" ),
    },
    {
        ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_NO_IOSQXZ,
        _( "Alphabet, minus IOSQXZ" ),
    },
    {
        ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
        _( "Alphabet, full 26 characters" ),
    },
};

DIALOG_CREATE_ARRAY::DIALOG_CREATE_ARRAY( PCB_BASE_FRAME* aParent,
                                          std::unique_ptr<ARRAY_OPTIONS>& aSettings,
                                          bool enableNumbering, wxPoint aOrigPos ) :
        DIALOG_CREATE_ARRAY_BASE( aParent ),
        m_settings( aSettings ),
        m_originalItemPosition( aOrigPos ),
        m_numberingEnabled( enableNumbering ),
        m_hSpacing( aParent, m_labelDx, m_entryDx, m_unitLabelDx ),
        m_vSpacing( aParent, m_labelDy, m_entryDy, m_unitLabelDy ),
        m_hOffset( aParent, m_labelOffsetX, m_entryOffsetX, m_unitLabelOffsetX ),
        m_vOffset( aParent, m_labelOffsetY, m_entryOffsetY, m_unitLabelOffsetY ),
        m_hCentre( aParent, m_labelCentreX, m_entryCentreX, m_unitLabelCentreX ),
        m_vCentre( aParent, m_labelCentreY, m_entryCentreY, m_unitLabelCentreY ),
        m_circRadius( aParent, m_labelCircRadius, m_valueCircRadius, m_unitLabelCircRadius ),
        m_circAngle( aParent, m_labelCircAngle, m_entryCircAngle, m_unitLabelCircAngle ),
        m_cfg_persister( s_arrayOptions.m_OptionsSet )
{
    // Configure display origin transforms
    m_hSpacing.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
    m_vSpacing.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );
    m_hOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
    m_vOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );
    m_hCentre.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_vCentre.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    // Set up numbering scheme drop downs character set strings
    for( const auto& numData : numberingTypeData )
    {
        const wxString label = wxGetTranslation( numData.m_label );
        void*          clientData = (void*) &numData;

        m_choicePriAxisNumbering->Append( label, clientData );
        m_choiceSecAxisNumbering->Append( label, clientData );
        m_choiceCircNumbering->Append( label, clientData );
    }

    m_choicePriAxisNumbering->SetSelection( 0 );
    m_choiceSecAxisNumbering->SetSelection( 0 );
    m_choiceCircNumbering->SetSelection( 0 );

    m_circAngle.SetUnits( EDA_UNITS::DEGREES );

    // bind grid options to persister
    m_cfg_persister.Add( *m_entryNx, s_arrayOptions.m_GridNx );
    m_cfg_persister.Add( *m_entryNy, s_arrayOptions.m_GridNy );
    m_cfg_persister.Add( m_hSpacing, s_arrayOptions.m_GridDx );
    m_cfg_persister.Add( m_vSpacing, s_arrayOptions.m_GridDy );

    m_cfg_persister.Add( m_hOffset, s_arrayOptions.m_GridOffsetX );
    m_cfg_persister.Add( m_vOffset, s_arrayOptions.m_GridOffsetY );
    m_cfg_persister.Add( *m_entryStagger, s_arrayOptions.m_GridStagger );

    m_cfg_persister.Add( *m_radioBoxGridStaggerType, s_arrayOptions.m_GridStaggerType );

    m_cfg_persister.Add( *m_radioBoxGridNumberingAxis, s_arrayOptions.m_GridNumberingAxis );
    m_cfg_persister.Add( *m_checkBoxGridReverseNumbering, s_arrayOptions.m_GridNumReverseAlt );

    m_cfg_persister.Add( *m_rbGridStartNumberingOpt, s_arrayOptions.m_GridNumStartSet );
    m_cfg_persister.Add( *m_radioBoxGridNumberingScheme, s_arrayOptions.m_Grid2dArrayNumbering );
    m_cfg_persister.Add( *m_choicePriAxisNumbering, s_arrayOptions.m_GridPrimaryAxisScheme );
    m_cfg_persister.Add( *m_choiceSecAxisNumbering, s_arrayOptions.m_GridSecondaryAxisScheme );

    m_cfg_persister.Add( *m_entryGridPriNumberingOffset, s_arrayOptions.m_GridPrimaryNumOffset );
    m_cfg_persister.Add( *m_entryGridSecNumberingOffset, s_arrayOptions.m_GridSecondaryNumOffset );
    m_cfg_persister.Add( *m_entryGridPriNumberingStep, s_arrayOptions.m_GridPrimaryAxisStep );
    m_cfg_persister.Add( *m_entryGridSecNumberingStep, s_arrayOptions.m_GridSecondaryAxisStep );

    // bind circular options to persister
    m_cfg_persister.Add( m_hCentre, s_arrayOptions.m_CircCentreX );
    m_cfg_persister.Add( m_vCentre, s_arrayOptions.m_CircCentreY );
    m_cfg_persister.Add( m_circAngle, s_arrayOptions.m_CircAngle );
    m_cfg_persister.Add( *m_entryCircCount, s_arrayOptions.m_CircCount );
    m_cfg_persister.Add( *m_entryRotateItemsCb, s_arrayOptions.m_CircRotatationStep );

    m_cfg_persister.Add( *m_rbCircStartNumberingOpt, s_arrayOptions.m_CircNumStartSet );
    m_cfg_persister.Add( *m_choiceCircNumbering, s_arrayOptions.m_GridCircNumScheme );
    m_cfg_persister.Add( *m_entryCircNumberingStart, s_arrayOptions.m_CircNumberingOffset );
    m_cfg_persister.Add( *m_entryCircNumberingStep, s_arrayOptions.m_CircNumberingStep );

    m_cfg_persister.Add( *m_gridTypeNotebook, s_arrayOptions.m_ArrayTypeTab );

    m_cfg_persister.RestoreConfigToControls();

    // Run the callbacks once to process the dialog contents
    setControlEnablement();
    calculateCircularArrayProperties();

    m_stdButtonsOK->SetDefault();
    Fit();
    SetMinSize( GetSize() );
}


void DIALOG_CREATE_ARRAY::OnParameterChanged( wxCommandEvent& event )
{
    setControlEnablement();
    calculateCircularArrayProperties();
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
        err.Printf( _( "Bad numeric value for %s: %s" ), description, entry.GetValue() );
        errors.Add( err );
        ok = false;
    }

    return ok;
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
static bool validateAxisOptions( const wxTextCtrl& offsetEntry, const wxChoice& typeEntry,
                                 const wxTextCtrl& aStepEntry, ARRAY_AXIS& aAxis,
                                 wxArrayString& errors )
{
    void*                      clientData = typeEntry.GetClientData( typeEntry.GetSelection() );
    const NUMBERING_LIST_DATA* numberingData = static_cast<NUMBERING_LIST_DATA*>( clientData );

    wxCHECK_MSG( numberingData, false, "Failed to get client data from list control." );

    aAxis.SetAxisType( numberingData->m_numbering_type );

    const wxString text = offsetEntry.GetValue();

    bool ok = aAxis.SetOffset( text );

    if( !ok )
    {
        errors.Add( wxString::Format( _( "Could not determine numbering start from \"%s\": "
                                         "expected value consistent with alphabet \"%s\"" ),
                                      text,
                                      aAxis.GetAlphabet() ) );
        return false;
    }

    long step;
    ok = validateLongEntry( aStepEntry, step, _( "step value" ), errors );

    if( ok )
        aAxis.SetStep( step );

    return ok;
}


bool DIALOG_CREATE_ARRAY::TransferDataFromWindow()
{
    std::unique_ptr<ARRAY_OPTIONS> newSettings;

    wxArrayString   errors;
    const wxWindow* page = m_gridTypeNotebook->GetCurrentPage();

    if( page == m_gridPanel )
    {
        auto newGrid = std::make_unique<ARRAY_GRID_OPTIONS>();
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
            newGrid->SetNumberingStartIsSpecified( m_rbGridStartNumberingOpt->GetSelection() == 1 );

            if( newGrid->GetNumberingStartIsSpecified() )
            {
                newGrid->m_2dArrayNumbering = m_radioBoxGridNumberingScheme->GetSelection() != 0;

                // validate from the input fields
                bool numOk = validateAxisOptions( *m_entryGridPriNumberingOffset,
                        *m_choicePriAxisNumbering, *m_entryGridPriNumberingStep,
                        newGrid->m_pri_axis, errors );

                if( newGrid->m_2dArrayNumbering )
                {
                    numOk = validateAxisOptions( *m_entryGridSecNumberingOffset,
                                    *m_choiceSecAxisNumbering, *m_entryGridSecNumberingStep,
                                    newGrid->m_sec_axis, errors )
                            && numOk;
                }

                ok = ok && numOk;
            }
            else
            {
                // artificial linear numeric scheme from 1
                newGrid->m_2dArrayNumbering = false;
                newGrid->m_pri_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );
                newGrid->m_pri_axis.SetOffset( 1 );
            }
        }

        // Only use settings if all values are good
        if( ok )
            newSettings = std::move( newGrid );
    }
    else if( page == m_circularPanel )
    {
        auto newCirc = std::make_unique<ARRAY_CIRCULAR_OPTIONS>();
        bool ok = true;

        newCirc->m_centre.x = m_hCentre.GetValue();
        newCirc->m_centre.y = m_vCentre.GetValue();
        newCirc->m_angle = DoubleValueFromString( EDA_UNITS::DEGREES,
                                                  m_entryCircAngle->GetValue() );

        ok = ok && validateLongEntry(*m_entryCircCount, newCirc->m_nPts, _("point count"), errors);

        newCirc->m_rotateItems = m_entryRotateItemsCb->GetValue();
        newCirc->SetShouldNumber( m_numberingEnabled );

        if ( m_numberingEnabled )
        {
            newCirc->SetNumberingStartIsSpecified( m_rbCircStartNumberingOpt->GetSelection() == 1 );

            if( newCirc->GetNumberingStartIsSpecified() )
            {
                ok = ok
                     && validateAxisOptions( *m_entryCircNumberingStart, *m_choiceCircNumbering,
                             *m_entryCircNumberingStep, newCirc->m_axis, errors );
            }
            else
            {
                // artificial linear numeric scheme from 1
                newCirc->m_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );
                newCirc->m_axis.SetOffset( 1 ); // Start at "1"
            }
        }

        // Only use settings if all values are good
        if( ok )
            newSettings = std::move( newCirc );
    }

    // If we got good settings, send them out and finish
    if( newSettings )
    {
        // assign pointer and ownership here
        m_settings = std::move( newSettings );

        // persist the control state for next time
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
        // If we set the start number, we can set the other options,
        // otherwise it's a hardcoded linear array
        const bool use_set_start_grid = m_rbGridStartNumberingOpt->GetSelection() == 1;

        m_radioBoxGridNumberingScheme->Enable( use_set_start_grid );
        m_labelPriAxisNumbering->Enable( use_set_start_grid );
        m_choicePriAxisNumbering->Enable( use_set_start_grid );

        // Disable the secondary axis numbering option if the
        // numbering scheme doesn't have two axes
        const bool num2d = m_radioBoxGridNumberingScheme->GetSelection() != 0;

        m_labelSecAxisNumbering->Enable( use_set_start_grid && num2d );
        m_choiceSecAxisNumbering->Enable( use_set_start_grid && num2d );

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

        m_labelPriAxisNumbering->Enable( false );
        m_labelSecAxisNumbering->Enable( false );

        m_choiceSecAxisNumbering->Enable( false );
        m_choicePriAxisNumbering->Enable( false );

        m_labelGridNumberingOffset->Enable( false );
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