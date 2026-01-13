import itertools
import json
import os
import shutil
import sys
import time
import uuid
import subprocess

import pytest

from . import (
    make_dsn,
    run,
    Envelope,
    split_log_request_cond,
    is_feedback_envelope,
    is_logs_envelope,
    SENTRY_VERSION,
)
from .proxy import (
    setup_proxy_env_vars,
    cleanup_proxy_env_vars,
    start_mitmdump,
    proxy_test_finally,
)
from .assertions import (
    assert_attachment,
    assert_meta,
    assert_breadcrumb,
    assert_stacktrace,
    assert_event,
    assert_exception,
    assert_inproc_crash,
    assert_session,
    assert_user_feedback,
    assert_user_report,
    assert_minidump,
    assert_breakpad_crash,
    assert_gzip_content_encoding,
    assert_gzip_file_header,
    assert_failed_proxy_auth_request,
    assert_attachment_view_hierarchy,
    assert_logs,
)
from .conditions import has_http, has_breakpad, has_files, is_kcov

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# fmt: on


@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_capture_http(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "none"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "release-env", "capture-event", "add-stacktrace"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_meta(envelope, "🤮🚀")
    assert_breadcrumb(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)


def test_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # start once without a release, but with a session
    run(
        tmp_path,
        "sentry_example",
        ["log", "release-env", "start-session"],
        env=env,
    )
    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session"],
        env=env,
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_session(envelope, {"init": True, "status": "exited", "errors": 0})


def test_capture_and_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_event(envelope)
    assert_session(envelope, {"init": True, "status": "ok", "errors": 0})

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)
    assert_session(envelope, {"status": "exited", "errors": 0})


def test_user_feedback_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-user-feedback"],
        env=env,
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_user_feedback(envelope)


def test_user_report_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-user-report"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_event(envelope, "Hello user feedback!")

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_user_report(envelope)


@pytest.mark.skipif(is_kcov, reason="kcov exits with 0 even when the process crashes")
@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_BACKEND": "inproc"}),
        pytest.param(
            {"SENTRY_BACKEND": "breakpad"},
            marks=pytest.mark.skipif(
                not has_breakpad, reason="test needs breakpad backend"
            ),
        ),
    ],
)
def test_external_crash_reporter_http(cmake, httpserver, build_args):
    tmp_path = cmake(["sentry_example", "sentry_crash_reporter"], build_args)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
        run(
            tmp_path,
            "sentry_example",
            ["log", "crash-reporter", "crash"],
            expect_failure=True,
            env=env,
        )

        # the session crash heuristic on Mac uses timestamps, so make sure we have
        # a small delay here
        time.sleep(1)

        run(
            tmp_path,
            "sentry_example",
            ["log", "no-setup"],
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
    assert_meta(envelope, integration=build_args.get("SENTRY_BACKEND", ""))

    envelope = Envelope.deserialize(feedback)
    assert_user_feedback(envelope)


def test_exception_and_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-exception", "add-stacktrace"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_exception(envelope)
    assert_stacktrace(envelope, inside_exception=True)
    assert_session(envelope, {"init": True, "status": "ok", "errors": 1})

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)
    assert_session(envelope, {"status": "exited", "errors": 1})


@pytest.mark.skipif(not has_files, reason="test needs a local filesystem")
def test_abnormal_session(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none"},
    )

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # create a bogus session file
    session = json.dumps(
        {
            "sid": "00000000-0000-0000-0000-000000000000",
            "did": "42",
            "status": "started",
            "errors": 0,
            "started": "2020-06-02T10:04:53.680Z",
            "duration": 10,
            "attrs": {"release": "test-example-release", "environment": "development"},
        }
    )
    db_dir = tmp_path.joinpath(".sentry-native")
    db_dir.mkdir(exist_ok=True)
    # 101 exceeds the max session items
    for i in range(101):
        run_dir = db_dir.joinpath(f"foo-{i}.run")
        run_dir.mkdir()
        with open(run_dir.joinpath("session.json"), "w") as session_file:
            session_file.write(session)

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 2
    envelope1 = Envelope.deserialize(httpserver.log[0][0].get_data())
    envelope2 = Envelope.deserialize(httpserver.log[1][0].get_data())

    session_count = 0
    for item in itertools.chain(envelope1, envelope2):
        if item.headers.get("type") == "session":
            session_count += 1
    assert session_count == 101

    assert_session(envelope1, {"status": "abnormal", "errors": 0, "duration": 10})


