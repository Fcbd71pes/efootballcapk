// admin_cmds.cpp — Admin & Manager command handlers
#include "admin_cmds.h"
#include "db.h"
#include "lang.h"
#include "config.h"
#include "utils.h"
#include <sstream>
#include <iomanip>

namespace admin_cmds {

static std::string dbl(double v, int d=2) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(d)<<v; return ss.str();
}

bool is_admin(long long uid) {
    if (uid == config::SUPER_ADMIN) return true;
    for (auto a : db::get_admins()) if (a == uid) return true;
    return false;
}
bool is_manager(long long uid) {
    return is_admin(uid) || [&]{ for (auto m : db::get_managers()) if (m == uid) return true; return false; }();
}

static void require_admin(long long uid, const std::string& lng, TgBot::Bot& bot,
                          std::function<void()> fn) {
    if (!is_admin(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
    fn();
}
static void require_manager(long long uid, const std::string& lng, TgBot::Bot& bot,
                             std::function<void()> fn) {
    if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
    fn();
}

void setup(TgBot::Bot& bot) {

    // /report — Daily report
    bot.getEvents().onCommand("report", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_admin(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        auto r = db::get_daily_report();
        std::string text =
            "📊 <b>Daily Report — " + r.date + "</b>\n\n"
            "👥 Users: " + std::to_string(r.total_users) + " (+"+std::to_string(r.new_users)+" today)\n"
            "⚔️ Matches: " + std::to_string(r.matches) + " ("+std::to_string(r.completed)+" done)\n"
            "💰 Match Fees: " + dbl(r.fees) + " TK\n\n"
            "📥 MFS Dep: " + std::to_string(r.mfs_dep_count) + " | " + dbl(r.mfs_dep_amount) + " TK\n"
            "🏦 Exc Dep: " + std::to_string(r.exc_dep_count) + " | " + dbl(r.exc_dep_usdt,4) + " USDT | " + dbl(r.exc_dep_tk) + " TK\n"
            "📤 MFS Wit: " + dbl(r.mfs_wit_amount) + " TK\n"
            "🏦 Exc Wit: " + dbl(r.exc_wit_usdt,4) + " USDT\n\n"
            "⏳ Pending:\n"
            "  MFS Dep: " + std::to_string(r.pending_mfs_dep) + "\n"
            "  Exc Dep: " + std::to_string(r.pending_exc_dep) + "\n"
            "  MFS Wit: " + std::to_string(r.pending_mfs_wit) + "\n"
            "  Exc Wit: " + std::to_string(r.pending_exc_wit) + "\n\n"
            "💱 Dep Rate: " + r.dep_rate + " | Wit Rate: " + r.wit_rate;
        bot.getApi().sendMessage(uid, text, nullptr, nullptr, nullptr, "HTML");
    });

    // /ban_user <uid>
    bot.getEvents().onCommand("ban_user", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_admin(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        std::string text = msg->text;
        auto sp = text.find(' ');
        if (sp == std::string::npos) { bot.getApi().sendMessage(uid, "/ban_user <uid>"); return; }
        try {
            long long target = std::stoll(text.substr(sp+1));
            db::update_user_int(target, "is_banned", 1);
            bot.getApi().sendMessage(uid, "✅ User " + std::to_string(target) + " banned.");
        } catch (...) { bot.getApi().sendMessage(uid, "❌ Invalid UID."); }
    });

    // /unban_user <uid>
    bot.getEvents().onCommand("unban_user", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_admin(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        std::string text = msg->text;
        auto sp = text.find(' ');
        if (sp == std::string::npos) { bot.getApi().sendMessage(uid, "/unban_user <uid>"); return; }
        try {
            long long target = std::stoll(text.substr(sp+1));
            db::update_user_int(target, "is_banned", 0);
            bot.getApi().sendMessage(uid, "✅ User " + std::to_string(target) + " unbanned.");
        } catch (...) { bot.getApi().sendMessage(uid, "❌ Invalid UID."); }
    });

    // /add_balance <uid> <amount>
    bot.getEvents().onCommand("add_balance", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_admin(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        std::istringstream iss(msg->text);
        std::string cmd; long long target; double amount;
        if (!(iss >> cmd >> target >> amount)) {
            bot.getApi().sendMessage(uid, "/add_balance <uid> <amount>"); return;
        }
        db::adjust_balance(target, amount);
        db::record_transaction(target, amount, "admin_adjust", "Admin balance add by " + std::to_string(uid));
        bot.getApi().sendMessage(uid, "✅ Added " + dbl(amount) + " TK to user " + std::to_string(target));
    });

    // /resolve <match_id> <winner_id>
    bot.getEvents().onCommand("resolve", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        std::istringstream iss(msg->text);
        std::string cmd, mid; long long winner_id;
        if (!(iss >> cmd >> mid >> winner_id)) {
            bot.getApi().sendMessage(uid, "/resolve <match_id> <winner_id>"); return;
        }
        auto m = db::resolve_match(mid, winner_id, uid);
        if (m.match_id.empty()) {
            bot.getApi().sendMessage(uid, "❌ Match not found."); return;
        }
        long long loser_id = (winner_id == m.p1_id) ? m.p2_id : m.p1_id;
        double prize = m.fee * 1.8;
        std::string w_lang = db::get_user_lang(winner_id);
        std::string l_lang = db::get_user_lang(loser_id);
        std::map<std::string,std::string> wa = {{"mid", mid}, {"prize", dbl(prize,0)}};
        std::map<std::string,std::string> la = {{"mid", mid}};
        try { bot.getApi().sendMessage(winner_id, lang::t("match_won", w_lang, wa), nullptr, nullptr, nullptr, "HTML"); } catch (...) {}
        try { bot.getApi().sendMessage(loser_id, lang::t("match_lost", l_lang, la), nullptr, nullptr, nullptr, "HTML"); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Match " + mid + " resolved. Winner: " + std::to_string(winner_id));
    });

    // /cancel_m <match_id>
    bot.getEvents().onCommand("cancel_m", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        std::string text = msg->text;
        auto sp = text.find(' ');
        if (sp == std::string::npos) { bot.getApi().sendMessage(uid, "/cancel_m <match_id>"); return; }
        std::string mid = text.substr(sp+1);
        auto m = db::get_match(mid);
        if (!m) { bot.getApi().sendMessage(uid, "❌ Match not found."); return; }
        db::cancel_match_refund(mid);
        std::string w_lang = db::get_user_lang(m->p1_id);
        std::string l_lang = db::get_user_lang(m->p2_id);
        try { bot.getApi().sendMessage(m->p1_id, lang::t("match_cancelled_ok", w_lang)); } catch (...) {}
        try { bot.getApi().sendMessage(m->p2_id, lang::t("match_cancelled_ok", l_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Match " + mid + " cancelled with refund.");
    });

    // /dep_approve_mfs <id>
    bot.getEvents().onCommand("dep_approve_mfs", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int dep_id = std::stoi(msg->text.substr(sp+1));
        auto d = db::approve_mfs_deposit(dep_id, admin_name);
        if (!d) { bot.getApi().sendMessage(uid, "❌ Not found or already processed."); return; }
        std::string u_lang = db::get_user_lang(d->user_id);
        std::map<std::string,std::string> args = {{"amount", dbl(d->amount)}};
        try { bot.getApi().sendMessage(d->user_id, lang::t("dep_approved", u_lang, args)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ MFS Deposit #" + std::to_string(dep_id) + " approved.");
    });

    // /dep_reject_mfs <id>
    bot.getEvents().onCommand("dep_reject_mfs", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int dep_id = std::stoi(msg->text.substr(sp+1));
        auto d = db::reject_mfs_deposit(dep_id, admin_name);
        if (!d) { bot.getApi().sendMessage(uid, "❌ Not found or already processed."); return; }
        std::string u_lang = db::get_user_lang(d->user_id);
        try { bot.getApi().sendMessage(d->user_id, lang::t("dep_rejected", u_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ MFS Deposit #" + std::to_string(dep_id) + " rejected.");
    });

    // /dep_approve_exc <id>
    bot.getEvents().onCommand("dep_approve_exc", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int dep_id = std::stoi(msg->text.substr(sp+1));
        auto d = db::approve_exc_deposit(dep_id, admin_name);
        if (!d) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(d->user_id);
        std::map<std::string,std::string> args = {{"amount", dbl(d->amount_tk)}};
        try { bot.getApi().sendMessage(d->user_id, lang::t("dep_approved", u_lang, args)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Exchange Deposit #" + std::to_string(dep_id) + " approved.");
    });

    // /dep_reject_exc <id>
    bot.getEvents().onCommand("dep_reject_exc", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        std::string lng = db::get_user_lang(uid);
        if (!is_manager(uid)) { bot.getApi().sendMessage(uid, lang::t("no_permission", lng)); return; }
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int dep_id = std::stoi(msg->text.substr(sp+1));
        auto d = db::reject_exc_deposit(dep_id, admin_name);
        if (!d) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(d->user_id);
        try { bot.getApi().sendMessage(d->user_id, lang::t("dep_rejected", u_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Exchange Deposit #" + std::to_string(dep_id) + " rejected.");
    });

    // /wit_approve_mfs <id>
    bot.getEvents().onCommand("wit_approve_mfs", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_manager(uid)) return;
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int wid = std::stoi(msg->text.substr(sp+1));
        auto w = db::approve_mfs_withdrawal(wid, admin_name);
        if (!w) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(w->user_id);
        std::map<std::string,std::string> args = {{"amount", dbl(w->amount)}};
        try { bot.getApi().sendMessage(w->user_id, lang::t("wit_approved", u_lang, args)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ MFS Withdrawal #" + std::to_string(wid) + " approved.");
    });

    // /wit_reject_mfs <id>
    bot.getEvents().onCommand("wit_reject_mfs", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_manager(uid)) return;
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int wid = std::stoi(msg->text.substr(sp+1));
        auto w = db::reject_mfs_withdrawal(wid, admin_name);
        if (!w) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(w->user_id);
        try { bot.getApi().sendMessage(w->user_id, lang::t("wit_rejected", u_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ MFS Withdrawal #" + std::to_string(wid) + " rejected.");
    });

    // /ewit_approve <id>
    bot.getEvents().onCommand("ewit_approve", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_manager(uid)) return;
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int wid = std::stoi(msg->text.substr(sp+1));
        auto w = db::approve_exc_withdrawal(wid, admin_name);
        if (!w) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(w->user_id);
        std::map<std::string,std::string> args = {{"amount", dbl(w->amount_usdt,4) + " USDT"}};
        try { bot.getApi().sendMessage(w->user_id, lang::t("wit_approved", u_lang, args)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Exc Withdrawal #" + std::to_string(wid) + " approved.");
    });

    // /ewit_reject <id>
    bot.getEvents().onCommand("ewit_reject", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_manager(uid)) return;
        auto u = db::get_user(uid);
        std::string admin_name = u ? u->ingame_name : std::to_string(uid);
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        int wid = std::stoi(msg->text.substr(sp+1));
        auto w = db::reject_exc_withdrawal(wid, admin_name);
        if (!w) { bot.getApi().sendMessage(uid, "❌ Not found."); return; }
        std::string u_lang = db::get_user_lang(w->user_id);
        try { bot.getApi().sendMessage(w->user_id, lang::t("wit_rejected", u_lang)); } catch (...) {}
        bot.getApi().sendMessage(uid, "✅ Exc Withdrawal #" + std::to_string(wid) + " rejected.");
    });

    // /set_rate dep/wit <value>
    bot.getEvents().onCommand("set_rate", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        std::istringstream iss(msg->text);
        std::string cmd, type; double val;
        if (!(iss >> cmd >> type >> val)) {
            bot.getApi().sendMessage(uid, "/set_rate dep|wit <value>"); return;
        }
        if (type == "dep") db::set_setting("usdt_deposit_rate", std::to_string(val));
        else if (type == "wit") db::set_setting("usdt_withdraw_rate", std::to_string(val));
        else { bot.getApi().sendMessage(uid, "Use 'dep' or 'wit'."); return; }
        bot.getApi().sendMessage(uid, "✅ Rate updated: " + type + " = " + dbl(val, 2));
    });

    // /backup
    bot.getEvents().onCommand("backup", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        std::string dest = "backup_" + std::to_string(time(nullptr)) + ".db";
        if (db::safe_backup(dest)) {
            bot.getApi().sendDocument(uid, "attach://" + dest);
        } else {
            bot.getApi().sendMessage(uid, "❌ Backup failed.");
        }
    });

    // /add_manager <uid>
    bot.getEvents().onCommand("add_manager", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) { bot.getApi().sendMessage(uid, "/add_manager <uid>"); return; }
        try {
            long long target = std::stoll(msg->text.substr(sp+1));
            db::add_manager(target, uid);
            bot.getApi().sendMessage(uid, "✅ Manager added: " + std::to_string(target));
        } catch (...) {}
    });

    // /remove_manager <uid>
    bot.getEvents().onCommand("remove_manager", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        try {
            long long target = std::stoll(msg->text.substr(sp+1));
            db::remove_manager(target);
            bot.getApi().sendMessage(uid, "✅ Manager removed: " + std::to_string(target));
        } catch (...) {}
    });

    // /set_rules <text>
    bot.getEvents().onCommand("set_rules", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) { bot.getApi().sendMessage(uid, "/set_rules <text>"); return; }
        db::set_setting("rules_text", msg->text.substr(sp+1));
        bot.getApi().sendMessage(uid, "✅ Rules updated.");
    });

    // /free_mode on|off
    bot.getEvents().onCommand("free_mode", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        std::string val = msg->text.substr(sp+1);
        db::set_setting("free_mode", val);
        bot.getApi().sendMessage(uid, "✅ Free mode: " + val);
    });

    // /broadcast <message>
    bot.getEvents().onCommand("broadcast", [&bot](TgBot::Message::Ptr msg) {
        long long uid = msg->chat->id;
        if (!is_admin(uid)) return;
        auto sp = msg->text.find(' ');
        if (sp == std::string::npos) return;
        std::string text = msg->text.substr(sp+1);
        for (auto chat : utils::broadcast_chats()) {
            try { bot.getApi().sendMessage(chat, text, nullptr, nullptr, nullptr, "HTML"); } catch (...) {}
        }
        bot.getApi().sendMessage(uid, "✅ Broadcast sent.");
    });
}

} // namespace admin_cmds
