#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;
using namespace cocos2d;

const float FONT_SCALE = 0.425f;
const float LINE_SPACING = 18.f;
const float BUTTON_SCALE = 0.35f;

std::array<ccColor3B, 4> textColors = {
    ccColor3B{163, 252, 150},
    ccColor3B{182, 192, 244},
    ccColor3B{209, 249, 14},
    ccColor3B{249, 61, 63}
};

int g_lives = 100;
int g_levelsBeat = 0;
int g_skips = 3;
int g_pruns = 3;
bool g_hasLostLifeThisSpawn = false;
bool g_practiceModeActive = false;
std::vector<CCLabelBMFont*> g_statLabels;

bool isModEnabled() {
    return Mod::get()->getSettingValue<bool>("ModEnabled");
}

class ResetConfirmPopup : public FLAlertLayerProtocol {
public:
    void FLAlert_Clicked(FLAlertLayer*, bool btn2) override {
        if (btn2) {
            g_lives = 100;
            g_levelsBeat = 0;
            g_skips = 3;
            g_pruns = 3;
            if (isModEnabled()) {
                void updateStatLabel(int index, const std::string& newText, ccColor3B flashColor = {255, 255, 255});
                updateStatLabel(0, "LIVES: 100");
                updateStatLabel(1, "LEVELS: 0");
                updateStatLabel(2, "SKIPS: 3");
                updateStatLabel(3, "P.RUNS: 3");
            }
        }
    }
};

void showResetConfirm() {
    static ResetConfirmPopup delegate;
    FLAlertLayer::create(
        &delegate,
        "Reset Stats",
        "Are you sure you would like to reset your stats?",
        "Cancel",
        "Yes"
    )->show();
}

void updateStatLabel(int index, const std::string& newText, ccColor3B flashColor = {255, 255, 255}) {
    if (!isModEnabled() || index >= g_statLabels.size()) return;
    auto* label = g_statLabels[index];
    label->setString(newText.c_str());

    float alpha = Mod::get()->getSettingValue<float>("TextTransparency");
    label->setOpacity(alpha * 255);

    auto flash = CCSequence::create(
        CCTintTo::create(0.05f, flashColor.r, flashColor.g, flashColor.b),
        CCDelayTime::create(0.15f),
        CCTintTo::create(0.1f, textColors[index].r, textColors[index].g, textColors[index].b),
        nullptr
    );

    label->runAction(flash);
}

void modifyStat(int index, int delta) {
    switch (index) {
        case 0: g_lives += delta; break;
        case 1: g_levelsBeat += delta; break;
        case 2: g_skips += delta; break;
        case 3: g_pruns += delta; break;
    }

    std::string newText;
    switch (index) {
        case 0: newText = "LIVES: " + std::to_string(g_lives); break;
        case 1: newText = "LEVELS: " + std::to_string(g_levelsBeat); break;
        case 2: newText = "SKIPS: " + std::to_string(g_skips); break;
        case 3: newText = "P.RUNS: " + std::to_string(g_pruns); break;
    }

    if (isModEnabled()) {
        updateStatLabel(index, newText);
    }
}

