#include "ThumbnailCache.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace ill {

    ThumbnailCache* ThumbnailCache::s_instance = nullptr;

    ThumbnailCache* ThumbnailCache::get() {
        if (!s_instance) s_instance = new ThumbnailCache();
        return s_instance;
    }

    std::string ThumbnailCache::baseUrl() const {
        return Mod::get()->getSettingValue<std::string>("thumbnail-base-url");
    }

    void ThumbnailCache::deliver(int levelID, CCTexture2D* tex) {
        auto it = m_waiters.find(levelID);
        if (it == m_waiters.end()) return;
        auto callbacks = std::move(it->second);
        m_waiters.erase(it);
        for (auto& cb : callbacks) {
            if (cb && tex) cb(tex);
        }
    }

    void ThumbnailCache::fail(int levelID) {
        m_failed.insert(levelID);
        m_waiters.erase(levelID);
    }

    // Depile tant qu'on est sous le plafond de requetes simultanees.
    void ThumbnailCache::pump() {
        while (m_inFlight.size() < kMaxParallel && !m_queue.empty()) {
            int id = m_queue.front();
            m_queue.pop_front();
            m_queued.erase(id);
            fetch(id);
        }
    }

    CCTexture2D* ThumbnailCache::request(int levelID, std::function<void(CCTexture2D*)> callback) {
        if (levelID <= 0) return nullptr;

        if (auto it = m_textures.find(levelID); it != m_textures.end()) {
            return it->second;
        }
        // Un 404 est definitif : beaucoup de niveaux de la liste n'ont pas de
        // vignette, inutile de retenter a chaque reconstruction de la liste.
        if (m_failed.count(levelID)) return nullptr;

        if (callback) m_waiters[levelID].push_back(std::move(callback));

        if (m_inFlight.count(levelID) || m_queued.count(levelID)) return nullptr;

        m_queue.push_back(levelID);
        m_queued.insert(levelID);
        pump();
        return nullptr;
    }

    void ThumbnailCache::fetch(int levelID) {
        m_inFlight.insert(levelID);

        auto req = web::WebRequest();
        req.userAgent("ImpossibleLevelsGeodeMod/1.0");

        std::string url = fmt::format("{}/thumbnail/{}/small", baseUrl(), levelID);

        async::spawn(req.get(url), [this, levelID](web::WebResponse res) {
            m_inFlight.erase(levelID);

            if (!res.ok()) {
                fail(levelID);
                pump();
                return;
            }

            // GD decode le WebP nativement via CCImage::initWithImageData :
            // aucun decodeur a embarquer (le mod Level Thumbnails fait pareil).
            auto data = std::move(res).data();
            auto img = new CCImage();
            if (!img->initWithImageData(data.data(), static_cast<int>(data.size()))) {
                delete img;
                fail(levelID);
                pump();
                log::warn("[ImpossibleLevels] vignette {} : image illisible", levelID);
                return;
            }

            auto tex = new CCTexture2D();
            bool ok = tex->initWithImage(img);
            delete img;

            if (!ok) {
                tex->release();
                fail(levelID);
                pump();
                return;
            }

            tex->autorelease();
            m_textures[levelID] = tex;
            deliver(levelID, tex);
            pump();
        });
    }

    void ThumbnailCache::clear() {
        m_textures.clear();
        m_failed.clear();
        m_waiters.clear();
        m_queue.clear();
        m_queued.clear();
    }

}
