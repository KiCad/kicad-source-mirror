#include "sentry_logs.h"
#include "sentry_testsupport.h"

#include "sentry_envelope.h"
#include <string.h>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#else
#    include <unistd.h>
#    define sleep_ms(MILLISECONDS) usleep(MILLISECONDS * 1000)
#endif

typedef struct {
    uint64_t called_count;
    bool has_validation_error;
} transport_validation_data_t;

static void
validate_logs_envelope(sentry_envelope_t *envelope, void *data)
{
    transport_validation_data_t *validation_data = data;

    // Check we have at least one envelope item (store error flag instead of
    // TEST_CHECK)
    if (sentry__envelope_get_item_count(envelope) == 0) {
        validation_data->has_validation_error = true;
        sentry_envelope_free(envelope);
        return;
    }

    // Get the first item and check its type
    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    sentry_value_t type_header = sentry__envelope_item_get_header(item, "type");
    const char *type = sentry_value_as_string(type_header);

    // Only validate and count log envelopes, skip others (e.g., session)
    if (strcmp(type, "log") == 0) {
        validation_data->called_count += 1;
    }

    sentry_envelope_free(envelope);
}

SENTRY_TEST(basic_logging_functionality)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__logs_wait_for_thread_startup();

    // These should not crash and should respect the enable_logs option
    TEST_CHECK_INT_EQUAL(sentry_log_trace("Trace message"), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_debug("Debug message"), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_info("Info message"), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_warn("Warning message"), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_error("Error message"), 0);
    // Sleep to allow first batch to flush (testing batch timing behavior)
    sleep_ms(20);
    TEST_CHECK_INT_EQUAL(sentry_log_fatal("Fatal message"), 0);
    sentry_close();

    // Validate results on main thread (no race condition)
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 2);
}

SENTRY_TEST(logs_disabled_by_default)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    // Don't explicitly enable logs - they should be disabled by default
    sentry_init(options);

    TEST_CHECK_INT_EQUAL(sentry_log_info("This should not be sent"), 3);

    sentry_close();

    // Transport should not be called since logs are disabled
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 0);
}

SENTRY_TEST(formatted_log_messages)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__logs_wait_for_thread_startup();

    // Test format specifiers
    TEST_CHECK_INT_EQUAL(sentry_log_info("String: %s, Integer: %d, Float: %.2f",
                             "test", 42, 3.14),
        0);
    TEST_CHECK_INT_EQUAL(
        sentry_log_warn("Character: %c, Hex: 0x%x", 'A', 255), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_error("Pointer: %p", (void *)0x1234), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_error("Big number: %zu", UINT64_MAX), 0);
    TEST_CHECK_INT_EQUAL(sentry_log_error("Small number: %d", INT64_MIN), 0);

    sentry_close();

    // Transport should be called once
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

static void
test_param_conversion_helper(const char *format, ...)
{
    sentry_value_t attributes = sentry_value_new_object();
    va_list args;
    va_start(args, format);
    int param_count = populate_message_parameters(attributes, format, args);
    va_end(args);

    // Verify we got the expected number of parameters
    TEST_CHECK_INT_EQUAL(param_count, 3);

    // Verify the parameters were extracted correctly
    sentry_value_t param0
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.0");
    sentry_value_t param1
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.1");
    sentry_value_t param2
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.2");

    TEST_CHECK(!sentry_value_is_null(param0));
    TEST_CHECK(!sentry_value_is_null(param1));
    TEST_CHECK(!sentry_value_is_null(param2));

    // Check the values
    sentry_value_t value0 = sentry_value_get_by_key(param0, "value");
    sentry_value_t value1 = sentry_value_get_by_key(param1, "value");
    sentry_value_t value2 = sentry_value_get_by_key(param2, "value");

    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value0), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value1), 2);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value2), 3);

    sentry_value_decref(attributes);
}

SENTRY_TEST(logs_param_conversion)
{
    // TODO this test shows the current limitation for parsing integers on
    //  32-bit systems
    int a = 1, b = 2, c = 3;
#if defined(__i386__) || defined(_M_IX86) || defined(__arm__)
    // Currently, on 32-bit platforms, we need to cast to a 64-bit integer type
    // since the parameter conversion expects long long for %d format specifiers
    test_param_conversion_helper(
        "%" PRId64 " %" PRId64 " %" PRId64, (int64_t)a, (int64_t)b, (int64_t)c);
#else
    // since we read these values as 64-bit, this is still undefined behaviour
    // but it works because the variadic arguments are passed in 8-byte slots
    test_param_conversion_helper("%d %d %d", a, b, c);
#endif
}

