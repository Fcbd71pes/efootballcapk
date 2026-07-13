// lang.cpp — Bilingual language strings
#include "lang.h"
#include <sstream>
#include <iomanip>

namespace lang {

std::unordered_map<std::string, LangString> STRINGS = {
    {"choose_lang",         {"🌐 ভাষা বেছে নিন:", "🌐 Choose your language:"}},
    {"ask_ign",             {"স্বাগতম! আপনার eFootball ইন-গেম নাম (IGN) পাঠান:", "Welcome! Please send your eFootball in-game name (IGN):"}},
    {"ask_phone",           {"✅ এখন আপনার ফোন নম্বর পাঠান:", "✅ Now send your phone number:"}},
    {"reg_done",            {"✅ রেজিস্ট্রেশন সম্পন্ন! 🎉", "✅ Registration complete! 🎉"}},
    {"reg_done_bonus",      {"✅ রেজিস্ট্রেশন সম্পন্ন! আপনি 10 TK বোনাস পেয়েছেন।", "✅ Registration complete! You got a 10 TK welcome bonus."}},
    {"welcome_back",        {"👋 আবার স্বাগতম!", "👋 Welcome back!"}},
    {"banned",              {"❌ আপনার একাউন্ট ব্যান করা হয়েছে।", "❌ Your account has been banned."}},
    {"join_channel_msg",    {"ব্যবহার করতে চ্যানেলে যোগ দিন।", "Please join our channel to continue."}},
    {"join_channel_btn",    {"চ্যানেলে যোগ দিন", "Join Channel"}},
    {"lang_set",            {"✅ ভাষা সেট হয়েছে। আপনার eFootball ইন-গেম নাম (IGN) পাঠান:", "✅ Language updated. Please send your eFootball in-game name (IGN):"}},
    {"register_first",      {"❌ /start করে রেজিস্ট্রেশন করুন।", "❌ Please /start and register first."}},
    {"cancelled",           {"❌ বাতিল হয়েছে।", "❌ Cancelled."}},
    {"no_permission",       {"❌ আপনার এই কাজ করার অনুমতি নেই।", "❌ You don't have permission to do this."}},
    {"already_in_queue",    {"⏳ আপনি ইতিমধ্যে কাউকে খুঁজছেন।", "⏳ You are already searching for an opponent."}},
    {"searching",           {"🔍 প্রতিপক্ষ খোঁজা হচ্ছে...", "🔍 Searching for opponent..."}},
    {"match_found_p1",      {"✅ ম্যাচ পাওয়া গেছে!\n🎮 প্রতিপক্ষ: <b>{opp}</b>\n\nRoom Code পাঠাও:", "✅ Match found!\n🎮 Opponent: <b>{opp}</b>\n\nSend your Room Code:"}},
    {"match_found_p2",      {"✅ ম্যাচ পাওয়া গেছে!\n🎮 প্রতিপক্ষ: <b>{opp}</b>", "✅ Match found!\n🎮 Opponent: <b>{opp}</b>"}},
    {"opponent_found_cb",   {"✅ প্রতিপক্ষ পাওয়া গেছে! চ্যাটে দেখুন।", "✅ Opponent found! Check your chat."}},
    {"no_active_match",     {"❌ আপনার কোনো সক্রিয় ম্যাচ নেই।", "❌ You have no active match."}},
    {"match_not_active",    {"❌ ম্যাচটি সক্রিয় নেই।", "❌ Match is not active."}},
    {"already_submitted",   {"✅ আপনি ইতিমধ্যে স্ক্রিনশট জমা দিয়েছেন।", "✅ You have already submitted a screenshot."}},
    {"ss_ask",              {"📸 ম্যাচ #{mid} এর স্ক্রিনশট পাঠান:", "📸 Send screenshot for Match #{mid}:"}},
    {"ss_received",         {"✅ স্ক্রিনশট পেয়েছি! ম্যানেজার যাচাই করবেন।", "✅ Screenshot received! A manager will verify."}},
    {"opp_submitted",       {"📸 প্রতিপক্ষ স্ক্রিনশট জমা দিয়েছে।", "📸 Your opponent has submitted their screenshot."}},
    {"match_won",           {"🏆 ম্যাচ জিতেছেন! +{prize:.0f} TK ✅\nMatch #{mid}", "🏆 You won the match! +{prize:.0f} TK ✅\nMatch #{mid}"}},
    {"match_lost",          {"❌ ম্যাচ হেরেছেন। আবার চেষ্টা করুন!\nMatch #{mid}", "❌ You lost the match. Try again!\nMatch #{mid}"}},
    {"cancel_req_sent",     {"⏳ Cancel অনুরোধ পাঠানো হয়েছে। প্রতিপক্ষ সম্মত হলে Cancel হবে।", "⏳ Cancel request sent. Match will cancel if opponent agrees."}},
    {"cancel_already",      {"⏳ আপনি আগেই cancel অনুরোধ করেছেন।", "⏳ You already requested a cancel."}},
    {"cancel_opp_notify",   {"⚠️ প্রতিপক্ষ ম্যাচ cancel করতে চায়। /cancel_match দিয়ে সম্মত হন।", "⚠️ Opponent wants to cancel. Use /cancel_match to agree."}},
    {"match_cancelled_ok",  {"✅ ম্যাচ cancel হয়েছে। টাকা ফেরত দেওয়া হয়েছে।", "✅ Match cancelled. Your fee has been refunded."}},
    {"room_code_fwd",       {"🔑 Room Code: <code>{code}</code>\n⏰ {mins} মিনিটের মধ্যে স্ক্রিনশট জমা দিন।", "🔑 Room Code: <code>{code}</code>\n⏰ Submit screenshot within {mins} minutes."}},
    {"room_code_confirm",   {"✅ Room Code পাঠানো হয়েছে! ⏰ {mins} মিনিটের মধ্যে স্ক্রিনশট দিন।", "✅ Room Code sent! ⏰ Submit screenshot within {mins} minutes."}},
    {"match_warning",       {"⚠️ সতর্কতা! {left} মিনিটের মধ্যে স্ক্রিনশট দিন নইলে auto-lose হবে।", "⚠️ Warning! Submit screenshot within {left} minutes or you will lose."}},
    {"match_timeout_lose",  {"❌ সময় শেষ! স্ক্রিনশট না দেওয়ায় আপনি হেরেছেন।", "❌ Time up! You lost due to not submitting a screenshot."}},
    {"wallet_text",         {"💰 আপনার ওয়ালেট\n\n💵 উপলব্ধ: {avail:.2f} TK\n🔒 লক: {locked:.2f} TK\n📊 মোট: {total:.2f} TK", "💰 Your Wallet\n\n💵 Available: {avail:.2f} TK\n🔒 Locked: {locked:.2f} TK\n📊 Total: {total:.2f} TK"}},
    {"profile_text",        {"📋 প্রোফাইল\n\n🎮 IGN: <b>{ign}</b>\n📱 Phone: <code>{phone}</code>\n💵 Balance: {avail:.2f} TK\n⭐ ELO: {elo}\n🏆 Win: {wins} | ❌ Loss: {losses}\n📅 Joined: {joined}", "📋 Profile\n\n🎮 IGN: <b>{ign}</b>\n📱 Phone: <code>{phone}</code>\n💵 Balance: {avail:.2f} TK\n⭐ ELO: {elo}\n🏆 Win: {wins} | ❌ Loss: {losses}\n📅 Joined: {joined}"}},
    {"stats_text",          {"📊 স্ট্যাটস\n\n🏆 জয়: {wins}\n❌ পরাজয়: {losses}\n🎮 মোট: {total}\n📈 Win Rate: {wr:.1f}%\n⭐ ELO: {elo}", "📊 Stats\n\n🏆 Wins: {wins}\n❌ Losses: {losses}\n🎮 Total: {total}\n📈 Win Rate: {wr:.1f}%\n⭐ ELO: {elo}"}},
    {"lb_title",            {"🏆 শীর্ষ খেলোয়াড়:\n\n", "🏆 Top Players:\n\n"}},
    {"no_history",          {"📭 কোনো ম্যাচ ইতিহাস নেই।", "📭 No match history found."}},
    {"history_title",       {"📜 আপনার শেষ ম্যাচগুলো:\n\n", "📜 Your recent matches:\n\n"}},
    {"match_select_fee",    {"💰 ম্যাচ ফি বেছে নিন:", "💰 Select match fee:"}},
    {"referral_text",       {"🔗 রেফারেল লিংক:\n{link}\n\n💰 প্রতি সফল রেফারেলে: {bonus:.0f} TK", "🔗 Your referral link:\n{link}\n\n💰 Bonus per referral: {bonus:.0f} TK"}},
    {"tourney_none",        {"❌ কোনো tournament খোলা নেই।", "❌ No tournaments are open."}},
    {"support_help",        {"📞 /support <বিষয়> দিয়ে ticket খুলুন।", "📞 Use /support <subject> to open a ticket."}},
    {"ticket_opened",       {"✅ Ticket #{id} খোলা হয়েছে।", "✅ Ticket #{id} opened."}},
    {"no_tickets",          {"📭 কোনো ticket নেই।", "📭 No tickets found."}},
    {"ticket_sent",         {"✅ পাঠানো হয়েছে।", "✅ Sent."}},
    {"ticket_closed",       {"✅ Ticket #{id} বন্ধ করা হয়েছে।", "✅ Ticket #{id} closed."}},
    {"ticket_reply_recv",   {"💬 Ticket #{id} এ Admin উত্তর:\n{msg}", "💬 Admin replied on Ticket #{id}:\n{msg}"}},
    {"no_permission",       {"❌ অনুমতি নেই।", "❌ No permission."}},
    {"insufficient_bal",    {"❌ পর্যাপ্ত ব্যালেন্স নেই।", "❌ Insufficient balance."}},
    {"invalid_number",      {"❌ সংখ্যা দিন।", "❌ Please enter a valid number."}},
    {"daily_already",       {"✅ আজ ইতিমধ্যে Daily Bonus নিয়েছেন।", "✅ You already claimed your daily bonus today."}},
    {"daily_claimed",       {"🎁 {amount} TK Daily Bonus পেয়েছেন!", "🎁 You claimed {amount} TK daily bonus!"}},
    {"mfs_dep_inst",        {"📱 <b>{name}</b> নম্বরে পাঠান: <code>{number}</code>\n\nTxID ও Amount পাঠান (ফরম্যাট: TxID Amount):", "📱 Send to <b>{name}</b>: <code>{number}</code>\n\nSend TxID and Amount (format: TxID Amount):"}},
    {"mfs_wrong_fmt",       {"❌ ভুল ফরম্যাট! TxID Amount দিন (উদাহরণ: ABC123 200)", "❌ Wrong format! Send TxID Amount (e.g., ABC123 200)"}},
    {"mfs_send_ss",         {"📸 এখন পেমেন্ট স্ক্রিনশট পাঠান:", "📸 Now send payment screenshot:"}},
    {"mfs_min_dep",         {"❌ ন্যূনতম ডিপোজিট {min:.0f} TK।", "❌ Minimum deposit is {min:.0f} TK."}},
    {"dep_submitted",       {"✅ ডিপোজিট অনুরোধ জমা! Admin যাচাই করবেন।", "✅ Deposit submitted! Admin will verify."}},
    {"dep_approved",        {"✅ ডিপোজিট অনুমোদিত! +{amount:.2f} TK যোগ হয়েছে।", "✅ Deposit approved! +{amount:.2f} TK added."}},
    {"dep_rejected",        {"❌ ডিপোজিট প্রত্যাখ্যাত।", "❌ Deposit rejected."}},
    {"wit_min",             {"❌ ন্যূনতম উইথড্র {min:.0f} TK।", "❌ Minimum withdrawal is {min:.0f} TK."}},
    {"wit_ask_amount_mfs",  {"💸 উইথড্র পরিমাণ লিখুন (ন্যূনতম {min:.0f} TK, উপলব্ধ: {avail:.2f} TK):", "💸 Enter withdrawal amount (min {min:.0f} TK, available: {avail:.2f} TK):"}},
    {"wit_ask_account",     {"📱 {method} নম্বর লিখুন:", "📱 Enter your {method} number:"}},
    {"wit_submitted",       {"✅ {amount} উইথড্র অনুরোধ জমা! Admin প্রক্রিয়া করবেন।", "✅ Withdrawal of {amount} submitted! Admin will process."}},
    {"wit_approved",        {"✅ উইথড্র অনুমোদিত! {amount:.2f} TK পাঠানো হয়েছে।", "✅ Withdrawal approved! {amount:.2f} TK sent."}},
    {"wit_rejected",        {"❌ উইথড্র প্রত্যাখ্যাত। ব্যালেন্স ফেরত দেওয়া হয়েছে।", "❌ Withdrawal rejected. Balance refunded."}},
    {"choose_dep_method",   {"💰 ডিপোজিট পদ্ধতি বেছে নিন:", "💰 Choose deposit method:"}},
    {"choose_wit_method",   {"💸 উইথড্র পদ্ধতি বেছে নিন:", "💸 Choose withdrawal method:"}},
    {"mfs_select",          {"📱 MFS পদ্ধতি বেছে নিন:", "📱 Select MFS method:"}},
    {"wit_mfs_select",      {"📱 MFS উইথড্র পদ্ধতি বেছে নিন:", "📱 Select MFS withdrawal method:"}},
    {"exc_select",          {"🏦 Exchange বেছে নিন:", "🏦 Select Exchange:"}},
    {"wit_exc_select",      {"🏦 Exchange উইথড্র বেছে নিন:", "🏦 Select Exchange for withdrawal:"}},
    {"exc_dep_show_uid",    {"🏦 <b>{name}</b>\n{uid_label}: <code>{our_uid}</code>\n\n{note}\n\nন্যূনতম: {min_dep:.2f} USDT\n\nএখন USDT পরিমাণ লিখুন:", "🏦 <b>{name}</b>\n{uid_label}: <code>{our_uid}</code>\n\n{note}\n\nMinimum: {min_dep:.2f} USDT\n\nEnter USDT amount:"}},
    {"exc_ask_uid",         {"{uid_label} লিখুন:", "Enter your {uid_label}:"}},
    {"exc_ask_ss",          {"📸 পেমেন্ট স্ক্রিনশট পাঠান:", "📸 Send payment screenshot:"}},
    {"exc_dep_submitted",   {"✅ {name} ডিপোজিট জমা!\n💵 {usdt:.4f} USDT = {bdt:.2f} TK\nUID: {user_uid}", "✅ {name} deposit submitted!\n💵 {usdt:.4f} USDT = {bdt:.2f} TK\nUID: {user_uid}"}},
    {"exc_min_usdt",        {"❌ ন্যূনতম {min:.2f} USDT।", "❌ Minimum {min:.2f} USDT."}},
    {"exc_uid_not_set",     {"❌ এই Exchange এর UID সেট নেই।", "❌ This exchanger's UID is not configured."}},
    {"exc_none_configured", {"❌ কোনো Exchange configure করা নেই।", "❌ No exchangers configured."}},
    {"wit_min_usdt",        {"❌ ন্যূনতম {min:.2f} USDT।", "❌ Minimum {min:.2f} USDT."}},
    {"wit_ask_amount_exc",  {"💸 USDT পরিমাণ লিখুন (উপলব্ধ: {avail:.2f} TK / {usdt_avail:.4f} USDT, ন্যূনতম: {min:.2f} USDT):", "💸 Enter USDT amount (available: {avail:.2f} TK / {usdt_avail:.4f} USDT, min: {min:.2f} USDT):"}},
    {"tutorial_text",       {"📖 <b>কীভাবে খেলবেন:</b>\n1. /start দিয়ে রেজিস্টার করুন\n2. ব্যালেন্স যোগ করুন\n3. Play 1v1 চাপুন\n4. ফি বেছে নিন\n5. Room Code দিয়ে খেলুন\n6. স্ক্রিনশট জমা দিন", "📖 <b>How to Play:</b>\n1. Register with /start\n2. Add balance\n3. Press Play 1v1\n4. Select fee\n5. Play with Room Code\n6. Submit screenshot"}},
    {"btn_play",            {"🎮 Play 1v1", "🎮 Play 1v1"}},
    {"btn_wallet",          {"💰 Wallet", "💰 Wallet"}},
    {"btn_profile",         {"📋 Profile", "📋 Profile"}},
    {"btn_tourney",         {"⚔️ Tournaments", "⚔️ Tournaments"}},
    {"btn_lb",              {"🏆 Leaderboard", "🏆 Leaderboard"}},
    {"btn_share",           {"🔗 Share & Earn", "🔗 Share & Earn"}},
    {"btn_rules",           {"📜 Rules", "📜 Rules"}},
    {"btn_lang",            {"🌐 Language", "🌐 Language"}},
    {"btn_result",          {"📸 Result", "📸 Result"}},
    {"btn_cancel",          {"❌ Cancel", "❌ Cancel"}},
    {"btn_support",         {"📞 Support", "📞 Support"}},
    {"btn_daily",           {"🎁 Daily Bonus", "🎁 Daily Bonus"}},
    {"btn_tutorial",        {"📖 How to Play", "📖 How to Play"}},
    {"btn_mfs",             {"📱 Mobile Banking", "📱 Mobile Banking"}},
    {"btn_exchange",        {"🏦 Exchange", "🏦 Exchange"}},
};

std::string t(const std::string& key, const std::string& language,
              const std::map<std::string, std::string>& kwargs) {
    std::string lang_code = (language == "bn") ? "bn" : "en";
    auto it = STRINGS.find(key);
    std::string text;
    if (it != STRINGS.end()) {
        text = (lang_code == "bn") ? it->second.bn : it->second.en;
    } else {
        text = "[" + key + "]";
    }
    for (const auto& kv : kwargs) {
        std::vector<std::string> phs = {
            "{" + kv.first + "}",
            "{" + kv.first + ":.2f}",
            "{" + kv.first + ":.4f}",
            "{" + kv.first + ":.0f}",
            "{" + kv.first + ":.1f}",
        };
        for (const auto& ph : phs) {
            size_t pos = 0;
            while ((pos = text.find(ph, pos)) != std::string::npos) {
                text.replace(pos, ph.length(), kv.second);
                pos += kv.second.length();
            }
        }
    }
    return text;
}

std::string fmt(double val, int decimals) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << val;
    return ss.str();
}

} // namespace lang
