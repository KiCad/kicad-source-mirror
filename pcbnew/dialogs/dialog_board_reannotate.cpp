/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <confirm.h>
#include <ctype.h>
#include <dialog_board_reannotate.h>
#include <fstream>
#include <kicad_string.h>  // StrNumCmp
#include <kiface_i.h>
#include <mail_type.h>
#include <pcbnew_settings.h>
#include <sstream>
#include <tool/tool_manager.h>
#include <tool/grid_menu.h>


bool SortYFirst;
bool DescendingFirst;
bool DescendingSecond;

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
    {                                                                      \
        SortYFirst       = ( ( DirArray[Code] & SORTYFIRST ) != 0 );       \
        DescendingFirst  = ( ( DirArray[Code] & DESCENDINGFIRST ) != 0 );  \
        DescendingSecond = ( ( DirArray[Code] & DESCENDINGSECOND ) != 0 ); \
    }


wxString AnnotateString[] = {
    _( "All" ),          // AnnotateAll
    _( "Only front" ),   // AnnotateFront
    _( "Only back" ),    // AnnotateBack
    _( "Only selected" ) // AnnotateSelected
};


wxString ActionMessage[] = {
    "",             // UpdateRefDes
    _( "Empty" ),   // EmptyRefDes
    _( "Invalid" ), // InvalidRefDes
    _( "Excluded" ) // Exclude
};


DIALOG_BOARD_REANNOTATE::DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame )
        : DIALOG_BOARD_REANNOTATE_BASE( aParentFrame ),
          m_footprints( aParentFrame->GetBoard()->Footprints() )
{
    m_Config = Kiface().KifaceSettings();
    InitValues();

    m_frame      = aParentFrame;
    m_screen     = m_frame->GetScreen();

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

    for( wxRadioButton* button : m_sortButtons )
        button->SetValue( false );

    m_sortButtons[m_sortCode]->SetValue( true );

    m_selection = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( !m_selection.Empty() )
        m_annotationChoice = AnnotationChoice::AnnotateSelected;

    for( wxRadioButton* button : AnnotateWhat )
        button->SetValue( false );

    m_annotationChoice = ( m_sortCode >= (int) AnnotateWhat.size() ) ?
                         AnnotationChoice::AnnotateAll :
                         m_annotationChoice;

    AnnotateWhat[m_annotationChoice]->SetValue( true );

    reannotate_down_right_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_right_down ) );
    reannotate_right_down_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_left_down ) );
    reannotate_down_left_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_right_up ) );
    reannotate_left_down_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_left_up ) );
    reannotate_up_right_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_down_left ) );
    reannotate_right_up_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_up_left ) );
    reannotate_up_left_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_down_right ) );
    reannotate_left_up_bitmap->SetBitmap( KiBitmap( BITMAPS::reannotate_up_right ) );

    m_ExcludeList->SetToolTip( m_ExcludeListText->GetToolTipText() );
    m_GridChoice->SetToolTip( m_SortGridText->GetToolTipText() );

    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    finishDialogSettings();
}


DIALOG_BOARD_REANNOTATE::~DIALOG_BOARD_REANNOTATE()
{
    GetParameters(); // Get the current menu settings
    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();
    cfg->m_Reannotate.sort_on_fp_location = m_locationChoice->GetSelection() == 0;
    cfg->m_Reannotate.remove_front_prefix     = m_RemoveFrontPrefix->GetValue();
    cfg->m_Reannotate.remove_back_prefix      = m_RemoveBackPrefix->GetValue();
    cfg->m_Reannotate.exclude_locked          = m_ExcludeLocked->GetValue();

    cfg->m_Reannotate.grid_index              = m_gridIndex;
    cfg->m_Reannotate.sort_code               = m_sortCode;
    cfg->m_Reannotate.annotation_choice       = m_annotationChoice;
    cfg->m_Reannotate.report_severity         = m_severity;

    cfg->m_Reannotate.front_refdes_start      = m_FrontRefDesStart->GetValue();
    cfg->m_Reannotate.back_refdes_start       = m_BackRefDesStart->GetValue();
    cfg->m_Reannotate.front_prefix            = m_FrontPrefix->GetValue();
    cfg->m_Reannotate.back_prefix             = m_BackPrefix->GetValue();
    cfg->m_Reannotate.exclude_list            = m_ExcludeList->GetValue();
    cfg->m_Reannotate.report_file_name        = m_MessageWindow->GetFileName();
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
    m_annotationChoice  = cfg->m_Reannotate.annotation_choice ;
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

    if( std::string::npos != tmps.find( lastc ) )
        return;

    tmps = aPrefix->GetValue();
    aPrefix->Clear();
    tmps.pop_back();
    aPrefix->AppendText( tmps );
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
    wxString warning;

    if( m_frame->GetBoard()->IsEmpty() )
    {
        ShowReport( _( "No PCB to reannotate!" ), RPT_SEVERITY_ERROR );
        return;
    }

    GetParameters(); // Figure out how this is to be done
    MakeSampleText( warning );

    if( !IsOK( m_frame, warning ) )
        return;

    if( ReannotateBoard() )
        ShowReport( _( "PCB and schematic successfully reannotated" ), RPT_SEVERITY_ACTION );

    m_MessageWindow->SetLazyUpdate( false );
    m_MessageWindow->Flush( false );
    m_frame->GetCanvas()->Refresh(); // Redraw
    m_frame->OnModify();             // Need to save file on exit.
}


