/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_MODEL_H
#define SIM_MODEL_H

#include <wx/string.h>
#include <map>
#include <utility>

#include <sch_field.h>
#include <lib_field.h>

// Must be included after sch_field.h (exactly eda_shape.h) to avoid a colliding
// declaration with a window header (under msys2)
#include <sim/sim_value.h>
#include <enum_vector.h>

class SIM_LIBRARY;
class SPICE_GENERATOR;


namespace SIM_MODEL_GRAMMAR
{
    using namespace SIM_VALUE_GRAMMAR;

    struct sep : plus<space> {};


    struct legacyPinNumber : digits {};
    struct legacyPinSequence : list<legacyPinNumber, sep> {};

    struct legacyPinSequenceGrammar : must<legacyPinSequence,
                                           tao::pegtl::eof> {};


    struct pinNumber : plus<not_at<sep>, any> {};
    struct pinSequence : list<pinNumber, sep> {};
    struct pinSequenceGrammar : must<opt<sep>,
                                     opt<pinSequence>,
                                     opt<sep>,
                                     tao::pegtl::eof> {};

    struct param : plus<alnum> {};

    struct unquotedString : plus<not_at<sep>, any> {};
    struct quotedStringContent : star<not_at<one<'"'>>, any> {}; // TODO: Allow escaping '"'.
    struct quotedString : seq<one<'"'>,
                              quotedStringContent,
                              one<'"'>> {};

    struct fieldParamValuePair : if_must<param,
                                         opt<sep>,
                                         one<'='>,
                                         opt<sep>,
                                         sor<quotedString,
                                             unquotedString>> {};
    struct fieldParamValuePairs : list<fieldParamValuePair, sep> {};
    struct fieldParamValuePairsGrammar : must<opt<sep>,
                                              opt<fieldParamValuePairs>,
                                              opt<sep>,
                                              tao::pegtl::eof> {};

    struct fieldInferValueType : plus<upper> {};
    struct fieldInferValuePrimaryValue : seq<// HACK: Because `number` matches empty string,
                                             // ensure it is not empty.
                                             at<sor<tao::pegtl::digit,
                                                    seq<one<'.'>>,
                                                        tao::pegtl::digit>>,
                                             // END HACK.
                                             number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>,
                                             // Hackish: match anything until param-value pairs.
                                             // Because the user may want to write something like
                                             // "10k 30% 30mW w=0.4", but we care only about the
                                             // "10k" and "w=0.4".
                                             star<not_at<sep,
                                                         try_catch<fieldParamValuePairs>>,
                                                  any>> {};
    struct fieldInferValue : sor<seq<fieldInferValueType,
                                     opt<sep,
                                         fieldParamValuePairs>>,
                                 seq<opt<fieldInferValuePrimaryValue>,
                                     opt<sep>,
                                     opt<fieldParamValuePairs>>> {};
    struct fieldInferValueGrammar : must<opt<sep>,
                                         fieldInferValue,
                                         opt<sep>,
                                         tao::pegtl::eof> {};


    template <typename> inline constexpr const char* errorMessage = nullptr;
    template <> inline constexpr auto errorMessage<opt<sep>> = "";
    template <> inline constexpr auto errorMessage<opt<pinSequence>> = "";
    template <> inline constexpr auto errorMessage<opt<sor<fieldInferValueType,
                                                           fieldInferValuePrimaryValue>>> = "";
    template <> inline constexpr auto errorMessage<one<'='>> =
        "expected '='";
    template <> inline constexpr auto errorMessage<sor<quotedString,
                                                       unquotedString>> =
        "expected quoted or unquoted string";
    template <> inline constexpr auto errorMessage<fieldParamValuePairs> =
        "expected parameter=value pairs";
    template <> inline constexpr auto errorMessage<opt<fieldParamValuePairs>> = "";
    template <> inline constexpr auto errorMessage<fieldInferValue> =
        "expected parameter=value pairs, together possibly preceded by a type or primary value";
    template <> inline constexpr auto errorMessage<tao::pegtl::eof> =
        "expected end of string";

