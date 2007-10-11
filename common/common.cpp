/**********************************************************/
/* Routines d'affichage de parametres et caracteristiques */
/**********************************************************/

/* Fichier common.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "macros.h"
#include "build_version.h"

/*****************************/
wxString GetBuildVersion()
/*****************************/

/* Return the build date
 */
{
    return g_BuildVersion;
}


/*********************************************************************************************/
Ki_PageDescr::Ki_PageDescr( const wxSize& size, const wxPoint& offset, const wxString& name )
/*********************************************************************************************/
{
    // All sizes are in 1/1000 inch
    m_Size = size; m_Offset = offset, m_Name = name;

    // Adjust the default value for margins to 400 mils (0,4 inch or 10 mm)
    m_LeftMargin = m_RightMargin = m_TopMargin = m_BottomMargin = 400;
}


/************************************/
wxString ReturnUnitSymbol( int Units )
/************************************/
{
    wxString label;

    switch( Units )
    {
    case INCHES:
        label = _( " (\"):" );
        break;

    case MILLIMETRE:
        label = _( " (mm):" );
        break;

    default:
        break;
    }

    return label;
}


/**************************************************/
void AddUnitSymbol( wxStaticText& Stext, int Units )
/**************************************************/

/* Add string "  (mm):" or " ("):" to the static text Stext.
 *  Used in dialog boxes for entering values depending on selected units
 */
{
    wxString msg = Stext.GetLabel() + ReturnUnitSymbol( Units );

    Stext.SetLabel( msg );
}


/****************************************************************************/
void PutValueInLocalUnits( wxTextCtrl& TextCtr, int Value, int Internal_Unit )
/****************************************************************************/

/* Convert the number Value in a string according to the internal units
 *  and the selected unit (g_UnitMetric) and put it in the wxTextCtrl TextCtrl
 */
{
    wxString msg = ReturnStringFromValue( g_UnitMetric, Value, Internal_Unit );

    TextCtr.SetValue( msg );
}


/*******************************************************************/
int ReturnValueFromTextCtrl( const wxTextCtrl& TextCtr, int Internal_Unit )
/********************************************************************/

/* Convert the Value in the wxTextCtrl TextCtrl in an integer,
 *  according to the internal units and the selected unit (g_UnitMetric)
 */
{
    int      value;
    wxString msg = TextCtr.GetValue();

    value = ReturnValueFromString( g_UnitMetric, msg, Internal_Unit );

    return value;
}


/****************************************************************************/
wxString ReturnStringFromValue( int Units, int Value, int Internal_Unit )
/****************************************************************************/

/* Return the string from Value, according to units (inch, mm ...) for display,
 *  and the initial unit for value
 *  Unit = display units (INCH, MM ..)
 *  Value = value in Internal_Unit
 *  Internal_Unit = units per inch for Value
 */
{
    wxString StringValue;
    double   value_to_print;

    if( Units >= CENTIMETRE )
        StringValue << Value;
    else
    {
        value_to_print = To_User_Unit( Units, Value, Internal_Unit );
        StringValue.Printf( ( Internal_Unit > 1000 ) ? wxT( "%.4f" ) : wxT( "%.3f" ),
                           value_to_print );
    }

    return StringValue;
}


/****************************************************************************/
int ReturnValueFromString( int Units, const wxString& TextValue, int Internal_Unit )
/****************************************************************************/

/* Return the string from Value, according to units (inch, mm ...) for display,
 *  and the initial unit for value
 *  Unit = display units (INCH, MM ..)
 *  Value = text
 *  Internal_Unit = units per inch for computed value
 */
{
    int    Value;
    double dtmp = 0;

    TextValue.ToDouble( &dtmp );
    if( Units >= CENTIMETRE )
        Value = (int) round( dtmp );
    else
        Value = From_User_Unit( Units, dtmp, Internal_Unit );

    return Value;
}


/******************************************************************/
double To_User_Unit( bool is_metric, int val, int internal_unit_value )
/******************************************************************/

/* Convert in inch or mm the variable "val" given in internal units
 */
{
    double value;

    if( is_metric )
        value = (double) (val) * 25.4 / internal_unit_value;
    else
        value = (double) (val) / internal_unit_value;

    return value;
}


/**********************************************************************/
int From_User_Unit( bool is_metric, double val, int internal_unit_value )
/**********************************************************************/

/* Return in internal units the value "val" given in inch or mm
 */
{
    double value;

    if( is_metric )
        value = val * internal_unit_value / 25.4;
    else
        value = val * internal_unit_value;

    return (int) round( value );
}


