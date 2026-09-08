#include "ImpossibleLevelsAPI.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/general.hpp>
#include <algorithm>
#include <cctype>

using namespace geode::prelude;

namespace ill {

    ImpossibleLevelsAPI* ImpossibleLevelsAPI::s_instance = nullptr;

    ImpossibleLevelsAPI* ImpossibleLevelsAPI::get() {
        if (!s_instance) s_instance = new ImpossibleLevelsAPI();
        return s_instance;
    }

    std::string ImpossibleLevelsAPI::baseUrl() const {
        return Mod::get()->getSettingValue<std::string>("api-base-url");
    }

    void ImpossibleLevelsAPI::openWebsite(std::string const& path) {
        geode::utils::web::openLinkInBrowser("https://impossiblelevels.com" + path);
    }

    // ------------------------------------------------------------------
    // Petits utilitaires de parsing tolérant
    // ------------------------------------------------------------------
    namespace {
        // Essaie plusieurs clés possibles dans un objet JSON et renvoie la
        // première trouvée sous forme de chaîne (peu importe le type source).
        std::string tryKeysStr(matjson::Value const& j, std::initializer_list<const char*> keys, std::string const& def = "") {
            for (auto* key : keys) {
                if (j.contains(key)) {
                    auto& v = j[key];
                    if (v.isString()) return v.asString().unwrapOr(def);
                    if (v.isNumber()) return std::to_string(v.asDouble().unwrapOr(0.0));
                    if (v.isBool())   return v.asBool().unwrapOr(false) ? "true" : "false";
                }
            }
            return def;
        }

        double tryKeysNum(matjson::Value const& j, std::initializer_list<const char*> keys, double def = 0.0) {
            for (auto* key : keys) {
                if (j.contains(key)) {
                    auto& v = j[key];
                    if (v.isNumber()) return v.asDouble().unwrapOr(def);
                    if (v.isString()) {
                        try { return std::stod(v.asString().unwrapOr("0")); } catch (...) {}
                    }
                }
            }
            return def;
        }

        bool tryKeysBool(matjson::Value const& j, std::initializer_list<const char*> keys, bool def = false) {
            for (auto* key : keys) {
                if (j.contains(key) && j[key].isBool()) return j[key].asBool().unwrapOr(def);
            }
            return def;
        }

