#include <jni.h>
#include <string>
#include <thread>
#include <atomic>
#include <android/log.h>

#define LOG_TAG "eFootballBot"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern std::atomic<bool> is_bot_running;
std::atomic<bool> is_bot_running(false);
std::thread bot_thread;

// Function prototype for the main bot loop (implemented in main.cpp)
extern int run_bot(const std::string& db_path);

extern "C" JNIEXPORT void JNICALL
Java_com_efootball_app_MainActivity_startBot(
        JNIEnv* env,
        jobject /* this */,
        jstring dbPathStr) {
    
    if (is_bot_running) {
        LOGI("Bot is already running.");
        return;
    }

    const char *db_path_chars = env->GetStringUTFChars(dbPathStr, 0);
    std::string db_path(db_path_chars);
    env->ReleaseStringUTFChars(dbPathStr, db_path_chars);

    is_bot_running = true;
    
    bot_thread = std::thread([db_path]() {
        LOGI("Starting bot thread with DB path: %s", db_path.c_str());
        run_bot(db_path);
        is_bot_running = false;
        LOGI("Bot thread exited.");
    });
    
    bot_thread.detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_efootball_app_MainActivity_stopBot(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("Stopping bot requested...");
    is_bot_running = false;
}
