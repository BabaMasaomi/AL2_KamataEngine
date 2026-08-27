#include "GameScene.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <random>
#include <string>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
GameScene::GameScene() {}
GameScene::~GameScene() {
	delete player_; // プレイヤーの解放

	for (Enemy* enemy : enemies_) {
		delete enemy; // 敵の解放(範囲for文を使う)
	}

	delete modelSkydome_; // 天球の3Dモデルの解放
	delete modelBlocks_;  // ブロックの3Dモデルの解放

	for (HitEffect* hitEffect : hitEffects_) {
		delete hitEffect; // ヒットエフェクトの3Dモデルの解放
	}

	// 溜め攻撃エフェクトの解放
	delete chargeEffect_;
	chargeEffect_ = nullptr;

	// プレイヤー死亡演出の解放
	delete playerDeathEffect_;
	playerDeathEffect_ = nullptr;

	// 複数ブロックの解放処理
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	for (DeathParticles* deathParticles : {deathParticles_}) {
		delete deathParticles; // パーティクルの解放
	}

	delete mapChipField_;     // マップチップフィールドの解放
	delete camaraController_; // カメラコントローラの解放
	delete debugCamera_;      // デバッグカメラの解放
	// チュートリアル用のガイドスプライトの解放
	delete moveGuideSprite_;
	moveGuideSprite_ = nullptr;

	delete normalAttackGuideSprite_;
	normalAttackGuideSprite_ = nullptr;

	delete chargedAttackGuideSprite_;
	chargedAttackGuideSprite_ = nullptr;

	delete hitTargetGuideSprite_;
	hitTargetGuideSprite_ = nullptr;
	// スコアの解放
	for (Sprite* digitSprite : scoreDigitSprites_) {
		delete digitSprite;
	}
	scoreDigitSprites_.clear();

	// カウントダウンの解放
	delete countdownSprite_;
	countdownSprite_ = nullptr;

	// 残り時間の解放
	for (Sprite* timeDigitSprite : timeDigitSprites_) {

		delete timeDigitSprite;
	}

	// HPの解放
	for (Sprite* hpDigitSprite : hpDigitSprites_) {

		delete hpDigitSprite;
	}

	hpDigitSprites_.clear();

	// アイコン類を解放
	delete scoreIconSprite_;
	scoreIconSprite_ = nullptr;

	delete timeIconSprite_;
	timeIconSprite_ = nullptr;

	// ゲーム終了時のスプライトを解放
	delete gameFinishSprite_;
	gameFinishSprite_ = nullptr;

	timeDigitSprites_.fill(nullptr);
	delete fade_; // フェードの解放
}

/*==============================================================
* メンバ関数
==============================================================*/
/*-------------------- 初期化 --------------------*/
void GameScene::Initialize(bool skipTutorial) {
	// メンバ変数への代入処理
	// フェーズをフェードインから開始
	phase_ = Phase::kFadeIn;

	// カメラの初期化
	camera_.farZ = 550.0f;
	camera_.Initialize();

	// マップチップフィールドの生成、初期化
	// マップチップフィールドの生成
	mapChipField_ = new MapChipField;

	// ファイル読み込み
	mapChipField_->LoadMapChipCsv("Resources/batField.csv");

	// 表示ブロックの生成
	GenerateBlocks();

	/*--------------- プレイヤー ---------------*/
	// プレイヤーの3Dモデルの生成
	model_ = Model::CreateFromOBJ("CapPlayer", true);
	// バットモデルの生成
	modelAttack_ = Model::CreateFromOBJ("Bat", true);

	// プレイヤーのワールドトランスフォームの初期化
	worldTransformPlayer_.Initialize();

	// プレイヤーの生成
	player_ = new Player();

	// 座標をマップチップ番号で指定
	Vector3 playerPos;

	if (skipTutorial) {
		// リスタート時：ステージ中央の地面
		playerPos = mapChipField_->GetMapChipPositionByIndex(30, 17);

	} else {
		// 通常開始時：チュートリアル用の左端
		playerPos = mapChipField_->GetMapChipPositionByIndex(2, 17);
	}

	// プレイヤーの初期化
	player_->Initialize(model_, modelAttack_, &camera_, playerPos);

	// マップチップデータのセット
	player_->SetMapChipField(mapChipField_);

	/*--------------- 雑魚敵 ---------------*/
	// 敵の3Dモデルの生成
	modelEnemy_ = Model::CreateFromOBJ("balloonEnemy", true);

	// 背景用の敵を生成
	InitializeBackgroundEnemies();

	if (skipTutorial) {
		// リトライ時は練習用の敵を生成しない
		tutorialLauncherEnemy_ = nullptr;
		tutorialTargetEnemy_ = nullptr;
		tutorialState_ = TutorialState::kFinished;
		isMainGameStarted_ = false;
	} else {
		InitializeTutorial();
	}

	isReinforcementUnlocked_ = false;
	playTimer_ = 0.0f;

	/*--------------- 天球 ---------------*/
	// 天球の3Dモデルの生成
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	// 天球のワールドトランスフォームの初期化
	worldTransformSkydome_.Initialize();

	// 天球の生成
	skydome_ = new Skydome();

	// 天球の初期化
	skydome_->Initialize(modelSkydome_, &camera_);

	/*--------------- ブロック ---------------*/
	// ブロックの3Dモデルの生成
	modelBlocks_ = Model::CreateFromOBJ("BatFieldBox", true);

	/*--------------- パーティクル ---------------*/
	// パーティクルの3Dモデルの生成
	modelParticles_ = Model::CreateFromOBJ("deathParticle", true);

	// パーティクルのワールドトランスフォームの初期化
	worldTransformPlayer_.Initialize();

	/*--------------- ヒットエフェクト ---------------*/
	// モデルの読み込み
	hitEffectModel_ = Model::CreateFromOBJ("particle", true);
	guardEffectModel_ = Model::CreateFromOBJ("ring", true);

	// circle.pngを使用するparticleモデルを残像にも共有
	Enemy::SetTrailModel(hitEffectModel_);

	HitEffect::SetHitModel(hitEffectModel_);
	HitEffect::SetGuardModel(guardEffectModel_);
	HitEffect::SetCamera(&camera_);

	// 溜め攻撃のエフェクトの生成
	chargeEffect_ = new ChargeEffect();
	chargeEffect_->Initialize(guardEffectModel_, &camera_);

	// プレイヤー死亡演出の生成
	playerDeathEffect_ = new PlayerDeathEffect();
	playerDeathEffect_->Initialize(hitEffectModel_, &camera_);

	/*--------------- カメラ ---------------*/
	// カメラコントローラの生成
	camaraController_ = new CameraController();

	// カメラコントローラの初期化
	camaraController_->Initialize(&camera_);

	// 追従対象をセット
	camaraController_->SetTarget(player_);

	// 現在読み込んでいるマップから移動範囲を計算
	CameraController::Rect cameraArea = CalculateCameraMovableArea();

	camaraController_->SetMovableArea(cameraArea);

	// リセット(瞬間合わせ)
	camaraController_->Reset();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	/*-------------------- プレイヤー死亡演出 --------------------*/
	hasCreatedPlayerDeathEffect_ = false;

	/*-------------------- ゲームの終了判定 --------------------*/
	finished_ = false;
	gameResult_ = GameResult::kNone;

	/*--------------- UI類 ---------------*/
	// カウントダウンの初期化
	InitializeCountdown();

	// スコア表示の初期化
	InitializeScoreDisplay();

	// 残り時間表示の初期化
	InitializeTimeDisplay();

	// HP表示の初期化
	InitializeHpDisplay();

	// チュートリアルガイドの初期化
	InitializeTutorialGuides();

	/*--------------- 終了演出 ---------------*/
	uint32_t gameFinishTexture = TextureManager::Load("GameFinish.png");

	gameFinishSprite_ = Sprite::Create(gameFinishTexture, {640.0f, 360.0f});

	gameFinishSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	gameFinishSprite_->SetSize({
	    kGameFinishWidth,
	    kGameFinishHeight,
	});

	gameFinishTimer_ = 0.0f;

	/*--------------- フェード ---------------*/
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// リトライ時はフェードイン後、チュートリアルを挟まず3カウントへ進む
	if (skipTutorial) {
		StartCountdown();
	}
}

