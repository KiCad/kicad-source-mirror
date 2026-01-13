#include "sentry_json.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"
#include <locale.h>
#include <math.h>
#include <stdint.h>

SENTRY_TEST(value_null)
{
    sentry_value_t val = sentry_value_new_null();
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    TEST_CHECK(sentry_value_is_null(val));
    TEST_CHECK(sentry_value_as_int32(val) == 0);
    TEST_CHECK(isnan(sentry_value_as_double(val)));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(val), "");
    TEST_CHECK(!sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "null");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
    TEST_CHECK(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_bool)
{
    sentry_value_t val = sentry_value_new_bool(true);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    TEST_CHECK(sentry_value_as_int32(val) == 0);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "true");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));

    val = sentry_value_new_bool(false);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    TEST_CHECK(sentry_value_as_int32(val) == 0);
    TEST_CHECK(!sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "false");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
    TEST_CHECK(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_int32)
{
    sentry_value_t val = sentry_value_new_int32(42);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    TEST_CHECK(sentry_value_as_int32(val) == 42);
    TEST_CHECK(sentry_value_as_double(val) == 42.0);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "42");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_CHECK(sentry_value_refcount(val) == 1);

    for (int32_t i = -255; i < 255; i++) {
        val = sentry_value_new_int32(i);
        TEST_CHECK_INT_EQUAL((int)i, (int)sentry_value_as_int32(val));
        TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    }

    val = sentry_value_new_int32(-1);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    TEST_CHECK(sentry_value_as_int32(val) == -1);
    TEST_CHECK(sentry_value_is_true(val) == true);
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
    TEST_CHECK(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_int64)
{
    sentry_value_t val = sentry_value_new_int64(42LL);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT64);
    TEST_CHECK(sentry_value_as_int64(val) == 42LL);
    // We don't convert int64 to double
    TEST_CHECK(isnan(sentry_value_as_double(val)));
    // We don't convert int64 to int32
    TEST_CHECK(sentry_value_as_int32(val) == 0);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "42");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    // Test large positive value
    val = sentry_value_new_int64(INT64_MAX);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT64);
    TEST_CHECK(sentry_value_as_int64(val) == INT64_MAX);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "9223372036854775807");
    sentry_value_decref(val);

    // Test large negative value
    val = sentry_value_new_int64(INT64_MIN);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT64);
    TEST_CHECK(sentry_value_as_int64(val) == INT64_MIN);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "-9223372036854775808");
    sentry_value_decref(val);

    // Test zero
    val = sentry_value_new_int64(0LL);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT64);
    TEST_CHECK(sentry_value_as_int64(val) == 0LL);
    TEST_CHECK(!sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "0");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    // We do convert int32 to int64
    val = sentry_value_new_int32(42);
    TEST_CHECK(sentry_value_as_int64(val) == 42LL);
    sentry_value_decref(val);

    // We don't convert uint64 to int64
    val = sentry_value_new_uint64(-42LL);
    TEST_CHECK(sentry_value_as_int64(val) == 0);
    sentry_value_decref(val);

    // We don't convert double to int64
    val = sentry_value_new_double(42.99);
    TEST_CHECK(sentry_value_as_int64(val) == 0);
    sentry_value_decref(val);
}

SENTRY_TEST(value_uint64)
{
    sentry_value_t val = sentry_value_new_uint64(42ULL);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_UINT64);
    TEST_CHECK(sentry_value_as_uint64(val) == 42ULL);
    // We don't convert uint64 to double
    TEST_CHECK(isnan(sentry_value_as_double(val)));
    // We don't convert uint64 to int32
    TEST_CHECK(sentry_value_as_int32(val) == 0);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "42");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    // Test large positive value
    val = sentry_value_new_uint64(UINT64_MAX);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_UINT64);
    TEST_CHECK(sentry_value_as_uint64(val) == UINT64_MAX);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "18446744073709551615");
    sentry_value_decref(val);

    // Test zero
    val = sentry_value_new_uint64(0ULL);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_UINT64);
    TEST_CHECK(sentry_value_as_uint64(val) == 0ULL);
    TEST_CHECK(!sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "0");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    // We don't convert int32 to uint64
    val = sentry_value_new_int32(42);
    TEST_CHECK(sentry_value_as_uint64(val) == 0);
    sentry_value_decref(val);

    // We don't convert double to uint64
    val = sentry_value_new_double(123.456);
    TEST_CHECK(sentry_value_as_uint64(val) == 0);
    sentry_value_decref(val);

    // We don't convert int64 to uint64
    val = sentry_value_new_int64(42LL);
    TEST_CHECK(sentry_value_as_uint64(val) == 0);
    sentry_value_decref(val);
}

SENTRY_TEST(value_double)
{
    sentry_value_t val = sentry_value_new_double(42.05);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    TEST_CHECK(sentry_value_as_double(val) == 42.05);
    TEST_CHECK(sentry_value_is_true(val));
    TEST_CHECK_JSON_VALUE(val, "42.05");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    val = sentry_value_new_double(4294967295.);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    TEST_CHECK(sentry_value_as_double(val) == 4294967295.);
    TEST_CHECK_JSON_VALUE(val, "4294967295");
    sentry_value_decref(val);
}

SENTRY_TEST(value_string)
{
    sentry_value_t val = sentry_value_new_string("Hello World!\n\t\r\f");
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    TEST_CHECK(sentry_value_is_true(val) == true);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(val), "Hello World!\n\t\r\f");
    TEST_CHECK_JSON_VALUE(val, "\"Hello World!\\n\\t\\r\\f\"");
    TEST_CHECK(sentry_value_refcount(val) == 1);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
}

