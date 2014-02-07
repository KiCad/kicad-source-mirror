import unittest
import platform

if platform.python_version() < '2.7':
    unittest = __import__('unittest2')
else:
    import unittest

if __name__ == '__main__':
    testsuite = unittest.TestLoader().discover('testcases',pattern="*.py")
    unittest.TextTestRunner(verbosity=100).run(testsuite)


