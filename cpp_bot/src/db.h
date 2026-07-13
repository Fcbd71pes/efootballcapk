// db.h — eFootball Bot Database Layer
#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <mutex>
#include <chrono>

namespace db {

    extern sqlite3* db_conn;
    bool init_db(const std::string& db_path);
    void close_db();
    bool execute_query(const std::string& query);
    bool execute_stmt(const std::string& sql, const std::vector<std::string>& params);

    struct User {
        long long user_id = 0;
        std::string username, ingame_name, phone, lang = "en";
        double available_bal = 0.0, locked_bal = 0.0;
        int elo = 1000, wins = 0, losses = 0;
        int is_registered = 0, is_banned = 0, welcome_given = 0;
        long long referrer_id = 0;
        std::string state, state_data, last_daily, created_at;
        int total_refs = 0;
    };

    struct Match {
        std::string match_id, status = "in_progress";
        long long p1_id = 0, p2_id = 0, winner_id = 0, verified_by = 0;
        double fee = 0.0;
        std::string p1_screenshot, p2_screenshot;
        int tourney_id = 0;
        std::string started_at, created_at;
    };

    struct QueueEntry {
        long long user_id = 0, lobby_msg_id = 0;
        double fee = 0.0;
        std::string extra_data = "[]";
        std::chrono::steady_clock::time_point queued_at;
    };

    struct MfsDeposit {
        int id = 0;
        long long user_id = 0;
        std::string method, txid, screenshot, status = "PENDING", resolved_by, created_at;
        double amount = 0.0;
    };

    struct ExcDeposit {
        int id = 0;
        long long user_id = 0;
        std::string exchanger, our_uid, user_uid, screenshot, status = "PENDING", resolved_by, created_at;
        double amount_usdt = 0.0, amount_tk = 0.0;
    };

    struct MfsWithdrawal {
        int id = 0;
        long long user_id = 0;
        std::string method, account, status = "PENDING", resolved_by, created_at;
        double amount = 0.0;
    };

    struct ExcWithdrawal {
        int id = 0;
        long long user_id = 0;
        std::string exchanger, user_uid, status = "PENDING", resolved_by, created_at;
        double amount_usdt = 0.0, amount_tk = 0.0;
    };

    struct Tournament {
        int id = 0, slots = 0;
        std::string name, status = "OPEN", created_at;
        double entry_fee = 0.0, prize_pool = 0.0;
    };

    struct Ticket {
        int id = 0;
        long long user_id = 0;
        std::string subject, status = "OPEN", created_at;
    };

    struct DailyReport {
        std::string date, dep_rate, wit_rate;
        int total_users=0, new_users=0, matches=0, completed=0;
        double fees=0.0, mfs_dep_amount=0.0, exc_dep_usdt=0.0, exc_dep_tk=0.0;
        double mfs_wit_amount=0.0, exc_wit_usdt=0.0;
        int mfs_dep_count=0, exc_dep_count=0;
        int pending_mfs_dep=0, pending_exc_dep=0, pending_mfs_wit=0, pending_exc_wit=0;
    };

    // Settings
    std::string get_setting(const std::string& key, const std::string& def = "");
    void set_setting(const std::string& key, const std::string& value);
    double deposit_rate();
    double withdraw_rate();
    void load_payment_settings();

    // Users
    std::optional<User> get_user(long long uid);
    bool create_user(long long uid, const std::string& username, long long referrer_id = 0);
    bool update_user_str(long long uid, const std::string& col, const std::string& val);
    bool update_user_int(long long uid, const std::string& col, int val);
    bool update_user_double(long long uid, const std::string& col, double val);
    bool set_state(long long uid, const std::string& state, const std::string& data = "");
    bool clear_state(long long uid);
    bool adjust_balance(long long uid, double amount);
    bool lock_balance(long long uid, double amount);
    bool unlock_balance(long long uid, double amount, bool restore = true);
    bool claim_daily_bonus(long long uid, double amount, const std::string& today);
    int increment_referrals(long long ref_id);
    std::vector<User> get_top_elo(int limit = 10);
    std::string get_user_lang(long long uid);

    // Admins/Managers
    std::vector<long long> get_admins();
    bool add_admin(long long uid, long long added_by);
    bool remove_admin(long long uid);
    std::vector<long long> get_managers();
    bool add_manager(long long uid, long long added_by);
    bool remove_manager(long long uid);

