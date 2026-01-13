#include "sentry_logs.h"
#include "sentry_core.h"
#include "sentry_cpu_relax.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_os.h"
#include "sentry_scope.h"
#include "sentry_sync.h"
#include <stdarg.h>
#include <string.h>

#ifdef SENTRY_UNITTEST
#    define QUEUE_LENGTH 5
#else
#    define QUEUE_LENGTH 100
#endif
#define FLUSH_TIMER 5

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#else
#    include <unistd.h>
#    define sleep_ms(MILLISECONDS) usleep(MILLISECONDS * 1000)
#endif
/**
 * Thread lifecycle states for the logs batching thread.
 */
typedef enum {
    /** Thread is not running (initial state or after shutdown) */
    SENTRY_LOGS_THREAD_STOPPED = 0,
    /** Thread is starting up but not yet ready */
    SENTRY_LOGS_THREAD_STARTING = 1,
    /** Thread is running and processing logs */
    SENTRY_LOGS_THREAD_RUNNING = 2,
} sentry_logs_thread_state_t;

typedef struct {
    sentry_value_t logs[QUEUE_LENGTH];
    long index; // (atomic) index for producer threads to get a unique slot
    long adding; // (atomic) count of in-flight writers on this buffer
    long sealed; // (atomic) 0=writeable, 1=sealed (meaning we drop)
} log_buffer_t;

static struct {
    log_buffer_t buffers[2]; // double buffer
    long active_idx; // (atomic) index to the active buffer
    long flushing; // (atomic) reentrancy guard to the flusher
    long thread_state; // (atomic) sentry_logs_thread_state_t
    sentry_cond_t request_flush; // condition variable to schedule a flush
    sentry_threadid_t batching_thread; // the batching thread
} g_logs_state = {
    {
        {
            .index = 0,
            .adding = 0,
            .sealed = 0,
        },
        {
            .index = 0,
            .adding = 0,
            .sealed = 0,
        },
    },
    .active_idx = 0,
    .flushing = 0,
    .thread_state = SENTRY_LOGS_THREAD_STOPPED,
};

// checks whether the currently active buffer should be flushed.
// otherwise we could miss the trigger of adding the last log if we're actively
// flushing the other buffer already.
// we can safely check the state of the active buffer, as the only thread that
// can change which buffer is active is the one calling this check function
// inside flush_logs_queue() below
static bool
check_for_flush_condition(void)
{
    // In flush_logs_queue, after finishing a flush:
    long current_active = sentry__atomic_fetch(&g_logs_state.active_idx);
    log_buffer_t *current_buf = &g_logs_state.buffers[current_active];

    // Check if current active buffer is also full
    // We could even lower the threshold for high-contention scenarios
    return sentry__atomic_fetch(&current_buf->index) >= QUEUE_LENGTH;
}

// Use a sleep spinner around a monotonic timer so we don't syscall sleep from
// a signal handler. While this is strictly needed only there, there is no
// reason not to use the same implementation across platforms.
static void
crash_safe_sleep_ms(uint64_t delay_ms)
{
    const uint64_t start = sentry__monotonic_time();
    const uint64_t end = start + delay_ms;
    while (sentry__monotonic_time() < end) {
        for (int i = 0; i < 64; i++) {
            sentry__cpu_relax();
        }
    }
}

