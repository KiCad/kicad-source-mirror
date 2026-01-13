#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_path.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_value.h"

static char *const SERIALIZED_ENVELOPE_STR
    = "{\"dsn\":\"https://foo@sentry.invalid/42\","
      "\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"trace\":{"
      "\"public_key\":\"foo\",\"org_id\":\"\",\"sample_rate\":0,\"sample_"
      "rand\":0.01006918276309107,\"release\":\"test-release\",\"environment\":"
      "\"production\",\"sampled\":\"false\"}}\n"
      "{\"type\":\"event\",\"length\":71}\n"
      "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"some-"
      "context\":1234}\n"
      "{\"type\":\"minidump\",\"length\":4}\n"
      "MDMP\n"
      "{\"type\":\"attachment\",\"length\":12}\n"
      "Hello World!";

SENTRY_TEST(basic_http_request_preparation_for_event)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"event\",\"length\":51}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_transaction)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t transaction = sentry_value_new_object();
    sentry_value_set_by_key(
        transaction, "event_id", sentry__value_new_uuid(&event_id));
    sentry_value_set_by_key(
        transaction, "type", sentry_value_new_string("transaction"));
    sentry__envelope_add_transaction(envelope, transaction);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\","
        "\"sent_at\":"
        "\"2021-12-16T05:53:59.343Z\"}\n"
        "{\"type\":\"transaction\",\"length\":72}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"type\":"
        "\"transaction\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_user_report)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t user_report;
    SENTRY_TEST_DEPRECATED(
        user_report = sentry_value_new_user_feedback(
            &event_id, "some-name", "some-email", "some-comment"));
    sentry__envelope_add_user_report(envelope, user_report);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"user_report\",\"length\":117}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"name\":"
        "\"some-name\",\"email\":\"some-email\",\"comments\":"
        "\"some-comment\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_value_decref(user_report);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_user_feedback)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t user_feedback = sentry_value_new_feedback(
        "some-message", "some-email", "some-name", &event_id);
    sentry__envelope_add_user_feedback(envelope, user_feedback);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    char *line1 = req->body;
    char *line1_end = strchr(line1, '\n');
    TEST_CHECK(line1_end != NULL);
    line1_end[0] = '\0';
    TEST_CHECK_STRING_EQUAL(
        line1, "{\"event_id\":\"4c035723-8638-4c3a-923f-2ab9d08b4018\"}");

    char *line2 = line1_end + 1;
    char *line2_end = strchr(line2, '\n');
    TEST_CHECK(line2_end != NULL);
    line2_end[0] = '\0';
    TEST_CHECK_STRING_EQUAL(line2, "{\"type\":\"feedback\",\"length\":269}");

    char *line3 = line2_end + 1;
    sentry_value_t line3_json = sentry__value_from_json(line3, strlen(line3));
    TEST_CHECK(!sentry_value_is_null(line3_json));

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(line3_json, "event_id")),
        "4c035723-8638-4c3a-923f-2ab9d08b4018");
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(line3_json, "timestamp")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(line3_json, "platform")),
        "native");

    sentry_value_t contexts = sentry_value_get_by_key(line3_json, "contexts");
    TEST_CHECK(!sentry_value_is_null(contexts));

    sentry_value_t actual = sentry_value_get_by_key(contexts, "feedback");
    TEST_CHECK(!sentry_value_is_null(actual));

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(actual, "message")),
        "some-message");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                actual, "contact_email")),
        "some-email");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(actual, "name")),
        "some-name");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                actual, "associated_event_id")),
        "c993afb6b4ac48a6b61b2558e601d65d");
    sentry_value_decref(line3_json);
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_event_with_attachment)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"event\",\"length\":51}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"attachment\",\"length\":12}\n"
        "Hello World!");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_minidump)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_envelope_t *envelope = sentry__envelope_new();
    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{}\n"
        "{\"type\":\"minidump\",\"length\":4}\n"
        "MDMP\n"
        "{\"type\":\"attachment\",\"length\":12}\n"
        "Hello World!");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

sentry_envelope_t *
create_test_envelope()
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "test-release");
    sentry_init(options);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry_value_set_by_key(
        event, "some-context", sentry_value_new_int32(1234));
    sentry__envelope_add_event(envelope, event);

    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");
    return envelope;
}

