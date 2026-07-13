// db.cpp — eFootball Bot Database Layer (Full Implementation)
#include "db.h"
#include "config.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <iomanip>

namespace db {

sqlite3* db_conn = nullptr;
std::map<long long, QueueEntry> MATCH_QUEUE;
std::mutex QUEUE_LOCK;

static std::string q_esc(const std::string& s) {
    std::string out;
    out += "'";
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    out += "'";
    return out;
}

static std::string today_str() {
    time_t t = time(nullptr);
    char buf[16];
    struct tm* ti = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d", ti);
    return std::string(buf);
}

static std::string gen_match_id() {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, (int)(sizeof(chars) - 2));
    std::string id;
    for (int i = 0; i < 8; i++) id += chars[dis(gen)];
    return id;
}

static double scalar_double(const std::string& sql) {
    sqlite3_stmt* stmt;
    double val = 0.0;
    if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        if (sqlite3_step(stmt) == SQLITE_ROW)
            val = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    return val;
}

static int scalar_int(const std::string& sql) {
    sqlite3_stmt* stmt;
    int val = 0;
    if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        if (sqlite3_step(stmt) == SQLITE_ROW)
            val = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return val;
}

static std::string scalar_str(const std::string& sql) {
    sqlite3_stmt* stmt;
    std::string val;
    if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* c = (const char*)sqlite3_column_text(stmt, 0);
            if (c) val = c;
        }
    }
    sqlite3_finalize(stmt);
    return val;
}

static std::string col_str(sqlite3_stmt* stmt, int col) {
    const char* c = (const char*)sqlite3_column_text(stmt, col);
    return c ? std::string(c) : "";
}

static User row_to_user(sqlite3_stmt* stmt) {
    User u;
    u.user_id = sqlite3_column_int64(stmt, 0);
    u.username = col_str(stmt, 1);
    u.ingame_name = col_str(stmt, 2);
    u.phone = col_str(stmt, 3);
    u.lang = col_str(stmt, 4);
    if (u.lang.empty()) u.lang = "en";
    u.available_bal = sqlite3_column_double(stmt, 5);
    u.locked_bal = sqlite3_column_double(stmt, 6);
    u.elo = sqlite3_column_int(stmt, 7);
    u.wins = sqlite3_column_int(stmt, 8);
    u.losses = sqlite3_column_int(stmt, 9);
    u.is_registered = sqlite3_column_int(stmt, 10);
    u.is_banned = sqlite3_column_int(stmt, 11);
    u.welcome_given = sqlite3_column_int(stmt, 12);
    u.referrer_id = sqlite3_column_int64(stmt, 13);
    u.state = col_str(stmt, 14);
    u.state_data = col_str(stmt, 15);
    u.last_daily = col_str(stmt, 16);
    u.total_refs = sqlite3_column_int(stmt, 17);
    u.created_at = col_str(stmt, 18);
    return u;
}

static Match row_to_match(sqlite3_stmt* stmt) {
    Match m;
    m.match_id = col_str(stmt, 0);
    m.p1_id = sqlite3_column_int64(stmt, 1);
    m.p2_id = sqlite3_column_int64(stmt, 2);
    m.fee = sqlite3_column_double(stmt, 3);
    m.status = col_str(stmt, 4);
    m.p1_screenshot = col_str(stmt, 5);
    m.p2_screenshot = col_str(stmt, 6);
    m.winner_id = sqlite3_column_int64(stmt, 7);
    m.verified_by = sqlite3_column_int64(stmt, 8);
    m.tourney_id = sqlite3_column_int(stmt, 9);
    m.started_at = col_str(stmt, 10);
    m.created_at = col_str(stmt, 11);
    return m;
}