/**********************/
wxString GenDate()
/**********************/

/* Return the string date "day month year" like "23 jun 2005"
 */
{
    static const wxString mois[12] =
    {
        wxT( "jan" ), wxT( "feb" ), wxT( "mar" ), wxT( "apr" ), wxT( "may" ), wxT( "jun" ),
        wxT( "jul" ), wxT( "aug" ), wxT( "sep" ), wxT( "oct" ), wxT( "nov" ), wxT( "dec" )
    };
    time_t buftime;
    struct tm*            Date;
    wxString string_date;

    time( &buftime );
    Date = gmtime( &buftime );
    string_date.Printf( wxT( "%d %s %d" ), Date->tm_mday,
                        mois[Date->tm_mon].GetData(),
                        Date->tm_year + 1900 );
    return string_date;
}


/***********************************/
void* MyMalloc( size_t nb_octets )
/***********************************/
/* My memory allocation */
{
    void* pt_mem;

    if( nb_octets == 0 )
    {
        DisplayError( NULL, wxT( "Allocate 0 bytes !!" ) );
        return NULL;
    }
    pt_mem = malloc( nb_octets );
    if( pt_mem == NULL )
    {
        wxString msg;
        msg.Printf( wxT( "Out of memory: allocation %d bytes" ), nb_octets );
        DisplayError( NULL, msg );
    }
    return pt_mem;
}


/************************************/
void* MyZMalloc( size_t nb_octets )
/************************************/

/* My memory allocation, memory space is cleared
 */
{
    void* pt_mem = MyMalloc( nb_octets );

    if( pt_mem )
        memset( pt_mem, 0, nb_octets );
    return pt_mem;
}


/*******************************/
void MyFree( void* pt_mem )
/*******************************/
{
    if( pt_mem )
        free( pt_mem );
}


/**************************************************************/
wxString ReturnPcbLayerName( int layer_number, bool omitSpacePadding )
/**************************************************************/

/* Return the name of the layer number "layer_number".
 *  if "is_filefame" == TRUE, the name can be used for a file name
 *  (not internatinalized, no space)
 */
{
    static const wxString layer_name_list[] = {
        _( "Copper   " ), _( "Inner L1 " ), _( "Inner L2 " ), _( "Inner L3 " ),
        _( "Inner L4 " ), _( "Inner L5 " ), _( "Inner L6 " ), _( "Inner L7 " ),
        _( "Inner L8 " ), _( "Inner L9 " ), _( "Inner L10" ), _( "Inner L11" ),
        _( "Inner L12" ), _( "Inner L13" ), _( "Inner L14" ), _( "Component" ),
        _( "Adhes Cop" ), _( "Adhes Cmp" ), _( "SoldP Cop" ), _( "SoldP Cmp" ),
        _( "SilkS Cop" ), _( "SilkS Cmp" ), _( "Mask Cop " ), _( "Mask Cmp " ),
        _( "Drawings " ), _( "Comments " ), _( "Eco1     " ), _( "Eco2     " ),
        _( "Edges Pcb" ), _( "---      " ), _( "---      " ), _( "---      " )
    };

    
    // Same as layer_name_list, without space, not internationalized
    static const wxString layer_name_list_for_filename[] = {
        wxT( "Copper" ),   wxT( "InnerL1" ),  wxT( "InnerL2" ),  wxT( "InnerL3" ),
        wxT( "InnerL4" ),  wxT( "InnerL5" ),  wxT( "InnerL6" ),  wxT( "InnerL7" ),
        wxT( "InnerL8" ),  wxT( "InnerL9" ),  wxT( "InnerL10" ), wxT( "InnerL11" ),
        wxT( "InnerL12" ), wxT( "InnerL13" ), wxT( "InnerL14" ), wxT( "Component" ),
        wxT( "AdhesCop" ), wxT( "AdhesCmp" ), wxT( "SoldPCop" ), wxT( "SoldPCmp" ),
        wxT( "SilkSCop" ), wxT( "SilkSCmp" ), wxT( "MaskCop" ),  wxT( "MaskCmp" ),
        wxT( "Drawings" ), wxT( "Comments" ), wxT( "Eco1" ),     wxT( "Eco2" ),
        wxT( "EdgesPcb" ), wxT( "---" ),      wxT( "---" ),      wxT( "---" )
    };

    if( (unsigned) layer_number >= 31u )
        layer_number = 31;

    return omitSpacePadding  ? 
        layer_name_list_for_filename[layer_number] :
        layer_name_list[layer_number];
}


