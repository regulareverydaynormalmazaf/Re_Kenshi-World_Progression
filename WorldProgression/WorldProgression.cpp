// ============================================================================
//  Copyright (C) 2026 regulareverydaynormalmazaf
//  SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  WorldProgression v21  —  KenshiLib plugin (Kenshi 1.0.65, Steam + GOG).
//
//  v21: окно прогресса по дням — две новые настройки (0 = выкл у обеих):
//    start_day (S) — реальный день, с которого мод начинает работать; он же «день 1»
//       логики бафа. До дня S новые NPC не бафаются. Внутренний день = realDay - S + 1.
//    stop_day (E) — реальный день, после которого рост замирает (баф как будто всегда
//       день E). Задаётся РЕАЛЬНЫМ днём: effReal = min(realDay, E).
//    Эффективный день формулы: eff = min(realDay, E) - S + 1 (при S=0 сдвиг не
//    применяется). Если обе включены, требуется S < E; иначе stop_day игнорируется
//    (предупреждение в лог). Существующие настройки не теряются — ключи добавляются
//    к конфигу с дефолтом 0 (см. механизм адаптации из v20).
//
//  v20: конфиг теперь хранится в КОРНЕ ИГРЫ (папке Kenshi.exe), а не рядом с DLL —
//  папку мода Steam Workshop перезаписывает при обновлении, корень игры нет. Порядок:
//  есть конфиг в корне игры -> читаем его; иначе есть старый рядом с DLL -> читаем и
//  переносим (миграция). Затем ВСЕГДА пересохраняем в корень; чтение по ключам с
//  дефолтом, поэтому новые параметры добавляются со значением по умолчанию, убранные
//  игнорируются, а заданные пользователем значения сохраняются. В сборку .config не
//  кладём — мод создаёт его сам при первом запуске.
//
//  Каждые N дней НОВЫЕ спавнящиеся NPC получают статы выше (плоско или процентом;
//  масштаб растёт с игровым днём). Загруженные из сейва не трогаются (правится
//  момент генерации статов новичка = _NV_init -> идемпотентно). ИГРОК НЕ трогается
//  никогда — мод прогрессирует МИР (NPC), игрок растёт сам по себе в игре.
//
//  Потолок статов (max_stat): 0 = выкл; иначе наш баф не поднимает стат выше него
//  и НИКОГДА не опускает ниже шаблонной базы (чужие высокие статы не режем).
//  Тумблеры: животные, пер-стат характеристики.
//
//  v14 (релизная чистка): убраны диагностические хуки прошлых расследований
//  (_randomiseStats и _NV_periodicUpdate/наблюдатель за игроком) — они больше не
//  нужны, а меньше хуков = меньше нагрузка и меньше поверхность для конфликтов.
//  Остались только рабочие хуки: CharStats::_NV_init (баф) и GameWorld main loop
//  (игровое время + повтор регистрации хаба). Расовый функционал убран в v11
//  (доступ к AppearanceManager при загрузке ломал анимацию стелса).
//
//  v16: убран баф игрока (и тумблеры buff_player/cap_player). Причина по логам:
//  _NV_init игрока срабатывает ОДИН раз в самом начале (day=-1) с шаблонной базой
//  (все статы ~1), ДО того как стартовый сценарий проставит реальные статы игрока —
//  наш баф там всё равно затирается; и на загрузке сейва _NV_init для игрока не
//  вызывается вовсе. Поэтому игрок теперь просто исключён. NPC не изменились.
//
//  v17: меню (Mod Hub) локализовано EN/RU. Язык берётся из ЯЗЫКА ЗАПУСКА ИГРЫ —
//  из Kenshi settings.cfg строка language= (тот же файл, что читает RE_Kenshi),
//  как это сделано в моде RangedDefence. Override: config-ключ language=auto/en/ru.
//
//  v18: починка вербовки. Наш баф поднимает боевые статы, а Kenshi разрешает
//  вербовку только пока они низкие (условие диалога DC_IS_RECRUITABLE: все 6 <=25
//  ИЛИ сумма <75). Из-за бафа диалог найма отключался. Хук на Dialogue::_checkCondition
//  для DC_IS_RECRUITABLE отвечает "вербуем" независимо от статов (тумблер allow_recruit,
//  по умолч. вкл) -> рекрутов снова можно нанять; в отряд они приходят с бафом.
//  Ни хранения, ни базовых статов не нужно; переживает перезагрузку.
//
//  v19: режим случайных статов (randomize, по умолч. ВКЛ — теперь основной). Вместо
//  того чтобы ставить стат ровно в максимум, берём случайное значение в диапазоне
//  [база игры .. наш максимум за день] (тип flat/percent задаёт максимум). Каждый стат
//  роллится независимо -> NPC получаются разнообразными. Кап и приоритет базы применяются
//  к максимуму. randomize=0 -> прежний стабильный рост (все статы = максимум).
//
//  Все игровые функции резолвятся ПО СИМВОЛАМ (KenshiLib) -> Steam и GOG.
//  Конфиг: WorldProgression.config в КОРНЕ ИГРЫ (переживает апдейты Мастерской).
//  Лог: WorldProgression.log в корне.
// ============================================================================
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <core/Functions.h>
#include <kenshi/Enums.h>
#include <kenshi/CharStats.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/util/TimeOfDay.h>
#include "Dialogue.h"          // shipped: minimal decl for Dialogue::_checkCondition (recruit fix)
#include "mod_hub_api.h"

