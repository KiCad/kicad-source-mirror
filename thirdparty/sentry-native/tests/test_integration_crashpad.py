import os
import shutil
import subprocess
import sys
import time

import pytest

from . import (
    make_dsn,
    run,
    Envelope,
    split_log_request_cond,
    extract_request,
    is_session_envelope,
    is_logs_envelope,
    is_feedback_envelope,
)
from .conditions import has_crashpad
from .proxy import (
    setup_proxy_env_vars,
    cleanup_proxy_env_vars,
    start_mitmdump,
    proxy_test_finally,
)
from .assertions import (
    assert_breadcrumb,
    assert_crashpad_upload,
    assert_meta,
    assert_session,
    assert_gzip_file_header,
    assert_logs,
    assert_user_feedback,
)

pytestmark = pytest.mark.skipif(
    not has_crashpad,
    reason="Tests need a crashpad backend and a valid environment for it",
)

# Windows and Linux are currently able to flush all the state on crash
flushes_state = sys.platform != "darwin"


def test_crashpad_capture(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2


def _setup_crashpad_proxy_test(cmake, httpserver, proxy):
    proxy_process = start_mitmdump(proxy) if proxy else None

    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver, proxy_host=True))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    return env, proxy_process, tmp_path


def test_crashpad_crash_proxy_env(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8080)
    try:
        env, proxy_process, tmp_path = _setup_crashpad_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        with httpserver.wait(timeout=10) as waiting:
            run(
                tmp_path,
                "sentry_example",
                ["log", "crash"],
                expect_failure=True,
                env=env,
            )
        assert waiting.result
    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(1, httpserver, proxy_process)


def test_crashpad_crash_proxy_env_port_incorrect(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8081)
    try:
        env, proxy_process, tmp_path = _setup_crashpad_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        with pytest.raises(AssertionError):
            with httpserver.wait(timeout=10):
                run(
                    tmp_path,
                    "sentry_example",
                    ["log", "crash"],
                    expect_failure=True,
                    env=env,
                )
    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(0, httpserver, proxy_process)


def test_crashpad_proxy_set_empty(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8080)  # we start the proxy but expect it to remain unused
    try:
        env, proxy_process, tmp_path = _setup_crashpad_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        with httpserver.wait(timeout=10) as waiting:
            run(
                tmp_path,
                "sentry_example",
                ["log", "crash", "proxy-empty"],
                expect_failure=True,
                env=env,
            )
        assert waiting.result

    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(1, httpserver, proxy_process, expected_proxy_logsize=0)


def test_crashpad_proxy_https_not_http(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    # we start the proxy but expect it to remain unused (dsn is http, so shouldn't use https proxy)
    os.environ["https_proxy"] = f"http://localhost:8080"
    try:
        env, proxy_process, tmp_path = _setup_crashpad_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        with httpserver.wait(timeout=10) as waiting:
            run(
                tmp_path,
                "sentry_example",
                ["log", "crash"],
                expect_failure=True,
                env=env,
            )
        assert waiting.result

    finally:
        del os.environ["https_proxy"]
        proxy_test_finally(1, httpserver, proxy_process, expected_proxy_logsize=0)


@pytest.mark.parametrize(
    "run_args",
    [
        pytest.param(["http-proxy"]),  # HTTP proxy test runs on all platforms
        pytest.param(
            ["socks5-proxy"],
            marks=pytest.mark.skipif(
                sys.platform not in ["darwin", "linux"],
                reason="SOCKS5 proxy tests are only supported on macOS and Linux",
            ),
        ),
    ],
)
@pytest.mark.parametrize("proxy_running", [True, False])
def test_crashpad_crash_proxy(cmake, httpserver, run_args, proxy_running):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    expected_logsize = 0

    try:
        proxy_to_start = run_args[0] if proxy_running else None
        env, proxy_process, tmp_path = _setup_crashpad_proxy_test(
            cmake, httpserver, proxy_to_start
        )

        try:
            with httpserver.wait(timeout=10) as waiting:
                run(
                    tmp_path,
                    "sentry_example",
                    ["log", "crash"] + run_args,
                    expect_failure=True,
                    env=env,
                )
        except AssertionError:
            expected_logsize = 0
            return

        assert waiting.result

        expected_logsize = 1
    finally:
        proxy_test_finally(expected_logsize, httpserver, proxy_process)


def test_crashpad_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "reinstall", "crash"],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 1


import psutil
import time


