/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <sim/sim_library_spice.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <fmt/core.h>
#include <locale_io.h>


class TEST_SIM_LIBRARY_SPICE_FIXTURE
{
public:
    std::string GetLibraryPath( const std::string& aBaseName )
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( "libraries" );
        fn.SetName( aBaseName );
        fn.SetExt( "lib.spice" );

        return std::string( fn.GetFullPath().ToUTF8() );
    }

    void LoadLibrary( const std::string& aBaseName )
    {
        NULL_REPORTER devnull;
        std::string   path = GetLibraryPath( aBaseName );

        m_library = std::make_unique<SIM_LIBRARY_SPICE>( true );
        m_library->ReadFile( path, devnull );
    }

    void CompareToUsualDiodeModel( const SIM_MODEL& aModel, const std::string& aModelName, int aModelIndex )
    {
        BOOST_CHECK( aModel.GetType() == SIM_MODEL::TYPE::D );
        BOOST_CHECK_EQUAL( aModelName,
                           fmt::format( "{}{}_Usual",
                                        boost::to_upper_copy( aModel.GetSpiceInfo().modelType ),
                                        aModelIndex ) );
        BOOST_CHECK_EQUAL( aModel.FindParam( "bv" )->value, "1.1u" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "cjo" )->value, "2.2m" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "ibv" )->value, "3.3" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "is" )->value, "4.4k" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "m_" )->value, "5.5M" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "n" )->value, "6.6G" );
    }

    void CompareToEmptyModel( const SIM_MODEL& aModel, const std::string& aModelName, int aModelIndex )
    {
        BOOST_TEST_CONTEXT( "Model index: " << aModelIndex )
        {
            BOOST_CHECK_EQUAL( aModelName,
                               fmt::format( "{}{}_Empty",
                                            boost::to_upper_copy( aModel.GetSpiceInfo().modelType ),
                                            aModelIndex ) );

            for( unsigned i = 0; i < aModel.GetParamCount(); ++i )
            {
                BOOST_TEST_CONTEXT( "Param name: " << aModel.GetParam( i ).info.name )
                {
                    BOOST_CHECK_EQUAL( aModel.GetParamOverride( i ).value, "" );
                }
            }
        }
    }

    void TestTransistor( const SIM_MODEL& aModel, const std::string& aModelName, int aModelIndex,
                         SIM_MODEL::TYPE aType, const std::vector<std::string>& aParamNames )
    {
        BOOST_TEST_CONTEXT( "Model index: " << aModelIndex
                            << ", Model name: " << aModelName
                            << ", Model device type: " << aModel.GetDeviceInfo().fieldValue
                            << ", Model type: " << aModel.GetTypeInfo().fieldValue )
        {
            if( aModel.GetType() != aType )
            {
                aModelIndex++;
            }

            BOOST_CHECK( aModel.GetType() == aType );

            std::string modelType = aModel.GetSpiceInfo().modelType;
            std::string fieldValue = aModel.GetTypeInfo().fieldValue;

            // Special case for VDMOS because Ngspice parses it differently.
            if( modelType == "VDMOS NCHAN" )
            {
                modelType = "NMOS";
                fieldValue = "VD"; // Not "VDMOS" because Ngspice gives an error if this string is
                                   // used in a model name (this is a bug in Ngspice, FIXME).
            }

            if( modelType == "VDMOS PCHAN" )
            {
                modelType = "PMOS";
                fieldValue = "VD";
            }

            BOOST_CHECK_EQUAL( aModelName, fmt::format( "_{}_{}_{}",
                                                        aModelIndex,
                                                        modelType,
                                                        fieldValue ) );

            for( int i = 0; i < aParamNames.size(); ++i )
            {
                std::string             paramName = aParamNames.at( i );
                const SIM_MODEL::PARAM* param = aModel.FindParam( paramName );

                BOOST_TEST_CONTEXT( "Param name: " << paramName )
                {
                    if( i % 10 == 0 )
                    {
                        BOOST_CHECK( param->value == "0M" || param->value == "0" );
                    }
                    else if( param->info.type == SIM_VALUE::TYPE_INT )
                    {
                        BOOST_CHECK_EQUAL( param->value, fmt::format( "{:d}", i % 10 ) );
                    }
                    else if( param->info.category == SIM_MODEL::PARAM::CATEGORY::FLAGS )
                    {
                        BOOST_CHECK_EQUAL( param->value, "" );
                    }
                    else
                    {
                        BOOST_CHECK_EQUAL( param->value, fmt::format( "{}000.0{}M", i % 10, i % 10 ) );
                    }
                }
            }
        }
    }

    std::unique_ptr<SIM_LIBRARY_SPICE> m_library;
};


