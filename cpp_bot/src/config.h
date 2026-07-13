#pragma once

#include <string>
#include <vector>
#include <map>

namespace config {
    extern const std::string TOKEN;
    extern const long long SUPER_API_ID;
    extern const std::string API_HASH;

    extern const std::string WEBAPP_URL;

    extern const std::string OCR_SPACE_API_KEY;
    extern const std::string TRONGRID_API_KEY;
    extern const std::string ADMIN_TRC20_ADDRESS;

    extern const long long SUPER_ADMIN;
    extern std::vector<long long> ADMINS;
    extern std::vector<long long> MANAGERS;

    extern const std::string BOT_USERNAME;
    extern const long long CHANNEL_ID;
    extern const long long LOBBY_CHANNEL_ID;
    extern const long long GROUP_ID;
    extern const std::string CHANNEL_USERNAME;

    extern const double REFERRAL_BONUS;

    extern const int MATCH_TIMEOUT_MINUTES;
    extern const int MATCH_WARNING_MINUTES;

    struct MobileBank {
        std::string name;
        std::string number;
        std::string emoji;
    };
    extern std::map<std::string, MobileBank> MOBILE_BANKING;

    extern const double MINIMUM_DEPOSIT;
    extern const double MINIMUM_WITHDRAWAL;

    extern const double USDT_DEPOSIT_RATE_DEFAULT;
    extern const double USDT_WITHDRAW_RATE_DEFAULT;
    extern const double MIN_USDT_DEPOSIT;
    extern const double MIN_USDT_WITHDRAWAL;

    extern const std::string USDT_TRC20_ADDRESS;
    extern const std::string USDT_BEP20_ADDRESS;
    extern const std::string USDT_ERC20_ADDRESS;

    struct Exchanger {
        std::string name;
        std::string emoji;
        std::string our_uid;
        std::string uid_label;
        std::string deposit_note_en;
        std::string deposit_note_bn;
        std::string withdraw_note_en;
        std::string withdraw_note_bn;
    };
    extern std::map<std::string, Exchanger> EXCHANGERS;

    extern const std::string LOCAL_DB;
    extern const long long SUPPORT_CHAT_ID;

    extern const std::string ADMIN_PANEL_HOST;
    extern const int ADMIN_PANEL_PORT;

    extern const bool USE_TURSO;
    extern const std::string TURSO_DB_URL;
    extern const std::string TURSO_AUTH_TOKEN;

    extern bool FREE_MODE;
}
