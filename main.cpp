#include <string>
#include <vector>

#include "random.cpp"
#include "raylib.h"

const int kScreenWidth = 600;
const int kScreenHeight = 800;
const float kGameFieldTop = kScreenHeight * 0.25;
const float kGameFieldBottom = kScreenHeight * 0.75;
const int kFPS = 60;

const float kMinBasketRefreshTime = 1;
const float kMaxBasketRefreshTime = 2;
const int kMaxBasketCount = 10;
const float kBasketDropSpeed = 50;
const int kPuffOpenBasketFrames = 9;
const float kPuffOpenBasketDelay = 3;

const int kMaxObjectsCount = 25;
const int kPuffMergeFoodFrames = 5;
const float kPuffMergeFoodDelay = 3;
const std::vector<std::string> kFoodNames = {
    "banana", "onion", "blueberries", "ginger", "raspberry", "cauliflower", "peach", "pumpkin", "pear", "papaya",
    "eggplant", "parsley", "courgette", "peanut", "peas"
};

struct TTextureStorage
{
    Texture2D background;

    Texture2D basket;
    Texture2D puffOpenBasket;

    Texture2D puffMergeFood;
    std::vector<Texture2D> food;
};

struct TBasket
{
    void Draw(Texture2D& texture)
    {
        DrawTexture(texture, x, y, RAYWHITE);
    }

    float x;
    float y;
    float width;
    float height;
    bool isFlying;
};

struct TFood
{
    void Draw(Texture2D texture)
    {
        DrawTexture(texture, x, y, RAYWHITE);
    }

    float x;
    float y;
    float width;
    float height;
    int tier;
    bool isHolding;
    bool isMerging;
};

struct TVisualEffect
{
    void Draw()
    {
        // puff must be x2 larger then object
        Rectangle frameRec;
        int currentFrame = frameCounter / delay;
        frameRec.x = width * currentFrame;
        frameRec.y = 0;
        frameRec.width = width;
        frameRec.height = height;
        DrawTextureRec(animation, frameRec, (Vector2){x, y}, RAYWHITE);
    }

    float x;
    float y;
    float width;
    float height;
    int frameCounter;
    int totalFrames;
    float delay;
    Texture2D& animation;
};

struct TScreenManager
{
    void Init(TTextureStorage* textureStorage)
    {
        textureStoragePtr = std::move(textureStorage);
        basketFramesCounter = 0;
        nextBasketSec = GenerateInt(kMinBasketRefreshTime, kMaxBasketRefreshTime);
        flyingBasket = false;
        basketCount = 0;
        baskets.resize(kMaxBasketCount);
        foodCount = 0;
        isFoodHolding = false;
        isFoodMerging = false;
        maxFoodTier = -1;
        food.resize(kMaxObjectsCount);
    }

    void Update()
    {
        UpdateBaskets();
        UpdateFood();
        UpdateVisualEffects();
    }

    void UpdateFlyingBasket()
    {
        if (flyingBasket) {
            if (baskets[basketCount - 1].y < flyingDistance) {
                baskets[basketCount - 1].y += kBasketDropSpeed;
            } else {
                flyingBasket = false;
                baskets[basketCount - 1].isFlying = false;
            }
        }
        if (!flyingBasket && basketCount < kMaxBasketCount && basketCount + foodCount < kMaxObjectsCount)
        {
            ++basketFramesCounter;
            if (basketFramesCounter > nextBasketSec * kFPS)
            {
                basketFramesCounter = 0;
                nextBasketSec = GenerateInt(kMinBasketRefreshTime, kMaxBasketRefreshTime);
                flyingBasket = true;
                flyingDistance = GenerateFloat(kGameFieldTop, kGameFieldBottom - textureStoragePtr->basket.height);
                baskets[basketCount++] = {
                    GenerateFloat(0, kScreenWidth - textureStoragePtr->basket.width),
                    0,
                    (float)textureStoragePtr->basket.width,
                    (float)textureStoragePtr->basket.height,
                    true
                };
            }
        }
    }

