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
        // Les N derniers niveaux ajoutes a la liste (par id interne
        // decroissant). L'API n'expose aucune date d'ajout, donc une fenetre
        // "cette semaine / ce mois-ci" est impossible a calculer : c'est un
        // nombre d'entrees, pas une duree.
        Recent
    };

    struct ImpossibleLevel {
        // `id` cote API : identifiant interne de la liste. Il s'incremente a
        // chaque ajout, donc l'ordre des id = l'ordre d'ajout a la liste (il
        // n'est PAS correle a datePublished : l'id le plus haut est un niveau
        // GD de 2014). C'est le seul indicateur d'anciennete disponible.
        int listId = 0;
        int rank = 0;                  // Position dans le classement (0 = inconnu)
        int levelID = 0;               // ID du niveau GD, 0 si l'API renvoie "N/A"
        std::string name = "Inconnu";
        std::string creator = "Inconnu";
        double fps = 0.0;
        double lengthSeconds = 0.0;    // `levelLength` : une duree en secondes
        double rating = 0.0;           // note ILL du niveau (peut etre negative)
        std::string videoUrl;          // `showcaseLink`
        std::string thumbnailUrl;      // presque toujours vide cote API
        long long publishedTimestamp = 0;  // `datePublished` = date de sortie GD
        bool cleared = false;          // inverse de `uncleared`
        bool visible = true;           // `visible` : false = masque de la liste

        static ImpossibleLevel fromJson(matjson::Value const& j);

        // "1:38" a partir de lengthSeconds, vide si inconnu.
        std::string lengthString() const;
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