@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_inproc_crash_http(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "inproc"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "attachment", "attach-view-hierarchy", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_session(envelope, {"init": True, "status": "crashed", "errors": 1})

    assert_meta(envelope, integration="inproc")
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_inproc_crash(envelope)


def test_inproc_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "reinstall", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1


def test_inproc_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "crash"],
        expect_failure=True,
        env=env,
    )
    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_breakpad_crash_http(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "breakpad"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "attachment", "attach-view-hierarchy", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_session(envelope, {"init": True, "status": "crashed", "errors": 1})

    assert_meta(envelope, integration="breakpad")
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_breakpad_crash(envelope)
    assert_minidump(envelope)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "reinstall", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "crash"],
        expect_failure=True,
        env=env,
    )

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


def test_shutdown_timeout(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    # the timings here are:
    # * the process waits 2s for the background thread to shut down, which fails
    # * it then dumps everything and waits another 1s before terminating the process
    # * the python runner waits for 2.4s in total to close the request, which
    #   will cleanly terminate the background worker.
    # the assumption here is that 2s < 2.4s < 2s+1s. but since those timers
    # run in different processes, this has the potential of being flaky

    def delayed(req):
        time.sleep(2.4)
        return "{}"

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_handler(delayed)
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # Using `sleep-after-shutdown` here means that the background worker will
    # deref/free itself, so we will not leak in that case!
    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "sleep-after-shutdown"],
        env=env,
    )

    httpserver.clear_all_handlers()
    httpserver.clear_log()

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 10


RFC3339_FORMAT = "%Y-%m-%dT%H:%M:%S.%fZ"


