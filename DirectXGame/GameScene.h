#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "GameResult.h"
#include "HitEffect.h"
#include "ChargeEffect.h"
#include "PlayerDeathEffect.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "BackgroundEnemy.h"
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
		kFinish,  // GameFinish表示
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
	static constexpr size_t kMaxEnemyCount = 18;

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

	/*-------------------- 背景用の敵 --------------------*/
	// 背景敵のリスト
	std::list<BackgroundEnemy*> backgroundEnemies_ = {};

	// 背景敵の表示数
	static constexpr size_t kBackgroundEnemyCount = 12;

	// 背景敵を生成
	void InitializeBackgroundEnemies();

	// 背景敵を更新
	void UpdateBackgroundEnemies();

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

	// チャージ収束エフェクト
	ChargeEffect* chargeEffect_ = nullptr;

	// プレイヤー被ダメージ用赤リング
	KamataEngine::Model* playerDamageEffectModel_ = nullptr;

	/*-------------------- 音声 --------------------*/
	KamataEngine::Audio* audio_ = nullptr;

	// プレイヤー攻撃
	uint32_t normalAttackHitSoundHandle_ = 0;
	uint32_t chargedAttackHitSoundHandle_ = 0;

	// プレイヤー被ダメージ
	uint32_t receiveDamageSoundHandle_ = 0;

	// カウントダウン
	uint32_t countDownSoundHandle_ = 0;
	uint32_t countStartSoundHandle_ = 0;

	// ゲーム終了
	uint32_t gameFinishSoundHandle_ = 0;

	// 敵関連
	uint32_t enemyBurstSoundHandle_ = 0;
	uint32_t enemyCollisionSoundHandle_ = 0;

	// 本編BGM
	uint32_t playBgmSoundHandle_ = 0;
	uint32_t playBgmVoiceHandle_ = 0;
	bool isPlayBgmPlaying_ = false;

	// 音量
	static constexpr float kPlayBgmTutorialVolume = 0.20f;
	static constexpr float kPlayBgmMainVolume = 0.48f;

	// チュートリアル案内音
	uint32_t tutorialGuideSoundHandle_ = 0;

	// 前回音を鳴らした案内状態
	TutorialState lastTutorialGuideSoundState_ = TutorialState::kFinished;

	void StopPlayBgm();
	void UpdateTutorialGuideSound();

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

	/*-------------------- プレイヤー死亡演出 --------------------*/
	// 専用の光エフェクト
	PlayerDeathEffect* playerDeathEffect_ = nullptr;

	// 二重開始防止
	bool hasCreatedPlayerDeathEffect_ = false;

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

	/*-------------------- チュートリアル操作ガイド --------------------*/
	// 移動説明
	KamataEngine::Sprite* moveGuideSprite_ = nullptr;

	// 通常攻撃説明
	KamataEngine::Sprite* normalAttackGuideSprite_ = nullptr;

	// 溜め攻撃説明
	KamataEngine::Sprite* chargedAttackGuideSprite_ = nullptr;

	// 敵同士をぶつける説明
	KamataEngine::Sprite* hitTargetGuideSprite_ = nullptr;

	// 操作ガイドの表示位置
	static constexpr float kTutorialGuideX = 640.0f;
	static constexpr float kTutorialGuideY = 100.0f;

	// 操作ガイドの表示サイズ
	static constexpr float kTutorialGuideWidth = 720.0f;
	static constexpr float kTutorialGuideHeight = 200.0f;

	// 初期化
	void InitializeTutorialGuides();

	// 描画
	void DrawTutorialGuide();

	/*-------------------- HP表示 --------------------*/
	// HPアイコンのスプライト
	std::vector<KamataEngine::Sprite*> hpIconSprites_;

	// HPアイコンの大きさ
	static constexpr float kHpIconWidth = 72.0f;
	static constexpr float kHpIconHeight = 60.0f;

	// アイコン同士の間隔
	static constexpr float kHpIconSpacing = 66.0f;

	// 左上の先頭位置
	static constexpr float kHpLeftX = 55.0f;
	static constexpr float kHpTopY = 55.0f;

	// 残りHPと失ったHPの透明度
	static constexpr float kHpActiveAlpha = 1.0f;
	static constexpr float kHpLostAlpha = 0.18f;

	void InitializeHpDisplay();
	void UpdateHpDisplay();
	void DrawHp();

	/*-------------------- スコア表示 --------------------*/
	// 0～9の数字テクスチャ
	std::array<uint32_t, 10> scoreDigitTextures_{};

	// 桁ごとのスプライト
	std::vector<KamataEngine::Sprite*> scoreDigitSprites_;

	// 現在表示している桁数
	size_t scoreDigitCount_ = 1;

	// 前回表示したスコア
	uint32_t displayedScore_ = UINT32_MAX;

	// 表示できる最大桁数
	static constexpr size_t kMaxScoreDigits = 8;

	// ゲーム中に表示する桁数
	static constexpr size_t kScoreDisplayDigits = 5;

	// 数字の表示サイズ
	static constexpr float kScoreDigitWidth = 48.0f;
	static constexpr float kScoreDigitHeight = 48.0f;

	// 数字同士の間隔
	static constexpr float kScoreDigitSpacing = 44.0f;	

	// 数字の中心Y座標
	static constexpr float kScoreTopY = 110.0f;

	// スコアアイコン
	KamataEngine::Sprite* scoreIconSprite_ = nullptr;

	// スコアアイコンの大きさ
	static constexpr float kScoreIconWidth = 150.0f;
	static constexpr float kScoreIconHeight = 50.0f;

	// 数字との間隔
	static constexpr float kScoreIconMargin = 12.0f;


	// 初期化
	void InitializeScoreDisplay();

	// 表示する数字と座標を更新
	void UpdateScoreDisplay();

	// 描画
	void DrawScore();

	// Time・Scoreラベルの共通中心X座標
	static constexpr float kHudLabelCenterX = 920.0f;

	// ラベル右側にある最初の数字の中心X座標
	static constexpr float kHudFirstDigitX = 1031.0f;

	/*-------------------- 開始カウントダウン --------------------*/
	enum class CountdownState {
		kNone,
		kThree,
		kTwo,
		kOne,
		kStart,
	};

	// 現在のカウント表示
	CountdownState countdownState_ = CountdownState::kNone;

	// カウントダウン中か
	bool isCountdownActive_ = false;

	// 現在の数字を表示している時間
	float countdownTimer_ = 0.0f;

	// 数字1つあたりの表示時間
	static constexpr float kCountdownNumberTime = 0.8f;

	// STARTの表示時間
	static constexpr float kCountdownStartTime = 0.6f;

	// カウント表示用スプライト
	KamataEngine::Sprite* countdownSprite_ = nullptr;

	// テクスチャ
	uint32_t countdownTexture0_ = 0;
	uint32_t countdownTexture9_ = 0;
	uint32_t countdownTexture8_ = 0;
	uint32_t countdownTexture7_ = 0;
	uint32_t countdownTexture6_ = 0;
	uint32_t countdownTexture5_ = 0;
	uint32_t countdownTexture4_ = 0;
	uint32_t countdownTexture3_ = 0;
	uint32_t countdownTexture2_ = 0;
	uint32_t countdownTexture1_ = 0;
	uint32_t countdownTextureStart_ = 0;

	// カウント開始時の初期サイズ
	static constexpr float kCountdownStartScale = 1.35f;

	// カウント終了時のサイズ
	static constexpr float kCountdownEndScale = 0.85f;

	// 残り時間アイコン
	KamataEngine::Sprite* timeIconSprite_ = nullptr;

	// 残り時間アイコンの大きさ
	static constexpr float kTimeIconWidth = 150.0f;
	static constexpr float kTimeIconHeight = 50.0f;

	// 数字との間隔
	static constexpr float kTimeIconMargin = 12.0f;

	void InitializeCountdown();
	void StartCountdown();
	void UpdateCountdown();
	void ChangeCountdownState(CountdownState state);

	/*-------------------- 残り時間表示 --------------------*/
	// 残り時間を表示する2桁のスプライト
	std::array<KamataEngine::Sprite*, 2> timeDigitSprites_{};

	// 前回表示した残り秒数
	int32_t displayedRemainingTime_ = -1;

	// 数字の表示サイズ
	static constexpr float kTimeDigitWidth = 48.0f;
	static constexpr float kTimeDigitHeight = 48.0f;

	// 数字同士の間隔
	static constexpr float kTimeDigitSpacing = 44.0f;

	// 時間表示のY座標
	static constexpr float kTimeTopY = 45.0f;

	// 初期化
	void InitializeTimeDisplay();

	// 数字と位置を更新
	void UpdateTimeDisplay();

	// 描画
	void DrawRemainingTime();

	/*-------------------- ゲームの終了判定 --------------------*/
	// ゲーム終了結果
	GameResult gameResult_ = GameResult::kNone;

	/*-------------------- ゲーム終了演出 --------------------*/
	KamataEngine::Sprite* gameFinishSprite_ = nullptr;

	float gameFinishTimer_ = 0.0f;

	static constexpr float kGameFinishDisplayTime = 1.5f;
	static constexpr float kGameFinishWidth = 1000.0f;
	static constexpr float kGameFinishHeight = 400.0f;

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
	void Initialize(bool skipTutorial = false);

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

	// 敵の破裂音を再生
	void PlayEnemyBurstSound();

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
