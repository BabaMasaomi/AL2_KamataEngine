#pragma once
#include "KamataEngine.h"
#include "Transform.h"

// 左右の向き
enum class LRDirection {
	kRight,
	kLeft,
};

class Player {
public:
	// コンストラクタ&デストラクタ
	Player();
	~Player();

	/// <summary>
	/// 自機の初期化
	/// </summary>
	/// <param name="model">3Dモデル</param>
	/// <param name="camera">カメラ</param>
	void Intialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3 pos);

	/// <summary>
	/// 自機の更新
	/// </summary>
	void Update();

	/// <summary>
	/// 自機の描画
	/// </summary>
	void Draw();

private:
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;

	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// モデル
	KamataEngine::Model* model_ = nullptr;

	// 以下、移動などに使う変数をまとめる
	KamataEngine::Vector3 velocity_ = {};

	// 左右移動の加速度
	static inline const float kAcceleration = 0.05f;

	// 移動減衰の基本の値
	static inline const float kAttenuation = 0.09f;

	// 制限速度
	static inline const float kLimitRunSpeed = 2.0f;

	// 左右の向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 旋回開始の角度
	float turnFirstRotationY_ = 0.0f;

	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間(秒)
	static inline const float kTimeTurn = 0.3f;

	// 接地フラグ
	bool onGround_ = true;

	// 重力加速度
	static inline const float kGravityAcceleration = 0.09f;

	// 最大落下速度
	static inline const float kLimitFallSpeed_ = 2.0f;

	// ジャンプ初速
	static inline const float kJumpAcceleration_ = 2.0f;
};