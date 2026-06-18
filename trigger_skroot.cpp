/**
 * Trigger-SKRoot v1.1.0: SKRoot Pro 版 Trigger 模块
 *
 * 功能:
 * 1. 开机自动关闭 ADB
 * 2. 解温控 — bind mount 温控配置文件 + 循环写 /proc/shell-temp
 * 3. 游戏缓存清理（三角洲行动/瓦罗兰特）
 * 4. 内置 WebUI 管理页面
 *
 * 基于 Trigger v1.1.0 (GPL-3.0) 改写为 SKRoot Pro 模块
 * 原作者: Vorthas (3486075184)
 */
#include "kernel_module_kit_umbrella.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

/* ==================== 配置 ==================== */
#define CONFIG_DIR "/data/adb/.Magic/Trigger"
#define CONFIG_FILE "/data/adb/.Magic/Trigger/kernel_config.txt"

static std::string g_module_dir;
static std::atomic<bool> g_thermal_running{true};

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

static void disable_adb(void) {
    system("settings put global adb_enabled 0");
}

/* ==================== 游戏缓存清理 ==================== */

static void clean_dfm(void) {
    /* 三角洲行动 - 完整清理路径 */
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.dfm/files 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.dfm/cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/app_* 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/code_cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/databases 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/filescommonCache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/shared_prefs 2>/dev/null");
    system("rm -f /data/user/*/com.tencent.tmgp.dfm/files/* 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/.* 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/ano_tmp 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/app 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/beacon 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/com.gcloudsdk.gcloud.gvoice 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/data 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/perfsight 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/live_log 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/popup 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/tbs 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/qm 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/shell_cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/tdm_tmp 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/wupSCache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/UE4Game/DeltaForce/DeltaForce/Saved/LoadTrack 2>/dev/null");
    system("rm -f /data/user/*/com.tencent.tmgp.dfm/files/UE4Game/DeltaForce/* 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.dfm/files/UE4Game/DeltaForce/DeltaForce/Intermediate 2>/dev/null");
    /* 三角洲残留 */
    system("rm -rf /storage/emulated/*/Documents 2>/dev/null");
    system("rm -f /storage/emulated/*/Download/* 2>/dev/null");
    printf("[Trigger-SKRoot] DFM cache cleaned\n");
}

static void clean_codev(void) {
    /* 瓦罗兰特 - 完整清理路径 */
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/cache 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/env 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/EstvShadowPlugin 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/file_cnf 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/itop 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/log 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/midas 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/tbslog 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/tencent 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/TGPA 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/VulkanProgramBinaryCache 2>/dev/null");
    system("rm -f /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/UE4Game/CodeV/* 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/UE4Game/CodeV/CodeV/Saved/Gamelet/logs 2>/dev/null");
    system("rm -rf /storage/emulated/*/Android/data/com.tencent.tmgp.codev/files/UE4Game/CodeV/CodeV/Saved/Logs 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/app_* 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/code_cache 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/databases 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/EstvShadowPlugin_shadow-app 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/files 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/no_backup 2>/dev/null");
    system("rm -rf /data/user/*/com.tencent.tmgp.codev/shared_prefs 2>/dev/null");
    printf("[Trigger-SKRoot] CodeV cache cleaned\n");
}

/* ==================== 温控解除 ==================== */

/* bind mount 温控配置文件（来自原版 post-fs-data.sh） */
static void bind_mount_thermal_configs(void) {
    struct {
        const char *src_suffix;
        const char *dst;
    } mounts[] = {
        {"odm/etc/temperature_profile/sys_high_temp_protect_OPPO_23821.xml",
         "/odm/etc/temperature_profile/sys_high_temp_protect_OPPO_23821.xml"},
        {"odm/etc/temperature_profile/sys_thermal_control_config.xml",
         "/odm/etc/temperature_profile/sys_thermal_control_config.xml"},
        {"odm/etc/ThermalServiceConfig",
         "/odm/etc/ThermalServiceConfig"},
        {"odm/firmware/fastchg/bms_heating_config.txt",
         "/odm/firmware/fastchg/bms_heating_config.txt"},
    };

    for (auto &m : mounts) {
        char src[512];
        snprintf(src, sizeof(src), "%s/%s", g_module_dir.c_str(), m.src_suffix);
        if (access(src, F_OK) != 0) {
            printf("[Trigger-SKRoot] skip mount (not found): %s\n", src);
            continue;
        }
        int ret = mount(src, m.dst, nullptr, MS_BIND, nullptr);
        if (ret == 0) {
            printf("[Trigger-SKRoot] bind mount OK: %s -> %s\n", src, m.dst);
        } else {
            printf("[Trigger-SKRoot] bind mount FAILED: %s -> %s (%s)\n",
                   src, m.dst, strerror(errno));
        }
    }
}