static void
flush_logs_queue(bool crash_safe)
{
    if (crash_safe) {
        // In crash-safe mode, spin lock with timeout and backoff
        int attempts = 0;
        while (!sentry__atomic_compare_swap(&g_logs_state.flushing, 0, 1)) {
            const int max_attempts = 200;
            if (++attempts > max_attempts) {
                SENTRY_WARN("flush_logs_queue: timeout waiting for flushing "
                            "lock in crash-safe mode");
                return;
            }

            // backoff max-wait with max_attempts = 200 based sleep slots:
            // 9ms + 450ms + 1010ms = 1500ish ms
            const uint32_t sleep_time = (attempts < 10) ? 1
                : (attempts < 100)                      ? 5
                                                        : 10;
            crash_safe_sleep_ms(sleep_time);
        }
    } else {
        // Normal mode: try once and return if already flushing
        const long already_flushing
            = sentry__atomic_store(&g_logs_state.flushing, 1);
        if (already_flushing) {
            return;
        }
    }
    do {
        // prep both buffers
        long old_buf_idx = sentry__atomic_fetch(&g_logs_state.active_idx);
        long new_buf_idx = 1 - old_buf_idx;
        log_buffer_t *old_buf = &g_logs_state.buffers[old_buf_idx];
        log_buffer_t *new_buf = &g_logs_state.buffers[new_buf_idx];

        // reset new buffer...
        sentry__atomic_store(&new_buf->index, 0);
        sentry__atomic_store(&new_buf->adding, 0);
        sentry__atomic_store(&new_buf->sealed, 0);

        // ...and make it active (after this we're good to go producer side)
        sentry__atomic_store(&g_logs_state.active_idx, new_buf_idx);

        // seal old buffer
        sentry__atomic_store(&old_buf->sealed, 1);

        // Wait for all in-flight producers of the old buffer
        while (sentry__atomic_fetch(&old_buf->adding) > 0) {
            sentry__cpu_relax();
        }

        long n = sentry__atomic_store(&old_buf->index, 0);
        if (n > QUEUE_LENGTH) {
            n = QUEUE_LENGTH;
        }

        if (n > 0) {
            // now we can do the actual batching of the old buffer

            sentry_value_t logs = sentry_value_new_object();
            sentry_value_t log_items = sentry_value_new_list();
            int i;
            for (i = 0; i < n; i++) {
                sentry_value_append(log_items, old_buf->logs[i]);
            }
            sentry_value_set_by_key(logs, "items", log_items);

            sentry_envelope_t *envelope = sentry__envelope_new();
            sentry__envelope_add_logs(envelope, logs);

            SENTRY_WITH_OPTIONS (options) {
                if (crash_safe) {
                    // Write directly to disk to avoid transport queuing during
                    // crash
                    sentry__run_write_envelope(options->run, envelope);
                    sentry_envelope_free(envelope);
                } else {
                    // Normal operation: use transport for HTTP transmission
                    sentry__capture_envelope(options->transport, envelope);
                }
            }
            sentry_value_decref(logs);
        }
    } while (check_for_flush_condition());

    sentry__atomic_store(&g_logs_state.flushing, 0);
}

#define ENQUEUE_MAX_RETRIES 2

static bool
enqueue_log(sentry_value_t log)
{
    for (int attempt = 0; attempt <= ENQUEUE_MAX_RETRIES; attempt++) {
        // retrieve the active buffer
        const long active_idx = sentry__atomic_fetch(&g_logs_state.active_idx);
        log_buffer_t *active = &g_logs_state.buffers[active_idx];

        // if the buffer is already sealed we retry or drop and exit early.
        if (sentry__atomic_fetch(&active->sealed) != 0) {
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }

        // `adding` is our boundary for this buffer since it keeps the flusher
        // blocked. We have to recheck that the flusher hasn't already switched
        // the active buffer or sealed the one this thread is on. If either is
        // true we have to unblock the flusher and retry or drop the log.
        sentry__atomic_fetch_and_add(&active->adding, 1);
        const long active_idx_check
            = sentry__atomic_fetch(&g_logs_state.active_idx);
        const long sealed_check = sentry__atomic_fetch(&active->sealed);
        if (active_idx != active_idx_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }
        if (sealed_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }

        // Now we can finally request a slot and check if the log fits in this
        // buffer.
        const long log_idx = sentry__atomic_fetch_and_add(&active->index, 1);
        if (log_idx < QUEUE_LENGTH) {
            // got a slot, write log to the buffer and unblock flusher
            active->logs[log_idx] = log;
            sentry__atomic_fetch_and_add(&active->adding, -1);

            // Check if active buffer is now full and trigger flush. We could
            // introduce additional watermarks here to trigger the flush earlier
            // under high contention.
            // TODO replace with a level-triggered flag
            if (log_idx == QUEUE_LENGTH - 1) {
                sentry__cond_wake(&g_logs_state.request_flush);
            }
            return true;
        }
        // ping the batching thread to flush, since we could miss a cond_wake
        // on adding the last item
        sentry__cond_wake(&g_logs_state.request_flush);
        // Buffer is already full, roll back our increments and retry or drop.
        sentry__atomic_fetch_and_add(&active->adding, -1);
        if (attempt == ENQUEUE_MAX_RETRIES) {
            // TODO report this (e.g. client reports)
            return false;
        }
    }
    return false;
}

