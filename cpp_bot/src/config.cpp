#include "config.h"

namespace config {
    const std::string TOKEN = "8010407192:AAH-9yMHbBsxcUE4Ss4WEkCVMX_U_a901C8";
    const long long SUPER_API_ID = 27725203;
    const std::string API_HASH = "815ccf8fc0e271eb58bb151cdb192837";

    const std::string WEBAPP_URL = "";

    const std::string OCR_SPACE_API_KEY = "";
    const std::string TRONGRID_API_KEY = "";
    const std::string ADMIN_TRC20_ADDRESS = "";

    const long long SUPER_ADMIN = 5172723202;
    std::vector<long long> ADMINS = {SUPER_ADMIN};
    std::vector<long long> MANAGERS = {};

    const std::string BOT_USERNAME = "esfootball_tournament_bot";
    const long long CHANNEL_ID = -1003079996041;
    const long long LOBBY_CHANNEL_ID = -1003079996041;
    const long long GROUP_ID = -1003079996041;
    const std::string CHANNEL_USERNAME = "xefootball_esports";

    const double REFERRAL_BONUS = 5.0;

    const int MATCH_TIMEOUT_MINUTES = 15;
    const int MATCH_WARNING_MINUTES = 10;

    std::map<std::string, MobileBank> MOBILE_BANKING = {
        {"bkash", {"Bkash", "01914573762", "🟣"}},
        {"nagad", {"Nagad", "01914573762", "🟠"}},
        {"rocket", {"Rocket", "01914573762", "🔵"}},
        {"upay", {"Upay", "01914573762", "🟢"}}
    };

    const double MINIMUM_DEPOSIT = 50.0;
    const double MINIMUM_WITHDRAWAL = 100.0;

    const double USDT_DEPOSIT_RATE_DEFAULT = 118.0;
    const double USDT_WITHDRAW_RATE_DEFAULT = 122.0;
    const double MIN_USDT_DEPOSIT = 1.0;
    const double MIN_USDT_WITHDRAWAL = 2.0;

    const std::string USDT_TRC20_ADDRESS = "";
    const std::string USDT_BEP20_ADDRESS = "";
    const std::string USDT_ERC20_ADDRESS = "";

    std::map<std::string, Exchanger> EXCHANGERS = {
        {"binance", {"Binance", "🟡", "837755101", "Binance UID / Pay ID",
                     "Send USDT to our Binance UID via Binance Pay or internal transfer.",
                     "আমাদের Binance UID-এ USDT পাঠান।",
                     "We will send USDT to your Binance account.",
                     "আমরা আপনার Binance একাউন্টে USDT পাঠাব।"}},
        {"bybit", {"Bybit", "🟠", "251189431", "Bybit UID",
                   "Send USDT to our Bybit UID via internal transfer.",
                   "আমাদের Bybit UID-এ USDT পাঠান।",
                   "We will send USDT to your Bybit account.",
                   "আমরা আপনার Bybit একাউন্টে USDT পাঠাব।"}},
        // Other exchangers could be added similarly
        {"binance_p2p", {"Binance P2P", "🤝", "", "Binance UID",
                         "Send USDT to our Binance UID via Binance Pay/P2P.",
                         "আমাদের Binance UID-এ P2P-তে USDT পাঠান।",
                         "We will send via Binance P2P to your UID.",
                         "আমরা Binance P2P-তে পাঠাব।"}}
    };

    const std::string LOCAL_DB = "efootball.db";
    const long long SUPPORT_CHAT_ID = -1001234567890;

    const std::string ADMIN_PANEL_HOST = "0.0.0.0";
    const int ADMIN_PANEL_PORT = 8000;

    const bool USE_TURSO = false;
    const std::string TURSO_DB_URL = "libsql://efootball-xxxourov.aws-us-east-2.turso.io";
    const std::string TURSO_AUTH_TOKEN = "YOUR_TURSO_AUTH_TOKEN_HERE";

    bool FREE_MODE = false;
}
