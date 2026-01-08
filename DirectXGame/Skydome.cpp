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
void Skydome::Intialize(Model* model, Camera* camera) {
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
	// 天球のscaleをクソデカくすること
	worldTransformSkydome_.scale_ = {100.0f, 100.0f, 100.0f};

	// 行列を定数バッファに転送
	// worldTransformSkydome_.translation_ = cameraSkydome_->translation_;	// カメラ位置の移動に合わせて天球が動く様になるらしい(メモ)
	worldTransformSkydome_.TransferMatrix();
}

/// <summary>
/// 天球の描画
/// </summary>
void Skydome::Draw() {
	// 天球を描画
	modelSkydome_->Draw(worldTransformSkydome_, *cameraSkydome_);
}