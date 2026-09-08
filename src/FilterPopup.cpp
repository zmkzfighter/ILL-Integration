#include "FilterPopup.hpp"

using namespace geode::prelude;

FilterPopup* FilterPopup::create(int minRank, int maxRank, std::function<void(int, int)> callback) {
    auto ret = new FilterPopup();
    if (ret->initAnchored(240.f, 160.f, minRank, maxRank, callback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FilterPopup::setup(int minRank, int maxRank, std::function<void(int, int)> callback) {
    m_callback = callback;
    setTitle("Filtrer par rang");

    auto winSize = m_mainLayer->getContentSize();

    auto label = CCLabelBMFont::create("Plage de rang dans le classement :", "bigFont.fnt");
    label->setScale(0.35f);
    label->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 35.f });
    m_mainLayer->addChild(label);

    m_minInput = TextInput::create(60.f, "Min");
    m_minInput->setCommonFilter(CommonFilter::Uint);
    m_minInput->setPosition({ winSize.width / 2.f - 45.f, winSize.height / 2.f + 5.f });
    if (minRank > 0) m_minInput->setString(std::to_string(minRank));
    m_mainLayer->addChild(m_minInput);

    auto dash = CCLabelBMFont::create("-", "bigFont.fnt");
    dash->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 5.f });
    m_mainLayer->addChild(dash);

    m_maxInput = TextInput::create(60.f, "Max");
    m_maxInput->setCommonFilter(CommonFilter::Uint);
    m_maxInput->setPosition({ winSize.width / 2.f + 45.f, winSize.height / 2.f + 5.f });
    if (maxRank > 0) m_maxInput->setString(std::to_string(maxRank));
    m_mainLayer->addChild(m_maxInput);

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    auto applySpr = ButtonSprite::create("Appliquer", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    applySpr->setScale(0.7f);
    auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(FilterPopup::onApply));
    applyBtn->setPosition({ winSize.width / 2.f + 45.f, winSize.height / 2.f - 40.f });
    menu->addChild(applyBtn);

    auto resetSpr = ButtonSprite::create("Reinitialiser", "goldFont.fnt", "GJ_button_06.png", 0.8f);
    resetSpr->setScale(0.7f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(FilterPopup::onReset));
    resetBtn->setPosition({ winSize.width / 2.f - 45.f, winSize.height / 2.f - 40.f });
    menu->addChild(resetBtn);

    return true;
}

void FilterPopup::onApply(cocos2d::CCObject*) {
    int minRank = 0, maxRank = 0;
    try { minRank = std::stoi(m_minInput->getString()); } catch (...) {}
    try { maxRank = std::stoi(m_maxInput->getString()); } catch (...) {}
    if (m_callback) m_callback(minRank, maxRank);
    onClose(nullptr);
}

void FilterPopup::onReset(cocos2d::CCObject*) {
    if (m_callback) m_callback(0, 0);
    onClose(nullptr);
}
