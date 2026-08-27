#pragma once
#include "CameraController.h"
#include "KamataEngine.h"
#include "Player.h"
#include "Fade.h"
#include "Transform.h"
#include <array>
#include <vector>

class TitleScene {
public:
	enum class Action {
		kNone,
		kPlay,
		kQuit,
	};

	/*-------------------- コンストラクタ&デストラクタ --------------------*/
	TitleScene();
	~TitleScene();

	/*-------------------- メンバ関数 --------------------*/
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	/*-------------------- アクセッサ --------------------*/
	bool GetIsFinished() const { return finished_; }
	Action GetAction() const { return action_; }

private:
	// シーンのフェーズ
	enum class Phase {
		kFadeIn,	// フェードイン
		kMain,		// メイン部
		kFadeOut,	// フェードアウト
	};

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// 終了フラグ
	bool finished_ = false;
	Action action_ = Action::kNone;

	/*-------------------- メニューUI --------------------*/
	enum class MenuItem {
		kPlay,
		kCredit,
		kQuit,
		kCount,
	};

	std::array<KamataEngine::Sprite*, static_cast<size_t>(MenuItem::kCount)> menuSprites_{};
	size_t selectedMenuIndex_ = 0;
	bool isCreditVisible_ = false;
	KamataEngine::Sprite* creditBackdropSprite_ = nullptr;
	KamataEngine::Sprite* creditPlaceholderSprite_ = nullptr;

	static constexpr float kMenuCenterX = 640.0f;
	// メニューの開始位置
	static constexpr float kMenuStartY = 380.0f;
	// メニュー同士の間隔
	static constexpr float kMenuSpacingY = 100.0f;
	// メニュー画像の大きさ
	static constexpr float kMenuSize = 250.0f;
	static constexpr float kSelectedAlpha = 1.0f;
	static constexpr float kUnselectedAlpha = 0.35f;

	void InitializeMenu();
	void UpdateMenuAppearance();
	void UpdateMenu();
	void DrawMenu();

	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// フェード用
	Fade* fade_ = nullptr;

	/*-------------------- タイトルフォント --------------------*/
	// タイトルフォントの3Dモデル
	KamataEngine::Model* modelTitle_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransformTitle_;

	/*-------------------- プレイヤー --------------------*/
	// プレイヤーの3Dモデル
	KamataEngine::Model* modelPlayer_ = nullptr;

	// プレイヤーのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformPlayer_;

	// プレイヤー
	Player* player_ = nullptr;

	/*-------------------- カメラ --------------------*/
	// カメラ
	KamataEngine::Camera camera_;

	// カメラのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformCamera_;

	/*-------------------- デバッグ --------------------*/
	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