static void
test_param_conversion_types(const char *format, ...)
{
    sentry_value_t attributes = sentry_value_new_object();
    va_list args;
    va_start(args, format);
    int param_count = populate_message_parameters(attributes, format, args);
    va_end(args);

    // Verify we got the expected number of parameters
    TEST_CHECK_INT_EQUAL(param_count, 7);

    // Verify the parameters were extracted correctly
    sentry_value_t param0
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.0");
    sentry_value_t param1
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.1");
    sentry_value_t param2
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.2");
    sentry_value_t param3
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.3");
    sentry_value_t param4
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.4");
    sentry_value_t param5
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.5");
    sentry_value_t param6
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.6");

    TEST_CHECK(!sentry_value_is_null(param0));
    TEST_CHECK(!sentry_value_is_null(param1));
    TEST_CHECK(!sentry_value_is_null(param2));
    TEST_CHECK(!sentry_value_is_null(param3));
    TEST_CHECK(!sentry_value_is_null(param4));
    TEST_CHECK(!sentry_value_is_null(param5));
    TEST_CHECK(!sentry_value_is_null(param6));

    // Check the values and types
    sentry_value_t value0 = sentry_value_get_by_key(param0, "value");
    sentry_value_t value1 = sentry_value_get_by_key(param1, "value");
    sentry_value_t value2 = sentry_value_get_by_key(param2, "value");
    sentry_value_t value3 = sentry_value_get_by_key(param3, "value");
    sentry_value_t value4 = sentry_value_get_by_key(param4, "value");
    sentry_value_t value5 = sentry_value_get_by_key(param5, "value");
    sentry_value_t value6 = sentry_value_get_by_key(param6, "value");

    sentry_value_t type0 = sentry_value_get_by_key(param0, "type");
    sentry_value_t type1 = sentry_value_get_by_key(param1, "type");
    sentry_value_t type2 = sentry_value_get_by_key(param2, "type");
    sentry_value_t type3 = sentry_value_get_by_key(param3, "type");
    sentry_value_t type4 = sentry_value_get_by_key(param4, "type");
    sentry_value_t type5 = sentry_value_get_by_key(param5, "type");
    sentry_value_t type6 = sentry_value_get_by_key(param6, "type");

    // Validate %u (unsigned) - should be string type with UINT64_MAX value
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type0), "string");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(value0), "18446744073709551615");

    // Validate %d (signed integer) - should be integer type with INT64_MIN
    // value
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type1), "integer");
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value1), INT64_MIN);

    // Validate %f (float) - should be double type with 3.14159 value
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type2), "double");
    TEST_CHECK(sentry_value_as_double(value2) == 3.14159);

    // Validate %c (character) - should be string type with 'A' value
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type3), "string");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(value3), "A");

    // Validate %s (string) - should be string type with "test" value
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type4), "string");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(value4), "test");

    // Validate %p (pointer) - should be string type with pointer representation
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type5), "string");
    // Pointer value format is platform-dependent, but should contain the hex
    // digits (can be upper- or lower-cased)
    const char *ptr_str = sentry_value_as_string(value5);
    TEST_CHECK(ptr_str != NULL);
    TEST_CHECK(strstr(ptr_str, "12345abc") != NULL
        || strstr(ptr_str, "12345ABC") != NULL);

    // Validate %x (hex uint64) - should be string type with hex representation
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type6), "string");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(value6), "deadbeefdeadbeef");

    sentry_value_decref(attributes);
}

SENTRY_TEST(logs_param_types)
{
    uint64_t a = UINT64_MAX;
    int64_t b = INT64_MIN;
    double c = 3.14159;
    char d = 'A';
    const char *e = "test";
    void *f = (void *)0x12345abc;
    uint64_t g = 0xDEADBEEFDEADBEEF;
    test_param_conversion_types("%u %d %f %c %s %p %x", a, b, c, d, e, f, g);
}

SENTRY_TEST(logs_force_flush)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__logs_wait_for_thread_startup();

    // These should not crash and should respect the enable_logs option
    TEST_CHECK_INT_EQUAL(sentry_log_trace("Trace message"), 0);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_log_debug("Debug message"), 0);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_log_info("Info message"), 0);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_log_warn("Warning message"), 0);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_log_error("Error message"), 0);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_log_fatal("Fatal message"), 0);
    sentry_flush(5000);
    sentry_close();

    // Validate results on main thread (no race condition)
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 6);
}

SENTRY_TEST(logs_custom_attributes_with_format_strings)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);
    sentry_options_set_logs_with_attributes(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__logs_wait_for_thread_startup();

    // Test 1: Custom attributes with format string
    sentry_value_t attributes1 = sentry_value_new_object();
    sentry_value_t attr1 = sentry_value_new_attribute(
        sentry_value_new_string("custom_value"), NULL);
    sentry_value_set_by_key(attributes1, "my.custom.attribute", attr1);
    TEST_CHECK_INT_EQUAL(sentry_log_info("User %s logged in with code %d",
                             attributes1, "Alice", 200),
        0);

    // Test 2: Null attributes with format string (should still work)
    TEST_CHECK_INT_EQUAL(sentry_log_warn("No custom attrs: %s has %d items",
                             sentry_value_new_null(), "cart", 5),
        0);

    // Test 3: Custom attributes with no format parameters
    sentry_value_t attributes2 = sentry_value_new_object();
    sentry_value_t attr2
        = sentry_value_new_attribute(sentry_value_new_int32(42), NULL);
    sentry_value_set_by_key(attributes2, "special.number", attr2);
    TEST_CHECK_INT_EQUAL(
        sentry_log_error("Simple message with custom attrs", attributes2), 0);

    // Test 4: Custom attributes with multiple format types
    sentry_value_t attributes3 = sentry_value_new_object();
    sentry_value_t attr3
        = sentry_value_new_attribute(sentry_value_new_string("tracking"), NULL);
    sentry_value_set_by_key(attributes3, "event.type", attr3);
    TEST_CHECK_INT_EQUAL(
        sentry_log_debug("Processing item %d of %d (%.1f%% complete)",
            attributes3, 3, 10, 30.0),
        0);

    sentry_close();

    // Validate that logs were sent
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}