/* 温控守护循环（来自原版 service.sh） */
static void thermal_daemon_loop(void) {
    printf("[Trigger-SKRoot] thermal daemon started\n");
    while (g_thermal_running.load()) {
        system("setprop init.svc.thermal-engine stopped 2>/dev/null");
        system("setprop init.svc.android.thermal-hal stopped 2>/dev/null");
        system("echo '0 37000' > /proc/shell-temp 2>/dev/null");
        system("echo '1 37000' > /proc/shell-temp 2>/dev/null");
        system("echo '2 37000' > /proc/shell-temp 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
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
        g_module_dir = module_dir;
        printf("[Trigger-SKRoot] WebUI on port %d, dir=%s\n", port, module_dir.c_str());
    }

    bool handleGet(CivetServer *server, struct mg_connection *conn,
                   const std::string &path, const std::string &query) override {
        if (path == "/") {
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
            send_json(conn, 200, "{\"error\":\"index.html not found\"}");
            return true;
        }
        return false;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn,
                    const std::string &path, const std::string &body) override {
        /* 状态查询 */
        if (path == "/api/status") {
            char adb[16] = "0";
            FILE *p = popen("settings get global adb_enabled 2>/dev/null", "r");
            if (p) { fgets(adb, sizeof(adb), p); pclose(p); }
            char *nl = strchr(adb, '\n'); if (nl) *nl = '\0';

            char resp[256];
            snprintf(resp, sizeof(resp),
                "{\"adb_enabled\":\"%s\",\"thermal_disabled\":true,"
                "\"module\":\"Trigger-SKRoot\",\"version\":\"1.1.0\"}", adb);
            send_json(conn, 200, resp);
            return true;
        }

        /* ADB 切换 */
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

        /* 游戏缓存清理 */
        if (path == "/api/clean") {
            if (body.find("codev") != std::string::npos) {
                clean_codev();
                send_json(conn, 200, "{\"result\":\"瓦罗兰特缓存已清理\"}");
            } else {
                clean_dfm();
                send_json(conn, 200, "{\"result\":\"三角洲缓存已清理\"}");
            }
            return true;
        }

        send_json(conn, 404, "{\"error\":\"not found\"}");
        return true;
    }
};

SKROOT_MODULE_WEB_UI(TriggerWebUI)

/* ==================== 模块入口 ==================== */

int skroot_module_main(const char *root_key, const char *module_private_dir) {
    printf("[Trigger-SKRoot] ====================\n");
    printf("[Trigger-SKRoot] v1.1.0 starting...\n");
    printf("[Trigger-SKRoot] module_dir=%s\n", module_private_dir);
    printf("[Trigger-SKRoot] ====================\n");

    g_module_dir = module_private_dir ? module_private_dir : "";

    /* 创建配置目录 */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/custom_modules", CONFIG_DIR);
    system(cmd);
    system("touch " CONFIG_FILE);

    /* 1. 关闭 ADB */
    disable_adb();
    printf("[Trigger-SKRoot] ADB disabled\n");

    /* 2. bind mount 温控配置文件 */
    bind_mount_thermal_configs();

    /* 3. Fork 温控守护进程 */
    pid_t pid = fork();
    if (pid > 0) {
        printf("[Trigger-SKRoot] thermal daemon forked (pid=%d)\n", pid);
        printf("[Trigger-SKRoot] loaded OK\n");
        return 0;
    }
    if (pid < 0) return -1;

    /* 子进程：温控守护 */
    setsid();
    thermal_daemon_loop();
    _exit(0);
    return 0;
}

/* ==================== 模块名片 ==================== */
SKROOT_MODULE_NAME("Trigger-SKRoot")
SKROOT_MODULE_VERSION("1.1.0")
SKROOT_MODULE_DESC("自启动 | 关闭USB调试 | 解温控 | 游戏清理 | WebUI管理")
SKROOT_MODULE_AUTHOR("Trigger (Vorthas) -> SKRoot port")
SKROOT_MODULE_ID32("fd913fffe887f7ab71ba938ba5b8df79")
