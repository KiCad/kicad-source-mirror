#include "sentry_envelope.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_json.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_ratelimiter.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include "sentry_value.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

struct sentry_envelope_item_s {
    sentry_value_t headers;
    sentry_value_t event;
    char *payload;
    size_t payload_len;
    sentry_envelope_item_t *next;
};

struct sentry_envelope_s {
    bool is_raw;
    union {
        struct {
            sentry_value_t headers;
            sentry_envelope_item_t *first_item;
            sentry_envelope_item_t *last_item;
            size_t item_count;
        } items;
        struct {
            char *payload;
            size_t payload_len;
        } raw;
    } contents;
};

static sentry_envelope_item_t *
envelope_add_item(sentry_envelope_t *envelope)
{
    if (envelope->is_raw) {
        return NULL;
    }

    // TODO: Envelopes may have at most one event item or one transaction item,
    // and not one of both. Some checking should be done here or in
    // `sentry__envelope_add_[transaction|event]` to ensure this can't happen.

    // Allocate new item
    sentry_envelope_item_t *item = SENTRY_MAKE(sentry_envelope_item_t);
    if (!item) {
        return NULL;
    }

    // Initialize item
    item->headers = sentry_value_new_object();
    item->event = sentry_value_new_null();
    item->payload = NULL;
    item->payload_len = 0;
    item->next = NULL;

    // Append to linked list
    if (envelope->contents.items.last_item) {
        envelope->contents.items.last_item->next = item;
    } else {
        envelope->contents.items.first_item = item;
    }
    envelope->contents.items.last_item = item;
    envelope->contents.items.item_count++;

    return item;
}

static void
envelope_item_cleanup(sentry_envelope_item_t *item)
{
    sentry_value_decref(item->headers);
    sentry_value_decref(item->event);
    sentry_free(item->payload);
}

sentry_value_t
sentry_envelope_get_header(const sentry_envelope_t *envelope, const char *key)
{
    return sentry_envelope_get_header_n(
        envelope, key, sentry__guarded_strlen(key));
}

sentry_value_t
sentry_envelope_get_header_n(
    const sentry_envelope_t *envelope, const char *key, size_t key_len)
{
    if (!envelope || envelope->is_raw) {
        return sentry_value_new_null();
    }
    return sentry_value_get_by_key_n(
        envelope->contents.items.headers, key, key_len);
}

void
sentry__envelope_item_set_header(
    sentry_envelope_item_t *item, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(item->headers, key, value);
}

static int
envelope_item_get_ratelimiter_category(const sentry_envelope_item_t *item)
{
    const char *ty = sentry_value_as_string(
        sentry_value_get_by_key(item->headers, "type"));
    if (sentry__string_eq(ty, "session")) {
        return SENTRY_RL_CATEGORY_SESSION;
    } else if (sentry__string_eq(ty, "transaction")) {
        return SENTRY_RL_CATEGORY_TRANSACTION;
    }
    // NOTE: the `type` here can be `event` or `attachment`.
    // Ideally, attachments should have their own RL_CATEGORY.
    return SENTRY_RL_CATEGORY_ERROR;
}

static sentry_envelope_item_t *
envelope_add_from_owned_buffer(
    sentry_envelope_t *envelope, char *buf, size_t buf_len, const char *type)
{
    if (!buf) {
        return NULL;
    }
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        sentry_free(buf);
        return NULL;
    }

    item->payload = buf;
    item->payload_len = buf_len;
    sentry_value_t length = sentry_value_new_int32((int32_t)buf_len);
    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string(type));
    sentry__envelope_item_set_header(item, "length", length);

    return item;
}

void
sentry_envelope_free(sentry_envelope_t *envelope)
{
    if (!envelope) {
        return;
    }
    if (envelope->is_raw) {
        sentry_free(envelope->contents.raw.payload);
        sentry_free(envelope);
        return;
    }
    sentry_value_decref(envelope->contents.items.headers);

    // Free all items in the linked list
    sentry_envelope_item_t *item = envelope->contents.items.first_item;
    while (item) {
        sentry_envelope_item_t *next = item->next;
        envelope_item_cleanup(item);
        sentry_free(item);
        item = next;
    }

    sentry_free(envelope);
}

