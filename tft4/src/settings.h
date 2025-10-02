#pragma once
#include <AutoOTA.h>
#include <SettingsGyver.h>

#include "config.h"
#include "db.h"
#include "gen.h"
// #include "timer.h"
SettingsGyver sett("AI Фоторамка v" F_VERSION, &db);
sets::Timer gentmr;

AutoOTA ota(F_VERSION, "AlexGyver/AiFrame/main/project.json");

void init_tmr() {
    int prd = db[kk::auto_prd];
    gentmr.setTime(0, max(prd, 60));
    if (db[kk::auto_gen].toBool()) gentmr.startInterval();
    else gentmr.stop();
}

void build(sets::Builder& b) {
    {
        sets::Group g(b, "Генерация");
        b.Select(kk::gen_style, "Стиль", gen.styles);
        b.Input(kk::gen_query, "Промт");
        b.Input(kk::gen_negative, "Исключить");
        b.Label(SH("status"), "Статус", gen.status);
        b.Button(SH("generate"), "Генерировать");
    }
    {
        sets::Group g(b, "Автогенерация");
        b.Switch(kk::auto_gen, "Включить");
        b.Time(kk::auto_prd, "Период");
    }
    {
        sets::Group g(b, "Настройки");
        {
            sets::Menu m(b, "WiFi");
            sets::Group g(b);
            b.Input(kk::wifi_ssid, "SSID");
            b.Pass(kk::wifi_pass, "Pass");
            b.Button(SH("wifi_save"), "Подключить");
        }
        {
            sets::Menu m(b, "API");
            sets::Group g(b);
            b.Input(kk::ya_folder_id, "Folder ID");
            b.Pass(kk::ya_api_id, "Application key");
            b.Button(SH("api_save"), "Применить");
        }
    }

    if (b.Confirm("update"_h)) ota.update();

    // actions
    if (b.build.isAction()) {
        switch (b.build.id) {
            case SH("generate"):
                generate();
                init_tmr();
                break;
            case SH("wifi_save"):
                db.update();
                ESP.reset();
                break;
            case SH("api_save"):
                gen.setKey(db[kk::ya_api_id], db[kk::ya_folder_id]);
                db.update();
                break;
            case kk::auto_gen:
            case kk::auto_prd:
                init_tmr();
                break;
        }
    }
}

void update(sets::Updater& u) {
    u.update(SH("status"), gen.status);
    if (ota.hasUpdate()) u.update("update"_h, "Доступно обновление. Обновить прошивку?");
}

void sett_init() {
    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(update);
    init_tmr();
}

void sett_tick() {
    ota.tick();
    sett.tick();
    if (gentmr) generate();
}