/*-------------------- 更新 --------------------*/
void GameScene::Update() {
	const float deltaTime = 1.0f / 60.0f;

	if (player_) {
		UpdateHpDisplay();
	}

	// ヒットストップ中
	if (phase_ == Phase::kPlay && hitStopTimer_ > 0.0f) {

		hitStopTimer_ -= deltaTime;
		hitStopTimer_ = (std::max)(hitStopTimer_, 0.0f);

		// エフェクトを含め、すべて停止
		return;
	}

	// プレイ時間タイマー
	if (isMainGameStarted_) {
		playTimer_ += deltaTime;

		playTimer_ = (std::min)(playTimer_, kClearTime);

		UpdateTimeDisplay();
	}

	// フェーズごとの更新処理
	switch (phase_) {
	case ::GameScene::Phase::kFadeIn:

		UpdateBackgroundEnemies();

		// 天球の更新
		skydome_->Update();

		// フェード処理処理宙にプレイヤーを正しい位置に描画させる
		player_->Update();

		if (chargeEffect_ && player_) {
			chargeEffect_->Update(player_->IsCharging(), player_->IsChargeReady(), player_->GetChargeRatio(), player_->GetChargeEffectPosition());
		}

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		// 全員更新後に1回だけ実行
		ResolveEnemyOverlaps();

		// カメラコントローラの更新
		camaraController_->Update();

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

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock) // 空白ならスキップ
					continue;

				// アフィン変換行列の作成
				worldTransformBlock->scale_ = {2.0f, 2.0f, 2.0f};
				worldTransformBlock->rotation_ = {0.0f, 0.0f, 0.0f};
				// worldTransformBlock->translation_ = {0, 0, 0};	// Initializeで設定したので変更しない

				// 行列を定数バッファに転送
				transform_.worldMatrixUpdate(*worldTransformBlock);
			}
		}

		// 総当たり当たり判定
		// プレイヤーと敵
		CheckAllCollisions();
		// 敵同士
		CheckEnemyCollisions();

		fade_->Update();
		break;

	case GameScene::Phase::kPlay: {

		// カウントダウン中
		if (isCountdownActive_) {
			// 背景は動かし続ける
			skydome_->Update();
			UpdateBackgroundEnemies();

			// カウントを更新
			UpdateCountdown();

			// カメラは現在位置を維持
			camaraController_->Update();

			if (isDebugCameraActive_) {
				debugCamera_->Update();

				camera_.matView = debugCamera_->GetCamera().matView;

				camera_.matProjection = debugCamera_->GetCamera().matProjection;

				camera_.TransferMatrix();

			} else {
				camera_.UpdateMatrix();
			}

			break;
		}

		// ここから従来のプレイ更新
		// インゲームの更新処理

		UpdateBackgroundEnemies();

		// 天球の更新
		skydome_->Update();

		// プレイヤーの更新
		player_->Update();

		if (chargeEffect_ && player_) {
			chargeEffect_->Update(player_->IsCharging(), player_->IsChargeReady(), player_->GetChargeRatio(), player_->GetChargeEffectPosition());
		}

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		// 通常・スタン状態の敵同士の重なりを解消
		ResolveEnemyOverlaps();

		// デスフラグの立った敵を削除
		size_t defeatedEnemyCount = 0;

		enemies_.remove_if([this, &defeatedEnemyCount](Enemy* enemy) {
			if (!enemy->GetIsDead()) {
				return false;
			}

			// 削除後にダングリングポインタを残さない
			if (enemy == tutorialLauncherEnemy_) {
				tutorialLauncherEnemy_ = nullptr;
			}

			if (enemy == tutorialTargetEnemy_) {
				tutorialTargetEnemy_ = nullptr;
			}

			// 通常プレイの敵だけ撃破数として扱う
			if (!enemy->IsTutorialEnemy()) {
				++defeatedEnemyCount;
			}

			delete enemy;
			return true;
		});

		// 本プレイ中に倒した通常敵だけスコアへ加算
		if (isMainGameStarted_ && defeatedEnemyCount > 0) {

			score_ += static_cast<uint32_t>(defeatedEnemyCount) * kScorePerEnemy;

			UpdateScoreDisplay();
		}

		// 最初の敵が倒されたら補充を解放
		if (defeatedEnemyCount > 0) {
			isReinforcementUnlocked_ = true;
		}

		// 制限時間前だけ敵を補充
		if (isMainGameStarted_ && playTimer_ < kClearTime) {
			ReplenishEnemies();
		}

		if (!isMainGameStarted_) {
			UpdateTutorial();
		}

		// ヒットエフェクトの更新
		UpdateHitEffects();

		// カメラコントローラの更新
		camaraController_->Update();

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

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock) // 空白ならスキップ
					continue;

				// アフィン変換行列の作成
				worldTransformBlock->scale_ = {2.0f, 2.0f, 2.0f};
				worldTransformBlock->rotation_ = {0.0f, 0.0f, 0.0f};
				// worldTransformBlock->translation_ = {0, 0, 0};	// Initializeで設定したので変更しない

				// 行列を定数バッファに転送
				transform_.worldMatrixUpdate(*worldTransformBlock);
			}
		}

		// 総当たり当たり判定
		// プレイヤーと敵
		CheckAllCollisions();
		// 敵同士
		CheckEnemyCollisions();

		break;
	}

	case GameScene::Phase::kDeath:
		// デス演出の更新処理

		UpdateBackgroundEnemies();

		// 天球の更新
		skydome_->Update();

		// プレイヤー本体の死亡演出を更新
		player_->UpdateDeathAnimation();

		// プレイヤー専用の光エフェクト
		if (playerDeathEffect_) {
			playerDeathEffect_->Update();
		}

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		// 通常・スタン状態の敵同士の重なりを解消
		ResolveEnemyOverlaps();

		// ヒットエフェクトの更新
		for (HitEffect* hitEffect : hitEffects_) {
			hitEffect->UpDate();
		}

		// デスフラグの立ったエフェクトを削除
		hitEffects_.remove_if([](HitEffect* effect) {
			if (effect->GetIsDead()) {
				delete effect;
				return true;
			}

			return false;
		});

		// パーティクルの更新
		if (deathParticles_ != nullptr) {
			deathParticles_->Update();
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

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock) // 空白ならスキップ
					continue;

				// アフィン変換行列の作成
				worldTransformBlock->scale_ = {2.0f, 2.0f, 2.0f};
				worldTransformBlock->rotation_ = {0.0f, 0.0f, 0.0f};

				// 行列を定数バッファに転送
				transform_.worldMatrixUpdate(*worldTransformBlock);
			}
		}

		break;

	case GameScene::Phase::kFinish:
		// 終了表示中はプレイヤーと敵を停止
		gameFinishTimer_ += deltaTime;
		break;

	case ::GameScene::Phase::kFadeOut:
		fade_->Update();
		break;

	default:
		break;
	}

	// フェーズの切り替え処理
	ChangePhase();

