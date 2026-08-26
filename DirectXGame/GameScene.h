#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "GameResult.h"
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

	// チュートリアル進行状態
	enum class TutorialState {
		kMove,          // 移動説明
		kNormalAttack,  // 通常攻撃でスタンさせる
		kChargedAttack, // 溜め攻撃で吹き飛ばす
		kHitTarget,     // 別の敵へ命中させる
		kFinished,      // チュートリアル終了
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

	/*-------------------- 敵の出現管理 --------------------*/
	// 開始時の敵数
	static constexpr size_t kInitialEnemyCount = 3;

	// 補充解放後の同時出現上限
	static constexpr size_t kMaxEnemyCount = 10;

	// 最初の敵が倒され、補充が解放されたか
	bool isReinforcementUnlocked_ = false;

	//// 補充位置を順番に使うための番号
	//size_t reinforcementSpawnCursor_ = 0;

	// 次の敵を補充できるまでの時間
	float reinforcementSpawnTimer_ = 0.0f;

	// 敵を補充する間隔
	static constexpr float kReinforcementSpawnInterval = 0.25f;

	/*-------------------- 制限時間 --------------------*/
	// プレイ経過時間
	float playTimer_ = 0.0f;

	// クリアまでの時間
	static constexpr float kClearTime = 60.0f;

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

	/*--------------- ヒットストップ ---------------*/
	// ヒットストップ開始までの残り時間
	float hitStopDelayTimer_ = 0.0f;

	// エフェクトを進めてから停止するまでの時間
	static constexpr float kHitStopDelayTime = 0.05f;

	// ヒットストップの残り時間
	float hitStopTimer_ = 0.0f;

	// 実際に停止する時間
	static constexpr float kChargedAttackHitStopTime = 0.08f;

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

	/*-------------------- スコア --------------------*/
	// 現在のスコア
	uint32_t score_ = 0;

	// 敵1体を倒したときの得点
	static constexpr uint32_t kScorePerEnemy = 100;

	/*-------------------- チュートリアル --------------------*/
	TutorialState tutorialState_ = TutorialState::kMove;

	// チュートリアル敵を識別するためのポインタ
	Enemy* tutorialLauncherEnemy_ = nullptr;
	Enemy* tutorialTargetEnemy_ = nullptr;

	// 通常プレイが始まっているか
	bool isMainGameStarted_ = false;

	// 中央に到着したと判断するX座標
	static constexpr float kTutorialCenterX = 42.0f;

	/*-------------------- ゲームの終了判定 --------------------*/
	// ゲーム終了結果
	GameResult gameResult_ = GameResult::kNone;

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

	// 指定位置へ敵を1体生成
	void SpawnEnemy(const KamataEngine::Vector3& position);

	// 画面外の補充位置を探す
	KamataEngine::Vector3 FindReinforcementSpawnPosition();

	// 上限まで敵を補充
	void ReplenishEnemies();

	// エフェクトの生成
	void CreateHitEffect(KamataEngine::Vector3 pos, HitEffectType type);

	// ヒットエフェクトを更新
	void UpdateHitEffects();

	// 溜め攻撃命中時のヒットストップを開始
	void StartChargedAttackHitStop();

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

	// マップのブロック配置からカメラ移動範囲を計算
	CameraController::Rect CalculateCameraMovableArea();

	// チュートリアル
	void InitializeTutorial();
	void UpdateTutorial();
	void StartMainGame();

	// フェーズの切り替え
	void ChangePhase();

	/*-------------------- アクセッサ --------------------*/
	bool GetIsFinished() const { return finished_; }

	uint32_t GetScore() const { return score_; }

	GameResult GetGameResult() const { return gameResult_; }
};
