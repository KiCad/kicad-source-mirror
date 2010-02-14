##
# KiCad CDash/CTest Support
# Run cmake, then ctest -D Experimental to push to cdash.
##
set(CTEST_PROJECT_NAME "KiCad")
set(CTEST_NIGHTLY_START_TIME "00:00:00 EST")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "my.cdash.org")
set(CTEST_DROP_LOCATION "/submit.php?project=KiCad")
set(CTEST_DROP_SITE_CDASH TRUE)

