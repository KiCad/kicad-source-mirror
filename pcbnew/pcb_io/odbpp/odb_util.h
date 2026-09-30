#ifndef _ODB_UTIL_H_
#define _ODB_UTIL_H_

#include <map>
#include <set>
#include <optional>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>
#include <layer_ids.h>
#include <padstack.h>
#include <base_units.h>
#include <drill/drill_span.h>
#include <exporters/fab_model/fab_sections.h>
#include <jobs/job_export_pcb_odb.h>
#include <wx/string.h>
#include "pcb_shape.h"
#include <wx/filename.h>


enum class ODB_POLARITY
{
    POSITIVE,
    NEGATIVE
};

namespace ODB
{
std::vector<wxString> UniqueNames( const std::vector<wxString>& aBases, const std::set<wxString>& aReserved,
                                   size_t aMaxLength );

struct VARIANT_NAMES
{
    static VARIANT_NAMES Build( const std::vector<wxString>& aNames );

    wxString LegalName( const wxString& aName ) const;
    wxString SelectedName( const wxString& aName ) const;
    wxString Join( const std::vector<wxString>& aNames ) const;

    std::vector<std::pair<wxString, wxString>> m_names;
    std::vector<wxString>                      m_renamed;
    bool                                       m_listsFit = true;
};
} // namespace ODB

/// What a caller asks of one ODB++ export
struct ODB_EXPORT_OPTIONS
{
    bool                            m_inch = false;
    int                             m_precision = 6;
    /// Unset exports the current variant of the board
    std::optional<wxString>         m_variant;
    wxString                        m_mpnField;
    wxString                        m_productModelName;
    JOB_EXPORT_PCB_ODB::ORIGIN      m_origin = JOB_EXPORT_PCB_ODB::ORIGIN::ABSOLUTE_COORDS;
    bool                            m_anonymizeNets = false;
    bool                            m_boardMetadata = true;
    /// Unset exports every section
    std::optional<FAB::SECTION_SET> m_sections;
    std::vector<ODB_LAYER_OVERRIDE> m_layerOverrides;
};

struct ODB_FORMAT
{
    // Internal units to output units for data values
    double      m_scale = 1.0 / PCB_IU_PER_MM;
    // Internal units to thousandths of output units for symbols and drills
    // This scale is 1000x m_scale
    double      m_symbolScale = 1.0 / PL_IU_PER_MM;
    int         m_sigfig = 6;
    std::string m_unitsStr = "MM";
    wxString    m_variantName;
    wxString    m_productModelName;
    wxString    m_mpnField;
    VECTOR2I    m_originOffset{ 0, 0 };
    bool        m_anonymizeNets = false;
    bool        m_writeEdaNets = true;
    bool        m_boardMetadata = true;
    std::optional<FAB::SECTION_SET> m_sections;
    std::vector<ODB_LAYER_OVERRIDE> m_layerOverrides;
    ODB::VARIANT_NAMES m_variantNames;

    bool Includes( FAB::SECTION aSection ) const
    {
        return !m_sections || m_sections->Contains( aSection );
    }
};

enum class ODB_CONTEXT
{
    BOARD,
    MISC
};

enum class ODB_DIELECTRIC_TYPE
{
    NONE,
    PREPREG,
    CORE
};

enum class ODB_TYPE
{
    UNDEFINED,

    SIGNAL,
    POWER_GROUND,
    DIELECTRIC,
    MIXED,
    SOLDER_MASK,
    SOLDER_PASTE,
    SILK_SCREEN,
    DRILL,
    ROUT,
    DOCUMENT,
    COMPONENT,
    MASK,
    CONDUCTIVE_PASTE,
};

enum class ODB_SUBTYPE
{
    COVERLAY,
    COVERCOAT,
    STIFFENER,
    BEND_AREA,
    FLEX_AREA,
    RIGID_AREA,
    PSA,
    SILVER_MASK,
    CARBON_MASK,
    BACKDRILL,
};

enum class ODB_FID_TYPE
{
    COPPER,
    LAMINATE,
    HOLE
};

enum class ODB_AUX_LAYER_TYPE
{
    COVERING,
    PLUGGING,
    TENTING,
    FILLING,
    CAPPING,
};

using ODB_AUX_LAYER_KEY = std::tuple<ODB_AUX_LAYER_TYPE, PCB_LAYER_ID, PCB_LAYER_ID>;

enum class ODB_LAYER_ROLE
{
    BOARD_LAYER,
    DRILL,
    BACKDRILL,
    VIA_PROTECTION,
    ROUT,
    COMPONENT
};

class PCB_VIA;
struct DRILL_OPERATION;

