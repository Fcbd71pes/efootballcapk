// user_cmds.cpp — User command handlers
#include "user_cmds.h"
#include "db.h"
#include "lang.h"
#include "config.h"
#include "utils.h"
#include <sstream>
#include <iomanip>

namespace user_cmds {

static std::string dbl(double v, int d=2) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(d)<<v; return ss.str();
}

void cmd_start(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    std::string username = msg->from ? msg->from->username : "";
    long long ref = 0;
    // Check for referral in text
    std::string text = msg->text;
    auto pos = text.find("ref_");
    if (pos != std::string::npos) {
        try { ref = std::stoll(text.substr(pos + 4)); } catch (...) {}
    }
    auto u_opt = utils::ensure_user(uid, username, ref);
    if (!u_opt) {
        bot.getApi().sendMessage(uid, lang::t("banned", "en"));
        return;
    }
    auto& u = *u_opt;
    std::string lng = u.lang;
    if (u.is_registered) {
        bot.getApi().sendMessage(uid, lang::t("welcome_back", lng), nullptr, nullptr, utils::main_kb(lng));
    } else {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        auto r = std::vector<TgBot::InlineKeyboardButton::Ptr>();
        auto b1 = std::make_shared<TgBot::InlineKeyboardButton>();
        b1->text = "🇧🇩 বাংলা"; b1->callbackData = "setlang_bn";
        auto b2 = std::make_shared<TgBot::InlineKeyboardButton>();
        b2->text = "🇬🇧 English"; b2->callbackData = "setlang_en";
        r.push_back(b1); r.push_back(b2);
        kb->inlineKeyboard.push_back(r);
        bot.getApi().sendMessage(uid, lang::t("choose_lang", "en"), nullptr, nullptr, kb, "HTML");
        db::set_state(uid, "awaiting_ign");
    }
}

void cmd_wallet(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    std::map<std::string,std::string> args = {
        {"avail",  dbl(u->available_bal)},
        {"locked", dbl(u->locked_bal)},
        {"total",  dbl(u->available_bal + u->locked_bal)},
    };
    auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
    auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
    auto d = std::make_shared<TgBot::InlineKeyboardButton>();
    d->text = "➕ Deposit"; d->callbackData = "deposit";
    auto w = std::make_shared<TgBot::InlineKeyboardButton>();
    w->text = "➖ Withdraw"; w->callbackData = "withdraw";
    row.push_back(d); row.push_back(w);
    kb->inlineKeyboard.push_back(row);
    bot.getApi().sendMessage(uid, lang::t("wallet_text", lng, args), nullptr, nullptr, kb, "HTML");
}

void cmd_play(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u || !u->is_registered) {
        bot.getApi().sendMessage(uid, lang::t("register_first", u ? u->lang : "en"));
        return;
    }
    std::string lng = u->lang;
    auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<int> fees1 = {20, 30, 50};
    std::vector<int> fees2 = {100, 200, 500};
    auto make_row = [&](std::vector<int>& fvec) {
        auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
        for (int f : fvec) {
            auto b = std::make_shared<TgBot::InlineKeyboardButton>();
            b->text = std::to_string(f) + " TK";
            b->callbackData = "play_fee_" + std::to_string(f);
            row.push_back(b);
        }
        return row;
    };
    kb->inlineKeyboard.push_back(make_row(fees1));
    kb->inlineKeyboard.push_back(make_row(fees2));
    bot.getApi().sendMessage(uid, lang::t("match_select_fee", lng), nullptr, nullptr, kb, "HTML");
}

void cmd_result(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    auto m = db::get_active_match(uid);
    if (!m) {
        bot.getApi().sendMessage(uid, lang::t("no_active_match", lng));
        return;
    }
    bool submitted = (uid == m->p1_id && !m->p1_screenshot.empty()) ||
                     (uid == m->p2_id && !m->p2_screenshot.empty());
    if (submitted) {
        bot.getApi().sendMessage(uid, lang::t("already_submitted", lng));
        return;
    }
    db::set_state(uid, "awaiting_screenshot", m->match_id);
    std::map<std::string,std::string> args = {{"mid", m->match_id}};
    bot.getApi().sendMessage(uid, lang::t("ss_ask", lng, args), nullptr, nullptr, utils::cancel_kb(lng));
}