void addStatLabels(CCNode* parent, bool withButtons = false) {
    if (!isModEnabled()) return;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    float x = 5.f;
    float y = winSize.height - 5.f;

    g_statLabels.clear();
    std::array<std::string, 4> labelTexts = {
        "LIVES: " + std::to_string(g_lives),
        "LEVELS: " + std::to_string(g_levelsBeat),
        "SKIPS: " + std::to_string(g_skips),
        "P.RUNS: " + std::to_string(g_pruns)
    };

    auto menu = CCMenu::create();
    menu->setZOrder(9999);
    menu->setPosition(CCPointZero);
    parent->addChild(menu);

    float buttonAlpha = Mod::get()->getSettingValue<float>("ButtonsTransparency");
    float textAlpha = Mod::get()->getSettingValue<float>("TextTransparency");

    for (int i = 0; i < labelTexts.size(); ++i) {
        CCPoint pos = {x, y - i * LINE_SPACING};

        auto label = CCLabelBMFont::create(labelTexts[i].c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(FONT_SCALE);
        label->setColor(textColors[i]);
        label->setOpacity(textAlpha * 255);
        label->setPosition(pos);
        label->setZOrder(9999);
        parent->addChild(label);
        g_statLabels.push_back(label);

        if (withButtons) {
            float spacing = 95.f;

            auto minusBtn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
                "Down.png"_spr, BUTTON_SCALE,
                [i](CCObject*) {
                    int delta = -1;
                    modifyStat(i, delta);
                }
            );
            minusBtn->setPosition({pos.x + spacing, pos.y - 8.f});
            if (auto sprite = typeinfo_cast<CCSprite*>(minusBtn->getNormalImage())) {
                sprite->setOpacity(buttonAlpha * 255);
            }
            menu->addChild(minusBtn);

            auto plusBtn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
                "Up.png"_spr, BUTTON_SCALE,
                [i](CCObject*) {
                    int delta = 1;
                    modifyStat(i, delta);
                }
            );
            plusBtn->setPosition({pos.x + spacing + 28.f, pos.y - 8.f});
            if (auto sprite = typeinfo_cast<CCSprite*>(plusBtn->getNormalImage())) {
                sprite->setOpacity(buttonAlpha * 255);
            }
            menu->addChild(plusBtn);
        }
    }

    if (withButtons && Mod::get()->getSettingValue<bool>("ResetEnabled")) {
        float resetScale = BUTTON_SCALE * 1.4f;
        float resetY = y - 4 * LINE_SPACING - 9.f;

        auto resetBtn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
            "Reset.png"_spr, resetScale,
            [](CCObject*) {
                showResetConfirm();
            }
        );
        resetBtn->setPosition({x + 10.f, resetY});
        if (auto sprite = typeinfo_cast<CCSprite*>(resetBtn->getNormalImage())) {
            sprite->setOpacity(buttonAlpha * 255);
        }
        menu->addChild(resetBtn);
    }
}

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool a2) {
        if (!LevelInfoLayer::init(level, a2)) return false;
        addStatLabels(this, true);
        return true;
    }
};

class $modify(MyUILayer, UILayer) {
    bool init(GJBaseGameLayer* g) {
        if (!UILayer::init(g)) return false;
        g_hasLostLifeThisSpawn = false;
        g_practiceModeActive = false;
        addStatLabels(this, false);
        return true;
    }
};

class $modify(MyPlayerObject, PlayerObject) {
    void resetObject() {
        PlayerObject::resetObject();
        g_hasLostLifeThisSpawn = false;
    }

    void playerDestroyed(bool cleanup) {
        PlayerObject::playerDestroyed(cleanup);
        auto playLayer = GameManager::sharedState()->getPlayLayer();
        if (!g_hasLostLifeThisSpawn && playLayer && !playLayer->m_isPracticeMode) {
            g_hasLostLifeThisSpawn = true;
            g_lives -= 1;
            if (isModEnabled()) {
                updateStatLabel(0, "LIVES: " + std::to_string(g_lives), ccColor3B{255, 0, 0});
            }
        }
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();
        if (!this->m_isPracticeMode) {
            g_levelsBeat++;
            if (isModEnabled()) {
                updateStatLabel(1, "LEVELS: " + std::to_string(g_levelsBeat), ccColor3B{255, 255, 255});
            }
        }
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void onPracticeMode(CCObject* sender) {
        PauseLayer::onPracticeMode(sender);

        auto playLayer = GameManager::sharedState()->getPlayLayer();
        if (!playLayer) return;

        bool isNowOn = playLayer->m_isPracticeMode;

        if (isNowOn && !g_practiceModeActive) {
            g_pruns--;
            if (isModEnabled()) {
                updateStatLabel(3, "P.RUNS: " + std::to_string(g_pruns), ccColor3B{255, 100, 100});
            }
        }

        g_practiceModeActive = isNowOn;
    }
};
