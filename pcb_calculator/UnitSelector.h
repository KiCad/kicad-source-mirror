 /**
  * @file UnitSelector.h
  * a wxChoiceBox to select units in Pcb_Calculator
  */

#ifndef _UnitSelector_h_
#define _UnitSelector_h_


#include <wx/string.h>
#include <wx/choice.h>


class UNIT_SELECTOR: public wxChoice
{
public:
    UNIT_SELECTOR(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 ):
            wxChoice( parent, id, pos, size, choices, style )
    {
    }

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (meter, herz, ohm, radian )
     */
    virtual double GetUnitScale() = 0;

    wxString GetUnitName()
    {
        return GetStringSelection();
    }
};

class UNIT_SELECTOR_LEN: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_LEN(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (meter)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_FREQUENCY: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_FREQUENCY(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_ANGLE: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_ANGLE(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};

class UNIT_SELECTOR_RESISTOR: public UNIT_SELECTOR
{
public:
    UNIT_SELECTOR_RESISTOR(wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style = 0 );

    /**
     * Function GetUnitScale
     * @return the scaling factor to convert users units
     * to normalized units (Hz)
     */
    virtual double GetUnitScale();
};
#endif  // _UnitSelector_h_