SENTRY_TEST(value_string_n)
{
    sentry_value_t val = sentry_value_new_string_n(NULL, 0);
    TEST_CHECK(sentry_value_is_null(val));
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    TEST_CHECK(sentry_value_is_true(val) == false);
    sentry_value_decref(val);

    char non_null_term_empty_str[] = { 'h', 'e', 'l', 'l', 'o' };
    val = sentry_value_new_string_n(
        non_null_term_empty_str, sizeof(non_null_term_empty_str));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(val), "hello");
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    TEST_CHECK(sentry_value_is_true(val) == true);
    sentry_value_decref(val);
}

SENTRY_TEST(value_unicode)
{
    // https://xkcd.com/1813/ :-)
    sentry_value_t val = sentry_value_new_string("őá…–🤮🚀¿ 한글 테스트 \a\v");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(val), "őá…–🤮🚀¿ 한글 테스트 \a\v");
    // json does not need to escape unicode, except for control characters
    TEST_CHECK_JSON_VALUE(val, "\"őá…–🤮🚀¿ 한글 테스트 \\u0007\\u000b\"");
    sentry_value_decref(val);
    char zalgo[] = "z̴̢̈͜ä̴̺̟́ͅl̸̛̦͎̺͂̃̚͝g̷̦̲͊͋̄̌͝o̸͇̞̪͙̞͌̇̀̓̏͜";
    val = sentry_value_new_string(zalgo);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(val), zalgo);
    sentry_value_decref(val);
}

SENTRY_TEST(value_list)
{
    sentry_value_t val = sentry_value_new_list();
    for (size_t i = 0; i < 10; i++) {
        TEST_CHECK(
            !sentry_value_append(val, sentry_value_new_int32((int32_t)i)));
    }
    for (size_t i = 0; i < 20; i++) {
        sentry_value_t child = sentry_value_get_by_index(val, i);
        if (i < 10) {
            TEST_CHECK(sentry_value_get_type(child) == SENTRY_VALUE_TYPE_INT32);
            TEST_CHECK(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            TEST_CHECK(sentry_value_is_null(child));
        }
    }
    TEST_CHECK(sentry_value_get_length(val) == 10);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_LIST);
    TEST_CHECK(sentry_value_is_true(val) == true);
    TEST_CHECK_JSON_VALUE(val, "[0,1,2,3,4,5,6,7,8,9]");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    TEST_CHECK(sentry_value_is_true(val) == false);
    TEST_CHECK_JSON_VALUE(val, "[]");
    sentry_value_t copy = sentry__value_clone(val);
    TEST_CHECK_JSON_VALUE(copy, "[]");
    sentry_value_decref(copy);
    sentry_value_decref(val);

    val = sentry_value_new_list();
    sentry_value_set_by_index(val, 5, sentry_value_new_int32(100));
    sentry_value_set_by_index(val, 2, sentry_value_new_int32(10));
    TEST_CHECK_JSON_VALUE(val, "[null,null,10,null,null,100]");
    sentry_value_remove_by_index(val, 2);
    TEST_CHECK_JSON_VALUE(val, "[null,null,null,null,100]");
    TEST_CHECK(!sentry_value_is_frozen(val));
    sentry_value_freeze(val);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
}

SENTRY_TEST(value_object)
{
    sentry_value_t val = sentry_value_new_object();
    for (size_t i = 0; i < 10; i++) {
        char key[100];
        snprintf(key, sizeof(key), "key%d", (int)i);
        sentry_value_set_by_key(val, key, sentry_value_new_int32((int32_t)i));
    }
    for (size_t i = 0; i < 20; i++) {
        char key[100];
        snprintf(key, sizeof(key), "key%d", (int)i);
        sentry_value_t child = sentry_value_get_by_key(val, key);
        if (i < 10) {
            TEST_CHECK(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            TEST_CHECK(sentry_value_is_null(child));
        }
    }

    TEST_CHECK(sentry_value_get_length(val) == 10);
    TEST_CHECK(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK(sentry_value_is_true(val) == true);
    TEST_CHECK_JSON_VALUE(val,
        "{\"key0\":0,\"key1\":1,\"key2\":2,\"key3\":3,\"key4\":4,\"key5\":5,"
        "\"key6\":6,\"key7\":7,\"key8\":8,\"key9\":9}");

    sentry_value_t val2 = sentry__value_clone(val);
    sentry_value_decref(val);
    val = val2;
    sentry_value_set_by_key(val, "key1", sentry_value_new_int32(100));

    for (size_t i = 0; i < 10; i += 2) {
        char key[100];
        snprintf(key, sizeof(key), "key%d", (int)i);
        sentry_value_remove_by_key(val, key);
    }

    TEST_CHECK(sentry_value_get_length(val) == 5);
    TEST_CHECK_JSON_VALUE(
        val, "{\"key1\":100,\"key3\":3,\"key5\":5,\"key7\":7,\"key9\":9}");

    sentry_value_decref(val);

    val = sentry_value_new_object();
    TEST_CHECK(sentry_value_is_true(val) == false);
    TEST_CHECK_JSON_VALUE(val, "{}");
    TEST_CHECK(!sentry_value_is_frozen(val));
    sentry_value_freeze(val);
    TEST_CHECK(sentry_value_is_frozen(val));
    sentry_value_decref(val);
}

SENTRY_TEST(value_object_merge)
{
    sentry_value_t dst = sentry_value_new_object();
    sentry_value_set_by_key(dst, "a", sentry_value_new_int32(1));
    sentry_value_set_by_key(dst, "b", sentry_value_new_int32(2));

    sentry_value_t src = sentry_value_new_object();
    sentry_value_set_by_key(src, "b", sentry_value_new_int32(20));
    sentry_value_set_by_key(src, "c", sentry_value_new_int32(30));

    int rv = sentry__value_merge_objects(dst, src);
    TEST_CHECK_INT_EQUAL(rv, 0);
    sentry_value_decref(src);

    sentry_value_t a = sentry_value_get_by_key(dst, "a");
    sentry_value_t b = sentry_value_get_by_key(dst, "b");
    sentry_value_t c = sentry_value_get_by_key(dst, "c");
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(a), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(b), 2);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(c), 30);

    sentry_value_decref(dst);
}