    struct error
    {
        template <typename Rule> static constexpr bool raise_on_failure = false;
        template <typename Rule> static constexpr auto message = errorMessage<Rule>;
    };

    template <typename Rule> using control = must_if<error>::control<Rule>;
}


class SIM_MODEL
{
public:
    friend class SPICE_GENERATOR;
    friend class NETLIST_EXPORTER_SPICE;

    struct PIN;
    struct PARAM;

    static constexpr auto REFERENCE_FIELD = "Reference";
    static constexpr auto VALUE_FIELD = "Value";

    static constexpr auto DEVICE_TYPE_FIELD = "Sim_Device";
    static constexpr auto TYPE_FIELD = "Sim_Type";
    static constexpr auto PINS_FIELD = "Sim_Pins";
    static constexpr auto PARAMS_FIELD = "Sim_Params";
    static constexpr auto ENABLE_FIELD = "Sim_Enable";


    // There's a trailing '_' because `DEVICE_TYPE` collides with something in Windows headers.
    DEFINE_ENUM_CLASS_WITH_ITERATOR( DEVICE_TYPE_,
        NONE,

        R,
        C,
        L,
        TLINE,
        SW,

        D,
        NPN,
        PNP,

        NJFET,
        PJFET,

        NMES,
        PMES,

        NMOS,
        PMOS,

        V,
        I,

        SUBCKT,
        XSPICE,

        KIBIS,

        SPICE
    )

    struct DEVICE_INFO
    {
        std::string fieldValue;
        std::string description;
    };


    DEFINE_ENUM_CLASS_WITH_ITERATOR( TYPE,
        NONE,

        R,
        R_POT,
        R_BEHAVIORAL,

        C,
        C_BEHAVIORAL,

        L,
        L_MUTUAL,
        L_BEHAVIORAL,

        TLINE_Z0,
        TLINE_RLGC,

        SW_V,
        SW_I,

        D,

        NPN_GUMMELPOON,
        PNP_GUMMELPOON,
        NPN_VBIC,
        PNP_VBIC,
        //NPN_MEXTRAM,
        //PNP_MEXTRAM,
        NPN_HICUM2,
        PNP_HICUM2,
        //NPN_HICUM_L0,
        //PNP_HICUM_L0,

        NJFET_SHICHMANHODGES,
        PJFET_SHICHMANHODGES,

        NJFET_PARKERSKELLERN,
        PJFET_PARKERSKELLERN,


        NMES_STATZ,
        PMES_STATZ,

        NMES_YTTERDAL,
        PMES_YTTERDAL,

        NMES_HFET1,
        PMES_HFET1,

        NMES_HFET2,
        PMES_HFET2,


        NMOS_MOS1,
        PMOS_MOS1,

        NMOS_MOS2,
        PMOS_MOS2,

        NMOS_MOS3,
        PMOS_MOS3,

        NMOS_BSIM1,
        PMOS_BSIM1,

        NMOS_BSIM2,
        PMOS_BSIM2,

        NMOS_MOS6,
        PMOS_MOS6,

        NMOS_MOS9,
        PMOS_MOS9,

        NMOS_BSIM3,
        PMOS_BSIM3,

        NMOS_B4SOI,
        PMOS_B4SOI,

        NMOS_BSIM4,
        PMOS_BSIM4,

        //NMOS_EKV2_6,
        //PMOS_EKV2_6,

        //NMOS_PSP,
        //PMOS_PSP,

        NMOS_B3SOIFD,
        PMOS_B3SOIFD,

        NMOS_B3SOIDD,
        PMOS_B3SOIDD,

        NMOS_B3SOIPD,
        PMOS_B3SOIPD,

        //NMOS_STAG,
        //PMOS_STAG,

        NMOS_HISIM2,
        PMOS_HISIM2,

        NMOS_HISIMHV1,
        PMOS_HISIMHV1,

        NMOS_HISIMHV2,
        PMOS_HISIMHV2,


        V,
        V_SIN,
        V_PULSE,
        V_EXP,
        /*V_SFAM,
        V_SFFM,*/
        V_PWL,
        V_WHITENOISE,
        V_PINKNOISE,
        V_BURSTNOISE,
        V_RANDUNIFORM,
        V_RANDNORMAL,
        V_RANDEXP,
        //V_RANDPOISSON,
        V_BEHAVIORAL,

        I,
        I_SIN,
        I_PULSE,
        I_EXP,
        /*I_SFAM,
        I_SFFM,*/
        I_PWL,
        I_WHITENOISE,
        I_PINKNOISE,
        I_BURSTNOISE,
        I_RANDUNIFORM,
        I_RANDNORMAL,
        I_RANDEXP,
        //I_RANDPOISSON,
        I_BEHAVIORAL,

        SUBCKT,
        XSPICE,

        KIBIS_DEVICE,
        KIBIS_DRIVER_DC,
        KIBIS_DRIVER_RECT,
        KIBIS_DRIVER_PRBS,

        RAWSPICE
    )

