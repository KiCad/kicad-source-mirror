import os
import pytest
from . import run
from .conditions import has_http


def test_unit(cmake, unittest):
    if unittest in ["basic_transport_thread_name"]:
        pytest.skip("excluded from unit test-suite")
    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)


@pytest.mark.skipif(not has_http, reason="tests need http transport")
def test_unit_transport(cmake, unittest):
    if unittest in ["custom_logger", "logger_enable_disable_functionality"]:
        pytest.skip("excluded from transport test-suite")

    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none"},
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)


def test_unit_with_test_path(cmake, unittest):
    if unittest in ["basic_transport_thread_name"]:
        pytest.skip("excluded from unit test-suite")
    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
        cflags=["-DSENTRY_TEST_PATH_PREFIX=./"],
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)