#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ---- buffable characteristics (per-stat toggles; default all ON) ----
struct StatDef { StatsEnumerated stat; const char* id; const char* label_en; const char* label_ru; };
static const StatDef kStats[] = {
    { STAT_STRENGTH,     "st_strength",     "Strength",                     "\xD0\xA1\xD0\xB8\xD0\xBB\xD0\xB0" },
    { STAT_DEXTERITY,    "st_dexterity",    "Dexterity",                    "\xD0\x9B\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C" },
    { STAT_TOUGHNESS,    "st_toughness",    "Toughness",                    "\xD0\x9A\xD1\x80\xD0\xB5\xD0\xBF\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C" },
    { STAT_PERCEPTION,   "st_perception",   "Perception",                   "\xD0\x92\xD0\xBE\xD1\x81\xD0\xBF\xD1\x80\xD0\xB8\xD1\x8F\xD1\x82\xD0\xB8\xD0\xB5" },
    { STAT_MELEE_ATTACK, "st_melee_attack", "Melee Attack",                 "\xD0\x90\xD1\x82\xD0\xB0\xD0\xBA\xD0\xB0 \xD0\xB2 \xD0\xB1\xD0\xBB\xD0\xB8\xD0\xB6\xD0\xBD\xD0\xB5\xD0\xBC \xD0\xB1\xD0\xBE\xD1\x8E" },
    { STAT_MELEE_DEFENCE,"st_melee_defence","Melee Defence",                "\xD0\x97\xD0\xB0\xD1\x89\xD0\xB8\xD1\x82\xD0\xB0 \xD0\xB2 \xD0\xB1\xD0\xBB\xD0\xB8\xD0\xB6\xD0\xBD\xD0\xB5\xD0\xBC \xD0\xB1\xD0\xBE\xD1\x8E" },
    { STAT_DODGE,        "st_dodge",        "Dodge",                        "\xD0\xA3\xD0\xBA\xD0\xBB\xD0\xBE\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5" },
    { STAT_MARTIALARTS,  "st_martial",      "Martial Arts",                 "\xD0\x91\xD0\xBE\xD0\xB5\xD0\xB2\xD1\x8B\xD0\xB5 \xD0\xB8\xD1\x81\xD0\xBA\xD1\x83\xD1\x81\xD1\x81\xD1\x82\xD0\xB2\xD0\xB0" },
    { STAT_ATHLETICS,    "st_athletics",    "Athletics (affects run speed)","\xD0\x90\xD1\x82\xD0\xBB\xD0\xB5\xD1\x82\xD0\xB8\xD0\xBA\xD0\xB0 (\xD1\x81\xD0\xBA\xD0\xBE\xD1\x80\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C \xD0\xB1\xD0\xB5\xD0\xB3\xD0\xB0)" },
    { STAT_ASSASSINATION,"st_assassin",     "Assassination",                "\xD0\xA1\xD0\xBA\xD1\x80\xD1\x8B\xD1\x82\xD0\xBD\xD0\xBE\xD0\xB5 \xD1\x83\xD0\xB1\xD0\xB8\xD0\xB9\xD1\x81\xD1\x82\xD0\xB2\xD0\xBE" },
    { STAT_KATANAS,      "st_katanas",      "Katanas",                      "\xD0\x9A\xD0\xB0\xD1\x82\xD0\xB0\xD0\xBD\xD1\x8B" },
    { STAT_SABRES,       "st_sabres",       "Sabres",                       "\xD0\xA1\xD0\xB0\xD0\xB1\xD0\xBB\xD0\xB8" },
    { STAT_HACKERS,      "st_hackers",      "Hackers",                      "\xD0\xA2\xD0\xB5\xD1\x81\xD0\xB0\xD0\xBA\xD0\xB8" },
    { STAT_HEAVYWEAPONS, "st_heavy",        "Heavy Weapons",                "\xD0\xA2\xD1\x8F\xD0\xB6\xD1\x91\xD0\xBB\xD0\xBE\xD0\xB5 \xD0\xBE\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5" },
    { STAT_BLUNT,        "st_blunt",        "Blunt",                        "\xD0\x94\xD1\x80\xD0\xBE\xD0\xB1\xD1\x8F\xD1\x89\xD0\xB5\xD0\xB5 \xD0\xBE\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5" },
    { STAT_POLEARMS,     "st_polearms",     "Polearms",                     "\xD0\x94\xD1\x80\xD0\xB5\xD0\xB2\xD0\xBA\xD0\xBE\xD0\xB2\xD0\xBE\xD0\xB5 \xD0\xBE\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5" },
    { STAT_CROSSBOWS,    "st_crossbows",    "Crossbows",                    "\xD0\x90\xD1\x80\xD0\xB1\xD0\xB0\xD0\xBB\xD0\xB5\xD1\x82\xD1\x8B" },
    { STAT_TURRETS,      "st_turrets",      "Turrets",                      "\xD0\xA2\xD1\x83\xD1\x80\xD0\xB5\xD0\xBB\xD0\xB8" },
    { STAT_WEAPONS,      "st_weapons",      "Weapons (unused stat)",        "\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 (\xD0\xBD\xD0\xB5 \xD0\xB8\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x83\xD0\xB5\xD1\x82\xD1\x81\xD1\x8F)" },
    { STAT_MASSCOMBAT,   "st_masscombat",   "Mass Combat",                  "\xD0\x9C\xD0\xB0\xD1\x81\xD1\x81\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9 \xD0\xB1\xD0\xBE\xD0\xB9" }
};
enum { NSTATS = (int)(sizeof(kStats)/sizeof(kStats[0])) };

// ---- log ----
static std::ofstream* g_log = 0;
static void L(const std::string& s) { if (g_log) { *g_log << s << "\n"; g_log->flush(); } }
static void Lc(const char* s)        { if (g_log && s) { *g_log << s << "\n"; g_log->flush(); } }

// ============================================================================
//  Localization (Mod Hub labels/descriptions). Language follows the GAME:
//  Kenshi's settings.cfg "language=" (the same value RE_Kenshi reads at launch) —
//  or forced via the config "language" key ("auto"/"en"/"ru"). Only EN + RU today.
//  Same method as the RangedDefence mod.
// ============================================================================
enum Lang { LANG_EN = 0, LANG_RU = 1 };
static int         g_lang = LANG_EN;          // resolved UI language
static std::string g_langPref = "auto";       // config override: "auto" | "en" | "ru"
// Pick a string for the current language. Both operands are static string literals.
static inline const char* tr(const char* en, const char* ru) { return (g_lang == LANG_RU) ? ru : en; }

static bool looksRussian(const std::string& raw) {
    std::string low = raw;
    for (size_t i = 0; i < low.size(); ++i) low[i] = (char)tolower((unsigned char)low[i]);
    if (low == "ru" || low.rfind("ru", 0) == 0 || low.find("russ") != std::string::npos) return true;
    for (size_t i = 0; i < raw.size(); ++i) {           // UTF-8 Cyrillic lead bytes -> "Русский"
        unsigned char c = (unsigned char)raw[i];
        if (c == 0xD0 || c == 0xD1) return true;
    }
    return false;
}
// Read the game's launch language from Kenshi's settings.cfg (process cwd = game dir,
// exactly like RE_Kenshi). The mod-config "language" override wins if set.
static void DetectLanguage() {
    if (g_langPref == "ru") { g_lang = LANG_RU; return; }
    if (g_langPref == "en") { g_lang = LANG_EN; return; }
    g_lang = LANG_EN;                                   // default
    std::ifstream f("settings.cfg");                    // Kenshi's, in the game dir
    if (!f.good()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        size_t sep = line.find_first_of("=:\t");
        if (sep == std::string::npos) continue;
        std::string key = line.substr(0, sep);
        size_t ka = key.find_first_not_of(" \t\r\n"); size_t kb = key.find_last_not_of(" \t\r\n");
        if (ka == std::string::npos) continue;
        key = key.substr(ka, kb - ka + 1);
        for (size_t i = 0; i < key.size(); ++i) key[i] = (char)tolower((unsigned char)key[i]);
        if (key != "language") continue;
        std::string val = line.substr(sep + 1);
        size_t va = val.find_first_not_of(" \t\r\n"); size_t vb = val.find_last_not_of(" \t\r\n");
        if (va == std::string::npos) continue;
        val = val.substr(va, vb - va + 1);
        if (looksRussian(val)) g_lang = LANG_RU;
        return;                                         // first "language" line wins
    }
}

// ---- config / state ----
static int32_t g_enabled=1, g_debug=1, g_mode=0, g_interval=5;
static int32_t g_max_stat=0;
static int32_t g_startDay=0;          // real day the mod starts working (0 = off). Counts as buff-day 1.
static int32_t g_stopDay=0;           // real day after which progress freezes (0 = off). Given as a REAL day.
static int32_t g_animals=0;
static int32_t g_allow_recruit=1;   // re-enable recruit dialogue for buffed NPCs (DC_IS_RECRUITABLE fix)
static int32_t g_randomize=1;        // 1 = each stat random in [base .. day-scaled max]; 0 = stable (= max)
static int32_t g_hookInit=1, g_hookFrame=1;   // support switches (config; relaunch to apply)
static int32_t g_hubRegister=1;
static int32_t g_inert=0;
static float   g_value=5.0f;
static int32_t g_statEnabled[NSTATS];

// Config persistence. The AUTHORITATIVE copy lives in the GAME ROOT (dir of the Kenshi
// executable) so it survives Steam Workshop updates — those replace the mod's own folder
// (where the DLL sits) but never touch the game root. The copy next to the DLL is LEGACY:
// read only for a one-time migration when no game-root copy exists yet. SaveConfig always
// writes the game-root copy.
static const char* CFG_NAME = "WorldProgression.config";
static std::string g_cfgPath       = "WorldProgression.config"; // authoritative (game root)
static std::string g_cfgPathLegacy = "";                        // legacy (next to DLL)
static const char* g_cfgSrc        = "defaults";                // where values were loaded from (log)

