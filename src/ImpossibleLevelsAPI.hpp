#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <string>
#include <vector>
#include <functional>
#include <ctime>

// ============================================================================
// NOTE IMPORTANTE SUR L'API
// ----------------------------------------------------------------------------
// Le site https://impossiblelevels.com/ charge sa liste via JavaScript, donc
// la structure exacte du JSON renvoyé par api.impossiblelevels.com n'a pas pu
// être inspectée à l'écriture de ce mod (site rendu côté client, endpoints
// non publics/documentés). Le parsing ci-dessous est donc VOLONTAIREMENT
// tolérant : il essaie plusieurs noms de clés possibles pour chaque champ, et
// n'importe quel champ manquant prend une valeur par défaut plutôt que de
// crasher.
//
// COMMENT AJUSTER SI CA NE MATCHE PAS DU PREMIER COUP :
// 1. Compile et lance le mod, ouvre la liste en jeu.
// 2. Ouvre la console Geode (Geode -> Console dans le menu du loader).
// 3. Le mod logue le JSON brut de la première réponse HTTP (voir
//    `ImpossibleLevelsAPI::fetchLevels`, log::debug("Raw JSON: {}", ...)).
// 4. Compare les clés réelles avec la liste `tryKeys(...)` de
//    `ImpossibleLevel::fromJson` ci-dessous et ajoute/corrige les noms.
// ============================================================================

namespace ill {

    enum class ListCategory {
        All,
        Week,
        Month
    };

    struct ImpossibleLevel {
        int rank = 0;                 // Position dans le classement (0 = inconnu)
        int levelID = 0;               // ID du niveau GD (obligatoire pour le charger)
        std::string name = "Inconnu";
        std::string creator = "Inconnu";
        double fps = 60.0;
        std::string length;            // texte libre ("XL", "2:30", etc. selon l'API)
        std::string difficulty;        // tier / placement textuel si fourni par l'API
        std::string videoUrl;
        std::string thumbnailUrl;
        std::string recordsUrl;
        long long addedTimestamp = 0;  // epoch (secondes) de la date d'ajout, 0 si inconnue
        bool verified = false;

        static ImpossibleLevel fromJson(matjson::Value const& j);

        bool isNewerThan(int days) const;
    };

    class ImpossibleLevelsAPI {
    protected:
        static ImpossibleLevelsAPI* s_instance;

        std::vector<ImpossibleLevel> m_cachedLevels;
        long long m_lastFetchTime = 0;
        bool m_isFetching = false;

        std::string baseUrl() const;

    public:
        static ImpossibleLevelsAPI* get();

        // Récupère (avec cache) la liste complète depuis l'API.
        // `force` ignore le cache et refait un appel réseau.
        void fetchLevels(bool force, std::function<void(std::vector<ImpossibleLevel> const&, bool /*success*/, std::string /*error*/)> callback);

        // Filtre la liste déjà en cache selon la catégorie (Tous / Semaine / Mois),
        // un texte de recherche, et une plage de rang.
        std::vector<ImpossibleLevel> filter(
            ListCategory category,
            std::string const& searchQuery,
            int minRank,
            int maxRank
        ) const;

        std::vector<ImpossibleLevel> const& cached() const { return m_cachedLevels; }

        // Ouvre la page web du niveau/liste dans le navigateur par défaut.
        static void openWebsite(std::string const& path = "");
    };

}