BOOST_FIXTURE_TEST_SUITE( SimLibrarySpice, TEST_SIM_LIBRARY_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Subckts )
{
    LOCALE_IO toggle;

    LoadLibrary( "subckts" );

    const std::vector<SIM_LIBRARY::MODEL> models = m_library->GetModels();

    BOOST_CHECK_EQUAL( models.size(), 7 );

    for( int i = 0; i < models.size(); ++i )
    {
        const auto& [modelName, model] = models.at( i );

        BOOST_TEST_CONTEXT( "SUBCKT index: " << i )

        switch( i )
        {
        case 0:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "NO_PARAMS_0" );
            break;

        case 1:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "ONE_PARAM_1" );
            BOOST_REQUIRE_EQUAL( model.GetParamCount(), 1 );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.name, "PARAM1" );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.defaultValue, "1" );
            BOOST_REQUIRE_EQUAL( model.GetPinCount(), 2 );
            BOOST_CHECK_EQUAL( model.GetPin( 0 ).modelPinName, "1" );
            BOOST_CHECK_EQUAL( model.GetPin( 1 ).modelPinName, "2" );
            break;

        case 2:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "ONE_PARAM_SHORT_FORM_2" );
            BOOST_REQUIRE_EQUAL( model.GetParamCount(), 1 );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.name, "PARAM1" );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.defaultValue, "1.0" );
            BOOST_CHECK_EQUAL( model.GetPin( 0 ).modelPinName, "1" );
            BOOST_CHECK_EQUAL( model.GetPin( 1 ).modelPinName, "2" );
            break;

        case 3:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "two_params_3" );
            BOOST_REQUIRE_EQUAL( model.GetParamCount(), 2 );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.name, "param1" );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.defaultValue, "1.1e+1" );
            BOOST_CHECK_EQUAL( model.GetParam( 1 ).info.name, "param2" );
            BOOST_CHECK_EQUAL( model.GetParam( 1 ).info.defaultValue, "2.2e+2" );
            BOOST_REQUIRE_EQUAL( model.GetPinCount(), 2 );
            BOOST_CHECK_EQUAL( model.GetPin( 0 ).modelPinName, "1" );
            BOOST_CHECK_EQUAL( model.GetPin( 1 ).modelPinName, "2" );
            break;

        case 4:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "two_params_short_form_4" );
            BOOST_REQUIRE_EQUAL( model.GetParamCount(), 2 );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.name, "param1" );
            BOOST_CHECK_EQUAL( model.GetParam( 0 ).info.defaultValue, "1.1E+1" );
            BOOST_CHECK_EQUAL( model.GetParam( 1 ).info.name, "param2" );
            BOOST_CHECK_EQUAL( model.GetParam( 1 ).info.defaultValue, "2.2E+2" );
            BOOST_REQUIRE_EQUAL( model.GetPinCount(), 2 );
            BOOST_CHECK_EQUAL( model.GetPin( 0 ).modelPinName, "1" );
            BOOST_CHECK_EQUAL( model.GetPin( 1 ).modelPinName, "2" );
            break;

        case 5:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "NOTHING_5" );
            BOOST_CHECK_EQUAL( model.GetParamCount(), 0 );
            BOOST_CHECK_EQUAL( model.GetPinCount(), 0 );
            break;

        case 6:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::SUBCKT );
            BOOST_CHECK_EQUAL( modelName, "Numparam_inside_6" );
            BOOST_CHECK_EQUAL( model.GetParamCount(), 0 );
            BOOST_CHECK_EQUAL( model.GetPinCount(), 2 );
            break;
        }
    }
}