bool execute_query(const std::string& query) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_conn, query.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (err ? err : "unknown") << std::endl;
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool init_db(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_conn);
    if (rc) { std::cerr << "Cannot open DB: " << sqlite3_errmsg(db_conn) << std::endl; return false; }
    execute_query("PRAGMA journal_mode=WAL;");
    execute_query("PRAGMA foreign_keys=ON;");
    execute_query(R"(CREATE TABLE IF NOT EXISTS users (user_id INTEGER PRIMARY KEY, username TEXT, ingame_name TEXT, phone TEXT, lang TEXT DEFAULT 'en', available_bal REAL DEFAULT 0, locked_bal REAL DEFAULT 0, elo INTEGER DEFAULT 1000, wins INTEGER DEFAULT 0, losses INTEGER DEFAULT 0, is_registered INTEGER DEFAULT 0, is_banned INTEGER DEFAULT 0, welcome_given INTEGER DEFAULT 0, referrer_id INTEGER, state TEXT, state_data TEXT, last_daily TEXT, total_refs INTEGER DEFAULT 0, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, amount REAL, type TEXT, status TEXT DEFAULT 'COMPLETED', detail TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS matches (match_id TEXT PRIMARY KEY, p1_id INTEGER, p2_id INTEGER, fee REAL, status TEXT DEFAULT 'in_progress', p1_screenshot TEXT, p2_screenshot TEXT, winner_id INTEGER, verified_by INTEGER, tourney_id INTEGER, started_at TEXT DEFAULT (datetime('now')), created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS cancel_requests (match_id TEXT PRIMARY KEY, requested_by INTEGER, agreed_by INTEGER, status TEXT DEFAULT 'PENDING', created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS mfs_deposits (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, method TEXT, txid TEXT UNIQUE, amount REAL, screenshot TEXT, status TEXT DEFAULT 'PENDING', resolved_by TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS exc_deposits (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, exchanger TEXT, our_uid TEXT, user_uid TEXT, amount_usdt REAL, amount_tk REAL, screenshot TEXT, status TEXT DEFAULT 'PENDING', resolved_by TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS mfs_withdrawals (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, method TEXT, account TEXT, amount REAL, status TEXT DEFAULT 'PENDING', resolved_by TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS exc_withdrawals (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, exchanger TEXT, user_uid TEXT, amount_usdt REAL, amount_tk REAL, status TEXT DEFAULT 'PENDING', resolved_by TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS tournaments (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, slots INTEGER, entry_fee REAL, prize_pool REAL, status TEXT DEFAULT 'OPEN', created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS tourney_players (tourney_id INTEGER, user_id INTEGER, status TEXT DEFAULT 'ACTIVE', PRIMARY KEY (tourney_id, user_id));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS support_tickets (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, subject TEXT, status TEXT DEFAULT 'OPEN', created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS ticket_messages (id INTEGER PRIMARY KEY AUTOINCREMENT, ticket_id INTEGER, sender_id INTEGER, role TEXT, message TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS managers (user_id INTEGER PRIMARY KEY, added_by INTEGER, added_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS admins (user_id INTEGER PRIMARY KEY, added_by INTEGER, added_at TEXT DEFAULT (datetime('now')));)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT);)");
    execute_query(R"(CREATE TABLE IF NOT EXISTS logs (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, role TEXT, action TEXT, detail TEXT, created_at TEXT DEFAULT (datetime('now')));)");
    execute_query("INSERT OR IGNORE INTO settings(key,value) VALUES('usdt_deposit_rate','118.0');");
    execute_query("INSERT OR IGNORE INTO settings(key,value) VALUES('usdt_withdraw_rate','122.0');");
    return true;
}

void close_db() { if (db_conn) sqlite3_close(db_conn); }

std::string get_setting(const std::string& key, const std::string& def) {
    std::string r = scalar_str("SELECT value FROM settings WHERE key=" + q_esc(key) + ";");
    return r.empty() ? def : r;
}
void set_setting(const std::string& key, const std::string& value) {
    execute_query("INSERT OR REPLACE INTO settings(key,value) VALUES(" + q_esc(key) + "," + q_esc(value) + ");");
}
double deposit_rate() { return std::stod(get_setting("usdt_deposit_rate","118.0")); }
double withdraw_rate() { return std::stod(get_setting("usdt_withdraw_rate","122.0")); }
void load_payment_settings() {
    for (auto& kv : config::MOBILE_BANKING) { std::string v=get_setting("mfs_number_"+kv.first); if(!v.empty()) kv.second.number=v; }
    for (auto& kv : config::EXCHANGERS) {
        std::string u=get_setting("exc_uid_"+kv.first); if(!u.empty()) kv.second.our_uid=u;
        std::string nb=get_setting("exc_note_"+kv.first+"_bn"); if(!nb.empty()) kv.second.deposit_note_bn=nb;
        std::string ne=get_setting("exc_note_"+kv.first+"_en"); if(!ne.empty()) kv.second.deposit_note_en=ne;
    }
}
void record_transaction(long long uid, double amount, const std::string& type, const std::string& detail, const std::string& status) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount;
    execute_query("INSERT INTO transactions(user_id,amount,type,detail,status) VALUES("+std::to_string(uid)+","+ss.str()+","+q_esc(type)+","+q_esc(detail)+","+q_esc(status)+");");
}
void db_log(long long uid, const std::string& role, const std::string& action, const std::string& detail) {
    execute_query("INSERT INTO logs(user_id,role,action,detail) VALUES("+std::to_string(uid)+","+q_esc(role)+","+q_esc(action)+","+q_esc(detail)+");");
}

