#include "Asteroid.h"
#include "Asteroids.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "GameUtil.h"
#include "GameWindow.h"
#include "GameWorld.h"
#include "GameDisplay.h"
#include "Spaceship.h"
#include "BoundingShape.h"
#include "BoundingSphere.h"
#include "GUILabel.h"
#include "Explosion.h"
#include "ExtraLife.h"
#include "Invulnerability.h"

// PUBLIC INSTANCE CONSTRUCTORS ///////////////////////////////////////////////

/** Constructor. Takes arguments from command line, just in case. */
Asteroids::Asteroids(int argc, char* argv[])
	: GameSession(argc, argv)
{
	mLevel = 0;
	mAsteroidCount = 0;
}

/** Destructor. */
Asteroids::~Asteroids(void)
{
}

// PUBLIC INSTANCE METHODS ////////////////////////////////////////////////////

/** Start an asteroids game. */
void Asteroids::Start()
{
	// Create a shared pointer for the Asteroids game object - DO NOT REMOVE
	shared_ptr<Asteroids> thisPtr = shared_ptr<Asteroids>(this);

	// Add this class as a listener of the game world
	mGameWorld->AddListener(thisPtr.get());

	// Add this as a listener to the world and the keyboard
	mGameWindow->AddKeyboardListener(thisPtr);

	// Add a score keeper to the game world
	mGameWorld->AddListener(&mScoreKeeper);

	// Add this class as a listener of the score keeper
	mScoreKeeper.AddListener(thisPtr);

	// Create an ambient light to show sprite textures
	GLfloat ambient_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat diffuse_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
	glEnable(GL_LIGHT0);

	Animation* explosion_anim = AnimationManager::GetInstance().CreateAnimationFromFile("explosion", 64, 1024, 64, 64, "explosion_fs.png");
	Animation* asteroid1_anim = AnimationManager::GetInstance().CreateAnimationFromFile("asteroid1", 128, 8192, 128, 128, "asteroid1_fs.png");
	Animation* spaceship_anim = AnimationManager::GetInstance().CreateAnimationFromFile("spaceship", 128, 128, 128, 128, "spaceship_fs.png");

	// Create spaceship but don't add to world yet
	CreateSpaceship();

	// Create some asteroids and add them to the world
	CreateAsteroids(10);

	//Create the GUI
	CreateGUI();

	// Add a player (watcher) to the game world
	mGameWorld->AddListener(&mPlayer);

	// Add this class as a listener of the player
	mPlayer.AddListener(thisPtr);

	// Set game state flags
	mGameStarted = false;
	mShowInstructions = false;

	// Show start screen, hide score and lives
	mStartLabel->SetVisible(true);
	mScoreLabel->SetVisible(false);
	mLivesLabel->SetVisible(false);

	// Start the game
	// Start timer to spawn extra life power-ups every 15 seconds
	SetTimer(15000, SPAWN_EXTRA_LIFE);
	// Start timer to spawn invulnerability power-ups every 20 seconds
	SetTimer(5000, SPAWN_INVULNERABILITY);

	// Start the game
	GameSession::Start();
}

/** Stop the current game. */
void Asteroids::Stop()
{
	// Stop the game
	GameSession::Stop();
}

// PUBLIC INSTANCE METHODS IMPLEMENTING IKeyboardListener /////////////////////

