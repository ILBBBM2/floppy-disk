#include "raylib.h"
#include "rcamera.h"
#include "raymath.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>

struct Box {
    Vector3 position;
    bool isActive;
    Vector3 velocity;
    int health;
};

bool CheckRayCollisionBox(Ray ray, BoundingBox box) {
    Vector3 invDir = { 1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z };
    Vector3 tMin = Vector3Multiply(Vector3Subtract(box.min, ray.position), invDir);
    Vector3 tMax = Vector3Multiply(Vector3Subtract(box.max, ray.position), invDir);
    Vector3 t1 = Vector3Min(tMin, tMax);
    Vector3 t2 = Vector3Max(tMin, tMax);
    float tNear = fmax(fmax(t1.x, t1.y), t1.z);
    float tFar = fmin(fmin(t2.x, t2.y), t2.z);
    return tNear <= tFar && tFar >= 0.0f;
}

std::vector<float> LoadAccuracy() {
    std::vector<float> accuracies;
    std::ifstream file("accuracy.txt");
    std::string line;
    while (std::getline(file, line)) {
        try {
            accuracies.push_back(std::stof(line));
        } catch (const std::invalid_argument& e) {
            std::cerr << "invalid data ion acurracy.txt " << line << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "out of range in accuracy.txt " << line << std::endl;
        }
    }
    file.close();
    
    return accuracies;
}

std::vector<float> clearaccuracy(){
    
    std::vector<float> accuracies;
    std::ifstream file("accuracy.txt");
    std::string line;
    file.clear();
    file.close();
    return accuracies;
}

void DrawGraph(const std::vector<float>& accuracies, int screenWidth, int screenHeight) {
    int graphWidth = screenWidth - 40;
    int graphHeight = 200;
    int graphX = 20;
    int graphY = screenHeight - graphHeight - 20;

    DrawRectangleLines(graphX, graphY, graphWidth, graphHeight, BLACK);

    if (accuracies.empty()) return;

    float maxAccuracy = 100.0f;
    float minAccuracy = 0.0f;

    for (size_t i = 1; i < accuracies.size(); ++i) {
        float x1 = graphX + (i - 1) * (graphWidth / (accuracies.size() - 1));
        float y1 = graphY + graphHeight - ((accuracies[i - 1] - minAccuracy) / (maxAccuracy - minAccuracy) * graphHeight);
        float x2 = graphX + i * (graphWidth / (accuracies.size() - 1));
        float y2 = graphY + graphHeight - ((accuracies[i] - minAccuracy) / (maxAccuracy - minAccuracy) * graphHeight);
        DrawLine(x1, y1, x2, y2, RED);
        DrawText(TextFormat("%.2f", accuracies[i - 1]), x1, y1 - 10, 10, BLACK);
    }
    // Draw the last point label
    float lastX = graphX + (accuracies.size() - 1) * (graphWidth / (accuracies.size() - 1));
    float lastY = graphY + graphHeight - ((accuracies.back() - minAccuracy) / (maxAccuracy - minAccuracy) * graphHeight);
    DrawText(TextFormat("%.2f", accuracies.back()), lastX, lastY - 10, 10, BLACK);
}


void SaveAccuracy(float accuracy) {
    std::ofstream file("accuracy.txt", std::ios::app);
    if (file.is_open()) {
        //just slime me out bro
        file << accuracy << std::endl;
        file.close();
    }
}

void ResetGame(float &shots, float &hits, float &accuracy, std::vector<Box> &redBoxes) {
    shots = 0;
    hits = 0;
    accuracy = 100;
    for (auto& box : redBoxes) {
        box.position = (Vector3){ GetRandomValue(-10, 10), GetRandomValue(2, 5), GetRandomValue(-10, 10) };
        box.isActive = true;
    }
}

