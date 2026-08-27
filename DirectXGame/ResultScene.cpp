#include "ResultScene.h"
#include <string>

using namespace KamataEngine;

ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	StopResultBgm();

	/*========== 背景 ==========*/
	delete backgroundSprite_;
	backgroundSprite_ = nullptr;

	/*========== Game Finish ==========*/
	delete gameFinishSprite_;
	gameFinishSprite_ = nullptr;

	/*========== スコアアイコン ==========*/
	delete scoreIconSprite_;
	scoreIconSprite_ = nullptr;

	/*========== スコア数字 ==========*/
	for (Sprite* digitSprite : scoreDigitSprites_) {

		delete digitSprite;
	}

	scoreDigitSprites_.clear();

	for (Sprite*& sprite : menuSprites_) {
		delete sprite;
		sprite = nullptr;
	}

	/*========== フェード ==========*/

	delete fade_;
	fade_ = nullptr;
}

void ResultScene::Initialize(GameResult result, uint32_t score) {

	result_ = result;
	score_ = score;

	finished_ = false;
	action_ = Action::kNone;
	selectedMenuIndex_ = 0;
	phase_ = Phase::kFadeIn;

	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	Vector4 backgroundColor;

	if (result_ == GameResult::kClear) {
		// クリア画面
		backgroundColor = {0.10f, 0.25f, 0.55f, 1.0f};

	} else {
		// ゲームオーバー画面
		backgroundColor = {0.45f, 0.08f, 0.08f, 1.0f};
	}

	backgroundSprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f}, backgroundColor);

	backgroundSprite_->SetSize({1280.0f, 720.0f});

	/*========== Game Finish画像 ==========*/
	uint32_t gameFinishTexture = TextureManager::Load("ResultBig_bat.png");

	gameFinishSprite_ = Sprite::Create(
	    gameFinishTexture, {
	                           60.0f,
	                           60.0f,
	                       });

	// 左上を基準に配置
	gameFinishSprite_->SetAnchorPoint({
	    0.0f,
	    0.0f,
	});

	gameFinishSprite_->SetSize({
	    600.0f,
	    200.0f,
	});

	/*========== スコア表示 ==========*/
	InitializeScoreDisplay();
	InitializeMenu();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 0.75f);

	/*--------------- UI音声 ---------------*/
	audio_ = Audio::GetInstance();

	cursorMovementSoundHandle_ = audio_->LoadWave("cursorMovement.mp3");

	selectSoundHandle_ = audio_->LoadWave("select.mp3");

	// リザルトBGM再生
	resultBgmSoundHandle_ = audio_->LoadWave("ResultBGM.mp3");

	resultBgmVoiceHandle_ = audio_->PlayWave(resultBgmSoundHandle_, false, kResultBgmVolume);

	isResultBgmPlaying_ = true;
}

void ResultScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		if (Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			selectedMenuIndex_ = 1 - selectedMenuIndex_;
			UpdateMenuAppearance();

			if (audio_) {
				audio_->PlayWave(cursorMovementSoundHandle_, false, 0.35f);
			}
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_RETURN)) {
			if (audio_) {
				audio_->PlayWave(selectSoundHandle_, false, 0.55f);
			}

			action_ = (selectedMenuIndex_ == 0) ? Action::kReturnToTitle : Action::kRetry;
			fade_->Start(Fade::Status::FadeOut, 0.75f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	fade_->Update();
}

// 描画
void ResultScene::Draw() {
	Sprite::PreDraw();

	/*========== 背景 ==========*/

	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}

	/*========== Game Finish ==========*/

	if (gameFinishSprite_) {
		gameFinishSprite_->Draw();
	}

	/*========== 最終スコア ==========*/

	DrawScore();
	DrawMenu();

	Sprite::PostDraw();

	// UIより手前にフェードを描画
	fade_->Draw();
}

