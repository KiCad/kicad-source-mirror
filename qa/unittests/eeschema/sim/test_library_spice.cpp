/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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


class TEST_SIM_LIBRARY_SPICE_FIXTURE
{
public:
    wxString GetLibraryPath( const wxString& aBaseName )
    {
        wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( "libraries" );
        fn.SetName( aBaseName );
        fn.SetExt( "lib.spice" );

        return fn.GetFullPath();
    }

    void LoadLibrary( const wxString& aBaseName )
    {
        wxString path = GetLibraryPath( aBaseName );
        m_library = std::make_unique<SIM_LIBRARY_SPICE>();
        BOOST_CHECK( m_library->ReadFile( path ) );
    }

    void CompareToUsualDiodeModel( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex )
    {
        BOOST_CHECK( aModel.GetType() == SIM_MODEL::TYPE::D );
        BOOST_CHECK_EQUAL( aModelName, aModel.GetSpiceInfo().modelType.Upper()
                                       + wxString::FromCDouble( aModelIndex )
                                       + "_Usual" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "bv" )->value->ToString(), "1.1u" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "cjo" )->value->ToString(), "2.2m" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "ibv" )->value->ToString(), "3.3" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "is" )->value->ToString(), "4.4k" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "m" )->value->ToString(), "5.5M" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "n" )->value->ToString(), "6.6G" );
    }

    void CompareToEmptyModel( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex )
    {
        BOOST_TEST_CONTEXT( "Model index: " << aModelIndex )
        {
            BOOST_CHECK_EQUAL( aModelName, aModel.GetSpiceInfo().modelType.Upper()
                                           + wxString::FromCDouble( aModelIndex )
                                           + "_Empty" );

            for( unsigned i = 0; i < aModel.GetParamCount(); ++i )
            {
                BOOST_TEST_CONTEXT( "Param name: " << aModel.GetParam( i ).info.name )
                {
                    BOOST_CHECK_EQUAL( aModel.GetUnderlyingParam( i ).value->ToString(), "" );
                }
            }
        }
    }

    void TestTransistor( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex,
                         SIM_MODEL::TYPE aType, const std::vector<wxString>& aParamNames )
    {
        BOOST_TEST_CONTEXT( "Model index: " << aModelIndex
                            << ", Model name: " << aModelName
                            << ", Model device type: " << aModel.GetDeviceTypeInfo().fieldValue
                            << ", Model type: " << aModel.GetTypeInfo().fieldValue )
        {
            BOOST_CHECK( aModel.GetType() == aType );
            BOOST_CHECK_EQUAL( aModelName, "_" + wxString::FromCDouble( aModelIndex )
                                           + "_" + aModel.GetSpiceInfo().modelType.Upper()
                                           + "_" + aModel.GetTypeInfo().fieldValue );

            for( int i = 0; i < aParamNames.size(); ++i )
            {
                wxString paramName = aParamNames.at( i );

                BOOST_TEST_CONTEXT( "Param name: " << paramName )
                {
                    if( i == 0 )
                    {
                        BOOST_CHECK_EQUAL( aModel.FindParam( paramName )->value->ToString(), "0" );
                    }
                    else
                    {
                        BOOST_CHECK_EQUAL( aModel.FindParam( paramName )->value->ToString(),
                                           wxString::FromCDouble( i ) + ".0000"
                                           + wxString::FromCDouble( i ) + "G" );
                    }
                }
            }
        }
    }

    std::unique_ptr<SIM_LIBRARY_SPICE> m_library;
};