#ifdef _DEBUG
	// デバッグ起動
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		if (isDebugCameraActive_) {
			isDebugCameraActive_ = false;
		} else {
			isDebugCameraActive_ = true;
		}
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

	// 背景用の敵を描画
	for (BackgroundEnemy* backgroundEnemy : backgroundEnemies_) {
		if (backgroundEnemy) {
			backgroundEnemy->Draw();
		}
	}

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) // 空白ならスキップ
				continue;

			modelBlocks_->Draw(*worldTransformBlock, camera_);
		}
	}

	// 敵の描画
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}

	// ヒットエフェクトの描画
	for (HitEffect* hitEffect : hitEffects_) {
		hitEffect->Draw();
	}

	// パーティクルの描画
	if (deathParticles_ != nullptr) {
		deathParticles_->Draw();
	}

	// プレイヤーの描画
	player_->Draw();

	// プレイヤー専用の死亡光
	if (playerDeathEffect_) {
		playerDeathEffect_->Draw();
	}

	// 溜め攻撃のエフェクトの描画
	if (chargeEffect_) {
		chargeEffect_->Draw();
	}

	Model::PostDraw();

	/*========== 2Dスプライト描画 ==========*/
	Sprite::PreDraw();

	// チュートリアル操作ガイド
	DrawTutorialGuide();

	// HPは左上へ表示
	if (phase_ == Phase::kPlay) {
		DrawHp();
	}

	// 本プレイ開始後の右上UI
	if (isMainGameStarted_) {
		DrawRemainingTime();
		DrawScore();
	}

	// 開始カウントダウン
	if (isCountdownActive_ && countdownSprite_) {
		countdownSprite_->Draw();
	}

	if ((phase_ == Phase::kFinish || phase_ == Phase::kFadeOut) && gameFinishSprite_) {

		gameFinishSprite_->Draw();
	}

	Sprite::PostDraw();

	// フェードを描画
	fade_->Draw();
}

/*-------------------- 指定位置へ敵を1体生成 --------------------*/
void GameScene::SpawnEnemy(const Vector3& position) {

	Enemy* newEnemy = new Enemy();

	newEnemy->SetMapChipField(mapChipField_);

	newEnemy->Initialize(modelEnemy_, &camera_, position, true);

	newEnemy->SetTarget(player_);
	newEnemy->SetGameScene(this);

	enemies_.push_back(newEnemy);
}

/*-------------------- 画面外の補充位置を探す --------------------*/
Vector3 GameScene::FindReinforcementSpawnPosition() {
	constexpr float kCameraHalfWidth = 21.0f;
	constexpr float kOutsideMargin = 2.0f;

	// 地形へ少しめり込まないための余白
	constexpr float kSpawnHeightMargin = 0.05f;

	// 既存の敵から離す距離
	constexpr float kMinimumSpawnDistance = 5.0f;

	// 浮遊足場を選ぶ確率
	constexpr float kElevatedSpawnRate = 0.70f;

	std::vector<Vector3> elevatedCandidates;
	std::vector<Vector3> groundCandidates;

	/*========== 実際に使われている右端を探す ==========*/

	uint32_t rightBoundaryIndex = 0;

	for (uint32_t y = 0; y < MapChipField::kNumBlockVertical; ++y) {

		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {

			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kBlock) {

				rightBoundaryIndex = (std::max)(rightBoundaryIndex, x);
			}
		}
	}

	/*========== 足場の上面を出現候補にする ==========*/

	/*
	 * y = 0はマップ上端の外壁なので除外。
	 * 上側のマスを調べるため、yは1から開始する。
	 */
	for (uint32_t y = 1; y < MapChipField::kNumBlockVertical; ++y) {

		for (uint32_t x = 2; x + 1 < rightBoundaryIndex; ++x) {

			// 現在位置がブロックでなければ床ではない
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kBlock) {

				continue;
			}

			// 1マス上が空白でなければ、その上には立てない
			if (mapChipField_->GetMapChipTypeByIndex(x, y - 1) != MapChipType::kBlank) {

				continue;
			}

			/*
			 * 足場の端への出現を避ける。
			 * 左右もブロックになっている場所だけを使う。
			 */
			bool hasLeftFloor = mapChipField_->GetMapChipTypeByIndex(x - 1, y) == MapChipType::kBlock;

			bool hasRightFloor = mapChipField_->GetMapChipTypeByIndex(x + 1, y) == MapChipType::kBlock;

			if (!hasLeftFloor || !hasRightFloor) {
				continue;
			}

			MapChipField::Rect floorRect = mapChipField_->GetRectByIndex(x, y);

			Vector3 candidatePosition = {
			    mapChipField_->GetMapChipPositionByIndex(x, y).x,

			    floorRect.top + Enemy::GetHeight() * 0.5f + kSpawnHeightMargin,

			    0.0f,
			};

			// プレイヤーから離す最低距離
			constexpr float kMinimumPlayerSpawnDistance = 16.0f;

			Vector3 playerPosition = player_->GetWorldPos();

			float differenceX = candidatePosition.x - playerPosition.x;
			float differenceY = candidatePosition.y - playerPosition.y;

			float distanceSquared = differenceX * differenceX + differenceY * differenceY;
			float minimumDistanceSquared = kMinimumPlayerSpawnDistance * kMinimumPlayerSpawnDistance;

			// プレイヤーに近すぎる位置には出現させない
			if (distanceSquared < minimumDistanceSquared) {

				continue;
			}

			/*
			 * 最下段のブロックなら地面、
			 * それ以外なら浮遊足場として分類する。
			 */
			if (y == MapChipField::kNumBlockVertical - 1) {

				groundCandidates.push_back(candidatePosition);

			} else {

				elevatedCandidates.push_back(candidatePosition);
			}
		}
	}

	/*========== 候補をランダムに並べる ==========*/

	static std::random_device randomDevice;
	static std::mt19937 randomEngine(randomDevice());

	std::shuffle(elevatedCandidates.begin(), elevatedCandidates.end(), randomEngine);

	std::shuffle(groundCandidates.begin(), groundCandidates.end(), randomEngine);

	/*========== 既存の敵との距離を確認 ==========*/

	auto findSeparatedCandidate = [this, kMinimumSpawnDistance](const std::vector<Vector3>& candidates, Vector3& result) {
		const float minimumDistanceSquared = kMinimumSpawnDistance * kMinimumSpawnDistance;

		for (const Vector3& candidate : candidates) {

			bool isTooClose = false;

			for (Enemy* enemy : enemies_) {
				if (!enemy) {
					continue;
				}

				Vector3 enemyPosition = enemy->GetWorldPos();

				float differenceX = candidate.x - enemyPosition.x;

				float differenceY = candidate.y - enemyPosition.y;

				float distanceSquared = differenceX * differenceX + differenceY * differenceY;

				if (distanceSquared < minimumDistanceSquared) {

					isTooClose = true;
					break;
				}
			}

			if (!isTooClose) {
				result = candidate;
				return true;
			}
		}

		return false;
	};

	/*========== 足場と地面のどちらを優先するか決定 ==========*/

	std::uniform_real_distribution<float> rateDistribution(0.0f, 1.0f);

	bool preferElevated = rateDistribution(randomEngine) < kElevatedSpawnRate;

	Vector3 selectedPosition{};

	if (preferElevated) {
		// まず浮遊足場を探す
		if (findSeparatedCandidate(elevatedCandidates, selectedPosition)) {

			return selectedPosition;
		}

		// 見つからなければ地面
		if (findSeparatedCandidate(groundCandidates, selectedPosition)) {

			return selectedPosition;
		}

	} else {
		// まず地面を探す
		if (findSeparatedCandidate(groundCandidates, selectedPosition)) {

			return selectedPosition;
		}

		// 見つからなければ浮遊足場
		if (findSeparatedCandidate(elevatedCandidates, selectedPosition)) {

			return selectedPosition;
		}
	}

	/*
	 * 敵同士の距離条件を満たす場所がなかった場合も、
	 * 地形上の候補があるならそこから選ぶ。
	 */
	if (preferElevated && !elevatedCandidates.empty()) {

		return elevatedCandidates.front();
	}

	if (!groundCandidates.empty()) {
		return groundCandidates.front();
	}

	if (!elevatedCandidates.empty()) {
		return elevatedCandidates.front();
	}

	/*========== 最終的な予備位置 ==========*/

	float direction = player_->GetWorldPos().x < camera_.translation_.x ? 1.0f : -1.0f;

	return {
	    camera_.translation_.x + direction * (kCameraHalfWidth + kOutsideMargin),
	    4.0f,
	    0.0f,
	};
}

