#include "TitleScene.h"
#include <numbers>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
TitleScene::TitleScene() {}
TitleScene::~TitleScene() {
	delete modelTitle_;
	modelTitle_ = nullptr;

	delete modelPlayer_;
	modelPlayer_ = nullptr;

	delete modelBat_;
	modelBat_ = nullptr;

	delete skydome_;
	skydome_ = nullptr;

	delete modelSkydome_;
	modelSkydome_ = nullptr;
	;

	for (Sprite*& sprite : menuSprites_) {
		delete sprite;
		sprite = nullptr;
	}

	delete creditBackdropSprite_;
	creditBackdropSprite_ = nullptr;

	delete creditPlaceholderSprite_;
	creditPlaceholderSprite_ = nullptr;

	delete fade_;
	fade_ = nullptr;
}

/*-------------------- 初期化 --------------------*/
void TitleScene::Initialize() {
	finished_ = false;
	action_ = Action::kNone;
	selectedMenuIndex_ = 0;
	isCreditVisible_ = false;
	phase_ = Phase::kFadeIn;

	// カメラ初期化
	camera_.farZ = 550.0f;
	camera_.Initialize();

	/*========== 天球 ==========*/
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	skydome_ = new Skydome();

	skydome_->Initialize(modelSkydome_, &camera_);
	skydome_->SetRotationSpeed(kSkydomeRotationSpeed);

	// Initialize直後から正しい行列を持たせる
	skydome_->Update();

	// タイトル用フォントのモデル
	modelTitle_ = Model::CreateFromOBJ("Ttile_Bat", true); // タイトルフォントの3Dモデルを生成
	worldTransformTitle_.Initialize();                     // タイトルのワールドトランスフォームの初期化
	worldTransformTitle_.translation_ = {-3.0f, 7.0f, 0.0f};
	worldTransformTitle_.scale_ = {2.0f, 2.0f, 2.0f};

	/*========== プレイヤー ==========*/
	modelPlayer_ = Model::CreateFromOBJ("CapPlayer", true);

	worldTransformPlayer_.Initialize();

	worldTransformPlayer_.translation_ = {-9.0f, -7.0f, 0.0f};
	worldTransformPlayer_.scale_ = {
	    7.0f,
	    7.0f,
	    7.0f,
	};
	// 最初は正面寄り
	worldTransformPlayer_.rotation_ = {0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f};

	/*========== バット ==========*/
	modelBat_ = Model::CreateFromOBJ("Bat", true);

	worldTransformBat_.Initialize();

	worldTransformBat_.scale_ = {
	    5.25f,
	    5.25f,
	    5.25f,
	};

	// バット先端を上げて構える
	worldTransformBat_.rotation_.x = -115.0f * std::numbers::pi_v<float> / 180.0f;
	worldTransformBat_.rotation_.z = -105.0f * std::numbers::pi_v<float> / 180.0f;

	isPlayStartAnimation_ = false;
	playerRotationSpeed_ = kPlayerIdleRotationSpeed;

	/*--------------- UI音声 ---------------*/
	audio_ = Audio::GetInstance();

	cursorMovementSoundHandle_ = audio_->LoadWave("cursorMovement.mp3");

	selectSoundHandle_ = audio_->LoadWave("select.mp3");

	// メニューUIの初期化
	InitializeMenu();

	// フェード用
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 2.0f);
}

/*-------------------- 更新 --------------------*/
void TitleScene::Update() {
	switch (phase_) {
	case TitleScene::Phase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case TitleScene::Phase::kMain:
		UpdateMenu();
		break;

	case TitleScene::Phase::kFadeOut:
		// フェードアウトが終わったらゲームシーンに移行
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;

	default:
		break;
	}

	// タイトルを揺らす
	static float t = 0.0f;
	t += 0.03f;
	worldTransformTitle_.translation_.y = 5.0f + sinf(t) * 0.5f;

	/*========== プレイヤーの回転 ==========*/
	float targetRotationSpeed = isPlayStartAnimation_ ? kPlayerStartRotationSpeed : kPlayerIdleRotationSpeed;

	// 急に速度が切り替わらないよう補間
	playerRotationSpeed_ += (targetRotationSpeed - playerRotationSpeed_) * kPlayerRotationLerpRate;

	worldTransformPlayer_.rotation_.y += playerRotationSpeed_;

	/*========== バットの手元位置を追従 ==========*/
	float rotationY = worldTransformPlayer_.rotation_.y;

	// プレイヤーを基準にした手元のローカル位置
	float localX = kBatHandOffsetX;
	float localZ = kBatHandOffsetZ;

	// プレイヤーのY回転に合わせて手元位置を回す
	float rotatedX = localX * std::cos(rotationY) + localZ * std::sin(rotationY);

	float rotatedZ = -localX * std::sin(rotationY) + localZ * std::cos(rotationY);

	worldTransformBat_.translation_ = {
	    worldTransformPlayer_.translation_.x + rotatedX,
	    worldTransformPlayer_.translation_.y + kBatHandOffsetY,
	    worldTransformPlayer_.translation_.z + rotatedZ,
	};

	// プレイヤーと同じ向きへ回転
	worldTransformBat_.rotation_.y = rotationY;

	/*========== 天球の回転 ==========*/
	if (skydome_) {
		skydome_->Update();
	}

	// 更新を反映
	transform_.worldMatrixUpdate(worldTransformTitle_);
	transform_.worldMatrixUpdate(worldTransformPlayer_);
	transform_.worldMatrixUpdate(worldTransformBat_);

	camera_.UpdateMatrix();

	// フェードを更新
	fade_->Update();
}