BOOST_AUTO_TEST_CASE( Diodes )
{
    LOCALE_IO toggle;

    LoadLibrary( "diodes" );

    const std::vector<SIM_LIBRARY::MODEL> models = m_library->GetModels();

    BOOST_CHECK_EQUAL( models.size(), 26 );

    for( int i = 0; i < models.size(); ++i )
    {
        const auto& [modelName, model] = models.at( i );

        BOOST_TEST_CONTEXT( "Diode index: " << i )

        switch( i )
        {
        case 0:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "1N4148" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value, "100" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value, "4p" );
            BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value, "100u" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "4n" );
            BOOST_CHECK_EQUAL( model.FindParam( "m_" )->value, "0.33" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "2" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "0.5" );
            BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value, "10n" );
            BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value, "0.8" );
            break;

        case 1:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D1" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "1.23n" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "1.23" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, ".7890" );
            BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value, "12.34m" );
            BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value, "3" );
            BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value, "1.23" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value, "0.9p" );
            BOOST_CHECK_EQUAL( model.FindParam( "m_" )->value, ".56" );
            BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value, ".78" );
            BOOST_CHECK_EQUAL( model.FindParam( "fc" )->value, ".9" );
            BOOST_CHECK_EQUAL( model.FindParam( "isr" )->value, "12.34n" );
            BOOST_CHECK_EQUAL( model.FindParam( "nr" )->value, "2.345" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value, "100" );
            BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value, "100u" );
            BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value, "12.34n" );
            break;

        case 2:
        case 3:
            CompareToUsualDiodeModel( model, modelName, i );
            break;

        case 4:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D4" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "0.1p" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "2" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value, "3p" );
            BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value, "45n" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value, "678" );
            BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value, "0.1p" );
            break;

        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            CompareToEmptyModel( model, modelName, i );
            break;

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            CompareToUsualDiodeModel( model, modelName, i );
            break;

        case 18:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D<>/?:\\|[]!@#$%^&-_18" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "-1.1" );
            BOOST_CHECK_EQUAL( model.FindParam( "m_" )->value, "2.2" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "-3.3m" );
            BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value, "44k" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value, "55u" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value, "6.6M" );
            break;

        case 19:
        case 20:
        case 21:
            CompareToUsualDiodeModel( model, modelName, i );
            break;

        case 22:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D22" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "11.1n" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "2.2" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "33.3m" );
            BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value, "99.9" );
            BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value, "3" );
            BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value, "1.1" );
            break;

        case 23:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D23" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "11.1n" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "2.2" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "33.3m" );
            BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value, "111.1" );
            BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value, "3" );
            BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value, "2.2" );
            BOOST_CHECK_EQUAL( model.FindParam( "m_" )->value, ".3" );
            break;

        case 24:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D24" );
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "11.1n" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "1.1" );
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "33.3m" );
            BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value, "99.9" );
            BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value, "3" );
            BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value, "1.1" );
            break;

        case 25:
            BOOST_CHECK( model.GetType() == SIM_MODEL::TYPE::D );
            BOOST_CHECK_EQUAL( modelName, "D25" );
            //level
            BOOST_CHECK_EQUAL( model.FindParam( "is" )->value, "0M" );
            //js
            BOOST_CHECK_EQUAL( model.FindParam( "jsw" )->value, "1000.01M" );
            BOOST_CHECK_EQUAL( model.FindParam( "tnom" )->value, "2000.02M" );
            //tref
            BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value, "3000.03M" );
            BOOST_CHECK_EQUAL( model.FindParam( "trs" )->value, "4000.04M" );
            //trs1
            BOOST_CHECK_EQUAL( model.FindParam( "trs2" )->value, "5000.05M" );
            BOOST_CHECK_EQUAL( model.FindParam( "n" )->value, "6000.06M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ns" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value, "8000.08M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ttt1" )->value, "9000.09M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ttt2" )->value, "0M" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value, "1000.01M" );
            //cj0
            //cj
            BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value, "2000.02M" );
            //pb
            BOOST_CHECK_EQUAL( model.FindParam( "m_" )->value, "3000.03M" );
            //mj
            BOOST_CHECK_EQUAL( model.FindParam( "tm1" )->value, "4000.04M" );
            BOOST_CHECK_EQUAL( model.FindParam( "tm2" )->value, "5000.05M" );
            BOOST_CHECK_EQUAL( model.FindParam( "cjp" )->value, "6000.06M" );
            //cjsw
            BOOST_CHECK_EQUAL( model.FindParam( "php" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "mjsw" )->value, "8000.08M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value, "9000.09M" );
            //ik
            BOOST_CHECK_EQUAL( model.FindParam( "ikr" )->value, "0M" );
            BOOST_CHECK_EQUAL( model.FindParam( "nbv" )->value, "1000.01M" );
            BOOST_CHECK_EQUAL( model.FindParam( "area_" )->value, "2000.02M" );
            BOOST_CHECK_EQUAL( model.FindParam( "pj_" )->value, "3000.03M" );
            BOOST_CHECK_EQUAL( model.FindParam( "tlev" )->value, "4" );
            BOOST_CHECK_EQUAL( model.FindParam( "tlevc" )->value, "5" );
            BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value, "6000.06M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "cta" )->value, "8000.08M" );
            //ctc
            BOOST_CHECK_EQUAL( model.FindParam( "ctp" )->value, "9000.09M" );
            BOOST_CHECK_EQUAL( model.FindParam( "tpb" )->value, "0M" );
            //tvj
            BOOST_CHECK_EQUAL( model.FindParam( "tphp" )->value, "1000.01M" );
            BOOST_CHECK_EQUAL( model.FindParam( "jtun" )->value, "2000.02M" );
            BOOST_CHECK_EQUAL( model.FindParam( "jtunsw" )->value, "3000.03M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ntun" )->value, "4000.04M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xtitun" )->value, "5000.05M" );
            BOOST_CHECK_EQUAL( model.FindParam( "keg" )->value, "6000.06M" );
            BOOST_CHECK_EQUAL( model.FindParam( "kf" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "af" )->value, "8000.08M" );
            BOOST_CHECK_EQUAL( model.FindParam( "fc" )->value, "9000.09M" );
            BOOST_CHECK_EQUAL( model.FindParam( "fcs" )->value, "0M" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value, "1000.01M" );
            BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value, "2000.02M" );
            //ib
            BOOST_CHECK_EQUAL( model.FindParam( "tcv" )->value, "3000.03M" );
            BOOST_CHECK_EQUAL( model.FindParam( "cond" )->value, "4000.04M" );
            BOOST_CHECK_EQUAL( model.FindParam( "isr" )->value, "5000.05M" );
            BOOST_CHECK_EQUAL( model.FindParam( "nr" )->value, "6000.06M" );
            BOOST_CHECK_EQUAL( model.FindParam( "fv_max" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "bv_max" )->value, "8000.08M" );
            BOOST_CHECK_EQUAL( model.FindParam( "id_max" )->value, "9000.09M" );
            BOOST_CHECK_EQUAL( model.FindParam( "te_max" )->value, "0M" );
            BOOST_CHECK_EQUAL( model.FindParam( "pd_max" )->value, "1000.01M" );
            BOOST_CHECK_EQUAL( model.FindParam( "rth0" )->value, "2000.02M" );
            BOOST_CHECK_EQUAL( model.FindParam( "cth0" )->value, "3000.03M" );
            BOOST_CHECK_EQUAL( model.FindParam( "lm_" )->value, "4000.04M" );
            BOOST_CHECK_EQUAL( model.FindParam( "lp_" )->value, "5000.05M" );
            BOOST_CHECK_EQUAL( model.FindParam( "wm_" )->value, "6000.06M" );
            BOOST_CHECK_EQUAL( model.FindParam( "wp_" )->value, "7000.07M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xom" )->value, "8000.08M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xoi" )->value, "9000.09M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xm" )->value, "0M" );
            BOOST_CHECK_EQUAL( model.FindParam( "xp" )->value, "1000.01M" );
            //d
            break;

        default:
            BOOST_FAIL( "Unknown parameter index" );
            break;
        }
    }
}


