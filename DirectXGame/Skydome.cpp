#include "Skydome.h"
#include <cassert>

// コンストラクタ&デストラクタ
Skydome::Skydome() {}
Skydome::~Skydome() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/// <summary>
/// 天球の初期化
/// </summary>
/// <param name="model">3Dモデル</param>
/// <param name="textureHandle">テクスチャハンドル</param>
/// <param name="camera">カメラ</param>
void Skydome::Initialize(Model* model, Camera* camera) {
	// ぬるぽチェック
	assert(model);

	// ワールドトランスフォームの初期化
	worldTransformSkydome_.Initialize();

	// 3Dモデルの生成
	modelSkydome_ = model;

	// 引き数の内容をメンバ変数に記録
	cameraSkydome_ = camera;
}

/// <summary>
/// 天球の更新
/// </summary>
void Skydome::Update() {
	// カメラを覆う大きさ
	worldTransformSkydome_.scale_ = {100.0f, 100.0f, 100.0f};

	// 設定された速度でゆっくり回転
	worldTransformSkydome_.rotation_.y += rotationSpeed_;

	// 通常の天球処理と同じ方法で行列を転送
	worldTransformSkydome_.TransferMatrix();
}

/// <summary>
/// 天球の描画
/// </summary>
void Skydome::Draw() {
	// 天球を描画
	modelSkydome_->Draw(worldTransformSkydome_, *cameraSkydome_);
}