static void
sentry__envelope_set_header(
    sentry_envelope_t *envelope, const char *key, sentry_value_t value)
{
    if (envelope->is_raw) {
        return;
    }
    sentry_value_set_by_key(envelope->contents.items.headers, key, value);
}

sentry_envelope_t *
sentry__envelope_new(void)
{
    sentry_envelope_t *rv = SENTRY_MAKE(sentry_envelope_t);
    if (!rv) {
        return NULL;
    }

    rv->is_raw = false;
    rv->contents.items.first_item = NULL;
    rv->contents.items.last_item = NULL;
    rv->contents.items.item_count = 0;
    rv->contents.items.headers = sentry_value_new_object();

    SENTRY_WITH_OPTIONS (options) {
        if (options->dsn && options->dsn->is_valid) {
            sentry__envelope_set_header(rv, "dsn",
                sentry_value_new_string(sentry_options_get_dsn(options)));
        }
    }

    return rv;
}

sentry_envelope_t *
sentry__envelope_from_path(const sentry_path_t *path)
{
    size_t buf_len;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    if (!buf) {
        SENTRY_WARNF("failed to read raw envelope from \"%s\"", path->path);
        return NULL;
    }

    sentry_envelope_t *envelope = SENTRY_MAKE(sentry_envelope_t);
    if (!envelope) {
        sentry_free(buf);
        return NULL;
    }

    envelope->is_raw = true;
    envelope->contents.raw.payload = buf;
    envelope->contents.raw.payload_len = buf_len;

    return envelope;
}

sentry_uuid_t
sentry__envelope_get_event_id(const sentry_envelope_t *envelope)
{
    if (envelope->is_raw) {
        return sentry_uuid_nil();
    }
    return sentry_uuid_from_string(sentry_value_as_string(
        sentry_value_get_by_key(envelope->contents.items.headers, "event_id")));
}

void
sentry__envelope_set_event_id(
    sentry_envelope_t *envelope, const sentry_uuid_t *event_id)
{
    sentry_value_t value = sentry__value_new_uuid(event_id);
    sentry__envelope_set_header(envelope, "event_id", value);
}

sentry_value_t
sentry_envelope_get_event(const sentry_envelope_t *envelope)
{
    if (envelope->is_raw) {
        return sentry_value_new_null();
    }

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (!sentry_value_is_null(item->event)
            && !sentry__event_is_transaction(item->event)) {
            return item->event;
        }
    }
    return sentry_value_new_null();
}

sentry_value_t
sentry_envelope_get_transaction(const sentry_envelope_t *envelope)
{
    if (envelope->is_raw) {
        return sentry_value_new_null();
    }

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (!sentry_value_is_null(item->event)
            && sentry__event_is_transaction(item->event)) {
            return item->event;
        }
    }
    return sentry_value_new_null();
}

