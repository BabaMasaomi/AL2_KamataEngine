#include "GameScene.h"
#include "KamataEngine.h"

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
GameScene::GameScene() {}
GameScene::~GameScene() { delete player_; }

/*-------------------- メンバ関数 --------------------*/
// 初期化
void GameScene::Initialize() {
	// メンバ変数への代入処理
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("./Resources./uvChecker.png");

	// 3Dモデルの生成
	model_ = Model::Create();

	// ワールドトランスフォームの初期化
	worldTrasform_.Initialize();

	// カメラの初期化
	camera_.Initialize();

	// 自機の生成
	player_ = new Player();

	// 自機の初期化
	player_->Intialize(model_, &camera_);
}

// 更新
void GameScene::Update() {
	// インゲームの更新処理

	// 自機の更新
	player_->Update();
}

// 描画
void GameScene::Draw() {
	// インゲームの描画処理
	// 3Dモデルの描画
	Model::PreDraw();

	// 自機の描画
	player_->Draw();

	Model::PostDraw();
}