SENTRY_TEST(serialize_envelope)
{
    sentry_envelope_t *envelope = create_test_envelope();

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);
    char *str = sentry__stringbuilder_into_string(&sb);
    TEST_ASSERT(!!str);

    TEST_CHECK_STRING_EQUAL(str, SERIALIZED_ENVELOPE_STR);

    sentry_envelope_free(envelope);
    sentry_free(str);

    sentry_close();
}

SENTRY_TEST(basic_write_envelope_to_file)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    int rv = sentry_envelope_write_to_file(envelope, test_file_str);
    TEST_CHECK_INT_EQUAL(rv, 0);
    TEST_ASSERT(sentry__path_is_file(test_file_path));

    size_t test_file_size;
    char *test_file_content
        = sentry__path_read_to_buffer(test_file_path, &test_file_size);
    TEST_CHECK_INT_EQUAL(test_file_size, strlen(SERIALIZED_ENVELOPE_STR));
    TEST_CHECK_STRING_EQUAL(test_file_content, SERIALIZED_ENVELOPE_STR);

    sentry_free(test_file_content);
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(read_write_envelope_to_file_null)
{
    sentry_envelope_t *empty_envelope = sentry__envelope_new();

    TEST_CHECK(!sentry_envelope_read_from_file(NULL));
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(NULL, "irrelevant/path"), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(empty_envelope, NULL), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file_n(NULL, "irrelevant/path", 0), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file_n(empty_envelope, NULL, 0), 1);

    sentry_envelope_free(empty_envelope);
}

SENTRY_TEST(read_write_envelope_to_invalid_path)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX
        "./directory_that_does_not_exist/sentry_test_envelope";
    TEST_CHECK(!sentry_envelope_read_from_file(test_file_str));
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    int rv = sentry_envelope_write_to_file(envelope, test_file_str);
    TEST_CHECK_INT_EQUAL(rv, 1);

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(write_raw_envelope_to_file)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(envelope, test_file_str), 0);

    sentry_envelope_t *raw_envelope
        = sentry__envelope_from_path(test_file_path);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(raw_envelope, test_file_str), 0);

    size_t test_file_size;
    char *test_file_content
        = sentry__path_read_to_buffer(test_file_path, &test_file_size);
    TEST_CHECK_INT_EQUAL(test_file_size, strlen(SERIALIZED_ENVELOPE_STR));
    TEST_CHECK_STRING_EQUAL(test_file_content, SERIALIZED_ENVELOPE_STR);
    TEST_CHECK_INT_EQUAL(sentry__path_remove(test_file_path), 0);

    sentry_free(test_file_content);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_envelope_free(raw_envelope);
    sentry_close();
}

SENTRY_TEST(read_envelope_from_file)
{
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry__path_write_buffer(test_file_path, SERIALIZED_ENVELOPE_STR,
            strlen(SERIALIZED_ENVELOPE_STR)),
        0);

    sentry_envelope_t *envelope = sentry_envelope_read_from_file(test_file_str);
    TEST_CHECK(!!envelope);

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_envelope_get_header(envelope, "dsn")),
        "https://foo@sentry.invalid/42");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "c993afb6-b4ac-48a6-b61b-2558e601d65d");

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    char event_id_str[37];
    sentry_uuid_as_string(&event_id, event_id_str);
    TEST_CHECK_STRING_EQUAL(
        event_id_str, "c993afb6-b4ac-48a6-b61b-2558e601d65d");

    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 3);

    sentry_value_t event = sentry_envelope_get_event(envelope);
    TEST_CHECK(!sentry_value_is_null(event));
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(event, "some-context")),
        1234);

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!item);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "event");
    size_t ev_len = 0;
    TEST_CHECK_STRING_EQUAL(sentry__envelope_item_get_payload(item, &ev_len),
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\","
        "\"some-context\":1234}");

    const sentry_envelope_item_t *minidump
        = sentry__envelope_get_item(envelope, 1);
    TEST_CHECK(!!minidump);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(minidump, "type")),
        "minidump");
    size_t dmp_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(minidump, &dmp_len), "MDMP");
    TEST_CHECK_INT_EQUAL(dmp_len, 4);

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 2);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    size_t attachment_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment_len),
        "Hello World!");
    TEST_CHECK_INT_EQUAL(attachment_len, 12);

    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(deserialize_envelope)
{
    const char *buf
        = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\",\"dsn\":\"https:/"
          "/e12d836b15bb49d7bbf99e64295d995b:@sentry.io/42\"}\n"
          "{\"type\":\"attachment\",\"length\":10,\"content_type\":\"text/"
          "plain\",\"filename\":\"hello.txt\"}\n"
          "\xef\xbb\xbfHello\r\n\n"
          "{\"type\":\"event\",\"length\":41,\"content_type\":\"application/"
          "json\",\"filename\":\"application.log\"}\n"
          "{\"message\":\"hello world\",\"level\":\"error\"}\n";

    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, strlen(buf));
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 2);

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_envelope_get_header(envelope, "dsn")),
        "https://e12d836b15bb49d7bbf99e64295d995b:@sentry.io/42");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK_JSON_VALUE(sentry_envelope_get_event(envelope),
        "{\"message\":\"hello world\",\"level\":\"error\"}");

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "filename")),
        "hello.txt");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "content_type")),
        "text/plain");
    size_t attachment_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment_len),
        "\xef\xbb\xbfHello\r\n");
    TEST_CHECK_INT_EQUAL(attachment_len, 10);

    sentry_envelope_free(envelope);
}

