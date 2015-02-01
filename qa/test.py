import unittest
import platform
import sys

if platform.python_version() < '2.7':
    unittest = __import__('unittest2')
else:
    import unittest

if __name__ == '__main__':
    testsuite = unittest.TestLoader().discover('testcases',pattern="*.py")
    results = unittest.TextTestRunner(verbosity=100).run(testsuite)

    # Return an error code if any of the testsuite tests fail
    if not results.wasSuccessful():
        sys.exit(1)