@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_transaction_only(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "none"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-transaction"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails.
    envelope.print_verbose()

    # The transaction is overwritten.
    assert_meta(
        envelope,
        transaction="little.teapot",
    )

    # Extract the one-and-only-item
    (event,) = envelope.items

    assert event.headers["type"] == "transaction"
    payload = event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    assert (
        trace_context["op"] == "Short and stout here is my handle and here is my spout"
    )

    assert trace_context["trace_id"]
    trace_id = uuid.UUID(hex=trace_context["trace_id"])
    assert trace_id

    assert trace_context["span_id"]
    assert trace_context["status"] == "ok"

    start_timestamp = time.strptime(payload["start_timestamp"], RFC3339_FORMAT)
    assert start_timestamp
    timestamp = time.strptime(payload["timestamp"], RFC3339_FORMAT)
    assert timestamp >= start_timestamp

    assert trace_context["data"] == {"url": "https://example.com"}


def test_before_transaction_callback(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-transaction", "before-transaction"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails.
    envelope.print_verbose()

    # callback has changed from default teapot to coffeepot
    assert_meta(
        envelope,
        transaction="little.coffeepot",
    )

    # Extract the one-and-only-item
    (event,) = envelope.items

    assert event.headers["type"] == "transaction"
    payload = event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    assert (
        trace_context["op"] == "Short and stout here is my handle and here is my spout"
    )

    assert trace_context["trace_id"]
    trace_id = uuid.UUID(hex=trace_context["trace_id"])
    assert trace_id

    assert trace_context["span_id"]
    assert trace_context["status"] == "ok"

    start_timestamp = time.strptime(payload["start_timestamp"], RFC3339_FORMAT)
    assert start_timestamp
    timestamp = time.strptime(payload["timestamp"], RFC3339_FORMAT)
    assert timestamp >= start_timestamp

    assert trace_context["data"] == {"url": "https://example.com"}


def test_before_transaction_discard(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-transaction", "discarding-before-transaction"],
        env=env,
    )

    # transaction should have been discarded
    assert len(httpserver.log) == 0


def test_transaction_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-transaction", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2
    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    tx_envelope = Envelope.deserialize(tx_body)
    event_envelope = Envelope.deserialize(event_body)

    # Show what the envelope looks like if the test fails.
    tx_envelope.print_verbose()

    # The transaction is overwritten.
    assert_meta(
        tx_envelope,
        transaction="little.teapot",
    )

    # Extract the transaction and event items
    (tx_event,) = tx_envelope.items
    (event,) = event_envelope.items

    assert tx_event.headers["type"] == "transaction"
    payload = tx_event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    assert trace_context["trace_id"]
    tx_trace_id = uuid.UUID(hex=trace_context["trace_id"])
    assert tx_trace_id
    event_trace_id = uuid.UUID(hex=event.payload.json["contexts"]["trace"]["trace_id"])
    # by default, transactions and events will have different trace id's because transactions create their own traces
    # unless a client explicitly called `sentry_set_trace()` (which transfers the burden of management to the caller).
    assert tx_trace_id != event_trace_id
    assert_event(event_envelope, "Hello World!", "")
    # non-scoped tx should differ in span_id from the event span_id
    assert trace_context["span_id"]
    assert (
        trace_context["span_id"] != event.payload.json["contexts"]["trace"]["span_id"]
    )


def test_transaction_trace_header(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "set-trace", "capture-transaction"],
        env=env,
    )

    assert len(httpserver.log) == 1
    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)

    # Extract the one-and-only-item
    (event,) = event_envelope.items

    payload = event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    # See https://develop.sentry.dev/sdk/telemetry/traces/dynamic-sampling-context/#dsc-specification
    trace_header = event_envelope.headers["trace"]
    # assert for random value to exist & be in the expected range
    assert ("sample_rand" in trace_header) and (0 <= trace_header["sample_rand"] < 1)
    del trace_header["sample_rand"]
    assert trace_header == {
        "environment": "development",
        "org_id": "",
        "public_key": "uiaeosnrtdy",
        "release": "test-example-release",
        "sample_rate": 1,
        "sampled": "true",
        "trace_id": "aaaabbbbccccddddeeeeffff00001111",
        "transaction": "little.teapot",
    }

    assert trace_context["trace_id"] == trace_header["trace_id"]


def test_event_trace_header(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "set-trace", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 1
    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)

    # Extract the one-and-only-item
    (event,) = event_envelope.items

    payload = event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    # See https://develop.sentry.dev/sdk/telemetry/traces/dynamic-sampling-context/#dsc-specification
    trace_header = event_envelope.headers["trace"]
    # assert for random value to exist & be in the expected range
    assert ("sample_rand" in trace_header) and (0 <= trace_header["sample_rand"] < 1)
    del trace_header["sample_rand"]
    assert trace_header == {
        "environment": "development",
        "org_id": "",
        "public_key": "uiaeosnrtdy",
        "release": "test-example-release",
        "sample_rate": 0,  # since we don't capture-transaction
        "sampled": "false",  # now sample_rand >= traces_sample_rate (=0)
        "trace_id": "aaaabbbbccccddddeeeeffff00001111",
    }

    assert trace_context["trace_id"] == trace_header["trace_id"]


def test_set_trace_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "set-trace", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 1
    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)

    # Extract the one-and-only-item
    (event,) = event_envelope.items

    payload = event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    assert_event(event_envelope, "Hello World!", "aaaabbbbccccddddeeeeffff00001111")
    assert trace_context["parent_span_id"]
    assert trace_context["parent_span_id"] == "f0f0f0f0f0f0f0f0"


