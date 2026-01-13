/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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

#include "dialog_board_reannotate.h"

#include <algorithm>
#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <confirm.h>
#include <ctype.h>
#include <gal/graphics_abstraction_layer.h>
#include <string_utils.h>  // StrNumCmp
#include <kiface_base.h>
#include <pcbnew_settings.h>
#include <refdes_utils.h>
#include <richio.h>
#include <tool/grid_menu.h>
#include <widgets/wx_html_report_panel.h>
#include <wx/valtext.h>


bool g_SortYFirst;
bool g_DescendingFirst;
bool g_DescendingSecond;

//
// This converts the index into a sort code. Note that Back sort code will have left and
// right swapped.
//
int FrontDirectionsArray[] = {
    SORTYFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   // "Top to bottom, left to right",  //  100
    SORTYFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  // "Top to bottom, right to left",  //  101
    SORTYFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  // "Back to Front, left to right",  //  110
    SORTYFIRST + DESCENDINGFIRST + DESCENDINGSECOND, // "Back to Front, right to left",  //  111
    SORTXFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   // "Left to right, Front to Back",  //  000
    SORTXFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  // "Left to right, Back to Front",  //  001
    SORTXFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  // "Right to left, Front to Back",  //  010
    SORTXFIRST + DESCENDINGFIRST + DESCENDINGSECOND  // "Right to left, Back to Front",  //  011
};


//
// Back Left/Right is opposite because it is a mirror image (coordinates are from the top)
//
int BackDirectionsArray[] = {
    SORTYFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  // "Top to bottom, left to right",  //  101
    SORTYFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   // "Top to bottom, right to left",  //  100
    SORTYFIRST + DESCENDINGFIRST + DESCENDINGSECOND, // "Bottom to top, left to right",  //  111
    SORTYFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  // "Bottom to top, right to left",  //  110
    SORTXFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  // "Left to right, top to bottom",  //  010
    SORTXFIRST + DESCENDINGFIRST + DESCENDINGSECOND, // "Left to right, bottom to top",  //  011
    SORTXFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   // "Right to left, top to bottom",  //  000
    SORTXFIRST + ASCENDINGFIRST + DESCENDINGSECOND   // "Right to left, bottom to top",  //  001
};

#define SetSortCodes( DirArray, Code )                                       \
    {                                                                        \
        g_SortYFirst = ( ( DirArray[Code] & SORTYFIRST ) != 0 );             \
        g_DescendingFirst  = ( ( DirArray[Code] & DESCENDINGFIRST ) != 0 );  \
        g_DescendingSecond = ( ( DirArray[Code] & DESCENDINGSECOND ) != 0 ); \
    }


wxString ActionMessage[] = {
    "",                                 // UPDATE_REFDES
    _( "(not updated)" ),               // EMPTY_REFDES
    _( "(unannotated; not updated)" ),  // INVALID_REFDES
    _( "(excluded)" )                   // EXCLUDE_REFDES
};


DIALOG_BOARD_REANNOTATE::DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame ) :
        DIALOG_BOARD_REANNOTATE_BASE( aParentFrame ),
        m_frame( aParentFrame ),
        m_footprints( aParentFrame->GetBoard()->Footprints() )
{
    // Init bitmaps associated to some wxRadioButton
    reannotate_down_right_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_right_down ) );
    reannotate_right_down_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_left_down ) );
    reannotate_down_left_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_right_up ) );
    reannotate_left_down_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_left_up ) );
    reannotate_up_right_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_down_left ) );
    reannotate_right_up_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_up_left ) );
    reannotate_up_left_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_down_right ) );
    reannotate_left_up_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::reannotate_up_right ) );

    m_FrontRefDesStart->SetValidator( wxTextValidator( wxFILTER_DIGITS ) );
    m_BackRefDesStart->SetValidator( wxTextValidator( wxFILTER_DIGITS ) );

    SetupStandardButtons( { { wxID_OK,     _( "Reannotate PCB" ) },
                            { wxID_CANCEL, _( "Close" )          } } );

    wxArrayString gridslist;
    GRID_MENU::BuildChoiceList( &gridslist, m_frame->GetWindowSettings( m_frame->config() ), aParentFrame );

    m_GridChoice->Set( gridslist );

    int gridIndex = m_frame->config()->m_Window.grid.last_size_idx;

    if( gridIndex >= 0 && gridIndex < (int) m_GridChoice->GetCount() )
        m_GridChoice->SetSelection( gridIndex );
    else
        m_GridChoice->SetSelection( 0 );

    m_ExcludeList->SetToolTip( m_ExcludeListText->GetToolTipText() );
    m_GridChoice->SetToolTip( m_SortGridText->GetToolTipText() );

    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    finishDialogSettings();
}


