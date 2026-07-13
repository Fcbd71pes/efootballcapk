// utils.cpp — Utility functions
#include "utils.h"
#include "lang.h"
#include <algorithm>

namespace utils {

std::string esc(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            default:  out += c;
        }
    }
    return out;
}

bool is_staff(long long uid) {
    auto admins = db::get_admins();
    auto mgrs = db::get_managers();
    if (uid == config::SUPER_ADMIN) return true;
    for (auto a : admins) if (a == uid) return true;
    for (auto m : mgrs) if (m == uid) return true;
    return false;
}

std::vector<long long> broadcast_chats() {
    std::vector<long long> chats;
    if (config::LOBBY_CHANNEL_ID) chats.push_back(config::LOBBY_CHANNEL_ID);
    if (config::GROUP_ID && config::GROUP_ID != config::LOBBY_CHANNEL_ID)
        chats.push_back(config::GROUP_ID);
    return chats;
}

std::vector<long long> staff_ids() {
    std::vector<long long> ids;
    ids.push_back(config::SUPER_ADMIN);
    for (auto a : db::get_admins()) if (a != config::SUPER_ADMIN) ids.push_back(a);
    for (auto m : db::get_managers()) ids.push_back(m);
    return ids;
}

TgBot::ReplyKeyboardMarkup::Ptr main_kb(const std::string& lng) {
    auto kb = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    kb->resizeKeyboard = true;
    auto row1 = std::vector<TgBot::KeyboardButton::Ptr>();
    auto row2 = std::vector<TgBot::KeyboardButton::Ptr>();
    auto row3 = std::vector<TgBot::KeyboardButton::Ptr>();
    auto row4 = std::vector<TgBot::KeyboardButton::Ptr>();
    auto mk_btn = [](const std::string& txt) {
        auto b = std::make_shared<TgBot::KeyboardButton>();
        b->text = txt;
        return b;
    };
    row1.push_back(mk_btn(lang::t("btn_play", lng)));
    row1.push_back(mk_btn(lang::t("btn_wallet", lng)));
    row2.push_back(mk_btn(lang::t("btn_profile", lng)));
    row2.push_back(mk_btn(lang::t("btn_lb", lng)));
    row3.push_back(mk_btn(lang::t("btn_tourney", lng)));
    row3.push_back(mk_btn(lang::t("btn_daily", lng)));
    row4.push_back(mk_btn(lang::t("btn_rules", lng)));
    row4.push_back(mk_btn(lang::t("btn_share", lng)));
    row4.push_back(mk_btn(lang::t("btn_lang", lng)));
    kb->keyboard = {row1, row2, row3, row4};
    return kb;
}

TgBot::ReplyKeyboardMarkup::Ptr cancel_kb(const std::string& lng) {
    auto kb = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    kb->resizeKeyboard = true;
    auto b = std::make_shared<TgBot::KeyboardButton>();
    b->text = lang::t("btn_cancel", lng);
    kb->keyboard = {{b}};
    return kb;
}

std::optional<db::User> ensure_user(long long uid, const std::string& username, long long ref) {
    auto u = db::get_user(uid);
    if (!u) {
        db::create_user(uid, username, ref);
        if (ref) db::increment_referrals(ref);
        u = db::get_user(uid);
    }
    if (u && u->is_banned) return std::nullopt;
    return u;
}

} // namespace utils