std::optional<User> get_user(long long uid) {
    sqlite3_stmt* stmt; std::optional<User> result;
    std::string sql="SELECT * FROM users WHERE user_id="+std::to_string(uid)+";";
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=row_to_user(stmt);
    sqlite3_finalize(stmt); return result;
}
bool create_user(long long uid, const std::string& username, long long referrer_id) {
    std::string ref=referrer_id?std::to_string(referrer_id):"NULL";
    return execute_query("INSERT OR IGNORE INTO users(user_id,username,referrer_id) VALUES("+std::to_string(uid)+","+q_esc(username)+","+ref+");");
}
bool update_user_str(long long uid, const std::string& col, const std::string& val) {
    return execute_query("UPDATE users SET "+col+"="+q_esc(val)+" WHERE user_id="+std::to_string(uid)+";");
}
bool update_user_int(long long uid, const std::string& col, int val) {
    return execute_query("UPDATE users SET "+col+"="+std::to_string(val)+" WHERE user_id="+std::to_string(uid)+";");
}
bool update_user_double(long long uid, const std::string& col, double val) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<val;
    return execute_query("UPDATE users SET "+col+"="+ss.str()+" WHERE user_id="+std::to_string(uid)+";");
}
bool set_state(long long uid, const std::string& state, const std::string& data) {
    return execute_query("UPDATE users SET state="+q_esc(state)+",state_data="+q_esc(data)+" WHERE user_id="+std::to_string(uid)+";");
}
bool clear_state(long long uid) {
    return execute_query("UPDATE users SET state=NULL,state_data=NULL WHERE user_id="+std::to_string(uid)+";");
}
bool adjust_balance(long long uid, double amount) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount;
    return execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+" WHERE user_id="+std::to_string(uid)+";");
}
bool lock_balance(long long uid, double amount) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount; std::string a=ss.str();
    return execute_query("UPDATE users SET available_bal=available_bal-"+a+",locked_bal=locked_bal+"+a+" WHERE user_id="+std::to_string(uid)+";");
}
bool unlock_balance(long long uid, double amount, bool restore) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount; std::string a=ss.str();
    if(restore) return execute_query("UPDATE users SET locked_bal=locked_bal-"+a+",available_bal=available_bal+"+a+" WHERE user_id="+std::to_string(uid)+";");
    return execute_query("UPDATE users SET locked_bal=locked_bal-"+a+" WHERE user_id="+std::to_string(uid)+";");
}
bool claim_daily_bonus(long long uid, double amount, const std::string& today) {
    if(scalar_str("SELECT last_daily FROM users WHERE user_id="+std::to_string(uid)+";")==today) return false;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount;
    execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+",last_daily="+q_esc(today)+" WHERE user_id="+std::to_string(uid)+";");
    record_transaction(uid,amount,"daily_bonus","Claimed daily bonus on "+today);
    return true;
}
int increment_referrals(long long ref_id) {
    execute_query("UPDATE users SET total_refs=total_refs+1 WHERE user_id="+std::to_string(ref_id)+";");
    return scalar_int("SELECT total_refs FROM users WHERE user_id="+std::to_string(ref_id)+";");
}
std::vector<User> get_top_elo(int limit) {
    sqlite3_stmt* stmt; std::vector<User> result;
    std::string sql="SELECT * FROM users WHERE is_registered=1 ORDER BY elo DESC LIMIT "+std::to_string(limit)+";";
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(row_to_user(stmt));
    sqlite3_finalize(stmt); return result;
}
std::string get_user_lang(long long uid) { auto u=get_user(uid); return u?u->lang:"en"; }

