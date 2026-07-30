This directory contains the pboettch/json-schema-validator project
from https://github.com/pboettch/json-schema-validator, version 2.4.0.

Sources are verbatim upstream apart from three changes, each marked with a
"KiCad patch" comment:

string-format-check.cpp keeps the pre-2.4.0 RFC 5322 regex for the "email"
format rather than pull in gene-hightower/smtp-address-validator for
is_address().  That also drops "idn-email", which now reports as
unsupported.  No KiCad schema uses either format.

string-format-check.cpp also turns JSON_SCHEMA_NO_REGEX into an #error.
Upstream sets NO_STD_REGEX for that configuration but still uses
REGEX_NAMESPACE throughout, so it does not compile either way.  KiCad never
defines the macro.

json-validator.cpp registers both "$defs" and "definitions" instead of
stopping at the first one found.  KiCad validates against subschemas by
initial URI, and upstream's early exit makes "#/definitions/Foo"
unresolvable in any schema that also carries "$defs".

The project is licensed under MIT, with the license text in this directory.
