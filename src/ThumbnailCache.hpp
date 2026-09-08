#pragma once

#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <vector>

namespace ill {

    // Vignettes de niveaux, servies par la meme API que le mod
    // "Level Thumbnails" (cdc.level_thumbnails) : les images affichees sont
    // donc exactement les memes. Ce mod n'est PAS declare comme une API Geode
    // (api: false dans son mod.json), il n'y a donc rien contre quoi lier :
    // on tape le meme endpoint public, et son URL reste configurable pour
    // suivre l'utilisateur s'il redirige le mod vers un autre serveur.
    //
    // Les images font 640x360 en qualite "small", soit ~370 Ko : elles sont
    // chargees paresseusement, uniquement pour les cellules reellement
    // affichees, et mises en cache en memoire pour la duree de la session.
    class ThumbnailCache {
    protected:
        static ThumbnailCache* s_instance;

        std::unordered_map<int, geode::Ref<cocos2d::CCTexture2D>> m_textures;
        std::unordered_set<int> m_inFlight;
        // Une page affiche jusqu'a 100 lignes : sans file d'attente, autant
        // de requetes partaient d'un coup et saturaient la connexion.
        std::deque<int> m_queue;
        std::unordered_set<int> m_queued;
        static constexpr size_t kMaxParallel = 6;
        std::unordered_set<int> m_failed;
        // Plusieurs cellules peuvent demander le meme niveau avant que la
        // reponse arrive : on empile les callbacks au lieu de refaire l'appel.
        std::unordered_map<int, std::vector<std::function<void(cocos2d::CCTexture2D*)>>> m_waiters;

        void deliver(int levelID, cocos2d::CCTexture2D* tex);
        void pump();
        void fetch(int levelID);
        void fail(int levelID);

    public:
        static ThumbnailCache* get();

        std::string baseUrl() const;

        // Renvoie la texture si elle est deja en cache. Sinon renvoie nullptr
        // et appelle `callback` plus tard sur le thread principal, une seule
        // fois, et jamais en cas d'echec.
        cocos2d::CCTexture2D* request(int levelID, std::function<void(cocos2d::CCTexture2D*)> callback);

        void clear();
    };

}
