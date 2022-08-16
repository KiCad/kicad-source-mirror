#include "sentry_boot.h"

const char *
sentry_sdk_version()
{
    return SENTRY_SDK_VERSION;
}

const char *
sentry_sdk_name()
{
    return SENTRY_SDK_NAME;
}

const char *
sentry_sdk_user_agent()
{
    return SENTRY_SDK_USER_AGENT;
}
