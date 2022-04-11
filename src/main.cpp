/*
 * Matthew Todd Geiger <mgeiger@newtonlabs.com>
 *
 * main.cpp
 */

// Standard Library
#include <iostream>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <chrono>
#include <string>

// SDL Includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// X Includes
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>

#ifdef DEBUG
#define DPRINT(str, ...) printf("DEBUG --> " str "\n", ##__VA_ARGS__)
#else
#define DPRINT(str, ...)
#endif

#define EPRINT(str, ...) fprintf(stderr, "ERROR --> " str "\nSTERROR: %s\n", ##__VA_ARGS__, SDL_GetError())

#define FATAL(str, ...) { fprintf(stderr, "FATAL --> " str "\nSTRERROR: %s\n", ##__VA_ARGS__, SDL_GetError()); exit(EXIT_FAILURE); } 

#if defined(UNUSED_PARAMETER)
#error UNUSED_PARAMETER has already been defined!
#else
#define UNUSED_PARAMETER(x) (void)(x)
#endif

#ifndef BASE_DIR
#define BASE_DIR "./"
#endif

#define FPS_LIMIT 250.0
#define FONT_DIR "fonts"
#define FONTSIZE 13
#define CIRCLESIZE 10.0f
#define SCREENSCALEFACTOR 1.25
#define SCREEN_WIDTH (1920/SCREENSCALEFACTOR)
#define SCREEN_HEIGHT (1080/SCREENSCALEFACTOR)
#define MOVEMENTSPEED 400.0f
#define VELOCITY 600.0
#define JUMPVELOCITY 1000.0
#define ACCELERATION 1200.0

class Clock {
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> lastRecordedTime;
		double fps = 0;

	public:
		Clock();
		double GetDeltaTime();
		double GetFPS();
};

double Clock::GetFPS() {
	return fps;
}

double Clock::GetDeltaTime() {
	auto curRecordedTime = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration<double, std::milli>(curRecordedTime - lastRecordedTime).count();
	lastRecordedTime = curRecordedTime;
	duration /= 1000;
	fps = (1.0/duration);
	return duration;
	
	//return 1.0/(fps = (1.0/duration));
}

Clock::Clock() {
	lastRecordedTime = std::chrono::high_resolution_clock::now();
}

class Character {
	private:
		//                    x    y    w   h
		SDL_FRect dimenesions;
		Uint8 cr = 0, cg = 0, cb = 0;

		void CalculateVertical(double &deltaTime, bool spaceKey);
		void CalculateHorizontal(double &deltaTime);

		bool inAir = false;
		bool isJumping = false;

		double velocity = 0;
		double acceleration = 1200.0;
		double jumpvelocity = 1200.0;
		double horizontalVelocity = 0;

	public:
		bool wantsToMoveRight = false;
		bool wantsToMoveLeft = false;
		