    void UpdateClickOnBasket()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mousePoint = GetMousePosition();
            int clickedOnIndex = -1;
            for (int index = 0; index < basketCount; ++index)
            {
                if (CheckCollisionPointRec(mousePoint, (Rectangle){
                    baskets[index].x,
                    baskets[index].y,
                    baskets[index].width,
                    baskets[index].height}) && !baskets[index].isFlying)
                {
                    clickedOnIndex = index;
                }
            }
            if (clickedOnIndex != -1)
            {
                food[foodCount++] = {
                    baskets[clickedOnIndex].x,
                    baskets[clickedOnIndex].y,
                    (float)textureStoragePtr->food[0].width,
                    (float)textureStoragePtr->food[0].height,
                    0,
                    false,
                    false
                };
                TVisualEffect puff{
                    baskets[clickedOnIndex].x - baskets[clickedOnIndex].width / 2,
                    baskets[clickedOnIndex].y - baskets[clickedOnIndex].height / 2,
                    2.0f * textureStoragePtr->basket.width,
                    2.0f * textureStoragePtr->basket.height,
                    -1,
                    kPuffOpenBasketFrames,
                    kPuffOpenBasketDelay,
                    textureStoragePtr->puffOpenBasket
                };
                visualEffects.push_back(puff);
                std::vector<TBasket> existingBaskets(kMaxBasketCount);
                int existingBasketsCount = 0;
                for (int index = 0; index < basketCount; ++index)
                {
                    if (index != clickedOnIndex)
                    {
                        existingBaskets[existingBasketsCount++] = baskets[index];
                    }
                }
                baskets = existingBaskets;
                basketCount = existingBasketsCount;
            }
        }
    }

    void UpdateBaskets()
    {
        UpdateFlyingBasket();
        UpdateClickOnBasket();
    }

    void UpdateHoldingFood()
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            Vector2 mousePoint = GetMousePosition();
            if (isFoodHolding)
            {
                food[holdingFood].x = std::max(0.0f,
                    std::min(kScreenWidth - food[holdingFood].width, mousePoint.x - food[holdingFood].width / 2));
                food[holdingFood].y = std::max(kGameFieldTop,
                    std::min(kGameFieldBottom - food[holdingFood].height, mousePoint.y - food[holdingFood].height / 2));
            } else {
                int holdingIndex = -1;
                for (int index = 0; index < foodCount; ++index)
                {
                    if (CheckCollisionPointRec(mousePoint, (Rectangle){
                        food[index].x,
                        food[index].y,
                        food[index].width,
                        food[index].height}))
                    {
                        holdingIndex = index;
                    }
                }
                if (holdingIndex != -1)
                {
                    isFoodHolding = true;
                    holdingFood = holdingIndex;
                    food[holdingIndex].isHolding = true;
                }
            }
        } else if (isFoodHolding) {
            isFoodHolding = false;
            food[holdingFood].isHolding = false;
            isFoodMerging = false;
            food[mergingFood].isMerging = false;
        }
    }

    void UpdateMergingFood()
    {
        if (isFoodHolding)
        {
            Vector2 mousePoint = GetMousePosition();
            mousePoint.x = std::max(0.0f, std::min<float>(kScreenWidth, mousePoint.x));
            mousePoint.y = std::max(kGameFieldTop, std::min(kGameFieldBottom, mousePoint.y));
            int mergingIndex = -1;
            for (int index = 0; index < foodCount; ++index)
            {
                if (index != holdingFood && food[index].tier == food[holdingFood].tier && food[holdingFood].tier + 1 < kFoodNames.size() &&
                    CheckCollisionPointRec(mousePoint, (Rectangle){
                        food[index].x - 0.1f,
                        food[index].y - 0.1f,
                        food[index].width + 0.2f,
                        food[index].height + 0.2f}))
                {
                    mergingIndex = index;
                }
            }
            if (mergingIndex != -1)
            {
                isFoodMerging = true;
                mergingFood = mergingIndex;
                food[mergingIndex].isMerging = true;
            } else {
                isFoodMerging = false;
                food[mergingIndex].isMerging = false;
            }
        }
    }

    void DoMergeFood()
    {
        isFoodHolding = false;
        food[holdingFood].isHolding = false;
        isFoodMerging = false;
        food[mergingFood].isMerging = false;
        ++food[holdingFood].tier;
        if (food[holdingFood].tier > maxFoodTier)
        {
            maxFoodTier = food[holdingFood].tier;
        }
        TVisualEffect puff{
            food[holdingFood].x - food[holdingFood].width / 2,
            food[holdingFood].y - food[holdingFood].height / 2,
            2.0f * textureStoragePtr->food[0].width,
            2.0f * textureStoragePtr->food[0].height,
            -1,
            kPuffMergeFoodFrames,
            kPuffMergeFoodDelay,
            textureStoragePtr->puffMergeFood
        };
        visualEffects.push_back(puff);
        std::vector<TFood> existingFood(kMaxObjectsCount);
        int existingFoodCount = 0;
        for (int index = 0; index < foodCount; ++index)
        {
            if (index != mergingFood)
            {
                existingFood[existingFoodCount++] = food[index];
            }
        }
        food = existingFood;
        foodCount = existingFoodCount;
    }

    void UpdateFood()
    {
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isFoodHolding && isFoodMerging)
        {
            DoMergeFood();
        }
        UpdateHoldingFood();
        UpdateMergingFood();
    }

    void UpdateVisualEffects()
    {
        std::vector<TVisualEffect> existingEffects;
        for (auto& effect : visualEffects)
        {
            ++effect.frameCounter;
            if (effect.frameCounter < effect.totalFrames * effect.delay)
            {
                existingEffects.push_back(effect);
            }
        }
        std::swap(visualEffects, existingEffects);
    }

    void DrawBackground()
    {
        DrawTexture(textureStoragePtr->background, 0, 0, RAYWHITE);
    }

    void DrawFood()
    {
        for (int foodIndex = 0; foodIndex < foodCount; ++foodIndex)
        {
            if (!isFoodHolding || foodIndex != holdingFood)
            {
                food[foodIndex].Draw(textureStoragePtr->food[food[foodIndex].tier]);
            }
        }
    }

    void DrawBaskets()
    {
        for (int basketIndex = 0; basketIndex < basketCount; ++basketIndex)
        {
            baskets[basketIndex].Draw(textureStoragePtr->basket);
        }
    }

    void DrawHoldingFood()
    {
        if (isFoodHolding)
        {
            food[holdingFood].Draw(textureStoragePtr->food[food[holdingFood].tier]);
        }
    }

    void DrawVisualEffects()
    {
        for (auto& effect : visualEffects)
        {
            effect.Draw();
        }
    }

    TTextureStorage* textureStoragePtr;
    int basketFramesCounter;
    int nextBasketSec;
    bool flyingBasket;
    int flyingDistance;
    int basketCount;
    std::vector<TBasket> baskets;
    int foodCount;
    bool isFoodHolding;
    int holdingFood;
    bool isFoodMerging;
    int mergingFood;
    int maxFoodTier;
    std::vector<TFood> food;
    std::vector<TVisualEffect> visualEffects;
};

