/**
 * Interface Sweet
 * is a Python interface file for SWIG.  Languages other than Python can
 * possibly also be supported with little addtional work.
 */
%module sweet

%{

#include <dsnlexer.h>
#include <sch_lib_table_lexer.h>
#include <sch_lib_table.h>
#include <sch_lpid.h>
#include <sweet_lexer.h>
#include <sch_part.h>

%}

%include "std_string.i"
%include "std_deque.i"
%include "utf8.h"

%ignore LINE_READER::operator char* () const;

namespace SCH {
%ignore PART::operator=( const PART& other );
}

%include "import_export.h"
%include "richio.h"
%include "dsnlexer.h"
//%include "sch_lib_table_lexer.h"
%include "sch_lpid.h"
%include "sch_lib.h"
%include "sch_lib_table.h"
%include "sweet_lexer.h"

%include "sch_part.h"
