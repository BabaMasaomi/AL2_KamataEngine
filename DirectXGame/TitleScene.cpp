#include "TitleScene.h"
#include <numbers>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
TitleScene::TitleScene() {}
TitleScene::~TitleScene() {
	delete modelTitle_;  // タイトルフォントの3Dモデルの解放
	delete modelPlayer_; // プレイヤーの3Dモデルの解放
}

/*-------------------- 初期化 --------------------*/
void TitleScene::Initialize() {

	// カメラ初期化
	camera_.farZ = 550.0f;
	camera_.Initialize();

	// タイトル用フォントのモデル
	modelTitle_ = Model::CreateFromOBJ("titleFont", true);	// タイトルフォントの3Dモデルを生成
	worldTransformTitle_.Initialize();						// タイトルのワールドトランスフォームの初期化
	worldTransformTitle_.translation_ = {0.0f, 5.0f, 0.0f};
	worldTransformTitle_.scale_ = {2.0f, 2.0f, 2.0f};

	// プレイヤーのモデル
	modelPlayer_ = Model::CreateFromOBJ("player", true);		// プレイヤーの3Dモデルを生成
	worldTransformPlayer_.Initialize();							// プレイヤーのワールドトランスフォームの初期化
	worldTransformPlayer_.translation_ = {0.0f, -10.0f, 0.0f};
	worldTransformPlayer_.scale_ = {5.0f, 5.0f, 5.0f};
}

/*-------------------- 更新 --------------------*/
void TitleScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		finished_ = true;
	}

	// タイトルを揺らす
	static float t = 0.0f;
	t += 0.03f;
	worldTransformTitle_.translation_.y = 5.0f + sinf(t) * 0.5f;

	// プレイヤーを回す
	worldTransformPlayer_.rotation_.y += 0.02f;

	// 更新を反映
	transform_.worldMatrixUpdate(worldTransformTitle_);
	transform_.worldMatrixUpdate(worldTransformPlayer_);
	camera_.UpdateMatrix();
}

/*-------------------- 描画 --------------------*/
void TitleScene::Draw() {
	Model::PreDraw();

	// モデルを描画
	modelTitle_->Draw(worldTransformTitle_, camera_);
	modelPlayer_->Draw(worldTransformPlayer_, camera_);

	Model::PostDraw();
}