def test_set_trace_transaction_scoped_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-transaction", "scope-transaction-event"],
        env=env,
    )

    assert len(httpserver.log) == 2
    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    tx_envelope = Envelope.deserialize(tx_body)
    event_envelope = Envelope.deserialize(event_body)

    # Show what the envelope looks like if the test fails.
    tx_envelope.print_verbose()

    # The transaction is overwritten.
    assert_meta(
        tx_envelope,
        transaction="little.teapot",
    )

    # Extract the transaction and event items
    (tx_event,) = tx_envelope.items
    (event,) = event_envelope.items

    assert tx_event.headers["type"] == "transaction"
    payload = tx_event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]

    assert trace_context["trace_id"]
    tx_trace_id = uuid.UUID(hex=trace_context["trace_id"])
    assert tx_trace_id
    event_trace_id = uuid.UUID(hex=event.payload.json["contexts"]["trace"]["trace_id"])
    # by default, transactions and events should have the same trace id (picked up from the propagation context)
    assert tx_trace_id == event_trace_id
    assert_event(event_envelope, "Hello World!", trace_context["trace_id"])
    # scoped tx should have the same span_id as the event span_id
    assert trace_context["span_id"]
    assert (
        trace_context["span_id"] == event.payload.json["contexts"]["trace"]["span_id"]
    )


def test_set_trace_transaction_update_from_header_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "capture-transaction",
            "update-tx-from-header",
            "scope-transaction-event",
        ],
        env=env,
    )

    assert len(httpserver.log) == 2
    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    tx_envelope = Envelope.deserialize(tx_body)
    event_envelope = Envelope.deserialize(event_body)

    # Show what the envelope looks like if the test fails.
    tx_envelope.print_verbose()

    # The transaction is overwritten.
    assert_meta(
        tx_envelope,
        transaction="little.teapot",
    )

    # Extract the transaction and event items
    (tx_event,) = tx_envelope.items
    (event,) = event_envelope.items

    assert tx_event.headers["type"] == "transaction"
    payload = tx_event.payload.json

    # See https://develop.sentry.dev/sdk/data-model/event-payloads/contexts/#trace-context
    trace_context = payload["contexts"]["trace"]
    expected_trace_id = "2674eb52d5874b13b560236d6c79ce8a"
    expected_parent_span_id = "a0f9fdf04f1a63df"

    assert trace_context["trace_id"]
    assert event.payload.json["contexts"]["trace"]["trace_id"]
    # Event should have the same trace_id as scoped span (set by update_from_header)
    assert (
        trace_context["trace_id"]
        == event.payload.json["contexts"]["trace"]["trace_id"]
        == expected_trace_id
    )
    assert_event(event_envelope, "Hello World!", trace_context["trace_id"])
    # scoped tx should have the same span_id as the event span_id
    assert trace_context["span_id"]
    assert (
        trace_context["span_id"] == event.payload.json["contexts"]["trace"]["span_id"]
    )
    # tx gets parent span_id from update_from_header
    assert trace_context["parent_span_id"]
    assert trace_context["parent_span_id"] == expected_parent_span_id


def test_capture_minidump(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "attachment", "attach-view-hierarchy", "capture-minidump"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_minidump(envelope)


def _setup_http_proxy_test(cmake, httpserver, proxy, proxy_auth=None):
    proxy_process = start_mitmdump(proxy, proxy_auth) if proxy else None

    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver, proxy_host=True))
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    return env, proxy_process, tmp_path


def test_proxy_from_env(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8080)
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event"],
            env=env,
        )

    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(1, httpserver, proxy_process)


def test_proxy_from_env_port_incorrect(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8081)
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event"],
            env=env,
        )

    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(0, httpserver, proxy_process)


def test_proxy_auth(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy", proxy_auth="user:password"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event", "http-proxy-auth"],
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver, proxy_host=True)),
        )
    finally:
        proxy_test_finally(
            1,
            httpserver,
            proxy_process,
            assert_failed_proxy_auth_request,
        )


