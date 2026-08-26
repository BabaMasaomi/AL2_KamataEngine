#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
//#include "ShieldEnemy.h"
#include "Fade.h"
#include "HitEffect.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "Transform.h"
#include "temporaryAABB.h"
#include <vector>

class GameScene {
private:

	// ゲームのフェーズ(型)
	enum class Phase {
		kFadeIn,  // フェードイン
		kPlay,    // プレイ中
		kDeath,   // 死亡
		kFadeOut, // フェードアウト
	};

	// ゲームのフェーズ(変数)
	Phase phase_;

	// 終了フラグ
	bool finished_ = false;

	// マップチップフィールド
	MapChipField* mapChipField_ = nullptr;

	// Translateクラス内の関数を使える様にする
	Transform transform_;

	/*-------------------- プレイヤー --------------------*/
	// プレイヤーの3Dモデル
	KamataEngine::Model* model_ = nullptr;
	// バットの3Dモデル
	KamataEngine::Model* modelAttack_ = nullptr;

	// プレイヤーのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformPlayer_;

	// プレイヤー
	Player* player_ = nullptr;

	/*-------------------- 雑魚敵 --------------------*/
	// 敵の3Dモデル
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 敵のワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformEnemy_;

	// 敵のリスト
	std::list<Enemy*> enemies_ = {};

	/*-------------------- 天球 --------------------*/
	// 天球の3Dモデル
	KamataEngine::Model* modelSkydome_ = nullptr;

	// 天球のワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformSkydome_;

	// 天球
	Skydome* skydome_ = nullptr;

	/*-------------------- ブロック --------------------*/
	// ブロックの3Dモデル
	KamataEngine::Model* modelBlocks_ = nullptr;

	// ブロック用可変個配列
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	/*-------------------- パーティクル --------------------*/
	// パーティクルの3Dモデル
	KamataEngine::Model* modelParticles_ = nullptr;

	// 死亡パーティクル
	DeathParticles* deathParticles_ = nullptr;

	/*--------------- HitEffect ---------------*/
	// ヒットエフェクトの3Dモデル
	KamataEngine::Model* hitEffectModel_ = nullptr;
	// ガードエフェクトのモデル
	KamataEngine::Model* guardEffectModel_ = nullptr;

	// ヒットエフェクトのリスト
	std::list<HitEffect*> hitEffects_ = {};	

	/*-------------------- 追従カメラ --------------------*/
	// カメラ
	KamataEngine::Camera camera_;

	// カメラのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformCamera_;

	// カメラコントローラ
	CameraController* camaraController_ = nullptr;

	/*-------------------- デバッグ --------------------*/
	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	/*-------------------- フェード用 --------------------*/
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// フェード用
	Fade* fade_ = nullptr;

public:
	/*-------------------- コンストラクタ&デストラクタ --------------------*/
	GameScene();
	~GameScene();

	/*-------------------- メンバ関数 --------------------*/
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	// エフェクトの生成
	void CreateHitEffect(KamataEngine::Vector3 pos, HitEffectType type);

	// 表示ブロックの生成
	void GenerateBlocks();	

	// 総当たり当たり判定
	void CheckAllCollisions();
	// 吹き飛び中の敵と他の敵との判定
	void CheckEnemyCollisions();

	// AABB同士の当たり判定
	bool CheckAABBCollision(const AABB& aabb1, const AABB& aabb2);

	// 通常・スタン状態の敵同士の重なりを解消
	void ResolveEnemyOverlaps();

	// スタンした敵の周囲にいる通常敵を押し出す
	void PushEnemiesAroundStunned(Enemy* stunnedEnemy);

	// フェーズの切り替え
	void ChangePhase();

	/*-------------------- アクセッサ --------------------*/
	bool GetIsFinished() const { return finished_; }
};