def wait_for_no_werfault(timeout=30.0, poll_interval=0.5):
    """
    Wait until no WerFault.exe process is running. Returns True if all WerFault processes have exited within timeout,
    False otherwise. Of course, this could find any WerFault.exe process running on the system, not just the one
    handling our crash. However, I prefer that to getting failed test runs because a WerFault.exe from a previous run
    still actively accesses the crashing process or the CWD from a previous run.
    """
    deadline = time.time() + timeout
    while time.time() < deadline:
        werfaults = [
            p
            for p in psutil.process_iter(["name"])
            if p.info["name"] and p.info["name"].lower() == "werfault.exe"
        ]
        if not werfaults:
            return True
        time.sleep(poll_interval)
    return False


@pytest.mark.skipif(
    sys.platform != "win32",
    reason="Test covers Windows-specific crashes which can only be covered via the Crashpad WER module",
)
# this test currently can't run on CI because the Windows-image doesn't properly support WER, if you want to run the
# test locally, invoke pytest with the --with_crashpad_wer option which is matched with this marker in the runtest setup
@pytest.mark.with_crashpad_wer
@pytest.mark.parametrize(
    "run_args",
    [
        # discarding via before-send or on-crash has no consequence for fast-fail crashes because they by-pass SEH and
        # thus the crash-handler gets no chance to invoke the FirstChanceHandler which in turn would trigger our hooks.
        (["stack-buffer-overrun"]),
        (["stack-buffer-overrun", "discarding-before-send"]),
        (["stack-buffer-overrun", "discarding-on-crash"]),
        (["fastfail"]),
        (["fastfail", "discarding-before-send"]),
        (["fastfail", "discarding-on-crash"]),
    ],
)
def test_crashpad_wer_crash(cmake, httpserver, run_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attachment",
                "attach-view-hierarchy",
                "overflow-breadcrumbs",
            ]
            + run_args,
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 2
    session_request, multipart = split_log_request_cond(
        httpserver.log, is_session_envelope
    )
    session = session_request.get_data()
    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(
        multipart, expect_attachment=True, expect_view_hierarchy=True
    )

    assert wait_for_no_werfault()


@pytest.mark.parametrize(
    "run_args,build_args",
    [
        # if we crash, we want a dump
        (["attachment"], {"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        (["attachment"], {"SENTRY_TRANSPORT_COMPRESSION": "On"}),
        # if we crash and before-send doesn't discard, we want a dump
        pytest.param(
            ["attachment", "before-send"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
            ),
        ),
        # if on_crash() is non-discarding, a discarding before_send() is overruled, so we get a dump
        pytest.param(
            ["attachment", "discarding-before-send", "on-crash"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
            ),
        ),
        pytest.param(
            ["attach-after-init"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't support dynamic attachments on macOS",
            ),
        ),
        pytest.param(
            ["attachment", "attach-after-init", "clear-attachments"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't support dynamic attachments on macOS",
            ),
        ),
    ],
)
def test_crashpad_dumping_crash(cmake, httpserver, run_args, build_args):
    build_args.update({"SENTRY_BACKEND": "crashpad"})
    tmp_path = cmake(["sentry_example"], build_args)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attach-view-hierarchy",
                "overflow-breadcrumbs",
                "crash",
            ]
            + run_args,
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 2
    session_request, multipart = split_log_request_cond(
        httpserver.log, is_session_envelope
    )
    session = session_request.get_data()
    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_file_header(session)

    envelope = Envelope.deserialize(session)
    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(
        multipart,
        expect_attachment="clear-attachments" not in run_args,
        expect_view_hierarchy="clear-attachments" not in run_args,
    )


@pytest.mark.parametrize(
    "build_args",
    [
        ({}),  # uses default of 64KiB
        pytest.param(
            {"SENTRY_HANDLER_STACK_SIZE": "16"},
            marks=pytest.mark.skipif(
                sys.platform != "win32",
                reason="handler stack size parameterization tests stack guarantee on windows only",
            ),
        ),
        pytest.param(
            {"SENTRY_HANDLER_STACK_SIZE": "32"},
            marks=pytest.mark.skipif(
                sys.platform != "win32",
                reason="handler stack size parameterization tests stack guarantee on windows only",
            ),
        ),
    ],
)
def test_crashpad_dumping_stack_overflow(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "crashpad"})
    tmp_path = cmake(["sentry_example"], build_args)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attachment",
                "attach-view-hierarchy",
                "stack-overflow",
            ],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 2
    session_request, multipart = split_log_request_cond(
        httpserver.log, is_session_envelope
    )
    session = session_request.get_data()

    envelope = Envelope.deserialize(session)
    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(
        multipart, expect_attachment=True, expect_view_hierarchy=True
    )


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
@pytest.mark.parametrize(
    "run_args",
    [(["discarding-before-send"]), (["discarding-on-crash"])],
)
def test_crashpad_non_dumping_crash(cmake, httpserver, run_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attachment",
                "overflow-breadcrumbs",
                "crash",
            ]
            + run_args,
            expect_failure=True,
            env=env,
        )

    assert waiting.result is False

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0]
    session = output.get_data()
    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "abnormal", "errors": 0})