void DIALOG_BOARD_REANNOTATE::MakeSampleText( wxString& aMessage )
{
    wxString tmp;

    aMessage.Printf( _( "\n%s footprints will be reannotated." ),
                     _( AnnotateString[m_annotationChoice] ) );

    if( !m_ExcludeList->GetValue().empty() )
    {
        aMessage += wxString::Format( _( "\nAny reference types %s will not be annotated." ),
                                      m_ExcludeList->GetValue() );
    }

    if( m_ExcludeLocked->GetValue() )
        aMessage += wxString::Format( _( "\nLocked footprints will not be annotated" ) );

    if( !m_AnnotateBack->GetValue() )
    {
        aMessage += wxString::Format( _( "\nFront footprints will start at %s" ),
                                      m_FrontRefDesStart->GetValue() );
    }

    if( !m_AnnotateFront->GetValue() )
    {
        bool frontPlusOne = ( 0 == wxAtoi( m_BackRefDesStart->GetValue() ) )
                            && !m_AnnotateBack->GetValue();

        aMessage += wxString::Format( _( "\nBack footprints will start at %s." ),
                                      frontPlusOne ? _( "the last front footprint + 1" ) :
                                      m_BackRefDesStart->GetValue() );
    }

    if( !m_FrontPrefix->GetValue().empty() )
    {
        if( m_RemoveFrontPrefix->GetValue() )
        {
            aMessage += wxString::Format( _( "\nFront footprints starting with '%s' will have "
                                             "the prefix removed." ),
                                          m_FrontPrefix->GetValue() );
        }
        else
        {
            aMessage += wxString::Format( _( "\nFront footprints will have '%s' inserted as a "
                                             "prefix." ),
                                          m_FrontPrefix->GetValue() );
        }
    }

    if( !m_BackPrefix->GetValue().empty() )
    {
        if( m_RemoveBackPrefix->GetValue() )
        {
            aMessage += wxString::Format( _( "\nBack footprints starting with '%s' will have the "
                                             "prefix removed." ),
                                          m_BackPrefix->GetValue() );
        }
        else
        {
            aMessage += wxString::Format( _( "\nBack footprints will have '%s' inserted as a "
                                             "prefix." ),
                                          m_BackPrefix->GetValue() );
        }
    }

    bool fpLocation = m_locationChoice->GetSelection() == 0;

    aMessage += wxString::Format( _( "\nPrior to sorting by %s, the coordinates of which will be "
                                     "rounded to a %s, %s grid." ),
                                  fpLocation ? _( "footprint location" )
                                             : _( "reference designator location" ),
                                  MessageTextFromValue( m_units, m_sortGridx ),
                                  MessageTextFromValue( m_units, m_sortGridy ) );

    ShowReport( aMessage, RPT_SEVERITY_INFO );
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

    if( m_gridIndex >= ( int ) m_settings->m_Window.grid.sizes.size() )
    {
        m_sortGridx = DoubleValueFromString( EDA_UNITS::MILS,
                                             m_settings->m_Window.grid.user_grid_x );
        m_sortGridy = DoubleValueFromString( EDA_UNITS::MILS,
                                             m_settings->m_Window.grid.user_grid_y );
    }
    else
    {
        m_sortGridx = DoubleValueFromString( EDA_UNITS::MILS,
                                             m_settings->m_Window.grid.sizes[ m_gridIndex ] );
        m_sortGridy = m_sortGridx;
    }

    int i = 0;

    for( wxRadioButton* button : AnnotateWhat )
    {
        if( button->GetValue() )
            break;
        else
            i++;
    }

    m_annotationChoice = ( i >= (int) AnnotateWhat.size() ) ? AnnotationChoice::AnnotateAll : i;

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
static bool ChangeArrayCompare( const RefDesChange& aA, const RefDesChange& aB )
{
    return ( StrNumCmp( aA.OldRefDesString, aB.OldRefDesString ) < 0 );
}


/// Compare function to sort footprints.
/// @return true if the first coordinate should be before the second coordinate
static bool ModuleCompare( const RefDesInfo& aA, const RefDesInfo& aB )
{
    int X0 = aA.roundedx, X1 = aB.roundedx, Y0 = aA.roundedy, Y1 = aB.roundedy;

    if( SortYFirst ) //If sorting by Y then X, swap X and Y
    {
        std::swap( X0, Y0 );
        std::swap( X1, Y1 );
    }

    // If descending, same compare just swap directions
    if( DescendingFirst )
        std::swap( X0, X1 );

    if( DescendingSecond )
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
    return wxString::Format( "%s, %s",
                             MessageTextFromValue( m_units, aX ),
                             MessageTextFromValue( m_units, aY ) );
}


void DIALOG_BOARD_REANNOTATE::ShowReport( wxString aMessage, SEVERITY aSeverity )
{
    size_t pos = 0, prev = 0;

    do
    {
        pos = aMessage.ToStdString().find( '\n', prev );
        m_MessageWindow->Report( aMessage.ToStdString().substr( prev, pos - prev ), aSeverity );
        prev = pos + 1;
    } while( std::string::npos != pos );

}


void DIALOG_BOARD_REANNOTATE::LogChangePlan()
{
    int      i = 1;
    wxString message;

    message.Printf( _( "\n\nThere are %i types of reference designations\n"
                       "**********************************************************\n" ),
                    (int) m_refDesTypes.size() );

    for( RefDesTypeStr Type : m_refDesTypes ) // Show all the types of refdes
        message += Type.RefDesType + ( 0 == ( i++ % 16 ) ? "\n" : " " );

    if( !m_excludeArray.empty() )
    {
        wxString excludes;

        for( wxString& exclude : m_excludeArray ) // Show the refdes we are excluding
            excludes += exclude + " ";

        message += wxString::Format( _( "\nExcluding: %s from reannotation\n\n" ), excludes );
    }

    message += _( "\n    Change Array\n***********************\n" );

    for( const RefDesChange& change : m_changeArray )
    {
        message += wxString::Format(
                "%s -> %s  %s %s\n", change.OldRefDesString, change.NewRefDes,
                ActionMessage[change.Action],
                UpdateRefDes != change.Action ? _( " will be ignored" ) : wxT( "" ) );
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}


void DIALOG_BOARD_REANNOTATE::LogFootprints( const wxString& aMessage,
                                             const std::vector<RefDesInfo>& aFootprints )
{
    wxString message = aMessage;

    if( aFootprints.empty() )
        message += _( "\nNo footprints" );
    else
    {
        int i = 1;
        bool fpLocations = m_locationChoice->GetSelection() == 0;

        message += wxString::Format( _( "\n*********** Sort on %s ***********" ),
                                     fpLocations ? _( "Footprint Coordinates" )
                                                 : _( "Reference Designator Coordinates" ) );

        message += wxString::Format( _( "\nSort Code %d" ), m_sortCode );

        for( const RefDesInfo& mod : aFootprints )
        {
            message += wxString::Format( _( "\n%d %s UUID: [%s], X, Y: %s, Rounded X, Y, %s" ),
                                         i++,
                                         mod.RefDesString,
                                         mod.Uuid.AsString(),
                                         CoordTowxString( mod.x, mod.y ),
                                         CoordTowxString( mod.roundedx, mod.roundedy ) );
        }
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}


bool DIALOG_BOARD_REANNOTATE::ReannotateBoard()
{
    std::vector<RefDesInfo> BadRefDes;
    wxString                message, badrefdes;
    STRING_FORMATTER        stringformatter;
    RefDesChange*           newref;
    NETLIST                 netlist;

    if( !BuildFootprintList( BadRefDes ) )
    {
        ShowReport( "Selected options resulted in errors! Change them and try again.",
                    RPT_SEVERITY_ERROR );
        return false;
    }

    if( !BadRefDes.empty() )
    {
        message.Printf(
                _( "\nPCB has %d empty or invalid reference designations."
                   "\nRecommend running DRC with 'Test footprints against schematic' checked.\n" ),
                (int) BadRefDes.size() );

        for( const RefDesInfo& mod : BadRefDes )
        {
            badrefdes += wxString::Format( _( "\nRefDes: %s Footprint: %s:%s at %s on PCB." ),
                                           mod.RefDesString,
                                           mod.FPID.GetLibNickname().wx_str(),
                                           mod.FPID.GetLibItemName().wx_str(),
                                           CoordTowxString( mod.x, mod.y ) );
        }

        ShowReport( message + badrefdes + "\n", RPT_SEVERITY_WARNING );
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

    commit.Push( "Geographic reannotation" );
    return true;
}


bool DIALOG_BOARD_REANNOTATE::BuildFootprintList( std::vector<RefDesInfo>& aBadRefDes )
{
    bool annotateSelected;
    bool annotateFront = m_AnnotateFront->GetValue(); // Unless only doing back
    bool annotateBack  = m_AnnotateBack->GetValue();  // Unless only doing front
    bool skipLocked    = m_ExcludeLocked->GetValue();

    int          errorcount = 0;
    unsigned int backstartrefdes;
    size_t       firstnum = 0;

    m_frontFootprints.clear();
    m_backFootprints.clear();
    m_excludeArray.clear();
    m_footprints = m_frame->GetBoard()->Footprints();

    std::vector<KIID> selected;

    if( m_AnnotateSelection->GetValue() )
    {
        for( EDA_ITEM* item : m_selection )
        {
            // Get the timestamps of selected footprints
            if( item->Type() == PCB_FOOTPRINT_T )
                selected.push_back( item->m_Uuid );
        }
    }

    annotateSelected = !selected.empty();

    wxString exclude;

    // Break exclude list into words.
    for( auto thischar : m_ExcludeList->GetValue() )
    {
    	if( ( ' ' == thischar ) || ( ',' == thischar ) )
        {
            m_excludeArray.push_back( exclude );
            exclude.clear();
        }
        else
            exclude += thischar;

        if( !exclude.empty() )
            m_excludeArray.push_back( exclude );
    }

    RefDesInfo fpData;
    bool       useModuleLocation = m_locationChoice->GetSelection() == 0;

    for( FOOTPRINT* footprint : m_footprints )
    {
        fpData.Uuid         = footprint->m_Uuid;
        fpData.RefDesString = footprint->GetReference();
        fpData.FPID         = footprint->GetFPID();
        fpData.x            = useModuleLocation ? footprint->GetPosition().x
                                                : footprint->Reference().GetPosition().x;
        fpData.y            = useModuleLocation ? footprint->GetPosition().y
                                                : footprint->Reference().GetPosition().y;
        fpData.roundedx     = RoundToGrid( fpData.x, m_sortGridx ); // Round to sort
        fpData.roundedy     = RoundToGrid( fpData.y, m_sortGridy );
        fpData.Front        = footprint->GetLayer() == F_Cu;
        fpData.Action       = UpdateRefDes; // Usually good

        if( fpData.RefDesString.IsEmpty() )
        {
            fpData.Action = EmptyRefDes;
        }
        else
        {
            firstnum = fpData.RefDesString.find_first_of( "0123456789" );

            if( std::string::npos == firstnum )
                fpData.Action = InvalidRefDes; // do not change ref des such as 12 or +1, or L
        }

        // Get the type (R, C, etc)
        fpData.RefDesType = fpData.RefDesString.substr( 0, firstnum );

        for( wxString excluded : m_excludeArray )
        {
            if( excluded == fpData.RefDesType ) // Am I supposed to exclude this type?
            {
                fpData.Action = Exclude; // Yes
                break;
            }
        }

        if(( fpData.Front && annotateBack ) ||            // If a front fp and doing backs only
                ( !fpData.Front && annotateFront ) ||     // If a back fp and doing front only
                ( footprint->IsLocked() && skipLocked ) ) // If excluding locked and it is locked
        {
            fpData.Action = Exclude;
        }

        if( annotateSelected )
        {                                // If only annotating selected c
            fpData.Action = Exclude;     // Assume it isn't selected

            for( KIID sel : selected )
            {
                if( fpData.Uuid == sel )
                {                                  // Found in selected footprints
                    fpData.Action = UpdateRefDes;  // Update it
                    break;
                }
            }
        }

        if( fpData.Front )
            m_frontFootprints.push_back( fpData );
        else
            m_backFootprints.push_back( fpData );
    }

    // Determine the sort order for the front.
    SetSortCodes( FrontDirectionsArray, m_sortCode );

    // Sort the front footprints.
    sort( m_frontFootprints.begin(), m_frontFootprints.end(), ModuleCompare );

    // Determine the sort order for the back.
    SetSortCodes( BackDirectionsArray, m_sortCode );

    // Sort the back footprints.
    sort( m_backFootprints.begin(), m_backFootprints.end(), ModuleCompare );

    m_refDesTypes.clear();
    m_changeArray.clear();
    backstartrefdes = wxAtoi( m_BackRefDesStart->GetValue() );

    if( !m_frontFootprints.empty() )
    {
        BuildChangeArray( m_frontFootprints, wxAtoi( m_FrontRefDesStart->GetValue() ),
                          m_FrontPrefix->GetValue(), m_RemoveFrontPrefix->GetValue(), aBadRefDes );
    }

    if( !m_backFootprints.empty() )
    {
        BuildChangeArray( m_backFootprints, backstartrefdes, m_BackPrefix->GetValue(),
                          m_RemoveBackPrefix->GetValue(), aBadRefDes );
    }

    if( !m_changeArray.empty() )
        sort( m_changeArray.begin(), m_changeArray.end(), ChangeArrayCompare );

    LogChangePlan();

    size_t changearraysize = m_changeArray.size();

    for( size_t i = 0; i < changearraysize; i++ ) // Scan through for duplicates if update or skip
    {
        if( ( m_changeArray[i].Action != EmptyRefDes )
                && ( m_changeArray[i].Action != InvalidRefDes ) )
        {
            for( size_t j = i + 1; j < changearraysize; j++ )
            {
                if( m_changeArray[i].NewRefDes == m_changeArray[j].NewRefDes )
                {
                    ShowReport( "Duplicate instances of " + m_changeArray[j].NewRefDes,
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

    return ( 0 == errorcount );
}


void DIALOG_BOARD_REANNOTATE::BuildChangeArray( std::vector<RefDesInfo>& aFootprints,
                                                unsigned int aStartRefDes, wxString aPrefix,
                                                bool aRemovePrefix,
                                                std::vector<RefDesInfo>& aBadRefDes )
{
    size_t        i;
    RefDesChange  change;
    RefDesTypeStr newtype;

    wxString refdestype;
    size_t   prefixsize = aPrefix.size();

    bool haveprefix = ( 0 != prefixsize );         // Do I have a prefix?
    bool addprefix  = haveprefix & !aRemovePrefix; // Yes- and I'm not removing it
    aRemovePrefix &= haveprefix;                   // Only remove if I have a prefix

    bool prefixpresent; // Prefix found

    wxString logstring = ( aFootprints.front().Front ) ? _( "\n\nFront Footprints" )
                                                       : _( "\n\nBack Footprints" );
    LogFootprints( logstring, aFootprints );

    if( 0 != aStartRefDes ) // Initialize the change array if present
    {
    	for( i = 0; i < m_refDesTypes.size(); i++ )
            m_refDesTypes[i].RefDesCount = aStartRefDes;
    }

    for( RefDesInfo fpData : aFootprints )
    {
        change.Uuid            = fpData.Uuid;
        change.Action          = fpData.Action;
        change.OldRefDesString = fpData.RefDesString;
        change.NewRefDes       = fpData.RefDesString;
        change.Front           = fpData.Front;

        if( fpData.RefDesString.IsEmpty() )
            fpData.Action = EmptyRefDes;

        if( ( change.Action == EmptyRefDes ) || ( change.Action == InvalidRefDes ) )
        {
            m_changeArray.push_back( change );
            aBadRefDes.push_back( fpData );
            continue;
        }

        if( change.Action == UpdateRefDes )
        {
            refdestype    = fpData.RefDesType;
            prefixpresent = ( 0 == fpData.RefDesType.find( aPrefix ) );

            if( addprefix && !prefixpresent )
                fpData.RefDesType.insert( 0, aPrefix ); // Add prefix once only

            if( aRemovePrefix && prefixpresent ) // If there is a prefix remove it
                fpData.RefDesType.erase( 0, prefixsize );

            for( i = 0; i < m_refDesTypes.size(); i++ ) // See if it is in the types array
            {
                if( m_refDesTypes[i].RefDesType == fpData.RefDesType ) // Found it!
                    break;
            }

            // Wasn't in the types array so add it
            if( i == m_refDesTypes.size() )
            {
                newtype.RefDesType  = fpData.RefDesType;
                newtype.RefDesCount = ( aStartRefDes == 0 ? 1 : aStartRefDes );
                m_refDesTypes.push_back( newtype );
            }

            change.NewRefDes = m_refDesTypes[i].RefDesType
                               + std::to_string( m_refDesTypes[i].RefDesCount++ );
        }

        m_changeArray.push_back( change );
    }
}


RefDesChange* DIALOG_BOARD_REANNOTATE::GetNewRefDes( FOOTPRINT* aFootprint )
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
