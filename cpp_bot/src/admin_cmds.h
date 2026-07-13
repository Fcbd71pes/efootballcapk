// admin_cmds.h
#pragma once
#include <tgbot/tgbot.h>

namespace admin_cmds {
    void setup(TgBot::Bot& bot);
    bool is_admin(long long uid);
    bool is_manager(long long uid);
}