BOOST_AUTO_TEST_CASE( Bjts )
{
    LOCALE_IO toggle;

    LoadLibrary( "bjts" );

    const std::vector<SIM_LIBRARY::MODEL> models = m_library->GetModels();

    BOOST_CHECK_EQUAL( models.size(), 8 );

    for( int i = 0; i < models.size(); ++i )
    {
        const auto& [modelName, model] = models.at( i );

        BOOST_TEST_CONTEXT( "BJT index: " << i )

        switch( i )
        {

        case 0:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_VBIC,
            {
                "rcx",
                "rci",
                "vo",
                "gamm",
                "hrcf",
                "rbx",
                "rbi",
                "re",
                "rs",
                "rbp",
                "is_",
                "nf",
                "nr",
                "fc",
                "cbeo",
                "cje",
                "pe",
                "me",
                "aje",
                "cbco",
                "cjc",
                "qco",
                "cjep",
                "pc",
                "mc",
                "ajc",
                "cjcp",
                "ps",
                "ms",
                "ajs",
                "ibei",
                "wbe",
                "nei",
                "iben",
                "nen",
                "ibci",
                "nci",
                "ibcn",
                "ncn",
                "avc1",
                "avc2",
                "isp",
                "wsp",
                "nfp",
                "ibeip",
                "ibenp",
                "ibcip",
                "ncip",
                "ibcnp",
                "ncnp",
                "vef",
                "ver",
                "ikf",
                "ikr",
                "ikp",
                "tf",
                "qtf",
                "xtf",
                "vtf",
                "itf",
                "tr",
                "td",
                "kfn",
                "afn",
                "bfn",
                "xre",
                "xrb",
                "xrbi",
                "xrc",
                "xrci",
                "xrs",
                "xvo",
                "ea",
                "eaie",
                "eaic",
                "eais",
                "eane",
                "eanc",
                "eans",
                "xis",
                "xii",
                "xin",
                "tnf",
                "tavc",
                "rth",
                "cth",
                "vrt",
                "art",
                "ccso",
                "qbm",
                "nkf",
                "xikf",
                "xrcx",
                "xrbx",
                "xrbp",
                "isrr",
                "xisr",
                "dear",
                "eap",
                "vbbe",
                "nbbe",
                "ibbe",
                "tvbbe1",
                "tvbbe2",
                "tnbbe",
                "ebbe",
                "dtemp_",
                "vers",
                "vref",
                "vbe_max",
                "vbc_max",
                "vce_max"
            } );
            break;

        case 1:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_VBIC,
            {
                "rcx",
                "rci",
                "vo",
                "gamm",
                "hrcf",
                "rbx",
                "rbi",
                "re",
                "rs",
                "rbp",
                "is_",
                "nf",
                "nr",
                "fc",
                "cbeo",
                "cje",
                "pe",
                "me",
                "aje",
                "cbco",
                "cjc",
                "qco",
                "cjep",
                "pc",
                "mc",
                "ajc",
                "cjcp",
                "ps",
                "ms",
                "ajs",
                "ibei",
                "wbe",
                "nei",
                "iben",
                "nen",
                "ibci",
                "nci",
                "ibcn",
                "ncn",
                "avc1",
                "avc2",
                "isp",
                "wsp",
                "nfp",
                "ibeip",
                "ibenp",
                "ibcip",
                "ncip",
                "ibcnp",
                "ncnp",
                "vef",
                "ver",
                "ikf",
                "ikr",
                "ikp",
                "tf",
                "qtf",
                "xtf",
                "vtf",
                "itf",
                "tr",
                "td",
                "kfn",
                "afn",
                "bfn",
                "xre",
                "xrb",
                "xrbi",
                "xrc",
                "xrci",
                "xrs",
                "xvo",
                "ea",
                "eaie",
                "eaic",
                "eais",
                "eane",
                "eanc",
                "eans",
                "xis",
                "xii",
                "xin",
                "tnf",
                "tavc",
                "rth",
                "cth",
                "vrt",
                "art",
                "ccso",
                "qbm",
                "nkf",
                "xikf",
                "xrcx",
                "xrbx",
                "xrbp",
                "isrr",
                "xisr",
                "dear",
                "eap",
                "vbbe",
                "nbbe",
                "ibbe",
                "tvbbe1",
                "tvbbe2",
                "tnbbe",
                "ebbe",
                "dtemp_",
                "vers",
                "vref",
                "vbe_max",
                "vbc_max",
                "vce_max"
            } );
            break;

        case 2:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_GUMMELPOON,
            {
                "is_",
                "nf",
                "ise",
                "ne",
                "bf",
                "ikf",
                "vaf",
                "nr",
                "isc",
                "nc",
                "br",
                "ikr",
                "var",
                "rb",
                "irb",
                "rbm",
                "re",
                "rc",
                "xtb",
                "eg",
                "xti",
                "cje",
                "vje",
                "mje",
                "tf",
                "xtf",
                "vtf",
                "itf",
                "ptf",
                "cjc",
                "vjc",
                "mjc",
                "xcjc",
                "tr",
                "cjs",
                "vjs",
                "mjs",
                "fc"
            } );
            break;

        case 3:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_GUMMELPOON,
            {
                "is_",
                "nf",
                "ise",
                "ne",
                "bf",
                "ikf",
                "vaf",
                "nr",
                "isc",
                "nc",
                "br",
                "ikr",
                "var",
                "rb",
                "irb",
                "rbm",
                "re",
                "rc",
                "xtb",
                "eg",
                "xti",
                "cje",
                "vje",
                "mje",
                "tf",
                "xtf",
                "vtf",
                "itf",
                "ptf",
                "cjc",
                "vjc",
                "mjc",
                "xcjc",
                "tr",
                "cjs",
                "vjs",
                "mjs",
                "fc"
            } );
            break;

        case 4:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_HICUM2,
            {
                "c10",
                "qp0",
                "ich",
                "hf0",
                "hfe",
                "hfc",
                "hjei",
                "ahjei",
                "rhjei",
                "hjci",
                "ibeis",
                "mbei",
                "ireis",
                "mrei",
                "ibeps",
                "mbep",
                "ireps",
                "mrep",
                "mcf",
                "tbhrec",
                "ibcis",
                "mbci",
                "ibcxs",
                "mbcx",
                "ibets",
                "abet",
                "tunode",
                "favl",
                "qavl",
                "kavl",
                "alfav",
                "alqav",
                "alkav",
                "rbi0",
                "rbx",
                "fgeo",
                "fdqr0",
                "fcrbi",
                "fqi",
                "re",
                "rcx",
                "itss",
                "msf",
                "iscs",
                "msc",
                "tsf",
                "rsu",
                "csu",
                "cjei0",
                "vdei",
                "zei",
                "ajei",
                //"aljei", Alias.
                "cjep0",
                "vdep",
                "zep",
                "ajep",
                //"aljep", Alias.
                "cjci0",
                "vdci",
                "zci",
                "vptci",
                "cjcx0",
                "vdcx",
                "zcx",
                "vptcx",
                "fbcpar",
                //"fbc", Alias.
                "fbepar",
                //"fbe", Alias.
                "cjs0",
                "vds",
                "zs",
                "vpts",
                "cscp0",
                "vdsp",
                "zsp",
                "vptsp",
                "t0",
                "dt0h",
                "tbvl",
                "tef0",
                "gtfe",
                "thcs",
                "ahc",
                //"alhc", Alias.
                "fthc",
                "rci0",
                "vlim",
                "vces",
                "vpt",
                "aick",
                "delck",
                "tr",
                "vcbar",
                "icbar",
                "acbar",
                "cbepar",
                //"ceox", Alias.
                "cbcpar",
                //"ccox", Alias.
                "alqf",
                "alit",
                "flnqs",
                "kf",
                "af",
                "cfbe",
                "flcono",
                "kfre",
                "afre",
                "latb",
                "latl",
                "vgb",
                "alt0",
                "kt0",
                "zetaci",
                "alvs",
                "alces",
                "zetarbi",
                "zetarbx",
                "zetarcx",
                "zetare",
                "zetacx",
                "vge",
                "vgc",
                "vgs",
                "f1vg",
                "f2vg",
                "zetact",
                "zetabet",
                "alb",
                "dvgbe",
                "zetahjei",
                "zetavgbe",
                "flsh",
                "rth",
                "zetarth",
                "alrth",
                "cth",
                "flcomp",
                "vbe_max",
                "vbc_max",
                "vce_max"
            } );
            break;

        case 5:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_HICUM2,
            {
                "c10",
                "qp0",
                "ich",
                "hf0",
                "hfe",
                "hfc",
                "hjei",
                "ahjei",
                "rhjei",
                "hjci",
                "ibeis",
                "mbei",
                "ireis",
                "mrei",
                "ibeps",
                "mbep",
                "ireps",
                "mrep",
                "mcf",
                "tbhrec",
                "ibcis",
                "mbci",
                "ibcxs",
                "mbcx",
                "ibets",
                "abet",
                "tunode",
                "favl",
                "qavl",
                "kavl",
                "alfav",
                "alqav",
                "alkav",
                "rbi0",
                "rbx",
                "fgeo",
                "fdqr0",
                "fcrbi",
                "fqi",
                "re",
                "rcx",
                "itss",
                "msf",
                "iscs",
                "msc",
                "tsf",
                "rsu",
                "csu",
                "cjei0",
                "vdei",
                "zei",
                "ajei",
                //"aljei", Alias.
                "cjep0",
                "vdep",
                "zep",
                "ajep",
                //"aljep", Alias.
                "cjci0",
                "vdci",
                "zci",
                "vptci",
                "cjcx0",
                "vdcx",
                "zcx",
                "vptcx",
                "fbcpar",
                //"fbc", Alias.
                "fbepar",
                //"fbe", Alias.
                "cjs0",
                "vds",
                "zs",
                "vpts",
                "cscp0",
                "vdsp",
                "zsp",
                "vptsp",
                "t0",
                "dt0h",
                "tbvl",
                "tef0",
                "gtfe",
                "thcs",
                "ahc",
                //"alhc", Alias.
                "fthc",
                "rci0",
                "vlim",
                "vces",
                "vpt",
                "aick",
                "delck",
                "tr",
                "vcbar",
                "icbar",
                "acbar",
                "cbepar",
                //"ceox", Alias.
                "cbcpar",
                //"ccox", Alias.
                "alqf",
                "alit",
                "flnqs",
                "kf",
                "af",
                "cfbe",
                "flcono",
                "kfre",
                "afre",
                "latb",
                "latl",
                "vgb",
                "alt0",
                "kt0",
                "zetaci",
                "alvs",
                "alces",
                "zetarbi",
                "zetarbx",
                "zetarcx",
                "zetare",
                "zetacx",
                "vge",
                "vgc",
                "vgs",
                "f1vg",
                "f2vg",
                "zetact",
                "zetabet",
                "alb",
                "dvgbe",
                "zetahjei",
                "zetavgbe",
                "flsh",
                "rth",
                "zetarth",
                "alrth",
                "cth",
                "flcomp",
                "vbe_max",
                "vbc_max",
                "vce_max"
            } );

            break;

        case 6:
        case 7:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_GUMMELPOON,
                            { "is_", "nf", "ise", "ne", "bf", "ikf", "vaf", "nr", "isc", "nc" } );
            break;
        }
    }
}