		Character(float x = 0.0f, float y = 0.0f, float w = 25.0f, float h = 25.0f, Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
		SDL_FRect* GetDimensions();
		void MovX(float);
		void MovY(float);
		void SetXY(float x, float y);
		void SetWH(float w, float h);
		Uint8 GetR();
		Uint8 GetG();
		Uint8 GetB();
		void Jump();
		void Update(double deltaTime, bool spaceKey = false);
};

Character::Character(float x, float y, float w, float h, Uint8 r, Uint8 g, Uint8 b) {
	SetXY(x, y);
	SetWH(w, h);

	cr = r; cg = g; cb = b;
}

void Character::Jump() {
	if(!inAir && dimenesions.y >= SCREEN_HEIGHT-dimenesions.h) {
		isJumping = true;
		inAir = true;
	}
}

void Character::CalculateVertical(double &deltaTime, bool spaceKey) {
	if(dimenesions.y >= SCREEN_HEIGHT-dimenesions.h) {
		inAir = false;
		if(spaceKey) Jump();
	} else {
		inAir = true;
	}

	double accel = acceleration;

	// Increases downward acceleration when going up
	if(velocity < 0.0) {
		accel *= 1.5;
	}

	if(inAir) {
		velocity += accel*deltaTime;
	} else if(velocity > 0.0) {
		velocity = 0.0-(velocity/2);
		if(velocity > -60)
			velocity = 0;
	}

	if(isJumping) {
		velocity = 0.0-jumpvelocity;
		isJumping = false;
	}
	
	MovY(velocity*deltaTime);
}

void Character::CalculateHorizontal(double &deltaTime) {
	bool wantsToMoveHoz = (wantsToMoveLeft || wantsToMoveRight) ? true : false;

	double hozAccelRight = 0.0-acceleration;
	double hozAccelLeft = acceleration;

	if(inAir) {
		hozAccelLeft /= 3;
		hozAccelRight /= 3;
	}

	// New horizontal movement includes velocity/acceleration
	if(wantsToMoveHoz && (wantsToMoveRight == false || wantsToMoveLeft == false)) {
		if(wantsToMoveRight) {
			// Kick start acceleration
			if(horizontalVelocity < 3.0 && horizontalVelocity > -3.0) {
				hozAccelLeft *= 3;
			} else if(horizontalVelocity < 5.0 && horizontalVelocity > -5.0) {
				hozAccelLeft *= 2;
			}

			// Boost stopping accel
			if(horizontalVelocity < 0.0) {
				hozAccelLeft *= 1.75;
			}

			horizontalVelocity += hozAccelLeft*deltaTime;
		}

		if(wantsToMoveLeft) {
			// Kick start acceleration
			if(horizontalVelocity < 3.0 && horizontalVelocity > -3.0) {
				hozAccelRight *= 3;
			} else if(horizontalVelocity < 5.0 && horizontalVelocity > -5.0) {
				hozAccelRight *= 2;
			}

			// Boost stopping accel
			if(horizontalVelocity > 0.0) {
				hozAccelRight *= 1.75;
			}

			horizontalVelocity += hozAccelRight*deltaTime;
		}

	} else {
		if(horizontalVelocity > 0.0) {
			horizontalVelocity += hozAccelRight*deltaTime;	
		} else {
			horizontalVelocity += hozAccelLeft*deltaTime;
		}
	}

	if(horizontalVelocity > -0.5 && horizontalVelocity < 0.5) {
		horizontalVelocity = 0;
	}

	MovX(horizontalVelocity*deltaTime);

}

void Character::Update(double deltaTime, bool spaceKey) {
	CalculateVertical(deltaTime, spaceKey);
	CalculateHorizontal(deltaTime);
}

Uint8 Character::GetR() {
	return cr;
}

Uint8 Character::GetG() {
	return cg;
}

Uint8 Character::GetB() {
	return cb;
}

void Character::MovX(float x) {
	dimenesions.x = std::clamp<float>(dimenesions.x + x, 0.0f, SCREEN_WIDTH-dimenesions.w);
	if(dimenesions.x == 0.0f || dimenesions.x == SCREEN_WIDTH-dimenesions.w)
		horizontalVelocity = 0.0;
}

void Character::MovY(float y) {
	dimenesions.y = std::clamp<float>(dimenesions.y + y, 0.0f, SCREEN_HEIGHT-dimenesions.h);
}

void Character::SetWH(float w, float h) {
	dimenesions.w = w;
	dimenesions.h = h;
}

void Character::SetXY(float x, float y) {
	dimenesions.x = x;
	dimenesions.y = y;
}

SDL_FRect* Character::GetDimensions() {
	return &dimenesions;
}


enum Keys {KEY_A = 0, KEY_D, KEY_S, KEY_W, KEY_SPACE, KEY_ESC, KEYS_TOTAL};

class App {
	private:
		std::string appName;
		std::string font;
		TTF_Font* tFont;
		bool keys[KEYS_TOTAL] = {0};
		bool running;
		double deltaTime;
		SDL_Renderer* renderer;
		SDL_Window* window;

		SDL_Surface* surfaceFPSText;
		SDL_Texture* textureFPSText;
		SDL_Rect rectFPSText = {0, 0, 300, 100};

		Character *player;
		Clock *timer;

		// In Loop
		void HandleKeydown(SDL_Keycode);
		void HandleKeyup(SDL_Keycode);
		void HandleKeys();

		// In render loop
		void PrepareScene();
		void PresentScene();

		// In init
		bool LoadFont();

	public:
		App(std::string name = std::string("SDL2 Application"), std::string sFont = std::string(BASE_DIR FONT_DIR "/Ubuntu-Regular.ttf"));

		int OnExecute();
		