void Asteroids::OnKeyPressed(uchar key, int x, int y)
{
	switch (key)
	{
		// Spacebar - shoot (only when game is active)
	case ' ':
		if (mGameStarted) mSpaceship->Shoot();
		break;

		// ENTER key - start the game from menu
	case 13:
		if (!mGameStarted && !mShowInstructions)
		{
			mGameStarted = true;
			mStartLabel->SetVisible(false);
			mScoreLabel->SetVisible(true);
			mLivesLabel->SetVisible(true);
			mGameWorld->AddObject(mSpaceship);
		}
		break;

		// I key - show instructions page
	case 'i':
	case 'I':
		if (!mGameStarted && !mShowInstructions)
		{
			mShowInstructions = true;
			mStartLabel->SetVisible(false);
			mInstructionsLabel->SetVisible(true);
		}
		break;

		// B key - go back to main menu from instructions
	case 'b':
	case 'B':
		if (mShowInstructions)
		{
			mShowInstructions = false;
			mInstructionsLabel->SetVisible(false);
			mStartLabel->SetVisible(true);
		}
		break;

	default:
		break;
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{
	if (!mGameStarted) return;
	switch (key)
	{
		// If up arrow key is pressed start applying forward thrust
	case GLUT_KEY_UP: mSpaceship->Thrust(10); break;
		// If left arrow key is pressed start rotating anti-clockwise
	case GLUT_KEY_LEFT: mSpaceship->Rotate(90); break;
		// If right arrow key is pressed start rotating clockwise
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(-90); break;
		// Default case - do nothing
	default: break;
	}
}

void Asteroids::OnSpecialKeyReleased(int key, int x, int y)
{
	if (!mGameStarted) return;
	switch (key)
	{
		// If up arrow key is released stop applying forward thrust
	case GLUT_KEY_UP: mSpaceship->Thrust(0); break;
		// If left arrow key is released stop rotating
	case GLUT_KEY_LEFT: mSpaceship->Rotate(0); break;
		// If right arrow key is released stop rotating
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(0); break;
		// Default case - do nothing
	default: break;
	}
}

// PUBLIC INSTANCE METHODS IMPLEMENTING IGameWorldListener ////////////////////

void Asteroids::OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object)
{
	// Handle asteroid removal
	if (object->GetType() == GameObjectType("Asteroid"))
	{
		// Create explosion at asteroid position
		shared_ptr<GameObject> explosion = CreateExplosion();
		explosion->SetPosition(object->GetPosition());
		explosion->SetRotation(object->GetRotation());
		mGameWorld->AddObject(explosion);

		// Decrement asteroid count and start next level if all gone
		mAsteroidCount--;
		if (mAsteroidCount <= 0)
		{
			SetTimer(500, START_NEXT_LEVEL);
		}
	}

	// Handle extra life power-up collection
	if (object->GetType() == GameObjectType("ExtraLife"))
	{
		// Only add life if properly collected by spaceship
		shared_ptr<ExtraLife> extraLife = dynamic_pointer_cast<ExtraLife>(object);
		if (extraLife && extraLife->mCollectedBySpaceship)
		{
			// Add a life to the player
			mPlayer.AddLife();

			// Update the lives label
			std::ostringstream msg_stream;
			msg_stream << "Lives: " << mPlayer.GetLives();
			mLivesLabel->SetText(msg_stream.str());
		}
	}
	
	// Handle invulnerability power-up collection
	if (object->GetType() == GameObjectType("Invulnerability"))
	{
		// Make spaceship invulnerable for 5 seconds
		mSpaceship->SetInvulnerable(true);
		mShieldLabel->SetVisible(true);
		SetTimer(5000, END_INVULNERABILITY);
	}
}

// PUBLIC INSTANCE METHODS IMPLEMENTING ITimerListener ////////////////////////

void Asteroids::OnTimer(int value)
{
	if (value == CREATE_NEW_PLAYER)
	{
		mSpaceship->Reset();
		mGameWorld->AddObject(mSpaceship);
	}

	if (value == START_NEXT_LEVEL)
	{
		mLevel++;
		int num_asteroids = 10 + 2 * mLevel;
		CreateAsteroids(num_asteroids);
	}

	if (value == SHOW_GAME_OVER)
	{
		mGameOverLabel->SetVisible(true);
	}
	
	// Spawn an extra life power-up and set timer for next one
	if (value == SPAWN_EXTRA_LIFE)
	{
		CreateExtraLife();
		SetTimer(15000, SPAWN_EXTRA_LIFE);
	}
	
	// Spawn invulnerability power-up
	if (value == SPAWN_INVULNERABILITY)
	{
		CreateInvulnerability();
		SetTimer(20000, SPAWN_INVULNERABILITY);
	}

	// End invulnerability
	if (value == END_INVULNERABILITY)
	{
		mSpaceship->SetInvulnerable(false);
		mShieldLabel->SetVisible(false);
	}
}

// PROTECTED INSTANCE METHODS /////////////////////////////////////////////////
shared_ptr<GameObject> Asteroids::CreateSpaceship()
{
	// Create a raw pointer to a spaceship that can be converted to
	// shared_ptrs of different types because GameWorld implements IRefCount
	mSpaceship = make_shared<Spaceship>();
	mSpaceship->SetBoundingShape(make_shared<BoundingSphere>(mSpaceship->GetThisPtr(), 4.0f));
	shared_ptr<Shape> bullet_shape = make_shared<Shape>("bullet.shape");
	mSpaceship->SetBulletShape(bullet_shape);
	Animation* anim_ptr = AnimationManager::GetInstance().GetAnimationByName("spaceship");
	shared_ptr<Sprite> spaceship_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	mSpaceship->SetSprite(spaceship_sprite);
	mSpaceship->SetScale(0.1f);
	// Reset spaceship back to centre of the world
	mSpaceship->Reset();
	// Return the spaceship so it can be added to the world
	return mSpaceship;
}

void Asteroids::CreateAsteroids(const uint num_asteroids)
{
	mAsteroidCount = num_asteroids;
	for (uint i = 0; i < num_asteroids; i++)
	{
		Animation* anim_ptr = AnimationManager::GetInstance().GetAnimationByName("asteroid1");
		shared_ptr<Sprite> asteroid_sprite
			= make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
		asteroid_sprite->SetLoopAnimation(true);
		shared_ptr<GameObject> asteroid = make_shared<Asteroid>();
		asteroid->SetBoundingShape(make_shared<BoundingSphere>(asteroid->GetThisPtr(), 10.0f));
		asteroid->SetSprite(asteroid_sprite);
		asteroid->SetScale(0.2f);
		mGameWorld->AddObject(asteroid);
	}
}

void Asteroids::CreateGUI()
{
	// Add a (transparent) border around the edge of the game display
	mGameDisplay->GetContainer()->SetBorder(GLVector2i(10, 10));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mScoreLabel = make_shared<GUILabel>("Score: 0");
	// Set the vertical alignment of the label to GUI_VALIGN_TOP
	mScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);
	// Add the GUILabel to the GUIComponent  
	shared_ptr<GUIComponent> score_component
		= static_pointer_cast<GUIComponent>(mScoreLabel);
	mGameDisplay->GetContainer()->AddComponent(score_component, GLVector2f(0.0f, 1.0f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mLivesLabel = make_shared<GUILabel>("Lives: 3");
	// Set the vertical alignment of the label to GUI_VALIGN_BOTTOM
	mLivesLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_BOTTOM);
	// Add the GUILabel to the GUIComponent  
	shared_ptr<GUIComponent> lives_component = static_pointer_cast<GUIComponent>(mLivesLabel);
	mGameDisplay->GetContainer()->AddComponent(lives_component, GLVector2f(0.0f, 0.0f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mGameOverLabel = shared_ptr<GUILabel>(new GUILabel("GAME OVER"));
	// Set the horizontal alignment of the label to GUI_HALIGN_CENTER
	mGameOverLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	// Set the vertical alignment of the label to GUI_VALIGN_MIDDLE
	mGameOverLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	// Set the visibility of the label to false (hidden)
	mGameOverLabel->SetVisible(false);
	// Add the GUILabel to the GUIContainer  
	shared_ptr<GUIComponent> game_over_component
		= static_pointer_cast<GUIComponent>(mGameOverLabel);
	mGameDisplay->GetContainer()->AddComponent(game_over_component, GLVector2f(0.5f, 0.5f));

	// Start screen label
	mStartLabel = make_shared<GUILabel>("ENTER: Start  I: Instructions");
	mStartLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mStartLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mStartLabel->SetVisible(true);
	shared_ptr<GUIComponent> start_component
		= static_pointer_cast<GUIComponent>(mStartLabel);
	mGameDisplay->GetContainer()->AddComponent(start_component, GLVector2f(0.5f, 0.5f));

	// Instructions label
	mInstructionsLabel = make_shared<GUILabel>("Arrows: Move  Space: Shoot  B: Back");
	mInstructionsLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstructionsLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstructionsLabel->SetVisible(false);
	shared_ptr<GUIComponent> inst1
		= static_pointer_cast<GUIComponent>(mInstructionsLabel);
	mGameDisplay->GetContainer()->AddComponent(inst1, GLVector2f(0.5f, 0.5f));

	// Shield active label - shown when invulnerability is active
	mShieldLabel = make_shared<GUILabel>("** SHIELD ACTIVE **");
	mShieldLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mShieldLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mShieldLabel->SetVisible(false);
	shared_ptr<GUIComponent> shield_component
		= static_pointer_cast<GUIComponent>(mShieldLabel);
	mGameDisplay->GetContainer()->AddComponent(shield_component, GLVector2f(0.5f, 0.3f));
}

void Asteroids::OnScoreChanged(int score)
{
	// Format the score message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Score: " << score;
	// Get the score message as a string
	std::string score_msg = msg_stream.str();
	mScoreLabel->SetText(score_msg);
}

void Asteroids::OnPlayerKilled(int lives_left)
{
	shared_ptr<GameObject> explosion = CreateExplosion();
	explosion->SetPosition(mSpaceship->GetPosition());
	explosion->SetRotation(mSpaceship->GetRotation());
	mGameWorld->AddObject(explosion);

	// Format the lives left message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Lives: " << lives_left;
	// Get the lives left message as a string
	std::string lives_msg = msg_stream.str();
	mLivesLabel->SetText(lives_msg);

	if (lives_left > 0)
	{
		SetTimer(1000, CREATE_NEW_PLAYER);
	}
	else
	{
		SetTimer(500, SHOW_GAME_OVER);
	}
}

shared_ptr<GameObject> Asteroids::CreateExplosion()
{
	Animation* anim_ptr = AnimationManager::GetInstance().GetAnimationByName("explosion");
	shared_ptr<Sprite> explosion_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	explosion_sprite->SetLoopAnimation(false);
	shared_ptr<GameObject> explosion = make_shared<Explosion>();
	explosion->SetSprite(explosion_sprite);
	explosion->Reset();
	return explosion;
}

void Asteroids::CreateExtraLife()
{
	// Create a new extra life power-up
	shared_ptr<GameObject> extra_life = make_shared<ExtraLife>();

	// Load the star sprite for the extra life power-up
	Animation* anim_ptr = AnimationManager::GetInstance().CreateAnimationFromFile(
		"extra_life", 64, 64, 64, 64, "Extra_Life.png");
	shared_ptr<Sprite> extra_life_sprite
		= make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	extra_life_sprite->SetLoopAnimation(false);

	// Set sprite, scale and bounding shape
	extra_life->SetSprite(extra_life_sprite);
	extra_life->SetScale(0.2f);
	extra_life->SetBoundingShape(make_shared<BoundingSphere>(extra_life->GetThisPtr(), 10.0f));

	// Add it to the game world
	mGameWorld->AddObject(extra_life);
}

void Asteroids::CreateInvulnerability()
{
	// Create a new invulnerability power-up
	shared_ptr<GameObject> invulnerability = make_shared<Invulnerability>();

	// Use a sprite for the invulnerability power-up
	Animation* anim_ptr = AnimationManager::GetInstance().CreateAnimationFromFile(
		"invulnerability", 64, 64, 64, 64, "defence.png");
	shared_ptr<Sprite> invulnerability_sprite
		= make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	invulnerability_sprite->SetLoopAnimation(false);

	// Set sprite, scale and bounding shape
	invulnerability->SetSprite(invulnerability_sprite);
	invulnerability->SetScale(0.2f);
	invulnerability->SetBoundingShape(make_shared<BoundingSphere>(invulnerability->GetThisPtr(), 10.0f));

	// Add it to the game world
	mGameWorld->AddObject(invulnerability);
}