sentry_envelope_item_t *
sentry__envelope_add_event(sentry_envelope_t *envelope, sentry_value_t event)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(event, &event_id);

    item->event = event;
    sentry__jsonwriter_write_value(jw, event);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("event"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    double traces_sample_rate = 0.0;
    SENTRY_WITH_OPTIONS (options) {
        traces_sample_rate = options->traces_sample_rate;
    }
    sentry_value_t dsc = sentry_value_new_null();
    sentry_value_t sample_rand = sentry_value_new_null();
    SENTRY_WITH_SCOPE (scope) {
        dsc = sentry__value_clone(scope->dynamic_sampling_context);
        sample_rand = sentry_value_get_by_key(
            sentry_value_get_by_key(scope->propagation_context, "trace"),
            "sample_rand");
    }
    if (!sentry_value_is_null(dsc)) {
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(
                sentry_value_get_by_key(event, "contexts"), "trace"),
            "trace_id");
        if (!sentry_value_is_null(trace_id)) {
            sentry_value_incref(trace_id);
            sentry_value_set_by_key(dsc, "trace_id", trace_id);
        } else {
            SENTRY_WARN("couldn't retrieve trace_id from scope to apply to the "
                        "dynamic sampling context");
        }
        if (!sentry_value_is_null(sample_rand)) {
            if (sentry_value_as_double(sample_rand) >= traces_sample_rate) {
                sentry_value_set_by_key(
                    dsc, "sampled", sentry_value_new_string("false"));
            } else {
                sentry_value_set_by_key(
                    dsc, "sampled", sentry_value_new_string("true"));
            }
        } else {
            // only for testing; in production, the SDK should always have a
            // non-null sample_rand. We don't set "sampled" to keep dsc empty
            SENTRY_WARN("couldn't retrieve sample_rand from scope to apply to "
                        "the dynamic sampling context");
        }
        // only add dsc if it has values
        if (sentry_value_is_true(dsc)) {
#ifdef SENTRY_UNITTEST
            // to make comparing the header feasible in unit tests
            sentry_value_set_by_key(dsc, "sample_rand",
                sentry_value_new_double(0.01006918276309107));
#endif
            sentry__envelope_set_header(envelope, "trace", dsc);
        } else {
            sentry_value_decref(dsc);
        }
    }

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_transaction(
    sentry_envelope_t *envelope, sentry_value_t transaction)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(transaction, &event_id);

    item->event = transaction;
    sentry__jsonwriter_write_value(jw, transaction);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("transaction"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    sentry_value_t dsc = sentry_value_new_null();

    SENTRY_WITH_SCOPE (scope) {
        dsc = sentry__value_clone(scope->dynamic_sampling_context);
    }

    if (!sentry_value_is_null(dsc)) {
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(
                sentry_value_get_by_key(transaction, "contexts"), "trace"),
            "trace_id");
        if (!sentry_value_is_null(trace_id)) {
            sentry_value_incref(trace_id);
            sentry_value_set_by_key(dsc, "trace_id", trace_id);
            sentry_value_set_by_key(
                dsc, "sampled", sentry_value_new_string("true"));
        } else {
            SENTRY_WARN("couldn't retrieve trace_id in transaction's trace "
                        "context to apply to the dynamic sampling context");
        }
        sentry_value_t transaction_name
            = sentry_value_get_by_key(transaction, "transaction");
        if (!sentry_value_is_null(transaction_name)) {
            sentry_value_incref(transaction_name);
            sentry_value_set_by_key(dsc, "transaction", transaction_name);
        }
        // only add dsc if it has values
        if (sentry_value_is_true(dsc)) {
            sentry__envelope_set_header(envelope, "trace", dsc);
        } else {
            sentry_value_decref(dsc);
        }
    }

#ifdef SENTRY_UNITTEST
    sentry_value_t now = sentry_value_new_string("2021-12-16T05:53:59.343Z");
#else
    sentry_value_t now = sentry__value_new_string_owned(
        sentry__usec_time_to_iso8601(sentry__usec_time()));
