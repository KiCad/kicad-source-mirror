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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialogs/dialog_create_array.h"

#include <wx/msgdlg.h>

#include <base_units.h>
#include <footprint.h>
#include <pcb_edit_frame.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tool/tool_manager.h>
#include <widgets/text_ctrl_eval.h>


/**
 * Struct containing the last-entered values for the dialog.
 */
struct CREATE_ARRAY_DIALOG_ENTRIES
{
    /**
     * Construct with some sensible defaults.
     * In future, this could be loaded from config?
     */
    CREATE_ARRAY_DIALOG_ENTRIES() {}

    bool     m_OptionsSet = true;

    long     m_GridNx                    = 5;
    long     m_GridNy                    = 5;
    long     m_GridDx                    = pcbIUScale.mmToIU( 2.54 );
    long     m_GridDy                    = pcbIUScale.mmToIU( 2.54 );
    long     m_GridOffsetX               = 0;
    long     m_GridOffsetY               = 0;
    long     m_GridStagger               = 1;
    bool     m_GridStaggerRows           = true;
    bool     m_GridPositionCentreOnItems = true;
    bool     m_GridPositionItemsInPlace  = true;

    bool     m_GridRenumberPads          = true;
    long     m_GridNumberingAxis         = 0;           // h then v
    bool     m_GridNumReverseAlt         = false;
    long     m_GridNumStartSet           = 1;           // use specified start
    long     m_Grid2dArrayNumbering      = 0;           // linear numbering
    long     m_GridPrimaryAxisScheme     = 0;           // numeric
    long     m_GridSecondaryAxisScheme   = 0;           // numeric
    wxString m_GridPrimaryNumOffset      = wxT( "1" );  // numeric
    wxString m_GridSecondaryNumOffset    = wxT( "1" );  // numeric
    long     m_GridPrimaryAxisStep       = 1;
    long     m_GridSecondaryAxisStep     = 1;

    long      m_CircCentreX              = 0;
    long      m_CircCentreY              = 0;
    EDA_ANGLE m_CircAngle                = ANGLE_90;
    long      m_CircleDirection          = 0;        // clockwise
    EDA_ANGLE m_CircOffsetAngle          = ANGLE_0;
    long      m_CircCount                = 4;
    bool      m_CircFullCircle           = 0;
    long      m_CircNumStartSet          = 1;        // use specified start
    long      m_GridCircNumScheme        = 0;
    wxString  m_CircNumberingOffset      = wxT("1");
    long      m_CircNumberingStep        = 1;
    bool      m_CircRotatationStep       = false;
    long      m_ArrayTypeTab             = 0;       // start on grid view
    bool      m_SelectionArrange         = false;
    bool      m_SelectionDuplicate       = true;    // Duplicate by default
    bool      m_FootprintKeepAnnotations = false;
    bool      m_FootprintReannotate      = true;    // Assign unique by default
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

DIALOG_CREATE_ARRAY::DIALOG_CREATE_ARRAY( PCB_BASE_FRAME*                 aParent,
                                          std::unique_ptr<ARRAY_OPTIONS>& aSettings,
                                          bool aIsFootprintEditor, const VECTOR2I& aOrigPos ) :
        DIALOG_CREATE_ARRAY_BASE( aParent ),
        m_frame( aParent ),
        m_settings( aSettings ),
        m_originalItemPosition( aOrigPos ), m_isFootprintEditor( aIsFootprintEditor ),
        m_hSpacing( aParent, m_labelDx, m_entryDx, m_unitLabelDx ),
        m_vSpacing( aParent, m_labelDy, m_entryDy, m_unitLabelDy ),
        m_hOffset( aParent, m_labelOffsetX, m_entryOffsetX, m_unitLabelOffsetX ),
        m_vOffset( aParent, m_labelOffsetY, m_entryOffsetY, m_unitLabelOffsetY ),
        m_hCentre( aParent, m_labelCentreX, m_entryCentreX, m_unitLabelCentreX ),
        m_vCentre( aParent, m_labelCentreY, m_entryCentreY, m_unitLabelCentreY ),
        m_circAngle( aParent, m_labelCircAngle, m_entryCircAngle, m_unitLabelCircAngle ),
        m_circOffset( aParent, m_labelCircOffset, m_entryCircOffset, m_unitLabelCircOffset ),
        m_cfg_persister( pcbIUScale, s_arrayOptions.m_OptionsSet )
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
    m_circOffset.SetUnits( EDA_UNITS::DEGREES );