SENTRY_THREAD_FN
batching_thread_func(void *data)
{
    (void)data;
    SENTRY_DEBUG("Starting batching thread");
    sentry_mutex_t task_lock;
    sentry__mutex_init(&task_lock);
    sentry__mutex_lock(&task_lock);

    // Transition from STARTING to RUNNING using compare-and-swap
    // CAS ensures atomic state verification: only succeeds if state is STARTING
    // If CAS fails, shutdown already set state to STOPPED, so exit immediately
    // Uses sequential consistency to ensure all thread initialization is
    // visible
    if (!sentry__atomic_compare_swap(&g_logs_state.thread_state,
            (long)SENTRY_LOGS_THREAD_STARTING,
            (long)SENTRY_LOGS_THREAD_RUNNING)) {
        SENTRY_DEBUG("logs thread detected shutdown during startup, exiting");
        sentry__mutex_unlock(&task_lock);
        sentry__mutex_free(&task_lock);
        return 0;
    }

    // Main loop: run while state is RUNNING
    while (sentry__atomic_fetch(&g_logs_state.thread_state)
        == SENTRY_LOGS_THREAD_RUNNING) {
        // Sleep for 5 seconds or until request_flush hits
        const int triggered_by = sentry__cond_wait_timeout(
            &g_logs_state.request_flush, &task_lock, 5000);

        // Check if we should still be running
        if (sentry__atomic_fetch(&g_logs_state.thread_state)
            != SENTRY_LOGS_THREAD_RUNNING) {
            break;
        }

        switch (triggered_by) {
        case 0:
#ifdef SENTRY_PLATFORM_WINDOWS
            if (GetLastError() == ERROR_TIMEOUT) {
                SENTRY_TRACE("Logs flushed by timeout");
                break;
            }
#endif
            SENTRY_TRACE("Logs flushed by filled buffer");
            break;
#ifdef SENTRY_PLATFORM_UNIX
        case ETIMEDOUT:
            SENTRY_TRACE("Logs flushed by timeout");
            break;
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
        case 1:
            SENTRY_TRACE("Logs flushed by filled buffer");
            break;
#endif
        default:
            SENTRY_WARN("Logs flush trigger returned unexpected value");
            continue;
        }

        // Try to flush logs
        flush_logs_queue(false);
    }

    sentry__mutex_unlock(&task_lock);
    sentry__mutex_free(&task_lock);
    SENTRY_DEBUG("batching thread exiting");
    return 0;
}

static const char *
level_as_string(sentry_level_t level)
{
    switch (level) {
    case SENTRY_LEVEL_TRACE:
        return "trace";
    case SENTRY_LEVEL_DEBUG:
        return "debug";
    case SENTRY_LEVEL_INFO:
        return "info";
    case SENTRY_LEVEL_WARNING:
        return "warn";
    case SENTRY_LEVEL_ERROR:
        return "error";
    case SENTRY_LEVEL_FATAL:
        return "fatal";
    default:
        return "unknown";
    }
}

// TODO to be portable, pass in the length format specifier
#ifndef SENTRY_UNITTEST
static
#endif
    sentry_value_t
    construct_param_from_conversion(const char conversion, va_list *args_copy)
{
    sentry_value_t param_obj = sentry_value_new_object();
    switch (conversion) {
    case 'd':
    case 'i': {
        long long val = va_arg(*args_copy, long long);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_int64(val));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("integer"));
        break;
    }
    case 'u':
    case 'x':
    case 'X':
    case 'o': {
        unsigned long long int val = va_arg(*args_copy, unsigned long long int);
        // TODO update once unsigned 64-bit can be sent as non-string
        char buf[26];
        char format[8];
        snprintf(format, sizeof(format), "%%ll%c", conversion);
        snprintf(buf, sizeof(buf), format, val);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string(buf));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G': {
        double val = va_arg(*args_copy, double);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_double(val));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("double"));
        break;
    }
    case 'c': {
        int val = va_arg(*args_copy, int);
        char str[2] = { (char)val, '\0' };
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string(str));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 's': {
        const char *val = va_arg(*args_copy, const char *);
        if (val) {
            sentry_value_set_by_key(
                param_obj, "value", sentry_value_new_string(val));
        } else {
            sentry_value_set_by_key(
                param_obj, "value", sentry_value_new_string("(null)"));
        }
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 'p': {
        void *val = va_arg(*args_copy, void *);
        char ptr_str[32];
        snprintf(ptr_str, sizeof(ptr_str), "%p", val);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string(ptr_str));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    default:
        // Unknown format specifier, skip the argument
        (void)va_arg(*args_copy, void *);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string("(unknown)"));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }

    return param_obj;
}