    struct INFO
    {
        DEVICE_TYPE_ deviceType;
        std::string fieldValue;
        std::string description;
    };


    struct SPICE_INFO
    {
        std::string itemType;
        std::string modelType = "";
        std::string inlineTypeString = "";
        std::string level = "";
        bool isDefaultLevel = false;
        bool hasExpression = false;
        std::string version = "";
    };


    struct PIN
    {
        const std::string name;
        std::string symbolPinNumber;

        static constexpr auto NOT_CONNECTED = -1;
    };


    struct PARAM
    {
        // MS Windows compilers complain about the names IN and OUT, so we prefix them.
        enum DIR
        {
            DIR_IN,
            DIR_OUT,
            DIR_INOUT
        };

        enum class CATEGORY
        {
            PRINCIPAL,
            GEOMETRY,
            AC,
            DC,
            CAPACITANCE,
            TEMPERATURE,
            NOISE,
            DISTRIBUTED_QUANTITIES,
            LIMITING_VALUES,
            ADVANCED,
            FLAGS,
            WAVEFORM,
            INITIAL_CONDITIONS,
            SUPERFLUOUS
        };

        struct FLAGS {}; // Legacy.

        struct INFO
        {
            std::string name = "";
            unsigned id = 0; // Legacy (don't remove).
            DIR dir = DIR_INOUT;
            SIM_VALUE::TYPE type = SIM_VALUE::TYPE_FLOAT;
            FLAGS flags = {}; // Legacy (don't remove).
            std::string unit = "";
            CATEGORY category = CATEGORY::PRINCIPAL;
            std::string defaultValue = "";
            std::string defaultValueOfOtherVariant = ""; // Legacy (don't remove).
            std::string description = "";
            bool isSpiceInstanceParam = false;
            bool isInstanceParam = false;
            std::string spiceModelName = "";
            std::string spiceInstanceName = "";
            std::vector<std::string> enumValues = {};

            // TODO: Stop using brace-initializers, use this constructor for all info structs.
            INFO( std::string aName = "",
                  unsigned aId = 0,
                  DIR aDir = DIR_INOUT,
                  SIM_VALUE::TYPE aType = SIM_VALUE::TYPE_FLOAT,
                  FLAGS aFlags = {},
                  const std::string& aUnit = "",
                  CATEGORY aCategory = CATEGORY::PRINCIPAL,
                  const std::string& aDefaultValue = "",
                  const std::string& aDefaultValueOfOtherVariant = "",
                  const std::string& aDescription = "",
                  bool aIsSpiceInstanceParam = false,
                  bool aIsInstanceParam = false,
                  const std::string& aSpiceModelName = "",
                  const std::string& aSpiceInstanceName = "",
                  std::vector<std::string> aEnumValues = {} ) :
                name( aName ),
                id( aId ),
                dir( aDir ),
                type( aType ),
                flags( aFlags ),
                unit( aUnit ),
                category( aCategory ),
                defaultValue( aDefaultValue ),
                defaultValueOfOtherVariant( aDefaultValueOfOtherVariant ),
                description( aDescription ),
                isSpiceInstanceParam( aIsSpiceInstanceParam ),
                isInstanceParam( aIsInstanceParam ),
                spiceModelName( aSpiceModelName ),
                spiceInstanceName( aSpiceInstanceName ),
                enumValues( std::move( aEnumValues ) )
            {
            }
        };