        // Convertit une date ISO 8601 ("2026-08-30T12:00:00Z" ou "2026-08-30")
        // ou un timestamp déjà numérique en epoch secondes.
        long long parseDateToEpoch(matjson::Value const& j, std::initializer_list<const char*> keys) {
            for (auto* key : keys) {
                if (!j.contains(key)) continue;
                auto& v = j[key];

                if (v.isNumber()) {
                    double num = v.asDouble().unwrapOr(0.0);
                    // Si c'est en millisecondes (13 chiffres), convertir en secondes
                    if (num > 1e12) num /= 1000.0;
                    return static_cast<long long>(num);
                }

                if (v.isString()) {
                    std::string s = v.asString().unwrapOr("");
                    if (s.size() < 10) continue;
                    struct tm tmVal{};
                    // Format attendu: YYYY-MM-DDTHH:MM:SS ou YYYY-MM-DD
                    int y, mo, d, h = 0, mi = 0, se = 0;
                    int matched = sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se);
                    if (matched < 3) {
                        matched = sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &d);
                    }
                    if (matched >= 3) {
                        tmVal.tm_year = y - 1900;
                        tmVal.tm_mon = mo - 1;
                        tmVal.tm_mday = d;
                        tmVal.tm_hour = h;
                        tmVal.tm_min = mi;
                        tmVal.tm_sec = se;
                        #if defined(_WIN32)
                        return static_cast<long long>(_mkgmtime(&tmVal));
                        #else
                        return static_cast<long long>(timegm(&tmVal));
                        #endif
                    }
                }
            }
            return 0;
        }
    }

    ImpossibleLevel ImpossibleLevel::fromJson(matjson::Value const& j) {
        ImpossibleLevel lvl;

        lvl.rank = static_cast<int>(tryKeysNum(j, {"rank", "placement", "position", "rank_position"}, 0));

        lvl.levelID = static_cast<int>(tryKeysNum(j, {
            "levelID", "levelId", "level_id", "gdLevelId", "gd_id", "id_level", "levelid"
        }, 0));

        lvl.name = tryKeysStr(j, {"name", "levelName", "level_name", "title"}, "Inconnu");
        lvl.creator = tryKeysStr(j, {"creator", "creatorName", "creator_name", "author", "publisher"}, "Inconnu");

        lvl.fps = tryKeysNum(j, {"fps", "frameRate", "frame_rate"}, 60.0);

        lvl.length = tryKeysStr(j, {"length", "duration", "levelLength"}, "");
        lvl.difficulty = tryKeysStr(j, {"difficulty", "tier", "tierName", "difficultyName"}, "");

        lvl.videoUrl = tryKeysStr(j, {"video", "videoUrl", "video_url", "showcase", "verification"}, "");
        lvl.thumbnailUrl = tryKeysStr(j, {"thumbnail", "thumbnailUrl", "thumbnail_url", "image", "cover"}, "");
        lvl.recordsUrl = tryKeysStr(j, {"records", "recordsUrl", "records_url", "leaderboard"}, "");

        lvl.verified = tryKeysBool(j, {"verified", "isVerified", "completed"}, false);

        lvl.addedTimestamp = parseDateToEpoch(j, {
            "createdAt", "created_at", "dateAdded", "date_added", "addedAt",
            "added_at", "creationDate", "creation_date", "publishedAt"
        });

        return lvl;
    }

    bool ImpossibleLevel::isNewerThan(int days) const {
        if (addedTimestamp <= 0) return false;
        long long now = static_cast<long long>(std::time(nullptr));
        long long windowSeconds = static_cast<long long>(days) * 24 * 60 * 60;
        return (now - addedTimestamp) <= windowSeconds && (now - addedTimestamp) >= 0;
    }

    // ------------------------------------------------------------------
    // Récupération réseau
    // ------------------------------------------------------------------
    void ImpossibleLevelsAPI::fetchLevels(
        bool force,
        std::function<void(std::vector<ImpossibleLevel> const&, bool, std::string)> callback
    ) {
        long long now = static_cast<long long>(std::time(nullptr));
        int cacheMinutes = Mod::get()->getSettingValue<int64_t>("cache-minutes");

        if (!force && !m_cachedLevels.empty() && cacheMinutes > 0 &&
            (now - m_lastFetchTime) < (cacheMinutes * 60)) {
            callback(m_cachedLevels, true, "");
            return;
        }

        if (m_isFetching) {
            // Un fetch est déjà en cours : on renvoie juste ce qu'on a pour
            // éviter d'empiler les requêtes.
            callback(m_cachedLevels, !m_cachedLevels.empty(), "Un chargement est déjà en cours...");
            return;
        }

        m_isFetching = true;

        // Endpoint principal supposé. A ajuster si besoin dans les settings
        // du mod ("URL de base de l'API") sans recompiler.
        std::string url = baseUrl() + "/levels";

        auto req = web::WebRequest();
        req.userAgent("ImpossibleLevelsGeodeMod/1.0");

        req.get(url).listen([this, callback](web::WebResponse* res) {
            m_isFetching = false;

            if (!res || !res->ok()) {
                std::string err = res ? fmt::format("Erreur HTTP {}", res->code()) : "Pas de réponse du serveur";
                log::warn("[ImpossibleLevels] Echec de la requete API: {}", err);
                callback(m_cachedLevels, false, err);
                return;
            }

            auto jsonRes = res->json();
            if (!jsonRes) {
                log::warn("[ImpossibleLevels] Reponse non-JSON recue.");
                callback(m_cachedLevels, false, "Reponse invalide (pas du JSON)");
                return;
            }

            matjson::Value root = jsonRes.unwrap();

            // Log de debug : décommente/consulte via la console Geode si le
            // parsing ne donne rien de correct, pour voir la vraie structure.
            log::debug("[ImpossibleLevels] Raw JSON (tronque): {}", root.dump().substr(0, 1500));

            // L'API peut renvoyer soit un tableau direct, soit un objet
            // wrapper du style { "levels": [...] } ou { "data": [...] }.
            matjson::Value const* arrPtr = nullptr;
            if (root.isArray()) {
                arrPtr = &root;
            } else if (root.isObject()) {
                for (auto* key : {"levels", "data", "results", "list", "items"}) {
                    if (root.contains(key) && root[key].isArray()) {
                        arrPtr = &root[key];
                        break;
                    }
                }
            }

            if (!arrPtr) {
                log::warn("[ImpossibleLevels] Impossible de trouver un tableau de niveaux dans la reponse.");
                callback(m_cachedLevels, false, "Format de reponse inattendu");
                return;
            }

            std::vector<ImpossibleLevel> parsed;
            parsed.reserve(arrPtr->size());
            for (auto& entry : arrPtr->asArray().unwrapOr({})) {
                parsed.push_back(ImpossibleLevel::fromJson(entry));
            }

            // Tri par rang croissant par défaut si un rang existe, sinon on
            // garde l'ordre renvoyé par l'API.
            std::stable_sort(parsed.begin(), parsed.end(), [](auto const& a, auto const& b) {
                if (a.rank == 0 || b.rank == 0) return false;
                return a.rank < b.rank;
            });

            m_cachedLevels = std::move(parsed);
            m_lastFetchTime = static_cast<long long>(std::time(nullptr));

            callback(m_cachedLevels, true, "");
        });
    }

    std::vector<ImpossibleLevel> ImpossibleLevelsAPI::filter(
        ListCategory category,
        std::string const& searchQuery,
        int minRank,
        int maxRank
    ) const {
        int weekDays = Mod::get()->getSettingValue<int64_t>("week-window-days");
        int monthDays = Mod::get()->getSettingValue<int64_t>("month-window-days");

        std::string query = searchQuery;
        std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) { return std::tolower(c); });

        std::vector<ImpossibleLevel> out;
        for (auto const& lvl : m_cachedLevels) {
            if (category == ListCategory::Week && !lvl.isNewerThan(weekDays)) continue;
            if (category == ListCategory::Month && !lvl.isNewerThan(monthDays)) continue;

            if (maxRank > 0 && lvl.rank > 0 && (lvl.rank < minRank || lvl.rank > maxRank)) continue;

            if (!query.empty()) {
                std::string name = lvl.name, creator = lvl.creator;
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
                std::transform(creator.begin(), creator.end(), creator.begin(), [](unsigned char c) { return std::tolower(c); });
                if (name.find(query) == std::string::npos && creator.find(query) == std::string::npos) {
                    continue;
                }
            }

            out.push_back(lvl);
        }
        return out;
    }

}