SENTRY_TEST(value_object_merge_nested)
{
    sentry_value_t dst = sentry_value_new_object();
    sentry_value_set_by_key(dst, "a", sentry_value_new_int32(1));
    sentry_value_t dst_nested = sentry_value_new_object();
    sentry_value_set_by_key(dst_nested, "ba", sentry_value_new_int32(1));
    sentry_value_set_by_key(dst_nested, "bb", sentry_value_new_int32(2));
    sentry_value_set_by_key(dst, "b", dst_nested);

    sentry_value_t src = sentry_value_new_object();
    sentry_value_t src_nested = sentry_value_new_object();
    sentry_value_set_by_key(src_nested, "bb", sentry_value_new_int32(20));
    sentry_value_set_by_key(src_nested, "bc", sentry_value_new_int32(30));
    sentry_value_set_by_key(src, "b", src_nested);

    int rv = sentry__value_merge_objects(dst, src);
    TEST_CHECK_INT_EQUAL(rv, 0);
    sentry_value_decref(src);

    sentry_value_t a = sentry_value_get_by_key(dst, "a");
    sentry_value_t nested = sentry_value_get_by_key(dst, "b");
    sentry_value_t ba = sentry_value_get_by_key(nested, "ba");
    sentry_value_t bb = sentry_value_get_by_key(nested, "bb");
    sentry_value_t bc = sentry_value_get_by_key(nested, "bc");
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(a), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(ba), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(bb), 2);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(bc), 30);

    sentry_value_decref(dst);
}

SENTRY_TEST(value_user)
{
    const char *id = "42";
    const char *username = "John Doe";
    const char *email = "john.doe@example.com";
    const char *ip_address = "127.0.0.1";
    sentry_value_t user
        = sentry_value_new_user(id, username, email, ip_address);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "id")), "42");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "username")),
        "John Doe");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "email")),
        "john.doe@example.com");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "ip_address")),
        "127.0.0.1");
    sentry_value_decref(user);

    sentry_value_t user_half = sentry_value_new_user(id, username, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_half, "id")), "42");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_half, "username")),
        "John Doe");
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_half, "email")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_half, "ip_address")));
    sentry_value_decref(user_half);

    sentry_value_t user_null = sentry_value_new_user(NULL, NULL, NULL, NULL);
    TEST_CHECK(sentry_value_is_null(user_null));
    sentry_value_decref(user_null);

    sentry_value_t user_empty_str = sentry_value_new_user("", "", "", "");
    TEST_CHECK(sentry_value_is_null(user_empty_str));
    sentry_value_decref(user_empty_str);
}