static const char *
skip_flags(const char *fmt_ptr)
{
    while (*fmt_ptr
        && (*fmt_ptr == '-' || *fmt_ptr == '+' || *fmt_ptr == ' '
            || *fmt_ptr == '#' || *fmt_ptr == '0')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

static const char *
skip_width(const char *fmt_ptr)
{
    while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

static const char *
skip_precision(const char *fmt_ptr)
{

    if (*fmt_ptr == '.') {
        fmt_ptr++;
        while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
            fmt_ptr++;
        }
    }
    return fmt_ptr;
}

static const char *
skip_length(const char *fmt_ptr)
{
    while (*fmt_ptr
        && (*fmt_ptr == 'h' || *fmt_ptr == 'l' || *fmt_ptr == 'L'
            || *fmt_ptr == 'z' || *fmt_ptr == 'j' || *fmt_ptr == 't')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

// returns how many parameters were added to the attributes object
#ifndef SENTRY_UNITTEST
static
#endif
    int
    populate_message_parameters(
        sentry_value_t attributes, const char *message, va_list args)
{
    if (!message || sentry_value_is_null(attributes)) {
        return 0;
    }

    const char *fmt_ptr = message;
    int param_index = 0;
    va_list args_copy;
    va_copy(args_copy, args);

    while (*fmt_ptr) {
        // Find the next format specifier
        if (*fmt_ptr == '%') {
            fmt_ptr++; // Skip the '%'

            if (*fmt_ptr == '%') {
                // Escaped '%', not a format specifier
                fmt_ptr++;
                continue;
            }

            fmt_ptr = skip_flags(fmt_ptr);
            fmt_ptr = skip_width(fmt_ptr);
            fmt_ptr = skip_precision(fmt_ptr);
            fmt_ptr = skip_length(fmt_ptr);

            // Get the conversion specifier
            char conversion = *fmt_ptr;
            if (conversion) {
                char key[64];
                snprintf(key, sizeof(key), "sentry.message.parameter.%d",
                    param_index);
                sentry_value_t param_obj
                    = construct_param_from_conversion(conversion, &args_copy);
                sentry_value_set_by_key(attributes, key, param_obj);
                param_index++;
                fmt_ptr++;
            }
        } else {
            fmt_ptr++;
        }
    }

    va_end(args_copy);
    return param_index;
}

/**
 * This function assumes that `value` is owned, so we have to make sure that the
 * `value` was created or cloned by the caller or even better inc_refed.
 *
 * No-op if 'name' already exists in the attributes.
 */
static void
add_attribute(sentry_value_t attributes, sentry_value_t value, const char *type,
    const char *name)
{
    if (!sentry_value_is_null(sentry_value_get_by_key(attributes, name))) {
        sentry_value_decref(value);
        return;
    }
    sentry_value_t param_obj = sentry_value_new_object();
    sentry_value_set_by_key(param_obj, "value", value);
    sentry_value_set_by_key(param_obj, "type", sentry_value_new_string(type));
    sentry_value_set_by_key(attributes, name, param_obj);
}

/**
 * Extracts data from the scope and options, and adds it to the attributes
 * as well as directly setting `trace_id` for the log.
 *
 * We clone most values instead of incref, since they might otherwise change
 * between constructing the log & flushing it to an envelope.
 */
static void
add_scope_and_options_data(sentry_value_t log, sentry_value_t attributes)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(scope->propagation_context, "trace"),
            "trace_id");
        sentry_value_incref(trace_id);
        sentry_value_set_by_key(log, "trace_id", trace_id);

        sentry_value_t parent_span_id = sentry_value_new_object();
        sentry_value_t scoped_span_trace_id = sentry_value_new_null();
        if (scope->transaction_object) {
            sentry_value_t span_id = sentry_value_get_by_key(
                scope->transaction_object->inner, "span_id");
            sentry_value_incref(span_id);
            sentry_value_set_by_key(parent_span_id, "value", span_id);
            scoped_span_trace_id = sentry_value_get_by_key(
                scope->transaction_object->inner, "trace_id");
            sentry_value_incref(scoped_span_trace_id);
        } else if (scope->span) {
            sentry_value_t span_id
                = sentry_value_get_by_key(scope->span->inner, "span_id");
            sentry_value_incref(span_id);
            sentry_value_set_by_key(parent_span_id, "value", span_id);
            scoped_span_trace_id
                = sentry_value_get_by_key(scope->span->inner, "trace_id");
            sentry_value_incref(scoped_span_trace_id);
        }
        sentry_value_set_by_key(
            parent_span_id, "type", sentry_value_new_string("string"));
        if (scope->transaction_object || scope->span) {
            sentry_value_set_by_key(
                attributes, "sentry.trace.parent_span_id", parent_span_id);
            sentry_value_set_by_key(log, "trace_id", scoped_span_trace_id);
        } else {
            sentry_value_decref(parent_span_id);
            sentry_value_decref(scoped_span_trace_id);
        }

        if (!sentry_value_is_null(scope->user)) {
            sentry_value_t user_id = sentry_value_get_by_key(scope->user, "id");
            if (!sentry_value_is_null(user_id)) {
                sentry_value_incref(user_id);
                add_attribute(attributes, user_id, "string", "user.id");
            }

            sentry_value_t user_username
                = sentry_value_get_by_key(scope->user, "username");
            if (!sentry_value_is_null(user_username)) {
                sentry_value_incref(user_username);
                add_attribute(attributes, user_username, "string", "user.name");
            }

            sentry_value_t user_email
                = sentry_value_get_by_key(scope->user, "email");
            if (!sentry_value_is_null(user_email)) {
                sentry_value_incref(user_email);
                add_attribute(attributes, user_email, "string", "user.email");
            }
        }
        sentry_value_t os_context
            = sentry_value_get_by_key(scope->contexts, "os");
        if (!sentry_value_is_null(os_context)) {
            sentry_value_t os_name
                = sentry_value_get_by_key(os_context, "name");
            sentry_value_t os_version
                = sentry_value_get_by_key(os_context, "version");
            if (!sentry_value_is_null(os_name)) {
                sentry_value_incref(os_name);
                add_attribute(attributes, os_name, "string", "os.name");
            }
            if (!sentry_value_is_null(os_version)) {
                sentry_value_incref(os_version);
                add_attribute(attributes, os_version, "string", "os.version");
            }
        }
    }

    SENTRY_WITH_OPTIONS (options) {
        if (options->environment) {
            add_attribute(attributes,
                sentry_value_new_string(options->environment), "string",
                "sentry.environment");
        }
        if (options->release) {
            add_attribute(attributes, sentry_value_new_string(options->release),
                "string", "sentry.release");
        }
        add_attribute(attributes,
            sentry_value_new_string(sentry_options_get_sdk_name(options)),
            "string", "sentry.sdk.name");
    }

    add_attribute(attributes, sentry_value_new_string(sentry_sdk_version()),
        "string", "sentry.sdk.version");
}

static sentry_value_t
construct_log(sentry_level_t level, const char *message, va_list args)
{
    sentry_value_t log = sentry_value_new_object();
    sentry_value_t attributes = sentry_value_new_null();
    SENTRY_WITH_SCOPE (scope) {
        attributes = sentry__value_clone(scope->attributes);
    }

    SENTRY_WITH_OPTIONS (options) {
        // Extract custom attributes if the option is enabled
        if (sentry_options_get_logs_with_attributes(options)) {
            va_list args_copy;
            va_copy(args_copy, args);
            sentry_value_t custom_attributes
                = va_arg(args_copy, sentry_value_t);
            va_end(args_copy);
            if (sentry_value_get_type(custom_attributes)
                == SENTRY_VALUE_TYPE_OBJECT) {
                sentry_value_decref(attributes);
                attributes = sentry__value_clone(custom_attributes);
            } else {
                SENTRY_DEBUG("Discarded custom attributes on log: non-object "
                             "sentry_value_t passed in");
            }
            sentry_value_decref(custom_attributes);
        }

        // Format the message with remaining args (or all args if not using
        // custom attributes)
        va_list args_copy_1, args_copy_2, args_copy_3;
        va_copy(args_copy_1, args);
        va_copy(args_copy_2, args);
        va_copy(args_copy_3, args);

        // Skip the first argument (attributes) if using custom attributes
        if (sentry_options_get_logs_with_attributes(options)) {
            va_arg(args_copy_1, sentry_value_t);
            va_arg(args_copy_2, sentry_value_t);
            va_arg(args_copy_3, sentry_value_t);
        }

        int len = vsnprintf(NULL, 0, message, args_copy_1) + 1;
        va_end(args_copy_1);
        size_t size = (size_t)len;
        char *fmt_message = sentry_malloc(size);
        if (!fmt_message) {
            va_end(args_copy_2);
            va_end(args_copy_3);
            return sentry_value_new_null();
        }

        vsnprintf(fmt_message, size, message, args_copy_2);
        va_end(args_copy_2);

        sentry_value_set_by_key(
            log, "body", sentry_value_new_string(fmt_message));
        sentry_free(fmt_message);

        // Parse variadic arguments and add them to attributes
        if (populate_message_parameters(attributes, message, args_copy_3)) {
            // only add message template if we have parameters
            add_attribute(attributes, sentry_value_new_string(message),
                "string", "sentry.message.template");
        }
        va_end(args_copy_3);
    }

    sentry_value_set_by_key(
        log, "level", sentry_value_new_string(level_as_string(level)));

    // timestamp in seconds
    uint64_t usec_time = sentry__usec_time();
    sentry_value_set_by_key(log, "timestamp",
        sentry_value_new_double((double)usec_time / 1000000.0));

    // adds data from the scope & options to the attributes, and adds `trace_id`
    // to the log
    add_scope_and_options_data(log, attributes);

    sentry_value_set_by_key(log, "attributes", attributes);

    return log;
}

static void
debug_print_log(sentry_level_t level, const char *log_body)
{
    // TODO if we enable our debug-macro as logging integration
    //  we need to avoid recursion here
    switch (level) {
    case SENTRY_LEVEL_TRACE:
        SENTRY_TRACEF("LOG: %s", log_body);
        break;
    case SENTRY_LEVEL_DEBUG:
        SENTRY_DEBUGF("LOG: %s", log_body);
        break;
    case SENTRY_LEVEL_INFO:
        SENTRY_INFOF("LOG: %s", log_body);
        break;
    case SENTRY_LEVEL_WARNING:
        SENTRY_WARNF("LOG: %s", log_body);
        break;
    case SENTRY_LEVEL_ERROR:
        SENTRY_ERRORF("LOG: %s", log_body);
        break;
    case SENTRY_LEVEL_FATAL:
        SENTRY_FATALF("LOG: %s", log_body);
        break;
    }
}

log_return_value_t
sentry__logs_log(sentry_level_t level, const char *message, va_list args)
{
    bool enable_logs = false;
    SENTRY_WITH_OPTIONS (options) {
        if (options->enable_logs)
            enable_logs = true;
    }
    if (enable_logs) {
        bool discarded = false;
        // create log from message
        sentry_value_t log = construct_log(level, message, args);
        SENTRY_WITH_OPTIONS (options) {
            if (options->before_send_log_func) {
                log = options->before_send_log_func(
                    log, options->before_send_log_data);
                if (sentry_value_is_null(log)) {
                    SENTRY_DEBUG(
                        "log was discarded by the `before_send_log` hook");
                    discarded = true;
                }
            }
            if (options->debug && !sentry_value_is_null(log)) {
                debug_print_log(level,
                    sentry_value_as_string(
                        sentry_value_get_by_key(log, "body")));
            }
        }
        if (discarded) {
            return SENTRY_LOG_RETURN_DISCARD;
        }
        if (!enqueue_log(log)) {
            sentry_value_decref(log);
            return SENTRY_LOG_RETURN_FAILED;
        }
        return SENTRY_LOG_RETURN_SUCCESS;
    }
    return SENTRY_LOG_RETURN_DISABLED;
}

log_return_value_t
sentry_log_trace(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_TRACE, message, args);
    va_end(args);
    return result;
}