/*-------------------- 上限まで敵を補充 --------------------*/
void GameScene::ReplenishEnemies() {
	if (!isReinforcementUnlocked_) {
		return;
	}

	// 既に最大数ならタイマーを初期化
	if (enemies_.size() >= kMaxEnemyCount) {
		reinforcementSpawnTimer_ = 0.0f;
		return;
	}

	// 補充待ち時間を進める
	reinforcementSpawnTimer_ += 1.0f / 60.0f;

	if (reinforcementSpawnTimer_ < kReinforcementSpawnInterval) {
		return;
	}

	reinforcementSpawnTimer_ = 0.0f;

	// 1回につき1体だけ補充する
	SpawnEnemy(FindReinforcementSpawnPosition());
}

/*-------------------- エフェクトの生成 --------------------*/
void GameScene::CreateHitEffect(Vector3 pos, HitEffectType type) {
	HitEffect* newHitEffect = HitEffect::Create(pos, type);
	hitEffects_.push_back(newHitEffect);
}

/*-------------------- ヒットエフェクトを更新 --------------------*/
void GameScene::UpdateHitEffects() {
	// ヒットエフェクトの更新
	for (HitEffect* hitEffect : hitEffects_) {
		hitEffect->UpDate();
	}

	// 終了したエフェクトを削除
	hitEffects_.remove_if([](HitEffect* effect) {
		if (effect->GetIsDead()) {
			delete effect;
			return true;
		}

		return false;
	});
}

/*-------------------- ヒットストップ開始 --------------------*/
void GameScene::StartChargedAttackHitStop() { hitStopTimer_ = (std::max)(hitStopTimer_, kChargedAttackHitStopTime); }