#endif
    sentry__envelope_set_header(envelope, "sent_at", now);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_logs(sentry_envelope_t *envelope, sentry_value_t logs)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry__jsonwriter_write_value(jw, logs);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("log"));
    sentry__envelope_item_set_header(item, "item_count",
        sentry_value_new_int32((int32_t)sentry_value_get_length(
            sentry_value_get_by_key(logs, "items"))));
    sentry__envelope_item_set_header(item, "content_type",
        sentry_value_new_string("application/vnd.sentry.items.log+json"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_user_report(
    sentry_envelope_t *envelope, sentry_value_t user_report)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(user_report, &event_id);

    sentry__jsonwriter_write_value(jw, user_report);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("user_report"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_user_feedback(
    sentry_envelope_t *envelope, sentry_value_t user_feedback)
{
    sentry_value_t event = sentry_value_new_event();
    sentry_value_t contexts = sentry_value_get_by_key(event, "contexts");
    if (sentry_value_is_null(contexts)) {
        contexts = sentry_value_new_object();
    }
    sentry_value_set_by_key(contexts, "feedback", user_feedback);
    sentry_value_set_by_key(event, "contexts", contexts);

    sentry_envelope_item_t *item = sentry__envelope_add_event(envelope, event);
    if (!item) {
        sentry_value_decref(event);
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("feedback"));

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_session(
    sentry_envelope_t *envelope, const sentry_session_t *session)
{
    if (!envelope || !session) {
        return NULL;
    }
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }
    sentry__session_to_json(session, jw);
    size_t payload_len = 0;
    char *payload = sentry__jsonwriter_into_string(jw, &payload_len);

    // NOTE: function will check for `payload` internally and free it on error
    return envelope_add_from_owned_buffer(
        envelope, payload, payload_len, "session");
}

static const char *
str_from_attachment_type(sentry_attachment_type_t attachment_type)
{
    switch (attachment_type) {
    case ATTACHMENT:
        return "event.attachment";
    case MINIDUMP:
        return "event.minidump";
    case VIEW_HIERARCHY:
        return "event.view_hierarchy";
    default:
        UNREACHABLE("Unknown attachment type");
        return "event.attachment";
    }
}

sentry_envelope_item_t *
sentry__envelope_add_attachment(
    sentry_envelope_t *envelope, const sentry_attachment_t *attachment)
{
    if (!envelope || !attachment) {
        return NULL;
    }

    sentry_envelope_item_t *item = NULL;
    if (attachment->buf) {
        item = sentry__envelope_add_from_buffer(
            envelope, attachment->buf, attachment->buf_len, "attachment");
    } else {
        item = sentry__envelope_add_from_path(
            envelope, attachment->path, "attachment");
    }
    if (!item) {
        return NULL;
    }
    if (attachment->type != ATTACHMENT) { // don't need to set the default
        sentry__envelope_item_set_header(item, "attachment_type",
            sentry_value_new_string(
                str_from_attachment_type(attachment->type)));
    }
    if (attachment->content_type) {
        sentry__envelope_item_set_header(item, "content_type",
            sentry_value_new_string(attachment->content_type));
    }
    sentry__envelope_item_set_header(item, "filename",
        sentry_value_new_string(sentry__path_filename(
            attachment->filename ? attachment->filename : attachment->path)));

    return item;
}

void
sentry__envelope_add_attachments(
    sentry_envelope_t *envelope, const sentry_attachment_t *attachments)
{
    if (!envelope || !attachments) {
        return;
    }

    SENTRY_DEBUG("adding attachments to envelope");
    for (const sentry_attachment_t *attachment = attachments; attachment;
        attachment = attachment->next) {
        sentry__envelope_add_attachment(envelope, attachment);
    }
}

sentry_envelope_item_t *
sentry__envelope_add_from_buffer(sentry_envelope_t *envelope, const char *buf,
    size_t buf_len, const char *type)
{
    // NOTE: function will check for the clone of `buf` internally and free it
    // on error
    return envelope_add_from_owned_buffer(
        envelope, sentry__string_clone_n(buf, buf_len), buf_len, type);
}

sentry_envelope_item_t *
sentry__envelope_add_from_path(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type)
{
    if (!envelope) {
        return NULL;
    }
    size_t buf_len;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    if (!buf) {
        SENTRY_WARNF("failed to read envelope item from \"%s\"", path->path);
        return NULL;
    }
    // NOTE: function will free `buf` on error
    return envelope_add_from_owned_buffer(envelope, buf, buf_len, type);
}

static void
sentry__envelope_serialize_headers_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(sb);
    if (jw) {
        sentry__jsonwriter_write_value(jw, envelope->contents.items.headers);
        sentry__jsonwriter_free(jw);
    }
}

static void
sentry__envelope_serialize_item_into_stringbuilder(
    const sentry_envelope_item_t *item, sentry_stringbuilder_t *sb)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(sb);
    if (!jw) {
        return;
    }
    sentry__stringbuilder_append_char(sb, '\n');

    sentry__jsonwriter_write_value(jw, item->headers);
    sentry__jsonwriter_free(jw);

    sentry__stringbuilder_append_char(sb, '\n');

    sentry__stringbuilder_append_buf(sb, item->payload, item->payload_len);
}

void
sentry__envelope_serialize_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb)
{
    if (envelope->is_raw) {
        sentry__stringbuilder_append_buf(sb, envelope->contents.raw.payload,
            envelope->contents.raw.payload_len);
        return;
    }

    SENTRY_DEBUG("serializing envelope into buffer");
    sentry__envelope_serialize_headers_into_stringbuilder(envelope, sb);

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        sentry__envelope_serialize_item_into_stringbuilder(item, sb);
    }
}