void DIALOG_BOARD_REANNOTATE::OnCloseClick( wxCommandEvent& event )
{
    EndDialog( wxID_OK );
}


bool DIALOG_BOARD_REANNOTATE::TransferDataToWindow()
{
    PCB_SELECTION selection = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( !selection.Empty() )
        m_AnnotateSelection->SetValue( true );

    // Ensure m_GridChoice selection validity
    // If not, the choice 0 is arbitrary
    if( m_GridChoice->GetSelection() < 0 || m_GridChoice->GetSelection() >= (int)m_GridChoice->GetCount() )
        m_GridChoice->SetSelection( 0 );

    return true;
}


void DIALOG_BOARD_REANNOTATE::FilterPrefix( wxTextCtrl* aPrefix )
{
    std::string tmps = VALIDPREFIX;

    if( aPrefix->GetValue().empty() )
        return; //Should never happen

    char lastc = aPrefix->GetValue().Last();

    if( isalnum( (int) lastc ) )
        return;

    if( tmps.find( lastc ) != std::string::npos )
        return;

    tmps = aPrefix->GetValue();
    aPrefix->Clear();
    tmps.pop_back();
    aPrefix->AppendText( tmps );
}


REFDES_PREFIX_INFO* DIALOG_BOARD_REANNOTATE::GetOrBuildRefDesInfo( const wxString& aRefDesPrefix,
                                                                   int aStartRefDes )
{
    for( size_t i = 0; i < m_refDesPrefixInfos.size(); i++ ) // See if it is in the info array
    {
        if( m_refDesPrefixInfos[i].RefDesPrefix == aRefDesPrefix ) // Found it!
            return &m_refDesPrefixInfos[i];
    }

    // Wasn't in the info array so add it
    REFDES_PREFIX_INFO newtype;
    newtype.RefDesPrefix = aRefDesPrefix;
    newtype.LastUsedRefDes = std::max( aStartRefDes - 1, 0 );
    m_refDesPrefixInfos.push_back( newtype );

    return &m_refDesPrefixInfos.back();
}


void DIALOG_BOARD_REANNOTATE::FilterFrontPrefix( wxCommandEvent& event )
{
    FilterPrefix( m_FrontPrefix );
}


void DIALOG_BOARD_REANNOTATE::FilterBackPrefix( wxCommandEvent& event )
{
    FilterPrefix( m_BackPrefix );
}


void DIALOG_BOARD_REANNOTATE::OnApplyClick( wxCommandEvent& event )
{
    m_MessageWindow->SetLazyUpdate( true );

    if( ReannotateBoard() )
    {
        ShowReport( _( "PCB successfully reannotated" ), RPT_SEVERITY_ACTION );
        ShowReport( _( "PCB annotation changes should be synchronized with schematic using "
                       "\"Update Schematic from PCB\"." ), RPT_SEVERITY_WARNING );
    }

    m_MessageWindow->SetLazyUpdate( false );
    m_MessageWindow->Flush( false );
    m_frame->GetCanvas()->Refresh(); // Redraw
    m_frame->OnModify();             // Need to save file on exit.
}


int DIALOG_BOARD_REANNOTATE::RoundToGrid( int aCoord, int aGrid )
{
    if( 0 == aGrid )
        aGrid = MINGRID;

    int rounder;
    rounder = aCoord % aGrid;
    aCoord -= rounder;

    if( abs( rounder ) > ( aGrid / 2 ) )
        aCoord += ( aCoord < 0 ? -aGrid : aGrid );

    return ( aCoord );
}


/// Compare function used to compare ChangeArray element for sort
/// @return true is A < B
static bool ChangeArrayCompare( const REFDES_CHANGE& aA, const REFDES_CHANGE& aB )
{
    return ( StrNumCmp( aA.OldRefDesString, aB.OldRefDesString ) < 0 );
}