void cmd_cancel_match(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    auto m = db::get_active_match(uid);
    if (!m) { bot.getApi().sendMessage(uid, lang::t("no_active_match", lng)); return; }
    std::string mid = m->match_id;
    long long opp_id = (uid == m->p1_id) ? m->p2_id : m->p1_id;
    long long req_by = 0;
    if (db::get_cancel_req_info(mid, req_by)) {
        if (req_by == uid) { bot.getApi().sendMessage(uid, lang::t("cancel_already", lng)); return; }
        db::agree_cancel(mid, uid);
        std::string opp_lang = db::get_user_lang(opp_id);
        try { bot.getApi().sendMessage(opp_id, lang::t("match_cancelled_ok", opp_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, lang::t("match_cancelled_ok", lng), nullptr, nullptr, utils::main_kb(lng));
        return;
    }
    db::create_cancel_req(mid, uid);
    std::string opp_lang = db::get_user_lang(opp_id);
    try { bot.getApi().sendMessage(opp_id, lang::t("cancel_opp_notify", opp_lang)); } catch (...) {}
    bot.getApi().sendMessage(uid, lang::t("cancel_req_sent", lng));
}

void cmd_profile(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    int total = u->wins + u->losses;
    std::map<std::string,std::string> args = {
        {"ign",    utils::esc(u->ingame_name)},
        {"phone",  utils::esc(u->phone)},
        {"avail",  dbl(u->available_bal)},
        {"locked", dbl(u->locked_bal)},
        {"elo",    std::to_string(u->elo)},
        {"wins",   std::to_string(u->wins)},
        {"losses", std::to_string(u->losses)},
        {"joined", u->created_at.substr(0, 10)},
    };
    bot.getApi().sendMessage(uid, lang::t("profile_text", lng, args), nullptr, nullptr, nullptr, "HTML");
}

void cmd_stats(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    int total = u->wins + u->losses;
    double wr = total > 0 ? (u->wins * 100.0 / total) : 0.0;
    std::map<std::string,std::string> args = {
        {"wins",   std::to_string(u->wins)},
        {"losses", std::to_string(u->losses)},
        {"total",  std::to_string(total)},
        {"wr",     dbl(wr, 1)},
        {"elo",    std::to_string(u->elo)},
    };
    bot.getApi().sendMessage(uid, lang::t("stats_text", lng, args));
}

void cmd_leaderboard(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    std::string lng = u ? u->lang : "en";
    auto rows = db::get_top_elo(10);
    std::string text = lang::t("lb_title", lng);
    int i = 1;
    for (auto& r : rows) {
        text += std::to_string(i++) + ". " + utils::esc(r.ingame_name) +
                " — ⭐ " + std::to_string(r.elo) + "\n";
    }
    bot.getApi().sendMessage(uid, text, nullptr, nullptr, nullptr, "HTML");
}

void cmd_share(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    std::string link = "https://t.me/" + config::BOT_USERNAME + "?start=ref_" + std::to_string(uid);
    std::map<std::string,std::string> args = {
        {"link",  link},
        {"bonus", dbl(config::REFERRAL_BONUS, 0)},
    };
    bot.getApi().sendMessage(uid, lang::t("referral_text", lng, args));
}

void cmd_tournaments(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    auto tourneys = db::get_open_tournaments();
    if (tourneys.empty()) { bot.getApi().sendMessage(uid, lang::t("tourney_none", lng)); return; }
    for (auto& tk : tourneys) {
        auto players = db::get_tourney_players(tk.id);
        int joined = (int)players.size();
        bool is_in = false;
        for (auto& [p_uid, status] : players) if (p_uid == uid) is_in = true;
        std::string text =
            "🏆 <b>" + utils::esc(tk.name) + "</b>\n"
            "💰 Entry: " + dbl(tk.entry_fee, 0) + " TK\n"
            "🎁 Prize: " + dbl(tk.prize_pool, 0) + " TK\n"
            "👥 " + std::to_string(joined) + "/" + std::to_string(tk.slots);
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
        auto b = std::make_shared<TgBot::InlineKeyboardButton>();
        if (is_in) {
            b->text = "✅ Joined"; b->callbackData = "none";
        } else if (tk.status == "OPEN" && joined < tk.slots) {
            b->text = "⚔️ Join";
            b->url = "https://t.me/" + config::BOT_USERNAME + "?start=tjoin_" + std::to_string(tk.id);
        } else {
            b->text = "🚫 Full"; b->callbackData = "none";
        }
        row.push_back(b);
        kb->inlineKeyboard.push_back(row);
        bot.getApi().sendMessage(uid, text, nullptr, nullptr, kb, "HTML");
    }
}

void cmd_support(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    std::string text = msg->text;
    // Extract subject after /support
    auto sp = text.find(' ');
    if (sp == std::string::npos) {
        bot.getApi().sendMessage(uid, lang::t("support_help", lng));
        return;
    }
    std::string subject = text.substr(sp + 1);
    int tid = db::create_ticket(uid, subject);
    db::add_ticket_msg(tid, uid, "user", subject);
    std::map<std::string,std::string> args = {{"id", std::to_string(tid)}};
    bot.getApi().sendMessage(uid, lang::t("ticket_opened", lng, args));
    // Notify staff
    std::string notif = "🎫 Ticket #" + std::to_string(tid) + "\n👤 " +
                        utils::esc(u->ingame_name) + " (" + std::to_string(uid) + ")\n📝 " +
                        utils::esc(subject) + "\n\nReply: /treply " + std::to_string(tid) + " <message>";
    for (auto sid : utils::staff_ids()) {
        try { bot.getApi().sendMessage(sid, notif); } catch (...) {}
    }
}

void cmd_language(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
    auto row = std::vector<TgBot::InlineKeyboardButton::Ptr>();
    auto b1 = std::make_shared<TgBot::InlineKeyboardButton>();
    b1->text = "🇧🇩 বাংলা"; b1->callbackData = "setlang_bn";
    auto b2 = std::make_shared<TgBot::InlineKeyboardButton>();
    b2->text = "🇬🇧 English"; b2->callbackData = "setlang_en";
    row.push_back(b1); row.push_back(b2);
    kb->inlineKeyboard.push_back(row);
    bot.getApi().sendMessage(uid, lang::t("choose_lang", "en"), nullptr, nullptr, kb);
}

void cmd_daily(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    time_t t = time(nullptr);
    char buf[16]; strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    std::string today(buf);
    if (db::claim_daily_bonus(uid, 2.0, today)) {
        std::map<std::string,std::string> args = {{"amount", "2"}};
        bot.getApi().sendMessage(uid, lang::t("daily_claimed", lng, args));
    } else {
        bot.getApi().sendMessage(uid, lang::t("daily_already", lng));
    }
}

void cmd_tutorial(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    std::string lng = u ? u->lang : "en";
    bot.getApi().sendMessage(uid, lang::t("tutorial_text", lng), nullptr, nullptr, nullptr, "HTML");
}

void cmd_mytickets(TgBot::Message::Ptr msg, TgBot::Bot& bot) {
    long long uid = msg->chat->id;
    auto u = db::get_user(uid);
    if (!u) return;
    std::string lng = u->lang;
    auto tickets = db::get_user_tickets(uid);
    if (tickets.empty()) { bot.getApi().sendMessage(uid, lang::t("no_tickets", lng)); return; }
    std::string text;
    for (auto& tk : tickets) {
        std::string se = (tk.status == "OPEN") ? "🟢" : "⚫";
        text += se + " #" + std::to_string(tk.id) + ": " +
                utils::esc(tk.subject.substr(0, 40)) + "\n\n";
    }
    bot.getApi().sendMessage(uid, text);
}

void setup(TgBot::Bot& bot) {
    bot.getEvents().onCommand("start",        [&bot](TgBot::Message::Ptr m){ cmd_start(m, bot); });
    bot.getEvents().onCommand("wallet",       [&bot](TgBot::Message::Ptr m){ cmd_wallet(m, bot); });
    bot.getEvents().onCommand("play",         [&bot](TgBot::Message::Ptr m){ cmd_play(m, bot); });
    bot.getEvents().onCommand("result",       [&bot](TgBot::Message::Ptr m){ cmd_result(m, bot); });
    bot.getEvents().onCommand("cancel_match", [&bot](TgBot::Message::Ptr m){ cmd_cancel_match(m, bot); });
    bot.getEvents().onCommand("profile",      [&bot](TgBot::Message::Ptr m){ cmd_profile(m, bot); });
    bot.getEvents().onCommand("stats",        [&bot](TgBot::Message::Ptr m){ cmd_stats(m, bot); });
    bot.getEvents().onCommand("leaderboard",  [&bot](TgBot::Message::Ptr m){ cmd_leaderboard(m, bot); });
    bot.getEvents().onCommand("share",        [&bot](TgBot::Message::Ptr m){ cmd_share(m, bot); });
    bot.getEvents().onCommand("tournaments",  [&bot](TgBot::Message::Ptr m){ cmd_tournaments(m, bot); });
    bot.getEvents().onCommand("support",      [&bot](TgBot::Message::Ptr m){ cmd_support(m, bot); });
    bot.getEvents().onCommand("language",     [&bot](TgBot::Message::Ptr m){ cmd_language(m, bot); });
    bot.getEvents().onCommand("daily",        [&bot](TgBot::Message::Ptr m){ cmd_daily(m, bot); });
    bot.getEvents().onCommand("tutorial",     [&bot](TgBot::Message::Ptr m){ cmd_tutorial(m, bot); });
    bot.getEvents().onCommand("mytickets",    [&bot](TgBot::Message::Ptr m){ cmd_mytickets(m, bot); });
}

} // namespace user_cmds
