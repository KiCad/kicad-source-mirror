/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
// This converts the index into a sort code. Note that Back sort code will have left and right swapped.
//
int FrontDirectionsArray[] = {
    SORTYFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Top to bottom, left to right",  //  100
    SORTYFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  //"Top to bottom, right to left",  //  101
    SORTYFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Back to Front, left to right",  //  110
    SORTYFIRST + DESCENDINGFIRST + DESCENDINGSECOND, //"Back to Front, right to left",  //  111
    SORTXFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Left to right, Front to Back",  //  000
    SORTXFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  //"Left to right, Back to Front",  //  001
    SORTXFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Right to left, Front to Back",  //  010
    SORTXFIRST + DESCENDINGFIRST + DESCENDINGSECOND  //"Right to left, Back to Front",  //  011
};


//
// Back Left/Right is opposite because it is a mirror image (coordinates are from the top)
//
int BackDirectionsArray[] = {
    SORTYFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  //"Top to bottom, left to right",  //  101
    SORTYFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Top to bottom, right to left",  //  100
    SORTYFIRST + DESCENDINGFIRST + DESCENDINGSECOND, //"Bottom to top, left to right",  //  111
    SORTYFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Bottom to top, right to left",  //  110
    SORTXFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Left to right, top to bottom",  //  010
    SORTXFIRST + DESCENDINGFIRST + DESCENDINGSECOND, //"Left to right, bottom to top",  //  011
    SORTXFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Right to left, top to bottom",  //  000
    SORTXFIRST + ASCENDINGFIRST + DESCENDINGSECOND   //"Right to left, bottom to top",  //  001
};

#define SetSortCodes( DirArray, Code )                                     \
    {                                                                      \
        SortYFirst       = ( ( DirArray[Code] & SORTYFIRST ) != 0 );       \
        DescendingFirst  = ( ( DirArray[Code] & DESCENDINGFIRST ) != 0 );  \
        DescendingSecond = ( ( DirArray[Code] & DESCENDINGSECOND ) != 0 ); \
    }


wxString AnnotateString[] = {
    _( "All" ),          //AnnotateAll
    _( "Only front" ),   //AnnotateFront
    _( "Only back" ),    //AnnotateBack
    _( "Only selected" ) //AnnotateSelected
};


wxString ActionMessage[] = {
    "",             //UpdateRefDes
    _( "Empty" ),   //EmptyRefDes
    _( "Invalid" ), //InvalidRefDes
    _( "Excluded" ) //Exclude
};