/// Compare function to sort footprints.
/// @return true if the first coordinate should be before the second coordinate
static bool FootprintCompare( const REFDES_INFO& aA, const REFDES_INFO& aB )
{
    int X0 = aA.roundedx, X1 = aB.roundedx, Y0 = aA.roundedy, Y1 = aB.roundedy;

    if( g_SortYFirst ) //If sorting by Y then X, swap X and Y
    {
        std::swap( X0, Y0 );
        std::swap( X1, Y1 );
    }

    // If descending, same compare just swap directions
    if( g_DescendingFirst )
        std::swap( X0, X1 );

    if( g_DescendingSecond )
        std::swap( Y0, Y1 );

    if( X0 < X1 )
        return true;    // yes, it's smaller
    else if( X0 > X1 )
        return false;   // no, it's not
    else if( Y0 < Y1 )
        return true;
    else
        return false;
}


wxString empty_str()
{
    return wxT( "<i>" ) + _( "unannotated footprint" ) + wxT( "</i>" );
}


wxString unknown_str()
{
    return wxT( "<i>" ) + _( "unknown" ) + wxT( "</i>" );
}


wxString DIALOG_BOARD_REANNOTATE::CoordTowxString( int aX, int aY )
{
    return wxString::Format( wxT( "%s, %s" ),
                             m_frame->MessageTextFromValue( aX ),
                             m_frame->MessageTextFromValue( aY ) );
}


void DIALOG_BOARD_REANNOTATE::ShowReport( const wxString& aMessage, SEVERITY aSeverity )
{
    wxStringTokenizer msgs( aMessage, "\n" );

    while( msgs.HasMoreTokens() )
        m_MessageWindow->Report( msgs.GetNextToken(), aSeverity );
}


void DIALOG_BOARD_REANNOTATE::LogChangePlan()
{
    wxString message;

    message = _( "Reference Designator Prefixes in Use" );
    message += wxT( "<br/>-------------------------------------------------------------<br/>" );

    int i = 1;

    for( const REFDES_PREFIX_INFO& info : m_refDesPrefixInfos ) // Show all the types of refdes
        message += info.RefDesPrefix + ( ( i++ % 16 ) == 0 ? wxT( "<br/>" ) : wxS( " " ) );

    message += wxT( "<br/>" );

    if( !m_excludeArray.empty() )
    {
        wxString excludes;

        for( wxString& exclude : m_excludeArray ) // Show the refdes we are excluding
            excludes += exclude + wxS( " " );

        message += wxString::Format( _( "(Excluding %s from reannotation.)" ), excludes );
    }

    ShowReport( message + wxT( "<br/>" ), RPT_SEVERITY_INFO );

    message = _( "Change Log" );
    message += wxT( "<br/>-------------------------------------------------------------<br/>" );

    for( const REFDES_CHANGE& change : m_changeArray )
    {
        if( change.Action != UPDATE_REFDES )
        {
            message += wxString::Format( wxT( "%s  <i>%s</i><br/>" ),
                                         change.OldRefDesString.IsEmpty() ? empty_str() : change.OldRefDesString,
                                         ActionMessage[change.Action] );
        }
        else
        {
            message += wxString::Format( wxT( "%s -> %s<br/>" ),
                                         change.OldRefDesString.IsEmpty() ? empty_str() : change.OldRefDesString,
                                         change.NewRefDes.IsEmpty() ? empty_str() : change.NewRefDes );
        }
    }

    ShowReport( message, RPT_SEVERITY_ACTION );
}


void DIALOG_BOARD_REANNOTATE::LogFootprints( const std::vector<REFDES_INFO>& aFootprints )
{
    wxString message = aFootprints.front().Front ? _( "Front Footprints" ) : _( "Back Footprints" );
    message += ' ';

    if( m_locationChoice->GetSelection() == 0 )
        message += _( "(sorted by footprint location)" );
    else
        message += _( "(sorted by reference designator location)" );

    message += wxT( "<br/>-------------------------------------------------------------" );

    int i = 1;

    for( const REFDES_INFO& fp : aFootprints )
    {
        message += wxString::Format( _( "<br/>%d %s at %s (rounded to %s)" ),
                                     i++,
                                     fp.RefDesString.IsEmpty() ? empty_str() : fp.RefDesString,
                                     CoordTowxString( fp.x, fp.y ),
                                     CoordTowxString( fp.roundedx, fp.roundedy ) );
    }

    ShowReport( message + wxT( "<br/>" ), RPT_SEVERITY_INFO );
}


