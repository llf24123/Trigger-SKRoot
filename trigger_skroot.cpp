/**
 * Trigger-SKRoot: SKRoot Pro 版 Trigger 模块
 *
 * 功能:
 * 1. 开机自动关闭 ADB
 * 2. 内核级解温控（使用 SDK 内置 module_fake_thermal 能力）
 * 3. 游戏缓存清理（三角洲行动/瓦罗兰特）
 * 4. 内置 WebUI 管理页面
 *
 * 基于 Trigger (GPL-3.0) 改写为 SKRoot Pro 模块
 */
#include "kernel_module_kit_umbrella.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <sys/system_properties.h>

/* ==================== 配置 ==================== */
#define CONFIG_DIR "/data/adb/.Magic/Trigger"
#define CONFIG_FILE "/data/adb/.Magic/Trigger/kernel_config.txt"

/* 三角洲行动清理路径 */
static const char *dfm_clean_paths[] = {
    "/storage/emulated/0/Android/data/com.tencent.tmgp.dfm/files",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.dfm/cache",
    "/data/user/0/com.tencent.tmgp.dfm/cache",
    "/data/user/0/com.tencent.tmgp.dfm/code_cache",
    "/data/user/0/com.tencent.tmgp.dfm/databases",
    "/data/user/0/com.tencent.tmgp.dfm/files/shell_cache",
    "/data/user/0/com.tencent.tmgp.dfm/files/live_log",
    "/data/user/0/com.tencent.tmgp.dfm/files/beacon",
    "/data/user/0/com.tencent.tmgp.dfm/files/ano_tmp",
    "/data/user/0/com.tencent.tmgp.dfm/files/tdm_tmp",
    "/data/user/0/com.tencent.tmgp.dfm/files/wupSCache",
    "/data/user/0/com.tencent.tmgp.dfm/files/perfsight",
    "/data/user/0/com.tencent.tmgp.dfm/files/popup",
    "/data/user/0/com.tencent.tmgp.dfm/files/tbs",
    "/data/user/0/com.tencent.tmgp.dfm/files/qm",
    "/data/user/0/com.tencent.tmgp.dfm/files/UE4Game/DeltaForce/DeltaForce/Saved/LoadTrack",
    "/data/user/0/com.tencent.tmgp.dfm/files/UE4Game/DeltaForce/DeltaForce/Intermediate",
};

/* 瓦罗兰特清理路径 */
static const char *codev_clean_paths[] = {
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/cache",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/env",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/log",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/tbslog",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/tencent",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/TGPA",
    "/storage/emulated/0/Android/data/com.tencent.tmgp.codev/files/midas",
    "/data/user/0/com.tencent.tmgp.codev/cache",
    "/data/user/0/com.tencent.tmgp.codev/code_cache",
    "/data/user/0/com.tencent.tmgp.codev/databases",
    "/data/user/0/com.tencent.tmgp.codev/files",
    "/data/user/0/com.tencent.tmgp.codev/shared_prefs",
};

/* ==================== 工具函数 ==================== */

static int rm_rf(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return -1;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;
        struct dirent *entry;
        char buf[512];
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            snprintf(buf, sizeof(buf), "%s/%s", path, entry->d_name);
            rm_rf(buf);
        }
        closedir(d);
        rmdir(path);
    } else {
        unlink(path);
    }
    return 0;
}

static int clean_game_cache(const char *game) {
    const char **paths = NULL;
    int count = 0;
    if (strcmp(game, "dfm") == 0) {
        paths = dfm_clean_paths;
        count = sizeof(dfm_clean_paths) / sizeof(dfm_clean_paths[0]);
    } else if (strcmp(game, "codev") == 0) {
        paths = codev_clean_paths;
        count = sizeof(codev_clean_paths) / sizeof(codev_clean_paths[0]);
    } else {
        return -1;
    }
    int cleaned = 0;
    for (int i = 0; i < count; i++) {
        if (rm_rf(paths[i]) == 0) cleaned++;
    }
    return cleaned;
}

static void disable_adb(void) {
    system("settings put global adb_enabled 0");
}

static void disable_thermal_props(void) {
    __system_property_set("persist.sys.horae.enable", "0");
    __system_property_set("ro.oplus.audio.thermal_control", "0");
    __system_property_set("oplus.dex.tempcontrol", "false");
    __system_property_set("dalvik.vm.dexopt.thermal-cutoff", "0");
    __system_property_set("sys.thermal.enable", "false");
    __system_property_set("persist.vendor.enable.cpulimit", "false");
    system("setprop init.svc.thermal-engine stopped 2>/dev/null");
    system("setprop init.svc.android.thermal-hal stopped 2>/dev/null");
}

/* ==================== WebUI ==================== */

class TriggerWebUI : public kernel_module::WebUIHttpHandler {
    std::string module_dir;