SENTRY_TEST(value_attribute)
{
    // Test valid attribute types
    sentry_value_t string_attr = sentry_value_new_attribute(
        sentry_value_new_string("test_value"), NULL);
    TEST_CHECK(sentry_value_get_type(string_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(string_attr, "type")),
        "string");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(string_attr, "value")),
        "test_value");
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(string_attr, "unit")));
    sentry_value_decref(string_attr);

    sentry_value_t integer_attr
        = sentry_value_new_attribute(sentry_value_new_int32(42), NULL);
    TEST_CHECK(sentry_value_get_type(integer_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(integer_attr, "type")),
        "integer");
    TEST_CHECK(
        sentry_value_as_int32(sentry_value_get_by_key(integer_attr, "value"))
        == 42);
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(integer_attr, "unit")));
    sentry_value_decref(integer_attr);

    sentry_value_t double_attr
        = sentry_value_new_attribute(sentry_value_new_double(3.14), NULL);
    TEST_CHECK(sentry_value_get_type(double_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(double_attr, "type")),
        "double");
    TEST_CHECK(
        sentry_value_as_double(sentry_value_get_by_key(double_attr, "value"))
        == 3.14);
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(double_attr, "unit")));
    sentry_value_decref(double_attr);

    sentry_value_t boolean_attr
        = sentry_value_new_attribute(sentry_value_new_bool(true), NULL);
    TEST_CHECK(sentry_value_get_type(boolean_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(boolean_attr, "type")),
        "boolean");
    TEST_CHECK(
        sentry_value_is_true(sentry_value_get_by_key(boolean_attr, "value")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(boolean_attr, "unit")));
    sentry_value_decref(boolean_attr);

    // Test attribute with unit
    sentry_value_t attr_with_unit
        = sentry_value_new_attribute(sentry_value_new_int32(100), "percent");
    TEST_CHECK(
        sentry_value_get_type(attr_with_unit) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(attr_with_unit, "type")),
        "integer");
    TEST_CHECK(
        sentry_value_as_int32(sentry_value_get_by_key(attr_with_unit, "value"))
        == 100);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(attr_with_unit, "unit")),
        "percent");
    sentry_value_decref(attr_with_unit);

    // Test invalid sentry_value_t types
    sentry_value_t invalid_attr
        = sentry_value_new_attribute(sentry_value_new_list(), NULL);
    TEST_CHECK(sentry_value_is_null(invalid_attr));
    sentry_value_decref(invalid_attr);

    // Test NULL type
    sentry_value_t null_type_attr
        = sentry_value_new_attribute(sentry_value_new_null(), NULL);
    TEST_CHECK(sentry_value_is_null(null_type_attr));
    sentry_value_decref(null_type_attr);

    // Test object type
    sentry_value_t object_type_attr
        = sentry_value_new_attribute(sentry_value_new_object(), NULL);
    TEST_CHECK(sentry_value_is_null(object_type_attr));
    sentry_value_decref(object_type_attr);

    // Test _n version with explicit lengths
    sentry_value_t string_attr_n = sentry_value_new_attribute_n(
        sentry_value_new_string("test_n"), "bytes", 5);
    TEST_CHECK(
        sentry_value_get_type(string_attr_n) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(string_attr_n, "type")),
        "string");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(string_attr_n, "value")),
        "test_n");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(string_attr_n, "unit")),
        "bytes");
    sentry_value_decref(string_attr_n);

    // Test list attribute types (arrays)
    sentry_value_t string_list = sentry_value_new_list();
    sentry_value_append(string_list, sentry_value_new_string("foo"));
    sentry_value_append(string_list, sentry_value_new_string("bar"));
    sentry_value_t string_list_attr
        = sentry_value_new_attribute(string_list, NULL);
    TEST_CHECK(
        sentry_value_get_type(string_list_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                string_list_attr, "type")),
        "string[]");
    sentry_value_t string_list_value
        = sentry_value_get_by_key(string_list_attr, "value");
    TEST_CHECK(
        sentry_value_get_type(string_list_value) == SENTRY_VALUE_TYPE_LIST);
    TEST_CHECK(sentry_value_get_length(string_list_value) == 2);
    sentry_value_decref(string_list_attr);

    sentry_value_t integer_list = sentry_value_new_list();
    sentry_value_append(integer_list, sentry_value_new_int32(1));
    sentry_value_append(integer_list, sentry_value_new_int32(2));
    sentry_value_append(integer_list, sentry_value_new_int32(3));
    sentry_value_t integer_list_attr
        = sentry_value_new_attribute(integer_list, NULL);
    TEST_CHECK(
        sentry_value_get_type(integer_list_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                integer_list_attr, "type")),
        "integer[]");
    sentry_value_decref(integer_list_attr);

    sentry_value_t double_list = sentry_value_new_list();
    sentry_value_append(double_list, sentry_value_new_double(1.1));
    sentry_value_append(double_list, sentry_value_new_double(2.2));
    sentry_value_t double_list_attr
        = sentry_value_new_attribute(double_list, NULL);
    TEST_CHECK(
        sentry_value_get_type(double_list_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                double_list_attr, "type")),
        "double[]");
    sentry_value_decref(double_list_attr);

    sentry_value_t boolean_list = sentry_value_new_list();
    sentry_value_append(boolean_list, sentry_value_new_bool(true));
    sentry_value_append(boolean_list, sentry_value_new_bool(false));
    sentry_value_t boolean_list_attr
        = sentry_value_new_attribute(boolean_list, NULL);
    TEST_CHECK(
        sentry_value_get_type(boolean_list_attr) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                boolean_list_attr, "type")),
        "boolean[]");
    sentry_value_decref(boolean_list_attr);

    // Test empty list (should return null since first element is null)
    sentry_value_t empty_list = sentry_value_new_list();
    sentry_value_t empty_list_attr
        = sentry_value_new_attribute(empty_list, NULL);
    TEST_CHECK(sentry_value_is_null(empty_list_attr));
    sentry_value_decref(empty_list_attr);

    // Test list with nested list (unsupported, should return null)
    sentry_value_t nested_list = sentry_value_new_list();
    sentry_value_append(nested_list, sentry_value_new_list());
    sentry_value_t nested_list_attr
        = sentry_value_new_attribute(nested_list, NULL);
    TEST_CHECK(sentry_value_is_null(nested_list_attr));
    sentry_value_decref(nested_list_attr);

    // Test list with object (unsupported, should return null)
    sentry_value_t object_list = sentry_value_new_list();
    sentry_value_append(object_list, sentry_value_new_object());
    sentry_value_t object_list_attr
        = sentry_value_new_attribute(object_list, NULL);
    TEST_CHECK(sentry_value_is_null(object_list_attr));
    sentry_value_decref(object_list_attr);
}

SENTRY_TEST(value_freezing)
{
    sentry_value_t val = sentry_value_new_list();
    sentry_value_t inner = sentry_value_new_object();
    sentry_value_append(val, inner);
    TEST_CHECK(!sentry_value_is_frozen(val));
    TEST_CHECK(!sentry_value_is_frozen(inner));
    sentry_value_freeze(val);
    TEST_CHECK(sentry_value_is_frozen(val));
    TEST_CHECK(sentry_value_is_frozen(inner));

    TEST_CHECK_INT_EQUAL(sentry_value_append(val, sentry_value_new_bool(1)), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(val), 1);

    TEST_CHECK_INT_EQUAL(
        sentry_value_set_by_key(inner, "foo", sentry_value_new_bool(1)), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(inner), 0);

    sentry_value_decref(val);
}

