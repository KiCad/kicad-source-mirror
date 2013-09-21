import unittest


if __name__ == '__main__':
    testsuite = unittest.TestLoader().discover('testcases',pattern="*.py")
    unittest.TextTestRunner(verbosity=100).run(testsuite)