    // bind grid options to persister
    m_cfg_persister.Add( *m_entryNx, s_arrayOptions.m_GridNx );
    m_cfg_persister.Add( *m_entryNy, s_arrayOptions.m_GridNy );
    m_cfg_persister.Add( m_hSpacing, s_arrayOptions.m_GridDx );
    m_cfg_persister.Add( m_vSpacing, s_arrayOptions.m_GridDy );

    m_cfg_persister.Add( m_hOffset, s_arrayOptions.m_GridOffsetX );
    m_cfg_persister.Add( m_vOffset, s_arrayOptions.m_GridOffsetY );
    m_cfg_persister.Add( *m_entryStagger, s_arrayOptions.m_GridStagger );

    m_cfg_persister.Add( *m_staggerRows, s_arrayOptions.m_GridStaggerRows );

    m_cfg_persister.Add( *m_rbItemsRemainInPlace, s_arrayOptions.m_GridPositionItemsInPlace );
    m_cfg_persister.Add( *m_rbCentreOnSource, s_arrayOptions.m_GridPositionCentreOnItems );

    m_cfg_persister.Add( *m_cbRenumberPads, s_arrayOptions.m_GridRenumberPads );
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

    m_cfg_persister.Add( *m_checkBoxFullCircle, s_arrayOptions.m_CircFullCircle );
    m_cfg_persister.Add( m_circAngle, s_arrayOptions.m_CircAngle );
    m_cfg_persister.Add( m_circOffset, s_arrayOptions.m_CircOffsetAngle );
    m_cfg_persister.Add( *m_rbCircDirection, s_arrayOptions.m_CircleDirection );
    m_cfg_persister.Add( *m_entryCircCount, s_arrayOptions.m_CircCount );
    m_cfg_persister.Add( *m_entryRotateItemsCb, s_arrayOptions.m_CircRotatationStep );

    m_cfg_persister.Add( *m_rbCircStartNumberingOpt, s_arrayOptions.m_CircNumStartSet );
    m_cfg_persister.Add( *m_choiceCircNumbering, s_arrayOptions.m_GridCircNumScheme );
    m_cfg_persister.Add( *m_entryCircNumberingStart, s_arrayOptions.m_CircNumberingOffset );
    m_cfg_persister.Add( *m_entryCircNumberingStep, s_arrayOptions.m_CircNumberingStep );

    m_cfg_persister.Add( *m_gridTypeNotebook, s_arrayOptions.m_ArrayTypeTab );

    m_cfg_persister.Add( *m_radioBtnArrangeSelection, s_arrayOptions.m_SelectionArrange );
    m_cfg_persister.Add( *m_radioBtnDuplicateSelection, s_arrayOptions.m_SelectionDuplicate );

    m_cfg_persister.Add( *m_radioBtnKeepRefs, s_arrayOptions.m_FootprintKeepAnnotations );
    m_cfg_persister.Add( *m_radioBtnUniqueRefs, s_arrayOptions.m_FootprintReannotate );

    m_cfg_persister.RestoreConfigToControls();

    // Run the callbacks once to process the dialog contents
    setControlEnablement();
    calculateCircularArrayProperties();

    SetupStandardButtons();
    Fit();
    SetMinSize( GetSize() );
}


DIALOG_CREATE_ARRAY::~DIALOG_CREATE_ARRAY()
{
}


void DIALOG_CREATE_ARRAY::OnParameterChanged( wxCommandEvent& event )
{
    if( m_checkBoxFullCircle->GetValue() && m_entryCircAngle == event.GetEventObject() )
    {
        return;
    }

    setControlEnablement();
    calculateCircularArrayProperties();
}


void DIALOG_CREATE_ARRAY::OnSelectCenterButton( wxCommandEvent& event )
{
    event.Skip();

    TOOL_MANAGER*    toolMgr = m_frame->GetToolManager();
    PCB_PICKER_TOOL* pickerTool = toolMgr->GetTool<PCB_PICKER_TOOL>();
    wxCHECK( pickerTool, /* void */ );

    // Hide, but do not close, the dialog
    Hide();

    if( event.GetEventObject() == m_btnSelectCenterItem )
    {
        toolMgr->RunAction( PCB_ACTIONS::selectItemInteractively,
                            PCB_PICKER_TOOL::INTERACTIVE_PARAMS { this, _( "Select center item..." ) } );
    }
    else if( event.GetEventObject() == m_btnSelectCenterPoint )
    {
        toolMgr->RunAction( PCB_ACTIONS::selectPointInteractively,
                            PCB_PICKER_TOOL::INTERACTIVE_PARAMS { this, _( "Select center point..." ) } );
    }
    else
    {
        wxFAIL_MSG( "Unknown event source" );
    }
}