void SpawnMovingBox(std::vector<Box> &redBoxes, const Camera &camera) {
    redBoxes.clear();
    Box movingBox;

    // Calculate the player's forward direction
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    // Keep trying to spawn the box until it is within the player's FOV
    bool validSpawn = false;
    while (!validSpawn) {
        movingBox.position = (Vector3){
            GetRandomValue(-10, 10), //x pos
            (float)GetRandomValue(1, 5),                    //y pos (rn its just a fixed value)
            GetRandomValue(-10, 10) //z pos
        };

        //calculate the direction from the player to the box
        Vector3 toBox = Vector3Normalize(Vector3Subtract(movingBox.position, camera.position));

        //FOV and angle check
        float dotProduct = Vector3DotProduct(forward, toBox);
        float angle = acosf(dotProduct) * RAD2DEG; //convert to degrees because radians

        if (angle <= 45.0f) { //using 90 degree fov (change in future if no longer true)
            validSpawn = true;
        }
    }

    movingBox.isActive = true;
    movingBox.velocity = (Vector3){ GetRandomValue(0, 1) == 0 ? -2.0f : 2.0f, 0.0f, 0.0f }; //moving shitttt
    movingBox.health = 3; //how many seconds HEAVY APPROXIMATION
    redBoxes.push_back(movingBox);
}