/*-------------------- 表示ブロックの生成 --------------------*/
void GameScene::GenerateBlocks() {
	// 要素数
	uint32_t kNumBlockVirtical = MapChipField::kNumBlockVertical;
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

/*-------------------- 総当たり当たり判定 --------------------*/
void GameScene::CheckAllCollisions() {
	AABB playerAABB, attackAABB, enemyAABB;

	// 自キャラのAABB取得
	playerAABB = player_->GetAABB();

	// 敵全員と当たり判定
	for (Enemy* enemy : enemies_) {
		// コリジョン無効の敵はスキップ
		if (enemy->IsCollisionDisEnabled()) {
			continue;
		}
		// 敵のAABB取得
		enemyAABB = enemy->GetAABB();

		bool hitByPlayerAttack = false;

		// 衝突応答
		// プレイヤーが攻撃可能状態か
		if (player_->CanAttackEnemy()) {
			attackAABB = player_->GetAttackAABB();

			if (CheckAABBCollision(attackAABB, enemyAABB)) {
				hitByPlayerAttack = enemy->OnCollisionPlayer(player_);
			}
		}

		// 自キャラの衝突判定時の処理
		if (!hitByPlayerAttack && enemy->CanDamagePlayer() && CheckAABBCollision(playerAABB, enemyAABB)) {

			if (player_->CanReceiveDamage()) {
				player_->OnCollisionEnemy(enemy);
			}
		}
	}
}

/*-------------------- 吹き飛び中の敵と他の敵との判定 --------------------*/
void GameScene::CheckEnemyCollisions() {
	for (Enemy* attacker : enemies_) {
		// 吹き飛び中かつ、
		// 現在の区間で攻撃可能な敵だけ
		if (!attacker->CanHitOtherEnemy()) {
			continue;
		}

		AABB attackerAABB = attacker->GetBlownAwayAttackAABB();

		for (Enemy* target : enemies_) {
			// 自分自身は除外
			if (attacker == target) {
				continue;
			}

			// 行動可能な敵だけを対象にする
			if (!target->CanReceiveBlownAwayHit()) {
				continue;
			}

			AABB targetAABB = target->GetAABB();

			if (!CheckAABBCollision(attackerAABB, targetAABB)) {
				continue;
			}

			Vector3 attackerPos = attacker->GetWorldPos();
			Vector3 targetPos = target->GetWorldPos();

			float differenceX = targetPos.x - attackerPos.x;
			float knockBackDirection;

			if (differenceX > 0.001f) {
				knockBackDirection = 1.0f;

			} else if (differenceX < -0.001f) {
				knockBackDirection = -1.0f;

			} else {
				knockBackDirection = attacker->GetBlownAwayDirectionX();
			}

			// 対象へスタンダメージとノックバック
			target->OnCollisionBlownAwayEnemy(knockBackDirection);

			// この飛行区間の攻撃権を消費
			attacker->ConsumeBlownAwayHit();

			// 通常攻撃のエフェクトを流用
			Vector3 effectPos = {
			    (attackerPos.x + targetPos.x) / 2.0f,
			    (attackerPos.y + targetPos.y) / 2.0f,
			    0.0f,
			};

			CreateHitEffect(effectPos, HitEffectType::kHit);

			break;
		}
	}
}

/*-------------------- AABB同士の当たり判定 --------------------*/
bool GameScene::CheckAABBCollision(const AABB& aabb1, const AABB& aabb2) {
	bool isCollide = true;

	// X軸方向の判定
	if (aabb1.max.x < aabb2.min.x || aabb2.max.x < aabb1.min.x) {
		isCollide = false;
	}

	// Y軸方向の判定
	if (aabb1.max.y < aabb2.min.y || aabb2.max.y < aabb1.min.y) {
		isCollide = false;
	}

	// Z軸方向の判定
	if (aabb1.max.z < aabb2.min.z || aabb2.max.z < aabb1.min.z) {
		isCollide = false;
	}

	return isCollide;
}

/*-------------------- 通常・スタン状態の敵同士の重なりを解消 --------------------*/
void GameScene::ResolveEnemyOverlaps() {
	// 完全に密着した状態で再衝突しにくくする余白
	constexpr float kEnemySeparationMargin = 0.01f;

	for (auto firstIterator = enemies_.begin(); firstIterator != enemies_.end(); ++firstIterator) {

		Enemy* first = *firstIterator;

		if (!first || !first->CanResolveEnemyOverlap()) {
			continue;
		}

		auto secondIterator = std::next(firstIterator);

		for (; secondIterator != enemies_.end(); ++secondIterator) {

			Enemy* second = *secondIterator;

			if (!second || !second->CanResolveEnemyOverlap()) {
				continue;
			}

			AABB firstAABB = first->GetAABB();

			AABB secondAABB = second->GetAABB();

			/*========== Y方向が重なっているか ==========*/
			float overlapY = (std::min)(firstAABB.max.y, secondAABB.max.y) - (std::max)(firstAABB.min.y, secondAABB.min.y);

			if (overlapY <= 0.0f) {
				continue;
			}

			/*========== X方向が重なっているか ==========*/
			float overlapX = (std::min)(firstAABB.max.x, secondAABB.max.x) - (std::max)(firstAABB.min.x, secondAABB.min.x);

			if (overlapX <= 0.0f) {
				continue;
			}

			overlapX += kEnemySeparationMargin;

			Vector3 firstPosition = first->GetWorldPos();
			Vector3 secondPosition = second->GetWorldPos();

			/*
			 * firstが左、secondが右になるように
			 * ポインタを入れ替える。
			 */
			Enemy* leftEnemy = first;
			Enemy* rightEnemy = second;

			if (firstPosition.x > secondPosition.x) {
				leftEnemy = second;
				rightEnemy = first;
			}

			bool leftIsStunned = leftEnemy->IsStunned();
			bool rightIsStunned = rightEnemy->IsStunned();

			/*========== ノックバック敵が後ろの敵を押す ==========*/
			bool leftIsHitKnockBack = leftEnemy->IsHitKnockBack();
			bool rightIsHitKnockBack = rightEnemy->IsHitKnockBack();

			/*
			 * 左側の敵が右へノックバックしている場合、
			 * 右側の敵だけを右へ押す。
			 */
			if (leftIsHitKnockBack && leftEnemy->GetHitKnockBackDirection() > 0.0f && !rightIsHitKnockBack) {

				float pushedDistance = rightEnemy->MoveForEnemySeparation(overlapX);

				/*
				 * 右側の敵が壁などで押し切れなかった場合、
				 * 残った重なり分だけノックバック敵を戻す。
				 */
				float remainingOverlap = (std::max)(overlapX - (std::max)(pushedDistance, 0.0f), 0.0f);

				if (remainingOverlap > 0.0f) {
					leftEnemy->MoveForEnemySeparation(-remainingOverlap);
				}

				continue;
			}

			/*
			 * 右側の敵が左へノックバックしている場合、
			 * 左側の敵だけを左へ押す。
			 */
			if (rightIsHitKnockBack && rightEnemy->GetHitKnockBackDirection() < 0.0f && !leftIsHitKnockBack) {

				float pushedMovement = leftEnemy->MoveForEnemySeparation(-overlapX);

				float pushedDistance = (std::max)(-pushedMovement, 0.0f);

				float remainingOverlap = (std::max)(overlapX - pushedDistance, 0.0f);

				if (remainingOverlap > 0.0f) {
					rightEnemy->MoveForEnemySeparation(remainingOverlap);
				}

				continue;
			}

			/*========== 通常敵同士 ==========*/
			if (!leftIsStunned && !rightIsStunned) {

				/*
				 * 足場への移動中は、全員が同じ踏み切り位置を
				 * 目指す可能性がある。
				 *
				 * この状態で位置補正すると互いを押し戻して
				 * スタックするため、通常敵同士の補正を行わない。
				 */
				if (leftEnemy->IsUsingPlatformRoute() || rightEnemy->IsUsingPlatformRoute()) {
					continue;
				}

				float halfCorrection = overlapX * 0.5f;

				leftEnemy->MoveForEnemySeparation(-halfCorrection);

				rightEnemy->MoveForEnemySeparation(halfCorrection);

				continue;
			}

			/*========== 左が通常、右がスタン ==========*/
			if (!leftIsStunned && rightIsStunned) {

				// まずスタン敵を右へ押す
				float stunnedMovement = rightEnemy->MoveForEnemySeparation(overlapX);
				float resolvedDistance = (std::max)(stunnedMovement, 0.0f);
				float remainingOverlap = (std::max)(overlapX - resolvedDistance, 0.0f);

				/*
				 * スタン敵が壁で動けなかった分だけ
				 * 通常敵を左へ戻す。
				 */
				if (remainingOverlap > 0.0f) {
					leftEnemy->MoveForEnemySeparation(-remainingOverlap);
				}

				continue;
			}

			/*========== 左がスタン、右が通常 ==========*/
			if (leftIsStunned && !rightIsStunned) {

				// まずスタン敵を左へ押す
				float stunnedMovement = leftEnemy->MoveForEnemySeparation(-overlapX);
				float resolvedDistance = (std::max)(-stunnedMovement, 0.0f);
				float remainingOverlap = (std::max)(overlapX - resolvedDistance, 0.0f);

				if (remainingOverlap > 0.0f) {
					rightEnemy->MoveForEnemySeparation(remainingOverlap);
				}

				continue;
			}

			/*========== スタン敵同士 ==========*/
			float halfCorrection = overlapX * 0.5f;

			leftEnemy->MoveForEnemySeparation(-halfCorrection);

			rightEnemy->MoveForEnemySeparation(halfCorrection);
		}
	}
}

/*-------------------- スタンした敵の周囲にいる通常敵を押し出す --------------------*/
void GameScene::PushEnemiesAroundStunned(Enemy* stunnedEnemy) {

	if (!stunnedEnemy) {
		return;
	}

	// 衝撃が届く横方向の範囲
	constexpr float kShockwaveRangeX = 4.0f;

	// 上下に離れた別足場の敵には当てない
	constexpr float kShockwaveRangeY = 1.5f;

	Vector3 sourcePosition = stunnedEnemy->GetWorldPos();

	for (Enemy* enemy : enemies_) {
		if (!enemy || enemy == stunnedEnemy || !enemy->CanReceiveStunShockwave()) {
			continue;
		}

		Vector3 targetPosition = enemy->GetWorldPos();

		float differenceX = targetPosition.x - sourcePosition.x;

		float differenceY = targetPosition.y - sourcePosition.y;

		if (std::abs(differenceX) > kShockwaveRangeX || std::abs(differenceY) > kShockwaveRangeY) {
			continue;
		}

		float direction = 0.0f;

		if (differenceX > 0.001f) {
			direction = 1.0f;

		} else if (differenceX < -0.001f) {
			direction = -1.0f;

		} else {
			/*
			 * 中心が完全に一致した場合は、
			 * ポインタの並びで左右を決める。
			 */
			direction = enemy > stunnedEnemy ? 1.0f : -1.0f;
		}

		enemy->StartStunShockwaveKnockBack(direction);
	}
}

/*-------------------- マップのブロック配置からカメラ移動範囲を計算 --------------------*/
CameraController::Rect GameScene::CalculateCameraMovableArea() {
	bool foundBlock = false;

	float mapLeft = 0.0f;
	float mapRight = 0.0f;
	float mapBottom = 0.0f;
	float mapTop = 0.0f;

	for (uint32_t y = 0; y < MapChipField::kNumBlockVertical; ++y) {

		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {

			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(x, y);

			if (!foundBlock) {
				mapLeft = blockRect.left;
				mapRight = blockRect.right;
				mapBottom = blockRect.bottom;
				mapTop = blockRect.top;

				foundBlock = true;
				continue;
			}

			mapLeft = (std::min)(mapLeft, blockRect.left);

			mapRight = (std::max)(mapRight, blockRect.right);

			mapBottom = (std::min)(mapBottom, blockRect.bottom);

			mapTop = (std::max)(mapTop, blockRect.top);
		}
	}

	if (!foundBlock) {
		return CameraController::Rect{0.0f, 0.0f, 0.0f, 0.0f};
	}

	/*
	 * 現在のカメラ距離Z=-30における、
	 * おおよその表示範囲。
	 */
	constexpr float kCameraHalfWidth = 21.0f;
	constexpr float kCameraHalfHeight = kCameraHalfWidth * 9.0f / 16.0f;

	float cameraLeft = mapLeft + kCameraHalfWidth;
	float cameraRight = mapRight - kCameraHalfWidth;

	float cameraBottom = mapBottom + kCameraHalfHeight;
	float cameraTop = mapTop - kCameraHalfHeight;

	// 画面より小さいマップにも対応
	if (cameraLeft > cameraRight) {
		float center = (mapLeft + mapRight) * 0.5f;

		cameraLeft = center;
		cameraRight = center;
	}

	if (cameraBottom > cameraTop) {
		float center = (mapBottom + mapTop) * 0.5f;

		cameraBottom = center;
		cameraTop = center;
	}

	return CameraController::Rect{
	    cameraLeft,
	    cameraRight,
	    cameraBottom,
	    cameraTop,
	};
}

/*--------------------  --------------------*/
void GameScene::InitializeTutorial() {
	tutorialState_ = TutorialState::kMove;
	isMainGameStarted_ = false;

	playTimer_ = 0.0f;
	isReinforcementUnlocked_ = false;

	// 吹き飛ばされる練習用の敵
	tutorialLauncherEnemy_ = new Enemy();
	tutorialLauncherEnemy_->SetMapChipField(mapChipField_);
	tutorialLauncherEnemy_->Initialize(modelEnemy_, &camera_, mapChipField_->GetMapChipPositionByIndex(28, 17));
	tutorialLauncherEnemy_->SetGameScene(this);
	tutorialLauncherEnemy_->SetPurpose(EnemyPurpose::kTutorialLauncher);

	// SetTarget(player_)は呼ばない
	enemies_.push_back(tutorialLauncherEnemy_);

	// 吹き飛ばした敵を当てる標的
	tutorialTargetEnemy_ = new Enemy();
	tutorialTargetEnemy_->SetMapChipField(mapChipField_);
	tutorialTargetEnemy_->Initialize(modelEnemy_, &camera_, mapChipField_->GetMapChipPositionByIndex(34, 17));
	tutorialTargetEnemy_->SetGameScene(this);
	tutorialTargetEnemy_->SetPurpose(EnemyPurpose::kTutorialTarget);

	enemies_.push_back(tutorialTargetEnemy_);
}

/*--------------------  --------------------*/
void GameScene::UpdateTutorial() {

	// 倒し方や現在の説明段階に関係なく、
	// チュートリアル敵が2体とも消えたら終了する
	if (tutorialState_ != TutorialState::kFinished && tutorialLauncherEnemy_ == nullptr && tutorialTargetEnemy_ == nullptr) {

		tutorialState_ = TutorialState::kFinished;
	}

	switch (tutorialState_) {
	case TutorialState::kMove:
		// ステージ中央付近に到着
		if (player_->GetWorldPos().x >= kTutorialCenterX) {
			tutorialState_ = TutorialState::kNormalAttack;
		}
		break;

	case TutorialState::kNormalAttack:
		// 通常攻撃を必要回数当て、敵がスタンした
		if (tutorialLauncherEnemy_ && tutorialLauncherEnemy_->IsStunned()) {

			tutorialState_ = TutorialState::kChargedAttack;
		}
		break;

	case TutorialState::kChargedAttack:
		// スタン敵に溜め攻撃が当たった
		if (tutorialLauncherEnemy_ && tutorialLauncherEnemy_->IsBlownAway()) {

			tutorialState_ = TutorialState::kHitTarget;
		}
		break;

	case TutorialState::kHitTarget:

		/*
		 * ぶつけられた標的と、
		 * 吹き飛ばした敵の両方が死亡演出を終え、
		 * enemies_から削除されるまで待つ。
		 */
		if (!tutorialTargetEnemy_ && !tutorialLauncherEnemy_) {

			tutorialState_ = TutorialState::kFinished;
		}

		break;

	case TutorialState::kFinished:
		if (!isCountdownActive_) {
			StartCountdown();
		}
		break;
	}
}

/*--------------------  --------------------*/
void GameScene::StartMainGame() {
	if (isMainGameStarted_) {
		return;
	}

	isMainGameStarted_ = true;

	score_ = 0;
	UpdateScoreDisplay();
	playTimer_ = 0.0f;

	displayedRemainingTime_ = -1;
	UpdateTimeDisplay();

	// 本プレイ開始直後から最大10体まで補充可能
	isReinforcementUnlocked_ = true;

	// 最初の1体はすぐに出現させる
	reinforcementSpawnTimer_ = kReinforcementSpawnInterval;

	/*
	 * ここでは敵を直接生成しない。
	 * ReplenishEnemies()が一定間隔で
	 * 1体ずつ最大10体まで生成する。
	 */
}

/*-------------------- フェーズの切り替え --------------------*/
void GameScene::ChangePhase() {
	switch (phase_) {
	case GameScene::Phase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = GameScene::Phase::kPlay;
		}
		break;

	case GameScene::Phase::kPlay:
		// 死亡を優先する
		if (player_->GetIsDead()) {
			gameResult_ = GameResult::kGameOver;

			phase_ = GameScene::Phase::kDeath;

			hasCreatedPlayerDeathEffect_ = false;

			// プレイヤー本体の回転・縮小を開始
			player_->StartDeathAnimation();

			// StartDeathAnimation()でプレイヤーが上へ移動した後の
			// 座標から光を散らす
			if (playerDeathEffect_) {
				playerDeathEffect_->Start(player_->GetWorldPos());

				hasCreatedPlayerDeathEffect_ = true;
			}

		} else if (playTimer_ >= kClearTime) {
			gameResult_ = GameResult::kClear;

			gameFinishTimer_ = 0.0f;
			phase_ = GameScene::Phase::kFinish;
		}

		break;

	case GameScene::Phase::kDeath: {
		bool isPlayerDeathEffectFinished = !playerDeathEffect_ || playerDeathEffect_->IsFinished();

		if (player_->IsDeathAnimationFinished() && hasCreatedPlayerDeathEffect_ && isPlayerDeathEffectFinished) {

			gameFinishTimer_ = 0.0f;
			phase_ = GameScene::Phase::kFinish;
		}

		break;
	}

	case GameScene::Phase::kFinish:
		if (gameFinishTimer_ >= kGameFinishDisplayTime) {
			phase_ = GameScene::Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 0.75f);
		}
		break;

	case GameScene::Phase::kFadeOut:
		if (fade_->IsFinished()) {
			// 終了フラグを立てる
			finished_ = true;
		}
		break;

	default:
		break;
	}
}

