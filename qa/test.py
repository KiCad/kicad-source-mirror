import unittest
import platform
import sys
import argparse

if platform.python_version() < '2.7':
    unittest = __import__('unittest2')
else:
    import unittest

try:
    import xmlrunner
    have_xml = True
except ImportError:
    have_xml = False


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
            description='Test suit for KiCad Python functions')
    parser.add_argument('--xml', action="store", type=str,
                        help='Output XML test results to the given directory')

    args = parser.parse_args()

    if args.xml and not have_xml:
        print("XML test reporting not available")
        print("Install the xmlrunner package.")
        sys.exit(2)

    testsuite = unittest.TestLoader().discover('testcases', pattern="*.py")

    if args.xml:
        # Dump XML results to the right directory
        runner = xmlrunner.XMLTestRunner(output=args.xml)
    else:
        # Use a normal text runner
        runner = unittest.TextTestRunner(verbosity=100)

    results = runner.run(testsuite)

    # Return an error code if any of the testsuite tests fail
    if not results.wasSuccessful():
        sys.exit(1)
