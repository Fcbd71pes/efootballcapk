// handlers.h
#pragma once
#include <tgbot/tgbot.h>

namespace handlers {
    void setup(TgBot::Bot& bot);
    void handle_text(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void handle_photo(TgBot::Message::Ptr msg, TgBot::Bot& bot);
    void handle_callback(TgBot::CallbackQuery::Ptr q, TgBot::Bot& bot);
}