void InitGame(TTextureStorage& textureStorage, TScreenManager& screenManager)
{
    InitWindow(kScreenWidth, kScreenHeight, "Fruits & Vegetables Evolution game");
    SetTargetFPS(kFPS);

    Texture2D background = LoadTexture("assets/background.png");
    textureStorage.background = (background);

    Texture2D basket = LoadTexture("assets/basket.png");
    textureStorage.basket = basket;

    Texture2D puffOpenBasket = LoadTexture("assets/puffOpenBasket.png");
    textureStorage.puffOpenBasket = puffOpenBasket;

    Texture2D puffMergeFood = LoadTexture("assets/puffMergeFood.png");
    textureStorage.puffMergeFood = puffMergeFood;

    textureStorage.food.resize(kFoodNames.size());
    for (int foodIndex = 0; foodIndex < kFoodNames.size(); ++foodIndex)
    {
        std::string filename = "assets/food/food_" + std::to_string(foodIndex) + ".png";
        Texture2D foodTexture = LoadTexture(filename.c_str());
        textureStorage.food[foodIndex] = foodTexture;
    }

    screenManager.Init(&textureStorage);
}

void UpdateGame(TScreenManager& screenManager)
{
    screenManager.Update();
}

void DrawGame(TScreenManager& screenManager)
{
    BeginDrawing();

    screenManager.DrawBackground();
    screenManager.DrawFood();
    screenManager.DrawBaskets();
    screenManager.DrawHoldingFood();
    screenManager.DrawVisualEffects();
    
    EndDrawing();
}

void DeinitGame(TTextureStorage& textureStorage)
{
    UnloadTexture(textureStorage.background);
    UnloadTexture(textureStorage.basket);
    UnloadTexture(textureStorage.puffOpenBasket);
    UnloadTexture(textureStorage.puffMergeFood);
    for (Texture2D texture : textureStorage.food)
    {
        UnloadTexture(texture);
    }

    CloseWindow();
}

int main()
{
    TTextureStorage textureStorage;
    TScreenManager screenManager;
    InitGame(textureStorage, screenManager);

    while (!WindowShouldClose())
    {
        UpdateGame(screenManager);
        DrawGame(screenManager);
    }

    DeinitGame(textureStorage);

    return 0;
}