namespace ODB
{
int IpcViaType( const DRILL_OPERATION& aOperation, bool aTop );
bool ViaInPad( const PCB_VIA& aVia );

/// Matrix types a layer override may assign to @a aLayer
std::vector<ODB_TYPE> OverridableLayerTypes( PCB_LAYER_ID aLayer );

wxString GenODBString( const wxString& aStr );

wxString GenLegalNetName( const wxString& aStr );

wxString GenLegalComponentName( const wxString& aStr );

wxString GenLegalEntityName( const wxString& aStr );

void RemoveWhitespace( wxString& aStr );

wxString Double2String( const ODB_FORMAT& aFormat, double aVal );

std::string Double2String( double aVal, int32_t aDigits );

wxString Data2String( const ODB_FORMAT& aFormat, double aVal );

wxString SymDouble2String( const ODB_FORMAT& aFormat, double aVal );

std::pair<wxString, wxString> AddXY( const ODB_FORMAT& aFormat, const VECTOR2I& aVec );
std::pair<wxString, wxString> AddRelativeXY( const ODB_FORMAT& aFormat, const VECTOR2I& aVec );

VECTOR2I GetShapePosition( const PCB_SHAPE& aShape );

template <typename T>
class EnumStringMap
{
public:
    static std::map<T, std::string>& GetMap()
    {
        static_assert( std::is_enum_v<T>, "Template parameter T must be an enum type" );

        static std::map<T, std::string> map = []()
        {
            std::map<T, std::string> result;

            if constexpr( std::is_same_v<T, ODB_POLARITY> )
            {
                result[ODB_POLARITY::POSITIVE] = "POSITIVE";
                result[ODB_POLARITY::NEGATIVE] = "NEGATIVE";
            }

            if constexpr( std::is_same_v<T, ODB_CONTEXT> )
            {
                result[ODB_CONTEXT::BOARD] = "BOARD";
                result[ODB_CONTEXT::MISC] = "MISC";
            }

            if constexpr( std::is_same_v<T, ODB_TYPE> )
            {
                //just for logical reasons.TYPE field must be defined.
                result[ODB_TYPE::UNDEFINED] = "";

                result[ODB_TYPE::SIGNAL] = "SIGNAL";
                result[ODB_TYPE::POWER_GROUND] = "POWER_GROUND";
                result[ODB_TYPE::DIELECTRIC] = "DIELECTRIC";
                result[ODB_TYPE::MIXED] = "MIXED";
                result[ODB_TYPE::SOLDER_MASK] = "SOLDER_MASK";
                result[ODB_TYPE::SOLDER_PASTE] = "SOLDER_PASTE";
                result[ODB_TYPE::SILK_SCREEN] = "SILK_SCREEN";
                result[ODB_TYPE::DRILL] = "DRILL";
                result[ODB_TYPE::ROUT] = "ROUT";
                result[ODB_TYPE::DOCUMENT] = "DOCUMENT";
                result[ODB_TYPE::COMPONENT] = "COMPONENT";
                result[ODB_TYPE::MASK] = "MASK";
                result[ODB_TYPE::CONDUCTIVE_PASTE] = "CONDUCTIVE_PASTE";
            }

            if constexpr( std::is_same_v<T, ODB_SUBTYPE> )
            {
                result[ODB_SUBTYPE::COVERLAY] = "COVERLAY";
                result[ODB_SUBTYPE::COVERCOAT] = "COVERCOAT";
                result[ODB_SUBTYPE::STIFFENER] = "STIFFENER";
                result[ODB_SUBTYPE::BEND_AREA] = "BEND_AREA";
                result[ODB_SUBTYPE::FLEX_AREA] = "FLEX_AREA";
                result[ODB_SUBTYPE::RIGID_AREA] = "RIGID_AREA";
                result[ODB_SUBTYPE::PSA] = "PSA";
                result[ODB_SUBTYPE::SILVER_MASK] = "SILVER_MASK";
                result[ODB_SUBTYPE::CARBON_MASK] = "CARBON_MASK";
                result[ODB_SUBTYPE::BACKDRILL] = "BACKDRILL";
            }

            if constexpr( std::is_same_v<T, ODB_DIELECTRIC_TYPE> )
            {
                result[ODB_DIELECTRIC_TYPE::NONE] = "NONE";
                result[ODB_DIELECTRIC_TYPE::PREPREG] = "PREPREG";
                result[ODB_DIELECTRIC_TYPE::CORE] = "CORE";
            }

            if constexpr( std::is_same_v<T, ODB_FID_TYPE> )
            {
                result[ODB_FID_TYPE::COPPER] = "C";
                result[ODB_FID_TYPE::LAMINATE] = "L";
                result[ODB_FID_TYPE::HOLE] = "H";
            }

            return result;
        }();

        return map;
    }
};

template <typename T>
std::string Enum2String( T value )
{
    const auto& map = EnumStringMap<T>::GetMap();
    auto        it = map.find( value );
    if( it != map.end() )
    {
        return it->second;
    }
    else
    {
        throw std::out_of_range( "Enum value not found in map" );
    }
}

class CHECK_ONCE
{
public:
    bool operator()()
    {
        if( first )
        {
            first = false;
            return true;
        }
        return false;
    }

private:
    bool first = true;
};

} // namespace ODB