def test_proxy_auth_incorrect(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy", proxy_auth="wrong:wrong"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event", "http-proxy-auth"],
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver, proxy_host=True)),
        )
    finally:
        proxy_test_finally(
            0,
            httpserver,
            proxy_process,
            assert_failed_proxy_auth_request,
        )


def test_proxy_ipv6(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event", "http-proxy-ipv6"],
            env=env,
        )

    finally:
        proxy_test_finally(1, httpserver, proxy_process)


def test_proxy_set_empty(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    setup_proxy_env_vars(port=8080)  # we start the proxy but expect it to remain unused
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event", "proxy-empty"],
            env=env,
        )

    finally:
        cleanup_proxy_env_vars()
        proxy_test_finally(1, httpserver, proxy_process, expected_proxy_logsize=0)


def test_proxy_https_not_http(cmake, httpserver):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    # we start the proxy but expect it to remain unused (dsn is http, so shouldn't use https proxy)
    os.environ["https_proxy"] = f"http://localhost:8080"
    try:
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, "http-proxy"
        )

        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event"],
            env=env,
        )

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
def test_capture_proxy(cmake, httpserver, run_args, proxy_running):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later
    expected_logsize = 0

    try:
        proxy_to_start = run_args[0] if proxy_running else None
        env, proxy_process, tmp_path = _setup_http_proxy_test(
            cmake, httpserver, proxy_to_start
        )
        run(
            tmp_path,
            "sentry_example",
            ["log", "capture-event"]
            + run_args,  # only passes if given proxy is running
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver, proxy_host=True)),
        )
        if proxy_running:
            expected_logsize = 1
        else:
            expected_logsize = 0
    finally:
        proxy_test_finally(expected_logsize, httpserver, proxy_process)


def test_capture_with_scope(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "attach-to-scope", "capture-with-scope"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_breadcrumb(envelope, "scoped crumb")
    assert_attachment(envelope)


def test_logs_timer(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "logs-timer"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2

    req_0 = httpserver.log[0][0]
    body_0 = req_0.get_data()

    envelope_0 = Envelope.deserialize(body_0)
    assert_logs(envelope_0, 10)

    req_1 = httpserver.log[1][0]
    body_1 = req_1.get_data()

    envelope_1 = Envelope.deserialize(body_1)
    assert_logs(envelope_1)


def test_logs_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "capture-event"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)
    # ensure that the event and the log are part of the same trace
    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    log_req = httpserver.log[1][0]
    log_body = log_req.get_data()

    log_envelope = Envelope.deserialize(log_body)
    assert_logs(log_envelope, 1, event_trace_id)


def test_logs_scoped_transaction(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "enable-logs",
            "logs-scoped-transaction",
            "capture-transaction",
            "scope-transaction-event",
        ],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 3

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)
    # ensure that the event and the log are part of the same trace
    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()

    tx_envelope = Envelope.deserialize(tx_body)
    # ensure that the transaction, event, and logs are part of the same trace
    tx_trace_id = tx_envelope.items[0].payload.json["contexts"]["trace"]["trace_id"]
    assert tx_trace_id == event_trace_id

    log_req = httpserver.log[2][0]
    log_body = log_req.get_data()

    log_envelope = Envelope.deserialize(log_body)
    assert_logs(log_envelope, 2, event_trace_id)


