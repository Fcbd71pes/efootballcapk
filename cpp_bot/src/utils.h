// utils.h — Utility functions
#pragma once
#include <tgbot/tgbot.h>
#include <string>
#include <vector>
#include "db.h"
#include "config.h"

namespace utils {
    // HTML escape
    std::string esc(const std::string& s);
    // Check if user is staff (admin or manager)
    bool is_staff(long long uid);
    // Broadcast chat IDs
    std::vector<long long> broadcast_chats();
    // Build main keyboard
    TgBot::ReplyKeyboardMarkup::Ptr main_kb(const std::string& lang);
    // Build cancel keyboard
    TgBot::ReplyKeyboardMarkup::Ptr cancel_kb(const std::string& lang);
    // Get all staff IDs
    std::vector<long long> staff_ids();
    // Ensure user exists in DB; returns user or nullopt
    std::optional<db::User> ensure_user(long long uid, const std::string& username, long long ref = 0);
}
