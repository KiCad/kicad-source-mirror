 /**
  * @file UnitSelector.cpp
  * a wxChoiceBox to select units in Pcb_Calculator
  */

#include "UnitSelector.h"
#include "units_scales.h"

UNIT_SELECTOR_LEN::UNIT_SELECTOR_LEN(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _("mm") );
    Append( _("um") );
    Append( _("cm") );
    Append( _("mil") );
    Append( _("inch") );
};


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (meter )
 */
double UNIT_SELECTOR_LEN::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
        case 0: return UNIT_MM; break;
        case 1: return UNIT_MICRON; break;
        case 2: return UNIT_CM; break;
        case 3: return UNIT_MIL; break;
        case 4: return UNIT_INCH; break;
    }
    return 1.0;
}


UNIT_SELECTOR_FREQUENCY::UNIT_SELECTOR_FREQUENCY(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _("GHz") );
    Append( _("MHz") );
    Append( _("KHz") );
    Append( _("Hz") );
};

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (herz )
 */
double UNIT_SELECTOR_FREQUENCY::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
        case 0: return UNIT_GHZ; break;
        case 1: return UNIT_MHZ; break;
        case 2: return UNIT_KHZ; break;
        case 3: return 1.0; break;
    }
    return 1.0;
}


UNIT_SELECTOR_ANGLE::UNIT_SELECTOR_ANGLE(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _("Radian") );
    Append( _("Degree") );
};

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( radian )
 */
double UNIT_SELECTOR_ANGLE::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
        case 0: return UNIT_RADIAN; break;
        case 1: return UNIT_DEGREE; break;
    }
    return 1.0;
}


UNIT_SELECTOR_RESISTOR::UNIT_SELECTOR_RESISTOR(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _("Ohm") );
    Append( _("KOhm") );
};


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( ohm )
 */
double UNIT_SELECTOR_RESISTOR::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
        case 0: return UNIT_OHM; break;
        case 1: return UNIT_KOHM; break;
    }
    return 1.0;
}