BOOST_AUTO_TEST_CASE( Fets )
{
    LOCALE_IO toggle;

    LoadLibrary( "fets" );

    const std::vector<SIM_LIBRARY::MODEL> models = m_library->GetModels();

    BOOST_CHECK_EQUAL( models.size(), 46 );

    for( int i = 0; i < models.size(); ++i )
    {
        const auto& [modelName, model] = models.at( i );

        BOOST_TEST_CONTEXT( "FET index: " << i )

        // TODO: Actually test ALL model parameters.

        switch( i )
        {
        case 0:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NJFET_SHICHMANHODGES,
                            { "vt0", "beta", "lambda", "rd", "rs", "cgs", "cgd", "pb", "is_",
                              "fc" } );
            break;

        case 1:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PJFET_SHICHMANHODGES,
                            { "vt0", "beta", "lambda", "rd", "rs", "cgs", "cgd", "pb", "is_",
                              "fc" } );
            break;

        case 2:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NJFET_PARKERSKELLERN,
                            { "vbi", "af", "beta", "cds", "cgd", "cgs", "delta", "hfeta", "mvst",
                              "mxi" } );
            break;

        case 3:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PJFET_PARKERSKELLERN,
                            { "vbi", "af", "beta", "cds", "cgd", "cgs", "delta", "hfeta", "mvst",
                              "mxi" } );
            break;

        case 4:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMES_STATZ,
                            { "vt0", "alpha", "beta", "lambda", "b", "rd", "rs", "cgs", "cgd_",
                              "pb" } );
            break;

        case 5:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMES_STATZ,
                            { "vt0", "alpha", "beta", "lambda", "b", "rd", "rs", "cgs", "cgd_",
                              "pb" } );
            break;

        case 6:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMES_YTTERDAL,
                            { "vto", "lambda", "lambdahf", "beta", "vs", "rd", "rs", "rg", "ri",
                              "rf" } );
            break;

        case 7:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMES_YTTERDAL,
                            { "vto", "lambda", "lambdahf", "beta", "vs", "rd", "rs", "rg", "ri",
                              "rf" } );
            break;

        case 8:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMES_HFET1,
                            { "vt0", "lambda", "rd", "rs", "rg", "rdi", "rsi", "rgs", "rgd",
                              "eta" } );
            break;

        case 9:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMES_HFET1,
                            { "vt0", "lambda", "rd", "rs", "rg", "rdi", "rsi", "rgs", "rgd",
                              "eta" } );
            break;

        case 10:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMES_HFET2,
                            { "vs", "ggr", "js", "del", "delta", "deltad", "di", "epsi", "eta",
                              "eta1" } );
            break;

        case 11:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMES_HFET2,
                            { "vs", "ggr", "js", "del", "delta", "deltad", "di", "epsi", "eta",
                              "eta1" } );
            break;

        case 12:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_VDMOS,
            {
                //"type",
                "vto",
                //"vth0",
                "kp",
                "phi",
                "lambda",
                "theta",
                "rd_",
                "rs_",
                "rg",
                "tnom",
                "kf",
                "af",
                //"vdmosn",
                //"vdmosp",
                //"vdmos",
                "rq",
                "vq",
                "mtriode",
                "tcvth",
                //"vtotc",
                "mu",
                //"bex",
                "texp0",
                "texp1",
                "trd1",
                "trd2",
                "trg1",
                "trg2",
                "trs1",
                "trs2",
                "trb1",
                "trb2",
                "subshift",
                "ksubthres",
                "tksubthres1",
                "tksubthres2",
                "bv",
                "ibv",
                "nbv",
                "rds",
                "rb",
                "n",
                "tt",
                "eg",
                "xti",
                "is_",
                "vj",
                "cjo",
                "m_",
                "fc",
                "cgdmin",
                "cgdmax",
                "a",
                "cgs_",
                "rthjc",
                "rthca",
                "cthj",
                "vgs_max",
                "vgd_max",
                "vds_max",
                "vgsr_max",
                "vgdr_max",
                "pd_max",
                "id_max",
                "idr_max",
                "te_max",
                "rth_ext",
                "derating"
            } );
            break;

        case 13:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_VDMOS,
            {
                //"type",
                "vto",
                //"vth0",
                "kp",
                "phi",
                "lambda",
                "theta",
                "rd_",
                "rs_",
                "rg",
                "tnom",
                "kf",
                "af",
                //"vdmosn",
                //"vdmosp",
                //"vdmos",
                "rq",
                "vq",
                "mtriode",
                "tcvth",
                //"vtotc",
                "mu",
                //"bex",
                "texp0",
                "texp1",
                "trd1",
                "trd2",
                "trg1",
                "trg2",
                "trs1",
                "trs2",
                "trb1",
                "trb2",
                "subshift",
                "ksubthres",
                "tksubthres1",
                "tksubthres2",
                "bv",
                "ibv",
                "nbv",
                "rds",
                "rb",
                "n",
                "tt",
                "eg",
                "xti",
                "is_",
                "vj",
                "cjo",
                "m_",
                "fc",
                "cgdmin",
                "cgdmax",
                "a",
                "cgs_",
                "rthjc",
                "rthca",
                "cthj",
                "vgs_max",
                "vgd_max",
                "vds_max",
                "vgsr_max",
                "vgdr_max",
                "pd_max",
                "id_max",
                "idr_max",
                "te_max",
                "rth_ext",
                "derating"
            } );
            break;

        case 14:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS1,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 15:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS1,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 16:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS2,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 17:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS2,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 18:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS3,
                            { "vto", "theta", "gamma", "phi", "eta", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 19:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS3,
                            { "vto", "theta", "gamma", "phi", "eta", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 20:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM1,
                            { "vfb", "lvfb", "wvfb", "phi", "lphi", "wphi", "k1", "lk1", "wk1",
                              "k2" } );
            break;

        case 21:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM1,
                            { "vfb", "lvfb", "wvfb", "phi", "lphi", "wphi", "k1", "lk1", "wk1",
                              "k2" } );
            break;

        case 22:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM2,
                            { "bib", "lbib", "wbib", "vghigh", "lvghigh", "wvghigh",
                              "waib", "bi0", "lbi0", "wbi0" } );
            break;

        case 23:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM2,
                            { "bib", "lbib", "wbib", "vghigh", "lvghigh", "wvghigh",
                              "waib", "bi0", "lbi0", "wbi0" } );
            break;

        case 24:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS6,
                            { "vto", "nvth", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 25:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS6,
                            { "vto", "nvth", "gamma", "phi", "lambda", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 26:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM3,
                            { "tox", "toxm", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "xj",
                              "vsat", "at" } );
            break;

        case 27:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM3,
                            { "tox", "toxm", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "xj",
                              "vsat", "at" } );
            break;

        case 28:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS9,
                            { "vto", "theta", "gamma", "phi", "eta", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 29:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS9,
                            { "vto", "theta", "gamma", "phi", "eta", "rd_", "rs_", "cbd_", "cbs_",
                              "is_" } );
            break;

        case 30:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B4SOI,
                            { "tox", "toxp", "toxm", "dtoxcv", "cdsc", "cdscb", "cdscd", "cit",
                              "nfactor", "vsat" } );
            break;

        case 31:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B4SOI,
                            { "tox", "toxp", "toxm", "dtoxcv", "cdsc", "cdscb", "cdscd", "cit",
                              "nfactor", "vsat" } );
            break;

        case 32:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM4,
                            { "rbps0", "rbpsl", "rbpsw", "rbpsnf", "rbpd0", "rbpdl", "rbpdw", "rbpdnf",
                              "rbpbx0", "rbpbxl" } );
            break;

        case 33:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM4,
                            { "rbps0", "rbpsl", "rbpsw", "rbpsnf", "rbpd0", "rbpdl", "rbpdw", "rbpdnf",
                              "rbpbx0", "rbpbxl" } );
            break;

        case 34:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIFD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 35:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIFD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 36:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIDD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 37:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIDD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 38:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIPD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 39:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIPD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 40:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIM2,
                            { "depmue0", "depmue0l", "depmue0lp", "depmue1", "depmue1l",
                              "depmue1lp", "depmueback0", "depmueback0l", "depmueback0lp",
                              "depmueback1" } );
            break;

        case 41:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIM2,
                            { "depmue0", "depmue0l", "depmue0lp", "depmue1", "depmue1l", "depmue1lp",
                              "depmueback0", "depmueback0l", "depmueback0lp", "depmueback1" } );
            break;

        case 42:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIMHV1,
                            { "prd", "prd22", "prd23", "prd24", "prdict1", "prdov13", "prdslp1",
                              "prdvb", "prdvd", "prdvg11" } );
            break;

        case 43:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIMHV1,
                            { "prd", "prd22", "prd23", "prd24", "prdict1", "prdov13", "prdslp1",
                              "prdvb", "prdvd", "prdvg11" } );
            break;

        case 44:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIMHV2,
                            { "pjs0d", "pjs0swd", "pnjd", "pcisbkd", "pvdiffjd", "pjs0s", "pjs0sws",
                              "prs", "prth0", "pvover" } );
            break;

        case 45:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIMHV2,
                            { "pjs0d", "pjs0swd", "pnjd", "pcisbkd", "pvdiffjd", "pjs0s", "pjs0sws",
                              "prs", "prth0", "pvover" } );
            break;
        }
    }
}


// Test that loading an invalid binary file does not crash. (Issue #22189)
// When a binary file like an OrCAD .olb is selected instead of a valid SPICE library,
// the parser should not crash but report an error gracefully.
BOOST_AUTO_TEST_CASE( InvalidBinaryFile )
{
    LOCALE_IO toggle;

    // This should not throw an exception or crash
    BOOST_CHECK_NO_THROW( LoadLibrary( "invalid_binary" ) );

    // The library should have no valid models since it's not a valid SPICE file
    const std::vector<SIM_LIBRARY::MODEL> models = m_library->GetModels();
    BOOST_CHECK_EQUAL( models.size(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