static std::string dirOf(const std::string& full) {
    size_t s = full.find_last_of("\\/");
    return (s==std::string::npos) ? std::string() : full.substr(0, s+1);
}

static void computeConfigPath() {
    // legacy path: next to THIS dll
    HMODULE hm=0;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&computeConfigPath, &hm) && hm) {
        char buf[MAX_PATH]; DWORD n = GetModuleFileNameA(hm, buf, MAX_PATH);
        if (n>0 && n<MAX_PATH) { std::string d = dirOf(std::string(buf,n)); if(!d.empty()) g_cfgPathLegacy = d + CFG_NAME; }
    }
    // authoritative path: game root = dir of the Kenshi executable (module NULL)
    { char buf[MAX_PATH]; DWORD n = GetModuleFileNameA((HMODULE)0, buf, MAX_PATH);
      if (n>0 && n<MAX_PATH) { std::string d = dirOf(std::string(buf,n)); if(!d.empty()) g_cfgPath = d + CFG_NAME; } }
    if (g_cfgPathLegacy.empty()) g_cfgPathLegacy = g_cfgPath;
}

static void SaveConfig() {
    std::ofstream o(g_cfgPath.c_str(), std::ios::out | std::ios::trunc);
    if (!o) return;
    o << "# WorldProgression config\n"
      << "enabled = "         << g_enabled      << "\n"
      << "debug = "           << g_debug        << "   # writes WorldProgression.log\n"
      << "mode = "            << g_mode         << "   # 0 = flat points, 1 = percent (this sets the MAX per period)\n"
      << "value = "           << g_value        << "   # per period: +value points (flat) or +value% (percent)\n"
      << "randomize = "       << g_randomize    << "   # 1 = each stat random between game base and the max (default); 0 = stable growth (every stat = max)\n"
      << "interval_days = "   << g_interval     << "\n"
      << "max_stat = "        << g_max_stat     << "   # 0 = no cap; else our buff never exceeds this. NEVER lowers a stat below its own template base.\n"
      << "start_day = "       << g_startDay     << "   # 0 = off. Real day the mod starts working; it counts as day 1 of the buff logic (before it: NPCs get no buff).\n"
      << "stop_day = "        << g_stopDay      << "   # 0 = off. Real day after which progress freezes (buff acts as if it's always this day). If both set, must be > start_day.\n"
      << "animals = "         << g_animals      << "\n"
      << "allow_recruit = "   << g_allow_recruit<< "   # 1 = buffed NPCs can still be recruited (re-enables recruit dialogue; they join with buffed stats)\n"
      << "language = "        << g_langPref     << "   # menu language: auto (follow the game) / en / ru\n"
      << "# NOTE: the player is never buffed. The mod progresses the WORLD (NPCs); the player grows through normal play.\n"
      << "# --- support switches (applied at game launch; change then RELAUNCH) ---\n"
      << "hook_init = "       << g_hookInit     << "   # install the buff hook (off = mod does nothing to stats)\n"
      << "hook_frame = "      << g_hookFrame    << "   # main-loop hook (game time + hub retry)\n"
      << "hub_register = "    << g_hubRegister  << "   # register the Mod Hub settings tab\n"
      << "inert = "           << g_inert        << "   # 1 = do absolutely nothing at load\n";
    for (int i=0;i<NSTATS;i++) o << kStats[i].id << " = " << g_statEnabled[i] << "\n";
}
static bool readVal(const std::string& s, const char* key, std::string& out) {
    std::string k(key);
    size_t p = 0;
    while ((p = s.find(k, p)) != std::string::npos) {
        bool okL = (p==0) || s[p-1]=='\n' || s[p-1]=='\r' || s[p-1]==' ' || s[p-1]=='\t';
        char after = (p+k.size()<s.size()) ? s[p+k.size()] : '=';
        bool okR = (after==' '||after=='\t'||after=='=');
        if (okL && okR) break;
        p += k.size();
    }
    if (p == std::string::npos) return false;
    p = s.find('=', p); if (p == std::string::npos) return false;
    std::string v = s.substr(p + 1);
    size_t h = v.find('#'); if (h != std::string::npos) v = v.substr(0, h);
    size_t a = v.find_first_not_of(" \t\r\n"); if (a == std::string::npos) { out.clear(); return true; }
    size_t b = v.find_last_not_of(" \t\r\n"); out = v.substr(a, b - a + 1); return true;
}
static int   iget(const std::string& all, const char* k, int d)   { std::string v; if (readVal(all,k,v)&&!v.empty()) return atoi(v.c_str()); return d; }
static float fget(const std::string& all, const char* k, float d) { std::string v; if (readVal(all,k,v)&&!v.empty()) return (float)atof(v.c_str()); return d; }
static void loadConfig() {
    for (int i=0;i<NSTATS;i++) g_statEnabled[i]=1;
    // Pick the source file: the stable game-root copy wins (it holds the newest values,
    // including in-game menu edits); if it doesn't exist yet, fall back to the legacy copy
    // next to the DLL to migrate old settings. Reading is per-key with defaults, so new
    // parameters default and removed ones are ignored — the user keeps every value set.
    std::string src;
    { std::ifstream r(g_cfgPath.c_str()); if (r.good()) { src = g_cfgPath; g_cfgSrc = "game-root"; } }
    if (src.empty() && g_cfgPathLegacy != g_cfgPath) {
        std::ifstream d(g_cfgPathLegacy.c_str());
        if (d.good()) { src = g_cfgPathLegacy; g_cfgSrc = "dll-legacy (migrating -> game root)"; }
    }
    std::string all;
    if (!src.empty()) { std::stringstream ss; { std::ifstream g(src.c_str()); ss << g.rdbuf(); } all = ss.str(); }
    g_enabled=iget(all,"enabled",1); g_debug=iget(all,"debug",1);
    g_mode=iget(all,"mode",0); g_value=fget(all,"value",5.0f);
    g_randomize=iget(all,"randomize",1);
    g_interval=iget(all,"interval_days",5); if (g_interval<1) g_interval=1;
    g_max_stat=iget(all,"max_stat",0); if (g_max_stat<0) g_max_stat=0;
    g_startDay=iget(all,"start_day",0); if (g_startDay<0) g_startDay=0;
    g_stopDay =iget(all,"stop_day",0);  if (g_stopDay<0)  g_stopDay=0;
    g_animals=iget(all,"animals",0);
    g_allow_recruit=iget(all,"allow_recruit",1);
    { std::string lp; if (readVal(all,"language",lp) && !lp.empty()) {
        for (size_t i=0;i<lp.size();++i) lp[i]=(char)tolower((unsigned char)lp[i]);
        if (lp=="en"||lp=="ru"||lp=="auto") g_langPref=lp; else g_langPref="auto"; } }
    g_hookInit=iget(all,"hook_init",1); g_hookFrame=iget(all,"hook_frame",1);
    g_hubRegister=iget(all,"hub_register",1); g_inert=iget(all,"inert",0);
    for (int i=0;i<NSTATS;i++) g_statEnabled[i]=iget(all,kStats[i].id,1);
    // Always normalize + persist to the stable game-root copy, preserving loaded values.
    SaveConfig();
}

// ---- captured GameWorld* (frame hook) for in-game time ----
static GameWorld* g_gw = 0;
static bool g_active = false;

