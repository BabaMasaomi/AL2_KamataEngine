#pragma once
#include "CameraController.h"
#include "KamataEngine.h"
#include "Player.h"
#include "Fade.h"
#include "Transform.h"
#include <vector>

class TitleScene {
public:
	/*-------------------- コンストラクタ&デストラクタ --------------------*/
	TitleScene();
	~TitleScene();

	/*-------------------- メンバ関数 --------------------*/
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	/*-------------------- アクセッサ --------------------*/
	bool GetIsFinished() const { return finished_; }

private:
	// シーンのフェーズ
	enum class Phase {
		kFadeIn,	// フェードイン
		kMain,		// メイン部
		kFadeOut,	// フェードアウト
	};

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// 終了フラグ
	bool finished_ = false;

	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// フェード用
	Fade* fade_ = nullptr;

	/*-------------------- タイトルフォント --------------------*/
	// タイトルフォントの3Dモデル
	KamataEngine::Model* modelTitle_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransformTitle_;

	/*-------------------- プレイヤー --------------------*/
	// プレイヤーの3Dモデル
	KamataEngine::Model* modelPlayer_ = nullptr;

	// プレイヤーのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformPlayer_;

	// プレイヤー
	Player* player_ = nullptr;

	/*-------------------- カメラ --------------------*/
	// カメラ
	KamataEngine::Camera camera_;

	// カメラのワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformCamera_;

	/*-------------------- デバッグ --------------------*/
	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