class ODB_TREE_WRITER;
class ODB_FILE_WRITER
{
public:
    ODB_FILE_WRITER( ODB_TREE_WRITER& aTreeWriter, const wxString& aFileName );

    virtual ~ODB_FILE_WRITER() { CloseFile(); }

    ODB_FILE_WRITER( ODB_FILE_WRITER&& ) = delete;
    ODB_FILE_WRITER& operator=( ODB_FILE_WRITER&& ) = delete;

    ODB_FILE_WRITER( ODB_FILE_WRITER const& ) = delete;
    ODB_FILE_WRITER& operator=( ODB_FILE_WRITER const& ) = delete;

    void                 CreateFile( const wxString& aFileName );
    bool                 CloseFile();
    inline std::ostream& GetStream() { return m_ostream; }

private:
    ODB_TREE_WRITER& m_treeWriter;
    std::ofstream    m_ostream;
};


class ODB_TREE_WRITER
{
public:
    ODB_TREE_WRITER( const wxString& aDir ) : m_currentPath( aDir ) {}

    ODB_TREE_WRITER( const wxString& aPareDir, const wxString& aSubDir )
    {
        CreateEntityDirectory( aPareDir, aSubDir );
    }

    virtual ~ODB_TREE_WRITER() {}

    [[nodiscard]] ODB_FILE_WRITER CreateFileProxy( const wxString& aFileName )
    {
        return ODB_FILE_WRITER( *this, aFileName );
    }

    void CreateEntityDirectory( const wxString& aPareDir, const wxString& aSubDir = wxEmptyString );

    inline const wxString GetCurrentPath() const { return m_currentPath; }

    inline void SetCurrentPath( const wxString& aDir ) { m_currentPath = aDir; }

    inline void SetRootPath( const wxString& aDir ) { m_rootPath = aDir; }

    inline const wxString GetRootPath() const { return m_rootPath; }


private:
    wxString m_currentPath;
    wxString m_rootPath;
};


class ODB_TEXT_WRITER
{
public:
    ODB_TEXT_WRITER( std::ostream& aStream ) : m_ostream( aStream ) {}
    virtual ~ODB_TEXT_WRITER() {}

    // void WriteEquationLine( const std::string &var, const std::string &value );
    void WriteEquationLine( const std::string& var, int value );
    void WriteEquationLine( const wxString& var, const wxString& value );
    template <typename T>
    void write_line_enum( const std::string& var, const T& value )
    {
        WriteEquationLine( var, ODB::Enum2String( value ) );
    }

    class ARRAY_PROXY
    {
        friend ODB_TEXT_WRITER;

    public:
        ~ARRAY_PROXY();

    private:
        ARRAY_PROXY( ODB_TEXT_WRITER& aWriter, const std::string& aStr );

        ODB_TEXT_WRITER& m_writer;

        ARRAY_PROXY( ARRAY_PROXY&& ) = delete;

        ARRAY_PROXY& operator=( ARRAY_PROXY&& ) = delete;

        ARRAY_PROXY( ARRAY_PROXY const& ) = delete;

        ARRAY_PROXY& operator=( ARRAY_PROXY const& ) = delete;
    };

    [[nodiscard]] ARRAY_PROXY MakeArrayProxy( const std::string& aStr )
    {
        return ARRAY_PROXY( *this, aStr );
    }

private:
    void WriteIndent();

    void BeginArray( const std::string& a );

    void EndArray();

    std::ostream& m_ostream;
    bool          in_array = false;
};


class ODB_DRILL_TOOLS
{
public:
    struct TOOLS
    {
        uint32_t m_num;
        wxString m_type;
        wxString m_type2 = wxT( "STANDARD" );
        uint32_t m_minTol;
        uint32_t m_maxTol;
        wxString m_bit = wxEmptyString;
        wxString m_finishSize;
        wxString m_drillSize;

        TOOLS() : m_num( 0 ), m_minTol( 0 ), m_maxTol( 0 ) {}
    };

    ODB_DRILL_TOOLS( const wxString& aUnits, const wxString& aThickness = "0",
                     const wxString& aUserParams = wxEmptyString );

    /**
     * Record the drill bit used for a hole.
     *
     * @param aType is the ODB++ plating class of the hole (PLATED, NON_PLATED or VIA).
     * @param aDiameter is the hole diameter in internal units.
     * @param aType2 is the ODB++ drilling method (STANDARD, LASER, BLIND, ...).
     */
    void AddDrillTool( const ODB_FORMAT& aFormat, const wxString& aType, int aDiameter,
                        const wxString& aType2 = wxT( "STANDARD" ) );

    void GenerateFile( std::ostream& aStream );

    wxString                            m_units;
    wxString                            m_thickness;
    wxString                            m_userParams;
    std::vector<ODB_DRILL_TOOLS::TOOLS> m_tools;
};

#endif // _ODB_UTIL_H_
