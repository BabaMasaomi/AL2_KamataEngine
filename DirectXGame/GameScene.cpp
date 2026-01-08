#include "GameScene.h"
#include "KamataEngine.h"

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
GameScene::GameScene() {}
GameScene::~GameScene() {
	delete player_;       // プレイヤーの解放
	delete modelSkydome_; // 天球の3Dモデルの解放
	delete modelBlocks_;  // ブロックの3Dモデルの解放

	// 複数ブロックの解放処理
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	delete mapChipField_; //	マップチップフィールドの解放
	delete debugCamera_;  // デバッグカメラの解放
}

/*==============================================================
* メンバ関数
==============================================================*/
/*-------------------- 初期化 --------------------*/
void GameScene::Initialize() {
	// メンバ変数への代入処理
	// カメラの初期化
	camera_.farZ = 550.0f;
	camera_.Initialize();

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField;

	// ファイル読み込み
	mapChipField_->LoadMapChipCsv("Resources/block.csv");

	// 表示ブロックの生成
	GenerateBlocks();

	// プレイヤーの生成、初期化
	// プレイヤーの3Dモデルの生成
	model_ = Model::CreateFromOBJ("player", true);

	// プレイヤーのワールドトランスフォームの初期化
	worldTrasform_.Initialize();

	// プレイヤーの生成
	player_ = new Player();

	// プレイヤーの初期化
	player_->Intialize(model_, &camera_);

	// 天球の生成、初期化
	// 天球の3Dモデルの生成
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	// 天球のワールドトランスフォームの初期化
	worldTrasformSkydome_.Initialize();

	// 天球の生成
	skydome_ = new Skydome();

	// 天球の初期化
	skydome_->Intialize(modelSkydome_, &camera_);

	// ブロックの生成、初期化
	// ブロックの3Dモデルの生成
	modelBlocks_ = Model::CreateFromOBJ("block", true);

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);
}

/*-------------------- 更新 --------------------*/
void GameScene::Update() {
	// インゲームの更新処理
	// プレイヤーの更新
	player_->Update();

	// 天球の更新
	skydome_->Update();

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) // 空白ならスキップ
				continue;

			// アフィン変換行列の作成
			worldTransformBlock->scale_ = {2, 2, 2};
			worldTransformBlock->rotation_ = {0, 0, 0};
			// worldTransformBlock->translation_ = {0, 0, 0};	// Intializeで設定したので変更しない

			// 行列を定数バッファに転送
			transform_.worldMatrixUpdate(*worldTransformBlock);
		}
	}

#ifdef _DEBUG
	// デバッグ起動
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (isDebugCameraActive_) {
			isDebugCameraActive_ = false;
		} else {
			isDebugCameraActive_ = true;
		}
	}

	// カメラの処理
	if (isDebugCameraActive_) {
		// デバッグカメラの更新
		debugCamera_->Update();

		// カメラ位置に行列を適用
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;

		// ビュープロジェクション行列の更新と転送
		camera_.TransferMatrix();

	} else {
		// ビュープロジェクション行列の更新と転送
		camera_.UpdateMatrix();
	}

#endif // DEBUG
}

/*-------------------- 描画 --------------------*/
void GameScene::Draw() {
	// インゲームの描画処理
	// 3Dモデルの描画
	Model::PreDraw();

	// 天球の描画
	skydome_->Draw();

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) // 空白ならスキップ
				continue;

			modelBlocks_->Draw(*worldTransformBlock, camera_);
		}
	}

	// プレイヤーの描画
	player_->Draw();

	Model::PostDraw();
}

/*-------------------- 表示ブロックの生成 --------------------*/
void GameScene::GenerateBlocks() {
	// 要素数
	uint32_t kNumBlockVirtical = MapChipField::kNumBlockVirchical;
	uint32_t kNumBlockHorizontal = MapChipField::kNumBlockHorizontal;

	// 要素数を更新する
	// 列数を設定(縦方向のブロック数)
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		// 1列の要素数を設定(横方向のブロック数)
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform;
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}