/*-------------------- 描画 --------------------*/
void TitleScene::Draw() {
	Model::PreDraw();

	// 最も奥
	if (skydome_) {
		skydome_->Draw();
	}

	// プレイヤー
	modelPlayer_->Draw(worldTransformPlayer_, camera_);

	// プレイヤーが持っているバット
	modelBat_->Draw(worldTransformBat_, camera_);

	// タイトル文字
	modelTitle_->Draw(worldTransformTitle_, camera_);

	Model::PostDraw();

	Sprite::PreDraw();
	DrawMenu();
	Sprite::PostDraw();

	fade_->Draw();
}

void TitleScene::InitializeMenu() {
	const std::array<const char*, 3> textureNames = {
	    "PlayGameUI_bat.png",
	    "ShowCreditUI_bat.png",
	    "QuitGameUI_bat.png",
	};

	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		uint32_t texture = TextureManager::Load(textureNames[i]);
		menuSprites_[i] = Sprite::Create(texture, {kMenuCenterX, kMenuStartY + static_cast<float>(i) * kMenuSpacingY});
		menuSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		menuSprites_[i]->SetSize({kMenuSize, kMenuSize});
	}

	uint32_t whiteTexture = TextureManager::Load("white1x1.png");
	creditBackdropSprite_ = Sprite::Create(whiteTexture, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.85f});
	creditBackdropSprite_->SetSize({1280.0f, 720.0f});

	uint32_t creditTexture = TextureManager::Load("ShowCreditUI_bat.png");
	creditPlaceholderSprite_ = Sprite::Create(creditTexture, {640.0f, 330.0f});
	creditPlaceholderSprite_->SetAnchorPoint({0.5f, 0.5f});
	creditPlaceholderSprite_->SetSize({220.0f, 220.0f});

	UpdateMenuAppearance();
}

void TitleScene::UpdateMenuAppearance() {
	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		if (!menuSprites_[i]) {
			continue;
		}
		float alpha = (i == selectedMenuIndex_) ? kSelectedAlpha : kUnselectedAlpha;
		menuSprites_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
}

void TitleScene::UpdateMenu() {
	Input* input = Input::GetInstance();

	if (isCreditVisible_) {
		if (input->TriggerKey(DIK_ESCAPE) || input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {

			isCreditVisible_ = false;

			if (audio_) {
				audio_->PlayWave(selectSoundHandle_, false, 0.55f);
			}
		}

		return;
	}

	// 上下移動操作
	bool movedCursor = false;

	if (input->TriggerKey(DIK_UP)) {
		selectedMenuIndex_ = (selectedMenuIndex_ + menuSprites_.size() - 1) % menuSprites_.size();

		movedCursor = true;

	} else if (input->TriggerKey(DIK_DOWN)) {
		selectedMenuIndex_ = (selectedMenuIndex_ + 1) % menuSprites_.size();

		movedCursor = true;
	}

	if (movedCursor) {
		UpdateMenuAppearance();

		if (audio_) {
			audio_->PlayWave(cursorMovementSoundHandle_, false, 0.35f);
		}
	}

	// 決定入力をチェック
	if (!input->TriggerKey(DIK_SPACE) && !input->TriggerKey(DIK_RETURN)) {
		return;
	}

	// 決定音を鳴らす
	if (audio_) {
		audio_->PlayWave(selectSoundHandle_, false, 0.55f);
	}

	switch (static_cast<MenuItem>(selectedMenuIndex_)) {
	case MenuItem::kPlay:

		action_ = Action::kPlay;

		// ゲーム開始時だけ高速回転
		isPlayStartAnimation_ = true;

		fade_->Start(Fade::Status::FadeOut, 1.0f);

		phase_ = Phase::kFadeOut;
		break;
	case MenuItem::kCredit:
		isCreditVisible_ = true;
		break;
	case MenuItem::kQuit:

		action_ = Action::kQuit;

		// 終了時は高速回転させない
		isPlayStartAnimation_ = false;

		fade_->Start(Fade::Status::FadeOut, 1.0f);

		phase_ = Phase::kFadeOut;
		break;
	case MenuItem::kCount:
		break;
	}
}

void TitleScene::DrawMenu() {
	if (isCreditVisible_) {
		if (creditBackdropSprite_) {
			creditBackdropSprite_->Draw();
		}
		if (creditPlaceholderSprite_) {
			creditPlaceholderSprite_->Draw();
		}
		return;
	}

	for (Sprite* sprite : menuSprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
}