std::vector<long long> get_admins() {
    sqlite3_stmt* stmt; std::vector<long long> r;
    if(sqlite3_prepare_v2(db_conn,"SELECT user_id FROM admins;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) r.push_back(sqlite3_column_int64(stmt,0));
    sqlite3_finalize(stmt); return r;
}
bool add_admin(long long uid, long long added_by) { return execute_query("INSERT OR IGNORE INTO admins(user_id,added_by) VALUES("+std::to_string(uid)+","+std::to_string(added_by)+");"); }
bool remove_admin(long long uid) { return execute_query("DELETE FROM admins WHERE user_id="+std::to_string(uid)+";"); }
std::vector<long long> get_managers() {
    sqlite3_stmt* stmt; std::vector<long long> r;
    if(sqlite3_prepare_v2(db_conn,"SELECT user_id FROM managers;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) r.push_back(sqlite3_column_int64(stmt,0));
    sqlite3_finalize(stmt); return r;
}
bool add_manager(long long uid, long long added_by) { return execute_query("INSERT OR IGNORE INTO managers(user_id,added_by) VALUES("+std::to_string(uid)+","+std::to_string(added_by)+");"); }
bool remove_manager(long long uid) { return execute_query("DELETE FROM managers WHERE user_id="+std::to_string(uid)+";"); }

void add_to_queue(long long uid, double fee, long long lobby_msg_id, const std::string& extra_data) {
    std::lock_guard<std::mutex> lock(QUEUE_LOCK);
    QueueEntry e; e.user_id=uid; e.fee=fee; e.lobby_msg_id=lobby_msg_id; e.extra_data=extra_data;
    e.queued_at=std::chrono::steady_clock::now(); MATCH_QUEUE[uid]=e;
}
std::optional<QueueEntry> get_from_queue(long long uid) {
    std::lock_guard<std::mutex> lock(QUEUE_LOCK);
    auto it=MATCH_QUEUE.find(uid);
    return (it!=MATCH_QUEUE.end())?std::optional<QueueEntry>(it->second):std::nullopt;
}
std::optional<QueueEntry> find_opponent(double fee, long long exclude) {
    std::lock_guard<std::mutex> lock(QUEUE_LOCK);
    QueueEntry* best=nullptr;
    for(auto& kv:MATCH_QUEUE){ if(kv.first==exclude) continue; if(!best||kv.second.queued_at<best->queued_at) best=&kv.second; }
    return best?std::optional<QueueEntry>(*best):std::nullopt;
}
void remove_from_queue(long long uid) { std::lock_guard<std::mutex> lock(QUEUE_LOCK); MATCH_QUEUE.erase(uid); }

std::string create_match(long long p1, long long p2, double fee, int tourney_id) {
    std::string mid=gen_match_id(); std::string tid=tourney_id?std::to_string(tourney_id):"NULL";
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<fee; std::string fs=ss.str();
    if(fee>0){
        execute_query("UPDATE users SET available_bal=available_bal-"+fs+" WHERE user_id="+std::to_string(p1)+";");
        execute_query("UPDATE users SET available_bal=available_bal-"+fs+" WHERE user_id="+std::to_string(p2)+";");
    }
    execute_query("INSERT INTO matches(match_id,p1_id,p2_id,fee,tourney_id) VALUES("+q_esc(mid)+","+std::to_string(p1)+","+std::to_string(p2)+","+fs+","+tid+");");
    if(fee>0){ record_transaction(p1,-fee,"match_fee","Match #"+mid); record_transaction(p2,-fee,"match_fee","Match #"+mid); }
    return mid;
}
std::optional<Match> get_match(const std::string& mid) {
    sqlite3_stmt* stmt; std::optional<Match> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM matches WHERE match_id="+q_esc(mid)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=row_to_match(stmt);
    sqlite3_finalize(stmt); return result;
}
std::optional<Match> get_active_match(long long uid) {
    sqlite3_stmt* stmt; std::optional<Match> result;
    std::string sql="SELECT * FROM matches WHERE (p1_id="+std::to_string(uid)+" OR p2_id="+std::to_string(uid)+") AND status='in_progress' ORDER BY created_at DESC LIMIT 1;";
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=row_to_match(stmt);
    sqlite3_finalize(stmt); return result;
}
Match submit_screenshot(const std::string& mid, long long uid, const std::string& file_id) {
    auto m=get_match(mid);
    if(m){ if(uid==m->p1_id) execute_query("UPDATE matches SET p1_screenshot="+q_esc(file_id)+" WHERE match_id="+q_esc(mid)+";");
           else execute_query("UPDATE matches SET p2_screenshot="+q_esc(file_id)+" WHERE match_id="+q_esc(mid)+";"); }
    return get_match(mid).value_or(Match{});
}
Match resolve_match(const std::string& mid, long long winner_id, long long manager_id) {
    auto m_opt=get_match(mid); if(!m_opt) return Match{};
    Match m=*m_opt; long long loser_id=(winner_id==m.p1_id)?m.p2_id:m.p1_id;
    double prize=m.fee*1.8; std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<prize;
    if(prize>0) execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+" WHERE user_id="+std::to_string(winner_id)+";");
    execute_query("UPDATE users SET wins=wins+1,elo=elo+15 WHERE user_id="+std::to_string(winner_id)+";");
    execute_query("UPDATE users SET losses=losses+1,elo=MAX(800,elo-15) WHERE user_id="+std::to_string(loser_id)+";");
    execute_query("UPDATE matches SET status='completed',winner_id="+std::to_string(winner_id)+",verified_by="+std::to_string(manager_id)+" WHERE match_id="+q_esc(mid)+";");
    if(prize>0) record_transaction(winner_id,prize,"match_win","Match #"+mid);
    return m;
}
bool cancel_match_refund(const std::string& mid) {
    auto m_opt=get_match(mid); if(!m_opt) return false;
    Match m=*m_opt; std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<m.fee; std::string fs=ss.str();
    if(m.fee>0){
        execute_query("UPDATE users SET available_bal=available_bal+"+fs+" WHERE user_id="+std::to_string(m.p1_id)+";");
        execute_query("UPDATE users SET available_bal=available_bal+"+fs+" WHERE user_id="+std::to_string(m.p2_id)+";");
        record_transaction(m.p1_id,m.fee,"match_refund","Match #"+mid);
        record_transaction(m.p2_id,m.fee,"match_refund","Match #"+mid);
    }
    return execute_query("UPDATE matches SET status='cancelled' WHERE match_id="+q_esc(mid)+";");
}
std::optional<Match> autowin_match(const std::string& mid, long long winner_id) {
    auto m_opt=get_match(mid); if(!m_opt||m_opt->status!="in_progress") return m_opt;
    Match m=*m_opt; long long loser_id=(winner_id==m.p1_id)?m.p2_id:m.p1_id;
    double prize=m.fee*1.8; std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<prize;
    if(prize>0) execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+" WHERE user_id="+std::to_string(winner_id)+";");
    execute_query("UPDATE users SET wins=wins+1,elo=elo+15 WHERE user_id="+std::to_string(winner_id)+";");
    execute_query("UPDATE users SET losses=losses+1,elo=MAX(800,elo-15) WHERE user_id="+std::to_string(loser_id)+";");
    execute_query("UPDATE matches SET status='completed',winner_id="+std::to_string(winner_id)+",verified_by=0 WHERE match_id="+q_esc(mid)+";");
    if(prize>0) record_transaction(winner_id,prize,"match_win","Auto-win Match #"+mid);
    return m;
}
std::vector<Match> get_stale_matches(int minutes) {
    time_t cutoff=time(nullptr)-(long long)minutes*60; char buf[32];
    strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&cutoff));
    sqlite3_stmt* stmt; std::vector<Match> result;
    std::string sql="SELECT * FROM matches WHERE status='in_progress' AND started_at<='"+std::string(buf)+"';";
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(row_to_match(stmt));
    sqlite3_finalize(stmt); return result;
}
std::vector<Match> get_pending_matches() {
    sqlite3_stmt* stmt; std::vector<Match> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM matches WHERE p1_screenshot IS NOT NULL AND p2_screenshot IS NOT NULL AND status='in_progress';",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(row_to_match(stmt));
    sqlite3_finalize(stmt); return result;
}
std::vector<Match> get_match_history(long long uid, int limit) {
    sqlite3_stmt* stmt; std::vector<Match> result;
    std::string sql="SELECT * FROM matches WHERE (p1_id="+std::to_string(uid)+" OR p2_id="+std::to_string(uid)+") AND status='completed' ORDER BY created_at DESC LIMIT "+std::to_string(limit)+";";
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(row_to_match(stmt));
    sqlite3_finalize(stmt); return result;
}