SENTRY_TEST(value_stringify)
{
#define STRINGIFY_AND_CHECK(Val, Expected)                                     \
    do {                                                                       \
        char *stringified = sentry__value_stringify(Val);                      \
        TEST_CHECK_STRING_EQUAL(stringified, Expected);                        \
        sentry_free(stringified);                                              \
        sentry_value_decref(Val);                                              \
    } while (0)

#define STRINGIFY_AND_CHECK_CONTAINS(Val, Expected)                            \
    do {                                                                       \
        char *stringified = sentry__value_stringify(Val);                      \
        for (char *p = stringified; *p; ++p)                                   \
            *p = tolower(*p);                                                  \
        TEST_CHECK(strstr(stringified, Expected) != NULL);                     \
        sentry_free(stringified);                                              \
        sentry_value_decref(Val);                                              \
    } while (0)

    sentry_value_t rv = sentry_value_new_list();
    STRINGIFY_AND_CHECK(rv, "");

    rv = sentry_value_new_object();
    STRINGIFY_AND_CHECK(rv, "");

    rv = sentry_value_new_null();
    STRINGIFY_AND_CHECK(rv, "");

    rv = sentry_value_new_bool(true);
    STRINGIFY_AND_CHECK(rv, "true");

    rv = sentry_value_new_bool(false);
    STRINGIFY_AND_CHECK(rv, "false");

    rv = sentry_value_new_string("hello");
    STRINGIFY_AND_CHECK(rv, "hello");

    rv = sentry_value_new_int64(INT64_MIN);
    STRINGIFY_AND_CHECK(rv, "-9223372036854775808");

    rv = sentry_value_new_uint64(UINT64_MAX);
    STRINGIFY_AND_CHECK(rv, "18446744073709551615");

    rv = sentry_value_new_int32(42);
    STRINGIFY_AND_CHECK(rv, "42");

    rv = sentry_value_new_int32(INT32_MAX);
    STRINGIFY_AND_CHECK(rv, "2147483647");

    rv = sentry_value_new_double(3.14);
    STRINGIFY_AND_CHECK(rv, "3.14");

    rv = sentry_value_new_double(1000000000000000);
    STRINGIFY_AND_CHECK(rv, "1e+15");

    rv = sentry_value_new_double(INFINITY);
    STRINGIFY_AND_CHECK_CONTAINS(rv, "inf");

    rv = sentry_value_new_double(NAN);
    STRINGIFY_AND_CHECK_CONTAINS(rv, "nan");
}

#define STRING(X) X, (sizeof(X) - 1)

SENTRY_TEST(value_json_parsing)
{
    sentry_value_t rv;

    rv = sentry__value_from_json(STRING("42"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_INT32);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(rv), 42);
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("-9223372036854775808"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_INT64);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(rv), INT64_MIN);
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("-9223372036854775809"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_DOUBLE);
    TEST_CHECK_INT_EQUAL(sentry_value_as_double(rv), -9.2233720368547758E+18);
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("18446744073709551615"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_UINT64);
    TEST_CHECK_UINT64_EQUAL(sentry_value_as_uint64(rv), UINT64_MAX);
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("18446744073709551616"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_DOUBLE);
    TEST_CHECK(sentry_value_as_double(rv) == 1.8446744073709552E+19);
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("false"));
    TEST_CHECK(sentry_value_get_type(rv) == SENTRY_VALUE_TYPE_BOOL);
    TEST_CHECK(!sentry_value_is_true(rv));
    sentry_value_decref(rv);

    rv = sentry__value_from_json(STRING("[42, \"foo\\u2603\"]"));
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_index(rv, 0)), 42);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_index(rv, 1)),
        "foo\xe2\x98\x83");
    sentry_value_decref(rv);

    rv = sentry__value_from_json(
        STRING("[false, 42, \"foo\\u2603\", \"bar\", {\"foo\": 42}]"));
    TEST_CHECK_JSON_VALUE(rv, "[false,42,\"foo☃\",\"bar\",{\"foo\":42}]");
    sentry_value_decref(rv);

    rv = sentry__value_from_json(
        STRING("{\"escapes\": "
               "\"quot: \\\", backslash: \\\\, slash: \\/, backspace: \\b, "
               "formfeed: \\f, linefeed: \\n, carriage: \\r, tab: \\t\", "
               "\"surrogates\": "
               "\"\\uD801\\udc37\"}"));
    // escaped forward slashes are parsed, but not generated
    TEST_CHECK_JSON_VALUE(rv,
        "{\"escapes\":"
        "\"quot: \\\", backslash: \\\\, slash: /, backspace: \\b, "
        "formfeed: \\f, linefeed: \\n, carriage: \\r, tab: \\t\","
        "\"surrogates\":\"𐐷\"}");
    sentry_value_decref(rv);

    // unmatched surrogates don't parse
    rv = sentry__value_from_json(STRING("\"\\uD801\""));
    TEST_CHECK(sentry_value_is_null(rv));
    rv = sentry__value_from_json(
        STRING("{\"valid key\": true, \"invalid key \\uD801\": false}"));
    TEST_CHECK_JSON_VALUE(rv, "{\"valid key\":true}");
    sentry_value_decref(rv);
}