static double currentDay_impl() {
    if (!g_gw) return -1.0;
    TimeOfDay tod = g_gw->getTimeStamp_inGameHours();
    double day = tod.getTotalDays();
    if (!(day >= 0.0 && day < 1.0e7)) return -1.0;
    return day;
}
static double currentDay() {
#ifdef _MSC_VER
    __try { return currentDay_impl(); } __except (EXCEPTION_EXECUTE_HANDLER) { return -1.0; }
#else
    return currentDay_impl();
#endif
}

// ---- buff ----
static long g_nInit=0, g_nBuffed=0, g_nSkip=0;
static int  g_logBudget = 200;
static bool logOk(){ if(g_debug && g_logBudget>0){ g_logBudget--; return true; } return false; }

static void applyBuff_impl(CharStats* cs, Character* ch) {
    if (!g_enabled || !cs || !ch) return;
    // The player is never buffed: the mod progresses the WORLD (NPCs). The player's
    // _NV_init also fires with template-base stats (all ~1) BEFORE the start scenario
    // assigns real stats, so a buff there would be overwritten anyway; and it never
    // re-fires on save-load. So we simply exclude the player.
    if (ch->isPlayerCharacter()) { g_nSkip++; if(logOk()) L("skip: player (world-progression only)"); return; }
    if (!g_animals && ch->isAnimal()) { g_nSkip++; if(logOk()) L("skip: animal"); return; }

    // ---- effective day: honour start_day (activation + rebase) and stop_day (freeze) ----
    // start_day (S): before real day S the mod does nothing; from S on, S counts as buff-day 1
    //   -> effective day = realDay - S + 1.
    // stop_day (E, a REAL day): from real day E on, progress is frozen at E's level
    //   -> effReal = min(realDay, E). If both set, S < E is required (validated at load).
    double dReal = currentDay();
    double dEff  = dReal;
    int periods;
    if (dReal < 0) {
        periods = 1;                    // day unknown -> minimum buff (original fallback); start/stop ignored
    } else {
        if (g_startDay > 0 && dReal < (double)g_startDay) {        // world hasn't started progressing yet
            g_nSkip++; if (logOk()) L("skip: before start_day"); return;
        }
        if (g_stopDay  > 0 && dEff > (double)g_stopDay) dEff = (double)g_stopDay;   // freeze after stop_day
        if (g_startDay > 0) dEff = dEff - (double)g_startDay + 1.0;                 // rebase: start_day = day 1
        if (dEff < 0.0) dEff = 0.0;
        periods = (int)(dEff/(double)g_interval);
        if (periods < 1) periods = 1;
    }

    bool pct=(g_mode==1);
    double mult = pct ? (1.0 + (g_value/100.0)*periods) : 0.0;
    double add  = pct ? 0.0 : ((double)g_value*periods);

    bool capApplies = (g_max_stat>0);

    std::ostringstream dbg; bool wlog=logOk();
    if (wlog) {
        dbg << "BUFF cs=" << (void*)cs
            << " day=" << (int)dReal << " eff=" << (int)dEff << " periods=" << periods << (pct?" x":" +")
            << (g_randomize?" rand":" stable")
            << (capApplies?" cap=":" cap=off ") << (capApplies?g_max_stat:0);
        if (g_startDay>0) dbg << " start=" << g_startDay;
        if (g_stopDay>0)  dbg << " stop="  << g_stopDay;
    }
    for (int i=0;i<NSTATS;i++) {
        if (!g_statEnabled[i]) continue;
        StatsEnumerated s = kStats[i].stat;
        float base = cs->getStat(s,true);
        // "target" = the day-scaled maximum this mod offers for the stat (flat or percent).
        double target = pct ? std::ceil(base*mult) : std::ceil(base+add);
        const char* tag = "";
        // Cap + base-priority applied to the MAX: never above max_stat, never below the
        // character's own base (strong uniques keep their stats).
        if (capApplies && target > (double)g_max_stat) { target = (double)g_max_stat; tag = "C"; }
        if (target < (double)base)                     { target = (double)base;       tag = "K"; }
        double nv;
        if (g_randomize) {
            // Uniform random between the game's base (min) and our max (target). base=min
            // means we never lower a stat; each stat rolls independently -> varied NPCs.
            double frac = (double)std::rand() / (double)RAND_MAX;   // 0..1
            nv = std::floor((double)base + frac * (target - (double)base) + 0.5);
            if (nv < (double)base)   nv = (double)base;
            if (nv > target)         nv = target;
        } else {
            nv = target;   // stable growth = the max (previous behaviour)
        }
        if (nv < 0) nv = 0;
        float& ref = cs->getStatRef(s);
        if (wlog) dbg<<" ["<<(int)s<<":"<<(int)base<<"->"<<(int)nv<<(g_randomize?"r":"")<<tag<<"]";
        ref = (float)nv;
    }
    g_nBuffed++;
    if (wlog) L(dbg.str());
}
static void applyBuff(CharStats* cs, Character* ch) {
#ifdef _MSC_VER
    __try { applyBuff_impl(cs, ch); } __except (EXCEPTION_EXECUTE_HANDLER) { Lc("applyBuff: exception (skipped)"); }
#else
    applyBuff_impl(cs, ch);
#endif
}

static void (*o_init)(CharStats*, GameData*, void*, Character*) = 0;
static void hk_init(CharStats* cs, GameData* statData, void* med, Character* charact) {
    o_init(cs, statData, med, charact);
    g_nInit++;
    if (g_active) applyBuff(cs, charact);
}

// ---- recruit-availability fix ----
// Kenshi gates the recruit dialogue on Dialogue::_checkCondition(DC_IS_RECRUITABLE),
// which is only true while the NPC's combat stats stay low (all <=25 or sum <75). Our
// buff raises those stats, so the check returns FALSE and the recruit dialogue turns
// off -> you can't recruit the character. We don't want to un-buff anyone, so we simply
// answer this ONE condition as "recruitable" regardless of stats. The game always asks
// it as an equality on a boolean (compareBy==0, val==1 -> "recruitable == true"), so we
// emulate recruitable=1: return (val!=0). Any other comparator falls back to the engine.
// The recruited NPC keeps its buffed stats (joins strong) — by design.
static long g_nRecruitFix = 0;
static bool (*o_checkCond)(Dialogue*, DialogConditionEnum, ComparisonEnum, int, Character*, Character*) = 0;
static bool hk_checkCond(Dialogue* self, DialogConditionEnum cond, ComparisonEnum cmp, int val,
                         Character* target, Character* actual) {
    if (g_allow_recruit && cond == DC_IS_RECRUITABLE && (int)cmp == 0) {
        g_nRecruitFix++;
        if (g_debug && g_nRecruitFix <= 15) {
            std::ostringstream o; o<<"recruit-fix: DC_IS_RECRUITABLE -> recruitable (val="<<val<<", #"<<g_nRecruitFix<<")"; L(o.str());
        }
        return (val != 0);
    }
    return o_checkCond(self, cond, cmp, val, target, actual);
}