DIALOG_BOARD_REANNOTATE::DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame )
        : DIALOG_BOARD_REANNOTATE_BASE( aParentFrame ),
        m_modules( aParentFrame->GetBoard()->Modules() )
{
    m_Config = Kiface().KifaceSettings();
    InitValues();

    m_frame      = aParentFrame;
    m_screen     = m_frame->GetScreen();
    m_Units      = m_frame->GetUserUnits();
    m_Standalone = !m_frame->TestStandalone(); //Do this here forces the menu on top

    if( m_Standalone )
    { //Only update the schematic if not in standalone mode
        m_UpdateSchematic->Enable( false );
        m_UpdateSchematic->SetValue( false );
    }

    m_FrontRefDesStart->SetValidator( wxTextValidator( wxFILTER_DIGITS ) );
    m_BackRefDesStart->SetValidator( wxTextValidator( wxFILTER_DIGITS ) );

    m_sdbSizerOK->SetLabel( _( "Reannotate PCB" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    m_Settings = aParentFrame->config();
    wxArrayString gridslist;
    GRID_MENU::BuildChoiceList( &gridslist, m_Settings,
                                aParentFrame->GetUserUnits() != EDA_UNITS::INCHES );

    if( -1 == m_GridIndex ) //If no default loaded
        m_GridIndex = m_Settings->m_Window.grid.last_size_idx;        //Get the current grid size

    m_SortGridx = m_frame->GetCanvas()->GetGAL()->GetGridSize().x;
    m_SortGridy = m_frame->GetCanvas()->GetGAL()->GetGridSize().y;

    m_GridChoice->Set( gridslist ); //Show the choice in the dialog
    m_GridChoice->SetSelection( m_GridIndex );

    for( wxRadioButton* button : m_sortButtons )
        button->SetValue( false );

    m_sortButtons[m_SortCode]->SetValue( true );

    m_selection = m_frame->GetToolManager()->GetTool<SELECTION_TOOL>()->GetSelection();

    if( !m_selection.Empty() )
        m_AnnotationChoice = AnnotationChoice::AnnotateSelected;

    for( wxRadioButton* button : AnnotateWhat )
        button->SetValue( false );

    m_AnnotationChoice = ( m_SortCode >= (int) AnnotateWhat.size() ) ?
                                 AnnotationChoice::AnnotateAll :
                                 m_AnnotationChoice;

    AnnotateWhat[m_AnnotationChoice]->SetValue( true );

    reannotate_down_right_bitmap->SetBitmap( KiBitmap( reannotate_right_down_xpm ) );
    reannotate_right_down_bitmap->SetBitmap( KiBitmap( reannotate_left_down_xpm ) );
    reannotate_down_left_bitmap->SetBitmap( KiBitmap( reannotate_right_up_xpm ) );
    reannotate_left_down_bitmap->SetBitmap( KiBitmap( reannotate_left_up_xpm ) );
    reannotate_up_right_bitmap->SetBitmap( KiBitmap( reannotate_down_left_xpm ) );
    reannotate_right_up_bitmap->SetBitmap( KiBitmap( reannotate_up_left_xpm ) );
    reannotate_up_left_bitmap->SetBitmap( KiBitmap( reannotate_down_right_xpm ) );
    reannotate_left_up_bitmap->SetBitmap( KiBitmap( reannotate_up_right_xpm ) );

    m_ExcludeList->SetToolTip( m_ExcludeListText->GetToolTipText() );
    m_GridChoice->SetToolTip( m_SortGridText->GetToolTipText() );

    if( m_MessageWindow->GetFileName().empty() )
    { //Set the reporter window filename to something sensible
        wxFileName fn = m_frame->GetBoard()->GetFileName();
        fn.SetName( "annotationreport" );
        fn.SetExt( "txt " );
        wxString fullname = fn.GetFullPath();
        m_MessageWindow->SetFileName( fullname );
    }

    m_MessageWindow->SetPrintInfo( false ); //Suppress the "Info: " prefix
}


DIALOG_BOARD_REANNOTATE::~DIALOG_BOARD_REANNOTATE()
{
    GetParameters(); //Get the current menu settings
    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();
    cfg->m_Reannotate.sort_on_modules      = m_SortOnModules->GetValue();
    cfg->m_Reannotate.remove_front_prefix  = m_RemoveFrontPrefix->GetValue();
    cfg->m_Reannotate.remove_back_prefix   = m_RemoveBackPrefix->GetValue();
    cfg->m_Reannotate.update_schematic     = m_UpdateSchematic->GetValue();
    cfg->m_Reannotate.exclude_locked       = m_ExcludeLocked->GetValue();

    cfg->m_Reannotate.grid_index           = m_GridIndex;
    cfg->m_Reannotate.sort_code            = m_SortCode;
    cfg->m_Reannotate.annotation_choice    = m_AnnotationChoice;
    cfg->m_Reannotate.report_severity      = m_Severity;

    cfg->m_Reannotate.front_refdes_start   = m_FrontRefDesStart->GetValue();
    cfg->m_Reannotate.back_refdes_start    = m_BackRefDesStart->GetValue();
    cfg->m_Reannotate.front_prefix         = m_FrontPrefix->GetValue();
    cfg->m_Reannotate.back_prefix          = m_BackPrefix->GetValue();
    cfg->m_Reannotate.exclude_list         = m_ExcludeList->GetValue();
    cfg->m_Reannotate.report_file_name     = m_MessageWindow->GetFileName();
}

///  Copy saved app settings to the dialog
void DIALOG_BOARD_REANNOTATE::InitValues( void )
{
    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();
    m_SortOnModules->SetValue( cfg->m_Reannotate.sort_on_modules );
    m_SortOnReference->SetValue( !cfg->m_Reannotate.sort_on_modules );
    m_RemoveFrontPrefix->SetValue( cfg->m_Reannotate.remove_front_prefix );
    m_RemoveBackPrefix->SetValue( cfg->m_Reannotate.remove_back_prefix );
    m_UpdateSchematic->SetValue( cfg->m_Reannotate.update_schematic );
    m_ExcludeLocked->SetValue( cfg->m_Reannotate.exclude_locked );

    m_GridIndex         = cfg->m_Reannotate.grid_index ;
    m_SortCode          = cfg->m_Reannotate.sort_code ;
    m_AnnotationChoice  = cfg->m_Reannotate.annotation_choice ;
    m_Severity          = cfg->m_Reannotate.report_severity;

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


//
///  Check to make sure the prefix (if there is one) is properly constructed
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

    GetParameters(); //Figure out how this is to be done
    MakeSampleText( warning );

    if( !IsOK( m_frame, warning ) )
        return;

    if( ReannotateBoard() )
        ShowReport( _( "PCB and schematic successfully reannotated" ), RPT_SEVERITY_ACTION );

    m_MessageWindow->SetLazyUpdate( false );
    m_MessageWindow->Flush( false );
    m_frame->GetCanvas()->Refresh(); //Redraw
    m_frame->OnModify();             //Need to save file on exit.
}


//
///  Make the text to summarize what is about to happen
void DIALOG_BOARD_REANNOTATE::MakeSampleText( wxString& aMessage )
{
    wxString tmp;

    aMessage.Printf( _( "\n%s components will be reannotated. " ),
                    _( AnnotateString[m_AnnotationChoice] ) );

    if( !m_ExcludeList->GetValue().empty() )
        aMessage += wxString::Format( _( "\nAny reference types %s will not be annotated." ),
                                      m_ExcludeList->GetValue() );

    if( m_ExcludeLocked->GetValue() )
        aMessage += wxString::Format( _( "\nLocked footprints will not be annotated" ) );

    if( !m_AnnotateBack->GetValue() )
        aMessage += wxString::Format( _( "\nFront components will start at %s" ),
                                      m_FrontRefDesStart->GetValue() );

    if( !m_AnnotateFront->GetValue() )
    {
        bool frontPlusOne = ( 0 == atoi( m_BackRefDesStart->GetValue() ) )
                            && !m_AnnotateBack->GetValue();

        aMessage += wxString::Format( _( "\nBack components will start at %s." ),
                                      frontPlusOne ? _( "the last front component + 1" ) :
                                      m_BackRefDesStart->GetValue() );
    }

    if( !m_FrontPrefix->GetValue().empty() )
    {
        if( m_RemoveFrontPrefix->GetValue() )
            aMessage += wxString::Format(
                    _( "\nFront components starting with %s will have the prefix removed." ),
                    m_FrontPrefix->GetValue() );
        else
            aMessage +=
                    wxString::Format( _( "\nFront components will have %s inserted as a prefix." ),
                            m_FrontPrefix->GetValue() );
    }

    if( !m_BackPrefix->GetValue().empty() )
    {
        if( m_RemoveBackPrefix->GetValue() )
            aMessage += wxString::Format(
                    _( "\nBack components starting with %s will have the prefix removed." ),
                    m_BackPrefix->GetValue() );
        else
            aMessage +=
                    wxString::Format( _( "\nBack components will have %s inserted as a prefix." ),
                                      m_BackPrefix->GetValue() );
    }

    aMessage += wxString::Format(
            _( "\nPrior to sorting by %s, the coordinates of which will be rounded to a %s, %s grid. " ),
            ( m_SortOnModules->GetValue() ? _( "footprints" ) : _( "references" ) ),
            MessageTextFromValue( m_Units, m_SortGridx, false ),
            MessageTextFromValue( m_Units, m_SortGridy, false ) );

    if( m_UpdateSchematic->GetValue() )
        aMessage += _( "\nThe schematic will be updated." );
    else
        aMessage += _( "\nThe schematic will not be updated." );

    ShowReport( aMessage, RPT_SEVERITY_INFO );
}


void DIALOG_BOARD_REANNOTATE::GetParameters()
{
    m_SortCode = 0; //Convert radio button to sort direction code

    for( wxRadioButton* sortbuttons : m_sortButtons )
    {
        if( sortbuttons->GetValue() )
            break;

        m_SortCode++;
    }

    if( m_SortCode >= (int) m_sortButtons.size() )
        m_SortCode = 0;

    m_FrontPrefixString = m_FrontPrefix->GetValue();
    m_BackPrefixString  = m_BackPrefix->GetValue();

    //Get the chosen sort grid for rounding
    m_GridIndex = m_GridChoice->GetSelection();

    if( m_GridIndex >= ( int ) m_Settings->m_Window.grid.sizes.size() )
    {
        m_SortGridx = DoubleValueFromString( EDA_UNITS::INCHES,
                                             m_Settings->m_Window.grid.user_grid_x, true );
        m_SortGridy = DoubleValueFromString( EDA_UNITS::INCHES,
                                             m_Settings->m_Window.grid.user_grid_y, true );
    }
    else
    {
        m_SortGridx = DoubleValueFromString( EDA_UNITS::INCHES,
                                             m_Settings->m_Window.grid.sizes[ m_GridIndex ], true );
        m_SortGridy = m_SortGridx;
    }

    int i = 0;

    for( wxRadioButton* button : AnnotateWhat )
    {
        if( button->GetValue() )
            break;
        else
            i++;
    }

    m_AnnotationChoice = ( i >= (int) AnnotateWhat.size() ) ? AnnotationChoice::AnnotateAll : i;

    m_MessageWindow->SetLazyUpdate( true );
}


//
/// Round an int coordinate to a suitable grid
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


//
///  Compare function used to compare ChangeArray element for sort
///  @return true is A < B
static bool ChangeArrayCompare( const RefDesChange& aA, const RefDesChange& aB )
{
    return ( aA.OldRefDesString < aB.OldRefDesString );
}


//
/// Compare function to sort modules.
/// @return true if the first coordinate should be before the second coordinate
static bool ModuleCompare( const RefDesInfo& aA, const RefDesInfo& aB )
{
    int X0 = aA.roundedx, X1 = aB.roundedx, Y0 = aA.roundedy, Y1 = aB.roundedy;

    if( SortYFirst ) //If sorting by Y then X, swap X and Y
    {
        std::swap( X0, Y0 );
        std::swap( X1, Y1 );
    }

    //If descending, same compare just swap directions
    if( DescendingFirst )
        std::swap( X0, X1 );

    if( DescendingSecond )
        std::swap( Y0, Y1 );

    if( X0 < X1 )
        return ( true ); //yes, its smaller

    if( X0 > X1 )
        return ( false ); //No its not

    if( Y0 < Y1 )
        return ( true ); //same but equal

    return ( false );
}


//
/// Convert coordinates to wxString
/// @return the string
wxString DIALOG_BOARD_REANNOTATE::CoordTowxString( int aX, int aY )
{
    return wxString::Format( "%s, %s", MessageTextFromValue( m_Units, aX, false ),
                             MessageTextFromValue( m_Units, aY, false ) );
}


//
/// Break report into strings separated by \n and sent to the reporter
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


//
/// Create an audit trail of the changes
void DIALOG_BOARD_REANNOTATE::LogChangePlan()
{
    int      i = 1;
    wxString message;

    message.Printf( _( "\n\nThere are %i "
                       " types of reference designations\n"
                       "**********************************************************\n" ),
            (int) m_RefDesTypes.size() );

    for( RefDesTypeStr Type : m_RefDesTypes ) //Show all the types of refdes
        message += Type.RefDesType + ( 0 == ( i++ % 16 ) ? "\n" : " " );

    if( !m_ExcludeArray.empty() )
    {
        message += _( "\nExcluding: " );
        for( wxString Exclude : m_ExcludeArray ) //Show the refdes we are excluding
            message += Exclude + " ";
        message += _( " from reannotation\n\n" );
    }

    message += _( "\n    Change Array\n***********************\n" );

    for( RefDesChange Change : m_ChangeArray )
    {
        message += wxString::Format( "%s -> %s  %s %s\n", Change.OldRefDesString, Change.NewRefDes,
                ActionMessage[Change.Action],
                ( std::string )( UpdateRefDes != Change.Action ? _( " will be ignored" ) : "" ) );
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}


//
/// Create a list of the modules and their coordinates
void DIALOG_BOARD_REANNOTATE::LogModules( wxString& aMessage, std::vector<RefDesInfo>& aModules )
{
    wxString message = aMessage;

    if( aModules.empty() )
        message += _( "\nNo modules" );
    else
    {
        int i = 1;

        message += wxString::Format(
                _( "\n*********** Sort on " )
                + ( std::string )( m_SortOnModules->GetValue() ? _( "Module" ) : _( "Reference" ) )
                + _( " Coordinates *******************" ) + _( "\nSort Code " )
                + std::to_string( m_SortCode ) );

        for( RefDesInfo mod : aModules )
        {
            message += wxString::Format( _( "\n%d %s Uuid: [%s], X, Y: %s, Rounded X, Y, %s" ),
                    i++, mod.RefDesString, mod.Uuid.AsString(), CoordTowxString( mod.x, mod.y ),
                    CoordTowxString( mod.roundedx, mod.roundedy ) );
        }
    }

    ShowReport( message, RPT_SEVERITY_INFO );
}

//
/// Actually reannotate the board
/// @return false if fail, true if success
bool DIALOG_BOARD_REANNOTATE::ReannotateBoard()
{
    std::string             payload;
    std::vector<RefDesInfo> BadRefDes;
    wxString                message, badrefdes;
    STRING_FORMATTER        stringformatter;
    RefDesChange*           newref;
    NETLIST                 netlist;

    if( !BuildModuleList( BadRefDes ) )
    {
        ShowReport( "Selected options resulted in errors! Change them and try again.",
                    RPT_SEVERITY_ERROR );
        return false;
    }

    if( !BadRefDes.empty() )
    {
        message.Printf(
                _( "\nPCB has %d empty or invalid reference designations "
                   "\nRecommend you run DRC with Test footprints against schematic checked.\n" ),
                (int) BadRefDes.size() );

        for( RefDesInfo mod : BadRefDes )
        {
            badrefdes += _( "\nRefdes: \"" ) + mod.RefDesString + _( "\" Module:" )
                         + mod.FPID.GetLibNickname() + ":" + mod.FPID.GetLibItemName();
            badrefdes += "at X, Y " + CoordTowxString( mod.x, mod.y ) + _( " on PCB " );
        }

        ShowReport( message + badrefdes + "\n", RPT_SEVERITY_WARNING );
        message += _( "Yes will attempt reannotate. Proceed?" );

        if( !IsOK( m_frame, message ) )
            return ( false );
    }

    payload.clear();           //If not updating schematic no netlist error

    if( m_UpdateSchematic->GetValue() )
    { //If updating schematic send a netlist

        for( MODULE* mod : m_modules )
        { // Create a netlist
            newref = GetNewRefDes( mod );

            if( nullptr == newref )
                return false; //Not found in changelist

            //add to the netlist
            netlist.AddComponent( new COMPONENT( mod->GetFPID(), newref->NewRefDes,
                                                 mod->GetValue(), mod->GetPath() ) );
        }

        netlist.Format( "pcb_netlist", &stringformatter, 0,
                        CTL_OMIT_FILTERS | CTL_OMIT_NETS | CTL_OMIT_FILTERS );

        payload = stringformatter.GetString(); //create netlist

        //Send netlist to eeSchema
        bool attemptreannotate =  m_frame->ReannotateSchematic( payload );

        if( !attemptreannotate )
        { //Didn't get a valid reply
            ShowReport( _( "\nReannotate failed!\n" ), RPT_SEVERITY_WARNING );
            return false;
        }

    } //If updating schematic

    bool reannotateok = payload.size( ) == 0;

    ShowReport( payload, reannotateok ? RPT_SEVERITY_ACTION : RPT_SEVERITY_ERROR );
    BOARD_COMMIT commit( m_frame );

    if( reannotateok )
    { //Only update if no errors

    	for( MODULE* mod : m_modules )
        { // Create a netlist
            newref = GetNewRefDes( mod );

            if( nullptr == newref )
                return false;

            commit.Modify( mod );                           //Make a copy for undo
            mod->SetReference( newref->NewRefDes );         //Update the PCB reference
            m_frame->GetCanvas()->GetView()->Update( mod ); //Touch the module
        }
    }

    commit.Push( "Geographic reannotation" );
    return reannotateok;
}


//
/// Build the module lists, sort it, filter for excludes, then build the change list
/// @returns true if success, false if errors
bool DIALOG_BOARD_REANNOTATE::BuildModuleList( std::vector<RefDesInfo>& aBadRefDes )
{
    bool annotateselected;
    bool annotatefront = m_AnnotateFront->GetValue(); //Unless only doing back
    bool annotateback  = m_AnnotateBack->GetValue();  //Unless only doing front
    bool skiplocked    = m_ExcludeLocked->GetValue();

    int          errorcount = 0;
    unsigned int backstartrefdes;

    size_t firstnum, changearraysize;

    m_FrontModules.clear();
    m_BackModules.clear();
    m_ExcludeArray.clear();
    m_modules = m_frame->GetBoard()->Modules();

    std::vector<KIID> selected;

    if( m_AnnotateSelection->GetValue() )
    {
        for( EDA_ITEM* item : m_selection )
        { //Get the timestamps of selected modules
            if( item->Type() == PCB_MODULE_T )
                selected.push_back( item->m_Uuid );
        }
    }

    annotateselected = !selected.empty();

    wxString exclude;

    for( auto thischar : m_ExcludeList->GetValue() )
    { //Break exclude list into words

    	if( ( ' ' == thischar ) || ( ',' == thischar ) )
        {
            m_ExcludeArray.push_back( exclude );
            exclude.clear();
        }
        else
            exclude += thischar;

        if( !exclude.empty() )
            m_ExcludeArray.push_back( exclude );
    }

    RefDesInfo thismodule;

    for( MODULE* mod : m_modules )
    {
        thismodule.Uuid         = mod->m_Uuid;
        thismodule.RefDesString = mod->GetReference();
        thismodule.FPID         = mod->GetFPID();
        thismodule.x            = m_SortOnModules->GetValue() ? mod->GetPosition().x :
                                                     mod->Reference().GetPosition().x;
        thismodule.y            = m_SortOnModules->GetValue() ? mod->GetPosition().y :
                                                     mod->Reference().GetPosition().y;
        thismodule.roundedx     = RoundToGrid( thismodule.x, m_SortGridx ); //Round to sort
        thismodule.roundedy     = RoundToGrid( thismodule.y, m_SortGridy );
        thismodule.Front        = mod->GetLayer() == F_Cu;
        thismodule.Action       = UpdateRefDes; //Usually good

        if( thismodule.RefDesString.IsEmpty() )
            thismodule.Action = EmptyRefDes;
        else
        {
            firstnum = thismodule.RefDesString.find_first_of( "0123456789" );

            if( std::string::npos == firstnum )
                thismodule.Action = InvalidRefDes; //do not change ref des such as 12 or +1, or L
        }

        //Get the type (R, C, etc)
        thismodule.RefDesType = thismodule.RefDesString.substr( 0, firstnum );

        for( wxString excluded : m_ExcludeArray )
        {
            if( excluded == thismodule.RefDesType ) //Am I supposed to exclude this type?
            {
                thismodule.Action = Exclude; //Yes
                break;
            }
        }

        if( ( thismodule.Front && annotateback ) ||       //If a front module and doing backs only
                ( !thismodule.Front && annotatefront ) || //If a back module and doing front only
                ( mod->IsLocked() && skiplocked ) )       //If excluding locked and it is locked
            thismodule.Action = Exclude;

        if( annotateselected )
        {                                //If onnly annotating selected c
            thismodule.Action = Exclude; //Assume it isn't selected

            for( KIID sel : selected )
            {
                if( thismodule.Uuid == sel )
                {                                     //Found in selected modules
                    thismodule.Action = UpdateRefDes; //Update it
                    break;
                }
            }
        }

        if( thismodule.Front )
            m_FrontModules.push_back( thismodule );
        else
            m_BackModules.push_back( thismodule );
    }

    SetSortCodes( FrontDirectionsArray, m_SortCode ); //Determine the sort order for the front
    sort( m_FrontModules.begin(), m_FrontModules.end(),ModuleCompare ); //Sort the front modules

    SetSortCodes( BackDirectionsArray, m_SortCode ); //Determine the sort order for the back
    sort( m_BackModules.begin(), m_BackModules.end(), ModuleCompare ); //Sort the back modules

    m_RefDesTypes.clear();
    m_ChangeArray.clear();
    backstartrefdes = atoi( m_BackRefDesStart->GetValue() );

    if( !m_FrontModules.empty() )
        BuildChangeArray( m_FrontModules, atoi( m_FrontRefDesStart->GetValue() ),
                          m_FrontPrefix->GetValue(), m_RemoveFrontPrefix->GetValue(), aBadRefDes );

    if( !m_BackModules.empty() )
        BuildChangeArray( m_BackModules, backstartrefdes, m_BackPrefix->GetValue(),
                          m_RemoveBackPrefix->GetValue(), aBadRefDes );

    if( !m_ChangeArray.empty() )
        sort( m_ChangeArray.begin(), m_ChangeArray.end(), ChangeArrayCompare );

    LogChangePlan();

    changearraysize = m_ChangeArray.size();

    for( size_t i = 0; i < changearraysize; i++ ) //Scan through for duplicates if update or skip
    {
        if( ( m_ChangeArray[i].Action != EmptyRefDes )
                && ( m_ChangeArray[i].Action != InvalidRefDes ) )
        {
            for( size_t j = i + 1; j < changearraysize; j++ )
            {
                if( m_ChangeArray[i].NewRefDes == m_ChangeArray[j].NewRefDes )
                {
                    ShowReport( "Duplicate instances of " + m_ChangeArray[j].NewRefDes,
                                RPT_SEVERITY_ERROR );

                    if( errorcount++ > MAXERROR )
                    {
                        ShowReport( _( "Aborted: too many errors " ), RPT_SEVERITY_ERROR );
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


//
/// Scan through the module arrays and create the from -> to array
void DIALOG_BOARD_REANNOTATE::BuildChangeArray( std::vector<RefDesInfo>& aModules,
                                                unsigned int aStartRefDes, wxString aPrefix,
                                                bool aRemovePrefix,
                                                std::vector<RefDesInfo>& aBadRefDes )
{
    size_t        i;
    RefDesChange  change;
    RefDesTypeStr newtype;

    wxString refdestype;
    size_t   prefixsize = aPrefix.size();

    bool haveprefix = ( 0 != prefixsize );         //Do I have a prefix?
    bool addprefix  = haveprefix & !aRemovePrefix; //Yes- and I'm not removing it
    aRemovePrefix &= haveprefix;                   //Only remove if I have a prefix

    bool prefixpresent; //Prefix found

    wxString logstring = ( aModules.front().Front ) ? _( "\n\nFront Modules" )
                                                    : _( "\n\nBack Modules" );
    LogModules( logstring, aModules );

    if( 0 != aStartRefDes ) //Initialize the change array if present
    	for( i = 0; i < m_RefDesTypes.size(); i++ )
            m_RefDesTypes[i].RefDesCount = aStartRefDes;

    for( RefDesInfo Mod : aModules )
    { //For each module
        change.Uuid            = Mod.Uuid;
        change.Action          = Mod.Action;
        change.OldRefDesString = Mod.RefDesString;
        change.NewRefDes       = Mod.RefDesString;
        change.Front           = Mod.Front;

        if( Mod.RefDesString.IsEmpty() )
            Mod.Action = EmptyRefDes;

        if( ( change.Action == EmptyRefDes ) || ( change.Action == InvalidRefDes ) )
        {
            m_ChangeArray.push_back( change );
            aBadRefDes.push_back( Mod );
            continue;
        }

        if( change.Action == UpdateRefDes )
        {
            refdestype    = Mod.RefDesType;
            prefixpresent = ( 0 == Mod.RefDesType.find( aPrefix ) );

            if( addprefix && !prefixpresent )
                Mod.RefDesType.insert( 0, aPrefix ); //Add prefix once only

            if( aRemovePrefix && prefixpresent ) //If there is a prefix remove it
                Mod.RefDesType.erase( 0, prefixsize );

            for( i = 0; i < m_RefDesTypes.size(); i++ ) //See if it is in the types array
                if( m_RefDesTypes[i].RefDesType == Mod.RefDesType ) //Found it!
                    break;

            if( i == m_RefDesTypes.size() )
            { //Wasn't in the types array so add it
                newtype.RefDesType  = Mod.RefDesType;
                newtype.RefDesCount = ( aStartRefDes == 0 ? 1 : aStartRefDes );
                m_RefDesTypes.push_back( newtype );
            }

            change.NewRefDes = m_RefDesTypes[i].RefDesType
                               + std::to_string( m_RefDesTypes[i].RefDesCount++ );
        }
        m_ChangeArray.push_back( change ); //Add to the change array
    }
}

//
/// @returns the new refdes for this module
RefDesChange* DIALOG_BOARD_REANNOTATE::GetNewRefDes( MODULE* aMod )
{
    size_t i;

    for( i = 0; i < m_ChangeArray.size(); i++ )
        if( aMod->m_Uuid == m_ChangeArray[i].Uuid )
            return ( &m_ChangeArray[i] );

    ShowReport( _( "Module not found in changelist " ) + aMod->GetReference(), RPT_SEVERITY_ERROR );
    return nullptr; //Should never happen
}