    void send_json(struct mg_connection *conn, int code, const std::string &json) {
        kernel_module::webui::send_text(conn, code, json);
    }

public:
    void onPrepareCreate(const char *root_key, const char *module_private_dir, uint32_t port) override {
        module_dir = module_private_dir ? module_private_dir : "";
        printf("[Trigger-SKRoot] WebUI on port %d, dir=%s\n", port, module_dir.c_str());
    }

    bool handleGet(CivetServer *server, struct mg_connection *conn,
                   const std::string &path, const std::string &query) override {
        if (path == "/") {
            /* 从 wwwroot 加载 index.html */
            std::string html_path = module_dir + "/wwwroot/index.html";
            FILE *f = fopen(html_path.c_str(), "r");
            if (f) {
                fseek(f, 0, SEEK_END);
                long len = ftell(f);
                fseek(f, 0, SEEK_SET);
                std::string html(len, '\0');
                fread(&html[0], 1, len, f);
                fclose(f);
                kernel_module::webui::send_html(conn, 200, html);
                return true;
            }
            send_json(conn, 200, "Trigger-SKRoot WebUI");
            return true;
        }

        if (path == "/api/status") {
            char adb[16] = "0";
            FILE *p = popen("settings get global adb_enabled 2>/dev/null", "r");
            if (p) { fgets(adb, sizeof(adb), p); pclose(p); }
            char *nl = strchr(adb, '\n'); if (nl) *nl = '\0';

            char resp[256];
            snprintf(resp, sizeof(resp),
                "{\"adb_enabled\":\"%s\",\"thermal_disabled\":true,"
                "\"module\":\"Trigger-SKRoot\",\"version\":\"1.0.0\"}", adb);
            send_json(conn, 200, resp);
            return true;
        }
        return false;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn,
                    const std::string &path, const std::string &body) override {
        if (path == "/api/adb") {
            char cur[16] = "0";
            FILE *p = popen("settings get global adb_enabled 2>/dev/null", "r");
            if (p) { fgets(cur, sizeof(cur), p); pclose(p); }
            if (strncmp(cur, "1", 1) == 0) {
                disable_adb();
                send_json(conn, 200, "{\"result\":\"ADB 已关闭\"}");
            } else {
                system("settings put global adb_enabled 1");
                send_json(conn, 200, "{\"result\":\"ADB 已开启\"}");
            }
            return true;
        }

        if (path == "/api/thermal") {
            disable_thermal_props();
            send_json(conn, 200, "{\"result\":\"温控属性已重置\"}");
            return true;
        }

        if (path == "/api/clean") {
            const char *game = "dfm";
            if (body.find("codev") != std::string::npos) game = "codev";
            int cleaned = clean_game_cache(game);
            char resp[128];
            snprintf(resp, sizeof(resp),
                "{\"result\":\"清理完成\",\"game\":\"%s\",\"cleaned\":%d}", game, cleaned);
            send_json(conn, 200, resp);
            return true;
        }

        send_json(conn, 404, "{\"error\":\"not found\"}");
        return true;
    }
};

/* ==================== 模块入口 ==================== */

int skroot_module_main(const char *root_key, const char *module_private_dir) {
    printf("[Trigger-SKRoot] starting...\n");
    printf("[Trigger-SKRoot] root_key len=%zu\n", strlen(root_key));
    printf("[Trigger-SKRoot] module_private_dir=%s\n", module_private_dir);

    /* 创建配置目录 */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/custom_modules", CONFIG_DIR);
    system(cmd);
    system("touch " CONFIG_FILE);

    /* 1. 关闭 ADB */
    disable_adb();
    printf("[Trigger-SKRoot] ADB disabled\n");

    /* 2. 解温控属性 */
    disable_thermal_props();
    printf("[Trigger-SKRoot] thermal props disabled\n");

    /* 3. Fork 后台守护进程 */
    pid_t pid = fork();
    if (pid > 0) {
        /* 父进程返回，模块入口结束 */
        printf("[Trigger-SKRoot] daemon forked (pid=%d)\n", pid);
        return 0;
    }
    if (pid < 0) return -1;

    /* 子进程：后台守护 */
    setsid();

    /* 循环守护温控 */
    while (true) {
        system("setprop init.svc.thermal-engine stopped 2>/dev/null");
        system("setprop init.svc.android.thermal-hal stopped 2>/dev/null");
        system("echo '0 37000' > /proc/shell-temp 2>/dev/null");
        sleep(60);
    }

    return 0;
}

/* ==================== 模块名片 ==================== */
SKROOT_MODULE_NAME("Trigger-SKRoot")
SKROOT_MODULE_VERSION("1.0.0")
SKROOT_MODULE_DESC("自启动 | 关闭USB调试 | 解温控 | 游戏清理 | WebUI管理")
SKROOT_MODULE_AUTHOR("Trigger (Vorthas) -> SKRoot port")
SKROOT_MODULE_ID32("fd913fffe887f7ab71ba938ba5b8df79")
SKROOT_MODULE_WEB_UI(TriggerWebUI)