bool DIALOG_BOARD_REANNOTATE::ReannotateBoard()
{
    std::vector<REFDES_INFO> BadRefDes;
    wxString                 message1, message2, badrefdes;
    STRING_FORMATTER         stringformatter;
    REFDES_CHANGE*           newref;
    NETLIST                  netlist;

    if( !BuildFootprintList( BadRefDes ) )
    {
        ShowReport( _( "Selected options resulted in errors! Change them and try again." ), RPT_SEVERITY_ERROR );
        return false;
    }

    if( !BadRefDes.empty() )
    {
        message1.Printf( _( "PCB has %d empty or invalid reference designations." ), (int) BadRefDes.size() );
        message2.Printf( _( "You may wish to run DRC with 'Test for parity between PCB and schematic' checked." ) );

        for( const REFDES_INFO& mod : BadRefDes )
        {
            badrefdes += wxString::Format( _( "<br/>    RefDes: %s; footprint: %s at %s on PCB." ),
                                           mod.RefDesString.IsEmpty() ? empty_str() : mod.RefDesString,
                                           mod.FPID.IsValid() ? wxString( mod.FPID.Format().c_str() ) : unknown_str(),
                                           CoordTowxString( mod.x, mod.y ) );
        }

        ShowReport( message1 + wxT( "<br/>" ) + message2 + badrefdes + wxT( "<br/>" ), RPT_SEVERITY_WARNING );

        if( !IsOK( m_frame, message1 + "\n" + message2 + "\n \n" + _( "Reannotate anyway?" ) ) )
            return false;
    }

    BOARD_COMMIT commit( m_frame );

    for( FOOTPRINT* footprint : m_footprints )
    {
        newref = GetNewRefDes( footprint );

        if( nullptr == newref )
            return false;

        commit.Modify( footprint );                           // Make a copy for undo
        footprint->SetReference( newref->NewRefDes );         // Update the PCB reference
        m_frame->GetCanvas()->GetView()->Update( footprint ); // Touch the footprint
    }

    commit.Push( _( "Annotation" ) );
    return true;
}