BOOST_FIXTURE_TEST_SUITE( SimLibrarySpice, TEST_SIM_LIBRARY_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Diodes )
{
    LoadLibrary( "diodes" );

    const std::vector<std::reference_wrapper<SIM_MODEL>> models = m_library->GetModels();
    const std::vector<wxString>& modelNames = m_library->GetModelNames();

    BOOST_CHECK_EQUAL( models.size(), 21 );

    for( int i = 0; i < models.size(); ++i )
    {
        const SIM_MODEL& model = models.at( i );
        const wxString& modelName = modelNames.at( i );

        switch( i )
        {
            case 0:
                BOOST_CHECK_EQUAL( modelName, "1N4148" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "100" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "4p" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100u" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "4n" );
                BOOST_CHECK_EQUAL( model.FindParam( "m" )->value->ToString(), "330m" );
                BOOST_CHECK_EQUAL( model.FindParam( "n" )->value->ToString(), "2" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "500m" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "10n" );
                BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value->ToString(), "800m" );
                break;

            case 1:
                BOOST_CHECK_EQUAL( modelName, "D1" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "1.23n" );
                BOOST_CHECK_EQUAL( model.FindParam( "n" )->value->ToString(), "1.23" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "789m" );
                BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value->ToString(), "12.34m" );
                BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value->ToString(), "3" );
                BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value->ToString(), "1.23" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "900f" );
                BOOST_CHECK_EQUAL( model.FindParam( "m" )->value->ToString(), "560m" );
                BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value->ToString(), "780m" );
                BOOST_CHECK_EQUAL( model.FindParam( "fc" )->value->ToString(), "900m" );
                BOOST_CHECK_EQUAL( model.FindParam( "isr" )->value->ToString(), "12.34n" );
                BOOST_CHECK_EQUAL( model.FindParam( "nr" )->value->ToString(), "2.345" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "100" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100u" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "12.34n" );
                break;

            case 2:
            case 3:
                CompareToUsualDiodeModel( model, modelName, i );
                break;

            case 4:
                BOOST_CHECK_EQUAL( modelName, "D4" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "100f" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "2" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "3p" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "45n" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "678" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100f" );
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
                BOOST_CHECK_EQUAL( modelName, "D18" );
                BOOST_CHECK_EQUAL( model.FindParam( "n" )->value->ToString(), "-1.1" );
                BOOST_CHECK_EQUAL( model.FindParam( "m" )->value->ToString(), "2.2" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "-3.3m" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "44k" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "55u" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "6.6M" );
                break;

            case 19:
            case 20:
                CompareToUsualDiodeModel( model, modelName, i );
                break;

            default:
                BOOST_FAIL( "Unknown parameter index" );
        }
    }
}


BOOST_AUTO_TEST_CASE( Bjts )
{
    LoadLibrary( "bjts" );

    const std::vector<std::reference_wrapper<SIM_MODEL>> models = m_library->GetModels();
    const std::vector<wxString>& modelNames = m_library->GetModelNames();

    BOOST_CHECK_EQUAL( models.size(), 6 );

    for( int i = 0; i < models.size(); ++i )
    {
        const SIM_MODEL& model = models.at( i );
        const wxString& modelName = modelNames.at( i );

        switch( i )
        {
        case 0:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_GUMMELPOON,
                            { "is", "nf", "ise", "ne", "bf", "ikf", "vaf", "nr", "isc", "nc" } );
            break;

        case 1:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_GUMMELPOON,
                            { "is", "nf", "ise", "ne", "bf", "ikf", "vaf", "nr", "isc", "nc" } );
            break;

        case 2:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_VBIC,
                            { "rcx", "rci", "vo", "gamm", "hrcf", "rbx", "rbi", "re", "rs",
                              "rbp" } );
            break;

        case 3:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_VBIC,
                            { "rcx", "rci", "vo", "gamm", "hrcf", "rbx", "rbi", "re", "rs",
                              "rbp" } );
            break;

        case 4:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NPN_HICUML2,
                            { "c10", "qp0", "ich", "hf0", "hfe", "hfc", "hjei", "ahjei", "rhjei",
                              "hjci" } );
            break;

        case 5:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PNP_HICUML2,
                            { "c10", "qp0", "ich", "hf0", "hfe", "hfc", "hjei", "ahjei", "rhjei",
                              "hjci" } );
            break;
        }
    }
}


