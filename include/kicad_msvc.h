/******************************************************************/
/* kicad_msvc.h : used to compile kicad with Microsoft Visual C++ */
/******************************************************************/

// __MSVC__ must be defined

//
// there are several issues
// 1 - the EXCHG macro uses the typeof keyword, this is unsupported in MSVC
// 2 - there is no round function in the msvc math library
// see ReadMe-MSVC. txt to view list of all changes to sources
//
#ifndef __KICAD_MSVC_ INC__
#define __KICAD_MSVC_ INC__

#ifdef __MSVC__
// the boost libs have a workaround for the typeof problem
#ifdef _MSC_VER
 #if ( _MSC_VER <= 1310 ) // 6.5 7.0 and 7.1 use the msvc bug
  #include <boost/typeof/msvc/typeof_impl.hpp>
 #else // 8.0 or greater
  #include <boost/typeof/typeof.hpp>
  // we have to register the types used with the typeof keyword with boost
  BOOST_TYPEOF_REGISTER_TYPE(wxPoint) ;
  BOOST_TYPEOF_REGISTER_TYPE(wxSize) ;
  BOOST_TYPEOF_REGISTER_TYPE(wxString) ;
  class DrawSheetLabelStruct;
  BOOST_TYPEOF_REGISTER_TYPE(DrawSheetLabelStruct *);
  class EDA_BaseStruct;
  BOOST_TYPEOF_REGISTER_TYPE(EDA_BaseStruct *);
  class D_PAD;
  BOOST_TYPEOF_REGISTER_TYPE(D_PAD *);
  BOOST_TYPEOF_REGISTER_TYPE(const D_PAD *);
  class BOARD_ITEM;
  BOOST_TYPEOF_REGISTER_TYPE(BOARD_ITEM *);
 #endif // _MSC_VER <= 1310
  #define typeof(expr) BOOST_TYPEOF(expr)
#endif // def _MSC_VER

inline double round(double x)
{
 return x >= 0.5 ? ceil(x) : floor(x);
}

#endif // def __MSVC__

#endif  // ndef __KICAD_MSVC_ INC__

