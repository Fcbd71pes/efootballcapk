// handlers.cpp — Message & callback dispatcher
#include "handlers.h"
#include "db.h"
#include "lang.h"
#include "config.h"
#include "utils.h"
#include "user_cmds.h"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace handlers {

static std::string dbl(double v, int d=2) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(d)<<v; return ss.str();
}

// ── Text Handler ───────────────────────────────────────────────────────────
void handle_text(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    std::string txt = msg->text;
    if (txt.empty() || txt[0] == '/') return;

    auto u_opt = db::get_user(uid);
    if (!u_opt) { db::create_user(uid, msg->from ? msg->from->username : ""); u_opt = db::get_user(uid); }
    if (!u_opt || u_opt->is_banned) return;
    auto& u = *u_opt;
    std::string lng = u.lang;
    std::string state = u.state;
    std::string sd = u.state_data;

    // Cancel button check
    if (txt == lang::t("btn_cancel","bn") || txt == lang::t("btn_cancel","en") ||
        txt == "❌ Cancel" || txt == "❌ বাতিল") {
        auto q = db::get_from_queue(uid);
        if (q) {
            db::remove_from_queue(uid);
            try { bot.getApi().deleteMessage(config::LOBBY_CHANNEL_ID, (int)q->lobby_msg_id); } catch(...) {}
        }
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, lang::t("cancelled", lng), nullptr, nullptr, utils::main_kb(lng));
        return;
    }

    // ── Registration ──────────────────────────────────────────────────────
    if (state == "awaiting_ign") {
        db::update_user_str(uid, "ingame_name", txt);
        db::set_state(uid, "awaiting_phone");
        bot.getApi().sendMessage(uid, lang::t("ask_phone", lng), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }
    if (state == "awaiting_phone") {
        db::update_user_str(uid, "phone", txt);
        db::update_user_int(uid, "is_registered", 1);
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, lang::t("reg_done", lng), nullptr, nullptr, utils::main_kb(lng));
        return;
    }

    // ── Profile edit ──────────────────────────────────────────────────────
    if (state == "awaiting_edit_ign") {
        if (txt.size() < 3 || txt.size() > 30) {
            bot.getApi().sendMessage(uid, "❌ নাম ৩-৩০ অক্ষরের মধ্যে হতে হবে।", nullptr, nullptr, utils::cancel_kb(lng));
            return;
        }
        db::update_user_str(uid, "ingame_name", txt);
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, "✅ IGN আপডেট হয়েছে: <b>" + utils::esc(txt) + "</b>", nullptr, nullptr, utils::main_kb(lng));
        return;
    }
    if (state == "awaiting_edit_phone") {
        db::update_user_str(uid, "phone", txt);
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, "✅ Phone আপডেট: <code>" + txt + "</code>", nullptr, nullptr, utils::main_kb(lng));
        return;
    }

    // ── Room Code ─────────────────────────────────────────────────────────
    if (state == "awaiting_room_code") {
        auto m_opt = db::get_match(sd);
        if (m_opt) {
            long long opp_id = (uid == m_opt->p1_id) ? m_opt->p2_id : m_opt->p1_id;
            std::string opp_lang = db::get_user_lang(opp_id);
            db::clear_state(uid);
            std::map<std::string,std::string> args = {
                {"code", utils::esc(txt)},
                {"mins", std::to_string(config::MATCH_TIMEOUT_MINUTES)}
            };
            try {
                bot.getApi().sendMessage(opp_id, lang::t("room_code_fwd", opp_lang, args), nullptr, nullptr, utils::main_kb(opp_lang));
            } catch (...) {}
            std::map<std::string,std::string> args2 = {{"mins", std::to_string(config::MATCH_TIMEOUT_MINUTES)}};
            bot.getApi().sendMessage(uid, lang::t("room_code_confirm", lng, args2), nullptr, nullptr, utils::main_kb(lng));
        } else {
            db::clear_state(uid);
            bot.getApi().sendMessage(uid, lang::t("no_active_match", lng), nullptr, nullptr, utils::main_kb(lng));
        }
        return;
    }

    // ── MFS deposit TxID ─────────────────────────────────────────────────
    if (state == "awaiting_mfs_dep_txid") {
        std::istringstream iss(txt);
        std::string txid, amount_str;
        if (!(iss >> txid >> amount_str)) {
            bot.getApi().sendMessage(uid, lang::t("mfs_wrong_fmt", lng), nullptr, nullptr, utils::cancel_kb(lng));
            return;
        }
        double amount = 0;
        try { amount = std::stod(amount_str); } catch (...) {
            bot.getApi().sendMessage(uid, lang::t("mfs_wrong_fmt", lng), nullptr, nullptr, utils::cancel_kb(lng));
            return;
        }
        if (amount < config::MINIMUM_DEPOSIT) {
            std::map<std::string,std::string> args = {{"min", dbl(config::MINIMUM_DEPOSIT, 0)}};
            bot.getApi().sendMessage(uid, lang::t("mfs_min_dep", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
            return;
        }
        // Build JSON-like state data: method|txid|amount
        auto method_end = sd.find('|');
        std::string method = (method_end != std::string::npos) ? sd.substr(0, method_end) : sd;
        db::set_state(uid, "awaiting_mfs_dep_screenshot", method + "|" + txid + "|" + dbl(amount));
        bot.getApi().sendMessage(uid, lang::t("mfs_send_ss", lng), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // ── MFS withdrawal amount ─────────────────────────────────────────────
    if (state == "awaiting_mfs_wit_amount") {
        double amount = 0;
        try { amount = std::stod(txt); } catch (...) {
            bot.getApi().sendMessage(uid, lang::t("invalid_number", lng)); return;
        }
        if (amount < config::MINIMUM_WITHDRAWAL) {
            std::map<std::string,std::string> args = {{"min", dbl(config::MINIMUM_WITHDRAWAL, 0)}};
            bot.getApi().sendMessage(uid, lang::t("wit_min", lng, args)); return;
        }
        if (amount > u.available_bal) {
            bot.getApi().sendMessage(uid, lang::t("insufficient_bal", lng)); return;
        }
        db::set_state(uid, "awaiting_mfs_wit_account", sd + "|" + dbl(amount));
        auto method_end = sd.find('|');
        std::string method = (method_end != std::string::npos) ? sd.substr(0, method_end) : sd;
        std::map<std::string,std::string> args = {{"method", method}};
        bot.getApi().sendMessage(uid, lang::t("wit_ask_account", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // ── MFS withdrawal account ────────────────────────────────────────────
    if (state == "awaiting_mfs_wit_account") {
        // sd: method|amount
        std::string method_part = sd;
        double amount = 0;
        auto sep = sd.rfind('|');
        if (sep != std::string::npos) {
            method_part = sd.substr(0, sep);
            try { amount = std::stod(sd.substr(sep + 1)); } catch (...) {}
        }
        auto sep2 = method_part.find('|');
        std::string method = (sep2 != std::string::npos) ? method_part.substr(0, sep2) : method_part;
        int wid = db::create_mfs_withdrawal(uid, method, txt, amount);
        db::clear_state(uid);
        std::map<std::string,std::string> args = {{"amount", dbl(amount) + " TK"}};
        bot.getApi().sendMessage(uid, lang::t("wit_submitted", lng, args), nullptr, nullptr, utils::main_kb(lng));
        // Notify staff
        auto mb_it = config::MOBILE_BANKING.find(method);
        std::string mb_name = (mb_it != config::MOBILE_BANKING.end()) ? mb_it->second.name : method;
        std::string notif = "💸 MFS Withdrawal #" + std::to_string(wid) + "\n" +
                            "👤 " + utils::esc(u.ingame_name) + " (" + std::to_string(uid) + ")\n" +
                            "📱 " + mb_name + ": " + txt + "\n" +
                            "💰 " + dbl(amount) + " TK\n\n" +
                            "✅ /wit_approve_mfs " + std::to_string(wid) +
                            "   ❌ /wit_reject_mfs " + std::to_string(wid);
        for (auto sid : utils::staff_ids()) {
            try { bot.getApi().sendMessage(sid, notif); } catch (...) {}
        }
        return;
    }

    // ── Exchange deposit amount ────────────────────────────────────────────
    if (state == "awaiting_exc_dep_amount") {
        double usdt = 0;
        try { usdt = std::stod(txt); } catch (...) {
            bot.getApi().sendMessage(uid, lang::t("invalid_number", lng)); return;
        }
        if (usdt < config::MIN_USDT_DEPOSIT) {
            std::map<std::string,std::string> args = {{"min", dbl(config::MIN_USDT_DEPOSIT)}};
            bot.getApi().sendMessage(uid, lang::t("exc_min_usdt", lng, args)); return;
        }
        double rate = db::deposit_rate();
        double amount_tk = usdt * rate;
        // sd = exchanger_key
        auto it = config::EXCHANGERS.find(sd);
        std::string uid_label = (it != config::EXCHANGERS.end()) ? it->second.uid_label : "UID";
        db::set_state(uid, "awaiting_exc_dep_uid", sd + "|" + dbl(usdt, 4) + "|" + dbl(amount_tk));
        std::map<std::string,std::string> args = {{"uid_label", uid_label}};
        bot.getApi().sendMessage(uid, lang::t("exc_ask_uid", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // ── Exchange deposit user UID ──────────────────────────────────────────
    if (state == "awaiting_exc_dep_uid") {
        // sd: exchanger|usdt|amount_tk
        db::set_state(uid, "awaiting_exc_dep_screenshot", sd + "|" + txt);
        bot.getApi().sendMessage(uid, lang::t("exc_ask_ss", lng), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // ── Exchange withdrawal amount ─────────────────────────────────────────
    if (state == "awaiting_exc_wit_amount") {
        double usdt = 0;
        try { usdt = std::stod(txt); } catch (...) {
            bot.getApi().sendMessage(uid, lang::t("invalid_number", lng)); return;
        }
        if (usdt < config::MIN_USDT_WITHDRAWAL) {
            std::map<std::string,std::string> args = {{"min", dbl(config::MIN_USDT_WITHDRAWAL)}};
            bot.getApi().sendMessage(uid, lang::t("wit_min_usdt", lng, args)); return;
        }
        double rate = db::withdraw_rate();
        double amount_tk = usdt * rate;
        if (amount_tk > u.available_bal) {
            bot.getApi().sendMessage(uid, lang::t("insufficient_bal", lng)); return;
        }
        auto it = config::EXCHANGERS.find(sd);
        std::string uid_label = (it != config::EXCHANGERS.end()) ? it->second.uid_label : "UID";
        db::set_state(uid, "awaiting_exc_wit_uid", sd + "|" + dbl(usdt, 4) + "|" + dbl(amount_tk));
        std::map<std::string,std::string> args = {{"method", uid_label}};
        bot.getApi().sendMessage(uid, lang::t("wit_ask_account", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // ── Exchange withdrawal user UID ───────────────────────────────────────
    if (state == "awaiting_exc_wit_uid") {
        // sd: exchanger|usdt|amount_tk
        std::string sep_data = sd;
        std::string exc_key, usdt_str, tk_str;
        auto p1 = sep_data.find('|');
        exc_key = sep_data.substr(0, p1);
        auto p2 = sep_data.find('|', p1+1);
        usdt_str = sep_data.substr(p1+1, p2-p1-1);
        tk_str = sep_data.substr(p2+1);
        double amount_usdt = 0, amount_tk = 0;
        try { amount_usdt = std::stod(usdt_str); amount_tk = std::stod(tk_str); } catch (...) {}
        int wid = db::create_exc_withdrawal(uid, exc_key, txt, amount_usdt, amount_tk);
        db::clear_state(uid);
        std::map<std::string,std::string> args = {{"amount", dbl(amount_usdt,4) + " USDT (" + dbl(amount_tk) + " TK)"}};
        bot.getApi().sendMessage(uid, lang::t("wit_submitted", lng, args), nullptr, nullptr, utils::main_kb(lng));
        auto it = config::EXCHANGERS.find(exc_key);
        std::string exc_name = (it != config::EXCHANGERS.end()) ? it->second.name : exc_key;
        std::string our_uid = (it != config::EXCHANGERS.end()) ? it->second.our_uid : "?";
        std::string notif = "💸 Exchange Withdrawal #" + std::to_string(wid) + "\n" +
                            "👤 " + utils::esc(u.ingame_name) + " (" + std::to_string(uid) + ")\n" +
                            "🏦 " + exc_name + "\n" +
                            "💵 " + dbl(amount_usdt,4) + " USDT = " + dbl(amount_tk) + " TK\n" +
                            "📤 Their UID: " + txt + "\n📥 Our UID: " + our_uid + "\n\n" +
                            "✅ /ewit_approve " + std::to_string(wid) +
                            "   ❌ /ewit_reject " + std::to_string(wid);
        for (auto sid : utils::staff_ids()) {
            try { bot.getApi().sendMessage(sid, notif); } catch (...) {}
        }
        return;
    }

    // ── Menu button routing ───────────────────────────────────────────────
    auto match_btn = [&](const std::string& key) {
        return txt == lang::t(key, "bn") || txt == lang::t(key, "en");
    };
    if (match_btn("btn_play"))   { user_cmds::cmd_play(msg, bot); return; }
    if (match_btn("btn_wallet")) { user_cmds::cmd_wallet(msg, bot); return; }
    if (match_btn("btn_profile")){ user_cmds::cmd_profile(msg, bot); return; }
    if (match_btn("btn_lb"))     { user_cmds::cmd_leaderboard(msg, bot); return; }
    if (match_btn("btn_share"))  { user_cmds::cmd_share(msg, bot); return; }
    if (match_btn("btn_tourney")){ user_cmds::cmd_tournaments(msg, bot); return; }
    if (match_btn("btn_result")) { user_cmds::cmd_result(msg, bot); return; }
    if (match_btn("btn_lang"))   { user_cmds::cmd_language(msg, bot); return; }
    if (match_btn("btn_daily"))  { user_cmds::cmd_daily(msg, bot); return; }
    if (match_btn("btn_tutorial")){ user_cmds::cmd_tutorial(msg, bot); return; }
    if (match_btn("btn_rules")) {
        std::string rules = db::get_setting("rules_text");
        bot.getApi().sendMessage(uid, rules.empty() ?
                                 (lng == "en" ? "No rules set." : "নিয়মাবলী সেট করা নেই।") : rules);
        return;
    }
}

// ── Photo Handler ─────────────────────────────────────────────────────────
void handle_photo(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u_opt = db::get_user(uid);
    if (!u_opt || u_opt->is_banned) return;
    auto& u = *u_opt;
    std::string lng = u.lang;
    std::string state = u.state;
    std::string sd = u.state_data;
    if (msg->photo.empty()) return;
    std::string file_id = msg->photo.back()->fileId;

    // ── Match screenshot ──────────────────────────────────────────────────
    if (state == "awaiting_screenshot") {
        std::string mid = sd;
        auto m_opt = db::get_match(mid);
        if (!m_opt) m_opt = db::get_active_match(uid);
        if (!m_opt) {
            db::clear_state(uid);
            bot.getApi().sendMessage(uid, lang::t("no_active_match", lng), nullptr, nullptr, utils::main_kb(lng));
            return;
        }
        mid = m_opt->match_id;
        if (m_opt->status != "in_progress") {
            db::clear_state(uid);
            bot.getApi().sendMessage(uid, lang::t("match_not_active", lng), nullptr, nullptr, utils::main_kb(lng));
            return;
        }
        bool already = (uid == m_opt->p1_id && !m_opt->p1_screenshot.empty()) ||
                       (uid == m_opt->p2_id && !m_opt->p2_screenshot.empty());
        if (already) {
            db::clear_state(uid);
            bot.getApi().sendMessage(uid, lang::t("already_submitted", lng), nullptr, nullptr, utils::main_kb(lng));
            return;
        }
        db::Match updated = db::submit_screenshot(mid, uid, file_id);
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, lang::t("ss_received", lng), nullptr, nullptr, utils::main_kb(lng));
        long long opp_id = (uid == updated.p1_id) ? updated.p2_id : updated.p1_id;
        std::string opp_lang = db::get_user_lang(opp_id);
        try { bot.getApi().sendMessage(opp_id, lang::t("opp_submitted", opp_lang)); } catch (...) {}

        // Both submitted — notify staff for manual review
        if (!updated.p1_screenshot.empty() && !updated.p2_screenshot.empty()) {
            std::string notif = "📸 Match <b>" + mid + "</b> — Both screenshots received.\n"
                                "P1: " + std::to_string(updated.p1_id) + " | "
                                "P2: " + std::to_string(updated.p2_id) + "\n"
                                "Fee: " + dbl(updated.fee, 0) + " TK\n\n"
                                "✅ /resolve " + mid + " <winner_id>\n"
                                "❌ /cancel_m " + mid;
            auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
            for (auto sid : utils::staff_ids()) {
                try { bot.getApi().sendMessage(sid, notif, nullptr, nullptr, kb, "HTML"); } catch (...) {}
            }
        }
        return;
    }

    // ── MFS Deposit screenshot ────────────────────────────────────────────
    if (state == "awaiting_mfs_dep_screenshot") {
        // sd: method|txid|amount
        std::string method, txid, amount_s;
        auto p1 = sd.find('|'); method = sd.substr(0, p1);
        auto p2 = sd.find('|', p1+1); txid = sd.substr(p1+1, p2-p1-1);
        amount_s = sd.substr(p2+1);
        double amount = 0;
        try { amount = std::stod(amount_s); } catch (...) {}
        int req_id = db::create_mfs_deposit(uid, method, txid, amount, file_id);
        db::clear_state(uid);
        bot.getApi().sendMessage(uid, lang::t("dep_submitted", lng), nullptr, nullptr, utils::main_kb(lng));
        auto mb_it = config::MOBILE_BANKING.find(method);
        std::string mb_name = (mb_it != config::MOBILE_BANKING.end()) ? mb_it->second.name : method;
        std::string notif = "🆕 MFS Deposit #" + std::to_string(req_id) + "\n" +
                            "👤 " + utils::esc(u.ingame_name) + " (" + std::to_string(uid) + ")\n" +
                            "📱 " + mb_name + " | TxID: " + txid + "\n" +
                            "💰 " + dbl(amount) + " TK\n\n" +
                            "✅ /dep_approve_mfs " + std::to_string(req_id) +
                            "   ❌ /dep_reject_mfs " + std::to_string(req_id);
        for (auto sid : utils::staff_ids()) {
            try { bot.getApi().sendPhoto(sid, file_id, notif); } catch (...) {}
        }
        return;
    }

    // ── Exchange Deposit screenshot ────────────────────────────────────────
    if (state == "awaiting_exc_dep_screenshot") {
        // sd: exchanger|usdt|amount_tk|user_uid
        std::string exc_key, usdt_s, tk_s, user_uid;
        auto p1 = sd.find('|'); exc_key = sd.substr(0, p1);
        auto p2 = sd.find('|', p1+1); usdt_s = sd.substr(p1+1, p2-p1-1);
        auto p3 = sd.find('|', p2+1); tk_s = sd.substr(p2+1, p3-p2-1);
        user_uid = sd.substr(p3+1);
        double amount_usdt = 0, amount_tk = 0;
        try { amount_usdt = std::stod(usdt_s); amount_tk = std::stod(tk_s); } catch (...) {}
        auto exc_it = config::EXCHANGERS.find(exc_key);
        std::string our_uid = (exc_it != config::EXCHANGERS.end()) ? exc_it->second.our_uid : "";
        std::string exc_name = (exc_it != config::EXCHANGERS.end()) ? exc_it->second.name : exc_key;
        int req_id = db::create_exc_deposit(uid, exc_key, our_uid, user_uid, amount_usdt, amount_tk, file_id);
        db::clear_state(uid);
        std::map<std::string,std::string> args = {
            {"name", exc_name}, {"usdt", dbl(amount_usdt, 4)},
            {"bdt", dbl(amount_tk)}, {"user_uid", user_uid}
        };
        bot.getApi().sendMessage(uid, lang::t("exc_dep_submitted", lng, args), nullptr, nullptr, utils::main_kb(lng));
        std::string notif = "🆕 Exchange Deposit #" + std::to_string(req_id) + "\n" +
                            "👤 " + utils::esc(u.ingame_name) + " (" + std::to_string(uid) + ")\n" +
                            "🏦 " + exc_name + "\n" +
                            "💵 " + dbl(amount_usdt,4) + " USDT = " + dbl(amount_tk) + " TK\n" +
                            "📥 Our UID: " + our_uid + "\n" +
                            "📤 Their UID: " + user_uid + "\n\n" +
                            "✅ /dep_approve_exc " + std::to_string(req_id) +
                            "   ❌ /dep_reject_exc " + std::to_string(req_id);
        for (auto sid : utils::staff_ids()) {
            try { bot.getApi().sendPhoto(sid, file_id, notif); } catch (...) {}
        }
        return;
    }

    // Unrecognized photo
    bot.getApi().sendMessage(uid, lng == "bn" ?
                             "📸 ছবি পেয়েছি, কিন্তু এখন কোনো প্রক্রিয়া নেই।" :
                             "📸 Photo received, but no active process found.", nullptr, nullptr, utils::main_kb(lng));
}

// ── Callback Handler ───────────────────────────────────────────────────────
void handle_callback(TgBot::CallbackQuery::Ptr q, TgBot::Bot& bot) {
    bot.getApi().answerCallbackQuery(q->id);
    std::string data = q->data;
    long long uid = q->from->id;
    if (data == "none") return;
    std::string lng = db::get_user_lang(uid);
    auto u_opt = db::get_user(uid);
    auto msg = q->message;

    // Language selection
    if (data == "setlang_bn" || data == "setlang_en") {
        std::string new_lang = data.substr(8);
        db::update_user_str(uid, "lang", new_lang);
        bot.getApi().editMessageText(lang::t("lang_set", new_lang) + "\n" + lang::t("ask_ign", new_lang),
                                     msg->chat->id, msg->messageId, "", "", false, nullptr);
        db::set_state(uid, "awaiting_ign");
        return;
    }

    // Deposit routing
    if (data == "deposit") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        auto r1 = std::vector<TgBot::InlineKeyboardButton::Ptr>();
        auto r2 = std::vector<TgBot::InlineKeyboardButton::Ptr>();
        auto b1 = std::make_shared<TgBot::InlineKeyboardButton>();
        b1->text = lang::t("btn_mfs", lng); b1->callbackData = "dep_mfs";
        auto b2 = std::make_shared<TgBot::InlineKeyboardButton>();
        b2->text = lang::t("btn_exchange", lng); b2->callbackData = "dep_exc";
        r1.push_back(b1); r2.push_back(b2);
        kb->inlineKeyboard = {r1, r2};
        bot.getApi().editMessageText(lang::t("choose_dep_method", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data == "dep_mfs") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (auto& kv : config::MOBILE_BANKING) {
            auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = kv.second.emoji + " " + kv.second.name;
            b->callbackData = "dep_mfs_" + kv.first;
            row.push_back(b); kb->inlineKeyboard.push_back(row);
        }
        bot.getApi().editMessageText(lang::t("mfs_select", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data.substr(0, 8) == "dep_mfs_") {
        std::string method = data.substr(8);
        auto it = config::MOBILE_BANKING.find(method);
        if (it == config::MOBILE_BANKING.end()) return;
        db::set_state(uid, "awaiting_mfs_dep_txid", method);
        std::map<std::string,std::string> args = {{"name", it->second.name}, {"number", it->second.number}};
        bot.getApi().sendMessage(uid, lang::t("mfs_dep_inst", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }
    if (data == "dep_exc") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (auto& kv : config::EXCHANGERS) {
            if (kv.second.our_uid.empty()) continue;
            auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = kv.second.emoji + " " + kv.second.name;
            b->callbackData = "dep_exc_" + kv.first;
            row.push_back(b); kb->inlineKeyboard.push_back(row);
        }
        if (kb->inlineKeyboard.empty()) {
            bot.getApi().sendMessage(uid, lang::t("exc_none_configured", lng)); return;
        }
        bot.getApi().editMessageText(lang::t("exc_select", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data.substr(0, 8) == "dep_exc_") {
        std::string exc_key = data.substr(8);
        auto it = config::EXCHANGERS.find(exc_key);
        if (it == config::EXCHANGERS.end()) return;
        if (it->second.our_uid.empty()) {
            bot.getApi().sendMessage(uid, lang::t("exc_uid_not_set", lng)); return;
        }
        std::string note = (lng == "bn") ? it->second.deposit_note_bn : it->second.deposit_note_en;
        db::set_state(uid, "awaiting_exc_dep_amount", exc_key);
        std::map<std::string,std::string> args = {
            {"name",      it->second.name},
            {"uid_label", it->second.uid_label},
            {"our_uid",   it->second.our_uid},
            {"note",      note},
            {"min_dep",   std::to_string(config::MIN_USDT_DEPOSIT)},
        };
        bot.getApi().sendMessage(uid, lang::t("exc_dep_show_uid", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // Withdraw routing
    if (data == "withdraw") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        auto b1 = std::make_shared<TgBot::InlineKeyboardButton>();
        b1->text = lang::t("btn_mfs", lng); b1->callbackData = "wit_mfs";
        auto b2 = std::make_shared<TgBot::InlineKeyboardButton>();
        b2->text = lang::t("btn_exchange", lng); b2->callbackData = "wit_exc";
        kb->inlineKeyboard = {{b1}, {b2}};
        bot.getApi().editMessageText(lang::t("choose_wit_method", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data == "wit_mfs") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (auto& kv : config::MOBILE_BANKING) {
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = kv.second.emoji + " " + kv.second.name;
            b->callbackData = "wit_mfs_" + kv.first;
            kb->inlineKeyboard.push_back({b});
        }
        bot.getApi().editMessageText(lang::t("wit_mfs_select", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data.substr(0, 8) == "wit_mfs_") {
        std::string method = data.substr(8);
        if (!u_opt) return;
        db::set_state(uid, "awaiting_mfs_wit_amount", method);
        std::map<std::string,std::string> args = {
            {"avail", std::to_string(u_opt->available_bal)},
            {"min",   std::to_string((int)config::MINIMUM_WITHDRAWAL)},
        };
        bot.getApi().sendMessage(uid, lang::t("wit_ask_amount_mfs", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }
    if (data == "wit_exc") {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (auto& kv : config::EXCHANGERS) {
            if (kv.second.our_uid.empty()) continue;
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = kv.second.emoji + " " + kv.second.name;
            b->callbackData = "wit_exc_" + kv.first;
            kb->inlineKeyboard.push_back({b});
        }
        if (kb->inlineKeyboard.empty()) {
            bot.getApi().sendMessage(uid, lang::t("exc_none_configured", lng)); return;
        }
        bot.getApi().editMessageText(lang::t("wit_exc_select", lng), msg->chat->id, msg->messageId, "", "", false, kb);
        return;
    }
    if (data.substr(0, 8) == "wit_exc_") {
        std::string exc_key = data.substr(8);
        if (!u_opt) return;
        double rate = db::withdraw_rate();
        db::set_state(uid, "awaiting_exc_wit_amount", exc_key);
        std::ostringstream ss_avail, ss_usdt_avail;
        ss_avail << std::fixed << std::setprecision(2) << u_opt->available_bal;
        ss_usdt_avail << std::fixed << std::setprecision(4) << (u_opt->available_bal / rate);
        std::map<std::string,std::string> args = {
            {"avail",      ss_avail.str()},
            {"usdt_avail", ss_usdt_avail.str()},
            {"min",        std::to_string(config::MIN_USDT_WITHDRAWAL)},
        };
        bot.getApi().sendMessage(uid, lang::t("wit_ask_amount_exc", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
        return;
    }

    // Play fee selection
    if (data.substr(0, 9) == "play_fee_") {
        double fee = 0;
        try { fee = std::stod(data.substr(9)); } catch (...) { return; }
        if (!u_opt || !u_opt->is_registered) return;
        std::string free_mode = db::get_setting("free_mode");
        if (free_mode == "on") fee = 0.0;
        if (fee > 0 && u_opt->available_bal < fee) {
            bot.getApi().sendMessage(uid, lang::t("insufficient_bal", lng)); return;
        }
        if (db::get_from_queue(uid)) {
            bot.getApi().sendMessage(uid, lang::t("already_in_queue", lng)); return;
        }
        auto opp = db::find_opponent(fee, uid);
        if (opp) {
            long long p2_id = opp->user_id;
            double final_fee = std::min(fee, opp->fee);
            db::remove_from_queue(p2_id);
            try { bot.getApi().deleteMessage(config::LOBBY_CHANNEL_ID, (int)opp->lobby_msg_id); } catch (...) {}
            std::string mid = db::create_match(uid, p2_id, final_fee);
            auto p2 = db::get_user(p2_id);
            std::string p2_lang = p2 ? p2->lang : "en";
            std::string fee_txt = "\n\n💰 <b>Fee: " + std::to_string((int)final_fee) + " TK</b>";
            std::string p2_ign = p2 ? p2->ingame_name : "?";
            std::string p1_ign = u_opt->ingame_name;
            std::map<std::string,std::string> a1 = {{"opp", utils::esc(p2_ign)}};
            std::map<std::string,std::string> a2 = {{"opp", utils::esc(p1_ign)}};
            bot.getApi().sendMessage(uid, lang::t("match_found_p1", lng, a1) + fee_txt, nullptr, nullptr, utils::cancel_kb(lng));
            db::set_state(uid, "awaiting_room_code", mid);
            try {
                bot.getApi().sendMessage(p2_id, lang::t("match_found_p2", p2_lang, a2) + fee_txt, nullptr, nullptr, utils::main_kb(p2_lang));
            } catch (...) {}
            try {
                bot.getApi().editMessageText(lang::t("opponent_found_cb", lng),
                                             msg->chat->id, msg->messageId, "", "", false, nullptr);
            } catch (...) {}
            // Broadcast
            for (auto chat : utils::broadcast_chats()) {
                try {
                    bot.getApi().sendMessage(chat,
                        "⚔️ <b>ম্যাচ শুরু!</b>\n🎮 <b>" + utils::esc(p1_ign) + "</b> vs <b>" +
                        utils::esc(p2_ign) + "</b>\n💰 Fee: <b>" + std::to_string((int)final_fee) + " TK</b>", nullptr, nullptr, nullptr, "HTML");
                } catch (...) {}
            }
        } else {
            // Put in queue + post lobby message
            std::string join_url = "https://t.me/" + config::BOT_USERNAME +
                                   "?start=join_" + std::to_string(uid) + "_" + std::to_string((int)fee);
            auto join_kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = "⚔️ " + std::to_string((int)fee) + " TK ম্যাচে যোগ দাও";
            b->url = join_url;
            join_kb->inlineKeyboard.push_back({b});
            std::string lobby_text =
                "🔍 <b>Opponent চাই!</b>\n\n🎮 Player: <b>" + utils::esc(u_opt->ingame_name) + "</b>\n"
                "💰 Fee: <b>" + std::to_string((int)fee) + " TK</b>\n\n👇 Join করতে নিচের বাটনে ক্লিক করো!";
            long long first_msg_id = 0;
            for (auto chat : utils::broadcast_chats()) {
                try {
                    auto sent = bot.getApi().sendMessage(chat, lobby_text, nullptr, nullptr, join_kb, "HTML");
                    if (!first_msg_id) first_msg_id = sent->messageId;
                } catch (...) {}
            }
            db::add_to_queue(uid, fee, first_msg_id);
            auto cancel_kb_inline = std::make_shared<TgBot::InlineKeyboardMarkup>();
            auto cb = std::make_shared<TgBot::InlineKeyboardButton>();
            cb->text = "❌ Cancel"; cb->callbackData = "cancel_search_" + std::to_string(uid);
            cancel_kb_inline->inlineKeyboard.push_back({cb});
            try {
                bot.getApi().editMessageText(lang::t("searching", lng), msg->chat->id, msg->messageId,
                                             "", "", false, cancel_kb_inline);
            } catch (...) {}
        }
        return;
    }

    // Cancel search
    if (data.substr(0, 14) == "cancel_search_") {
        long long target = 0;
        try { target = std::stoll(data.substr(14)); } catch (...) { return; }
        if (uid != target) { bot.getApi().answerCallbackQuery(q->id, "Not yours.", true); return; }
        auto entry = db::get_from_queue(target);
        if (entry) {
            db::remove_from_queue(target);
            try { bot.getApi().deleteMessage(config::LOBBY_CHANNEL_ID, (int)entry->lobby_msg_id); } catch (...) {}
        }
        try {
            bot.getApi().editMessageText(lang::t("cancelled", lng), msg->chat->id, msg->messageId,
                                         "", "", false, nullptr);
        } catch (...) {}
        return;
    }
}

// ── Setup ──────────────────────────────────────────────────────────────────
void setup(TgBot::Bot& bot) {
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr msg) {
        if (!msg || msg->text.empty()) return;
        if (msg->text[0] == '/') return;
        if (!msg->photo.empty()) return;
        handle_text(msg, bot);
    });
    bot.getEvents().onNonCommandMessage([&bot](TgBot::Message::Ptr msg) {
        if (!msg || msg->photo.empty()) return;
        handle_photo(msg, bot);
    });
    bot.getEvents().onCallbackQuery([&bot](TgBot::CallbackQuery::Ptr q) {
        handle_callback(q, bot);
    });
}

} // namespace handlers