int main(void)
{
    int screenWidth = 800;
    int screenHeight = 600; 
    bool aimlabs = false;
    InitWindow(screenWidth, screenHeight, "hawk tuah!");
    Texture2D boxTexture = LoadTexture("assets/box.png");
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    Model boxModel = LoadModelFromMesh(cubeMesh);
    boxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = boxTexture;   
    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };   
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };      
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          
    camera.fovy = 60.0f;                                
    camera.projection = CAMERA_PERSPECTIVE;             
    int cameraMode = CAMERA_FIRST_PERSON;
    Vector3 pillarPosition = { 0.0f, 1.0f, 0.0f };
    float pillarHeight = 2.0f;
    float pillarWidth = 1.0f;
    float pillarDepth = 1.0f;
    float pillarSpeed = 2.0f;
    Vector3 playerPosition = { 0.0f, 1.0f, 0.0f };
    float playerVelocityY = 0.0f;
    const float gravity = -9.81f;   
    float actualsens = 1;
    float shots = 0;
    float hits = 0;
    float accuracy = 100;
    int bruh = 0;
    bool isshowinggraph = false;
    std::vector<Box> redBoxes;
    for (int i = 0; i < 5; i++) {
        redBoxes.push_back({ (Vector3){ GetRandomValue(-10, 10), GetRandomValue(2, 5), GetRandomValue(-10, 10) }, true });
    }
    DisableCursor();
    SetTargetFPS(200);
    float damageTimer = 0.0f;
    
    while (!WindowShouldClose())
    {
        //std::cout<< "bruh";
        if (IsKeyPressed(KEY_F5)) {
            aimlabs = true;
            std::cout << "aimlabs mode activated";
            ResetGame(shots, hits, accuracy, redBoxes);
            SpawnMovingBox(redBoxes, camera); // Pass the camera to restrict spawn area
        }
        if (IsKeyPressed(KEY_F1))
        {
            cameraMode = CAMERA_FIRST_PERSON;
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        }
        if (IsKeyPressed(KEY_F2))
        {
            cameraMode = CAMERA_ORBITAL;
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        }
        if (IsKeyPressed(KEY_PERIOD)){
            if(actualsens > 0){
                actualsens += 1;
                //std::cout << "worked";
            }
        }
        if(IsKeyPressed(KEY_COMMA)){
            if(actualsens > 0){
                actualsens -= 1;
            }
        }
        if(IsKeyPressed(KEY_R)){
            shots = 0;
            hits = 0;
            accuracy = 100;
            for (auto& box : redBoxes) {
                box.position = (Vector3){ GetRandomValue(-10, 10), GetRandomValue(2, 5), GetRandomValue(-10, 10) };
                box.isActive = true;
            }
        }
        if(IsKeyPressed(KEY_DELETE)){
            clearaccuracy();
        }
        if(IsKeyPressed(KEY_F9)){
            SetWindowSize(1920, 1080);
            SetWindowPosition(0, 0);
            screenWidth = 1920;
            screenHeight = 1080;
        }
        if(IsKeyPressed(KEY_F10)){
            SetWindowSize(800, 600);
            SetWindowPosition(1920/4 + 100, 1080/4);
            screenWidth = 800;
            screenHeight = 600;
        }
        if (IsKeyPressed(KEY_P))
        {
            if (camera.projection == CAMERA_PERSPECTIVE)
            {
                cameraMode = CAMERA_THIRD_PERSON;
                camera.position = (Vector3){ 0.0f, 2.0f, -100.0f };
                camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
                camera.up = (Vector3){ 0.0f, 100.0f, 0.0f };
                camera.projection = CAMERA_ORTHOGRAPHIC;
                camera.fovy = 20.0f;
                CameraYaw(&camera, -135 * DEG2RAD, true);
                CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
            }
            else if (camera.projection == CAMERA_ORTHOGRAPHIC)
            {
                cameraMode = CAMERA_FIRST_PERSON;
                camera.position = (Vector3){ 0.0f, 2.0f, 10.0f };
                camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
                camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
                camera.projection = CAMERA_PERSPECTIVE;
                camera.fovy = 60.0f;
            }
        }

        UpdateCamera(&camera, cameraMode);        
        if (cameraMode == CAMERA_FIRST_PERSON)
        {
            Vector2 mouseDelta = GetMouseDelta();
            float sensitivity = 0.005f;
            //std::cout<<actualsens;
            
            sensitivity = actualsens * 0.0005f;
            //std::cout << sensitivity;
            camera.target = Vector3Add(camera.target, Vector3Scale(camera.up, -mouseDelta.y * sensitivity));
            camera.target = Vector3Add(camera.target, Vector3Scale(Vector3CrossProduct(camera.up, Vector3Subtract(camera.target, camera.position)), -mouseDelta.x * sensitivity));
        }
        int boxhealth = 100;
        int currboxhealth = 100;
        if (aimlabs) {
            shots++;
            // Update the moving box
            if (!redBoxes.empty()) {
                Box &movingBox = redBoxes[0];
                movingBox.position.x += movingBox.velocity.x * GetFrameTime();

                // Reverse direction if it hits boundaries
                if (movingBox.position.x > 10.0f || movingBox.position.x < -10.0f) {
                    movingBox.velocity.x *= -1;
                }
            }

            // Constant laser firing
            Ray ray = GetMouseRay((Vector2){ screenWidth / 2, screenHeight / 2 }, camera);
            if (!redBoxes.empty()) {
                Box &movingBox = redBoxes[0];
                BoundingBox boxBounds = { 
                    (Vector3){ movingBox.position.x - 0.5f, movingBox.position.y - 0.5f, movingBox.position.z - 0.5f },
                    (Vector3){ movingBox.position.x + 0.5f, movingBox.position.y + 0.5f, movingBox.position.z + 0.5f }
                };

                if (movingBox.isActive && CheckRayCollisionBox(ray, boxBounds)) {
                    damageTimer += GetFrameTime();
                    hits ++;

                    if (damageTimer >= 1.0f) { // Apply damage every second
                        movingBox.health -= 1;
                        damageTimer = 0.0f;
                        if (movingBox.health <= 0) {
                            movingBox.isActive = false;
                            hits += 1;
                            accuracy = (hits / (shots + 1)) * 100; // Update accuracy
                            SpawnMovingBox(redBoxes, camera); // Respawn the moving box
                        }
                    }
                } else {
                    damageTimer = 0.0f; // Reset timer if the laser is not hitting the box
                }
            }
        }


        pillarPosition.x += pillarSpeed * GetFrameTime();
        if (pillarPosition.x > 15.0f || pillarPosition.x < -15.0f)
        {
            pillarSpeed *= -1;
        }

        playerVelocityY += gravity * GetFrameTime();
        playerPosition.y += playerVelocityY * GetFrameTime();

        //collision with ground
        if (playerPosition.y <= 1.0f)
        {
            playerPosition.y = 1.0f;
            playerVelocityY = 0.0f;
            
        }
        
        if(cameraMode == CAMERA_FIRST_PERSON){
            camera.position.y = playerPosition.y + 1.0f;

            playerPosition.x = camera.position.x;
            playerPosition.z = camera.position.z;
        }
        //camera.position.y = playerPosition.y + 1.0f;
        //playerPosition.x = camera.position.x;
        //playerPosition.z = camera.position.z;

        //shots
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if(cameraMode == CAMERA_FIRST_PERSON){
                Ray ray = GetMouseRay((Vector2){screenWidth / 2, screenHeight / 2}, camera);
            shots += 1;
            
            for (auto& box : redBoxes)
            {
                BoundingBox boxBounds = { (Vector3){ box.position.x - 0.5f, box.position.y - 0.5f, box.position.z - 0.5f },
                                          (Vector3){ box.position.x + 0.5f, box.position.y + 0.5f, box.position.z + 0.5f } };
                if (box.isActive && CheckRayCollisionBox(ray, boxBounds))
                {
                    
                    box.isActive = false;
                    hits += 1;
                    
                    
                    box.position = (Vector3){ GetRandomValue(-10, 10), GetRandomValue(2, 5), GetRandomValue(-10, 10) };
                    box.isActive = true;
                }
            }
            }
            
        }
        if(shots > 0){
            accuracy = (hits/shots) * 100;
        }

        if (IsKeyPressed(KEY_F7)) {
            SaveAccuracy(accuracy);
            ResetGame(shots, hits, accuracy, redBoxes);
        }

        std::vector<float> accuracies = LoadAccuracy();
        
        if(IsKeyPressed(KEY_HOME)){
            if(isshowinggraph){
                bruh = 1;
                
            }
            if(!isshowinggraph){
                isshowinggraph = true;
            }
            
        }
        
        if(bruh == 1){
            isshowinggraph = false;
            bruh = 0;
        }
        BeginDrawing();
            if (aimlabs && !redBoxes.empty()) {
                Box &movingBox = redBoxes[0];
                if (movingBox.isActive) {
                    DrawModel(boxModel, movingBox.position, 1.0f, RED);
                    DrawCubeWires(movingBox.position, 1.0f, 1.0f, 1.0f, DARKPURPLE);
                }
            }
            if(isshowinggraph){
                ClearBackground(RAYWHITE);
                BeginDrawing();
                ClearBackground(RAYWHITE);
                DrawText("Accuracy Graph", 20, 20, 20, BLACK);
                DrawGraph(accuracies, screenWidth, screenHeight);
                EndDrawing();
            }
            //bruh = 1;
            if(!isshowinggraph){
                
                //bruh = 0;
                ClearBackground(RAYWHITE);

                BeginMode3D(camera);

                    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 1000.0f, 1000.0f }, LIGHTGRAY); // Floor
                    DrawCube((Vector3){ -16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, BLUE); // Blue wall
                    DrawCube((Vector3){ 16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, LIME); // Green wall
                    DrawCube((Vector3){ 0.0f, 2.5f, 16.0f }, 32.0f, 5.0f, 1.0f, GOLD); // Yellow wall
                    //the movidng pillar
                    DrawCube(pillarPosition, pillarWidth, pillarHeight, pillarDepth, RED);
                    DrawCubeWires(pillarPosition, pillarWidth, pillarHeight, pillarDepth, MAROON);

                    //player cube
                    //DrawCube(playerPosition, 0.5f, 0.5f, 0.5f, PURPLE);
                    //DrawCubeWires(playerPosition, 0.5f, 0.5f, 0.5f, DARKPURPLE);

                    //floating red boxes
                    for (const auto& box : redBoxes)
                    {
                        if (box.isActive)
                        {
                            DrawModel(boxModel, box.position, 1.0f, WHITE);
                            DrawCubeWires(box.position, 1.0f, 1.0f, 1.0f, DARKPURPLE);
                        }
                    }

                    //debug ray
                    Ray ray = GetMouseRay((Vector2){screenWidth / 2, screenHeight / 2}, camera);
                    DrawRay(ray, GREEN);

                EndMode3D();

                //crosshair
                int centerX = screenWidth / 2;
                int centerY = screenHeight / 2;
                DrawLine(centerX - 10, centerY, centerX + 10, centerY, BLACK);
                DrawLine(centerX, centerY - 10, centerX, centerY + 10, BLACK);

                //info boxes
                DrawText("john romero is my bitch", 15, 5, 20, BLACK);
                DrawText(TextFormat("pos: (%06.3f, %06.3f, %06.3f)", camera.position.x, camera.position.y, camera.position.z), 610, 60, 10, BLACK);
                DrawText(TextFormat("tar: (%06.3f, %06.3f, %06.3f)", camera.target.x, camera.target.y, camera.target.z), 610, 75, 10, BLACK);
                DrawText(TextFormat("up: (%06.3f, %06.3f, %06.3f)", camera.up.x, camera.up.y, camera.up.z), 610, 90, 10, BLACK);
                DrawText(TextFormat("sens: (%06.3f)", actualsens), 15, 45, 10, BLACK);
                DrawText(TextFormat("accuracy: (%06.3f)", accuracy), 15, 55, 10, BLACK);
                DrawText("press ESC to leave", 15, 30, 10, BLACK);
                EndDrawing();
            }
            


    }


    UnloadTexture(boxTexture); //Unload texture
    UnloadModel(boxModel); //Unload modelss
    SaveAccuracy(accuracy);
    CloseWindow();

    return 0;
}