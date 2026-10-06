/**
* Author: Lewis Zhao
* Assignment: Simple 2D Scene
* Date due: [10/05/2026]
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// A top-down view of the Outer Wilds solar system that plays one full 22 minutes time loop in about 70 seconds
// Icons: https://steamcommunity.com/sharedfiles/filedetails/?id=2633399791
// Credit: Mobius Digital

// Warning: This program contains spoilers for Outer Wilds. Do not run it if you want the best possible experience.

#include "CS3113/cs3113.h"
#include <math.h>
#include <cstdlib>

// Enums
enum Body {
    INTERLOPER, SUN_STATION, SUN,
    SAND_COLUMN, ASH_TWIN, EMBER_TWIN,
    TIMBER_HEARTH, ATTLEROCK,
    BRITTLE_HOLLOW, HOLLOWS_LANTERN,
    GIANTS_DEEP, DARK_BRAMBLE,
    BODY_COUNT
};

// Global Constants
constexpr int SCREEN_WIDTH  = 1440,
              SCREEN_HEIGHT = 1080,
              FPS           = 60;

constexpr int STAR_COUNT = 170;
constexpr float DEGREES_TO_RADIANS = 3.14159265f / 180.0f;

constexpr Vector2 ORIGIN = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

// Time in game seconds
constexpr float PLAYBACK_SPEED = 20.0f;
constexpr float MINUTE = 60.0f;
constexpr float SUPERNOVA_TIME = 22.0f * MINUTE;
constexpr float LOOP_DURATION = SUPERNOVA_TIME + 100.0f;

constexpr float TIDALLY_LOCKED = 0.0f;

// The Interloper's orbit
constexpr float INTERLOPER_DISTANCE = 352.0f;
constexpr float INTERLOPER_ECCENTRICITY = 0.79f;
constexpr float INTERLOPER_TILT = 15.0f;

// Colors
const Color BACKGROUND_BLACK = ColorFromHex("#010104");
const Color BACKGROUND_RED = ColorFromHex("#130408");
const Color BACKGROUND_BLUE = ColorFromHex("#070E24");
const Color SCREEN_BLACK = ColorFromHex("#000000");
const Color SUN_RED = ColorFromHex("#FF9673");
const Color SUN_BLUE = ColorFromHex("#6EAAFF");
const Color BLAST_RIM = ColorFromHex("#BEE4FF");
const Color BLAST_FILL = ColorFromHex("#2B4471");
const Color ORBIT_COLOR = ColorFromHex("#9FB2D9");
const Color STAR_COLOR = ColorFromHex("#E1E8FF");
const Color STAR_FLARE = ColorFromHex("#FFF5DC");

// Global Variables
AppStatus gAppStatus = RUNNING;
float gPreviousTicks = 0.0f;
float gTime = 0.0f;

Texture2D gTextures[BODY_COUNT];
Vector2 gPositions[BODY_COUNT];
Vector2 gSizes[BODY_COUNT];
float gScales[BODY_COUNT];
float gAngles[BODY_COUNT];
Color gTints[BODY_COUNT];

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

// A moment in the loop
float at(float minutes, float seconds) {
    return minutes * MINUTE + seconds;
}

float mix(float from, float to, float amount) {
    return from + (to - from) * amount;
}

float progress(float start, float end) {
    float t = fminf(fmaxf((gTime - start) / (end - start), 0.0f), 1.0f);
    return t;
}

Color mixColors(Color from, Color to, float amount) {
    Color result;
    result.r = static_cast<unsigned char>(mix(from.r, to.r, amount));
    result.g = static_cast<unsigned char>(mix(from.g, to.g, amount));
    result.b = static_cast<unsigned char>(mix(from.b, to.b, amount));
    result.a = static_cast<unsigned char>(mix(from.a, to.a, amount));
    return result;
}

Color withOpacity(Color color, float opacity) {
    color.a = static_cast<unsigned char>(255.0f * opacity);
    return color;
}

float redGiant() {
    return progress(at(10, 0), at(12, 0));
}

float collapse() {
    return progress(at(21, 30), at(21, 57));
}

float whiteOut() {
    return progress(at(22, 35), at(23, 0));
}

Color backgroundColor() {
    // supernova
    if (gTime > SUPERNOVA_TIME) {
        return mixColors(BACKGROUND_BLUE, WHITE, whiteOut());
    }

    // Deep black turning red with the red giant and back to black as the Sun collapses
    return mixColors(BACKGROUND_BLACK, BACKGROUND_RED, redGiant() * (1.0f - collapse()));
}

void loadBody(int body, const char *file, float width) {
    gTextures[body] = LoadTexture(file);
    gSizes[body] = { width, width * gTextures[body].height / gTextures[body].width };
    gScales[body] = 1.0f;
    gTints[body] = WHITE;
}

Vector2 pointOnOrbit(Vector2 center, float distance, float eccentricity, float tilt, float E) {
    float along = distance * (cos(E) - eccentricity);
    float across = distance * sqrtf(1.0f - eccentricity * eccentricity) * sin(E);
    float tiltX = cos(tilt * DEGREES_TO_RADIANS);
    float tiltY = sin(tilt * DEGREES_TO_RADIANS);

    Vector2 point;
    point.x = center.x + along * tiltX - across * tiltY;
    point.y = center.y - (along * tiltY + across * tiltX);
    return point;
}

void orbit(int body, Vector2 center, float distance, float lapTime, float startAngle, float spinTime, float eccentricity = 0.0f, float tilt = 0.0f) {
    float M = (startAngle + 360.0f * gTime / lapTime) * DEGREES_TO_RADIANS;
    float E = M;
    for (int i = 0; i < 50; i++) {
        E = M + eccentricity * sin(E);
    }

    Vector2 position = pointOnOrbit(center, distance, eccentricity, tilt, E);
    gPositions[body] = position;

    if (spinTime == TIDALLY_LOCKED) {
        gAngles[body] = atan2f(position.y - center.y, position.x - center.x) / DEGREES_TO_RADIANS;
    } else {
        gAngles[body] = 360.0f * gTime / spinTime;
    }
}

void drawBody(int body) {
    Vector2 size = { gSizes[body].x * gScales[body], gSizes[body].y * gScales[body] };

    Rectangle textureArea = {
        0.0f, 0.0f,
        static_cast<float>(gTextures[body].width),
        static_cast<float>(gTextures[body].height)
    };

    Rectangle destinationArea = {
        gPositions[body].x,
        gPositions[body].y,
        size.x,
        size.y
    };

    Vector2 objectOrigin = {
        size.x / 2.0f,
        size.y / 2.0f
    };

    DrawTexturePro(
        gTextures[body],
        textureArea,
        destinationArea,
        objectOrigin,
        gAngles[body],
        gTints[body]
    );
}

void drawRing(Vector2 center, float radius, float width, Color ring, Color inside) {
    DrawPoly(center, 360, radius + width / 2.0f, 0.0f, ring);
    DrawPoly(center, 360, radius - width / 2.0f, 0.0f, inside);
}

void drawOrbitLines() {
    Color background = backgroundColor();
    Color line = mixColors(background, ORBIT_COLOR, 0.3f * (1.0f - collapse()));
    int planets[] = { DARK_BRAMBLE, GIANTS_DEEP, BRITTLE_HOLLOW, TIMBER_HEARTH, SAND_COLUMN };

    for (int planet : planets) {
        float radius = hypotf(gPositions[planet].x - ORIGIN.x, gPositions[planet].y - ORIGIN.y);
        drawRing(ORIGIN, radius, 1.0f, line, background);
    }

    if (gTime < at(20, 0)) {
        for (float E = 0.0f; E < 360.0f * DEGREES_TO_RADIANS; E += 1.0f / INTERLOPER_DISTANCE) {
            Vector2 point = pointOnOrbit(ORIGIN, INTERLOPER_DISTANCE, INTERLOPER_ECCENTRICITY, INTERLOPER_TILT, E);
            DrawPoly(point, 4, 0.7071f, 45.0f, line);
        }
    }
}

void drawStars() {
    Color background = backgroundColor();

    srand(1);
    for (int i = 0; i < STAR_COUNT; i++) {
        Vector2 position = { static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT) };
        float brightness = 0.3f + 0.7f * rand() / RAND_MAX;
        float explosionTime = MINUTE + rand() % static_cast<int>(24.0f * MINUTE);
        float t = progress(explosionTime, explosionTime + 20.0f);

        if (gTime < explosionTime) {
            DrawPoly(position, 4, 1.4142f, 45.0f, withOpacity(STAR_COLOR, brightness));
        } else if (t < 1.0f) {
            Color ring = mixColors(background, STAR_FLARE, 1.0f - t);
            Color inside = mixColors(background, STAR_FLARE, 0.4f * (1.0f - t));
            drawRing(position, 10.0f * t, 2.0f, ring, inside);
        }
    }
}

// Function Definitions
void initialise() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Outer Wilds 22 Minutes Loop");

    loadBody(INTERLOPER, "assets/game/interloper.png", 44);
    loadBody(SUN_STATION, "assets/game/sun_station.png", 33);
    loadBody(SUN, "assets/game/sun.png", 74);
    loadBody(SAND_COLUMN, "assets/game/sand_column.png", 48);
    loadBody(ASH_TWIN, "assets/game/ash_twin.png", 34);
    loadBody(EMBER_TWIN, "assets/game/ember_twin.png", 34);
    loadBody(TIMBER_HEARTH, "assets/game/timber_hearth.png", 56);
    loadBody(ATTLEROCK, "assets/game/attlerock.png", 20);
    loadBody(BRITTLE_HOLLOW, "assets/game/brittle_hollow.png", 52);
    loadBody(HOLLOWS_LANTERN, "assets/game/hollows_lantern.png", 26);
    loadBody(GIANTS_DEEP, "assets/game/giants_deep.png", 74);
    loadBody(DARK_BRAMBLE, "assets/game/dark_bramble.png", 58);

    SetTargetFPS(FPS);
}

void processInput() {
    if (WindowShouldClose()) {
        gAppStatus = TERMINATED;
    }
}

void update() {
    float ticks = static_cast<float>(GetTime());
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;

    gTime += PLAYBACK_SPEED * deltaTime;
    if (gTime >= LOOP_DURATION) {
        gTime -= LOOP_DURATION;
    }

    // The solar system
    orbit(SUN, ORIGIN, 5, 180, 0, 240);
    orbit(SUN_STATION, gPositions[SUN], 60, 60, 0, TIDALLY_LOCKED);
    orbit(SAND_COLUMN, ORIGIN, 133, 111, 30, -75); // the twins' center of mass
    orbit(ASH_TWIN, gPositions[SAND_COLUMN], 24, 75, 0, TIDALLY_LOCKED);
    orbit(EMBER_TWIN, gPositions[SAND_COLUMN], 24, 75, 180, TIDALLY_LOCKED);
    orbit(TIMBER_HEARTH, ORIGIN, 231, 250, 200, 120);
    orbit(ATTLEROCK, gPositions[TIMBER_HEARTH], 42, 100, 90, TIDALLY_LOCKED);
    orbit(BRITTLE_HOLLOW, ORIGIN, 342, 397, 120, 90);
    orbit(HOLLOWS_LANTERN, gPositions[BRITTLE_HOLLOW], 42, 150, 300, -40);
    orbit(GIANTS_DEEP, ORIGIN, 437, 664, 300, 180);
    orbit(DARK_BRAMBLE, ORIGIN, 507, 887, 70, -300);

    float fall = progress(at(19, 30), at(20, 0));
    float interloperDistance = INTERLOPER_DISTANCE * mix(1.0f, 0.4f, fall);
    orbit(INTERLOPER, ORIGIN, interloperDistance, 11.0f * MINUTE, 65.45f, TIDALLY_LOCKED,
        INTERLOPER_ECCENTRICITY, INTERLOPER_TILT);
    gScales[INTERLOPER] = 1.0f - fall;

    // The sun swells into a red giant then collapses
    gScales[SUN] = mix(mix(1.0f, 2.2f, redGiant()), 0.15f, collapse());
    gTints[SUN] = mixColors(WHITE, SUN_RED, redGiant());

    // red giant swallows the sun station
    gTints[SUN_STATION] = withOpacity(WHITE, 1.0f - progress(at(10, 50), at(11, 5)));
}

void render() {
    BeginDrawing();

    ClearBackground(backgroundColor());

    drawOrbitLines();
    drawStars();

    for (int body = 0; body < BODY_COUNT; body++) {
        drawBody(body);
    }

    // The collapsing sun turns blue
    float sunRadius = gSizes[SUN].x * gScales[SUN] / 2.0f;
    DrawPoly(gPositions[SUN], 360, sunRadius, 0.0f, withOpacity(SUN_BLUE, fminf(3.0f * collapse(), 1.0f)));

    // Supernova
    if (gTime > SUPERNOVA_TIME) {
        float blastRadius = 1300.0f * progress(SUPERNOVA_TIME, at(23, 0));
        Color rim = mixColors(BLAST_RIM, WHITE, whiteOut());
        Color fill = mixColors(BLAST_FILL, WHITE, whiteOut());
        drawRing(gPositions[SUN], blastRadius, 6.0f, rim, fill);
    }

    // between loops
    float black;
    if (gTime > SUPERNOVA_TIME) {
        black = progress(at(23, 0), LOOP_DURATION);
    } else {
        black = 1.0f - progress(0.0f, 30.0f);
    }
    DrawPoly(ORIGIN, 4, 1300.0f, 45.0f, withOpacity(SCREEN_BLACK, black));

    EndDrawing();
}

void shutdown() {
    for (int body = 0; body < BODY_COUNT; body++) {
        UnloadTexture(gTextures[body]);
    }

    CloseWindow();
}

int main(void) {
    initialise();

    while (gAppStatus == RUNNING) {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}
