#pragma once

#include "Fade.h"
#include "GameResult.h"
#include "KamataEngine.h"

class ResultScene {
public:
	ResultScene();
	~ResultScene();

	void Initialize(GameResult result);
	void Update();
	void Draw();

	bool GetIsFinished() const { return finished_; }

private:
	enum class Phase {
		kFadeIn,
		kMain,
		kFadeOut,
	};

	Phase phase_ = Phase::kFadeIn;

	GameResult result_ = GameResult::kNone;

	bool finished_ = false;

	KamataEngine::Sprite* backgroundSprite_ = nullptr;

	Fade* fade_ = nullptr;
};