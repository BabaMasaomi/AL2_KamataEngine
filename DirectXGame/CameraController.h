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

	/// <summary>
	/// カメラの初期化
	/// </summary>
	void Intialize(KamataEngine::Camera* camera);

	/// <summary>
	/// カメラの更新
	/// </summary>
	void Update();

	/// <summary>
	/// 最初のピッタリ補正のためのリセット
	/// </summary>
	void Reset();

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
	static inline const float kInterpolationRate = 0.2f;

	// 速度掛け率
	static inline const float kVelocityBias = 7.5f;

	// 追従対象の各方向へのカメラ移動範囲(-left,+right,-bottom,+topの順)
	static inline const Rect cameraMovementMargin = {-100.0f, 100.0f, -100.0f, 100.0f};
};