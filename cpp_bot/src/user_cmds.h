// user_cmds.h
#pragma once
#include <tgbot/tgbot.h>

namespace user_cmds {
    void setup(TgBot::Bot& bot);
    void cmd_start(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_wallet(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_play(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_result(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_cancel_match(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_profile(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_stats(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_leaderboard(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_share(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_tournaments(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_support(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_language(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_daily(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_tutorial(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void cmd_mytickets(TgBot::Message::Ptr msg, TgBot::Bot& bot);
}