char *
sentry_envelope_serialize_ratelimited(const sentry_envelope_t *envelope,
    const sentry_rate_limiter_t *rl, size_t *size_out, bool *owned_out)
{
    if (envelope->is_raw) {
        *size_out = envelope->contents.raw.payload_len;
        *owned_out = false;
        return envelope->contents.raw.payload;
    }
    *owned_out = true;

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__envelope_serialize_headers_into_stringbuilder(envelope, &sb);

    size_t serialized_items = 0;
    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (rl) {
            int category = envelope_item_get_ratelimiter_category(item);
            if (sentry__rate_limiter_is_disabled(rl, category)) {
                continue;
            }
        }
        sentry__envelope_serialize_item_into_stringbuilder(item, &sb);
        serialized_items += 1;
    }

    if (!serialized_items) {
        sentry__stringbuilder_cleanup(&sb);
        *size_out = 0;
        return NULL;
    }

    *size_out = sentry__stringbuilder_len(&sb);
    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry_envelope_serialize(const sentry_envelope_t *envelope, size_t *size_out)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);

    if (size_out) {
        *size_out = sentry__stringbuilder_len(&sb);
    }
    return sentry__stringbuilder_into_string(&sb);
}

MUST_USE int
sentry_envelope_write_to_path(
    const sentry_envelope_t *envelope, const sentry_path_t *path)
{
    sentry_filewriter_t *fw = sentry__filewriter_new(path);
    if (!fw) {
        return 1;
    }

    if (envelope->is_raw) {
        size_t rv = sentry__filewriter_write(fw, envelope->contents.raw.payload,
            envelope->contents.raw.payload_len);
        sentry__filewriter_free(fw);
        return rv != 0;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_fw(fw);
    if (jw) {
        sentry__jsonwriter_write_value(jw, envelope->contents.items.headers);
        sentry__jsonwriter_reset(jw);

        for (const sentry_envelope_item_t *item
            = envelope->contents.items.first_item;
            item; item = item->next) {
            const char newline = '\n';
            sentry__filewriter_write(fw, &newline, sizeof(char));

            sentry__jsonwriter_write_value(jw, item->headers);
            sentry__jsonwriter_reset(jw);

            sentry__filewriter_write(fw, &newline, sizeof(char));

            sentry__filewriter_write(fw, item->payload, item->payload_len);
        }
        sentry__jsonwriter_free(jw);
    }

    size_t rv = sentry__filewriter_byte_count(fw);
    sentry__filewriter_free(fw);

    return rv == 0;
}

int
sentry_envelope_write_to_file_n(
    const sentry_envelope_t *envelope, const char *path, size_t path_len)
{
    if (!envelope || !path) {
        return 1;
    }
    sentry_path_t *path_obj = sentry__path_from_str_n(path, path_len);

    int rv = sentry_envelope_write_to_path(envelope, path_obj);

    sentry__path_free(path_obj);

    return rv;
}

int
sentry_envelope_write_to_file(
    const sentry_envelope_t *envelope, const char *path)
{
    if (!envelope || !path) {
        return 1;
    }

    return sentry_envelope_write_to_file_n(envelope, path, strlen(path));
}

// https://develop.sentry.dev/sdk/data-model/envelopes/
sentry_envelope_t *
sentry_envelope_deserialize(const char *buf, size_t buf_len)
{
    if (!buf || buf_len == 0) {
        return NULL;
    }

    sentry_envelope_t *envelope = sentry__envelope_new();
    if (!envelope) {
        goto fail;
    }

    const char *ptr = buf;
    const char *end = buf + buf_len;

    // headers
    const char *headers_end = memchr(ptr, '\n', (size_t)(end - ptr));
    if (!headers_end) {
        headers_end = end;
    }
    size_t headers_len = (size_t)(headers_end - ptr);
    sentry_value_decref(envelope->contents.items.headers);
    envelope->contents.items.headers
        = sentry__value_from_json(ptr, headers_len);
    if (sentry_value_get_type(envelope->contents.items.headers)
        != SENTRY_VALUE_TYPE_OBJECT) {
        goto fail;
    }

    ptr = headers_end;
    if (ptr < end) {
        ptr++; // skip newline
    }

    // items
    while (ptr < end) {
        sentry_envelope_item_t *item = envelope_add_item(envelope);
        if (!item) {
            goto fail;
        }

        // item headers
        const char *item_headers_end = memchr(ptr, '\n', (size_t)(end - ptr));
        if (!item_headers_end) {
            item_headers_end = end;
        }
        size_t item_headers_len = (size_t)(item_headers_end - ptr);
        sentry_value_decref(item->headers);
        item->headers = sentry__value_from_json(ptr, item_headers_len);
        if (sentry_value_get_type(item->headers) != SENTRY_VALUE_TYPE_OBJECT) {
            goto fail;
        }
        ptr = item_headers_end + 1; // skip newline

        if (ptr > end) {
            goto fail;
        }

        // item payload
        sentry_value_t length
            = sentry_value_get_by_key(item->headers, "length");
        if (sentry_value_is_null(length)) {
            // length omitted -> find newline or end of buffer
            const char *payload_end = memchr(ptr, '\n', (size_t)(end - ptr));
            if (!payload_end) {
                payload_end = end;
            }
            item->payload_len = (size_t)(payload_end - ptr);
        } else if (sentry_value_get_type(length) == SENTRY_VALUE_TYPE_UINT64) {
            uint64_t payload_len = sentry_value_as_uint64(length);
            if (payload_len >= SIZE_MAX) {
                goto fail;
            }
            item->payload_len = (size_t)payload_len;
        } else {
            int64_t payload_len = sentry_value_as_int64(length);
            if (payload_len < 0 || (uint64_t)payload_len >= SIZE_MAX) {
                goto fail;
            }
            item->payload_len = (size_t)payload_len;
        }
        if (item->payload_len > 0) {
            if (ptr + item->payload_len > end
                || item->payload_len >= SIZE_MAX) {
                goto fail;
            }
            item->payload = sentry_malloc(item->payload_len + 1);
            if (!item->payload) {
                goto fail;
            }
            memcpy(item->payload, ptr, item->payload_len);
            item->payload[item->payload_len] = '\0';

            // item event/transaction
            const char *type = sentry_value_as_string(
                sentry_value_get_by_key(item->headers, "type"));
            if (type
                && (sentry__string_eq(type, "event")
                    || sentry__string_eq(type, "transaction"))) {
                item->event
                    = sentry__value_from_json(item->payload, item->payload_len);
            }

            ptr += item->payload_len;
        }

        while (ptr < end && *ptr == '\n') {
            ptr++;
        }
    }

    return envelope;

fail:
    sentry_envelope_free(envelope);
    return NULL;
}

static sentry_envelope_t *
parse_envelope_from_file(sentry_path_t *path)
{
    if (!path) {
        return NULL;
    }

    size_t buf_len = 0;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    sentry_free(buf);
    sentry__path_free(path);
    return envelope;
}

sentry_envelope_t *
sentry_envelope_read_from_file(const char *path)
{
    if (!path) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_str(path));
}

