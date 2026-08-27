#pragma once
#include "KamataEngine.h"

// 前方宣言
class Player;

class CameraController {
public:
	// 矩形
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	// カメラのモード
	enum class Mode {
		kFollow,		// プレイヤー追従
		kForcedScroll,	// 強制スクロール
	};

	/// <summary>
	/// カメラの初期化
	/// </summary>
	void Initialize(KamataEngine::Camera* camera);

	/// <summary>
	/// カメラの更新
	/// </summary>
	void Update();

	void UpdateFollow();
	void UpdateForcedScroll();

	/// <summary>
	/// 最初のピッタリ補正のためのリセット
	/// </summary>
	void Reset();

	// 画面揺れを開始
	void StartShake(float duration, float power);

	// セッター
	// 追従対象の位置
	void SetTarget(Player* target) { target_ = target; }

	// カメラの移動範囲
	void SetMovableArea(const Rect& area) { movableArea_ = area; }

private:
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// 追従対象
	Player* target_ = nullptr;

	// 追従対象とカメラの座標の差
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.0f, -30.0f};

	// カメラの移動範囲
	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};

	// カメラの目標座標
	KamataEngine::Vector3 targetCoordinate = {};

	// 座標補間割合
	static inline const float kInterpolationRate = 0.08f;

	// 速度掛け率
	// 横方向の速度先読み倍率
	static constexpr float kVelocityBiasX = 2.0f;

	// 縦方向の速度先読み倍率
	static constexpr float kVelocityBiasY = 0.5f;

	// 追従対象の各方向へのカメラ移動範囲(-left,+right,-bottom,+topの順)
	static inline const Rect cameraMovementMargin = {-100.0f, 100.0f, -100.0f, 100.0f};

	// 画面揺れ
	float shakeTimer_ = 0.0f;
	float shakeDuration_ = 0.0f;
	float shakePower_ = 0.0f;

	KamataEngine::Vector3 shakeOffset_ = {};

	/*--------------- モード管理 ---------------*/
	Mode mode_ = Mode::kFollow;

	float forceScrollSpeed_ = 0.08f;
};