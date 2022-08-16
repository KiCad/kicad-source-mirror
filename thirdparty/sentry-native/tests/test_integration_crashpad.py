import pytest
import os
import sys
import time
from . import make_dsn, run, Envelope
from .conditions import has_crashpad
from .assertions import assert_crashpad_upload, assert_session

pytestmark = pytest.mark.skipif(not has_crashpad, reason="tests need crashpad backend")


# Windows and Linux are currently able to flush all the state on crash
flushes_state = sys.platform != "darwin"


def test_crashpad_capture(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2


def test_crashpad_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(tmp_path, "sentry_example", ["log", "reinstall", "crash"], env=env)
        assert child.returncode  # well, its a crash after all

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1


def run_crashpad_dumping_crash(cmake, httpserver, custom_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "start-session", "attachment", "overflow-breadcrumbs", "crash"]
            + custom_args,
            env=env,
        )
        assert child.returncode  # well, its a crash after all

    assert waiting.result

    # the session crash heuristic on mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 2
    outputs = (httpserver.log[0][0], httpserver.log[1][0])
    session, multipart = (
        (outputs[0].get_data(), outputs[1])
        if b'"type":"session"' in outputs[0].get_data()
        else (outputs[1].get_data(), outputs[0])
    )

    return session, multipart


def run_crashpad_non_dumping_crash(cmake, httpserver, custom_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attachment",
                "overflow-breadcrumbs",
                "crash",
            ]
            + custom_args,
            env=env,
        )
        assert child.returncode  # well, its a crash after all

    assert waiting.result is False

    # the session crash heuristic on mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0]
    session = output.get_data()

    return session


def test_crashpad_crash(cmake, httpserver):
    session, multipart = run_crashpad_dumping_crash(cmake, httpserver, [])

    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
def test_crashpad_crash_before_send(cmake, httpserver):
    session, multipart = run_crashpad_dumping_crash(cmake, httpserver, ["before-send"])

    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)

    # TODO(supervacuus): we would expect to see a change coming from the
    # before_send() hook, but in contrast to the other backends the crashpad
    # backend currently only checks for null-value as a signal not to produce
    # a minidump and after this decrefs the event.


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
def test_crashpad_crash_discarding_before_send(cmake, httpserver):
    session = run_crashpad_non_dumping_crash(
        cmake, httpserver, ["discarding-before-send"]
    )

    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "abnormal", "errors": 0})


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
def test_crashpad_crash_discarding_on_crash(cmake, httpserver):
    session = run_crashpad_non_dumping_crash(cmake, httpserver, ["discarding-on-crash"])

    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "abnormal", "errors": 0})


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
def test_crashpad_crash_discarding_before_send_and_on_crash(cmake, httpserver):
    session, multipart = run_crashpad_dumping_crash(
        cmake, httpserver, ["discarding-before-send", "on-crash"]
    )

    envelope = Envelope.deserialize(session)

    # on_crash() is defined and overrules the discarding before_send()
    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)


@pytest.mark.skipif(
    sys.platform == "linux", reason="linux clears the signal handlers on shutdown"
)
def test_crashpad_crash_after_shutdown(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "crash-after-shutdown"],
            env=env,
        )
        assert child.returncode  # well, its a crash after all

    assert waiting.result

    # the session crash heuristic on mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1

    assert_crashpad_upload(httpserver.log[0][0])


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path, "sentry_example", ["log", "capture-multiple", "crash"], env=env
        )
        assert child.returncode  # well, its a crash after all

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


def test_disable_backend(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        child = run(
            tmp_path, "sentry_example", ["disable-backend", "log", "crash"], env=env
        )
        # we crash so process should return non-zero
        assert child.returncode

    # crashpad is disabled, and we are only crashing, so we expect the wait to timeout
    assert waiting.result is False

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # crashpad is disabled, and we are only crashing, so we expect no requests
    assert len(httpserver.log) == 0
