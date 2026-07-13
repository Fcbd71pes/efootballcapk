// main.cpp — eFootball Bot Entry Point
#include <iostream>
#include <atomic>
#include <tgbot/tgbot.h>
#include "config.h"
#include "db.h"
#include "user_cmds.h"
#include "admin_cmds.h"
#include "handlers.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "eFootballBot", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eFootballBot", __VA_ARGS__)
extern std::atomic<bool> is_bot_running;
#else
#define LOGI(...) std::cout << __VA_ARGS__ << std::endl
#define LOGE(...) std::cerr << __VA_ARGS__ << std::endl
std::atomic<bool> is_bot_running(true);
#endif

int run_bot(const std::string& db_path) {
    LOGI("eFootball Bot (C++) starting...");

    if (!db::init_db(db_path)) {
        LOGE("Failed to initialize database. Exiting.");
        return 1;
    }
    LOGI("Database initialized: %s", db_path.c_str());

    // Load dynamic admins/managers from DB
    config::ADMINS = db::get_admins();
    config::MANAGERS = db::get_managers();
    if (config::ADMINS.empty()) {
        config::ADMINS.push_back(config::SUPER_ADMIN);
        db::add_admin(config::SUPER_ADMIN, config::SUPER_ADMIN);
    }

    // Load payment settings
    db::load_payment_settings();

    TgBot::Bot bot(config::TOKEN);
    LOGI("Bot token loaded.");

    // Register command handlers
    user_cmds::setup(bot);
    admin_cmds::setup(bot);
    handlers::setup(bot);

    LOGI("All handlers registered. Starting polling...");
    TgBot::TgLongPoll longPoll(bot);
    
    while (is_bot_running) {
        try {
            // Using a short timeout for polling so we can check is_bot_running frequently
            longPoll.start();
        } catch (TgBot::TgException& e) {
            LOGE("TgBot Exception: %s", e.what());
        } catch (std::exception& e) {
            LOGE("Exception: %s", e.what());
        }
    }

    db::close_db();
    LOGI("Bot stopped gracefully.");
    return 0;
}

#ifndef __ANDROID__
int main() {
    is_bot_running = true;
    return run_bot(config::LOCAL_DB);
}
#endif
