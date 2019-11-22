
#ifndef UTILITY_REGISTRY_H
#define UTILITY_REGISTRY_H

#include <qa_utils/utility_program.h>

#include <map>
#include <memory>
#include <string>


/**
 * A class to handle the registration of utility programs
 */
class UTILITY_REGISTRY
{
public:
    using PLUGIN_MAP = std::map<std::string, KI_TEST::UTILITY_PROGRAM>;

    /**
     * Register a utility program factory function against an ID string.
     *
     * This will be used to create the required utility program if needed.
     *
     * @param  aName      the name of the utility program
     * @param  aFactory   the factory function that will construct the plugin
     * @return            true if registered OK
     */
    static bool Register( const KI_TEST::UTILITY_PROGRAM& aProgInfo )
    {
        PLUGIN_MAP& map = GetInfoMap();

        if( map.find( aProgInfo.m_name ) == map.end() )
        {
            map[aProgInfo.m_name] = aProgInfo;
            return true;
        }

        // Already exists in map
        return false;
    }

    /**
     * Accessor for the static registry map.
     * This is needed to prevent the Static Init Order Fiasco that might occur
     * if we just accessed a static class member.
     */
    static PLUGIN_MAP& GetInfoMap()
    {
        static PLUGIN_MAP info_map;
        return info_map;
    }
};

#endif // UTILITY_REGISTRY_H