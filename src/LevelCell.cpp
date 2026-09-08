#include "LevelCell.hpp"

using namespace geode::prelude;

LevelCell* LevelCell::create(
    ill::ImpossibleLevel const& level,
    bool featured,
    float width,
    float height,
    std::function<void(ill::ImpossibleLevel const&)> onPlay,
    std::function<void(ill::ImpossibleLevel const&)> onRecords
) {
    auto ret = new LevelCell();
    ret->m_onPlay = onPlay;
    ret->m_onRecords = onRecords;
    if (ret->init(level, featured, width, height)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool LevelCell::init(ill::ImpossibleLevel const& level, bool featured, float width, float height) {
    m_level = level;
    m_featured = featured;

    cocos2d::ccColor4B bg = featured
        ? cocos2d::ccColor4B{ 30, 40, 70, 255 }
        : (level.rank % 2 == 0
            ? cocos2d::ccColor4B{ 0, 0, 0, 60 }
            : cocos2d::ccColor4B{ 0, 0, 0, 30 });

    if (!CCLayerColor::initWithColor(bg, width, height)) return false;

    setAnchorPoint({ 0.f, 0.f });
    setContentSize({ width, height });

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    addChild(menu, 10);

    float pad = 10.f;

    // --- Tag "NOUVEAU" pour les cellules mises en avant ---------------
    if (featured) {
        auto tag = CCLabelBMFont::create("NOUVEAU", "bigFont.fnt");
        tag->setScale(0.35f);
        tag->setColor({ 255, 220, 90 });
        tag->setAnchorPoint({ 0.f, 1.f });
        tag->setPosition({ pad, height - 6.f });
        addChild(tag, 11);
    }

    // --- Rang -----------------------------------------------------------
    if (level.rank > 0) {
        auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", level.rank).c_str(), "goldFont.fnt");
        rankLabel->setScale(featured ? 0.55f : 0.45f);
        rankLabel->setAnchorPoint({ 0.f, 0.5f });
        rankLabel->setPosition({ pad, height * (featured ? 0.72f : 0.5f) });
        addChild(rankLabel, 11);
    }

    // --- Nom du niveau ----------------------------------------------------
    float textStartX = level.rank > 0 ? pad + 46.f : pad;

    auto nameLabel = CCLabelBMFont::create(level.name.c_str(), "bigFont.fnt");
    nameLabel->setScale(featured ? 0.42f : 0.35f);
    nameLabel->setAnchorPoint({ 0.f, 0.5f });
    nameLabel->setPosition({ textStartX, height * (featured ? 0.7f : 0.62f) });
    if (nameLabel->getScaledContentSize().width > width - textStartX - 90.f) {
        nameLabel->setScale(nameLabel->getScale() * (width - textStartX - 90.f) / nameLabel->getScaledContentSize().width);
    }
    addChild(nameLabel, 11);

    // --- Créateur + infos ---------------------------------------------
    std::string infoStr = fmt::format("par {}", level.creator);
    if (!level.difficulty.empty()) infoStr += fmt::format("  |  {}", level.difficulty);
    if (!level.length.empty()) infoStr += fmt::format("  |  {}", level.length);
    infoStr += fmt::format("  |  {:.0f} FPS", level.fps);

    auto infoLabel = CCLabelBMFont::create(infoStr.c_str(), "chatFont.fnt");
    infoLabel->setScale(0.4f);
    infoLabel->setColor({ 180, 180, 190 });
    infoLabel->setAnchorPoint({ 0.f, 0.5f });
    infoLabel->setPosition({ textStartX, height * (featured ? 0.42f : 0.34f) });
    addChild(infoLabel, 11);

    // --- Bouton Jouer ----------------------------------------------------
    auto playSpr = ButtonSprite::create("Jouer", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    playSpr->setScale(featured ? 0.75f : 0.6f);
    auto playBtn = CCMenuItemSpriteExtra::create(playSpr, this, menu_selector(LevelCell::onPlay));
    playBtn->setPosition({ width - pad - playBtn->getScaledContentSize().width / 2.f, height * 0.5f });
    menu->addChild(playBtn);

    // --- Bouton copier l'ID (petit bouton discret) -----------------------
    auto copySpr = CCSprite::createWithSpriteFrameName("GJ_copyBtn_001.png");
    if (copySpr) {
        copySpr->setScale(0.6f);
        auto copyBtn = CCMenuItemSpriteExtra::create(copySpr, this, menu_selector(LevelCell::onCopyId));
        copyBtn->setPosition({ width - pad - playBtn->getScaledContentSize().width - 18.f, height * 0.5f });
        menu->addChild(copyBtn);
    }

    return true;
}

void LevelCell::onPlay(cocos2d::CCObject*) {
    if (m_level.levelID <= 0) {
        Notification::create("ID de niveau introuvable pour ce niveau.", NotificationIcon::Error)->show();
        return;
    }
    if (m_onPlay) m_onPlay(m_level);
}

void LevelCell::onRecords(cocos2d::CCObject*) {
    if (m_onRecords) m_onRecords(m_level);
}

void LevelCell::onCopyId(cocos2d::CCObject*) {
    if (m_level.levelID <= 0) return;
    geode::utils::clipboard::write(std::to_string(m_level.levelID));
    Notification::create(fmt::format("ID {} copie !", m_level.levelID), NotificationIcon::Success)->show();
}
