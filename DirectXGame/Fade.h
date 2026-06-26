#pragma once
#include "KamataEngine.h"
#include "Transform.h"

class Fade {
public:
	/*-------------------- コンストラクタ&デストラクタ --------------------*/
	Fade();
	~Fade();

	// フェードの状態
	enum class Status {
		None,
		FadeIn,
		FadeOut,
	};

	/*-------------------- メンバ関数 --------------------*/
	void Initialize();
	void Update();
	void Draw();

	// フェード開始
	void Start(Status status, float duration);
	// フェード停止
	void Stop();
	// フェード終了判定
	bool IsFinished() const;

private:
	// Spriteの生成
	KamataEngine::Sprite* sprite_ = nullptr;

	// 現在のフェードの状態
	Status status_ = Status::None;	

	// フェードの時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;
};
