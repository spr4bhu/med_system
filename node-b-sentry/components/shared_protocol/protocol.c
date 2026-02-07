#include "protocol.h"
#include "message_types.h"

/* Protocol utility functions */

const char* get_message_type_name(message_type_t msg_type) {
    switch (msg_type) {
        case MSG_TYPE_SENSOR_DATA:
            return "SENSOR_DATA";
        case MSG_TYPE_FALL_ALERT:
            return "FALL_ALERT";
        case MSG_TYPE_SOS:
            return "SOS";
        case MSG_TYPE_ACCESS_LOG:
            return "ACCESS_LOG";
        case MSG_TYPE_INTRUSION:
            return "INTRUSION";
        case MSG_TYPE_VITALS_ABNORMAL:
            return "VITALS_ABNORMAL";
        default:
            return "UNKNOWN";
    }
}
