/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "renum_type.h"
#include <base_units.h>
#include <confirm.h>
#include <ctype.h>
#include <dialog_board_renum.h>
#include <fstream>
#include <mail_type.h>

#define Swap( a, b ) \
    {                \
        int t;       \
        t = a;       \
        a = b;       \
        b = t;       \
    }

//
struct DIALOG_BOARD_RENUM_PARAMETERS
{
    DIALOG_BOARD_RENUM_PARAMETERS()
            : SortOnModules( true ),
              RemoveFrontPrefix( false ),
              RemoveBackPrefix( false ),
              WriteLogFile( false ),
              WriteChangeFile( false ),

              FrontStartRefDes( 1 ),
              BackStartRefDes( 0 ),
              SortDir( 0 ),
              SortGrid( 1.0 ),
              RenumDialog( NULL )

    {
    }

    bool SortOnModules; //Sort on modules/ref des
    bool RemoveFrontPrefix;
    bool RemoveBackPrefix;
    bool WriteLogFile;
    bool WriteChangeFile;

    unsigned int FrontStartRefDes; //The starting Front ref des;;
    unsigned int BackStartRefDes;  //The starting Back ref des
    int          SortDir;          //The sort code (left to right, etc.)

    double              SortGrid;    //The sort grid
    wxString            FrontPrefix; //The Front Prefix std::string
    wxString            BackPrefix;  //The Back Prefix std::string
    wxString            ExcludeList; //The Back Prefix std::string
    DIALOG_BOARD_RENUM* RenumDialog;

    wxString                   m_LogFile;
    wxString                   m_ChangeFile;
    std::vector<RefDesChange>  ChangeArray;
    std::vector<RefDesInfo>    FrontModules;
    std::vector<RefDesInfo>    BackModules;
    std::vector<RefDesTypeStr> RefDesTypes;
    std::vector<wxString>      ExcludeArray;
};

bool SortYFirst;
bool DescendingFirst;
bool DescendingSecond;

static DIALOG_BOARD_RENUM_PARAMETERS s_savedDialogParameters;