    // Match Queue
    extern std::map<long long, QueueEntry> MATCH_QUEUE;
    extern std::mutex QUEUE_LOCK;
    void add_to_queue(long long uid, double fee, long long lobby_msg_id, const std::string& extra_data = "[]");
    std::optional<QueueEntry> get_from_queue(long long uid);
    std::optional<QueueEntry> find_opponent(double fee, long long exclude);
    void remove_from_queue(long long uid);

    // Matches
    std::string create_match(long long p1, long long p2, double fee, int tourney_id = 0);
    std::optional<Match> get_match(const std::string& mid);
    std::optional<Match> get_active_match(long long uid);
    Match submit_screenshot(const std::string& mid, long long uid, const std::string& file_id);
    Match resolve_match(const std::string& mid, long long winner_id, long long manager_id);
    bool cancel_match_refund(const std::string& mid);
    std::optional<Match> autowin_match(const std::string& mid, long long winner_id);
    std::vector<Match> get_stale_matches(int minutes);
    std::vector<Match> get_pending_matches();
    std::vector<Match> get_match_history(long long uid, int limit = 10);

    // Cancel Requests
    bool create_cancel_req(const std::string& mid, long long uid);
    bool get_cancel_req_info(const std::string& mid, long long& requested_by);
    bool agree_cancel(const std::string& mid, long long uid);

    // MFS Deposits
    int create_mfs_deposit(long long uid, const std::string& method, const std::string& txid, double amount, const std::string& screenshot);
    std::optional<MfsDeposit> get_mfs_deposit(int dep_id);
    std::vector<MfsDeposit> get_pending_mfs_deposits();
    std::optional<MfsDeposit> approve_mfs_deposit(int dep_id, const std::string& admin_name);
    std::optional<MfsDeposit> reject_mfs_deposit(int dep_id, const std::string& admin_name);

    // Exchange Deposits
    int create_exc_deposit(long long uid, const std::string& exchanger, const std::string& our_uid,
                           const std::string& user_uid, double amount_usdt, double amount_tk,
                           const std::string& screenshot);
    std::optional<ExcDeposit> get_exc_deposit(int dep_id);
    std::vector<ExcDeposit> get_pending_exc_deposits();
    std::optional<ExcDeposit> approve_exc_deposit(int dep_id, const std::string& admin_name);
    std::optional<ExcDeposit> reject_exc_deposit(int dep_id, const std::string& admin_name);

    // MFS Withdrawals
    int create_mfs_withdrawal(long long uid, const std::string& method, const std::string& account, double amount);
    std::optional<MfsWithdrawal> get_mfs_withdrawal(int wid);
    std::vector<MfsWithdrawal> get_pending_mfs_withdrawals();
    std::optional<MfsWithdrawal> approve_mfs_withdrawal(int wid, const std::string& admin_name);
    std::optional<MfsWithdrawal> reject_mfs_withdrawal(int wid, const std::string& admin_name);

    // Exchange Withdrawals
    int create_exc_withdrawal(long long uid, const std::string& exchanger, const std::string& user_uid,
                              double amount_usdt, double amount_tk);
    std::optional<ExcWithdrawal> get_exc_withdrawal(int wid);
    std::vector<ExcWithdrawal> get_pending_exc_withdrawals();
    std::optional<ExcWithdrawal> approve_exc_withdrawal(int wid, const std::string& admin_name);
    std::optional<ExcWithdrawal> reject_exc_withdrawal(int wid, const std::string& admin_name);

    // Tournaments
    int create_tournament(const std::string& name, int slots, double entry_fee, double prize_pool);
    std::vector<Tournament> get_open_tournaments();
    std::optional<Tournament> get_tournament(int tid);
    std::vector<std::pair<long long, std::string>> get_tourney_players(int tid, const std::string& status_filter = "");
    bool join_tournament(int tid, long long uid, double fee);
    bool eliminate_player(int tid, long long uid);
    bool update_tournament_status(int tid, const std::string& status);

    // Support Tickets
    int create_ticket(long long uid, const std::string& subject);
    bool add_ticket_msg(int ticket_id, long long sender, const std::string& role, const std::string& msg);
    std::optional<Ticket> get_ticket(int ticket_id);
    std::vector<Ticket> get_user_tickets(long long uid);
    std::vector<Ticket> get_open_tickets();
    std::vector<std::map<std::string,std::string>> get_ticket_msgs(int ticket_id);
    bool close_ticket(int ticket_id);

    // Reports & Misc
    DailyReport get_daily_report();
    void record_transaction(long long uid, double amount, const std::string& type,
                            const std::string& detail = "", const std::string& status = "COMPLETED");
    void db_log(long long uid, const std::string& role, const std::string& action, const std::string& detail = "");
    bool safe_backup(const std::string& dest_path);
}