sentry_envelope_t *
sentry_envelope_read_from_file_n(const char *path, size_t path_len)
{
    if (!path || path_len == 0) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_str_n(path, path_len));
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_envelope_t *
sentry_envelope_read_from_filew(const wchar_t *path)
{
    if (!path) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_wstr(path));
}

sentry_envelope_t *
sentry_envelope_read_from_filew_n(const wchar_t *path, size_t path_len)
{
    if (!path || path_len == 0) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_wstr_n(path, path_len));
}
#endif

#ifdef SENTRY_UNITTEST
size_t
sentry__envelope_get_item_count(const sentry_envelope_t *envelope)
{
    return envelope->is_raw ? 0 : envelope->contents.items.item_count;
}

const sentry_envelope_item_t *
sentry__envelope_get_item(const sentry_envelope_t *envelope, size_t idx)
{
    if (envelope->is_raw) {
        return NULL;
    }

    // Traverse linked list to find item at index
    size_t current_idx = 0;
    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (current_idx == idx) {
            return item;
        }
        current_idx++;
    }

    return NULL;
}

sentry_value_t
sentry__envelope_item_get_header(
    const sentry_envelope_item_t *item, const char *key)
{
    return sentry_value_get_by_key(item->headers, key);
}

const char *
sentry__envelope_item_get_payload(
    const sentry_envelope_item_t *item, size_t *payload_len_out)
{
    if (payload_len_out) {
        *payload_len_out = item->payload_len;
    }
    return item->payload;
}
#endif