void ResultScene::InitializeScoreDisplay() {
	/*========== 数字テクスチャの読み込み ==========*/

	scoreDigitTextures_[0] = TextureManager::Load("count0.png");

	scoreDigitTextures_[1] = TextureManager::Load("count1.png");

	scoreDigitTextures_[2] = TextureManager::Load("count2.png");

	scoreDigitTextures_[3] = TextureManager::Load("count3.png");

	scoreDigitTextures_[4] = TextureManager::Load("count4.png");

	scoreDigitTextures_[5] = TextureManager::Load("count5.png");

	scoreDigitTextures_[6] = TextureManager::Load("count6.png");

	scoreDigitTextures_[7] = TextureManager::Load("count7.png");

	scoreDigitTextures_[8] = TextureManager::Load("count8.png");

	scoreDigitTextures_[9] = TextureManager::Load("count9.png");

	/*========== 数字スプライトの生成 ==========*/

	scoreDigitSprites_.clear();

	for (size_t i = 0; i < kMaxScoreDigits; ++i) {

		Sprite* digitSprite = Sprite::Create(
		    scoreDigitTextures_[0], {
		                                kResultScoreRightX,
		                                kResultScoreY,
		                            });

		digitSprite->SetAnchorPoint({
		    0.5f,
		    0.5f,
		});

		digitSprite->SetSize({
		    kResultDigitWidth,
		    kResultDigitHeight,
		});

		scoreDigitSprites_.push_back(digitSprite);
	}

	/*========== スコアアイコン ==========*/
	uint32_t scoreIconTexture = TextureManager::Load("Score_Bat.png");

	scoreIconSprite_ = Sprite::Create(
	    scoreIconTexture, {
	                          700.0f,
	                          kResultScoreY,
	                      });

	scoreIconSprite_->SetAnchorPoint({
	    0.5f,
	    0.5f,
	});

	scoreIconSprite_->SetSize({
	    kResultScoreIconWidth,
	    kResultScoreIconHeight,
	});

	// スコアに合わせて数字とアイコンを配置
	UpdateScoreDisplay();
}

void ResultScene::UpdateScoreDisplay() {
	std::string scoreText = std::to_string(score_);

	// 最大桁数を超えた場合は末尾だけ使用
	if (scoreText.size() > kMaxScoreDigits) {

		scoreText = scoreText.substr(scoreText.size() - kMaxScoreDigits);
	}

	scoreDigitCount_ = scoreText.size();

	/*========== 数字を設定 ==========*/

	for (size_t i = 0; i < scoreDigitCount_; ++i) {

		uint32_t digit = static_cast<uint32_t>(scoreText[i] - '0');

		scoreDigitSprites_[i]->SetTextureHandle(scoreDigitTextures_[digit]);

		/*
		 * 一番右の数字を固定し、
		 * 桁数が増えたら左へ伸ばす。
		 */
		float positionX = kResultScoreRightX - static_cast<float>(scoreDigitCount_ - 1 - i) * kResultDigitSpacing;

		scoreDigitSprites_[i]->SetPosition({
		    positionX,
		    kResultScoreY,
		});

		scoreDigitSprites_[i]->SetSize({
		    kResultDigitWidth,
		    kResultDigitHeight,
		});
	}

	/*========== アイコンを数字の左へ配置 ==========*/

	if (scoreIconSprite_ && scoreDigitCount_ > 0) {

		float leftmostDigitX = kResultScoreRightX - static_cast<float>(scoreDigitCount_ - 1) * kResultDigitSpacing;

		float iconX = leftmostDigitX - kResultDigitWidth * 0.5f - kResultScoreIconMargin - kResultScoreIconWidth * 0.5f;

		scoreIconSprite_->SetPosition({
		    iconX,
		    kResultScoreY,
		});
	}
}

void ResultScene::DrawScore() {
	// スコアアイコン
	if (scoreIconSprite_) {
		scoreIconSprite_->Draw();
	}

	// スコアの数字
	for (size_t i = 0; i < scoreDigitCount_; ++i) {

		if (i >= scoreDigitSprites_.size()) {

			break;
		}

		if (scoreDigitSprites_[i]) {
			scoreDigitSprites_[i]->Draw();
		}
	}
}

void ResultScene::InitializeMenu() {
	const std::array<const char*, 2> textureNames = {
	    "go_to_title.png",
	    "retry.png",
	};

	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		uint32_t texture = TextureManager::Load(textureNames[i]);
		menuSprites_[i] = Sprite::Create(texture, {kMenuX, kMenuStartY + static_cast<float>(i) * kMenuSpacingY});
		menuSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		menuSprites_[i]->SetSize({kMenuWidth, kMenuHeight});
	}

	UpdateMenuAppearance();
}

// メニューの選択状態に応じて透明度を更新
void ResultScene::UpdateMenuAppearance() {
	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		if (!menuSprites_[i]) {
			continue;
		}
		float alpha = (i == selectedMenuIndex_) ? kSelectedAlpha : kUnselectedAlpha;
		menuSprites_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
}

// メニュー描画
void ResultScene::DrawMenu() {
	for (Sprite* sprite : menuSprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
}

// リザルトBGMを停止する
void ResultScene::StopResultBgm() {
	if (!audio_ || !isResultBgmPlaying_) {
		return;
	}

	audio_->StopWave(resultBgmVoiceHandle_);

	resultBgmVoiceHandle_ = 0;
	isResultBgmPlaying_ = false;
}