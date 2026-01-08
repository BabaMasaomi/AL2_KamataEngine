#include "GameScene.h"
#include "KamataEngine.h"

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
GameScene::GameScene() {}
GameScene::~GameScene() {
	delete player_;
	delete modelBlocks_;

	// 複数ブロックの解放処理
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTrasformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTrasformBlocks_.clear();

	delete debugCamera_;
}

// 関数
// 単位行列
Matrix4x4 MakeIdentityMatrix() {
	Matrix4x4 result{};

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result{};

	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;

	return result;
}

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = MakeIdentityMatrix();

	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;
}

// Z回転行列
Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result{};

	// Z
	result.m[0][0] = cosf(radian);
	result.m[0][1] = sinf(radian);
	result.m[1][0] = -sinf(radian);
	result.m[1][1] = cosf(radian);
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// X回転行列
Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result{};

	// X
	result.m[0][0] = 1.0f;
	result.m[1][1] = cosf(radian);
	result.m[1][2] = sinf(radian);
	result.m[2][1] = -sinf(radian);
	result.m[2][2] = cosf(radian);
	result.m[3][3] = 1.0f;

	return result;
}

// Y回転行列
Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result{};

	// y
	result.m[0][0] = cosf(radian);
	result.m[0][2] = -sinf(radian);
	result.m[1][1] = 1.0f;
	result.m[2][0] = sinf(radian);
	result.m[2][2] = cosf(radian);
	result.m[3][3] = 1.0f;

	return result;
}

// 4x4行列の掛け算
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			result.m[y][x] = m1.m[y][0] * m2.m[0][x] + m1.m[y][1] * m2.m[1][x] + m1.m[y][2] * m2.m[2][x] + m1.m[y][3] * m2.m[3][x];
		}
	}
	return result;
}

/*-------------------- メンバ関数 --------------------*/
// 初期化
void GameScene::Initialize() {
	// メンバ変数への代入処理
	

	// ワールドトランスフォームの初期化
	worldTrasform_.Initialize();

	// カメラの初期化
	camera_.Initialize();

	/*-------------------- プレイヤーの生成、初期化 --------------------*/
	// 3Dモデルの生成
	model_ = Model::Create();

	// プレイヤーの生成
	player_ = new Player();

	// プレイヤーの初期化
	player_->Intialize(model_, &camera_);


	/*-------------------- ブロックの生成、初期化 --------------------*/
	// ブロックの3Dモデルの生成
	modelBlocks_ = Model::Create();

	// ブロックの初期化
	// 要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;

	// 1ブロック分の幅
	const float kBlockWidth = 2.0f;  // 横
	const float kBlockHeight = 2.0f; // 縦

	// 要素数を更新する
	// 列数を設定(縦方向のブロック数)
	worldTrasformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		// 1列の要素数を設定(横方向のブロック数)
		worldTrasformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			// チェッカー柄にするための判定
			if ((i + j) % 2 == 0) {
				worldTrasformBlocks_[i][j] = new WorldTransform();
				worldTrasformBlocks_[i][j]->Initialize();
				worldTrasformBlocks_[i][j]->translation_.x = kBlockWidth * j;
				worldTrasformBlocks_[i][j]->translation_.y = kBlockHeight * i;

			} else {
				// 置かないマス
				worldTrasformBlocks_[i][j] = nullptr;
			}
		}
	}

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);
}

// 更新
void GameScene::Update() {
	// インゲームの更新処理
	// プレイヤーの更新
	player_->Update(); // playerに戻せ


	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTrasformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) // 空白ならスキップ
				continue;

			// アフィン変換行列の作成
			worldTransformBlock->scale_ = {1, 1, 1};
			worldTransformBlock->rotation_ = {0, 0, 0};

			Matrix4x4 rotateMatrix = Multiply(MakeRotateXMatrix(0.0f), Multiply(MakeRotateYMatrix(0.0f), MakeRotateZMatrix(0.0f)));

			// ワールド行列を作成
			worldTransformBlock->matWorld_ =
			    Multiply(Multiply(MakeScaleMatrix(worldTransformBlock->scale_),rotateMatrix), MakeTranslateMatrix(worldTransformBlock->translation_));

			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
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

// 描画
void GameScene::Draw() {
	// インゲームの描画処理
	// 3Dモデルの描画
	Model::PreDraw();

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTrasformBlocks_) {
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