enum textbox {
    ID_TEXTBOX_LIST = 8010
};

BEGIN_EVENT_TABLE( WinEDA_TextFrame, wxDialog )
EVT_LISTBOX_DCLICK( ID_TEXTBOX_LIST, WinEDA_TextFrame::D_ClickOnList )
EVT_LISTBOX( ID_TEXTBOX_LIST, WinEDA_TextFrame::D_ClickOnList )
EVT_CLOSE( WinEDA_TextFrame::OnClose )
END_EVENT_TABLE()

/***************************************************************************/
WinEDA_TextFrame::WinEDA_TextFrame( wxWindow* parent, const wxString& title ) :
    wxDialog( parent, -1, title, wxPoint( -1, -1 ), wxSize( 250, 350 ),
              wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT | MAYBE_RESIZE_BORDER  )
/***************************************************************************/
{
    wxSize size;

    m_Parent = parent;

    CentreOnParent();

    size   = GetClientSize();
    m_List = new wxListBox( this, ID_TEXTBOX_LIST,
                            wxPoint( 0, 0 ), size,
                            0, NULL,
                            wxLB_ALWAYS_SB | wxLB_SINGLE );

    m_List->SetBackgroundColour( wxColour( 200, 255, 255 ) );
    SetReturnCode( -1 );
}


/***************************************************/
void WinEDA_TextFrame::Append( const wxString& text )
/***************************************************/
{
    m_List->Append( text );
}


/**********************************************************/
void WinEDA_TextFrame::D_ClickOnList( wxCommandEvent& event )
/**********************************************************/
{
    int ii = m_List->GetSelection();

    EndModal( ii );
}


/*************************************************/
void WinEDA_TextFrame::OnClose( wxCloseEvent& event )
/*************************************************/
{
    EndModal( -1 );
}


/*****************************************************************************/
void Affiche_1_Parametre( WinEDA_DrawFrame* frame, int pos_X,
                          const wxString& texte_H, const wxString& texte_L, int color )
/*****************************************************************************/

/*
 *  Routine d'affichage d'un parametre.
 *  pos_X = cadrage horizontal
 *      si pos_X < 0 : la position horizontale est la derniere
 *          valeur demandee >= 0
 *  texte_H = texte a afficher en ligne superieure.
 *      si "", par d'affichage sur cette ligne
 *  texte_L = texte a afficher en ligne inferieure.
 *      si "", par d'affichage sur cette ligne
 *  color = couleur d'affichage
 */
{
    frame->MsgPanel->Affiche_1_Parametre( pos_X, texte_H, texte_L, color );
}


/****************************************************************************/
void AfficheDoc( WinEDA_DrawFrame* frame, const wxString& Doc, const wxString& KeyW )
/****************************************************************************/

/*
 *  Routine d'affichage de la documentation associee a un composant
 */
{
    wxString Line1( wxT( "Doc:  " ) ), Line2( wxT( "KeyW: " ) );

    int color = BLUE;

    if( frame && frame->MsgPanel )
    {
        frame->MsgPanel->EraseMsgBox();
        Line1 += Doc;
        Line2 += KeyW;
        frame->MsgPanel->Affiche_1_Parametre( 10, Line1, Line2, color );
    }
}


/***********************/
int GetTimeStamp()
/***********************/

/*
 *  Retourne une identification temporelle (Time stamp) differente a chaque appel
 */
{
    static int OldTimeStamp, NewTimeStamp;

    NewTimeStamp = time( NULL );
    if( NewTimeStamp <= OldTimeStamp )
        NewTimeStamp = OldTimeStamp + 1;
    OldTimeStamp = NewTimeStamp;
    return NewTimeStamp;
}


/*************************************************/
void valeur_param( int valeur, wxString& buf_texte )
/*************************************************/

/* Retourne pour affichage la valeur d'un parametre, selon type d'unites choisies
 *  entree : valeur en mils , buffer de texte
 *  retourne en buffer : texte : valeur exprimee en pouces ou millimetres
 *                      suivie de " ou mm
 */
{
    if( g_UnitMetric )
    {
        buf_texte.Printf( wxT( "%3.3f mm" ), valeur * 0.00254 );
    }
    else
    {
        buf_texte.Printf( wxT( "%2.4f \"" ), valeur * 0.0001 );
    }
}