void DIALOG_CREATE_ARRAY::OnAxisNumberingChange( wxCommandEvent& aEvent )
{
    // On an alphabet change, make sure the offset control is valid by default.

    const int newAlphabet = aEvent.GetSelection();

    wxCHECK( newAlphabet >= 0 && newAlphabet < static_cast<int>( numberingTypeData.size() ),
             /* void */ );

    const ARRAY_AXIS::NUMBERING_TYPE numberingType =
            numberingTypeData[newAlphabet].m_numbering_type;

    wxTextCtrl* matchingTextCtrl = nullptr;

    if( aEvent.GetEventObject() == m_choicePriAxisNumbering )
        matchingTextCtrl = m_entryGridPriNumberingOffset;
    else if( aEvent.GetEventObject() == m_choiceSecAxisNumbering )
        matchingTextCtrl = m_entryGridSecNumberingOffset;
    else if( aEvent.GetEventObject() == m_choiceCircNumbering )
        matchingTextCtrl = m_entryCircNumberingStart;

    wxCHECK( matchingTextCtrl, /* void */ );

    ARRAY_AXIS dummyAxis;
    dummyAxis.SetAxisType( numberingType );

    // If the text control has a valid value for the new alphabet, keep it
    // else reset to the first value in the new alphabet.

    const bool isAlreadyOK = dummyAxis.SetOffset( matchingTextCtrl->GetValue() );

    if( !isAlreadyOK )
    {
        dummyAxis.SetOffset( ARRAY_AXIS::TypeIsNumeric( numberingType ) ? 1 : 0 );
        matchingTextCtrl->SetValue( dummyAxis.GetItemNumber( 0 ) );
    }
}


// Implement the RECEIVER interface for the callback from the TOOL
void DIALOG_CREATE_ARRAY::UpdatePickedItem( const EDA_ITEM* aItem )
{
    if( aItem )
    {
        m_hCentre.SetValue( aItem->GetPosition().x );
        m_vCentre.SetValue( aItem->GetPosition().y );
    }

    Show( true );
}