static void
test_deserialize_envelope_empty_attachments(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 2);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    for (int i = 0; i < 2; ++i) {
        const sentry_envelope_item_t *attachment
            = sentry__envelope_get_item(envelope, i);
        TEST_CHECK(!!attachment);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry__envelope_item_get_header(attachment, "type")),
            "attachment");
        TEST_CHECK_INT_EQUAL(
            sentry_value_as_int32(
                sentry__envelope_item_get_header(attachment, "length")),
            0);
        size_t attachment_len = 0;
        TEST_CHECK(
            !sentry__envelope_item_get_payload(attachment, &attachment_len));
        TEST_CHECK_INT_EQUAL(attachment_len, 0);
    }

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_empty_attachments)
{
    const char *buf = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\"}\n"
                      "{\"type\":\"attachment\",\"length\":0}\n"
                      "\n"
                      "{\"type\":\"attachment\",\"length\":0}\n"
                      "\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_empty_attachments(buf, buf_len);

    // eof
    test_deserialize_envelope_empty_attachments(buf, buf_len - 1);
}

static void
test_deserialize_envelope_implicit_length(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(sentry__envelope_item_get_header(
                             attachment, "length")),
        0);
    size_t attachment1_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment1_len),
        "helloworld");
    TEST_CHECK_INT_EQUAL(attachment1_len, 10);

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_implicit_length)
{
    const char *buf = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\"}\n"
                      "{\"type\":\"attachment\"}\n"
                      "helloworld\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_implicit_length(buf, buf_len);

    // eof
    test_deserialize_envelope_implicit_length(buf, buf_len - 1);
}

SENTRY_TEST(deserialize_envelope_no_headers)
{
    const char *session = "{\"started\": "
                          "\"2020-02-07T14:16:00Z\",\"attrs\":{\"release\":"
                          "\"sentry-test@1.0.0\"}"
                          "}";
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{}\n"
        "{\"type\":\"session\"}\n"
        "%s",
        session);

    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, strlen(buf));
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "event_id")));
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!item);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "session");
    TEST_CHECK(
        sentry_value_is_null(sentry__envelope_item_get_header(item, "length")));
    size_t session_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(item, &session_len), session);
    TEST_CHECK_INT_EQUAL(session_len, strlen(session));

    sentry_envelope_free(envelope);
}

static void
test_deserialize_envelope_empty(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 0);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "event_id")));
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_empty)
{
    const char *buf = "{}\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_empty(buf, buf_len);

    // eof
    test_deserialize_envelope_empty(buf, buf_len - 1);
}

SENTRY_TEST(deserialize_envelope_invalid)
{
    TEST_CHECK(!sentry_envelope_deserialize("", 0));
    TEST_CHECK(!sentry_envelope_deserialize("{}", 0));
    TEST_CHECK(!sentry_envelope_deserialize("\n", 1));
    TEST_CHECK(!sentry_envelope_deserialize("{}\n{}", 5));
    TEST_CHECK(!sentry_envelope_deserialize("{}\ninvalid\n", 11));
    TEST_CHECK(!sentry_envelope_deserialize("invalid", 7));
    TEST_CHECK(!sentry_envelope_deserialize("{}\n{\"length\":-1}\n", 17));
    char buf[128];
    snprintf(buf, sizeof(buf), "{}\n{\"length\":%zu}\n", SIZE_MAX);
    TEST_CHECK(!sentry_envelope_deserialize(buf, strlen(buf)));
}