// ============================================================================
//  Mod Hub  (no AppearanceManager access)
// ============================================================================
static EMC_Result __cdecl cbGetBool (void* ud, int32_t* out){ *out=(*(int32_t*)ud)?1:0; return EMC_OK; }
static EMC_Result __cdecl cbSetBool (void* ud, int32_t v, char*, uint32_t){ *(int32_t*)ud=(v!=0); SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbGetInt  (void* ud, int32_t* out){ *out=*(int32_t*)ud; return EMC_OK; }
static EMC_Result __cdecl cbSetInt  (void* ud, int32_t v, char*, uint32_t){ if(v<0)v=0; *(int32_t*)ud=v; SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbSetInterval(void* ud, int32_t v, char*, uint32_t){ if(v<1)v=1; *(int32_t*)ud=v; SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbGetFloat(void* ud, float* out){ *out=*(float*)ud; return EMC_OK; }
static EMC_Result __cdecl cbSetFloat(void* ud, float v, char*, uint32_t){ if(v<0.0f)v=0.0f; *(float*)ud=v; SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbGetSel  (void* ud, int32_t* out){ *out=*(int32_t*)ud; return EMC_OK; }
static EMC_Result __cdecl cbSetSel  (void* ud, int32_t v, char*, uint32_t){ *(int32_t*)ud=v; SaveConfig(); return EMC_OK; }

static EMC_ModDescriptorV1 g_modDesc;
static const EMC_SelectOptionV1 kModeOpts_en[] = { {0,"Flat points"}, {1,"Percent"} };
static const EMC_SelectOptionV1 kModeOpts_ru[] = {
    {0,"\xD0\x9F\xD0\xBB\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 (+\xD0\xBE\xD1\x87\xD0\xBA\xD0\xB8)"},   // Плоский (+очки)
    {1,"\xD0\x9F\xD1\x80\xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD1\x82 (%)"}                                     // Процент (%)
};
static const EMC_HubApiV1* g_api=0; static uint32_t g_apiSize=0; static EMC_ModHandle g_mod=0;

static void regBool(const char* id, const char* label, const char* desc, int32_t* field){
    EMC_BoolSettingDefV1 d={id,label,desc,field,&cbGetBool,&cbSetBool}; g_api->register_bool_setting(g_mod,&d); }
static void regIntF(const char* id, const char* label, const char* desc, int32_t lo,int32_t hi,int32_t step,int32_t* field, EMC_SetIntCallback setcb){
    EMC_IntSettingDefV1 d={id,label,desc,field,lo,hi,step,&cbGetInt,setcb}; g_api->register_int_setting(g_mod,&d); }
static void regFloat(const char* id, const char* label, const char* desc, float lo,float hi,float step,uint32_t dec,float* field){
    EMC_FloatSettingDefV1 d={id,label,desc,field,lo,hi,step,dec,&cbGetFloat,&cbSetFloat}; g_api->register_float_setting(g_mod,&d); }
static void regSelect(const char* id, const char* label, const char* desc, int32_t* field, const EMC_SelectOptionV1* opts, uint32_t count){
    if (g_apiSize>=EMC_HUB_API_V1_SELECT_SETTING_MIN_SIZE && g_api->register_select_setting){
        EMC_SelectSettingDefV1 d={id,label,desc,field,opts,count,&cbGetSel,&cbSetSel}; g_api->register_select_setting(g_mod,&d); } }
static void section(const char* settingId, const char* sectionId, const char* sectionName){
    if (g_apiSize>=EMC_HUB_API_V1_SETTING_SECTION_MIN_SIZE && g_api->register_setting_section){
        EMC_SettingSectionDefV1 d; d.setting_id=settingId; d.section_id=sectionId; d.section_display_name=sectionName;
        g_api->register_setting_section(g_mod,&d); } }
static void disableWhen(const char* target, const char* controller, int expected){
    if (g_apiSize>=EMC_HUB_API_V1_BOOL_CONDITION_RULE_MIN_SIZE && g_api->register_bool_condition_rule){
        EMC_BoolConditionRuleDefV1 d; d.target_setting_id=target; d.controller_setting_id=controller;
        d.effect=EMC_BOOL_CONDITION_EFFECT_DISABLE; d.expected_bool_value=expected;
        g_api->register_bool_condition_rule(g_mod,&d); } }

static bool g_hubHardFail=false;
static bool DoRegisterModHub() {
    static const char* kNames[] = { "Emkejs-Mod-Core.dll", "Emkejs-Mod-Core" };
    typedef EMC_Result(__cdecl* GetApiFn)(uint32_t, uint32_t, const EMC_HubApiV1**, uint32_t*);
    GetApiFn getApi=0;
    for (size_t i=0;i<sizeof(kNames)/sizeof(kNames[0]) && !getApi;++i){ HMODULE h=GetModuleHandleA(kNames[i]); if(!h)continue;
        FARPROC p=GetProcAddress(h,EMC_MOD_HUB_GET_API_EXPORT_NAME); if(!p)p=GetProcAddress(h,EMC_MOD_HUB_GET_API_COMPAT_EXPORT_NAME);
        if(p) getApi=(GetApiFn)p; }
    if(!getApi){ HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());
        if(snap!=INVALID_HANDLE_VALUE){ MODULEENTRY32 me; me.dwSize=sizeof(me);
            if(Module32First(snap,&me)){ do{ FARPROC p=GetProcAddress(me.hModule,EMC_MOD_HUB_GET_API_EXPORT_NAME);
                if(!p)p=GetProcAddress(me.hModule,EMC_MOD_HUB_GET_API_COMPAT_EXPORT_NAME);
                if(p){ getApi=(GetApiFn)p; break; } }while(Module32Next(snap,&me)); } CloseHandle(snap); } }
    if(!getApi) return false;

    EMC_Result gr=getApi(EMC_HUB_API_VERSION_1, EMC_HUB_API_V1_MIN_SIZE, &g_api, &g_apiSize);
    if (gr!=EMC_OK || !g_api || g_apiSize<EMC_HUB_API_V1_MIN_SIZE){ L("ModHub: GetApi failed"); g_hubHardFail=true; return true; }
    g_modDesc.namespace_id="emkej.qol"; g_modDesc.namespace_display_name="Emkej QoL";
    g_modDesc.mod_id="world_progression";
    g_modDesc.mod_display_name=tr("World Progression","\xD0\x9F\xD1\x80\xD0\xBE\xD0\xB3\xD1\x80\xD0\xB5\xD1\x81\xD1\x81\xD0\xB8\xD1\x8F \xD0\xBC\xD0\xB8\xD1\x80\xD0\xB0"); // Прогрессия мира
    g_modDesc.mod_user_data=0;
    EMC_Result rr=g_api->register_mod(&g_modDesc,&g_mod);
    if (rr==EMC_ERR_CONFLICT){ L("ModHub: already registered"); return true; }
    if (rr!=EMC_OK || !g_mod){ L("ModHub: register_mod failed"); g_hubHardFail=true; return true; }
    L("ModHub: attached.");

    regBool ("wp_enabled",
             tr("Enabled","\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xBE"),               // Включено
             tr("Turn the whole mod on/off.","\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C/\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB2\xD0\xB5\xD1\x81\xD1\x8C \xD0\xBC\xD0\xBE\xD0\xB4."),
             &g_enabled);
    regBool ("wp_debug",
             tr("Debug log","\xD0\x9E\xD1\x82\xD0\xBB\xD0\xB0\xD0\xB4\xD0\xBE\xD1\x87\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xBB\xD0\xBE\xD0\xB3"),  // Отладочный лог
             tr("Write WorldProgression.log in the game root.","\xD0\x9F\xD0\xB8\xD1\x81\xD0\xB0\xD1\x82\xD1\x8C WorldProgression.log \xD0\xB2 \xD0\xBA\xD0\xBE\xD1\x80\xD0\xBD\xD0\xB5 \xD0\xB8\xD0\xB3\xD1\x80\xD1\x8B."),
             &g_debug);
    regSelect("wp_mode",
             tr("Bonus type","\xD0\xA2\xD0\xB8\xD0\xBF \xD0\xB1\xD0\xBE\xD0\xBD\xD1\x83\xD1\x81\xD0\xB0"),      // Тип бонуса
             tr("Flat points, or percent (percent compounds toward late game).","\xD0\x9F\xD0\xBB\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB5 \xD0\xBE\xD1\x87\xD0\xBA\xD0\xB8 \xD0\xB8\xD0\xBB\xD0\xB8 \xD0\xBF\xD1\x80\xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD1\x82\xD1\x8B (\xD0\xBF\xD1\x80\xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD1\x82\xD1\x8B \xD1\x81\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xB5\xD0\xB5 \xD0\xBA \xD0\xBF\xD0\xBE\xD0\xB7\xD0\xB4\xD0\xBD\xD0\xB5\xD0\xB9 \xD0\xB8\xD0\xB3\xD1\x80\xD0\xB5)."),
             &g_mode, (g_lang==LANG_RU?kModeOpts_ru:kModeOpts_en), 2);
    regFloat("wp_value",
             tr("Bonus per period","\xD0\x91\xD0\xBE\xD0\xBD\xD1\x83\xD1\x81 \xD0\xB7\xD0\xB0 \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB8\xD0\xBE\xD0\xB4"),  // Бонус за период
             tr("Per period: +N points (flat) or +N% (percent).","\xD0\x97\xD0\xB0 \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB8\xD0\xBE\xD0\xB4: +N \xD0\xBE\xD1\x87\xD0\xBA\xD0\xBE\xD0\xB2 (\xD0\xBF\xD0\xBB\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9) \xD0\xB8\xD0\xBB\xD0\xB8 +N% (\xD0\xBF\xD1\x80\xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD1\x82)."),
             0.0f,10000.0f,1.0f,2u,&g_value);
    regIntF ("wp_interval",
             tr("Interval (days)","\xD0\x98\xD0\xBD\xD1\x82\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB0\xD0\xBB (\xD0\xB4\xD0\xBD\xD0\xB8)"),  // Интервал (дни)
             tr("Every N in-game days the bonus grows one step.","\xD0\x9A\xD0\xB0\xD0\xB6\xD0\xB4\xD1\x8B\xD0\xB5 N \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xB2\xD1\x8B\xD1\x85 \xD0\xB4\xD0\xBD\xD0\xB5\xD0\xB9 \xD0\xB1\xD0\xBE\xD0\xBD\xD1\x83\xD1\x81 \xD1\x80\xD0\xB0\xD1\x81\xD1\x82\xD1\x91\xD1\x82 \xD0\xBD\xD0\xB0 \xD1\x88\xD0\xB0\xD0\xB3."),
             1,3650,1,&g_interval,&cbSetInterval);
    regIntF ("wp_max_stat",
             tr("Max stat (cap)","\xD0\x9F\xD0\xBE\xD1\x82\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xBA \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD0\xB0 (\xD0\xBA\xD0\xB0\xD0\xBF)"),  // Потолок стата (кап)
             tr("0 = no cap. Otherwise our buff never exceeds this. Never lowers a stat below its own template base (strong uniques keep their stats).","0 = \xD0\xB1\xD0\xB5\xD0\xB7 \xD0\xBA\xD0\xB0\xD0\xBF\xD0\xB0. \xD0\x98\xD0\xBD\xD0\xB0\xD1\x87\xD0\xB5 \xD0\xBD\xD0\xB0\xD1\x88 \xD0\xB1\xD0\xB0\xD1\x84 \xD0\xBD\xD0\xB5 \xD0\xBF\xD1\x80\xD0\xB5\xD0\xB2\xD1\x8B\xD1\x88\xD0\xB0\xD0\xB5\xD1\x82 \xD1\x8D\xD1\x82\xD0\xBE. \xD0\x9D\xD0\xB8\xD0\xBA\xD0\xBE\xD0\xB3\xD0\xB4\xD0\xB0 \xD0\xBD\xD0\xB5 \xD0\xBE\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA\xD0\xB0\xD0\xB5\xD1\x82 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82 \xD0\xBD\xD0\xB8\xD0\xB6\xD0\xB5 \xD0\xB5\xD0\xB3\xD0\xBE \xD0\xB1\xD0\xB0\xD0\xB7\xD1\x8B \xD1\x88\xD0\xB0\xD0\xB1\xD0\xBB\xD0\xBE\xD0\xBD\xD0\xB0 (\xD1\x81\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB5 \xD1\x83\xD0\xBD\xD0\xB8\xD0\xBA\xD0\xB0\xD0\xBB\xD1\x8B \xD1\x81\xD0\xBE\xD1\x85\xD1\x80\xD0\xB0\xD0\xBD\xD1\x8F\xD1\x8E\xD1\x82 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD1\x8B)."),
             0,10000,5,&g_max_stat,&cbSetInt);
    regIntF ("wp_start_day",
             tr("Start day (0=off)","\xD0\x94\xD0\xB5\xD0\xBD\xD1\x8C\x20\xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD0\xB0\x20\x28\x30\x3D\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\x29"),
             tr("Real day the mod begins working; it counts as day 1 of the buff logic. Before it, new NPCs get no buff. 0 = off (works from the start).","\xD0\xA0\xD0\xB5\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB9\x20\xD0\xB4\xD0\xB5\xD0\xBD\xD1\x8C\x2C\x20\xD1\x81\x20\xD0\xBA\xD0\xBE\xD1\x82\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB3\xD0\xBE\x20\xD0\xBC\xD0\xBE\xD0\xB4\x20\xD0\xBD\xD0\xB0\xD1\x87\xD0\xB8\xD0\xBD\xD0\xB0\xD0\xB5\xD1\x82\x20\xD1\x80\xD0\xB0\xD0\xB1\xD0\xBE\xD1\x82\xD0\xB0\xD1\x82\xD1\x8C\x3B\x20\xD0\xBE\xD0\xBD\x20\xD1\x81\xD1\x87\xD0\xB8\xD1\x82\xD0\xB0\xD0\xB5\xD1\x82\xD1\x81\xD1\x8F\x20\xD0\xB4\xD0\xBD\xD1\x91\xD0\xBC\x20\x31\x20\xD0\xBB\xD0\xBE\xD0\xB3\xD0\xB8\xD0\xBA\xD0\xB8\x20\xD0\xB1\xD0\xB0\xD1\x84\xD0\xB0\x2E\x20\xD0\x94\xD0\xBE\x20\xD0\xBD\xD0\xB5\xD0\xB3\xD0\xBE\x20\xD0\xBD\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB5\x20\x4E\x50\x43\x20\xD0\xBD\xD0\xB5\x20\xD0\xB1\xD0\xB0\xD1\x84\xD0\xB0\xD1\x8E\xD1\x82\xD1\x81\xD1\x8F\x2E\x20\x30\x20\x3D\x20\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\x20\x28\xD1\x80\xD0\xB0\xD0\xB1\xD0\xBE\xD1\x82\xD0\xB0\xD0\xB5\xD1\x82\x20\xD1\x81\x20\xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD0\xB0\x29\x2E"),
             0,3650,1,&g_startDay,&cbSetInt);
    regIntF ("wp_stop_day",
             tr("Stop day (0=off)","\xD0\x94\xD0\xB5\xD0\xBD\xD1\x8C\x20\xD0\xBE\xD1\x81\xD1\x82\xD0\xB0\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB8\x20\x28\x30\x3D\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\x29"),
             tr("Real day after which progress freezes: the buff acts as if it's always this day. Must be greater than Start day. 0 = off (grows forever).","\xD0\xA0\xD0\xB5\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB9\x20\xD0\xB4\xD0\xB5\xD0\xBD\xD1\x8C\x2C\x20\xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5\x20\xD0\xBA\xD0\xBE\xD1\x82\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB3\xD0\xBE\x20\xD1\x80\xD0\xBE\xD1\x81\xD1\x82\x20\xD0\xB7\xD0\xB0\xD0\xBC\xD0\xB8\xD1\x80\xD0\xB0\xD0\xB5\xD1\x82\x3A\x20\xD0\xB1\xD0\xB0\xD1\x84\x20\xD0\xB2\xD0\xB5\xD0\xB4\xD1\x91\xD1\x82\x20\xD1\x81\xD0\xB5\xD0\xB1\xD1\x8F\x20\xD0\xBA\xD0\xB0\xD0\xBA\x20\xD0\xB1\xD1\x83\xD0\xB4\xD1\x82\xD0\xBE\x20\xD0\xB2\xD1\x81\xD0\xB5\xD0\xB3\xD0\xB4\xD0\xB0\x20\xD1\x8D\xD1\x82\xD0\xBE\xD1\x82\x20\xD0\xB4\xD0\xB5\xD0\xBD\xD1\x8C\x2E\x20\xD0\x94\xD0\xBE\xD0\xBB\xD0\xB6\xD0\xB5\xD0\xBD\x20\xD0\xB1\xD1\x8B\xD1\x82\xD1\x8C\x20\xD0\xB1\xD0\xBE\xD0\xBB\xD1\x8C\xD1\x88\xD0\xB5\x20\xD0\x94\xD0\xBD\xD1\x8F\x20\xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD0\xB0\x2E\x20\x30\x20\x3D\x20\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\x20\x28\xD1\x80\xD0\xB0\xD1\x81\xD1\x82\xD1\x91\xD1\x82\x20\xD0\xB2\xD1\x81\xD0\xB5\xD0\xB3\xD0\xB4\xD0\xB0\x29\x2E"),
             0,3650,1,&g_stopDay,&cbSetInt);
    regBool ("wp_randomize",
             tr("Randomize stats (range)","\xD0\xA1\xD0\xBB\xD1\x83\xD1\x87\xD0\xB0\xD0\xB9\xD0\xBD\xD1\x8B\xD0\xB5\x20\xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD1\x8B\x20\x28\xD0\xB4\xD0\xB8\xD0\xB0\xD0\xBF\xD0\xB0\xD0\xB7\xD0\xBE\xD0\xBD\x29"),  // Случайные статы (диапазон)
             tr("Each new NPC gets each stat as a random value between the game's base and the mod's day-scaled maximum (using the chosen flat/percent type). Turn OFF for stable growth: every stat is set exactly to that maximum.","\xD0\x9A\xD0\xB0\xD0\xB6\xD0\xB4\xD1\x8B\xD0\xB9\x20\xD0\xBD\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9\x20\x4E\x50\x43\x20\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x83\xD1\x87\xD0\xB0\xD0\xB5\xD1\x82\x20\xD0\xBA\xD0\xB0\xD0\xB6\xD0\xB4\xD1\x8B\xD0\xB9\x20\xD1\x81\xD1\x82\xD0\xB0\xD1\x82\x20\xD1\x81\xD0\xBB\xD1\x83\xD1\x87\xD0\xB0\xD0\xB9\xD0\xBD\xD1\x8B\xD0\xBC\x20\xD0\xB7\xD0\xBD\xD0\xB0\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5\xD0\xBC\x20\xD0\xBC\xD0\xB5\xD0\xB6\xD0\xB4\xD1\x83\x20\xD0\xB1\xD0\xB0\xD0\xB7\xD0\xBE\xD0\xB9\x20\xD0\xB8\xD0\xB3\xD1\x80\xD1\x8B\x20\xD0\xB8\x20\xD0\xBC\xD0\xB0\xD0\xBA\xD1\x81\xD0\xB8\xD0\xBC\xD1\x83\xD0\xBC\xD0\xBE\xD0\xBC\x20\xD0\xBC\xD0\xBE\xD0\xB4\xD0\xB0\x20\x28\xD1\x81\x20\xD1\x83\xD1\x87\xD1\x91\xD1\x82\xD0\xBE\xD0\xBC\x20\xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBE\xD0\xB3\xD0\xBE\x20\xD0\xB4\xD0\xBD\xD1\x8F\x20\xD0\xB8\x20\xD0\xB2\xD1\x8B\xD0\xB1\xD1\x80\xD0\xB0\xD0\xBD\xD0\xBD\xD0\xBE\xD0\xB3\xD0\xBE\x20\xD1\x82\xD0\xB8\xD0\xBF\xD0\xB0\x3A\x20\xD0\xBF\xD0\xBB\xD0\xBE\xD1\x81\xD0\xBA\xD0\xBE\x2F\xD0\xBF\xD1\x80\xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD1\x82\x29\x2E\x20\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\x20\xD0\xB4\xD0\xBB\xD1\x8F\x20\xD1\x81\xD1\x82\xD0\xB0\xD0\xB1\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE\xD0\xB3\xD0\xBE\x20\xD1\x80\xD0\xBE\xD1\x81\xD1\x82\xD0\xB0\x20\xE2\x80\x94\x20\xD0\xB2\xD1\x81\xD0\xB5\x20\xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD1\x8B\x20\xD1\x81\xD1\x82\xD0\xB0\xD0\xB2\xD1\x8F\xD1\x82\xD1\x81\xD1\x8F\x20\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBD\xD0\xBE\x20\xD0\xB2\x20\xD1\x8D\xD1\x82\xD0\xBE\xD1\x82\x20\xD0\xBC\xD0\xB0\xD0\xBA\xD1\x81\xD0\xB8\xD0\xBC\xD1\x83\xD0\xBC\x2E"),
             &g_randomize);
    regBool ("wp_animals",
             tr("Also animals","\xD0\xA2\xD0\xB0\xD0\xBA\xD0\xB6\xD0\xB5 \xD0\xB6\xD0\xB8\xD0\xB2\xD0\xBE\xD1\x82\xD0\xBD\xD1\x8B\xD0\xB5"),  // Также животные
             tr("Apply to animals too. The player is never affected (world-progression only).","\xD0\x9F\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x8F\xD1\x82\xD1\x8C \xD0\xB8 \xD0\xBA \xD0\xB6\xD0\xB8\xD0\xB2\xD0\xBE\xD1\x82\xD0\xBD\xD1\x8B\xD0\xBC. \xD0\x98\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA \xD0\xBD\xD0\xB5 \xD0\xB7\xD0\xB0\xD1\x82\xD1\x80\xD0\xB0\xD0\xB3\xD0\xB8\xD0\xB2\xD0\xB0\xD0\xB5\xD1\x82\xD1\x81\xD1\x8F \xD0\xBD\xD0\xB8\xD0\xBA\xD0\xBE\xD0\xB3\xD0\xB4\xD0\xB0."),
             &g_animals);
    regBool ("wp_allow_recruit",
             tr("Allow recruiting buffed NPCs","\xD0\xA0\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB5\xD1\x88\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB2\xD0\xB5\xD1\x80\xD0\xB1\xD0\xBE\xD0\xB2\xD0\xBA\xD1\x83 \xD1\x83\xD1\x81\xD0\xB8\xD0\xBB\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD1\x85 \x4E\x50\x43"),
             tr("Buffed recruits normally can't be recruited: Kenshi disables the recruit dialogue when combat stats get high. This re-enables their recruit dialogue so you can still hire them (they join with their buffed stats).","\xD0\xA3\xD1\x81\xD0\xB8\xD0\xBB\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD1\x85 \xD1\x80\xD0\xB5\xD0\xBA\xD1\x80\xD1\x83\xD1\x82\xD0\xBE\xD0\xB2 \xD0\xBE\xD0\xB1\xD1\x8B\xD1\x87\xD0\xBD\xD0\xBE \xD0\xBD\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x8F \xD0\xB7\xD0\xB0\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB1\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C\x3A \x4B\x65\x6E\x73\x68\x69 \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB0\xD0\xB5\xD1\x82 \xD0\xB4\xD0\xB8\xD0\xB0\xD0\xBB\xD0\xBE\xD0\xB3 \xD0\xBD\xD0\xB0\xD0\xB9\xD0\xBC\xD0\xB0, \xD0\xBA\xD0\xBE\xD0\xB3\xD0\xB4\xD0\xB0 \xD0\xB1\xD0\xBE\xD0\xB5\xD0\xB2\xD1\x8B\xD0\xB5 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD1\x8B \xD0\xB2\xD1\x8B\xD1\x81\xD0\xBE\xD0\xBA\xD0\xB8\xD0\xB5. \xD0\xAD\xD1\x82\xD0\xBE \xD1\x81\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB0 \xD0\xB2\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB0\xD0\xB5\xD1\x82 \xD0\xB8\xD1\x85 \xD0\xB4\xD0\xB8\xD0\xB0\xD0\xBB\xD0\xBE\xD0\xB3 \xD0\xBD\xD0\xB0\xD0\xB9\xD0\xBC\xD0\xB0, \xD1\x87\xD1\x82\xD0\xBE\xD0\xB1\xD1\x8B \xD0\xB8\xD1\x85 \xD0\xBC\xD0\xBE\xD0\xB6\xD0\xBD\xD0\xBE \xD0\xB1\xD1\x8B\xD0\xBB\xD0\xBE \xD0\xBD\xD0\xB0\xD0\xBD\xD1\x8F\xD1\x82\xD1\x8C \xE2\x80\x94 \xD0\xB2 \xD0\xBE\xD1\x82\xD1\x80\xD1\x8F\xD0\xB4 \xD0\xBE\xD0\xBD\xD0\xB8 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x85\xD0\xBE\xD0\xB4\xD1\x8F\xD1\x82 \xD1\x81 \xD0\xB1\xD0\xB0\xD1\x84\xD0\xBD\xD1\x83\xD1\x82\xD1\x8B\xD0\xBC\xD0\xB8 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD0\xB0\xD0\xBC\xD0\xB8."),
             &g_allow_recruit);

    const char* charsSection = tr("Characteristics","\xD0\xA5\xD0\xB0\xD1\x80\xD0\xB0\xD0\xBA\xD1\x82\xD0\xB5\xD1\x80\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD0\xBA\xD0\xB8"); // Характеристики
    const char* charsDesc = tr("Include this characteristic in the buff.","\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD1\x8D\xD1\x82\xD1\x83 \xD1\x85\xD0\xB0\xD1\x80\xD0\xB0\xD0\xBA\xD1\x82\xD0\xB5\xD1\x80\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD0\xBA\xD1\x83 \xD0\xB2 \xD0\xB1\xD0\xB0\xD1\x84."); // Включить эту характеристику в баф.
    for (int i=0;i<NSTATS;i++) {
        regBool(kStats[i].id, tr(kStats[i].label_en, kStats[i].label_ru), charsDesc, &g_statEnabled[i]);
        section(kStats[i].id, "wp_chars", charsSection);
    }

    L("ModHub: rows registered.");
    return true;
}

static int g_hubState=0, g_hubAcc=0, g_hubTries=0;
static const int HUB_THROTTLE=20, HUB_MAXTRIES=600;
static void HubTick() {
    if (!g_hubRegister) return;
    if (g_hubState!=0) return;
    if (++g_hubAcc<HUB_THROTTLE) return; g_hubAcc=0;
    if (++g_hubTries>HUB_MAXTRIES){ g_hubState=-1; L("ModHub: gave up retrying."); return; }
    if (DoRegisterModHub()) g_hubState = g_hubHardFail?-1:1;
}

static void (*frame_orig)(GameWorld*, float) = 0;
static int  g_sumAcc=0; static long g_sumLast=-1;
static void frame_hook(GameWorld* gw, float t){
    g_gw = gw; HubTick();
    if (g_debug && ++g_sumAcc>=1200) { g_sumAcc=0;
        long tot=g_nInit+g_nBuffed;
        if (tot!=g_sumLast) { g_sumLast=tot; std::ostringstream o;
            o<<"counters: init="<<g_nInit<<" buffed="<<g_nBuffed<<" skipped="<<g_nSkip; L(o.str()); }
    }
    frame_orig(gw, t);
}

__declspec(dllexport) void startPlugin() {
    computeConfigPath();
    loadConfig();
    DetectLanguage();                               // resolve menu language before hub rows
    std::srand((unsigned)GetTickCount());            // seed RNG for randomized spawn stats
    g_log = new std::ofstream("WorldProgression.log", std::ios::out | std::ios::trunc);
    L("WorldProgression v21 (start_day / stop_day window; config in game root) loaded.");
    { std::ostringstream o; o<<"config file (game root): "<<g_cfgPath; L(o.str()); }
    { std::ostringstream o; o<<"config legacy (next to dll): "<<g_cfgPathLegacy; L(o.str()); }
    { std::ostringstream o; o<<"config loaded from: "<<g_cfgSrc; L(o.str()); }
    { std::ostringstream o; o<<"language: pref="<<g_langPref<<" -> "<<(g_lang==LANG_RU?"ru":"en")<<" (from Kenshi settings.cfg)"; L(o.str()); }
    if (g_inert) { L("INERT MODE: doing nothing at load."); return; }
    // Validate the start/stop window: if both are set, start must be before stop; else ignore stop.
    if (g_startDay>0 && g_stopDay>0 && g_startDay >= g_stopDay) {
        std::ostringstream o; o<<"WARN: start_day ("<<g_startDay<<") >= stop_day ("<<g_stopDay
            <<") -> ignoring stop_day"; L(o.str());
        g_stopDay = 0;
    }
    { std::ostringstream o; o<<"config: enabled="<<g_enabled<<" mode="<<(g_mode?"percent":"flat")<<" value="<<g_value
        <<" N="<<g_interval<<" max_stat="<<g_max_stat<<" randomize="<<g_randomize
        <<" start_day="<<g_startDay<<" stop_day="<<g_stopDay
        <<" animals="<<g_animals<<" allow_recruit="<<g_allow_recruit<<" (player: never buffed)"; L(o.str()); }

    if (g_hookInit) {
        if (KenshiLib::SUCCESS==KenshiLib::AddHook(KenshiLib::GetRealAddress(&CharStats::_NV_init), &hk_init, &o_init)) {
            g_active=true; L("hook installed: CharStats::_NV_init (buff).");
        } else L("hook FAILED: CharStats::_NV_init");
    } else L("hook SKIPPED (config): CharStats::_NV_init -> no buffing this run.");

    if (g_hookFrame) {
        if (KenshiLib::SUCCESS==KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), &frame_hook, &frame_orig))
            L("frame hook installed (game time + hub retry).");
        else L("frame hook FAILED.");
    } else L("hook SKIPPED (config): GameWorld main loop.");

    // Recruit-availability fix: installed always; behavior gated by the live toggle
    // g_allow_recruit (off = passes straight through to the engine).
    if (KenshiLib::SUCCESS==KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_checkCondition), &hk_checkCond, &o_checkCond))
        L("hook installed: Dialogue::_checkCondition (recruit-availability fix).");
    else L("hook FAILED: Dialogue::_checkCondition (recruit fix unavailable).");

    if (!g_hubRegister) { L("ModHub: registration DISABLED by config."); return; }
    if (DoRegisterModHub()) g_hubState = g_hubHardFail?-1:1;
    else L("ModHub: not attached yet -> retrying each frame.");
}