void DIALOG_CREATE_ARRAY::UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint )
{
    if( aPoint )
    {
        m_hCentre.SetValue( aPoint->x );
        m_vCentre.SetValue( aPoint->y );
    }

    Show( true );
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

    wxCHECK_MSG( numberingData, false, wxT( "Failed to get client data from list control." ) );

    aAxis.SetAxisType( numberingData->m_numbering_type );

    const wxString text = offsetEntry.GetValue();

    bool ok = aAxis.SetOffset( text );

    if( !ok )
    {
        errors.Add( wxString::Format( _( "Could not determine numbering start from '%s': "
                                         "expected value consistent with alphabet '%s'." ),
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
        ok &= validateLongEntry(*m_entryNx, newGrid->m_nx, _("horizontal count"), errors);
        ok &= validateLongEntry(*m_entryNy, newGrid->m_ny, _("vertical count"), errors);

        newGrid->m_delta.x = m_hSpacing.GetIntValue();
        newGrid->m_delta.y = m_vSpacing.GetIntValue();

        newGrid->m_offset.x = m_hOffset.GetIntValue();
        newGrid->m_offset.y = m_vOffset.GetIntValue();

        newGrid->m_centred = m_rbCentreOnSource->GetValue();

        ok &= validateLongEntry(*m_entryStagger, newGrid->m_stagger, _("stagger"), errors);

        newGrid->m_stagger_rows = m_staggerRows->GetValue();

        newGrid->m_horizontalThenVertical = m_radioBoxGridNumberingAxis->GetSelection() == 0;
        newGrid->m_reverseNumberingAlternate = m_checkBoxGridReverseNumbering->GetValue();

        newGrid->SetShouldNumber( m_isFootprintEditor && m_cbRenumberPads->GetValue() );

        if( m_isFootprintEditor )
        {
            newGrid->SetNumberingStartIsSpecified( m_rbGridStartNumberingOpt->GetSelection() == 1 );

            if( newGrid->GetNumberingStartIsSpecified() )
            {
                newGrid->m_2dArrayNumbering = m_radioBoxGridNumberingScheme->GetSelection() != 0;

                // validate from the input fields
                bool numOk = validateAxisOptions( *m_entryGridPriNumberingOffset,
                                                  *m_choicePriAxisNumbering,
                                                  *m_entryGridPriNumberingStep,
                                                  newGrid->m_pri_axis, errors );

                if( newGrid->m_2dArrayNumbering )
                {
                    numOk &= validateAxisOptions( *m_entryGridSecNumberingOffset,
                                                  *m_choiceSecAxisNumbering,
                                                  *m_entryGridSecNumberingStep,
                                                  newGrid->m_sec_axis, errors );
                }

                ok &= numOk;
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
        auto   newCirc = std::make_unique<ARRAY_CIRCULAR_OPTIONS>();
        bool   ok = true;
        double angle = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_entryCircAngle->GetValue() );
        double offset = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_entryCircOffset->GetValue() );

        newCirc->m_centre.x = m_hCentre.GetIntValue();
        newCirc->m_centre.y = m_vCentre.GetIntValue();
        newCirc->m_angle = EDA_ANGLE( angle, DEGREES_T );
        newCirc->m_angleOffset = EDA_ANGLE( offset, DEGREES_T );
        newCirc->m_clockwise = m_rbCircDirection->GetSelection() == 0;

        ok = validateLongEntry(*m_entryCircCount, newCirc->m_nPts, _("point count"), errors);

        newCirc->m_rotateItems = m_entryRotateItemsCb->GetValue();
        newCirc->SetShouldNumber( m_isFootprintEditor );

        if( m_isFootprintEditor )
        {
            newCirc->SetNumberingStartIsSpecified( m_rbCircStartNumberingOpt->GetSelection() == 1 );

            if( newCirc->GetNumberingStartIsSpecified() )
            {
                ok &= validateAxisOptions( *m_entryCircNumberingStart, *m_choiceCircNumbering,
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

    bool ret = false;

    // If we got good settings, send them out and finish
    if( newSettings )
    {
        // assign pointer and ownership here
        m_settings = std::move( newSettings );

        m_settings->SetShouldArrangeSelection( m_radioBtnArrangeSelection->GetValue() );
        m_settings->SetSShouldReannotateFootprints( m_radioBtnUniqueRefs->GetValue() );

        // persist the control state for next time
        m_cfg_persister.ReadConfigFromControls();

        ret = true;
    }
    else
    {
        wxString errorStr;

        if( errors.IsEmpty() )
            errorStr = _("Bad parameters");
        else
            errorStr = wxJoin( errors, '\n' );

        wxMessageBox( errorStr );
        ret = false;
    }

    // This dialog is not modal, so close it now if successful
    if( ret )
        Close();

    return ret;
}


void DIALOG_CREATE_ARRAY::setControlEnablement()
{
    if( m_checkBoxFullCircle->GetValue() )
    {
        m_entryCircAngle->Disable();
    }
    else
    {
        m_entryCircAngle->Enable();
    }

    if( m_isFootprintEditor )
    {
        m_footprintReannotatePanel->Show( false );

        m_gridPadNumberingPanel->Show( true );
        m_circularPadNumberingPanel->Show( true );

        // In no pad re-numbering, everything is disabled
        bool renumber_pads = m_cbRenumberPads->GetValue();

        m_radioBoxGridNumberingAxis->Enable( renumber_pads );
        m_checkBoxGridReverseNumbering->Enable( renumber_pads );
        m_rbGridStartNumberingOpt->Enable( renumber_pads );

        // If we set the start number, we can set the other options,
        // otherwise it's a hardcoded linear array
        const bool use_set_start_grid = renumber_pads && m_rbGridStartNumberingOpt->GetSelection() == 1;

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
        m_labelGridNumberingStep->Enable( use_set_start_grid );
        m_entryGridPriNumberingOffset->Enable( use_set_start_grid );
        m_entryGridPriNumberingStep->Enable( use_set_start_grid );
        m_entryGridSecNumberingOffset->Enable( use_set_start_grid && num2d );
        m_entryGridSecNumberingStep->Enable( use_set_start_grid && num2d );

        // disable the circular number offset in the same way
        const bool use_set_start_circ = renumber_pads && m_rbCircStartNumberingOpt->GetSelection() == 1;
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

        m_gridPadNumberingPanel->Show( false );

        // circular
        m_rbCircStartNumberingOpt->Enable( false );
        m_entryCircNumberingStart->Enable( false );

        m_circularPadNumberingPanel->Show( false );

        m_footprintReannotatePanel->Show( true );
    }

    if( m_radioBtnArrangeSelection->GetValue() )
    {
        m_footprintReannotatePanel->Show( false );
    }
}


void DIALOG_CREATE_ARRAY::calculateCircularArrayProperties()
{
    // In full circle mode, the division angle is computed from the number of points
    if( m_checkBoxFullCircle->GetValue() )
    {
        long nPts;
        if( m_entryCircCount->GetValue().ToLong( &nPts ) )
        {
            EDA_ANGLE division = EDA_ANGLE( 360, DEGREES_T ) / nPts;
            m_circAngle.SetAngleValue( division );
        }
    }
}