	private:
		bool OnInit();
		void OnEvent(SDL_Event* event);
		void OnLoop();
		void OnRender();
		void OnCleanup();

};

// PRIVATES

bool App::LoadFont() {
	if((tFont = TTF_OpenFont(font.c_str(), FONTSIZE)) == nullptr)
		return false;

	return true;
}

void App::HandleKeys() {
	if(keys[KEY_A]) {
		//player->MovX(0.0-(MOVEMENTSPEED*deltaTime)); // Old Movement style
		player->wantsToMoveLeft = true;
	} else {
		player->wantsToMoveLeft = false;
	}

	if(keys[KEY_D]) {
		//player->MovX(MOVEMENTSPEED*deltaTime); // Old movement style
		player->wantsToMoveRight = true;
	} else {
		player->wantsToMoveRight = false;
	}

	if(keys[KEY_SPACE])
		player->Jump();
	if(keys[KEY_ESC])
		running = false;
}

void App::HandleKeydown(SDL_Keycode keycode) {
	switch(keycode) {
		case SDLK_a: 		keys[KEY_A] 	= true; break;
		case SDLK_d: 		keys[KEY_D] 	= true; break;
		case SDLK_SPACE: 	keys[KEY_SPACE] = true; break;
		case SDLK_ESCAPE: 	keys[KEY_ESC] 	= true; break;
	}
}

void App::HandleKeyup(SDL_Keycode keycode) {
	switch(keycode) {
		case SDLK_a: 		keys[KEY_A] 	= false; break;
		case SDLK_d: 		keys[KEY_D] 	= false; break;
		case SDLK_SPACE: 	keys[KEY_SPACE] = false; break;
		case SDLK_ESCAPE: 	keys[KEY_ESC] 	= false; break;
	}
}

void App::PrepareScene() {
	SDL_RenderClear(renderer);
	// Draw background
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	// Draw Rect
	SDL_RenderDrawRectF(renderer, player->GetDimensions());
	SDL_SetRenderDrawColor(renderer, player->GetR(), player->GetG(), player->GetB(), 255);

	// Draw FPS
	std::string FPSTextString = std::string("FPS: ") + std::to_string(1.0/deltaTime);
	surfaceFPSText = TTF_RenderText_Solid(tFont, FPSTextString.c_str(), {255, 255, 255, 255});
	rectFPSText.w = surfaceFPSText->w;
	rectFPSText.h = surfaceFPSText->h;
	textureFPSText = SDL_CreateTextureFromSurface(renderer, surfaceFPSText);	
	SDL_RenderCopy(renderer, textureFPSText, nullptr, &rectFPSText);

}

void App::PresentScene() {
	SDL_RenderPresent(renderer);

	SDL_FreeSurface(surfaceFPSText);
	SDL_DestroyTexture(textureFPSText);
}

// PUBLICS
bool App::OnInit() {
	int retry = 0;

	// Init SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		EPRINT("Failed to init SDL!");
		return false;
	}

	if(TTF_Init() == -1) {
		EPRINT("Failed to init TTF Library");
		return false;
	}

	// Create window
	if((window = SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
					SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN)) == nullptr) {
RETRY:
		DPRINT("VULKAN not available, attempting OpenGL");
		if((window = SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
					SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL)) == nullptr) {
			EPRINT("Failed to create window!");
			return false;
		}
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
//	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	// Create Renderer	
	if((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == nullptr) {
		EPRINT("Failed to create renderer");
		if(retry == 0) {
			retry++;
			SDL_DestroyWindow(window);
			goto RETRY;
		}
		return false;
	}

	player = new Character();
	SDL_FRect *playerInfo = player->GetDimensions();	
	player->SetXY(static_cast<float>(SCREEN_WIDTH)/2 - playerInfo->w/2, static_cast<float>(SCREEN_HEIGHT)/2 - playerInfo->h/2);

	timer = new Clock;

	if(!LoadFont()) {
		EPRINT("Failed to load font!");
		return false;
	}

	DPRINT("App::OnInit() Completed");

	return true;
}

void App::OnEvent(SDL_Event* event) {
	switch(event->type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			HandleKeydown(event->key.keysym.sym);
			break;
		case SDL_KEYUP:
			HandleKeyup(event->key.keysym.sym);
			break;
		default:
			break;
	}
}

void App::OnLoop() {
	HandleKeys();
	player->Update(deltaTime, keys[KEY_SPACE]);
}

void App::OnRender() {
	PrepareScene();
	PresentScene();
}

void App::OnCleanup() {
	delete player;
	delete timer;
	TTF_CloseFont(tFont);
	DPRINT("App::OnCleanup() Completed");
}

App::App(std::string name, std::string sFont) {
	running = true;
	appName = name;
	font = sFont;
}

int App::OnExecute() {
	DPRINT("App::OnExecute() Starting");
	if(OnInit() == false)
		return -1;

	SDL_Event event;

	while(running) {
		deltaTime += timer->GetDeltaTime();
		//std::cout << "deltaTime: " << deltaTime << " of " << (1.0/60.0) << std::endl;

		if(deltaTime > 1.0/FPS_LIMIT) {
			// Handle SDL Events
			while(SDL_PollEvent(&event)) {
				OnEvent(&event);
			}

			// Game Logic
			OnLoop();

			// Render...
			OnRender();

			deltaTime = 0.0;
		}
	}

	// Cleanup your nasty ass
	OnCleanup();

	return 0;
}

int main(int argc, char** argv) {
	UNUSED_PARAMETER(argc);

	App *application = new App(argv[0]);

	if(application->OnExecute() < 0)
		FATAL("Application FATAL Error!");

	delete application;

	return EXIT_SUCCESS;
}

