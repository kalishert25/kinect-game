#include "raylib.h"
#include "raymath.h"
#include <deque>

enum class Lane {
	left = -1, center, right
};

struct Obstacle {
	Lane lane;
	float distance;
};

const float PLAYER_RADIUS = .4f;

struct GameState {

	std::deque<Obstacle> obstacles;
	Camera3D camera; // = { 0 };
	Model roadModel; // 

	// we render to a texture so we can use shaders on it to blur it when the game is paused
	RenderTexture2D renderTexture;
	Shader blurShader;
	Shader tilingShader;
	int renderWidthLoc;
	int renderHeightLoc;


	int screenWidth;
	int screenHeight;
	

	bool isPaused; // false
	
	Model playerModel;
	Vector3 playerVelocity;
	Vector3 playerPosition;
	float forwardVelocty;
	Vector3 roadPosition;

};

void DrawGame(const GameState& gs);
void InitializeGameState(GameState& gs) {
	gs.isPaused = false;
	
	gs.renderTexture = LoadRenderTexture(gs.screenWidth, gs.screenHeight);
	gs.camera = { 0 };
	gs.camera.position = Vector3{ 0.0f, 3.0f, 5.0f };
	gs.camera.target = Vector3{ 0.0f, 0.0f, -5.0f };
	gs.camera.up = Vector3{ 0.0f, 1.0f, 0.0f  };
	gs.camera.fovy = 45.0f;
	gs.camera.projection = CAMERA_PERSPECTIVE;

	Texture2D roadTexture = LoadTexture("Assets/road.png");
	gs.roadModel = LoadModelFromMesh(GenMeshPlane(4.0f, 200.0f, 2, 2));
	gs.roadModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = roadTexture;
	gs.blurShader = LoadShader(0, "Shaders/blur.fs");
	float tiling[2] = { 1.0f, 10.0f };
	gs.tilingShader = LoadShader(0, "Shaders/tiling.fs");
	SetShaderValue(gs.tilingShader, GetShaderLocation(gs.tilingShader, "tiling"), tiling, SHADER_UNIFORM_VEC2);
	gs.roadModel.materials[0].shader = gs.tilingShader;

	gs.playerPosition = { 0.0f, PLAYER_RADIUS * 1.0f, 0.0f };
	gs.playerVelocity = { 0.0f };
	gs.roadPosition = { 0.0f };
	gs.forwardVelocty = 2;

	int tintColorLoc = GetShaderLocation(gs.blurShader, "tintColor");
	int tintStrengthLoc = GetShaderLocation(gs.blurShader, "tintStrength");
	gs.renderWidthLoc = GetShaderLocation(gs.blurShader, "renderWidth");
	gs.renderHeightLoc = GetShaderLocation(gs.blurShader, "renderHeight");


	float tintColor[3] = { 0 / 255.0, 5.0 / 255.0, 25.0 / 255.0 };
	float tintStrength = 0.4f;

	SetShaderValue(gs.blurShader, tintColorLoc, tintColor, SHADER_UNIFORM_VEC3);
	SetShaderValue(gs.blurShader, tintStrengthLoc, &tintStrength, SHADER_UNIFORM_FLOAT);
	SetShaderValue(gs.blurShader, gs.renderWidthLoc, &gs.screenWidth, SHADER_UNIFORM_FLOAT);
	SetShaderValue(gs.blurShader, gs.renderHeightLoc, &gs.screenHeight, SHADER_UNIFORM_FLOAT);

	gs.playerModel = LoadModel("Assets/basketball.obj");
	Texture playerTexture = LoadTexture("Assets/balldimpled.png");
	gs.playerModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = playerTexture;

}
int main() {

	GameState gs{};

	gs.screenWidth = 800;
	gs.screenHeight = 450;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT); // Make the window resizable
	InitWindow(gs.screenWidth, gs.screenHeight, "Kinect Game");
	InitializeGameState(gs);
	// camera

	SetTargetFPS(60);

	// -------------------------------------------------------
	// main game loop
	// -------------------------------------------------------
	float spawnTimer = 0; //sec
	while (!WindowShouldClose()) {
		if (GetScreenWidth() != gs.screenWidth || GetScreenHeight() != gs.screenHeight) {
			gs.renderTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
			gs.screenWidth = GetScreenWidth();
			gs.screenHeight = GetScreenHeight();
			SetShaderValue(gs.blurShader, gs.renderWidthLoc, &gs.screenWidth, SHADER_UNIFORM_FLOAT);
			SetShaderValue(gs.blurShader, gs.renderHeightLoc, &gs.screenHeight, SHADER_UNIFORM_FLOAT);
		}
		if (IsKeyPressed(KEY_SPACE)) { gs.isPaused = !gs.isPaused; }
		if (gs.isPaused) {

			BeginTextureMode(gs.renderTexture);
			DrawGame(gs);
			EndTextureMode();

			BeginDrawing();
			ClearBackground(WHITE);
			BeginShaderMode(gs.blurShader);
			DrawTextureRec(gs.renderTexture.texture, Rectangle{ 0, 0, (float)gs.renderTexture.texture.width, (float)-gs.renderTexture.texture.height }, Vector2{ 0, 0 }, WHITE);
			EndShaderMode();


			//DrawRectangle(0, 0, W, H, Color{ 255, 255, 255, 50 });
			DrawText("Paused", gs.screenWidth * 0.5 - MeasureText("Paused", 50) * 0.5, gs.screenHeight * 0.1, 50, BLACK);

			EndDrawing();

			continue;
		}
		//Update

		const float deltaTime = GetFrameTime();
		Vector3 originalPlayerPos = gs.playerPosition;
		gs.forwardVelocty += 0.1 * deltaTime;
		spawnTimer -= deltaTime;
		if (spawnTimer < 0) {
			//spawn new obstacle
			gs.obstacles.push_back(Obstacle{ (Lane)GetRandomValue(-1, 1), 20.0 });
			// reset timer
			spawnTimer = GetRandomValue(2, 4);
		}
		const float playerLateralAcceleration = 2;
		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { gs.playerVelocity.x -= playerLateralAcceleration * deltaTime; }
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { gs.playerVelocity.x += playerLateralAcceleration * deltaTime; }

		gs.playerPosition = Vector3Add(gs.playerPosition, Vector3Scale(gs.playerVelocity, deltaTime));
		gs.playerPosition.x = Clamp(gs.playerPosition.x, -1, 1);
		if (gs.playerPosition.x == 1 || gs.playerPosition.x == -1) {
			gs.playerVelocity.x = 0;
		}
		//friction
		gs.playerVelocity.x = Clamp(gs.playerVelocity.x * 0.98, -1.0, 1.0);
		for (int i = 0; i < gs.obstacles.size(); i++) { gs.obstacles[i].distance -= gs.forwardVelocty * deltaTime; }
		gs.roadPosition.z += gs.forwardVelocty * deltaTime;
		if (gs.roadPosition.z >= 20.0f) gs.roadPosition.z -= 20.0f;

		
		Vector3 playerRelativeVelocity = Vector3Add(gs.playerVelocity, Vector3{ 0.0f, 0.0f, -gs.forwardVelocty });
		Vector3 up = Vector3{ 0.0f, 1.0f, 0.0f };
		Vector3 axis = Vector3CrossProduct(playerRelativeVelocity, up);
		float angularSpeed = Vector3Length(playerRelativeVelocity) / PLAYER_RADIUS;
		gs.playerModel.transform = MatrixMultiply(gs.playerModel.transform, MatrixRotate(axis, -angularSpeed  * deltaTime));
		
		if (gs.obstacles.size() > 0 && gs.obstacles[0].distance <= -10.0f) {
			gs.obstacles.pop_front();
		}

		for (size_t i = 0; i < gs.obstacles.size(); i++)
		{	
			Obstacle obs = gs.obstacles[i];
			// calculate obstacle-player intersection
		}
		
		//Draw
		BeginDrawing();
		DrawGame(gs);
		EndDrawing();
	}

	CloseWindow();
}

void DrawGame(const GameState& gs)
{

	ClearBackground(RAYWHITE);
	BeginMode3D(gs.camera);
		DrawModel(gs.playerModel, gs.playerPosition, PLAYER_RADIUS * 1.0f, WHITE);
		//DrawLine3D(gs.playerPosition, Vector3Add(gs.playerPosition, Vector3Normalize(playerRelativeVelocity)), BLUE);
		//DrawLine3D(gs.playerPosition, Vector3Add(gs.playerPosition, Vector3Normalize(axis)), RED);

	for (int i = 0; i < gs.obstacles.size(); i++) {
		Vector3 pos{ (float)gs.obstacles[i].lane, 0.8f, -gs.obstacles[i].distance };
		DrawCube(pos, 0.8f, 1.0f, 0.8f, Color{ 155, 50, 120, 255 });
		DrawCubeWires(pos, 0.8f, 1.0f, 0.8f, BLACK);

	}
	DrawModel(gs.roadModel, gs.roadPosition, 1.0, WHITE);
	EndMode3D();
	DrawFPS(10, 10);

}