@pytest.mark.skipif(
    sys.platform == "linux", reason="linux clears the signal handlers on shutdown"
)
def test_crashpad_crash_after_shutdown(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "crash-after-shutdown"],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 1

    assert_crashpad_upload(httpserver.log[0][0])


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-multiple", "crash"],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "enable-logs", "capture-log", "crash"],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, multipart = split_log_request_cond(httpserver.log, is_logs_envelope)
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_logs_and_session_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "enable-logs", "capture-log", "crash", "start-session"],
            expect_failure=True,
            env=env,
        )

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we expect 1 envelope with the log, 1 for the crash, and 1 for the session
    assert len(httpserver.log) == 3

    logs_request, remaining = extract_request(httpserver.log, is_logs_envelope)
    session_request, remaining = extract_request(remaining, is_session_envelope)
    multipart = remaining[0][0]  # The crash/minidump

    logs_envelope = Envelope.deserialize(logs_request.get_data())
    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)

    session_envelope = Envelope.deserialize(session_request.get_data())
    assert session_envelope is not None


def test_disable_backend(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["disable-backend", "log", "crash"],
            expect_failure=True,
            env=env,
        )

    # crashpad is disabled, and we are only crashing, so we expect the wait to timeout
    assert waiting.result is False

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # crashpad is disabled, and we are only crashing, so we expect no requests
    assert len(httpserver.log) == 0


@pytest.mark.skipif(
    sys.platform != "darwin" or not os.getenv("CI"),
    reason="retry mechanism test only runs on macOS in CI",
)
def test_crashpad_retry(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    subprocess.run(
        ["sudo", "ifconfig", "lo0", "down"]
    )  # Disables the loopback network interface

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    # crash but fail to send data
    run(tmp_path, "sentry_example", ["log", "crash"], expect_failure=True, env=env)

    assert len(httpserver.log) == 0

    subprocess.run(
        ["sudo", "ifconfig", "lo0", "up"], check=True
    )  # Enables the loopback network interface again
    # don't rmtree here, we don't want to be isolated (example should pick up previous crash from .sentry-native DB)
    # we also sleep to give Crashpad enough time to handle the previous crash
    run(tmp_path, "sentry_example", ["log", "sleep"], env=env)

    assert len(httpserver.log) == 1


@pytest.mark.parametrize(
    "run_args",
    [
        (["crash"]),
    ],
)
def test_crashpad_external_crash_reporter(cmake, httpserver, run_args):
    tmp_path = cmake(
        ["sentry_example", "sentry_crash_reporter"], {"SENTRY_BACKEND": "crashpad"}
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")
    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "crash-reporter"] + run_args,
            expect_failure=True,
            env=env,
        )
    assert waiting.result

    assert len(httpserver.log) == 2
    feedback_request, crash_request = split_log_request_cond(
        httpserver.log, is_feedback_envelope
    )
    feedback = feedback_request.get_data()
    crash = crash_request.get_data()

    envelope = Envelope.deserialize(crash)
    assert_meta(envelope, integration="crashpad")
    assert_breadcrumb(envelope)

    envelope = Envelope.deserialize(feedback)
    assert_user_feedback(envelope)


@pytest.mark.skipif(
    sys.platform != "win32",
    reason="Test covers Windows-specific crashes which can only be covered via the Crashpad WER module",
)
# this test currently can't run on CI because the Windows-image doesn't properly support WER, if you want to run the
# test locally, invoke pytest with the --with_crashpad_wer option which is matched with this marker in the runtest setup
@pytest.mark.with_crashpad_wer
@pytest.mark.parametrize(
    "run_args",
    [
        (["fastfail"]),
    ],
)
def test_crashpad_external_crash_reporter_wer(cmake, httpserver, run_args):
    test_crashpad_external_crash_reporter(cmake, httpserver, run_args)
