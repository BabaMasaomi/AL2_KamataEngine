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
void Player::Intialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	// ぬるぽチェック
	assert(model);

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// メンバ変数への代入処理
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("./Resources./uvChecker.png");

	// 3Dモデルの生成
	model_ = Model::Create();
}

/// <summary>
/// 自機の更新
/// </summary>
void Player::Update() {
	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();
}

/// <summary>
/// 自機の描画
/// </summary>
void Player::Draw() {

	Model::PreDraw();
	model_->Draw(worldTransform_, *camera_, textureHandle_);

	Model::PostDraw();
}