//
// This converts the index into a sort code. Note that Back sort code will have left and right swapped.
//
int FrontDirectionsArray[8] = {
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
int BackDirectionsArray[8] = {
    SORTYFIRST + ASCENDINGFIRST + DESCENDINGSECOND,  //"Top to bottom, left to right",  //  101
    SORTYFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Top to bottom, right to left",  //  100
    SORTYFIRST + DESCENDINGFIRST + DESCENDINGSECOND, //"Bottom to top, left to right",  //  111
    SORTYFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Bottom to top, right to left",  //  110
    SORTXFIRST + DESCENDINGFIRST + ASCENDINGSECOND,  //"Left to right, top to bottom",  //  010
    SORTXFIRST + DESCENDINGFIRST + DESCENDINGSECOND, //"Left to right, bottom to top",  //  011
    SORTXFIRST + ASCENDINGFIRST + ASCENDINGSECOND,   //"Right to left, top to bottom",  //  000
    SORTXFIRST + ASCENDINGFIRST + DESCENDINGSECOND   //"Right to left, bottom to top",  //  001
};

#define TraceReport( mess )              \
    {                                    \
        std::cerr << mess << std::flush; \
    }

DIALOG_BOARD_RENUM::DIALOG_BOARD_RENUM( PCB_EDIT_FRAME* aParentFrame )
        : DIALOG_BOARD_RENUM_BASE( aParentFrame ), m_modules( aParentFrame->GetBoard()->Modules() )
{
    s_savedDialogParameters.RenumDialog = this;
    m_frame                             = aParentFrame;
    m_board                             = aParentFrame->GetBoard();
    m_modules                           = m_board->Modules();

    s_savedDialogParameters.RemoveFrontPrefix &=
            ( 0 != s_savedDialogParameters.FrontPrefix.size() ); //Don't remove if nothing to remove
    s_savedDialogParameters.RemoveBackPrefix &= ( 0 != s_savedDialogParameters.BackPrefix.size() );

    m_FrontRefDesStart->ChangeValue( std::to_string( s_savedDialogParameters.FrontStartRefDes ) );
    m_FrontPrefix->ChangeValue( s_savedDialogParameters.FrontPrefix );
    m_RemoveFrontPrefix->SetValue( 0 == s_savedDialogParameters.RemoveFrontPrefix ? false : true );

    if( 0 == s_savedDialogParameters.BackStartRefDes )
        m_BackRefDesStart->ChangeValue( "" );
    else
        m_BackRefDesStart->ChangeValue( std::to_string( s_savedDialogParameters.BackStartRefDes ) );

    m_BackPrefix->ChangeValue( s_savedDialogParameters.BackPrefix );
    m_RemoveBackPrefix->SetValue( 0 == s_savedDialogParameters.RemoveBackPrefix ? false : true );
    m_SortGrid->ChangeValue( std::to_string( s_savedDialogParameters.SortGrid ) );
    m_WriteChangeFile->SetValue( 0 == s_savedDialogParameters.WriteChangeFile ? false : true );
    m_WriteLogFile->SetValue( 0 == s_savedDialogParameters.WriteLogFile ? false : true );
    m_SortOnModules->SetValue( s_savedDialogParameters.SortOnModules ? true : false );
    m_SortOnRefDes->SetValue( !s_savedDialogParameters.SortOnModules ? true : false );
    m_SortDir->SetSelection( s_savedDialogParameters.SortDir );
    m_ExcludeList->ChangeValue( s_savedDialogParameters.ExcludeList );
    //
    // Make labels for dialog to allow translation.
    //
}

void DIALOG_BOARD_RENUM::OnRenumberClick( wxCommandEvent& event )
{
    (void) event;

    wxFileName       filename;
    NETLIST          netlist;
    STRING_FORMATTER stringformatter;
    wxString         message;

    if( m_board->IsEmpty() )
    {
        ShowWarning( _( "\nNo Board to re-annotate!\n" ) );
        return;
    }

    if( !m_frame->TestStandalone() )
        return; //Not in standalone mode

    if( !IsOK( m_frame,
                _( "This operation will change the annotation of the PCB and schematic and cannot be undone. Proceed?" ) ) )
        return;

    GetParameters(); //Figure out how this is to be done
    this->m_LogFile.clear();

    std::vector<RefDesInfo> BadRefDes;
    BuildModuleList( BadRefDes );

    if( !BadRefDes.empty() )
    {
        message.Printf(
                _( "\n\nPCB has %d empty or invalid reference designations "
                   "\nRecommend you run DRC with Test footprints against schematic checked.\n" ),
                (int) BadRefDes.size() );

        wxString badrefdes;
        for( auto mod : BadRefDes )
        {
            badrefdes += _( "\nRefdes: \"" ) + mod.RefDesString + _( "\" Module:" )
                         + mod.FPID.GetLibNickname() + ":" + mod.FPID.GetLibItemName();
            badrefdes += "at X, Y " + CoordTowxString( mod.x, mod.y ) + _( " on PCB " );
        }
        ShowWarning( message + badrefdes + "\n\n" );
        message += _( "Yes will attempt renumber. Proceed?" );
        if( !IsOK( m_frame, message ) )
            return;
    }

    RefDesChange* newrefdes;

    for( auto mod : m_modules )
    { // Create a netlist
        newrefdes = GetNewRefDes( mod );
        if( UpdateRefDes == newrefdes->Action ) //Ignore blanks
            netlist.AddComponent(               //add to the netlist
                    new COMPONENT( mod->GetFPID(), newrefdes->NewRefDes, mod->GetValue(),
                            mod->GetPath() ) );
    }
    netlist.Format( "pcb_netlist", &stringformatter, 0,
            CTL_OMIT_FILTERS | CTL_OMIT_NETS | CTL_OMIT_FILTERS );

    std::string payload =
            stringformatter
                    .GetString(); //write netlist back to payload (eeschema will receive that as payload is sent here as reference)

    bool attemptrenum = m_frame->RenumberSchematic( payload, MAIL_RENUMBER );

    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    if( ( false == attemptrenum ) || ( 0 == payload.size() ) )
    { //Didn't get a valid reply
        ShowWarning( _( "\nRenumber failed!\n" ) );
    }
    else
    {
        if( 0 == payload.find( RENUM_OK ) )
        {
            m_MessageWindow->AppendText( payload ); //Give the result
            for( auto mod : m_modules )
            {
                newrefdes = GetNewRefDes( mod );
                if( UpdateRefDes == newrefdes->Action )
                {                                              //Ignore blanks
                    mod->SetReference( newrefdes->NewRefDes ); //Update the PCB reference
                    view->Update( mod );                       //Touch the module
                }
            }
            message = _( "\nPCB and schematic successfully renumbered\n" ); //Give the result
            m_MessageWindow->AppendText( message );                         //Give the result
            payload += message;
        }
        else
        {
            ShowWarning( payload );
        }
    }
    m_frame->GetCanvas()->Refresh(); //Redraw
    m_frame->OnModify();             //Need to save file on exit.
    FlushFiles();
}

void DIALOG_BOARD_RENUM::OKDone( wxCommandEvent& event )
{
    (void) event;
    Close( true );
}

void DIALOG_BOARD_RENUM::ShowWarning( const wxString& aMessage )
{
    m_MessageWindow->SetDefaultStyle( wxColour( "RED" ) );
    m_MessageWindow->AppendText( aMessage );
    m_MessageWindow->SetDefaultStyle( wxColour( "BLACK" ) );
    LogMessage( (wxString&) aMessage );
}

DIALOG_BOARD_RENUM::~DIALOG_BOARD_RENUM()
{
}

void DIALOG_BOARD_RENUM::GetParameters( void )
{
    s_savedDialogParameters.SortDir           = m_SortDir->GetSelection();
    s_savedDialogParameters.FrontStartRefDes  = atoi( m_FrontRefDesStart->GetValue().c_str() );
    s_savedDialogParameters.BackStartRefDes   = atoi( m_BackRefDesStart->GetValue().c_str() );
    s_savedDialogParameters.RemoveFrontPrefix = m_RemoveFrontPrefix->GetValue();
    s_savedDialogParameters.RemoveBackPrefix  = m_RemoveBackPrefix->GetValue();
    s_savedDialogParameters.FrontPrefix       = m_FrontPrefix->GetValue();
    s_savedDialogParameters.BackPrefix        = m_BackPrefix->GetValue();
    s_savedDialogParameters.ExcludeList       = m_ExcludeList->GetValue();

    s_savedDialogParameters.WriteChangeFile = m_WriteChangeFile->GetValue();
    s_savedDialogParameters.WriteLogFile    = m_WriteLogFile->GetValue();
    s_savedDialogParameters.SortOnModules   = m_SortOnModules->GetValue();

    if( !m_SortGrid->GetValue().ToDouble( &s_savedDialogParameters.SortGrid ) )
        s_savedDialogParameters.SortGrid = DEFAULT_GRID;
    s_savedDialogParameters.SortGrid =
            s_savedDialogParameters.SortGrid < MINGRID ? MINGRID : s_savedDialogParameters.SortGrid;
}
//
// Round an int coordinate to a suitable grid
//
int DIALOG_BOARD_RENUM::RoundToGrid( int aCoord )
{
    int rounder, sortgrid = (int) ( 1000000 * s_savedDialogParameters.SortGrid );

    rounder = aCoord % sortgrid;
    aCoord -= rounder;
    if( abs( rounder ) > ( sortgrid / 2 ) )
        aCoord += ( aCoord < 0 ? -sortgrid : sortgrid );
    return ( aCoord );
}
//
// Used to compare ChangeArray element for sort
//
static bool ChangeArrayCompare( const RefDesChange& aA, const RefDesChange& aB )
{
    return ( aA.OldRefDesString < aB.OldRefDesString );
}

//
// Use std::sort() to sort modules. Because it is a structure a compare function is needed
// Returns true if the first coordinate should be before the second coordinate
//

static bool ModuleCompare( const RefDesInfo& aA, const RefDesInfo& aB )
{
    int X0 = aA.roundedx, X1 = aB.roundedx, Y0 = aA.roundedy, Y1 = aB.roundedy;

    if( SortYFirst ) //If sorting by Y then X, swap X and Y
    {
        Swap( X0, Y0 );
        Swap( X1, Y1 );
    }

    if( DescendingFirst )
        Swap( X0, X1 ); //If descending, same compare just swap directions
    if( DescendingSecond )
        Swap( Y0, Y1 );

    if( X0 < X1 )
        return ( true ); //yes, its smaller
    if( X0 > X1 )
        return ( false ); //No its not
    if( Y0 < Y1 )
        return ( true ); //same but equal
    return ( false );
}

std::string DIALOG_BOARD_RENUM::CoordTowxString( int aX, int aY )
{
    EDA_UNITS units = s_savedDialogParameters.RenumDialog->m_frame->GetUserUnits();

    std::string coordstr = MessageTextFromValue( units, aX, false ).ToStdString();
    coordstr += ", " + MessageTextFromValue( units, aY, false ).ToStdString();
    return coordstr;
}

void DIALOG_BOARD_RENUM::FlushFiles( void )
{
    if( s_savedDialogParameters.WriteLogFile )
        WriteRenumFile( "_renumlog", this->m_LogFile ); //Write out the log file

    if( s_savedDialogParameters.WriteChangeFile )
        WriteRenumFile( "_renumchange", this->m_ChangeFile ); //Write out the change file
}

//
//Write the string to filename
//
void DIALOG_BOARD_RENUM::WriteRenumFile( const char* aFileType, wxString& aBuffer )
{
    wxFileName filename = s_savedDialogParameters.RenumDialog->m_board->GetFileName();

    filename.SetName( filename.GetName() + aFileType );
    filename.SetExt( "txt" );

    wxString      fullfilename = filename.GetFullPath().ToStdString();
    wxString      errmess      = _( "\n\nCan't write " ) + fullfilename;
    std::ofstream tmphandle(
            fullfilename.ToStdString(), std::ios::trunc ); //Open the file for writing

    if( tmphandle.is_open() )
        tmphandle << aBuffer; //Write the buffer
    if( !tmphandle.is_open() || tmphandle.bad() )
    { //Error?
        ShowWarning( errmess );
    }
    tmphandle.close();
}

void DIALOG_BOARD_RENUM::LogMessage( std::string& aMessage )
{
    if( !s_savedDialogParameters.WriteLogFile )
        return;
    this->m_LogFile += wxString( aMessage );
}

void DIALOG_BOARD_RENUM::LogMessage( wxString& aMessage )
{
    if( !s_savedDialogParameters.WriteLogFile )
        return;
    this->m_LogFile += aMessage;
}

void DIALOG_BOARD_RENUM::LogChangeArray( void )
{
    if( !s_savedDialogParameters.WriteChangeFile && !s_savedDialogParameters.WriteLogFile )
        return;

    this->m_ChangeFile =
            _( "\n\n    Change Array\n"
               "***********************\n" );
    for( auto Change : this->m_ChangeArray )
    { //Show all the types of refdes
        this->m_ChangeFile += Change.OldRefDesString + " -> " + Change.NewRefDes;

        if( EmptyRefDes == Change.Action )
            this->m_ChangeFile += _( " Empty" );
        if( InvalidRefDes == Change.Action )
            this->m_ChangeFile += _( " Invalid" );
        if( Exclude == Change.Action )
            this->m_ChangeFile += _( " Excluded" );
        if( UpdateRefDes != Change.Action )
            this->m_ChangeFile += _( " ref des will be ignored" );

        this->m_ChangeFile += "\n";
    }
    LogMessage( (wxString&) this->m_ChangeFile ); //Include in this->m_LogFile if logging
}

void DIALOG_BOARD_RENUM::LogExcludeList( void )
{
    if( 0 == this->m_ExcludeArray.size() )
        return;

    wxString message = _( "\nExcluding: " );

    for( auto Exclude : m_ExcludeArray ) //Show the refdes we are excluding
        message += Exclude + " ";

    message += _( " from reannotation\n\n" );
    LogMessage( message );
}

void DIALOG_BOARD_RENUM::LogRefDesTypes( void )
{
    int      i       = 1;
    wxString message = _( "\n\n\nThere are " ) + std::to_string( this->m_RefDesTypes.size() );

    message +=
            ( " types of reference designations\n"
              "**********************************************************\n" );

    for( auto Type : this->m_RefDesTypes ) //Show all the types of refdes
        message += Type.RefDesType + ( 0 == ( i++ % 16 ) ? "\n" : " " );

    message += "\n";
    LogMessage( message );
}

void DIALOG_BOARD_RENUM::LogModules( wxString& aMessage, std::vector<RefDesInfo>& aModules )
{

    int i = 1;

    wxString tmps,
            message = aMessage + _( "\n*********** Sort on " )
                      + ( std::string )( s_savedDialogParameters.SortOnModules ? _( "Module" ) :
                                                                                 _( "Ref Des" ) )
                      + _( " Coordinates *******************" ) + "\nSort Code "
                      + std::to_string( s_savedDialogParameters.SortDir );

    for( auto mod : aModules )
    {
        tmps.Printf( _( "\n%d %s Time Stamp: %X, X, Y, %s, Rounded X, Y, %s" ), i++,
                mod.RefDesString, mod.TimeStamp, CoordTowxString( mod.x, mod.y ),
                CoordTowxString( mod.roundedx, mod.roundedy ) );
        message += tmps;
    }
    LogMessage( message );
}
//CoordTowxStrin g(mod.x)

RefDesChange* DIALOG_BOARD_RENUM::GetNewRefDes( MODULE* aMod )
{
    size_t i;

    for( i = 0; i < this->m_ChangeArray.size(); i++ )
        if( aMod->GetTimeStamp() == this->m_ChangeArray[i].TimeStamp )
            break; //Found it

    if( i < this->m_ChangeArray.size() ) //Found
        return ( &this->m_ChangeArray[i] );

    wxString warning = _( "\nNot found: " ) + aMod->GetReference().ToStdString();
    ShowWarning( warning );
    return ( &this->m_ChangeArray[i] );
}

void DIALOG_BOARD_RENUM::BuildExcludeList( void )
{
    wxString excludethis = "";
    this->m_ExcludeArray.clear();
    for( auto thischar : s_savedDialogParameters.ExcludeList ) //Break exclude list into words
    {
        if( ( ' ' == thischar ) || ( ',' == thischar ) )
        {
            this->m_ExcludeArray.push_back( excludethis );
            excludethis.clear();
        }
        else
            excludethis += thischar;
        if( 0 != excludethis.size() )
            this->m_ExcludeArray.push_back( excludethis );
    }
}

void DIALOG_BOARD_RENUM::BuildModuleList( std::vector<RefDesInfo>& aBadRefDes )
{
    bool         sortonmodules = s_savedDialogParameters.SortOnModules;
    unsigned int refdesnum, maxrefdes = 0;

    size_t firstnum;

    wxPoint  referencecoordinates, modulecoordinates;
    wxString modulerefdes;
    wxString errmess;

    RefDesInfo    thismodule;
    RefDesTypeStr thisrefdestype;

    DIALOG_BOARD_RENUM_PARAMETERS renumdialog = s_savedDialogParameters;

    this->m_FrontModules.clear();
    this->m_BackModules.clear();

    for( auto mod : m_modules )
    {
        thismodule.RefDesString = mod->GetReference();
        thismodule.TimeStamp    = mod->GetTimeStamp();
        thismodule.FPID         = mod->GetFPID();
        refdesnum               = 0;

        firstnum = thismodule.RefDesString.find_first_of( "0123456789" );
        if( std::string::npos != firstnum )
            refdesnum = wxAtoi( thismodule.RefDesString.substr( firstnum ) );

        thismodule.x = sortonmodules ? mod->GetPosition().x : mod->Reference().GetPosition().x;
        thismodule.y = sortonmodules ? mod->GetPosition().y : mod->Reference().GetPosition().y;
        thismodule.roundedx = RoundToGrid( thismodule.x ); //Round to sort
        thismodule.roundedy = RoundToGrid( thismodule.y );
        thismodule.Front    = mod->GetLayer() == F_Cu ? true : false; //

        if( thismodule.Front )
        {
            if( refdesnum > maxrefdes ) //Get the highest refdes on the front for error check
                maxrefdes = refdesnum;  //Need to know max front refdes for error handling
            this->m_FrontModules.push_back( thismodule );
        }
        else
            this->m_BackModules.push_back( thismodule );
    }

    SetSortCodes( FrontDirectionsArray,
            s_savedDialogParameters.SortDir ); //Determine the sort order for the front
    sort( this->m_FrontModules.begin(), this->m_FrontModules.end(),
            ModuleCompare ); //Sort the front modules
    SetSortCodes( BackDirectionsArray,
            s_savedDialogParameters.SortDir ); //Determing the sort order for the back
    sort( this->m_BackModules.begin(), this->m_BackModules.end(),
            ModuleCompare ); //Sort the back modules

    //
    // If I'm not restarting from the front check if starting back refdes < MaxRefDes. If so, warn and correct
    //
    bool backprefixempty =
            s_savedDialogParameters.BackPrefix.empty(); //These are shortcut variables
    bool      removefrontprefix = s_savedDialogParameters.RemoveFrontPrefix;
    bool      removebackprefix  = s_savedDialogParameters.RemoveBackPrefix;
    wxString& FrontPrefix       = s_savedDialogParameters.FrontPrefix;
    wxString& BackPrefix        = s_savedDialogParameters.BackPrefix;

    unsigned int BackStartRefDes = s_savedDialogParameters.BackStartRefDes;
    wxString     warning;

    if( ( !backprefixempty && removebackprefix ) //If removing prefix have an error
            || ( !FrontPrefix.empty() && removefrontprefix ) )
    {
        //
        // If removing back prefrix, you can end up with (for example) two R1s because the former
        // B_R1 can become the R1. So I reset the back refdes start to front max + 1 and warn to do it
        // again.
        //
        warning = _( "**** \nRemoving Prefixes takes two passes ****\n"
                     "**** Overiding and renumbering back starting at %d ****\n" )
                  + std::to_string( maxrefdes )
                  + _( "**** Rerun again after this completes ********\n" );
        ShowWarning( warning );

        BackStartRefDes = maxrefdes + 1;
    }
    else if( ( BackStartRefDes != 0 ) && ( BackStartRefDes <= maxrefdes ) )
    {
        if( ( backprefixempty )                  //Doesn't matter if I'm using a back prefix
                || ( BackPrefix == FrontPrefix ) //It does if same as front prefix
                || removebackprefix )
        { //It does if removing it

            warning =
                    _( "\nWarning: Back Ref Des Start < highest Front Ref Des\nStarting Back at " )
                    + std::to_string( maxrefdes ) + "\n";
            ShowWarning( warning );
            BackStartRefDes = maxrefdes + 1;
        }
    }

    this->m_RefDesTypes.clear();
    this->m_ChangeArray.clear();
    BuildExcludeList(); //Exclude these modules from renumbering

    BuildChangeArray( this->m_FrontModules, s_savedDialogParameters.FrontStartRefDes,
            aBadRefDes ); //Create the ChangeArray from front

    if( 0 != BackStartRefDes ) //If I don't carry on from the front
        for( auto Type : this->m_RefDesTypes )
            Type.RefDesCount = BackStartRefDes; //Back ref des start here
    else
        BackStartRefDes =
                s_savedDialogParameters.FrontStartRefDes; //Otherwise a continuation from the front

    BuildChangeArray( this->m_BackModules, BackStartRefDes, aBadRefDes ); //Add to the back
    sort( this->m_ChangeArray.begin(), this->m_ChangeArray.end(),
            ChangeArrayCompare ); //Sort the front modules

    LogRefDesTypes(); //Show the types of ref deses
    LogExcludeList();
    LogChangeArray(); //Show the Change Array
} //void BuildModuleList(  )

//
// Scan through the module arrays and create the from -> to array
//
void DIALOG_BOARD_RENUM::BuildChangeArray( std::vector<RefDesInfo>& aModules,
        unsigned int aStartRefDes, std::vector<RefDesInfo>& aBadRefDes )
{
    size_t       i, firstnum;
    RefDesChange change;
    wxString     thistype;
    //
    //Define some shortcuts for readability
    //
    bool addfrontprefix = !s_savedDialogParameters.RemoveFrontPrefix
                          && !s_savedDialogParameters.FrontPrefix.empty();
    bool removefrontprefix = s_savedDialogParameters.RemoveFrontPrefix
                             && !s_savedDialogParameters.FrontPrefix.empty();
    bool addbackprefix = !s_savedDialogParameters.RemoveBackPrefix
                         && !s_savedDialogParameters.BackPrefix.empty();
    bool removebackprefix =
            s_savedDialogParameters.RemoveBackPrefix && !s_savedDialogParameters.BackPrefix.empty();

    wxString& frontprefix = s_savedDialogParameters.FrontPrefix;
    wxString& backprefix  = s_savedDialogParameters.BackPrefix;

    RefDesTypeStr newtype;

    if( 0 == aModules.size() )
        return; //Nothing to do

    wxString logstring;

    logstring = ( aModules.front().Front ) ? _( "\n\nFront Modules" ) : _( "\n\nBack Modules" );
    LogModules( logstring, aModules );

    for( auto Mod : aModules ) //For each module
    {
        change.TimeStamp       = Mod.TimeStamp;
        change.Action          = UpdateRefDes;
        change.OldRefDesString = Mod.RefDesString;
        change.NewRefDes       = Mod.RefDesString;
        change.Front           = Mod.Front;

        if( change.OldRefDesString.IsEmpty() )
        {
            change.Action = EmptyRefDes;
            this->m_ChangeArray.push_back( change );
            aBadRefDes.push_back( Mod );
            continue;
        }

        firstnum = change.OldRefDesString.find_first_of( "0123456789" );

        if( ( ( std::string::npos == firstnum ) || ( 0 == firstnum ) ) )
        {                                  //Has to have a number, but can't be the first
            change.Action = InvalidRefDes; //do not change ref des such as 12 or +1, or L
            this->m_ChangeArray.push_back( change );
            aBadRefDes.push_back( Mod );
            continue;
        }

        thistype = change.OldRefDesString.substr( 0, firstnum ); //Get the type (R, C, etc)

        for( auto exclude : this->m_ExcludeArray ) //Am I supposed to exclude this type?
            if( exclude == thistype )
            {
                change.Action = Exclude;
                this->m_ChangeArray.push_back( change );
                continue;
            }

        for( i = 0; i < this->m_RefDesTypes.size(); i++ )       //See if it is in the types array
            if( this->m_RefDesTypes[i].RefDesType == thistype ) //Found it!
                break;

        if( i == this->m_RefDesTypes.size() )
        { //Not found in the types array so add it
            newtype.RefDesType  = thistype;
            newtype.RefDesCount = aStartRefDes;
            this->m_RefDesTypes.push_back( newtype );
        }

        change.NewRefDes =
                thistype                                                  //Create the new ref des
                + std::to_string( this->m_RefDesTypes[i].RefDesCount++ ); //This is the new refdes

        if( change.Front )
        { //Now deal with prefixes. This is easy to screw up so I check to make sure no prefix is already there
            if( ( ( change.NewRefDes.substr( 0, frontprefix.size() ) != frontprefix )
                        && addfrontprefix ) )
                change.NewRefDes.insert( 0, frontprefix ); //No double dipping
            if( removefrontprefix )
                change.NewRefDes.erase( 0., frontprefix.size() );
        }
        else
        { //Doing the back
            if( ( ( change.NewRefDes.SubString( 0, backprefix.size() ) != backprefix )
                        && addbackprefix ) )
                change.NewRefDes.insert( 0, backprefix );
            if( removebackprefix )
                change.NewRefDes.erase( 0, backprefix.size() );
        }
        this->m_ChangeArray.push_back( change ); //Add to the change array
    }                                            //Loop
} //void BuildChangeArray( )
