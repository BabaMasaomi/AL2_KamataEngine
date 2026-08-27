#pragma once

#include "Fade.h"
#include "GameResult.h"
#include "KamataEngine.h"
#include <array>
#include <vector>

class ResultScene {
public:
	enum class Action {
		kNone,
		kReturnToTitle,
		kRetry,
	};

	ResultScene();
	~ResultScene();

	void Initialize(GameResult result, uint32_t score);
	void Update();
	void Draw();

	bool GetIsFinished() const { return finished_; }
	Action GetAction() const { return action_; }

private:
	enum class Phase {
		kFadeIn,
		kMain,
		kFadeOut,
	};

	Phase phase_ = Phase::kFadeIn;

	GameResult result_ = GameResult::kNone;

	bool finished_ = false;
	Action action_ = Action::kNone;

	/*-------------------- リザルトメニュー --------------------*/
	std::array<KamataEngine::Sprite*, 2> menuSprites_{};
	size_t selectedMenuIndex_ = 0;
	static constexpr float kMenuX = 260.0f;
	static constexpr float kMenuStartY = 430.0f;
	static constexpr float kMenuSpacingY = 95.0f;
	static constexpr float kMenuWidth = 300.0f;
	static constexpr float kMenuHeight = 100.0f;
	static constexpr float kSelectedAlpha = 1.0f;
	static constexpr float kUnselectedAlpha = 0.35f;

	void InitializeMenu();
	void UpdateMenuAppearance();
	void DrawMenu();

	KamataEngine::Sprite* backgroundSprite_ = nullptr;

	Fade* fade_ = nullptr;

	/*-------------------- リザルトスコア --------------------*/
	// 最終スコア
	uint32_t score_ = 0;

	// 数字テクスチャ
	std::array<uint32_t, 10> scoreDigitTextures_{};

	// 数字スプライト
	std::vector<KamataEngine::Sprite*> scoreDigitSprites_;

	// 現在表示する桁数
	size_t scoreDigitCount_ = 1;

	// 最大表示桁数
	static constexpr size_t kMaxScoreDigits = 8;

	// 数字の大きさ
	static constexpr float kResultDigitWidth = 160.0f;
	static constexpr float kResultDigitHeight = 160.0f;

	// 数字同士の間隔
	static constexpr float kResultDigitSpacing = 136.0f;

	// 右下の基準位置
	static constexpr float kResultScoreRightX = 1160.0f;
	static constexpr float kResultScoreY = 590.0f;

	// スコアアイコンの大きさと数字との間隔
	static constexpr float kResultScoreIconWidth = 300.0f;
	static constexpr float kResultScoreIconHeight = 100.0f;
	static constexpr float kResultScoreIconMargin = 24.0f;

	// スコアを示すアイコン
	KamataEngine::Sprite* scoreIconSprite_ = nullptr;

	// Game Finish画像
	KamataEngine::Sprite* gameFinishSprite_ = nullptr;

	// 初期化
	void InitializeScoreDisplay();

	// 描画内容更新
	void UpdateScoreDisplay();

	// スコア描画
	void DrawScore();
};
