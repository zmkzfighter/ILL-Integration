// _CRT_SECURE_NO_WARNINGS est defini dans CMakeLists.txt (le PCH de Geode
// est force-inclut avant ce fichier, un #define ici serait sans effet).
#include "ImpossibleLevelsAPI.hpp"
#include <Geode/utils/async.hpp>
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

    // Champs reels renvoyes par GET https://api.impossiblelevels.com/api/levels
    // (verifie sur les 2282 entrees de la liste ILL) :
    //   id, name, rating, fps, levelLength, levelId, uploader, showcaseLink,
    //   thumbnailUrl, uncleared, datePublished, rank, visible, ...
    // Les anciens noms devines (creator, video, createdAt...) n'existent pas :
    // c'est ce qui laissait la liste sans createur ni date.
    ImpossibleLevel ImpossibleLevel::fromJson(matjson::Value const& j) {
        ImpossibleLevel lvl;

        lvl.listId  = static_cast<int>(tryKeysNum(j, {"id"}, 0));
        lvl.rank    = static_cast<int>(tryKeysNum(j, {"rank"}, 0));

        // `levelId` est une CHAINE, et vaut litteralement "N/A" pour les
        // niveaux dont l'id GD est inconnu -> tryKeysNum renvoie 0 et la
        // cellule desactivera le bouton Jouer.
        lvl.levelID = static_cast<int>(tryKeysNum(j, {"levelId"}, 0));

        lvl.name    = tryKeysStr(j, {"name"}, "Inconnu");
        lvl.creator = tryKeysStr(j, {"uploader"}, "Inconnu");
        if (lvl.creator.empty() || lvl.creator == "N/A") lvl.creator = "Inconnu";

        lvl.fps           = tryKeysNum(j, {"fps"}, 0.0);
        lvl.lengthSeconds = tryKeysNum(j, {"levelLength"}, 0.0);
        lvl.rating        = tryKeysNum(j, {"rating"}, 0.0);

        lvl.videoUrl     = tryKeysStr(j, {"showcaseLink"}, "");
        lvl.thumbnailUrl = tryKeysStr(j, {"thumbnailUrl"}, "");

        // `uncleared` = personne n'a fini le niveau. On stocke l'inverse.
        lvl.cleared = !tryKeysBool(j, {"uncleared"}, false);
        lvl.visible = tryKeysBool(j, {"visible"}, true);

        // Date de sortie du niveau sur GD (PAS la date d'ajout a la liste,
        // que l'API n'expose nulle part).
        lvl.publishedTimestamp = parseDateToEpoch(j, {"datePublished"});

        return lvl;
    }

    std::string ImpossibleLevel::lengthString() const {
        if (lengthSeconds <= 0.0) return "";
        int total = static_cast<int>(lengthSeconds + 0.5);
        return fmt::format("{}:{:02d}", total / 60, total % 60);
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

        // Geode 5 : plus de .listen() sur WebFuture. On passe la future a
        // async::spawn, qui appelle le callback sur le thread principal.
        async::spawn(req.get(url), [this, callback](web::WebResponse res) {
            m_isFetching = false;

            if (!res.ok()) {
                std::string err = fmt::format("Erreur HTTP {}", res.code());
                log::warn("[ImpossibleLevels] Echec de la requete API: {}", err);
                callback(m_cachedLevels, false, err);
                return;
            }

            auto jsonRes = res.json();
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
            int hidden = 0;
            // matjson::Value expose directement begin()/end() sur les
            // elements d'un tableau. On passe par la plutot que par
            // asArray(), dont le Result contient une *reference*
            // (std::vector<Value> const&) : unwrapOrDefault() ne compile pas
            // sur un type reference (contrainte default_initializable).
            // arrPtr a deja ete valide par isArray() plus haut.
            for (auto const& entry : *arrPtr) {
                auto lvl = ImpossibleLevel::fromJson(entry);
                // ~200 entrees sur 2282 sont masquees cote site.
                if (!lvl.visible) { hidden++; continue; }
                parsed.push_back(std::move(lvl));
            }
            log::info("[ImpossibleLevels] {} niveaux visibles ({} masques ignores)",
                      parsed.size(), hidden);

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
        // Seuil d'id au-dela duquel un niveau compte comme "recemment ajoute".
        int recentCount = static_cast<int>(Mod::get()->getSettingValue<int64_t>("recent-count"));
        int recentThreshold = 0;
        if (category == ListCategory::Recent && !m_cachedLevels.empty()) {
            std::vector<int> ids;
            ids.reserve(m_cachedLevels.size());
            for (auto const& lvl : m_cachedLevels) ids.push_back(lvl.listId);
            std::sort(ids.begin(), ids.end(), std::greater<int>());
            size_t idx = std::min<size_t>(static_cast<size_t>(std::max(recentCount, 1)), ids.size()) - 1;
            recentThreshold = ids[idx];
        }

        std::string query = searchQuery;
        std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) { return std::tolower(c); });

        std::vector<ImpossibleLevel> out;
        for (auto const& lvl : m_cachedLevels) {
            if (category == ListCategory::Recent && lvl.listId < recentThreshold) continue;

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