log_return_value_t
sentry_log_debug(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    const log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_DEBUG, message, args);
    va_end(args);
    return result;
}

log_return_value_t
sentry_log_info(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    const log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_INFO, message, args);
    va_end(args);
    return result;
}

log_return_value_t
sentry_log_warn(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    const log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_WARNING, message, args);
    va_end(args);
    return result;
}

log_return_value_t
sentry_log_error(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    const log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_ERROR, message, args);
    va_end(args);
    return result;
}

log_return_value_t
sentry_log_fatal(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    const log_return_value_t result
        = sentry__logs_log(SENTRY_LEVEL_FATAL, message, args);
    va_end(args);
    return result;
}

void
sentry__logs_startup(void)
{
    // Mark thread as starting before actually spawning so thread can transition
    // to RUNNING. This prevents shutdown from thinking the thread was never
    // started if it races with the thread's initialization.
    sentry__atomic_store(
        &g_logs_state.thread_state, (long)SENTRY_LOGS_THREAD_STARTING);

    sentry__cond_init(&g_logs_state.request_flush);

    sentry__thread_init(&g_logs_state.batching_thread);
    int spawn_result = sentry__thread_spawn(
        &g_logs_state.batching_thread, batching_thread_func, NULL);

    if (spawn_result == 1) {
        SENTRY_ERROR("Failed to start batching thread");
        // Failed to spawn, reset to STOPPED
        // Note: condition variable doesn't need explicit cleanup for static
        // storage (pthread_cond_t on POSIX and CONDITION_VARIABLE on Windows)
        sentry__atomic_store(
            &g_logs_state.thread_state, (long)SENTRY_LOGS_THREAD_STOPPED);
    }
}