bool create_cancel_req(const std::string& mid, long long uid) {
    return execute_query("INSERT OR IGNORE INTO cancel_requests(match_id,requested_by) VALUES("+q_esc(mid)+","+std::to_string(uid)+");");
}
bool get_cancel_req_info(const std::string& mid, long long& requested_by) {
    sqlite3_stmt* stmt; bool found=false;
    if(sqlite3_prepare_v2(db_conn,("SELECT requested_by FROM cancel_requests WHERE match_id="+q_esc(mid)+" AND status='PENDING';").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW){ requested_by=sqlite3_column_int64(stmt,0); found=true; }
    sqlite3_finalize(stmt); return found;
}
bool agree_cancel(const std::string& mid, long long uid) {
    cancel_match_refund(mid);
    return execute_query("UPDATE cancel_requests SET agreed_by="+std::to_string(uid)+",status='AGREED' WHERE match_id="+q_esc(mid)+";");
}

static MfsDeposit stmt_to_mfsdep(sqlite3_stmt* s){ MfsDeposit d; d.id=sqlite3_column_int(s,0); d.user_id=sqlite3_column_int64(s,1); d.method=col_str(s,2); d.txid=col_str(s,3); d.amount=sqlite3_column_double(s,4); d.screenshot=col_str(s,5); d.status=col_str(s,6); d.resolved_by=col_str(s,7); d.created_at=col_str(s,8); return d; }
int create_mfs_deposit(long long uid, const std::string& method, const std::string& txid, double amount, const std::string& screenshot) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount;
    execute_query("INSERT INTO mfs_deposits(user_id,method,txid,amount,screenshot) VALUES("+std::to_string(uid)+","+q_esc(method)+","+q_esc(txid)+","+ss.str()+","+q_esc(screenshot)+");");
    return (int)sqlite3_last_insert_rowid(db_conn);
}
std::optional<MfsDeposit> get_mfs_deposit(int dep_id) {
    sqlite3_stmt* stmt; std::optional<MfsDeposit> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM mfs_deposits WHERE id="+std::to_string(dep_id)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_mfsdep(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<MfsDeposit> get_pending_mfs_deposits() {
    sqlite3_stmt* stmt; std::vector<MfsDeposit> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM mfs_deposits WHERE status='PENDING' ORDER BY created_at;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_mfsdep(stmt));
    sqlite3_finalize(stmt); return result;
}
std::optional<MfsDeposit> approve_mfs_deposit(int dep_id, const std::string& admin_name) {
    auto d=get_mfs_deposit(dep_id); if(!d||d->status!="PENDING") return d;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<d->amount;
    execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+" WHERE user_id="+std::to_string(d->user_id)+";");
    execute_query("UPDATE mfs_deposits SET status='APPROVED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(dep_id)+";");
    record_transaction(d->user_id,d->amount,"mfs_deposit","TxID: "+d->txid);
    return get_mfs_deposit(dep_id);
}
std::optional<MfsDeposit> reject_mfs_deposit(int dep_id, const std::string& admin_name) {
    auto d=get_mfs_deposit(dep_id); if(!d||d->status!="PENDING") return d;
    execute_query("UPDATE mfs_deposits SET status='REJECTED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(dep_id)+";");
    return get_mfs_deposit(dep_id);
}

static ExcDeposit stmt_to_excdep(sqlite3_stmt* s){ ExcDeposit d; d.id=sqlite3_column_int(s,0); d.user_id=sqlite3_column_int64(s,1); d.exchanger=col_str(s,2); d.our_uid=col_str(s,3); d.user_uid=col_str(s,4); d.amount_usdt=sqlite3_column_double(s,5); d.amount_tk=sqlite3_column_double(s,6); d.screenshot=col_str(s,7); d.status=col_str(s,8); d.resolved_by=col_str(s,9); d.created_at=col_str(s,10); return d; }
int create_exc_deposit(long long uid, const std::string& exchanger, const std::string& our_uid, const std::string& user_uid, double amount_usdt, double amount_tk, const std::string& screenshot) {
    std::ostringstream su,st; su<<std::fixed<<std::setprecision(6)<<amount_usdt; st<<std::fixed<<std::setprecision(6)<<amount_tk;
    execute_query("INSERT INTO exc_deposits(user_id,exchanger,our_uid,user_uid,amount_usdt,amount_tk,screenshot) VALUES("+std::to_string(uid)+","+q_esc(exchanger)+","+q_esc(our_uid)+","+q_esc(user_uid)+","+su.str()+","+st.str()+","+q_esc(screenshot)+");");
    return (int)sqlite3_last_insert_rowid(db_conn);
}
std::optional<ExcDeposit> get_exc_deposit(int dep_id) {
    sqlite3_stmt* stmt; std::optional<ExcDeposit> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM exc_deposits WHERE id="+std::to_string(dep_id)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_excdep(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<ExcDeposit> get_pending_exc_deposits() {
    sqlite3_stmt* stmt; std::vector<ExcDeposit> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM exc_deposits WHERE status='PENDING' ORDER BY created_at;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_excdep(stmt));
    sqlite3_finalize(stmt); return result;
}
std::optional<ExcDeposit> approve_exc_deposit(int dep_id, const std::string& admin_name) {
    auto d=get_exc_deposit(dep_id); if(!d||d->status!="PENDING") return d;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<d->amount_tk;
    execute_query("UPDATE users SET available_bal=available_bal+"+ss.str()+" WHERE user_id="+std::to_string(d->user_id)+";");
    execute_query("UPDATE exc_deposits SET status='APPROVED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(dep_id)+";");
    record_transaction(d->user_id,d->amount_tk,"exc_deposit","Exchanger: "+d->exchanger);
    return get_exc_deposit(dep_id);
}
std::optional<ExcDeposit> reject_exc_deposit(int dep_id, const std::string& admin_name) {
    auto d=get_exc_deposit(dep_id); if(!d||d->status!="PENDING") return d;
    execute_query("UPDATE exc_deposits SET status='REJECTED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(dep_id)+";");
    return get_exc_deposit(dep_id);
}

static MfsWithdrawal stmt_to_mfswit(sqlite3_stmt* s){ MfsWithdrawal w; w.id=sqlite3_column_int(s,0); w.user_id=sqlite3_column_int64(s,1); w.method=col_str(s,2); w.account=col_str(s,3); w.amount=sqlite3_column_double(s,4); w.status=col_str(s,5); w.resolved_by=col_str(s,6); w.created_at=col_str(s,7); return w; }
int create_mfs_withdrawal(long long uid, const std::string& method, const std::string& account, double amount) {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<amount; std::string a=ss.str();
    execute_query("UPDATE users SET available_bal=available_bal-"+a+",locked_bal=locked_bal+"+a+" WHERE user_id="+std::to_string(uid)+" AND available_bal>="+a+";");
    execute_query("INSERT INTO mfs_withdrawals(user_id,method,account,amount) VALUES("+std::to_string(uid)+","+q_esc(method)+","+q_esc(account)+","+a+");");
    int wid=(int)sqlite3_last_insert_rowid(db_conn);
    record_transaction(uid,-amount,"mfs_withdrawal","Withdrawal #"+std::to_string(wid),"PENDING");
    return wid;
}
std::optional<MfsWithdrawal> get_mfs_withdrawal(int wid) {
    sqlite3_stmt* stmt; std::optional<MfsWithdrawal> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM mfs_withdrawals WHERE id="+std::to_string(wid)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_mfswit(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<MfsWithdrawal> get_pending_mfs_withdrawals() {
    sqlite3_stmt* stmt; std::vector<MfsWithdrawal> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM mfs_withdrawals WHERE status='PENDING' ORDER BY created_at;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_mfswit(stmt));
    sqlite3_finalize(stmt); return result;
}
std::optional<MfsWithdrawal> approve_mfs_withdrawal(int wid, const std::string& admin_name) {
    auto w=get_mfs_withdrawal(wid); if(!w||w->status!="PENDING") return w;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<w->amount;
    execute_query("UPDATE users SET locked_bal=locked_bal-"+ss.str()+" WHERE user_id="+std::to_string(w->user_id)+";");
    execute_query("UPDATE mfs_withdrawals SET status='APPROVED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(wid)+";");
    return get_mfs_withdrawal(wid);
}
std::optional<MfsWithdrawal> reject_mfs_withdrawal(int wid, const std::string& admin_name) {
    auto w=get_mfs_withdrawal(wid); if(!w||w->status!="PENDING") return w;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<w->amount; std::string a=ss.str();
    execute_query("UPDATE users SET locked_bal=locked_bal-"+a+",available_bal=available_bal+"+a+" WHERE user_id="+std::to_string(w->user_id)+";");
    execute_query("UPDATE mfs_withdrawals SET status='REJECTED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(wid)+";");
    record_transaction(w->user_id,w->amount,"withdrawal_refund","Refund #"+std::to_string(wid));
    return get_mfs_withdrawal(wid);
}

static ExcWithdrawal stmt_to_excwit(sqlite3_stmt* s){ ExcWithdrawal w; w.id=sqlite3_column_int(s,0); w.user_id=sqlite3_column_int64(s,1); w.exchanger=col_str(s,2); w.user_uid=col_str(s,3); w.amount_usdt=sqlite3_column_double(s,4); w.amount_tk=sqlite3_column_double(s,5); w.status=col_str(s,6); w.resolved_by=col_str(s,7); w.created_at=col_str(s,8); return w; }
int create_exc_withdrawal(long long uid, const std::string& exchanger, const std::string& user_uid, double amount_usdt, double amount_tk) {
    std::ostringstream su,st; su<<std::fixed<<std::setprecision(6)<<amount_usdt; st<<std::fixed<<std::setprecision(6)<<amount_tk; std::string tk=st.str();
    execute_query("UPDATE users SET available_bal=available_bal-"+tk+",locked_bal=locked_bal+"+tk+" WHERE user_id="+std::to_string(uid)+" AND available_bal>="+tk+";");
    execute_query("INSERT INTO exc_withdrawals(user_id,exchanger,user_uid,amount_usdt,amount_tk) VALUES("+std::to_string(uid)+","+q_esc(exchanger)+","+q_esc(user_uid)+","+su.str()+","+tk+");");
    int wid=(int)sqlite3_last_insert_rowid(db_conn);
    record_transaction(uid,-amount_tk,"exc_withdrawal","Withdrawal #"+std::to_string(wid),"PENDING");
    return wid;
}
std::optional<ExcWithdrawal> get_exc_withdrawal(int wid) {
    sqlite3_stmt* stmt; std::optional<ExcWithdrawal> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM exc_withdrawals WHERE id="+std::to_string(wid)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_excwit(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<ExcWithdrawal> get_pending_exc_withdrawals() {
    sqlite3_stmt* stmt; std::vector<ExcWithdrawal> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM exc_withdrawals WHERE status='PENDING' ORDER BY created_at;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_excwit(stmt));
    sqlite3_finalize(stmt); return result;
}
std::optional<ExcWithdrawal> approve_exc_withdrawal(int wid, const std::string& admin_name) {
    auto w=get_exc_withdrawal(wid); if(!w||w->status!="PENDING") return w;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<w->amount_tk;
    execute_query("UPDATE users SET locked_bal=locked_bal-"+ss.str()+" WHERE user_id="+std::to_string(w->user_id)+";");
    execute_query("UPDATE exc_withdrawals SET status='APPROVED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(wid)+";");
    return get_exc_withdrawal(wid);
}
std::optional<ExcWithdrawal> reject_exc_withdrawal(int wid, const std::string& admin_name) {
    auto w=get_exc_withdrawal(wid); if(!w||w->status!="PENDING") return w;
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<w->amount_tk; std::string tk=ss.str();
    execute_query("UPDATE users SET locked_bal=locked_bal-"+tk+",available_bal=available_bal+"+tk+" WHERE user_id="+std::to_string(w->user_id)+";");
    execute_query("UPDATE exc_withdrawals SET status='REJECTED',resolved_by="+q_esc(admin_name)+" WHERE id="+std::to_string(wid)+";");
    record_transaction(w->user_id,w->amount_tk,"withdrawal_refund","Refund #"+std::to_string(wid));
    return get_exc_withdrawal(wid);
}

static Tournament stmt_to_tourney(sqlite3_stmt* s){ Tournament t; t.id=sqlite3_column_int(s,0); t.name=col_str(s,1); t.slots=sqlite3_column_int(s,2); t.entry_fee=sqlite3_column_double(s,3); t.prize_pool=sqlite3_column_double(s,4); t.status=col_str(s,5); t.created_at=col_str(s,6); return t; }
int create_tournament(const std::string& name, int slots, double entry_fee, double prize_pool) {
    std::ostringstream se,sp; se<<std::fixed<<std::setprecision(6)<<entry_fee; sp<<std::fixed<<std::setprecision(6)<<prize_pool;
    execute_query("INSERT INTO tournaments(name,slots,entry_fee,prize_pool) VALUES("+q_esc(name)+","+std::to_string(slots)+","+se.str()+","+sp.str()+");");
    return (int)sqlite3_last_insert_rowid(db_conn);
}
std::vector<Tournament> get_open_tournaments() {
    sqlite3_stmt* stmt; std::vector<Tournament> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM tournaments WHERE status='OPEN' ORDER BY id;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_tourney(stmt));
    sqlite3_finalize(stmt); return result;
}
std::optional<Tournament> get_tournament(int tid) {
    sqlite3_stmt* stmt; std::optional<Tournament> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM tournaments WHERE id="+std::to_string(tid)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_tourney(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<std::pair<long long,std::string>> get_tourney_players(int tid, const std::string& status_filter) {
    std::string sql="SELECT user_id,status FROM tourney_players WHERE tourney_id="+std::to_string(tid);
    if(!status_filter.empty()) sql+=" AND status="+q_esc(status_filter); sql+=";";
    sqlite3_stmt* stmt; std::vector<std::pair<long long,std::string>> result;
    if(sqlite3_prepare_v2(db_conn,sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back({sqlite3_column_int64(stmt,0),col_str(stmt,1)});
    sqlite3_finalize(stmt); return result;
}
bool join_tournament(int tid, long long uid, double fee) {
    bool ok=execute_query("INSERT OR IGNORE INTO tourney_players(tourney_id,user_id) VALUES("+std::to_string(tid)+","+std::to_string(uid)+");");
    if(ok&&fee>0){ std::ostringstream ss; ss<<std::fixed<<std::setprecision(6)<<fee;
        execute_query("UPDATE users SET available_bal=available_bal-"+ss.str()+" WHERE user_id="+std::to_string(uid)+";");
        record_transaction(uid,-fee,"tourney_fee","Tournament #"+std::to_string(tid)); }
    return ok;
}
bool eliminate_player(int tid, long long uid) { return execute_query("UPDATE tourney_players SET status='ELIMINATED' WHERE tourney_id="+std::to_string(tid)+" AND user_id="+std::to_string(uid)+";"); }
bool update_tournament_status(int tid, const std::string& status) { return execute_query("UPDATE tournaments SET status="+q_esc(status)+" WHERE id="+std::to_string(tid)+";"); }

static Ticket stmt_to_ticket(sqlite3_stmt* s){ Ticket t; t.id=sqlite3_column_int(s,0); t.user_id=sqlite3_column_int64(s,1); t.subject=col_str(s,2); t.status=col_str(s,3); t.created_at=col_str(s,4); return t; }
int create_ticket(long long uid, const std::string& subject) {
    execute_query("INSERT INTO support_tickets(user_id,subject) VALUES("+std::to_string(uid)+","+q_esc(subject)+");");
    return (int)sqlite3_last_insert_rowid(db_conn);
}
bool add_ticket_msg(int ticket_id, long long sender, const std::string& role, const std::string& msg) {
    return execute_query("INSERT INTO ticket_messages(ticket_id,sender_id,role,message) VALUES("+std::to_string(ticket_id)+","+std::to_string(sender)+","+q_esc(role)+","+q_esc(msg)+");");
}
std::optional<Ticket> get_ticket(int ticket_id) {
    sqlite3_stmt* stmt; std::optional<Ticket> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM support_tickets WHERE id="+std::to_string(ticket_id)+";").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        if(sqlite3_step(stmt)==SQLITE_ROW) result=stmt_to_ticket(stmt);
    sqlite3_finalize(stmt); return result;
}
std::vector<Ticket> get_user_tickets(long long uid) {
    sqlite3_stmt* stmt; std::vector<Ticket> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT * FROM support_tickets WHERE user_id="+std::to_string(uid)+" ORDER BY created_at DESC LIMIT 5;").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_ticket(stmt));
    sqlite3_finalize(stmt); return result;
}
std::vector<Ticket> get_open_tickets() {
    sqlite3_stmt* stmt; std::vector<Ticket> result;
    if(sqlite3_prepare_v2(db_conn,"SELECT * FROM support_tickets WHERE status='OPEN' ORDER BY created_at;",-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW) result.push_back(stmt_to_ticket(stmt));
    sqlite3_finalize(stmt); return result;
}
std::vector<std::map<std::string,std::string>> get_ticket_msgs(int ticket_id) {
    sqlite3_stmt* stmt; std::vector<std::map<std::string,std::string>> result;
    if(sqlite3_prepare_v2(db_conn,("SELECT sender_id,role,message,created_at FROM ticket_messages WHERE ticket_id="+std::to_string(ticket_id)+" ORDER BY created_at;").c_str(),-1,&stmt,nullptr)==SQLITE_OK)
        while(sqlite3_step(stmt)==SQLITE_ROW){
            std::map<std::string,std::string> row;
            row["sender_id"]=std::to_string(sqlite3_column_int64(stmt,0)); row["role"]=col_str(stmt,1);
            row["message"]=col_str(stmt,2); row["created_at"]=col_str(stmt,3); result.push_back(row);
        }
    sqlite3_finalize(stmt); return result;
}
bool close_ticket(int ticket_id) { return execute_query("UPDATE support_tickets SET status='CLOSED' WHERE id="+std::to_string(ticket_id)+";"); }

DailyReport get_daily_report() {
    DailyReport r; r.date=today_str(); std::string d=r.date;
    auto si=[&](const std::string& s){ return scalar_int(s); };
    auto sd=[&](const std::string& s){ return scalar_double(s); };
    r.total_users=si("SELECT COUNT(*) FROM users;");
    r.new_users=si("SELECT COUNT(*) FROM users WHERE DATE(datetime(created_at,'localtime'))='"+d+"';");
    r.matches=si("SELECT COUNT(*) FROM matches WHERE DATE(datetime(created_at,'localtime'))='"+d+"';");
    r.completed=si("SELECT COUNT(*) FROM matches WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='completed';");
    r.fees=sd("SELECT COALESCE(SUM(fee),0) FROM matches WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='completed';");
    r.mfs_dep_count=si("SELECT COUNT(*) FROM mfs_deposits WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.mfs_dep_amount=sd("SELECT COALESCE(SUM(amount),0) FROM mfs_deposits WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.exc_dep_count=si("SELECT COUNT(*) FROM exc_deposits WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.exc_dep_usdt=sd("SELECT COALESCE(SUM(amount_usdt),0) FROM exc_deposits WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.exc_dep_tk=sd("SELECT COALESCE(SUM(amount_tk),0) FROM exc_deposits WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.mfs_wit_amount=sd("SELECT COALESCE(SUM(amount),0) FROM mfs_withdrawals WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.exc_wit_usdt=sd("SELECT COALESCE(SUM(amount_usdt),0) FROM exc_withdrawals WHERE DATE(datetime(created_at,'localtime'))='"+d+"' AND status='APPROVED';");
    r.pending_mfs_dep=si("SELECT COUNT(*) FROM mfs_deposits WHERE status='PENDING';");
    r.pending_exc_dep=si("SELECT COUNT(*) FROM exc_deposits WHERE status='PENDING';");
    r.pending_mfs_wit=si("SELECT COUNT(*) FROM mfs_withdrawals WHERE status='PENDING';");
    r.pending_exc_wit=si("SELECT COUNT(*) FROM exc_withdrawals WHERE status='PENDING';");
    r.dep_rate=get_setting("usdt_deposit_rate","118.0");
    r.wit_rate=get_setting("usdt_withdraw_rate","122.0");
    return r;
}

bool safe_backup(const std::string& dest_path) {
    sqlite3* dst=nullptr;
    if(sqlite3_open(dest_path.c_str(),&dst)!=SQLITE_OK) return false;
    sqlite3_backup* bk=sqlite3_backup_init(dst,"main",db_conn,"main");
    if(!bk){ sqlite3_close(dst); return false; }
    sqlite3_backup_step(bk,-1); sqlite3_backup_finish(bk); sqlite3_close(dst);
    return true;
}

} // namespace db