void GameScene::InitializeBackgroundEnemies() {
	backgroundEnemies_.clear();

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> xDistribution(-25.0f, 25.0f);
	std::uniform_real_distribution<float> yDistribution(-12.0f, 12.0f);
	std::uniform_real_distribution<float> zDistribution(8.0f, 18.0f);

	for (size_t i = 0; i < kBackgroundEnemyCount; ++i) {
		BackgroundEnemy* backgroundEnemy = new BackgroundEnemy();

		Vector3 position = {
		    camera_.translation_.x + xDistribution(randomEngine),
		    camera_.translation_.y + yDistribution(randomEngine),
		    zDistribution(randomEngine),
		};

		backgroundEnemy->Initialize(modelEnemy_, &camera_, position);

		backgroundEnemies_.push_back(backgroundEnemy);
	}
}

void GameScene::UpdateBackgroundEnemies() {
	for (BackgroundEnemy* backgroundEnemy : backgroundEnemies_) {
		if (backgroundEnemy) {
			backgroundEnemy->Update();
		}
	}
}

void GameScene::InitializeTutorialGuides() {
	uint32_t moveTexture = TextureManager::Load("tutorialMove.png");

	uint32_t normalAttackTexture = TextureManager::Load("tutorialNormalAttack.png");

	uint32_t chargedAttackTexture = TextureManager::Load("tutorialChargedAttack.png");

	uint32_t hitTargetTexture = TextureManager::Load("tutorialHitTarget.png");

	Vector2 guidePosition = {
	    kTutorialGuideX,
	    kTutorialGuideY,
	};

	Vector2 guideSize = {
	    kTutorialGuideWidth,
	    kTutorialGuideHeight,
	};

	/*========== 移動説明 ==========*/

	moveGuideSprite_ = Sprite::Create(moveTexture, guidePosition);

	moveGuideSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	moveGuideSprite_->SetSize(guideSize);

	/*========== 通常攻撃説明 ==========*/

	normalAttackGuideSprite_ = Sprite::Create(normalAttackTexture, guidePosition);

	normalAttackGuideSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	normalAttackGuideSprite_->SetSize(guideSize);

	/*========== 溜め攻撃説明 ==========*/

	chargedAttackGuideSprite_ = Sprite::Create(chargedAttackTexture, guidePosition);

	chargedAttackGuideSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	chargedAttackGuideSprite_->SetSize(guideSize);

	/*========== 敵同士をぶつける説明 ==========*/

	hitTargetGuideSprite_ = Sprite::Create(hitTargetTexture, guidePosition);

	hitTargetGuideSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	hitTargetGuideSprite_->SetSize(guideSize);
}

