#include "FilterPopup.hpp"

using namespace geode::prelude;

FilterPopup* FilterPopup::create(int minRank, int maxRank, ill::SortMode sort,
                                 std::function<void(int, int, ill::SortMode)> callback) {
    auto ret = new FilterPopup();
    if (ret->init(minRank, maxRank, sort, callback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FilterPopup::init(int minRank, int maxRank, ill::SortMode sort,
                       std::function<void(int, int, ill::SortMode)> callback) {
    if (!Popup::init(260.f, 200.f)) return false;

    m_callback = callback;
    m_sort = sort;
    this->setTitle("Filtres et tri");

    // m_size = taille reelle du popup, fournie par Popup::init.
    auto size = m_size;

    auto label = CCLabelBMFont::create("Plage de rang dans le classement :", "bigFont.fnt");
    label->setScale(0.35f);
    m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(0.f, 55.f));

    m_minInput = TextInput::create(60.f, "Min");
    m_minInput->setCommonFilter(CommonFilter::Uint);
    if (minRank > 0) m_minInput->setString(std::to_string(minRank));
    m_mainLayer->addChildAtPosition(m_minInput, Anchor::Center, ccp(-45.f, 28.f));

    auto dash = CCLabelBMFont::create("-", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(dash, Anchor::Center, ccp(0.f, 28.f));

    m_maxInput = TextInput::create(60.f, "Max");
    m_maxInput->setCommonFilter(CommonFilter::Uint);
    if (maxRank > 0) m_maxInput->setString(std::to_string(maxRank));
    m_mainLayer->addChildAtPosition(m_maxInput, Anchor::Center, ccp(45.f, 28.f));

    // --- Tri : un seul bouton qui fait defiler les modes, plutot qu'une
    //     liste de boutons a caser dans un popup deja etroit.
    auto sortTitle = CCLabelBMFont::create("Ordre d'affichage :", "bigFont.fnt");
    sortTitle->setScale(0.35f);
    m_mainLayer->addChildAtPosition(sortTitle, Anchor::Center, ccp(0.f, -6.f));

    m_sortSprite = ButtonSprite::create(ill::sortModeName(m_sort), "goldFont.fnt", "GJ_button_02.png", 0.8f);
    m_sortSprite->setScale(0.7f);
    auto sortBtn = CCMenuItemSpriteExtra::create(m_sortSprite, this, menu_selector(FilterPopup::onCycleSort));
    m_buttonMenu->addChildAtPosition(sortBtn, Anchor::Center, ccp(0.f, -28.f));

    auto applySpr = ButtonSprite::create("Appliquer", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    applySpr->setScale(0.7f);
    auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(FilterPopup::onApply));
    m_buttonMenu->addChildAtPosition(applyBtn, Anchor::Center, ccp(45.f, -66.f));

    auto resetSpr = ButtonSprite::create("Reinitialiser", "goldFont.fnt", "GJ_button_06.png", 0.8f);
    resetSpr->setScale(0.7f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(FilterPopup::onReset));
    m_buttonMenu->addChildAtPosition(resetBtn, Anchor::Center, ccp(-45.f, -66.f));

    return true;
}

void FilterPopup::onApply(cocos2d::CCObject*) {
    int minRank = 0, maxRank = 0;
    try { minRank = std::stoi(std::string(m_minInput->getString())); } catch (...) {}
    try { maxRank = std::stoi(std::string(m_maxInput->getString())); } catch (...) {}
    if (m_callback) m_callback(minRank, maxRank, m_sort);
    this->onClose(nullptr);
}

void FilterPopup::onReset(cocos2d::CCObject*) {
    if (m_callback) m_callback(0, 0, ill::SortMode::Rank);
    this->onClose(nullptr);
}

void FilterPopup::onCycleSort(cocos2d::CCObject*) {
    int next = (static_cast<int>(m_sort) + 1) % static_cast<int>(ill::SortMode::COUNT);
    m_sort = static_cast<ill::SortMode>(next);
    updateSortLabel();
}

void FilterPopup::updateSortLabel() {
    if (m_sortSprite) m_sortSprite->setString(ill::sortModeName(m_sort));
}