def test_logs_threaded(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "logs-threads"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # there is a chance we drop logs while flushing buffers
    assert 1 <= len(httpserver.log) <= 50
    total_count = 0

    for i in range(len(httpserver.log)):
        req = httpserver.log[i][0]
        body = req.get_data()

        envelope = Envelope.deserialize(body)
        assert_logs(envelope)
        total_count += envelope.items[0].headers["item_count"]
    print(f"Total amount of captured logs: {total_count}")
    assert total_count >= 100


def test_before_send_log(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "before-send-log"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails.
    envelope.print_verbose()

    # Extract the log item
    (log_item,) = envelope.items

    assert log_item.headers["type"] == "log"
    payload = log_item.payload.json

    # Get the first log item from the logs payload
    log_entry = payload["items"][0]
    attributes = log_entry["attributes"]

    # Check that the before_send_log callback added the expected attribute
    assert "coffeepot.size" in attributes
    assert attributes["coffeepot.size"]["value"] == "little"
    assert attributes["coffeepot.size"]["type"] == "string"


def test_before_send_log_discard(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "discarding-before-send-log"],
        env=env,
    )

    # log should have been discarded
    assert len(httpserver.log) == 0


def test_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    # log should have been discarded since we have no backend to hook into the crash
    assert len(httpserver.log) == 0


def test_inproc_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, crash_request = split_log_request_cond(
        httpserver.log, is_logs_envelope
    )
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, crash_request = split_log_request_cond(
        httpserver.log, is_logs_envelope
    )
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)


def test_logs_with_custom_attributes(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "log-attributes"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails
    envelope.print_verbose()

    # Extract the log item
    (log_item,) = envelope.items

    assert log_item.headers["type"] == "log"
    payload = log_item.payload.json

    # We expect 3 log entries based on the example
    assert len(payload["items"]) == 3

    # Test 1: Log with custom attributes and format string
    log_entry_0 = payload["items"][0]
    assert log_entry_0["body"] == "logging with 3 custom attributes"
    attributes_0 = log_entry_0["attributes"]

    # Check custom attributes exist
    assert "my.custom.attribute" in attributes_0
    assert attributes_0["my.custom.attribute"]["value"] == "my_attribute"
    assert attributes_0["my.custom.attribute"]["type"] == "string"

    assert "number.first" in attributes_0
    assert attributes_0["number.first"]["value"] == 2**63 - 1  # INT64_MAX
    assert attributes_0["number.first"]["type"] == "integer"
    assert attributes_0["number.first"]["unit"] == "fermions"

    assert "number.second" in attributes_0
    assert attributes_0["number.second"]["value"] == -(2**63)  # INT64_MIN
    assert attributes_0["number.second"]["type"] == "integer"
    assert attributes_0["number.second"]["unit"] == "bosons"

    # Check that format parameters were parsed
    assert "sentry.message.parameter.0" in attributes_0
    assert attributes_0["sentry.message.parameter.0"]["value"] == 3
    assert attributes_0["sentry.message.parameter.0"]["type"] == "integer"

    # Check that default attributes are still present
    assert "sentry.sdk.name" in attributes_0
    assert "sentry.sdk.version" in attributes_0

    # Test 2: Log with empty custom attributes object
    log_entry_1 = payload["items"][1]
    assert log_entry_1["body"] == "logging with no custom attributes"
    attributes_1 = log_entry_1["attributes"]

    # Should still have default attributes
    assert "sentry.sdk.name" in attributes_1
    assert "sentry.sdk.version" in attributes_1

    # Check that format string parameter was parsed
    assert "sentry.message.parameter.0" in attributes_1
    assert attributes_1["sentry.message.parameter.0"]["value"] == "no"
    assert attributes_1["sentry.message.parameter.0"]["type"] == "string"

    # Test 3: Log with custom attributes that override defaults
    log_entry_2 = payload["items"][2]
    assert log_entry_2["body"] == "logging with a custom parameter attribute"
    attributes_2 = log_entry_2["attributes"]

    # Check custom attribute exists
    assert "sentry.message.parameter.0" in attributes_2
    assert attributes_2["sentry.message.parameter.0"]["value"] == "parameter"
    assert attributes_2["sentry.message.parameter.0"]["type"] == "string"

    # Check that sentry.sdk.name was overwritten by custom attribute
    assert "sentry.sdk.name" in attributes_2
    assert attributes_2["sentry.sdk.name"]["value"] == "custom-sdk-name"
    assert attributes_2["sentry.sdk.name"]["type"] == "string"
