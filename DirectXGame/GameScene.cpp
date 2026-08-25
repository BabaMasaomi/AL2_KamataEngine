#include "GameScene.h"

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
	delete fade_;             // フェードの解放
}

/*==============================================================
* メンバ関数
==============================================================*/
/*-------------------- 初期化 --------------------*/
void GameScene::Initialize() {
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
	model_ = Model::CreateFromOBJ("player", true);
	// 攻撃エフェクトの3Dモデルの生成
	modelAttack_ = Model::CreateFromOBJ("hit_effect", true);

	// プレイヤーのワールドトランスフォームの初期化
	worldTransformPlayer_.Initialize();

	// プレイヤーの生成
	player_ = new Player();

	// 座標をマップチップ番号で指定
	Vector3 playerPos = mapChipField_->GetMapChipPositionByIndex(13, 17);

	// プレイヤーの初期化
	player_->Initialize(model_, modelAttack_, &camera_, playerPos);

	// マップチップデータのセット
	player_->SetMapChipField(mapChipField_);

	/*--------------- 雑魚敵 ---------------*/
	// 敵の3Dモデルの生成
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);

	// 敵のワールドトランスフォームの初期化
	worldTransformEnemy_.Initialize();

	for (int32_t i = 0; i < 3; i++) {
		// 敵の生成
		Enemy* newEnemy = new Enemy();

		// 
		newEnemy->SetMapChipField(mapChipField_);

		// 座標をマップチップ番号で指定
		Vector3 enemyPos = mapChipField_->GetMapChipPositionByIndex(40 + i * 7, 16);

		// 敵の初期化
		newEnemy->Initialize(modelEnemy_, &camera_, enemyPos);

		// 追跡対象としてプレイヤーを設定
		newEnemy->SetTarget(player_);

		// 敵にゲームシーンを渡す
		newEnemy->SetGameScene(this);

		// リストに追加
		enemies_.push_back(newEnemy);
	}

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
	modelBlocks_ = Model::CreateFromOBJ("block", true);

	/*--------------- パーティクル ---------------*/
	// パーティクルの3Dモデルの生成
	modelParticles_ = Model::CreateFromOBJ("deathParticle", true);

	// パーティクルのワールドトランスフォームの初期化
	worldTransformPlayer_.Initialize();

	/*--------------- ヒットエフェクト ---------------*/
	// モデルの読み込み
	hitEffectModel_ = Model::CreateFromOBJ("particle", true);
	guardEffectModel_ = Model::CreateFromOBJ("ring", true);

	HitEffect::SetHitModel(hitEffectModel_);
	HitEffect::SetGuardModel(guardEffectModel_);
	HitEffect::SetCamera(&camera_);

	/*--------------- カメラ ---------------*/
	// カメラコントローラの生成
	camaraController_ = new CameraController();

	// カメラコントローラの初期化
	camaraController_->Initialize(&camera_);

	// 追従対象をセット
	camaraController_->SetTarget(player_);

	// 移動範囲を指定
	camaraController_->SetMovableArea(CameraController::Rect{20.0f, 180.0f, 10.0f, 200.0f});

	// リセット(瞬間合わせ)
	camaraController_->Reset();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	/*--------------- フェード ---------------*/
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

/*-------------------- 更新 --------------------*/
void GameScene::Update() {
	// フェーズごとの更新処理
	switch (phase_) {
	case ::GameScene::Phase::kFadeIn:
		// 天球の更新
		skydome_->Update();

		// フェード処理処理宙にプレイヤーを正しい位置に描画させる
		player_->Update();

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

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

	case GameScene::Phase::kPlay:
		// インゲームの更新処理
		// 天球の更新
		skydome_->Update();

		// プレイヤーの更新
		player_->Update();

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		// デスフラグの立った敵を削除
		enemies_.remove_if([](Enemy* enemy) {
			if (enemy->GetIsDead()) {
				delete enemy;
				return true;
			}
			return false;
		});

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

	case GameScene::Phase::kDeath:
		// デス演出の更新処理
		// 天球の更新
		skydome_->Update();

		// 敵の更新
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

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
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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

	// フェードを更新
	fade_->Draw();

	Model::PostDraw();
}

/*-------------------- エフェクトの生成 --------------------*/
void GameScene::CreateHitEffect(Vector3 pos, HitEffectType type) {
	HitEffect* newHitEffect = HitEffect::Create(pos, type);
	hitEffects_.push_back(newHitEffect);
}

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

		AABB attackerAABB = attacker->GetAABB();

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

/*-------------------- フェーズの切り替え --------------------*/
void GameScene::ChangePhase() {
	switch (phase_) {
	case GameScene::Phase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = GameScene::Phase::kPlay;
		}
		break;

	case GameScene::Phase::kPlay:
		// ゲームプレイフェーズの処理
		if (player_->GetIsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = GameScene::Phase::kDeath;

			// 自キャラの座標を取得
			const Vector3 deathParticlesPosition = player_->GetWorldPos();

			// 自機の座標を飛ばす	(強硬策だと思うので後で修正)
			player_->SetWorldPos({-100.0f, -100.0f, -100.0f});
			player_->Update();

			// 自キャラの座標にパーティクルをセット
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(modelParticles_, &camera_, deathParticlesPosition);
		}

		break;

	case GameScene::Phase::kDeath:
		// デス演出フェーズの処理
		if (deathParticles_ && deathParticles_->GetIsFinished()) {
			phase_ = GameScene::Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 0.5f);
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