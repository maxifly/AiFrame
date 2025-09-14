#pragma once
#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "settings.db");

enum kk : size_t {
    wifi_ssid,
    wifi_pass,
    ya_folder_id,
    ya_api_id,
    gen_query,
    gen_negative,
    gen_style,
    auto_gen,
    auto_prd,
};

void db_init() {
    LittleFS.begin();
    db.begin();
    db.init(kk::wifi_ssid, "");
    db.init(kk::wifi_pass, "");
    db.init(kk::ya_folder_id, "");
    db.init(kk::ya_api_id, "");

    db.init(kk::gen_query, "");
    db.init(kk::gen_negative, "");
    db.init(kk::gen_style, 0);

    db.init(kk::auto_gen, 0);
    db.init(kk::auto_prd, 60);
}