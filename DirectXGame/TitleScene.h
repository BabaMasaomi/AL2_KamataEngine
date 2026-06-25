#pragma once
#include "CameraController.h"
#include "KamataEngine.h"
#include "Player.h"
#include "Transform.h"
#include <vector>

class TitleScene {
private:
	// 終了フラグ
	bool finished_ = false;

	// Translateクラス内の関数を使える様にする
	Transform transform_;

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
};
