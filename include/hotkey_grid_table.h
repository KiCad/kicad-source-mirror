#ifndef __hotkeys_grid_table__
#define __hotkeys_grid_table__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/grid.h>

#include <vector>
#include <utility>

#include <fctsys.h>
#include <pgm_base.h>
#include <common.h>
#include <hotkeys_basic.h>

class HOTKEY_EDITOR_GRID_TABLE : public wxGridTableBase
{

public:
    typedef std::pair< wxString, EDA_HOTKEY* > hotkey_spec;
    typedef std::vector< hotkey_spec > hotkey_spec_vector;

    HOTKEY_EDITOR_GRID_TABLE( struct EDA_HOTKEY_CONFIG* origin );
    virtual ~HOTKEY_EDITOR_GRID_TABLE();
    hotkey_spec_vector& getHotkeys();

private:
    virtual int GetNumberRows();
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );
    virtual wxString  GetTypeName( int row, int col );
    virtual bool CanGetValueAs( int row, int col, const wxString& typeName );
    virtual bool CanSetValueAs( int row, int col, const wxString& typeName );
    virtual long GetValueAsLong( int row, int col );
    virtual double GetValueAsDouble( int row, int col );
    virtual bool GetValueAsBool( int row, int col );
    virtual void SetValueAsLong( int row, int col, long value );
    virtual void SetValueAsDouble( int row, int col, double value );
    virtual void SetValueAsBool( int row, int col, bool value );
    virtual void* GetValueAsCustom( int row, int col );
    virtual void SetValueAsCustom( int row, int col, void* value );
    virtual wxString GetColLabelValue( int col );

public:
    virtual bool IsHeader( int row );
    virtual void SetKeyCode( int row, long key );
    virtual void RestoreFrom( struct EDA_HOTKEY_CONFIG* origin );

protected:
   std::vector< hotkey_spec > m_hotkeys;

};

#endif
