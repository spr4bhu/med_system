#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>
#include "protocol.h"

/* Posture states */
typedef enum {
    POSTURE_UNKNOWN   = 0,
    POSTURE_STANDING  = 1,
    POSTURE_SITTING   = 2,
    POSTURE_LYING     = 3
} posture_t;

/* Emergency reasons (bitfield) */
typedef enum {
    EMERGENCY_NONE      = 0,
    EMERGENCY_FALL      = (1 << 0),
    EMERGENCY_BUTTON    = (1 << 1),
    EMERGENCY_HIGH_TEMP = (1 << 2),
    EMERGENCY_HIGH_HR   = (1 << 3)
} emergency_reason_t;

/* System state */
typedef enum {
    SYSTEM_STATE_IDLE      = 0,
    SYSTEM_STATE_NORMAL    = 1,
    SYSTEM_STATE_EMERGENCY = 2
} system_state_t;

/* Security state (Node B) */
typedef enum {
    SECURITY_STATE_IDLE        = 0,
    SECURITY_STATE_IR_DETECTED = 1,
    SECURITY_STATE_AUTHORIZED  = 2,
    SECURITY_STATE_INTRUSION   = 3
} security_state_t;

#endif /* MESSAGE_TYPES_H */