void GameScene::DrawTutorialGuide() {
	// 本プレイ開始後は表示しない
	if (isMainGameStarted_) {
		return;
	}

	// カウントダウンに入ったら表示しない
	if (isCountdownActive_) {
		return;
	}

	switch (tutorialState_) {
	case TutorialState::kMove:
		if (moveGuideSprite_) {
			moveGuideSprite_->Draw();
		}
		break;

	case TutorialState::kNormalAttack:
		if (normalAttackGuideSprite_) {
			normalAttackGuideSprite_->Draw();
		}
		break;

	case TutorialState::kChargedAttack:
		if (chargedAttackGuideSprite_) {
			chargedAttackGuideSprite_->Draw();
		}
		break;

	case TutorialState::kHitTarget:

		if (hitTargetGuideSprite_) {
			hitTargetGuideSprite_->Draw();
		}

		break;

	case TutorialState::kFinished:
		break;
	}
}

void GameScene::InitializeHpDisplay() {
	hpDigitSprites_.clear();

	for (size_t i = 0; i < kMaxHpDigits; ++i) {

		float positionX = kHpLeftX + static_cast<float>(i) * kHpDigitSpacing;

		Sprite* hpDigitSprite = Sprite::Create(
		    scoreDigitTextures_[0], {
		                                positionX,
		                                kHpTopY,
		                            });

		hpDigitSprite->SetAnchorPoint({
		    0.5f,
		    0.5f,
		});

		hpDigitSprite->SetSize({
		    kHpDigitWidth,
		    kHpDigitHeight,
		});

		hpDigitSprites_.push_back(hpDigitSprite);
	}

	displayedHp_ = -1;
	UpdateHpDisplay();
}

//-------------------- HP表示の更新 --------------------//
void GameScene::UpdateHpDisplay() {
	if (!player_) {
		return;
	}

	int32_t currentHp = (std::max)(player_->GetHp(), 0);

	if (currentHp == displayedHp_) {
		return;
	}

	displayedHp_ = currentHp;

	std::string hpText = std::to_string(currentHp);

	if (hpText.size() > kMaxHpDigits) {
		hpText = hpText.substr(hpText.size() - kMaxHpDigits);
	}

	hpDigitCount_ = hpText.size();

	for (size_t i = 0; i < hpDigitCount_; ++i) {

		uint32_t digit = static_cast<uint32_t>(hpText[i] - '0');

		hpDigitSprites_[i]->SetTextureHandle(scoreDigitTextures_[digit]);

		float positionX = kHpLeftX + static_cast<float>(i) * kHpDigitSpacing;

		hpDigitSprites_[i]->SetPosition({
		    positionX,
		    kHpTopY,
		});
	}
}

//-------------------- HP表示の描画 --------------------//
void GameScene::DrawHp() {
	for (size_t i = 0; i < hpDigitCount_; ++i) {

		if (i >= hpDigitSprites_.size()) {
			break;
		}

		if (hpDigitSprites_[i]) {
			hpDigitSprites_[i]->Draw();
		}
	}
}

//-------------------- スコア表示の初期化 --------------------//
void GameScene::InitializeScoreDisplay() {
	// カウントダウン用画像の1～3を流用
	scoreDigitTextures_[0] = TextureManager::Load("count0.png");

	scoreDigitTextures_[1] = countdownTexture1_;

	scoreDigitTextures_[2] = countdownTexture2_;

	scoreDigitTextures_[3] = countdownTexture3_;

	scoreDigitTextures_[4] = TextureManager::Load("count4.png");

	scoreDigitTextures_[5] = TextureManager::Load("count5.png");

	scoreDigitTextures_[6] = TextureManager::Load("count6.png");

	scoreDigitTextures_[7] = TextureManager::Load("count7.png");

	scoreDigitTextures_[8] = TextureManager::Load("count8.png");

	scoreDigitTextures_[9] = TextureManager::Load("count9.png");

	scoreDigitSprites_.clear();

	uint32_t scoreIconTexture = TextureManager::Load("Score_Bat.png");

	scoreIconSprite_ = Sprite::Create(
	    scoreIconTexture, {
	                          kHudLabelCenterX,
	                          kScoreTopY,
	                      });

	scoreIconSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	scoreIconSprite_->SetSize({
	    kScoreIconWidth,
	    kScoreIconHeight,
	});

	// スコア表示用の桁数分のスプライトを生成
	for (size_t i = 0; i < kMaxScoreDigits; ++i) {
		Sprite* digitSprite = Sprite::Create(scoreDigitTextures_[0], {kHudFirstDigitX, kScoreTopY});

		digitSprite->SetAnchorPoint({0.5f, 0.5f});

		digitSprite->SetSize({
		    kScoreDigitWidth,
		    kScoreDigitHeight,
		});

		scoreDigitSprites_.push_back(digitSprite);
	}

	displayedScore_ = UINT32_MAX;
	UpdateScoreDisplay();
}

void GameScene::UpdateScoreDisplay() {
	// スコアが変わっていなければ更新不要
	if (score_ == displayedScore_) {
		return;
	}

	displayedScore_ = score_;

	std::string scoreText = std::to_string(score_);

	// 5桁を超えた場合は末尾5桁を表示
	if (scoreText.size() > kScoreDisplayDigits) {
		scoreText = scoreText.substr(scoreText.size() - kScoreDisplayDigits);
	}

	// 5桁未満なら先頭を0で埋める
	if (scoreText.size() < kScoreDisplayDigits) {
		scoreText.insert(scoreText.begin(), kScoreDisplayDigits - scoreText.size(), '0');
	}

	scoreDigitCount_ = scoreText.size();

	for (size_t i = 0; i < scoreDigitCount_; ++i) {
		uint32_t digit = static_cast<uint32_t>(scoreText[i] - '0');

		scoreDigitSprites_[i]->SetTextureHandle(scoreDigitTextures_[digit]);

		/*
		 * 右端を固定して並べる。
		 *
		 * 例：
		 *   100 なら右から 0、0、1の順に位置を計算
		 */
		float positionX = kHudFirstDigitX + static_cast<float>(i) * kScoreDigitSpacing;

		scoreDigitSprites_[i]->SetPosition({
		    positionX,
		    kScoreTopY,
		});

		scoreDigitSprites_[i]->SetSize({
		    kScoreDigitWidth,
		    kScoreDigitHeight,
		});
	}

	// if (scoreIconSprite_ && scoreDigitCount_ > 0) {

	//	// 一番左の数字の中心位置
	//	float leftmostDigitX = kScoreRightX - static_cast<float>(scoreDigitCount_ - 1) * kScoreDigitSpacing;

	//	// 数字列のすぐ左へアイコンを置く
	//	float iconPositionX = leftmostDigitX - kScoreDigitWidth * 0.5f - kScoreIconMargin - kScoreIconWidth * 0.5f;

	//	scoreIconSprite_->SetPosition({
	//	    iconPositionX,
	//	    kScoreTopY,
	//	});
	//}
}

