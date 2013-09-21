
#include <io_mgr.h>
//#include <string>


#define FMT_UNIMPLEMENTED   _( "Plugin '%s' does not implement the '%s' function." )
#define FMT_NOTFOUND        _( "Plugin type '%s' is not found." )


BOARD* PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


void PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


wxArrayString PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


MODULE* PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                    PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}