SENTRY_TEST(value_json_deeply_nested)
{
    sentry_value_t root = sentry_value_new_list();
    sentry_value_t child = root;
    for (int i = 0; i < 128; i++) {
        sentry_value_t new_child;
        if (i % 2) {
            // odd = object
            sentry_value_set_by_key(child, "_1", sentry_value_new_null());
            new_child = sentry_value_new_list();
            sentry_value_set_by_key(child, "_2", new_child);
            sentry_value_set_by_key(child, "_3", sentry_value_new_null());
        } else {
            // even = list
            sentry_value_append(child, sentry_value_new_null());
            new_child = sentry_value_new_object();
            sentry_value_append(child, new_child);
            sentry_value_append(child, sentry_value_new_null());
        }
        child = new_child;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    TEST_ASSERT(!!jw);
    sentry__jsonwriter_write_value(jw, root);
    size_t serialized_len = 0;
    char *serialized = sentry__jsonwriter_into_string(jw, &serialized_len);
    TEST_ASSERT(!!serialized);
    sentry_value_decref(root);

    sentry_value_t parsed = sentry__value_from_json(serialized, serialized_len);
    sentry_free(serialized);

    TEST_CHECK(!sentry_value_is_null(parsed));
    sentry_value_decref(parsed);
}

SENTRY_TEST(value_json_escaping)
{
    sentry_value_t rv = sentry__value_from_json(
        STRING("{\"escapes\": "
               "\"quot: \\\", backslash: \\\\, slash: \\/, backspace: \\b, "
               "formfeed: \\f, linefeed: \\n, carriage: \\r, tab: \\t\"}"));
    // escaped forward slashes are parsed, but not generated
    TEST_CHECK_JSON_VALUE(rv,
        "{\"escapes\":"
        "\"quot: \\\", backslash: \\\\, slash: /, backspace: \\b, "
        "formfeed: \\f, linefeed: \\n, carriage: \\r, tab: \\t\"}");
    sentry_value_decref(rv);

    // trailing blackslash
    rv = sentry__value_from_json(STRING("\"\\\""));
    TEST_CHECK(sentry_value_is_null(rv));
}

SENTRY_TEST(value_json_surrogates)
{
    sentry_value_t rv = sentry__value_from_json(
        STRING("{\"surrogates\": \"oh \\uD801\\udc37 hi\"}"));
    TEST_CHECK_JSON_VALUE(rv, "{\"surrogates\":\"oh 𐐷 hi\"}");
    sentry_value_decref(rv);

    // unmatched surrogates don't parse
    rv = sentry__value_from_json(STRING("\"\\uD801\""));
    TEST_CHECK(sentry_value_is_null(rv));
    rv = sentry__value_from_json(
        STRING("{\"valid key\": true, \"invalid key \\uD801\": false}"));
    TEST_CHECK_JSON_VALUE(rv, "{\"valid key\":true}");
    sentry_value_decref(rv);
}

SENTRY_TEST(value_json_locales)
{
    // we set a locale that uses decimal-commas to make sure we parse/stringify
    // correctly with a decimal dot.
    setlocale(LC_ALL, "de-DE");

    sentry_value_t rv = sentry__value_from_json(
        STRING("{\"dbl_max\": 1.7976931348623158e+308,"
               "\"dbl_min\": 2.2250738585072014e-308,"
               "\"max_int32\": 4294967295,"
               "\"max_safe_int\": 9007199254740991}"));

    // thou shalt not use exact comparison for floating point values
    TEST_CHECK(sentry_value_as_double(sentry_value_get_by_key(rv, "dbl_max"))
        == 1.7976931348623158e+308);
    TEST_CHECK(sentry_value_as_double(sentry_value_get_by_key(rv, "dbl_min"))
        == 2.2250738585072014e-308);

    TEST_CHECK(sentry_value_as_int64(sentry_value_get_by_key(rv, "max_int32"))
        == 4294967295.);
    TEST_CHECK(
        sentry_value_as_int64(sentry_value_get_by_key(rv, "max_safe_int"))
        == 9007199254740991.);

    // we format to 16 digits:
    TEST_CHECK_JSON_VALUE(rv,
        "{\"dbl_max\":1.797693134862316e+308,"
        "\"dbl_min\":2.225073858507201e-308,"
        "\"max_int32\":4294967295,"
        "\"max_safe_int\":9007199254740991}");

    sentry_value_decref(rv);
}

SENTRY_TEST(value_json_invalid_doubles)
{
    sentry_value_t val;

    val = sentry_value_new_double(INFINITY);
    TEST_CHECK_JSON_VALUE(val, "null");
    sentry_value_decref(val);

    val = sentry_value_new_double(-INFINITY);
    TEST_CHECK_JSON_VALUE(val, "null");
    sentry_value_decref(val);

    val = sentry_value_new_double(NAN);
    TEST_CHECK_JSON_VALUE(val, "null");
    sentry_value_decref(val);
}

SENTRY_TEST(value_wrong_type)
{
    sentry_value_t val = sentry_value_new_null();

    TEST_CHECK(sentry_value_set_by_key(val, "foobar", val) == 1);
    TEST_CHECK(sentry_value_remove_by_key(val, "foobar") == 1);
    TEST_CHECK(sentry_value_append(val, val) == 1);
    TEST_CHECK(sentry_value_set_by_index(val, 1, val) == 1);
    TEST_CHECK(sentry_value_remove_by_index(val, 1) == 1);
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_key(val, "foobar")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key_owned(val, "foobar")));
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_index(val, 1)));
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_index_owned(val, 1)));
    TEST_CHECK(sentry_value_get_length(val) == 0);
}