        bool                       resolved;
        wxString                   source;
        std::unique_ptr<SIM_VALUE> value;
        const INFO&                info;
        bool                       isOtherVariant = false; // Legacy.

        PARAM( const INFO& aInfo, bool aIsOtherVariant = false )
            : resolved( true ),
              value( SIM_VALUE::Create( aInfo.type ) ),
              info( aInfo ),
              isOtherVariant( aIsOtherVariant )
        {}
    };


    static DEVICE_INFO DeviceTypeInfo( DEVICE_TYPE_ aDeviceType );
    static INFO TypeInfo( TYPE aType );
    static SPICE_INFO SpiceInfo( TYPE aType );


    template <typename T>
    static TYPE ReadTypeFromFields( const std::vector<T>& aFields, int aSymbolPinCount );

    static DEVICE_TYPE_ InferDeviceTypeFromRef( const std::string& aRef );

    static TYPE InferTypeFromRefAndValue( const std::string& aRef, const std::string& aValue,
                                          int aSymbolPinCount );

    template <typename T>
    static TYPE InferTypeFromLegacyFields( const std::vector<T>& aFields );


    static std::unique_ptr<SIM_MODEL> Create( TYPE aType, unsigned aSymbolPinCount, bool aResolve );
    static std::unique_ptr<SIM_MODEL> Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              bool aResolve );

    template <typename T>
    static std::unique_ptr<SIM_MODEL> Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields, bool aResolve );

    template <typename T>
    static std::unique_ptr<SIM_MODEL> Create( unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields, bool aResolve );

    template <typename T>
    static std::string GetFieldValue( const std::vector<T>* aFields, const std::string& aFieldName );

    template <typename T>
    static void SetFieldValue( std::vector<T>& aFields, const std::string& aFieldName,
                               const std::string& aValue );

    const SPICE_GENERATOR& SpiceGenerator() const { return *m_spiceGenerator; }


    // Move semantics.
    // Rule of five.
    virtual ~SIM_MODEL(); // = default in implementation file.
    SIM_MODEL() = delete;
    SIM_MODEL( const SIM_MODEL& aOther ) = delete;
    SIM_MODEL( SIM_MODEL&& aOther ) = default;
    SIM_MODEL& operator=(SIM_MODEL&& aOther ) = delete;

    template <typename T>
    void ReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields );

    // C++ doesn't allow virtual template methods, so we do this:
    virtual void ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields );
    virtual void ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields );


    template <typename T>
    void WriteFields( std::vector<T>& aFields ) const;

    // C++ doesn't allow virtual template methods, so we do this:
    virtual void WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const;
    virtual void WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const;


    virtual bool HasToIncludeSpiceLibrary() const { return GetBaseModel() && !HasOverrides(); }

    SPICE_INFO GetSpiceInfo() const { return SpiceInfo( GetType() ); }

    void AddPin( const PIN& aPin );
    void DeletePins();

    int FindModelPinIndex( const std::string& aSymbolPinNumber );
    void AddParam( const PARAM::INFO& aInfo, bool aIsOtherVariant = false );

    DEVICE_INFO GetDeviceTypeInfo() const { return DeviceTypeInfo( GetDeviceType() ); }
    INFO GetTypeInfo() const { return TypeInfo( GetType() ); }

    DEVICE_TYPE_ GetDeviceType() const { return GetTypeInfo().deviceType; }
    TYPE GetType() const { return m_type; }

    const SIM_MODEL* GetBaseModel() const { return m_baseModel; }
    virtual void SetBaseModel( const SIM_MODEL& aBaseModel )
    {
        if( GetType() != aBaseModel.GetType() )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Simulation model type must be the same as of its base class: '%s', but is '%s'" ),
                    aBaseModel.GetTypeInfo().fieldValue,
                    GetTypeInfo().fieldValue ) );
        }

        m_baseModel = &aBaseModel;
    }

    int GetPinCount() const { return static_cast<int>( m_pins.size() ); }
    const PIN& GetPin( unsigned aIndex ) const { return m_pins.at( aIndex ); }

    std::vector<std::reference_wrapper<const PIN>> GetPins() const;

    void SetPinSymbolPinNumber( int aPinIndex, const std::string& aSymbolPinNumber )
    {
        m_pins.at( aPinIndex ).symbolPinNumber = aSymbolPinNumber;
    }


    int GetParamCount() const { return static_cast<int>( m_params.size() ); }
    virtual const PARAM& GetParam( unsigned aParamIndex ) const; // Return base parameter unless it's overridden.

    virtual const PARAM* GetTunerParam() const { return nullptr; }

    const PARAM* FindParam( const std::string& aParamName ) const;

    std::vector<std::reference_wrapper<const PARAM>> GetParams() const;

    const PARAM& GetUnderlyingParam( unsigned aParamIndex ) const; // Return the actual parameter.
    const PARAM& GetBaseParam( unsigned aParamIndex ) const; // Always return base parameter if it exists.


    virtual void SetParamValue( int aParamIndex, const SIM_VALUE& aValue );
    void SetParamValue( int aParamIndex, const std::string& aValue,
                        SIM_VALUE::NOTATION aNotation = SIM_VALUE::NOTATION::SI );
    void SetParamValue( const std::string& aParamName, const SIM_VALUE& aValue );
    void SetParamValue( const std::string& aParamName, const std::string& aValue,
                        SIM_VALUE::NOTATION aNotation = SIM_VALUE::NOTATION::SI );

    void SetParamSource( const std::string& aParamName, const wxString& aSource );
    void SetParamSource( int aParamIndex, const wxString& aSource );

    bool HasOverrides() const;
    bool HasNonInstanceOverrides() const;
    bool HasSpiceNonInstanceOverrides() const;

    // Can modifying a model parameter also modify other parameters?
    virtual bool HasAutofill() const { return false; }

    void SetIsEnabled( bool aIsEnabled ) { m_isEnabled = aIsEnabled; }
    bool IsEnabled() const { return m_isEnabled; }

    void SetIsInferred( bool aIsInferred ) { m_isInferred = aIsInferred; }
    bool IsInferred() const { return m_isInferred; }

