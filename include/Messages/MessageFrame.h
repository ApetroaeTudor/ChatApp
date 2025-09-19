#pragma once

#include "resources.h"
#include "string.h"

struct MessageFrame
{
    char msg[constants::MAX_LEN];
    int msg_len;

    void clear_msg() {
        memset(msg,'\0',sizeof(msg));
    }
};