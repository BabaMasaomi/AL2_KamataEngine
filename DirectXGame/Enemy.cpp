#define NOMINMAX
#include "Enemy.h"
#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <numbers>

// コンストラクタ&デストラクタ
Enemy::Enemy() {}
Enemy::~Enemy() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/// <summary>
/// 敵の初期化
/// </summary>
/// <param name="model"></param>
/// <param name="pos"></param>
void Enemy::Initialize(Model* model, Camera* camera, const Vector3 pos) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// メンバ変数への代入処理
	// 敵の拡縮,回転,平行移動情報
	worldTransform_.scale_ = {2, 2, 2};
	worldTransform_.translation_ = pos;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 3Dモデルの生成
	modelEnemy_ = model;

	// 移動速度の初期化
	velocity_ = {-kMoveSpeed, 0.0f, 0.0f};

	// アニメーションタイマーの初期化
	walkTimer_ = 0.0f;
}

/// <summary>
/// 敵の更新
/// </summary>
void Enemy::Update() {
	// 移動処理
	worldTransform_.translation_.x += velocity_.x;

	// 歩行アニメーション処理
	// タイマーを加算(1/60秒)
	walkTimer_ += 1.0f / 60.0f;

	// 回転アニメーション
	float param = std::sin((walkTimer_ / kWalkMotionTime) * 2.0f * std::numbers::pi_v<float>);
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * ((param + 1.0f) / 2.0f);

	// 度をラジアンに変換
	worldTransform_.rotation_.x = degree * std::numbers::pi_v<float> / 180.0f;

	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(worldTransform_);
}

/// <summary>
/// 敵の描画
/// </summary>
void Enemy::Draw() {
	// 敵を描画
	modelEnemy_->Draw(worldTransform_, *camera_);
}

Vector3 Enemy::GetWorldPos() {
	Vector3 worldPos;

	// ワールド行列の平行移動成分を取得(正しいやり方がワカンナイヨー)
	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;

	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPos();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

void Enemy::OnCollisionPlayer(Player* player) { 
	// 攻撃中のプレイヤーと接触したら死亡
	isDead_ = true;

	(void)player;
}