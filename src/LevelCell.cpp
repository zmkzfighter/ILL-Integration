#include "LevelCell.hpp"

using namespace geode::prelude;

ILLLevelCell* ILLLevelCell::create(
    ill::ImpossibleLevel const& level,
    bool featured,
    float width,
    float height,
    std::function<void(ill::ImpossibleLevel const&)> onPlay,
    std::function<void(ill::ImpossibleLevel const&)> onShowcase
) {
    auto ret = new ILLLevelCell();
    ret->m_onPlay = onPlay;
    ret->m_onShowcase = onShowcase;
    if (ret->init(level, featured, width, height)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ILLLevelCell::init(ill::ImpossibleLevel const& level, bool featured, float width, float height) {
    m_level = level;
    m_featured = featured;

    cocos2d::ccColor4B bg = featured
        ? cocos2d::ccColor4B{ 30, 40, 70, 255 }
        : (level.rank % 2 == 0
            ? cocos2d::ccColor4B{ 0, 0, 0, 60 }
            : cocos2d::ccColor4B{ 0, 0, 0, 30 });

    if (!CCLayerColor::initWithColor(bg, width, height)) return false;
    setContentSize({ width, height });

    const float pad       = 8.f;
    const float rankCol   = 34.f;   // largeur reservee au rang
    const float btnCol    = 132.f;  // largeur reservee aux boutons de droite
    const bool  showThumb = Mod::get()->getSettingValue<bool>("show-thumbnails");
    const float thumbW    = showThumb ? (featured ? 74.f : 58.f) : 0.f;
    const float thumbGap  = showThumb ? 6.f : 0.f;

    // --- Boutons a droite, dans un RowLayout : plus d'offsets en dur qui se
    //     chevauchaient des qu'un bouton apparaissait ou disparaissait.
    auto menu = CCMenu::create();
    menu->setContentSize({ btnCol, height });
    menu->setAnchorPoint({ 1.f, 0.5f });
    menu->setPosition({ width - pad, height / 2.f });
    menu->setLayout(
        geode::RowLayout::create()
            ->setGap(5.f)
            ->setAxisAlignment(geode::AxisAlignment::End)
            ->setAutoScale(false)
    );
    addChild(menu, 10);

    // Ordre d'ajout = ordre de gauche a droite : Jouer finit a droite.
    if (!m_level.videoUrl.empty()) {
        auto vidSpr = ButtonSprite::create("Vid", "goldFont.fnt", "GJ_button_05.png", 0.8f);
        vidSpr->setScale(0.42f);
        menu->addChild(CCMenuItemSpriteExtra::create(vidSpr, this, menu_selector(ILLLevelCell::onShowcase)));
    }

    bool playable = level.levelID > 0;

    if (playable) {
        // GJ_copyBtn_001.png existe bien ; l'ancienne icone video devinee
        // (gj_watchVideoBtn_001.png) n'existe pas et s'affichait en damier.
        if (auto copySpr = CCSprite::createWithSpriteFrameName("GJ_copyBtn_001.png")) {
            copySpr->setScale(0.5f);
            menu->addChild(CCMenuItemSpriteExtra::create(copySpr, this, menu_selector(ILLLevelCell::onCopyId)));
        }
    }

    auto playSpr = ButtonSprite::create(
        playable ? "Jouer" : "Pas d'ID", "goldFont.fnt",
        playable ? "GJ_button_01.png" : "GJ_button_04.png", 0.8f);
    playSpr->setScale(0.48f);
    menu->addChild(CCMenuItemSpriteExtra::create(playSpr, this, menu_selector(ILLLevelCell::onPlay)));

    menu->updateLayout();

    // --- Colonne de gauche : rang, puis le tag NOUVEAU EN DESSOUS. Les deux
    //     etaient dessines a x = pad a 11 px d'ecart : ils se superposaient.
    if (level.rank > 0) {
        auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", level.rank).c_str(), "goldFont.fnt");
        rankLabel->setAnchorPoint({ 0.f, 0.5f });
        rankLabel->limitLabelWidth(rankCol - 6.f, featured ? 0.5f : 0.42f, 0.2f);
        rankLabel->setPosition({ pad, featured ? height * 0.66f : height * 0.5f });
        addChild(rankLabel, 11);
    }

    if (featured) {
        auto tag = CCLabelBMFont::create("NOUVEAU", "bigFont.fnt");
        tag->setAnchorPoint({ 0.f, 0.5f });
        tag->limitLabelWidth(rankCol - 6.f, 0.3f, 0.15f);
        tag->setColor({ 255, 220, 90 });
        tag->setPosition({ pad, height * 0.28f });
        addChild(tag, 11);
    }

    // --- Vignette, entre le rang et le texte -----------------------------
    const float thumbX = pad + (level.rank > 0 ? rankCol : 0.f);
    if (showThumb) {
        setupThumbnail(thumbX, thumbW, thumbW * 9.f / 16.f);
    }

    // --- Bloc texte, borne par les colonnes reservees --------------------
    const float textLeft  = thumbX + thumbW + thumbGap;
    const float textWidth = std::max(40.f, width - pad - btnCol - textLeft - 6.f);

    auto nameLabel = CCLabelBMFont::create(level.name.c_str(), "bigFont.fnt");
    nameLabel->setAnchorPoint({ 0.f, 0.5f });
    nameLabel->limitLabelWidth(textWidth, featured ? 0.45f : 0.38f, 0.16f);
    nameLabel->setPosition({ textLeft, height * 0.66f });
    addChild(nameLabel, 11);

    std::string infoStr = fmt::format("par {}", level.creator);
    if (auto len = level.lengthString(); !len.empty()) infoStr += fmt::format("  |  {}", len);
    if (level.fps > 0.0) infoStr += fmt::format("  |  {:.0f} FPS", level.fps);
    infoStr += fmt::format("  |  {:.1f} pts", level.rating);
    if (!level.cleared) infoStr += "  |  jamais battu";

    auto infoLabel = CCLabelBMFont::create(infoStr.c_str(), "chatFont.fnt");
    infoLabel->setAnchorPoint({ 0.f, 0.5f });
    infoLabel->limitLabelWidth(textWidth, 0.38f, 0.14f);
    infoLabel->setColor({ 180, 180, 190 });
    infoLabel->setPosition({ textLeft, height * 0.30f });
    addChild(infoLabel, 11);

    return true;
}

void ILLLevelCell::setupThumbnail(float x, float w, float h) {
    // Cadre toujours present : il tient la place pendant le chargement, ce
    // qui evite que la ligne se reorganise quand l'image arrive.
    auto frame = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    frame->setContentSize({ w, h });
    frame->setAnchorPoint({ 0.f, 0.5f });
    frame->setPosition({ x, getContentSize().height / 2.f });
    frame->setOpacity(70);
    addChild(frame, 10);

    if (m_level.levelID <= 0) return;

    // Le sprite est cree vide et retenu par le callback : si la cellule est
    // detruite avant l'arrivee de l'image, le Ref garde le sprite en vie et
    // la mise a jour se fait dans le vide, sans acces invalide.
    auto holder = cocos2d::CCSprite::create();
    holder->setAnchorPoint({ 0.f, 0.5f });
    holder->setPosition({ x, getContentSize().height / 2.f });
    addChild(holder, 11);

    geode::Ref<cocos2d::CCSprite> ref = holder;
    auto apply = [ref, w, h](cocos2d::CCTexture2D* tex) {
        if (!tex) return;
        ref->setTexture(tex);
        ref->setTextureRect({ 0.f, 0.f, tex->getContentSize().width, tex->getContentSize().height });
        auto size = ref->getContentSize();
        if (size.width > 0.f && size.height > 0.f) {
            // `contain` : on garde le ratio, l'image tient dans le cadre.
            ref->setScale(std::min(w / size.width, h / size.height));
        }
    };

    if (auto cached = ill::ThumbnailCache::get()->request(m_level.levelID, apply)) {
        apply(cached);
    }
}

void ILLLevelCell::onPlay(cocos2d::CCObject*) {
    if (m_level.levelID <= 0) {
        Notification::create("L'API ne fournit pas d'ID GD pour ce niveau.", NotificationIcon::Error)->show();
        return;
    }
    if (m_onPlay) m_onPlay(m_level);
}

void ILLLevelCell::onShowcase(cocos2d::CCObject*) {
    if (m_onShowcase) m_onShowcase(m_level);
}

void ILLLevelCell::onCopyId(cocos2d::CCObject*) {
    if (m_level.levelID <= 0) return;
    geode::utils::clipboard::write(std::to_string(m_level.levelID));
    Notification::create(fmt::format("ID {} copie !", m_level.levelID), NotificationIcon::Success)->show();
}