protected:
    static std::unique_ptr<SIM_MODEL> Create( TYPE aType, bool aResolve );

    SIM_MODEL( TYPE aType );
    SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator );

    virtual void CreatePins( unsigned aSymbolPinCount );

    template <typename T>
    void WriteInferredDataFields( std::vector<T>& aFields, const std::string& aValue = "" ) const;

    virtual std::string GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const;

    std::string GenerateValueField( const std::string& aPairSeparator ) const;
    std::string GenerateParamsField( const std::string& aPairSeparator ) const;
    void ParseParamsField( const std::string& aParamsField );

    void ParsePinsField( unsigned aSymbolPinCount, const std::string& aPinsField );
    void ParseEnableField( const std::string& aDisabledField );

    template <typename T>
    void InferredReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields );
    std::vector<PARAM> m_params;
    const SIM_MODEL* m_baseModel;

private:
    static std::unique_ptr<SIM_MODEL> doCreate( TYPE aType );

    static TYPE readTypeFromSpiceStrings( const std::string& aTypeString,
                                          const std::string& aLevel = "",
                                          const std::string& aVersion = "",
                                          bool aSkipDefaultLevel = true );


    template <typename T>
    void doReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields );

    template <typename T>
    void doWriteFields( std::vector<T>& aFields ) const;

    std::string generateDeviceTypeField() const;
    std::string generateTypeField() const;

    std::string generatePinsField() const;
    std::string generateEnableField() const;

    std::string parseFieldFloatValue( std::string aFieldFloatValue );

    virtual bool hasPrimaryValue() const { return false; }
    virtual bool requiresSpiceModelLine() const;

    virtual std::vector<std::string> getPinNames() const { return {}; }


    std::unique_ptr<SPICE_GENERATOR> m_spiceGenerator;

    const TYPE m_type;
    std::vector<PIN> m_pins;
    bool m_isEnabled;
    bool m_isInferred;
};

#endif // SIM_MODEL_H