SENTRY_TEST(value_set_by_null_key)
{
    sentry_value_t value = sentry_value_new_object();
    sentry_value_t payload = sentry_value_new_object();

    TEST_CHECK(sentry_value_refcount(payload) == 1);
    TEST_CHECK_INT_EQUAL(1, sentry_value_set_by_key(value, NULL, payload));
    TEST_CHECK(sentry_value_get_length(value) == 0);

    payload = sentry_value_new_object();
    TEST_CHECK(sentry_value_refcount(payload) == 1);
    TEST_CHECK_INT_EQUAL(1, sentry_value_set_by_key_n(value, NULL, 0, payload));
    TEST_CHECK(sentry_value_get_length(value) == 0);

    payload = sentry_value_new_object();
    TEST_CHECK(sentry_value_refcount(payload) == 1);
    TEST_CHECK_INT_EQUAL(
        1, sentry_value_set_by_key_n(value, NULL, 10, payload));
    TEST_CHECK(sentry_value_get_length(value) == 0);

    sentry_value_decref(value);
}

SENTRY_TEST(value_remove_by_null_key)
{
    sentry_value_t value = sentry_value_new_object();

    TEST_CHECK_INT_EQUAL(0,
        sentry_value_set_by_key(value, "some_key", sentry_value_new_object()));
    TEST_CHECK(sentry_value_get_length(value) == 1);

    TEST_CHECK_INT_EQUAL(1, sentry_value_remove_by_key(value, NULL));
    TEST_CHECK_INT_EQUAL(1, sentry_value_get_length(value));
    TEST_CHECK_INT_EQUAL(1, sentry_value_remove_by_key_n(value, NULL, 0));
    TEST_CHECK_INT_EQUAL(1, sentry_value_get_length(value));
    TEST_CHECK_INT_EQUAL(1, sentry_value_remove_by_key_n(value, NULL, 10));
    TEST_CHECK_INT_EQUAL(1, sentry_value_get_length(value));

    sentry_value_decref(value);
}

SENTRY_TEST(value_get_by_null_key)
{
    sentry_value_t value = sentry_value_new_object();

    const char *some_key = "some_key";
    TEST_CHECK_INT_EQUAL(
        0, sentry_value_set_by_key(value, some_key, sentry_value_new_object()));
    TEST_CHECK(sentry_value_get_length(value) == 1);

    sentry_value_t rv = sentry_value_get_by_key(value, NULL);
    TEST_CHECK(sentry_value_is_null(rv));
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));

    rv = sentry_value_get_by_key_owned(value, NULL);
    TEST_CHECK(sentry_value_is_null(rv));
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));
    sentry_value_decref(rv);
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));

    rv = sentry_value_get_by_key_owned(value, some_key);
    TEST_CHECK(!sentry_value_is_null(rv));
    TEST_CHECK_INT_EQUAL(2, sentry_value_refcount(rv));
    sentry_value_decref(rv);
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));

    // if `k_len` != any length of keys stored in the object this won't
    // segfault because the `sentry_slice_t` equality check already fails due to
    // the length-inequality and never reaches `memcmp()`.
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_key_n(value, NULL, 0)));
    // If `k_len' == any key-length, we'd segfault without a NULL-check.
    TEST_CHECK(sentry_value_is_null(
        sentry_value_get_by_key_n(value, NULL, strlen(some_key))));

    rv = sentry_value_get_by_key_owned_n(value, NULL, strlen(some_key));
    TEST_CHECK(sentry_value_is_null(rv));
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));
    sentry_value_decref(rv);
    TEST_CHECK_INT_EQUAL(1, sentry_value_refcount(rv));

    sentry_value_decref(value);
}

SENTRY_TEST(value_set_stacktrace)
{
#if defined(SENTRY_PLATFORM_NX)
    SKIP_TEST();
#endif

    sentry_value_t exc
        = sentry_value_new_exception("std::out_of_range", "vector");
    sentry_value_set_stacktrace(exc, NULL, 0);

    sentry_value_t stacktrace = sentry_value_get_by_key(exc, "stacktrace");
    TEST_CHECK(!sentry_value_is_null(stacktrace));
    TEST_CHECK(SENTRY_VALUE_TYPE_OBJECT == sentry_value_get_type(stacktrace));

    sentry_value_t frames = sentry_value_get_by_key(stacktrace, "frames");
    TEST_CHECK(!sentry_value_is_null(frames));
    TEST_CHECK(SENTRY_VALUE_TYPE_LIST == sentry_value_get_type(frames));
    TEST_CHECK(0 < sentry_value_get_length(frames));

    sentry_value_decref(exc);
}

SENTRY_TEST(message_with_null_text_is_valid)
{
    sentry_value_t message_event = sentry_value_new_message_event(
        SENTRY_LEVEL_WARNING, "some-logger", NULL);

    TEST_CHECK(!sentry_value_is_null(message_event));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                message_event, "logger")),
        "some-logger");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(message_event, "level")),
        "warning");

    sentry_value_decref(message_event);
}

SENTRY_TEST(breadcrumb_without_type_or_message_still_valid)
{
    sentry_value_t breadcrumb = sentry_value_new_breadcrumb(NULL, NULL);
    TEST_CHECK(!sentry_value_is_null(breadcrumb));
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(breadcrumb, "timestamp")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(breadcrumb, "type")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(breadcrumb, "message")));
    sentry_value_decref(breadcrumb);

    char *const test_type = "navigation";
    breadcrumb = sentry_value_new_breadcrumb(test_type, NULL);
    TEST_CHECK(!sentry_value_is_null(breadcrumb));
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(breadcrumb, "timestamp")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(breadcrumb, "type")),
        test_type);
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(breadcrumb, "message")));
    sentry_value_decref(breadcrumb);

    char *const test_message = "a fork in the road, take it";
    breadcrumb = sentry_value_new_breadcrumb(NULL, test_message);
    TEST_CHECK(!sentry_value_is_null(breadcrumb));
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(breadcrumb, "timestamp")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(breadcrumb, "type")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(breadcrumb, "message")),
        test_message);
    sentry_value_decref(breadcrumb);
}

