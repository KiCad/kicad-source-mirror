#ifndef __hotkeys_grid_table__
#define __hotkeys_grid_table__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/grid.h>

#include <vector>
#include <utility>

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "hotkeys_basic.h"

class HotkeyGridTable : public wxGridTableBase
{

public:
    typedef std::pair< wxString, Ki_HotkeyInfo* > hotkey_spec;
    typedef std::vector< hotkey_spec > hotkey_spec_vector;

    HotkeyGridTable( struct Ki_HotkeyInfoSectionDescriptor* origin );
    virtual ~HotkeyGridTable();
    hotkey_spec_vector& getHotkeys();

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

    virtual bool isHeader( int row );
    virtual void SetKeyCode( int row, long key );
    virtual void RestoreFrom( struct Ki_HotkeyInfoSectionDescriptor* origin );

protected:
   std::vector< hotkey_spec > m_hotkeys;

};

#endif