void
sentry__logs_shutdown(uint64_t timeout)
{
    (void)timeout;
    SENTRY_DEBUG("shutting down logs system");

    // Atomically transition to STOPPED and get the previous state
    // This handles the race where thread might be in STARTING state:
    // - If thread's CAS hasn't run yet: CAS will fail, thread exits cleanly
    // - If thread already transitioned to RUNNING: normal shutdown path
    const long old_state = sentry__atomic_store(
        &g_logs_state.thread_state, (long)SENTRY_LOGS_THREAD_STOPPED);

    // If thread was never started, nothing to do
    if (old_state == SENTRY_LOGS_THREAD_STOPPED) {
        SENTRY_DEBUG("logs thread was not started, skipping shutdown");
        return;
    }

    // Thread was started (either STARTING or RUNNING), signal it to stop
    sentry__cond_wake(&g_logs_state.request_flush);

    // Always join the thread to avoid leaks
    sentry__thread_join(g_logs_state.batching_thread);

    // Perform final flush to ensure any remaining logs are sent
    flush_logs_queue(false);

    sentry__thread_free(&g_logs_state.batching_thread);

    SENTRY_DEBUG("logs system shutdown complete");
}

void
sentry__logs_flush_crash_safe(void)
{
    SENTRY_DEBUG("crash-safe logs flush");

    // Check if logs system is initialized
    const long state = sentry__atomic_fetch(&g_logs_state.thread_state);
    if (state == SENTRY_LOGS_THREAD_STOPPED) {
        return;
    }

    // Signal the thread to stop but don't wait, since the crash-safe flush
    // will spin-lock on flushing anyway.
    sentry__atomic_store(
        &g_logs_state.thread_state, (long)SENTRY_LOGS_THREAD_STOPPED);

    // Perform crash-safe flush directly to disk to avoid transport queuing
    // This is safe because we're in a crash scenario and the main thread
    // is likely dead or dying anyway
    flush_logs_queue(true);

    SENTRY_DEBUG("crash-safe logs flush complete");
}

void
sentry__logs_force_flush(void)
{
    while (sentry__atomic_fetch(&g_logs_state.flushing)) {
        sentry__cpu_relax();
    }
    flush_logs_queue(false);
}

#ifdef SENTRY_UNITTEST
/**
 * Wait for the logs batching thread to be ready.
 * This is a test-only helper to avoid race conditions in tests.
 */
void
sentry__logs_wait_for_thread_startup(void)
{
    const int max_wait_ms = 1000;
    const int check_interval_ms = 10;
    const int max_attempts = max_wait_ms / check_interval_ms;

    for (int i = 0; i < max_attempts; i++) {
        const long state = sentry__atomic_fetch(&g_logs_state.thread_state);
        if (state == SENTRY_LOGS_THREAD_RUNNING) {
            SENTRY_DEBUGF(
                "logs thread ready after %d ms", i * check_interval_ms);
            return;
        }
        sleep_ms(check_interval_ms);
    }

    SENTRY_WARNF(
        "logs thread failed to start within %d ms timeout", max_wait_ms);
}
#endif
