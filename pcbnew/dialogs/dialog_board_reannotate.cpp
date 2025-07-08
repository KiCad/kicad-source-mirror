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

#define SetSortCodes( DirArray, Code )                                     \
    {                                                                                                                  \
        g_SortYFirst = ( ( DirArray[Code] & SORTYFIRST ) != 0 );       \
        g_DescendingFirst  = ( ( DirArray[Code] & DESCENDINGFIRST ) != 0 );  \
        g_DescendingSecond = ( ( DirArray[Code] & DESCENDINGSECOND ) != 0 ); \
    }


wxString AnnotateString[] = {
    _( "All" ),          // ANNOTATE_ALL
    _( "Only front" ),   // AnnotateFront
    _( "Only back" ),    // AnnotateBack
    _( "Only selected" ) // ANNOTATE_SELECTED
};


wxString ActionMessage[] = {
    "",             // UPDATE_REFDES
    _( "Empty" ),   // EMPTY_REFDES
    _( "Invalid" ), // INVALID_REFDES
    _( "Excluded" ) // EXCLUDE_REFDES
};


DIALOG_BOARD_REANNOTATE::DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame )
        : DIALOG_BOARD_REANNOTATE_BASE( aParentFrame ),
          m_footprints( aParentFrame->GetBoard()->Footprints() )
{
    m_frame  = aParentFrame;
    m_Config = Kiface().KifaceSettings();
    InitValues();

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

    m_sdbSizerOK->SetLabel( _( "Reannotate PCB" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizer->Layout();

    m_settings = aParentFrame->config();
    wxArrayString gridslist;
    GRID_MENU::BuildChoiceList( &gridslist, m_settings, aParentFrame );

    if( -1 == m_gridIndex ) // If no default loaded
        m_gridIndex = m_settings->m_Window.grid.last_size_idx;        // Get the current grid size

    m_sortGridx = m_frame->GetCanvas()->GetGAL()->GetGridSize().x;
    m_sortGridy = m_frame->GetCanvas()->GetGAL()->GetGridSize().y;

    m_GridChoice->Set( gridslist );
    m_GridChoice->SetSelection( m_gridIndex );

    // Ensure m_sortCode is a valid value (0 .. m_sortButtons.size()-1)
    m_sortCode = std::max( 0, m_sortCode );
    m_sortCode = std::min( m_sortCode, (int)m_sortButtons.size()-1 );

    for( wxRadioButton* button : m_sortButtons )
        button->SetValue( false );

    m_selection = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( !m_selection.Empty() )
        m_annotationScope = ANNOTATE_SELECTED;

    // Ensure m_annotationScope is a valid value (0 .. m_scopeRadioButtons.size()-1)
    m_annotationScope = std::max( 0, m_annotationScope );
    m_annotationScope = std::min( m_annotationScope, (int)m_scopeRadioButtons.size()-1 );

    for( wxRadioButton* button : m_scopeRadioButtons )
        button->SetValue( false );

    m_scopeRadioButtons.at( m_annotationScope )->SetValue( true );
    m_sortButtons.at( m_sortCode )->SetValue( true );

    m_ExcludeList->SetToolTip( m_ExcludeListText->GetToolTipText() );
    m_GridChoice->SetToolTip( m_SortGridText->GetToolTipText() );

    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    finishDialogSettings();
}


DIALOG_BOARD_REANNOTATE::~DIALOG_BOARD_REANNOTATE()
{
    GetParameters(); // Get the current menu settings

    PCBNEW_SETTINGS* cfg = nullptr;

    try
    {
        cfg = m_frame->GetPcbNewSettings();
    }
    catch( const std::runtime_error& e )
    {
        wxFAIL_MSG( e.what() );
    }

    if( cfg )
    {
        cfg->m_Reannotate.sort_on_fp_location = m_locationChoice->GetSelection() == 0;
        cfg->m_Reannotate.remove_front_prefix = m_RemoveFrontPrefix->GetValue();
        cfg->m_Reannotate.remove_back_prefix  = m_RemoveBackPrefix->GetValue();
        cfg->m_Reannotate.exclude_locked      = m_ExcludeLocked->GetValue();

        cfg->m_Reannotate.grid_index          = m_gridIndex;
        cfg->m_Reannotate.sort_code           = m_sortCode;
        cfg->m_Reannotate.annotation_choice   = m_annotationScope;
        cfg->m_Reannotate.report_severity     = m_severity;

        cfg->m_Reannotate.front_refdes_start  = m_FrontRefDesStart->GetValue();
        cfg->m_Reannotate.back_refdes_start   = m_BackRefDesStart->GetValue();
        cfg->m_Reannotate.front_prefix        = m_FrontPrefix->GetValue();
        cfg->m_Reannotate.back_prefix         = m_BackPrefix->GetValue();
        cfg->m_Reannotate.exclude_list        = m_ExcludeList->GetValue();
        cfg->m_Reannotate.report_file_name    = m_MessageWindow->GetFileName();
    }
}


void DIALOG_BOARD_REANNOTATE::InitValues( void )
{
    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();
    m_locationChoice->SetSelection( cfg->m_Reannotate.sort_on_fp_location ? 0 : 1 );
    m_RemoveFrontPrefix->SetValue( cfg->m_Reannotate.remove_front_prefix );
    m_RemoveBackPrefix->SetValue( cfg->m_Reannotate.remove_back_prefix );
    m_ExcludeLocked->SetValue( cfg->m_Reannotate.exclude_locked );

    m_gridIndex         = cfg->m_Reannotate.grid_index ;
    m_sortCode          = cfg->m_Reannotate.sort_code ;
    m_annotationScope   = cfg->m_Reannotate.annotation_choice ;
    m_severity          = cfg->m_Reannotate.report_severity;

    m_FrontRefDesStart->SetValue( cfg->m_Reannotate.front_refdes_start );
    m_BackRefDesStart->SetValue( cfg->m_Reannotate.back_refdes_start );
    m_FrontPrefix->SetValue( cfg->m_Reannotate.front_prefix );
    m_BackPrefix->SetValue( cfg->m_Reannotate.back_prefix );
    m_ExcludeList->SetValue( cfg->m_Reannotate.exclude_list );
    m_MessageWindow->SetFileName( cfg->m_Reannotate.report_file_name );
}


void DIALOG_BOARD_REANNOTATE::OnCloseClick( wxCommandEvent& event )
{
    EndDialog( wxID_OK );
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
    newtype.LastUsedRefDes = aStartRefDes - 1;
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
    if( m_frame->GetBoard()->IsEmpty() )
    {
        ShowReport( _( "No PCB to reannotate!" ), RPT_SEVERITY_ERROR );
        return;
    }

    GetParameters(); // Figure out how this is to be done

    if( ReannotateBoard() )
    {
        ShowReport( _( "PCB successfully reannotated" ), RPT_SEVERITY_ACTION );
        ShowReport( _( "PCB annotation changes should be synchronized with schematic using "
                       "the \"Update Schematic from PCB\" tool." ), RPT_SEVERITY_WARNING );
    }

    m_MessageWindow->SetLazyUpdate( false );
    m_MessageWindow->Flush( false );
    m_frame->GetCanvas()->Refresh(); // Redraw
    m_frame->OnModify();             // Need to save file on exit.
}


void DIALOG_BOARD_REANNOTATE::GetParameters()
{
    m_sortCode = 0; // Convert radio button to sort direction code

    for( wxRadioButton* sortbuttons : m_sortButtons )
    {
        if( sortbuttons->GetValue() )
            break;

        m_sortCode++;
    }

    if( m_sortCode >= (int) m_sortButtons.size() )
        m_sortCode = 0;

    m_frontPrefixString = m_FrontPrefix->GetValue();
    m_backPrefixString  = m_BackPrefix->GetValue();

    // Get the chosen sort grid for rounding
    m_gridIndex = m_GridChoice->GetSelection();

    m_sortGridx = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                             m_settings->m_Window.grid.grids[m_gridIndex].x );
    m_sortGridy = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                             m_settings->m_Window.grid.grids[m_gridIndex].y );

    m_annotationScope = ANNOTATE_ALL;

    for( wxRadioButton* button : m_scopeRadioButtons )
    {
        if( button->GetValue() )
            break;
        else
            m_annotationScope++;
    }

    // Ensure m_annotationScope value is valid
    if( m_annotationScope >= (int)m_scopeRadioButtons.size() )
        m_annotationScope = ANNOTATE_ALL;

    m_MessageWindow->SetLazyUpdate( true );
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
        return ( true );  // yes, its smaller

    if( X0 > X1 )
        return ( false ); // No its not

    if( Y0 < Y1 )
        return ( true );  // same but equal

    return ( false );
}


