#include "Player.h"
#include <cassert>

// コンストラクタ&デストラクタ
Player::Player() {}
Player::~Player() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/// <summary>
/// 自機の初期化関数
/// </summary>
/// <param name="model">3Dモデル</param>
/// <param name="camera">カメラ</param>
void Player::Intialize(Model* model, Camera* camera) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// メンバ変数への代入処理
	// ファイル名を指定してテクスチャを読み込む

	// プレイヤーの拡縮,回転,平行移動情報
	worldTransform_.scale_ = {2, 2, 2};
	worldTransform_.rotation_ = {0, 3, 0};
	worldTransform_.translation_ = {2, 2, 0};

	// 3Dモデルの生成
	model_ = model;
}

/// <summary>
/// 自機の更新
/// </summary>
void Player::Update() {
	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(worldTransform_);
}

/// <summary>
/// 自機の描画
/// </summary>
void Player::Draw() {
	// 自機を描画
	model_->Draw(worldTransform_, *camera_);
}