SENTRY_TEST(exception_without_type_or_value_still_valid)
{
    sentry_value_t exception = sentry_value_new_exception(NULL, NULL);
    TEST_CHECK(!sentry_value_is_null(exception));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(exception, "type")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(exception, "value")));
    sentry_value_decref(exception);

    char *const test_type = "EXC_BAD_ACCESS / KERN_INVALID_ADDRESS / 0x61";
    exception = sentry_value_new_exception(test_type, NULL);
    TEST_CHECK(!sentry_value_is_null(exception));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(exception, "type")),
        test_type);
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(exception, "value")));
    sentry_value_decref(exception);

    char *const test_value
        = "Fatal Error: EXC_BAD_ACCESS / KERN_INVALID_ADDRESS / 0x61";
    exception = sentry_value_new_exception(NULL, test_value);
    TEST_CHECK(!sentry_value_is_null(exception));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(exception, "type")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(exception, "value")),
        test_value);
    sentry_value_decref(exception);
}

SENTRY_TEST(thread_without_name_still_valid)
{
    sentry_value_t thread = sentry_value_new_thread(0xFF00FF00FF00FF00, NULL);
    TEST_CHECK(!sentry_value_is_null(thread));
    TEST_CHECK(!sentry_value_is_null(sentry_value_get_by_key(thread, "id")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(thread, "id")),
        "18374966859414961920");
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_key(thread, "name")));
    sentry_value_decref(thread);

    char *const test_name = "worker";
    thread = sentry_value_new_thread(0xAA00AA00AA00AA00, test_name);
    TEST_CHECK(!sentry_value_is_null(thread));
    TEST_CHECK(!sentry_value_is_null(sentry_value_get_by_key(thread, "id")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(thread, "id")),
        "12249977906276641280");
    TEST_CHECK(!sentry_value_is_null(sentry_value_get_by_key(thread, "name")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(thread, "name")),
        test_name);
    sentry_value_decref(thread);
}

SENTRY_TEST(user_report_is_valid)
{
    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_value_t user_feedback;
    SENTRY_TEST_DEPRECATED(
        user_feedback = sentry_value_new_user_feedback(
            &event_id, "some-name", "some-email", "some-comment"));

    TEST_CHECK(!sentry_value_is_null(user_feedback));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "name")),
        "some-name");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "email")),
        "some-email");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "comments")),
        "some-comment");

    sentry_value_decref(user_feedback);
}

SENTRY_TEST(user_feedback_with_null_args)
{
    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_value_t user_feedback;
    SENTRY_TEST_DEPRECATED(user_feedback
        = sentry_value_new_user_feedback(&event_id, NULL, NULL, NULL));

    TEST_CHECK(!sentry_value_is_null(user_feedback));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_feedback, "name")));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_feedback, "email")));
    TEST_CHECK(sentry_value_is_null(
        sentry_value_get_by_key(user_feedback, "comments")));

    sentry_value_decref(user_feedback);

    SENTRY_TEST_DEPRECATED(user_feedback = sentry_value_new_user_feedback(
                               &event_id, NULL, "some-email", "some-comment"));

    TEST_CHECK(!sentry_value_is_null(user_feedback));
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_feedback, "name")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "email")),
        "some-email");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "comments")),
        "some-comment");

    sentry_value_decref(user_feedback);

    SENTRY_TEST_DEPRECATED(user_feedback = sentry_value_new_user_feedback(
                               &event_id, "some-name", NULL, "some-comment"));

    TEST_CHECK(!sentry_value_is_null(user_feedback));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "name")),
        "some-name");
    TEST_CHECK(
        sentry_value_is_null(sentry_value_get_by_key(user_feedback, "email")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "comments")),
        "some-comment");

    sentry_value_decref(user_feedback);

    SENTRY_TEST_DEPRECATED(user_feedback = sentry_value_new_user_feedback(
                               &event_id, "some-name", "some-email", NULL));

    TEST_CHECK(!sentry_value_is_null(user_feedback));

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "name")),
        "some-name");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "email")),
        "some-email");
    TEST_CHECK(sentry_value_is_null(
        sentry_value_get_by_key(user_feedback, "comments")));

    sentry_value_decref(user_feedback);
}

SENTRY_TEST(user_feedback_is_valid)
{
    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_value_t user_feedback = sentry_value_new_feedback(
        "some-message", "some-email", "some-name", &event_id);

    TEST_CHECK(!sentry_value_is_null(user_feedback));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user_feedback, "name")),
        "some-name");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "contact_email")),
        "some-email");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "message")),
        "some-message");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                user_feedback, "associated_event_id")),
        "c993afb6b4ac48a6b61b2558e601d65d");

    sentry_value_decref(user_feedback);
}

SENTRY_TEST(event_with_id)
{
    sentry_uuid_t event_id
        = sentry_uuid_from_string("ad59c6f8-eb88-4dca-b330-94dee9a46fe8");

    sentry_value_t event = sentry__value_new_event_with_id(&event_id);

    TEST_CHECK(!sentry_value_is_null(event));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(event, "event_id")),
        "ad59c6f8-eb88-4dca-b330-94dee9a46fe8");
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(event, "timestamp")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(event, "platform")),
        "native");

    sentry_value_decref(event);
}
