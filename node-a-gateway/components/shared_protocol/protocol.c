#include "protocol.h"
#include "message_types.h"

/* Protocol utility functions */

const char *get_message_type_name(message_type_t msg_type)
{
    switch (msg_type) {
        case MSG_TYPE_EMERGENCY:
            return "EMERGENCY";
        case MSG_TYPE_CLEAR:
            return "CLEAR";
        case MSG_TYPE_SECURITY_ALERT:
            return "SECURITY_ALERT";
        case MSG_TYPE_STATUS:
            return "STATUS";
        default:
            return "UNKNOWN";
    }
}