BOOST_AUTO_TEST_CASE( Fets )
{
    LoadLibrary( "fets" );

    const std::vector<std::reference_wrapper<SIM_MODEL>> models = m_library->GetModels();
    const std::vector<wxString>& modelNames = m_library->GetModelNames();

    BOOST_CHECK_EQUAL( models.size(), 44 );

    for( int i = 0; i < models.size(); ++i )
    {
        const SIM_MODEL& model = models.at( i );
        const wxString& modelName = modelNames.at( i );

        switch( i )
        {
        case 0:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NJFET_SHICHMANHODGES,
                            { "vt0", "beta", "lambda", "rd", "rs", "cgs", "cgd", "pb", "is",
                              "fc" } );
            break;

        case 1:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PJFET_SHICHMANHODGES,
                            { "vt0", "beta", "lambda", "rd", "rs", "cgs", "cgd", "pb", "is",
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
                            { "vt0", "alpha", "beta", "lambda", "b", "rd", "rs", "cgs", "cgd",
                              "pb" } );
            break;

        case 5:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMES_STATZ,
                            { "vt0", "alpha", "beta", "lambda", "b", "rd", "rs", "cgs", "cgd",
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
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS1,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 13:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS1,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 14:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS2,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 15:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS2,
                            { "vto", "kp", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 16:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS3,
                            { "vto", "theta", "gamma", "phi", "eta", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 17:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS3,
                            { "vto", "theta", "gamma", "phi", "eta", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;
        
        case 18:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM1,
                            { "vfb", "lvfb", "wvfb", "phi", "lphi", "wphi", "k1", "lk1", "wk1",
                              "k2" } );
            break;

        case 19:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM1,
                            { "vfb", "lvfb", "wvfb", "phi", "lphi", "wphi", "k1", "lk1", "wk1",
                              "k2" } );
            break;

        case 20:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM2,
                            { "bib", "lbib", "wbib", "vghigh", "lvghigh", "wvghigh",
                              "waib", "bi0", "lbi0", "wbi0" } );
            break;

        case 21:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM2,
                            { "bib", "lbib", "wbib", "vghigh", "lvghigh", "wvghigh",
                              "waib", "bi0", "lbi0", "wbi0" } );
            break;

        case 22:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS6,
                            { "vto", "nvth", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 23:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS6,
                            { "vto", "nvth", "gamma", "phi", "lambda", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 24:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM3,
                            { "tox", "toxm", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "xj",
                              "vsat", "at" } );
            break;

        case 25:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM3,
                            { "tox", "toxm", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "xj",
                              "vsat", "at" } );
            break;

        case 26:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_MOS9,
                            { "vto", "theta", "gamma", "phi", "eta", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 27:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_MOS9,
                            { "vto", "theta", "gamma", "phi", "eta", "rd", "rs", "cbd", "cbs",
                              "is" } );
            break;

        case 28:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B4SOI,
                            { "tox", "toxp", "toxm", "dtoxcv", "cdsc", "cdscb", "cdscd", "cit",
                              "nfactor", "vsat" } );
            break;

        case 29:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B4SOI,
                            { "tox", "toxp", "toxm", "dtoxcv", "cdsc", "cdscb", "cdscd", "cit",
                              "nfactor", "vsat" } );
            break;

        case 30:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_BSIM4,
                            { "rbps0", "rbpsl", "rbpsw", "rbpsnf", "rbpd0", "rbpdl", "rbpdw", "rbpdnf",
                              "rbpbx0", "rbpbxl" } );
            break;

        case 31:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_BSIM4,
                            { "rbps0", "rbpsl", "rbpsw", "rbpsnf", "rbpd0", "rbpdl", "rbpdw", "rbpdnf",
                              "rbpbx0", "rbpbxl" } );
            break;

        case 32:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIFD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 33:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIFD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 34:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIDD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 35:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIDD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 36:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_B3SOIPD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 37:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_B3SOIPD,
                            { "tox", "cdsc", "cdscb", "cdscd", "cit", "nfactor", "vsat", "at", "a0",
                              "ags" } );
            break;

        case 38:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIM2,
                            { "depmue0", "depmue0l", "depmue0lp", "depmue1", "depmue1l",
                              "depmue1lp", "depmueback0", "depmueback0l", "depmueback0lp",
                              "depmueback1" } );
            break;

        case 39:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIM2,
                            { "depmue0", "depmue0l", "depmue0lp", "depmue1", "depmue1l", "depmue1lp",
                              "depmueback0", "depmueback0l", "depmueback0lp", "depmueback1" } );
            break;

        case 40:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIMHV1,
                            { "prd", "prd22", "prd23", "prd24", "prdict1", "prdov13", "prdslp1",
                              "prdvb", "prdvd", "prdvg11" } );
            break;

        case 41:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIMHV1,
                            { "prd", "prd22", "prd23", "prd24", "prdict1", "prdov13", "prdslp1", 
                              "prdvb", "prdvd", "prdvg11" } );
            break;

        case 42:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::NMOS_HISIMHV2,
                            { "pjs0d", "pjs0swd", "pnjd", "pcisbkd", "pvdiffjd", "pjs0s", "pjs0sws",
                              "prs", "prth0", "pvover" } );
            break;

        case 43:
            TestTransistor( model, modelName, i, SIM_MODEL::TYPE::PMOS_HISIMHV2,
                            { "pjs0d", "pjs0swd", "pnjd", "pcisbkd", "pvdiffjd", "pjs0s", "pjs0sws",
                              "prs", "prth0", "pvover" } );
            break;
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