bool DIALOG_BOARD_REANNOTATE::BuildFootprintList( std::vector<REFDES_INFO>& aBadRefDes )
{
    bool annotateSelected = m_AnnotateSelection->GetValue();
    bool annotateFront    = m_AnnotateFront->GetValue();
    bool annotateBack     = m_AnnotateBack->GetValue();
    bool skipLocked       = m_ExcludeLocked->GetValue();

    GRID sortGridMils = m_frame->config()->m_Window.grid.grids[ m_GridChoice->GetSelection() ];
    int  sortGridx = (int) EDA_UNIT_UTILS::UI::ValueFromString( pcbIUScale, EDA_UNITS::MILS, sortGridMils.x );
    int  sortGridy = (int) EDA_UNIT_UTILS::UI::ValueFromString( pcbIUScale, EDA_UNITS::MILS, sortGridMils.y );

    int    errorcount = 0;
    size_t firstnum   = 0;

    m_frontFootprints.clear();
    m_backFootprints.clear();
    m_excludeArray.clear();
    m_footprints = m_frame->GetBoard()->Footprints();

    wxStringTokenizer tokenizer( m_ExcludeList->GetValue(), ", \t\r\n", wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
        m_excludeArray.push_back( tokenizer.GetNextToken() );

    REFDES_INFO fpData;
    bool        useFPLocation = m_locationChoice->GetSelection() == 0;

    for( FOOTPRINT* footprint : m_footprints )
    {
        fpData.Uuid         = footprint->m_Uuid;
        fpData.RefDesString = footprint->GetReference();
        fpData.FPID         = footprint->GetFPID();
        fpData.x            = useFPLocation ? footprint->GetPosition().x
                                            : footprint->Reference().GetPosition().x;
        fpData.y            = useFPLocation ? footprint->GetPosition().y
                                            : footprint->Reference().GetPosition().y;
        fpData.roundedx     = RoundToGrid( fpData.x, sortGridx ); // Round to sort
        fpData.roundedy     = RoundToGrid( fpData.y, sortGridy );
        fpData.Front        = footprint->GetLayer() == F_Cu;
        fpData.Action       = UPDATE_REFDES; // Usually good

        if( fpData.RefDesString.IsEmpty() )
        {
            fpData.Action = EMPTY_REFDES;
        }
        else
        {
            firstnum = fpData.RefDesString.find_first_of( wxT( "0123456789" ) );

            if( std::string::npos == firstnum )
                fpData.Action = INVALID_REFDES;
        }

        // Get the type (R, C, etc)
        fpData.RefDesPrefix = fpData.RefDesString.substr( 0, firstnum );

        for( const wxString& excluded : m_excludeArray )
        {
            // If exclusion ends in *, apply it to entire refdes
            if( excluded.EndsWith( '*' ) )
            {
                if( fpData.RefDesString.Matches( excluded ) )
                {
                    fpData.Action = EXCLUDE_REFDES;
                    break;
                }
            }
            else if( excluded == fpData.RefDesPrefix )
            {
                fpData.Action = EXCLUDE_REFDES;
                break;
            }
        }

        if( footprint->IsLocked() && skipLocked )
            fpData.Action = EXCLUDE_REFDES;
        else if( annotateSelected )
            fpData.Action = footprint->IsSelected() ? UPDATE_REFDES : EXCLUDE_REFDES;
        else if( annotateFront )
            fpData.Action = fpData.Front ? UPDATE_REFDES : EXCLUDE_REFDES;
        else if( annotateBack )
            fpData.Action = fpData.Front ? EXCLUDE_REFDES : UPDATE_REFDES;

        if( fpData.Front )
            m_frontFootprints.push_back( fpData );
        else
            m_backFootprints.push_back( fpData );
    }

    int sortCode = 0; // Convert radio button to sort direction code

    for( wxRadioButton* sortbuttons : m_sortButtons )
    {
        if( sortbuttons->GetValue() )
            break;

        sortCode++;
    }

    if( sortCode >= (int) m_sortButtons.size() )
        sortCode = 0;

    // Determine the sort order for the front.
    SetSortCodes( FrontDirectionsArray, sortCode );

    // Sort the front footprints.
    sort( m_frontFootprints.begin(), m_frontFootprints.end(), FootprintCompare );

    // Determine the sort order for the back.
    SetSortCodes( BackDirectionsArray, sortCode );

    // Sort the back footprints.
    sort( m_backFootprints.begin(), m_backFootprints.end(), FootprintCompare );

    m_refDesPrefixInfos.clear();
    m_changeArray.clear();

    BuildUnavailableRefsList();

    if( !m_frontFootprints.empty() )
    {
        BuildChangeArray( m_frontFootprints, wxAtoi( m_FrontRefDesStart->GetValue() ),
                          m_FrontPrefix->GetValue(), m_RemoveFrontPrefix->GetValue(), aBadRefDes );
    }

    if( !m_backFootprints.empty() )
    {
        BuildChangeArray( m_backFootprints, wxAtoi( m_BackRefDesStart->GetValue() ),
                          m_BackPrefix->GetValue(), m_RemoveBackPrefix->GetValue(), aBadRefDes );
    }

    if( !m_changeArray.empty() )
        sort( m_changeArray.begin(), m_changeArray.end(), ChangeArrayCompare );

    LogChangePlan();

    size_t changearraysize = m_changeArray.size();

    for( size_t i = 0; i < changearraysize; i++ ) // Scan through for duplicates if update or skip
    {
        if( m_changeArray[i].Action != EMPTY_REFDES && m_changeArray[i].Action != INVALID_REFDES )
        {
            for( size_t j = i + 1; j < changearraysize; j++ )
            {
                if( m_changeArray[i].NewRefDes == m_changeArray[j].NewRefDes )
                {
                    ShowReport( wxString::Format( _( "Duplicate instances of %s" ), m_changeArray[j].NewRefDes ),
                                RPT_SEVERITY_ERROR );

                    if( errorcount++ > MAXERROR )
                    {
                        ShowReport( _( "Aborted: too many errors" ), RPT_SEVERITY_ERROR );
                        break;
                    }
                }
            }
        }

        if( errorcount > MAXERROR )
            break;
    }

    return ( errorcount == 0 );
}

void DIALOG_BOARD_REANNOTATE::BuildUnavailableRefsList()
{
    std::vector<REFDES_INFO> excludedFootprints;

    for( const REFDES_INFO& fpData : m_frontFootprints )
    {
        if( fpData.Action == EXCLUDE_REFDES )
            excludedFootprints.push_back( fpData );
    }

    for( const REFDES_INFO& fpData : m_backFootprints )
    {
        if( fpData.Action == EXCLUDE_REFDES )
            excludedFootprints.push_back( fpData );
    }

    for( const REFDES_INFO& fpData : excludedFootprints )
    {
        if( fpData.Action == EXCLUDE_REFDES )
        {
            REFDES_PREFIX_INFO* refDesInfo = GetOrBuildRefDesInfo( fpData.RefDesPrefix );
            refDesInfo->UnavailableRefs.insert( UTIL::GetRefDesNumber( fpData.RefDesString ) );
        }
    }
}


void DIALOG_BOARD_REANNOTATE::BuildChangeArray( std::vector<REFDES_INFO>& aFootprints,
                                                unsigned int aStartRefDes, const wxString& aPrefix,
                                                bool aRemovePrefix, std::vector<REFDES_INFO>& aBadRefDes )
{
    size_t   prefixsize = aPrefix.size();

    bool haveprefix = ( 0 != prefixsize );         // Do I have a prefix?
    bool addprefix  = haveprefix & !aRemovePrefix; // Yes- and I'm not removing it
    aRemovePrefix &= haveprefix;                   // Only remove if I have a prefix

    bool prefixpresent; // Prefix found

    LogFootprints( aFootprints );

    if( aStartRefDes != 0 ) // Initialize the change array if present
    {
        for( REFDES_PREFIX_INFO& prefixInfo : m_refDesPrefixInfos )
            prefixInfo.LastUsedRefDes = aStartRefDes - 1;
    }

    for( REFDES_INFO fpData : aFootprints )
    {
        REFDES_CHANGE change;

        change.Uuid            = fpData.Uuid;
        change.Action          = fpData.Action;
        change.OldRefDesString = fpData.RefDesString;
        change.NewRefDes       = fpData.RefDesString;
        change.Front           = fpData.Front;

        if( fpData.RefDesString.IsEmpty() )
            fpData.Action = EMPTY_REFDES;

        if( ( change.Action == EMPTY_REFDES ) || ( change.Action == INVALID_REFDES ) )
        {
            m_changeArray.push_back( change );
            aBadRefDes.push_back( fpData );
            continue;
        }

        if( change.Action == UPDATE_REFDES )
        {
            prefixpresent = ( fpData.RefDesPrefix.find( aPrefix ) == 0 );

            if( addprefix && !prefixpresent )
                fpData.RefDesPrefix.insert( 0, aPrefix ); // Add prefix once only

            if( aRemovePrefix && prefixpresent ) // If there is a prefix remove it
                fpData.RefDesPrefix.erase( 0, prefixsize );

            REFDES_PREFIX_INFO* refDesInfo = GetOrBuildRefDesInfo( fpData.RefDesPrefix, aStartRefDes );
            unsigned int  newRefDesNumber = refDesInfo->LastUsedRefDes + 1;

            while( refDesInfo->UnavailableRefs.count( newRefDesNumber ) )
                newRefDesNumber++;

            change.NewRefDes = refDesInfo->RefDesPrefix + std::to_string( newRefDesNumber );
            refDesInfo->LastUsedRefDes = newRefDesNumber;
        }

        m_changeArray.push_back( change );
    }
}


REFDES_CHANGE* DIALOG_BOARD_REANNOTATE::GetNewRefDes( FOOTPRINT* aFootprint )
{
    size_t i;

    for( i = 0; i < m_changeArray.size(); i++ )
    {
        if( aFootprint->m_Uuid == m_changeArray[i].Uuid )
            return ( &m_changeArray[i] );
    }

    ShowReport( _( "Footprint not found in changelist" ) + wxS( " " ) + aFootprint->GetReference(),
                RPT_SEVERITY_ERROR );

    return nullptr; // Should never happen
}
