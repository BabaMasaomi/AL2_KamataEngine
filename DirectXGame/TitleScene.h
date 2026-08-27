#pragma once
#include "CameraController.h"
#include "KamataEngine.h"
#include "Player.h"
#include "Skydome.h"
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

	static constexpr float kMenuCenterX = 880.0f;
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

	// バットの3Dモデル
	KamataEngine::Model* modelBat_ = nullptr;

	// プレイヤーのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformPlayer_;

	// バットのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformBat_;

	// ゲーム開始時の高速回転中か
	bool isPlayStartAnimation_ = false;

	// 現在の回転速度
	float playerRotationSpeed_ = 0.0f;

	// 通常時の回転速度
	static constexpr float kPlayerIdleRotationSpeed = 0.008f;

	// ゲーム開始時の回転速度
	static constexpr float kPlayerStartRotationSpeed = 0.45f;

	// 回転速度の補間率
	static constexpr float kPlayerRotationLerpRate = 0.14f;

	// プレイヤーから見たバットの手元位置
	static constexpr float kBatHandOffsetX = 2.2f;
	static constexpr float kBatHandOffsetY = 0.3f;
	static constexpr float kBatHandOffsetZ = -0.4f;

	/*-------------------- 天球 --------------------*/
	// 天球モデル
	KamataEngine::Model* modelSkydome_ = nullptr;

	// 天球本体
	Skydome* skydome_ = nullptr;

	// 天球の回転速度
	static constexpr float kSkydomeRotationSpeed = 0.0015f;

	/*-------------------- カメラ --------------------*/
	// カメラ
	KamataEngine::Camera camera_;

	// カメラのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformCamera_;

	/*-------------------- UI音声 --------------------*/
	KamataEngine::Audio* audio_ = nullptr;

	uint32_t cursorMovementSoundHandle_ = 0;
	uint32_t selectSoundHandle_ = 0;

	/*-------------------- デバッグ --------------------*/
	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