wxString DIALOG_BOARD_REANNOTATE::CoordTowxString( int aX, int aY )
{
    return wxString::Format( wxT( "%s, %s" ),
                             m_frame->MessageTextFromValue( aX ),
                             m_frame->MessageTextFromValue( aY ) );
}


void DIALOG_BOARD_REANNOTATE::ShowReport( const wxString& aMessage, SEVERITY aSeverity )
{
    wxStringTokenizer msgs( aMessage, wxT( "\n" ) );

    while( msgs.HasMoreTokens() )
        m_MessageWindow->Report( msgs.GetNextToken(), aSeverity );
}


void DIALOG_BOARD_REANNOTATE::LogChangePlan()
{
    int      i = 1;
    wxString message;

    message.Printf( _( "<br/>There are %i reference designator prefixes in use<br/>"
                       "**********************************************************<br/>" ),
                    (int) m_refDesPrefixInfos.size() );

    for( const REFDES_PREFIX_INFO& info : m_refDesPrefixInfos ) // Show all the types of refdes
        message += info.RefDesPrefix + ( ( i++ % 16 ) == 0 ? wxT( "<br/>" ) : wxS( " " ) );

    message += wxT( "<br/>" );

    if( !m_excludeArray.empty() )
    {
        wxString excludes;

        for( wxString& exclude : m_excludeArray ) // Show the refdes we are excluding
            excludes += exclude + wxS( " " );

        message += wxString::Format( _( "Excluding: %s from reannotation<br/>" ), excludes );
    }

    message += _( "<br/>Change Array\n***********************<br/>" );

    for( const REFDES_CHANGE& change : m_changeArray )
    {
        message += wxString::Format( wxT( "%s -> %s  %s %s\n" ),
                                     change.OldRefDesString,
                                     change.NewRefDes,
                                     ActionMessage[change.Action],
                                     change.Action != UPDATE_REFDES ? _( "(will be ignored)" ) : wxString( "" ) );
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}


void DIALOG_BOARD_REANNOTATE::LogFootprints( const wxString& aMessage,
                                             const std::vector<REFDES_INFO>& aFootprints )
{
    wxString message = aMessage;

    if( aFootprints.empty() )
        message += _( "<br/>No footprints" );
    else
    {
        int i = 1;

        if( m_locationChoice->GetSelection() == 0 )
            message += _( "<br/>*********** Sort on Footprint Coordinates ***********" );
        else
            message += _( "<br/>*********** Sort on Reference Coordinates ***********" );

        for( const REFDES_INFO& fp : aFootprints )
        {
            message += wxString::Format( _( "<br/>%d %s X, Y: %s; rounded X, Y: %s" ),
                                         i++,
                                         fp.RefDesString,
                                         CoordTowxString( fp.x, fp.y ),
                                         CoordTowxString( fp.roundedx, fp.roundedy ) );
        }
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}


bool DIALOG_BOARD_REANNOTATE::ReannotateBoard()
{
    std::vector<REFDES_INFO> BadRefDes;
    wxString                 message, badrefdes;
    STRING_FORMATTER         stringformatter;
    REFDES_CHANGE*           newref;
    NETLIST                  netlist;

    if( !BuildFootprintList( BadRefDes ) )
    {
        ShowReport( _( "Selected options resulted in errors! Change them and try again." ),
                    RPT_SEVERITY_ERROR );
        return false;
    }

    if( !BadRefDes.empty() )
    {
        message.Printf( _( "<br/>PCB has %d empty or invalid reference designations."
                           "<br/>Recommend running DRC with 'Test for parity between PCB and schematic' checked.\n" ),
                        (int) BadRefDes.size() );

        for( const REFDES_INFO& mod : BadRefDes )
        {
            badrefdes += wxString::Format( _( "<br/>RefDes: %s Footprint: %s:%s at %s on PCB." ),
                                           mod.RefDesString,
                                           mod.FPID.GetLibNickname().wx_str(),
                                           mod.FPID.GetLibItemName().wx_str(),
                                           CoordTowxString( mod.x, mod.y ) );
        }

        ShowReport( message + badrefdes + wxT( "\n" ), RPT_SEVERITY_WARNING );
        message += _( "Reannotate anyway?" );

        if( !IsOK( m_frame, message ) )
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

    int    errorcount = 0;
    size_t firstnum   = 0;

    m_frontFootprints.clear();
    m_backFootprints.clear();
    m_excludeArray.clear();
    m_footprints = m_frame->GetBoard()->Footprints();

    wxString exclude;

    // Break exclude list into words.
    for( auto thischar : m_ExcludeList->GetValue() )
    {
    	if( thischar == ' ' || thischar == ',' )
        {
            m_excludeArray.push_back( exclude );
            exclude.clear();
        }
        else
        {
            exclude += thischar;
        }
    }

    if( !exclude.empty() )      // last item to exclude
        m_excludeArray.push_back( exclude );

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
        fpData.roundedx     = RoundToGrid( fpData.x, m_sortGridx ); // Round to sort
        fpData.roundedy     = RoundToGrid( fpData.y, m_sortGridy );
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
                fpData.Action = INVALID_REFDES; // do not change ref des such as 12 or +1, or L
        }

        // Get the type (R, C, etc)
        fpData.RefDesPrefix = fpData.RefDesString.substr( 0, firstnum );

        for( wxString excluded : m_excludeArray )
        {
            if( excluded == fpData.RefDesPrefix ) // Am I supposed to exclude this type?
            {
                fpData.Action = EXCLUDE_REFDES; // Yes
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

    // Determine the sort order for the front.
    SetSortCodes( FrontDirectionsArray, m_sortCode );

    // Sort the front footprints.
    sort( m_frontFootprints.begin(), m_frontFootprints.end(), FootprintCompare );

    // Determine the sort order for the back.
    SetSortCodes( BackDirectionsArray, m_sortCode );

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

    for( REFDES_INFO fpData : m_frontFootprints )
    {
        if( fpData.Action == EXCLUDE_REFDES )
            excludedFootprints.push_back( fpData );
    }

    for( REFDES_INFO fpData : m_backFootprints )
    {
        if( fpData.Action == EXCLUDE_REFDES )
            excludedFootprints.push_back( fpData );
    }

    for( REFDES_INFO fpData : excludedFootprints )
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
                                                bool aRemovePrefix,
                                                std::vector<REFDES_INFO>& aBadRefDes )
{
    size_t   prefixsize = aPrefix.size();

    bool haveprefix = ( 0 != prefixsize );         // Do I have a prefix?
    bool addprefix  = haveprefix & !aRemovePrefix; // Yes- and I'm not removing it
    aRemovePrefix &= haveprefix;                   // Only remove if I have a prefix

    bool prefixpresent; // Prefix found

    wxString logstring = ( aFootprints.front().Front ) ? _( "<br/><br/>Front Footprints" )
                                                       : _( "<br/><br/>Back Footprints" );
    LogFootprints( logstring, aFootprints );

    if( aStartRefDes != 0 ) // Initialize the change array if present
    {
    	for( size_t i = 0; i < m_refDesPrefixInfos.size(); i++ )
            m_refDesPrefixInfos[i].LastUsedRefDes = aStartRefDes - 1;
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