void GameScene::DrawScore() {
	if (scoreIconSprite_) {
		scoreIconSprite_->Draw();
	}

	for (size_t i = 0; i < scoreDigitCount_; ++i) {

		if (i >= scoreDigitSprites_.size()) {
			break;
		}

		if (scoreDigitSprites_[i]) {
			scoreDigitSprites_[i]->Draw();
		}
	}
}

/* -- -- -- -- -- -- -- -- -- --カウントダウンの初期化-- -- -- -- -- -- -- -- -- --*/
void GameScene::InitializeCountdown() {

	countdownTexture0_ = TextureManager::Load("count0.png");
	countdownTexture9_ = TextureManager::Load("count9.png");
	countdownTexture8_ = TextureManager::Load("count8.png");
	countdownTexture7_ = TextureManager::Load("count7.png");
	countdownTexture6_ = TextureManager::Load("count6.png");
	countdownTexture5_ = TextureManager::Load("count5.png");
	countdownTexture4_ = TextureManager::Load("count4.png");
	countdownTexture3_ = TextureManager::Load("count3.png");
	countdownTexture2_ = TextureManager::Load("count2.png");
	countdownTexture1_ = TextureManager::Load("count1.png");
	countdownTextureStart_ = TextureManager::Load("countStart.png");

	countdownSprite_ = Sprite::Create(countdownTexture3_, {640.0f, 360.0f});

	// スプライトの中心を画面中央へ合わせる
	countdownSprite_->SetAnchorPoint({0.5f, 0.5f});
	countdownSprite_->SetSize({256.0f, 256.0f});

	countdownState_ = CountdownState::kNone;
	isCountdownActive_ = false;
	countdownTimer_ = 0.0f;
}

void GameScene::StartCountdown() {
	if (isCountdownActive_ || isMainGameStarted_) {
		return;
	}

	isCountdownActive_ = true;
	countdownTimer_ = 0.0f;

	ChangeCountdownState(CountdownState::kThree);

	/*
	 * チュートリアル敵を画面から取り除く。
	 * 本プレイ用の敵はカウント終了後に補充される。
	 */
	enemies_.remove_if([this](Enemy* enemy) {
		if (!enemy->IsTutorialEnemy()) {
			return false;
		}

		delete enemy;
		return true;
	});

	tutorialLauncherEnemy_ = nullptr;
	tutorialTargetEnemy_ = nullptr;
}

void GameScene::UpdateCountdown() {
	if (!isCountdownActive_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;
	countdownTimer_ += deltaTime;

	float duration = kCountdownNumberTime;

	if (countdownState_ == CountdownState::kStart) {
		duration = kCountdownStartTime;
	}

	// 表示開始時は大きく、徐々に小さくする
	float t = std::clamp(countdownTimer_ / duration, 0.0f, 1.0f);

	float scale = kCountdownStartScale + (kCountdownEndScale - kCountdownStartScale) * t;

	countdownSprite_->SetSize({
	    256.0f * scale,
	    256.0f * scale,
	});

	if (countdownTimer_ < duration) {
		return;
	}

	switch (countdownState_) {
	case CountdownState::kThree:
		ChangeCountdownState(CountdownState::kTwo);
		break;

	case CountdownState::kTwo:
		ChangeCountdownState(CountdownState::kOne);
		break;

	case CountdownState::kOne:
		ChangeCountdownState(CountdownState::kStart);
		break;

	case CountdownState::kStart:
		countdownState_ = CountdownState::kNone;
		isCountdownActive_ = false;

		// この瞬間から制限時間と敵の出現を開始
		StartMainGame();
		break;

	case CountdownState::kNone:
		break;
	}
}

void GameScene::ChangeCountdownState(CountdownState state) {

	countdownState_ = state;
	countdownTimer_ = 0.0f;

	switch (countdownState_) {
	case CountdownState::kThree:
		countdownSprite_->SetTextureHandle(countdownTexture3_);
		break;

	case CountdownState::kTwo:
		countdownSprite_->SetTextureHandle(countdownTexture2_);
		break;

	case CountdownState::kOne:
		countdownSprite_->SetTextureHandle(countdownTexture1_);
		break;

	case CountdownState::kStart:
		countdownSprite_->SetTextureHandle(countdownTextureStart_);
		break;

	case CountdownState::kNone:
		break;
	}
}

void GameScene::InitializeTimeDisplay() {
	for (size_t i = 0; i < timeDigitSprites_.size(); ++i) {

		float positionX = kHudFirstDigitX + static_cast<float>(i) * kTimeDigitSpacing;

		timeDigitSprites_[i] = Sprite::Create(
		    scoreDigitTextures_[0], {
		                                positionX,
		                                kTimeTopY,
		                            });

		timeDigitSprites_[i]->SetAnchorPoint({
		    0.5f,
		    0.5f,
		});

		timeDigitSprites_[i]->SetSize({
		    kTimeDigitWidth,
		    kTimeDigitHeight,
		});
	}

	uint32_t timeIconTexture = TextureManager::Load("Time_Bat.png");

	timeIconSprite_ = Sprite::Create(
	    timeIconTexture, {
	                         kHudLabelCenterX,
	                         kTimeTopY,
	                     });

	timeIconSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	timeIconSprite_->SetSize({
	    kTimeIconWidth,
	    kTimeIconHeight,
	});

	displayedRemainingTime_ = -1;

	UpdateTimeDisplay();
}

void GameScene::UpdateTimeDisplay() {
	/*
	 * ceilを使うことで、
	 * 開始直後に60から59へ即座に変わることを防ぐ。
	 */
	int32_t remainingTime = static_cast<int32_t>(std::ceil(kClearTime - playTimer_));

	remainingTime = std::clamp(remainingTime, 0, 99);

	// 表示秒数が変化していなければ更新不要
	if (remainingTime == displayedRemainingTime_) {

		return;
	}

	displayedRemainingTime_ = remainingTime;

	// 十の位
	uint32_t tensDigit = static_cast<uint32_t>(remainingTime / 10);

	// 一の位
	uint32_t onesDigit = static_cast<uint32_t>(remainingTime % 10);

	timeDigitSprites_[0]->SetTextureHandle(scoreDigitTextures_[tensDigit]);

	timeDigitSprites_[1]->SetTextureHandle(scoreDigitTextures_[onesDigit]);
}

void GameScene::DrawRemainingTime() {
	if (timeIconSprite_) {
		timeIconSprite_->Draw();
	}

	for (Sprite* timeDigitSprite : timeDigitSprites_) {

		if (timeDigitSprite) {
			timeDigitSprite->Draw();
		}
	}
}
