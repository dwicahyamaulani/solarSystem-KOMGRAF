#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H   

#include "Shader.h"
#include "Sphere.h"
#include "Camera.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <wtypes.h>
#include <ctime>
#include <filesystem>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#define TAU (M_PI * 2.0)

std::vector<glm::vec3> sparkleVertices;
GLuint sparkleVAO, sparkleVBO;




void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
bool keyPressedE = false;
void RenderText(Shader &s, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
unsigned int loadTexture(char const * path);
unsigned int loadCubemap(std::vector<std::string> faces);
void ShowInfo(Shader &s);
glm::vec2 WorldToScreen(glm::vec3 worldPos, glm::mat4 view, glm::mat4 projection);
void GetDesktopResolution(float& horizontal, float& vertical)
{
	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	horizontal = desktop.right;
	vertical = desktop.bottom;
	
}


GLfloat deltaTime = 0.0f;	
GLfloat lastFrame = 0.0f;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool onRotate = false;
bool onFreeCam = true;
bool SkyBoxExtra = false;
float SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

glm::vec3 point = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 PlanetPos = glm::vec3(0.0f, 0.0f, 0.0f);
GLfloat lastX = (GLfloat)(SCREEN_WIDTH / 2.0);
GLfloat lastY = (GLfloat)(SCREEN_HEIGHT / 2.0);
float PlanetSpeed = .1f;
int PlanetView = 0;

glm::vec3 PlanetsPositions[9];
bool PlanetClicked = false;
double mouseX = 0, mouseY = 0;
int SelectedPlanet = -1;

bool resetOrbitCam = false;   // <--- TAMBAHKAN INI DI BAGIAN GLOBAL


int hoveredPlanet = -1;               

bool isMovingToPlanet = false;   
bool isZoomingOut = false;  

glm::vec3 initialCameraPos; 
glm::vec3 initialTargetPos; 
glm::vec3 targetCameraPos;   

bool showPlanetInfo = false;


bool keys[1024];
GLfloat SceneRotateY = 0.0f;
GLfloat SceneRotateX = 0.0f;
bool onPlanet = false;

// ===== ORBIT CAMERA STATE (OrbitControls-style) =====
glm::vec3 orbitTarget = glm::vec3(0.0f, 0.0f, 0.0f); // pusat orbit (Sun di origin)
float orbitRadius = 600.0f;                      // jarak kamera dari Sun
float orbitYaw = glm::radians(90.0f);         // sudut horizontal
float orbitPitch = glm::radians(-40.0f);        // sudut vertikal
bool orbitPanning = false;                       // right-drag = pan

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		//camera.Position = PlanetPos;
		onPlanet = true;
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

bool firstMouse = true;
GLfloat xoff = 0.0f, yoff = 0.0f;

struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};
std::map<GLchar, Character> Characters;
GLuint textVAO, textVBO;
GLuint uiBackgroundTex;

struct PlanetInfo {
	std::string Name;
	std::string OrbitSpeed;
	std::string Mass;
	std::string Gravity;
};
PlanetInfo Info;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos;
	mouseY = ypos;

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = (float)(xpos - lastX);
	float yoffset = (float)(lastY - ypos);

	lastX = xpos;
	lastY = ypos;

	//  FREE CAM (F1)
	if (camera.FreeCam)
	{
		camera.ProcessMouseMovement(xoffset, yoffset);
		return;
	}

	//  PLANET CAM (1–8)
	//  Mouse tidak boleh gerakkan kamera
	if (PlanetView > 0)
	{
		return;
	}

	//  ORBIT CAM (default, PlanetView == 0)
	// Left drag = ROTATE (orbit)
	if (onRotate)
	{
		float rotateSpeed = 0.005f;

		orbitYaw += xoffset * rotateSpeed;
		orbitPitch += yoffset * rotateSpeed;

		float maxPitch = glm::radians(89.0f);
		if (orbitPitch > maxPitch) orbitPitch = maxPitch;
		if (orbitPitch < -maxPitch) orbitPitch = -maxPitch;

		return;
	}

	// Right drag = PAN
	if (orbitPanning)
	{
		float panSpeed = orbitRadius * 0.001f;

		orbitTarget -= xoffset * panSpeed * camera.Right;
		orbitTarget += yoffset * panSpeed * camera.Up;

		return;
	}

	// ==========================
	// HOVER PLANET (ORBIT CAM)
	// ==========================
	if (!camera.FreeCam && PlanetView == 0 && !onRotate && !orbitPanning && !PlanetClicked)
	{
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom),
			(float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
			0.1f, 10000.0f);

		hoveredPlanet = -1;
		float minDist = 99999.0f;
		float hoverRadius = 60.0f; // boleh kamu kecilin/besarin

		for (int i = 0; i < 8; i++)
		{
			glm::vec2 screenPos = WorldToScreen(PlanetsPositions[i], view, proj);

			float dx = (float)mouseX - screenPos.x;
			float dy = (float)mouseY - screenPos.y;
			float dist = sqrtf(dx * dx + dy * dy);

			if (dist < hoverRadius && dist < minDist)
			{
				minDist = dist;
				hoveredPlanet = i;
			}
		}
	}
	else
	{
		// kalau lagi FreeCam / PlanetCam / drag → nggak ada hover
		hoveredPlanet = -1;
	}
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// FREE CAM: maju / mundur sepanjang arah pandang
	if (camera.FreeCam)
	{
		camera.Position += camera.Front * (float)yoffset * 30.0f;
	}
	// ORBIT CAM: zoom (ubah radius orbit)
	else if (PlanetView == 0)
	{
		float zoomSpeed = 30.0f;
		orbitRadius -= (float)yoffset * zoomSpeed; // scroll up → mendekat

		if (orbitRadius < 150.0f)  orbitRadius = 150.0f;
		if (orbitRadius > 3000.0f) orbitRadius = 3000.0f;
	}
	// PlanetCam (PlanetView > 0): sementara scroll diabaikan
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// LEFT MOUSE: ROTATE + CLICK PLANET
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		onRotate = true;

		// Ambil posisi mouse
		double mx, my;
		glfwGetCursorPos(window, &mx, &my);
		mouseX = mx;
		mouseY = my;

		// Ambil matrix kamera
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom),
			(float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
			0.1f, 10000.0f);

		// RESET
		SelectedPlanet = -1;
		float minDist = 99999.0f;

		// --- DETEKSI KLIK PAKAI WORLD → SCREEN ---
		for (int i = 0; i < 8; i++)
		{
			glm::vec2 screenPos = WorldToScreen(PlanetsPositions[i], view, proj);

			float dx = (float)mouseX - screenPos.x;
			float dy = (float)mouseY - screenPos.y;
			float dist = sqrtf(dx * dx + dy * dy);

			float clickableRadius = 50.0f; // makin besar makin gampang diklik

			if (dist < clickableRadius)
			{
				if (dist < minDist)
				{
					minDist = dist;
					SelectedPlanet = i;
				}
			}
		}

		if (SelectedPlanet != -1)
		{
			PlanetClicked = true;
			onRotate = false;
			isMovingToPlanet = true;
			isZoomingOut = false;
			showPlanetInfo = false;
			hoveredPlanet = -1;   // matikan hover saat sudah klik

			// Simpan posisi OrbitCam sebelum zoom (buat balik lagi nanti)
			initialCameraPos = camera.Position;
			initialTargetPos = orbitTarget;   // sekarang orbitTarget = Sun / hasil pan

			// Planet yang diklik jadi pusat baru
			orbitTarget = PlanetsPositions[SelectedPlanet];

			// Arah dari planet ke kamera sekarang
			glm::vec3 dir = camera.Position - orbitTarget;
			float distNow = glm::length(dir);
			if (distNow < 1.0f) dir = glm::vec3(0.0f, 0.0f, 1.0f);
			else               dir = glm::normalize(dir);

			// Jarak yang kamu mau saat “close-up”
			float desiredDist = 150.0f;   // nanti bisa kamu tweak per-planet

			targetCameraPos = orbitTarget + dir * desiredDist;
			orbitRadius = desiredDist;  // supaya nanti muter pakai radius ini juga

			std::cout << "PLANET " << SelectedPlanet << " DIKLIK!" << std::endl;
		}

		if (showPlanetInfo && PlanetClicked && SelectedPlanet != -1)
		{
			// Ambil posisi mouse (sudah ada mx,my)
			double mx = mouseX;
			double my = mouseY;

			float boxX = 20.0f;
			float boxY = SCREEN_HEIGHT - 20.0f;
			float boxW = 420.0f;
			float boxH = 200.0f;

			float closeSize = 24.0f;
			float closeX = boxX + boxW - closeSize - 20.0f;
			float closeY = boxY - 20.0f;

			// area klik X (sedikit toleransi)
			float padding = 10.0f;
			float minX = closeX - padding;
			float maxX = closeX + closeSize + padding;
			float minY = closeY - closeSize - padding;
			float maxY = closeY + padding;

			if (mx >= minX && mx <= maxX && my >= minY && my <= maxY)
			{
				std::cout << "CLICK X\n";

				// Matikan semua mode planet
				PlanetClicked = false;
				SelectedPlanet = -1;
				showPlanetInfo = false;

				// MATIKAN PlanetView sepenuhnya → ini kunci utamanya!!
				PlanetView = 0;

				// Matikan animasi lama
				isMovingToPlanet = false;
				isZoomingOut = false;

				// Aktifkan freecam & orbit mode kembali
				camera.FreeCam = true;

				// Trigger reset orbit
				resetOrbitCam = true;
			}

		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		onRotate = false;
	}

	// RIGHT MOUSE: PAN ORBITCAM
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		orbitPanning = true;

		// sync posisi cursor ke lastX/lastY biar pan awal nggak nyentak
		glfwGetCursorPos(window, &mouseX, &mouseY);
		lastX = mouseX;
		lastY = mouseY;
		firstMouse = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		orbitPanning = false;
	}
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void RenderText(Shader& s, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	s.Use();
	glUniform3f(glGetUniformLocation(s.ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// KOTAK INFO – digambar pakai pipeline text (VAO/VBO + TextShader)
void DrawInfoBackground(Shader& textShader, float x, float y, float w, float h)
{
	glDisable(GL_DEPTH_TEST);

	textShader.Use();
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);

	GLint colorLoc = glGetUniformLocation(textShader.ID, "textColor");

	auto drawQuad = [&](float x0, float y0, float x1, float y1, const glm::vec3& color)
		{
			glUniform3f(colorLoc, color.r, color.g, color.b);

			GLfloat vertices[6][4] = {
				{ x0, y1, 0.0f, 0.0f },
				{ x0, y0, 0.0f, 1.0f },
				{ x1, y0, 1.0f, 1.0f },

				{ x0, y1, 0.0f, 0.0f },
				{ x1, y0, 1.0f, 1.0f },
				{ x1, y1, 1.0f, 0.0f }
			};

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindTexture(GL_TEXTURE_2D, uiBackgroundTex);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		};

	// ==========================
	// 1) OUTLINE — biru lembut
	// ==========================
	float border = 6.0f;       // sedikit lebih tebal
	glm::vec3 outlineColor = glm::vec3(0.15f, 0.55f, 1.0f);   // biru soft modern

	drawQuad(
		x - border,
		y + border,
		x + w + border,
		y - h - border,
		outlineColor
	);

	// ==========================
	// 2) PANEL DALAM — hitam abu elegan
	// ==========================
	glm::vec3 panelColor = glm::vec3(0.03f, 0.03f, 0.05f);  // gelap elegan

	drawQuad(
		x,
		y,
		x + w,
		y - h,
		panelColor
	);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		if (!keyPressedE)
		{
			SkyBoxExtra = !SkyBoxExtra; // toggle starfield <-> blue
			keyPressedE = true;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
	{
		keyPressedE = false;
	}

	//   AKTIFKAN FREE CAM (F1)
// ==============================
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		PlanetView = 0;
		onFreeCam = true;
		camera.FreeCam = true;

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		PlanetView = 0;
		onFreeCam = true;
		camera.FreeCam = false;
		camera.Position = glm::vec3(0.0f, 250.0f, -450.0f);
		camera.Yaw = 90.0f;
		camera.Pitch = -40.0f;
		camera.GetViewMatrix();
		camera.ProcessMouseMovement(xoff, yoff);
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		PlanetView = 1;

		// buka box info
		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 0;  // MERCURY

		onFreeCam = false;
		camera.FreeCam = false;
	}
	
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		PlanetView = 2;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 1;  // VENUS
	}

	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		PlanetView = 3;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 2;  // EARTH
	}

	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
	{
		PlanetView = 4;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 3;  // MARS
	}

	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
	{
		PlanetView = 5;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 4;  // JUPITER
	}

	if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
	{
		PlanetView = 6;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 5;  // SATURN
	}

	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
	{
		PlanetView = 7;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 6;  // URANUS
	}

	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
	{
		PlanetView = 8;

		showPlanetInfo = true;
		PlanetClicked = true;
		SelectedPlanet = 7;  // NEPTUNE
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		PlanetView = 0;
		camera.FreeCam = false;
		onFreeCam = false;
		PlanetClicked = false;
		SelectedPlanet = -1;

		// balik ke default OrbitCam yang sama dengan init
		orbitTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		orbitRadius = 1700.0f;
		orbitYaw = glm::radians(120.0f);
		orbitPitch = glm::radians(25.0f);
	}



}

glm::vec2 WorldToScreen(glm::vec3 worldPos, glm::mat4 view, glm::mat4 projection)
{
	glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0);
	glm::vec3 ndc = glm::vec3(clipSpacePos) / clipSpacePos.w;

	glm::vec2 screen;
	screen.x = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
	screen.y = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;
	return screen;
}

glm::vec3 ScreenPosToWorldRay(double mouseX, double mouseY,
	int screenWidth, int screenHeight,
	glm::mat4 view, glm::mat4 projection)
{
	// Konversi koordinat layar ? NDC
	float x = (2.0f * mouseX) / screenWidth - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / screenHeight;

	glm::vec4 ray_clip = glm::vec4(x, y, -1.0f, 1.0f);

	// NDC ? Eye space
	glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

	// Eye ? World (ray direction)
	glm::vec3 ray_world = glm::normalize(glm::vec3(glm::inverse(view) * ray_eye));

	return ray_world;
}


int main() {
	GetDesktopResolution(SCREEN_WIDTH, SCREEN_HEIGHT); // get resolution for create window
	camera.LookAtPos = point;

	/* GLFW INIT */
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	/* GLFW INIT */

	/* GLFW WINDOW CREATION */
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	/* GLFW WINDOW CREATION */


	/* LOAD GLAD */
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	/* LOAD GLAD */


	/* CONFIGURATION FOR TEXT RENDER */
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	FT_Face face;
	if (FT_New_Face(ft, "fonts/Montserrat-SemiBold.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 52);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	std::cout << "DEBUG TEST 123\n";
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	/* CONFIGURATION FOR TEXT RENDER */

	// === INIT 1x1 WHITE TEXTURE UNTUK PANEL UI ===
	unsigned char whitePixel = 255;
	glGenTextures(1, &uiBackgroundTex);
	glBindTexture(GL_TEXTURE_2D, uiBackgroundTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
		1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	// ===============================================


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* SHADERS */
	Shader SimpleShader("simpleVS.vs", "simpleFS.fs");
	Shader SkyboxShader("skybox.vs", "skybox.fs");
	Shader texShader("simpleVS.vs", "texFS.fs");
	Shader TextShader("TextShader.vs", "TextShader.fs");
	Shader RectShader("UIRect.vs", "UIRect.fs");

	/* SHADERS */

	// PROJECTION FOR TEXT RENDER
		glm::mat4 Text_projection = glm::ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT);
		TextShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(TextShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Text_projection));

	float cube[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	/* SKYBOX GENERATION */
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	/* SKYBOX GENERATION */

	/* VERTEX GENERATION FOR ORBITS */
	std::vector<float> orbVert;
	GLfloat xx;
	GLfloat zz;
	float angl;
	for (int i = 0; i < 2000; i++)
	{
		angl = (float)(M_PI / 2 - i * (M_PI / 1000));
		xx = sin(angl) * 100.0f;
		zz = cos(angl) * 100.0f;
		orbVert.push_back(xx);
		orbVert.push_back(0.0f);
		orbVert.push_back(zz);

	}
	/* VERTEX GENERATION FOR ORBITS */

	/* VAO-VBO for ORBITS*/
	GLuint VBO_t, VAO_t;
	glGenVertexArrays(1, &VAO_t);
	glGenBuffers(1, &VBO_t);
	glBindVertexArray(VAO_t);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_t);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*orbVert.size(), orbVert.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	/* VAO-VBO for ORBITS*/

	// =======================================
//  SPARKLE BELT (BATU-BATU KECIL DI ORBIT)
// =======================================
	int numSparkles = 2000;
	float radiusInner = 120.0f;  // dekat matahari
	float radiusOuter = 400.0f;  // agak luar

	for (int i = 0; i < numSparkles; ++i)
	{
		float angle = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
		float radius = radiusInner + ((float)rand() / RAND_MAX) * (radiusOuter - radiusInner);

		float x = sin(angle) * radius;
		float z = cos(angle) * radius;
		float y = ((float)rand() / RAND_MAX) * 6.0f - 3.0f; // sedikit sebaran vertikal

		sparkleVertices.emplace_back(x, y, z);
	}

	// VAO/VBO untuk sparkle
	glGenVertexArrays(1, &sparkleVAO);
	glGenBuffers(1, &sparkleVBO);
	glBindVertexArray(sparkleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sparkleVBO);
	glBufferData(GL_ARRAY_BUFFER,
		sparkleVertices.size() * sizeof(glm::vec3),
		sparkleVertices.data(),
		GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindVertexArray(0);


	/* TEXT RENDERING VAO-VBO*/
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	/* TEXT RENDERING VAO-VBO*/

	/* LOAD TEXTURES */
	unsigned int texture_earth = loadTexture("resources/planets/earth2k.jpg");
	unsigned int t_sun = loadTexture("resources/planets/2k_sun.jpg");
	unsigned int texture_moon = loadTexture("resources/planets/2k_moon.jpg");
	unsigned int texture_mercury = loadTexture("resources/planets/2k_mercury.jpg");
	unsigned int texture_venus = loadTexture("resources/planets/2k_mercury.jpg");
	unsigned int texture_mars = loadTexture("resources/planets/2k_mars.jpg");
	unsigned int texture_jupiter = loadTexture("resources/planets/2k_jupiter.jpg");
	unsigned int texture_saturn = loadTexture("resources/planets/2k_saturn.jpg");
	unsigned int texture_uranus = loadTexture("resources/planets/2k_uranus.jpg");
	unsigned int texture_neptune = loadTexture("resources/planets/2k_neptune.jpg");
	unsigned int texture_saturn_ring = loadTexture("resources/planets/r.jpg");
	unsigned int texture_earth_clouds = loadTexture("resources/planets/2k_earth_clouds.jpg");
	/* LOAD TEXTURES */

	/* SPHERE GENERATION */
	Sphere Sun(100.0f, 36 * 5, 18 * 5);
	Sphere Mercury(10.0f, 36, 18);
	Sphere Venus(12.0f, 36, 18);
	Sphere Earth(11.8f, 36, 18);
	Sphere Mars(8.0f, 36, 18);
	Sphere Jupiter(40.0f, 36, 18);
	Sphere Saturn(37.0f, 36, 18);
	Sphere Uranus(30.0f, 36, 18);
	Sphere Neptune(30.0f, 36, 19);
	Sphere Moon(5.5f, 36, 18);
	/* SPHERE GENERATION */

	std::vector<std::string> faces
	{
		"resources/skybox/starfield/starfield_rt.tga",
		"resources/skybox/starfield/starfield_lf.tga",
		"resources/skybox/starfield/starfield_up.tga",
		"resources/skybox/starfield/starfield_dn.tga",
		"resources/skybox/starfield/starfield_ft.tga",
		"resources/skybox/starfield/starfield_bk.tga",
	};
	std::vector<std::string> faces_extra
	{
		"resources/skybox/blue/bkg1_right.png",
		"resources/skybox/blue/bkg1_left.png",
		"resources/skybox/blue/bkg1_top.png",
		"resources/skybox/blue/bkg1_bot.png",
		"resources/skybox/blue/bkg1_front.png",
		"resources/skybox/blue/bkg1_back.png",
	};

	unsigned int cubemapTexture = loadCubemap(faces);
	unsigned int cubemapTextureExtra = loadCubemap(faces_extra);
	GLfloat camX = 10.0f;
	GLfloat camZ = 10.0f;
	
	camera.FreeCam = false;
	onFreeCam = false;        // default-nya bukan freecam

	// ===== INITIAL ORBITCAM STATE =====
	orbitTarget = glm::vec3(0.0f, 0.0f, 0.0f);   // pusat di Sun
	orbitRadius = 1700.0f;                       // bisa kamu kecil/besarkan
	orbitYaw = glm::radians(120.0f);          // putaran horizontal
	orbitPitch = glm::radians(25.0f);         // sudut dari atas

	// Hitung posisi kamera dari orbit state ini
	float cosPitch = cos(orbitPitch);
	float sinPitch = sin(orbitPitch);
	float cosYaw = cos(orbitYaw);
	float sinYaw = sin(orbitYaw);

	camera.Position = orbitTarget + glm::vec3(
		orbitRadius * cosPitch * cosYaw,
		orbitRadius * sinPitch,
		orbitRadius * cosPitch * sinYaw
	);

	glm::vec3 front = glm::normalize(orbitTarget - camera.Position);
	camera.Front = front;
	camera.Right = glm::normalize(glm::cross(front, camera.WorldUp));
	camera.Up = glm::normalize(glm::cross(camera.Right, front));

	glm::mat4 view;
	while (!glfwWindowShouldClose(window))
	{
		// UPDATE SIZE WINDOW AKTUAL
		int winW, winH;
		glfwGetWindowSize(window, &winW, &winH);
		SCREEN_WIDTH = (float)winW;
		SCREEN_HEIGHT = (float)winH;

		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glm::mat4 Text_projection = glm::ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT);
		TextShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(TextShader.ID, "projection"),
			1, GL_FALSE, glm::value_ptr(Text_projection));

		/* ZOOM CONTROL */
		if (!camera.FreeCam)
		{
			if (camera.Position.y < 200 && camera.Position.y > 200.0f)
				camera.MovementSpeed = 300.0f;
			if (camera.Position.y < 125.f && camera.Position.y > 70.0f)
				camera.MovementSpeed = 200.0f;
			if (camera.Position.y < 70.f && camera.Position.y > 50.0f)
				camera.MovementSpeed = 100.0f;

			if (camera.Position.y > 200 && camera.Position.y < 400.0f)
				camera.MovementSpeed = 400.0f;
			if (camera.Position.y > 125.f && camera.Position.y < 200.0f)
				camera.MovementSpeed = 300.0f;
			if (camera.Position.y > 70.f && camera.Position.y < 125.0f)
				camera.MovementSpeed = 200.0f;
		}
		/* ZOOM CONTROL */

		processInput(window); // input
		// ===============================
		// RESET ORBITCAM SAAT TEKAN X
		// ===============================
		if (resetOrbitCam)
		{
			orbitTarget = glm::vec3(0.0f, 0.0f, 0.0f);
			orbitRadius = 1700.0f;
			orbitYaw = glm::radians(120.0f);
			orbitPitch = glm::radians(25.0f);

			float cosP = cos(orbitPitch);
			float sinP = sin(orbitPitch);
			float cosY = cos(orbitYaw);
			float sinY = sin(orbitYaw);

			camera.Position = orbitTarget + glm::vec3(
				orbitRadius * cosP * cosY,
				orbitRadius * sinP,
				orbitRadius * cosP * sinY
			);

			camera.LookAtPos = orbitTarget;
			camera.Front = glm::normalize(orbitTarget - camera.Position);
			camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
			camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

			camera.FreeCam = true;

			resetOrbitCam = false;

			std::cout << "RESET OrbitCam DONE\n";
		}


		if (!camera.FreeCam && PlanetView == 0 &&
			PlanetClicked && SelectedPlanet != -1 &&
			!isMovingToPlanet && !isZoomingOut)
		{
			orbitTarget = PlanetsPositions[SelectedPlanet];
		}

		// UPDATE ORBIT CAMERA
		if (!camera.FreeCam && PlanetView == 0 && !isMovingToPlanet && !isZoomingOut)
		{
			// Hitung posisi kamera dari orbit state
			float cosPitch = cos(orbitPitch);
			float sinPitch = sin(orbitPitch);
			float cosYaw = cos(orbitYaw);
			float sinYaw = sin(orbitYaw);

			camera.Position = orbitTarget + glm::vec3(
				orbitRadius * cosPitch * cosYaw,
				orbitRadius * sinPitch,
				orbitRadius * cosPitch * sinYaw
			);

			glm::vec3 front = glm::normalize(orbitTarget - camera.Position);
			camera.Front = front;
			camera.Right = glm::normalize(glm::cross(front, camera.WorldUp));
			camera.Up = glm::normalize(glm::cross(camera.Right, front));
			camera.LookAtPos = orbitTarget;
		}


		if (!onFreeCam)
		{
			SceneRotateY = 0.0f;
			SceneRotateX = 0.0f;
		}
		if (camera.FreeCam || PlanetView > 0)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// render
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!camera.FreeCam && PlanetView == 0 && !PlanetClicked)
		{
			float cosPitch = cos(orbitPitch);
			float sinPitch = sin(orbitPitch);
			float cosYaw = cos(orbitYaw);
			float sinYaw = sin(orbitYaw);

			// Posisi kamera di sphere mengelilingi orbitTarget
			camera.Position = orbitTarget + glm::vec3(
				orbitRadius * cosPitch * cosYaw,
				orbitRadius * sinPitch,
				orbitRadius * cosPitch * sinYaw
			);

			glm::vec3 front = glm::normalize(orbitTarget - camera.Position);
			camera.Front = front;
			camera.Right = glm::normalize(glm::cross(front, camera.WorldUp));
			camera.Up = glm::normalize(glm::cross(camera.Right, front));
		}

		SimpleShader.Use();

		glm::mat4 model = glm::mat4(1.0f);
	
		double viewX;
		double viewZ;
		glm::vec3 viewPos;
		
		SimpleShader.Use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 10000.0f);
		view = camera.GetViewMatrix();
		SimpleShader.setMat4("model", model);
		SimpleShader.setMat4("view", view);
		SimpleShader.setMat4("projection", projection);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t_sun);


		/* SUN */
		glm::mat4 model_sun;
		model_sun = glm::rotate(model_sun, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_sun = glm::rotate(model_sun, (GLfloat)glfwGetTime() * glm::radians(23.5f) * 0.25f, glm::vec3(0.0f, 0.0f, 1.f));
		model_sun = glm::translate(model_sun, point);
		SimpleShader.setMat4("model", model_sun);
		Sun.Draw();
		/* SUN */

		/* MERCURY */
		double xx = sin(glfwGetTime() * PlanetSpeed) * 100.0f * 2.0f * 1.3f;
		double zz = cos(glfwGetTime() * PlanetSpeed) * 100.0f * 2.0f * 1.3f;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_mercury);

		glm::mat4 model_mercury = glm::mat4(1.0f);
		model_mercury = glm::translate(model_mercury, point);
		model_mercury = glm::rotate(model_mercury, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_mercury = glm::rotate(model_mercury, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_mercury = glm::translate(model_mercury, glm::vec3(xx, 0.0f, zz));

		PlanetsPositions[0] = glm::vec3(xx, 0.0f, zz);

		model_mercury = glm::rotate(model_mercury, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_mercury = glm::rotate(model_mercury,
			(GLfloat)glfwGetTime() * glm::radians(-90.0f) * 0.05f,
			glm::vec3(0.0f, 0.0f, 1.f));

		// ========== HOVER HIGHLIGHT ==========
		if (!PlanetClicked && hoveredPlanet == 0)
			model_mercury = glm::scale(model_mercury, glm::vec3(1.2f));

		// NOTE:
		// Tidak pakai lagi "PlanetClicked && SelectedPlanet == 0"
		// karena zoom-in kamera yang menangani interaksi klik.

		SimpleShader.setMat4("model", model_mercury);
		Mercury.Draw();
		/* MERCURY */


		/* VENUS */
		xx = sin(glfwGetTime() * PlanetSpeed *0.75f) * 100.0f * 3.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.75f) * 100.0f * 3.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_venus);
		glm::mat4 model_venus = glm::mat4(1.0f);
		model_venus = glm::translate(model_venus, point);
		model_venus = glm::rotate(model_venus, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_venus = glm::rotate(model_venus, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_venus = glm::translate(model_venus, glm::vec3(xx, 0.0f, zz));
		PlanetsPositions[1] = glm::vec3(xx, 0.0f, zz);
		model_venus = glm::rotate(model_venus, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_venus = glm::rotate(model_venus, glm::radians(-132.5f), glm::vec3(0.0f, 1.0f, 0.f));
		model_venus = glm::rotate(model_venus, (GLfloat)glfwGetTime() * glm::radians(-132.5f) * 0.012f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_venus = glm::scale(model_venus, glm::vec3(1.5f));
		SimpleShader.setMat4("model", model_venus);
		Venus.Draw();
		/* VENUS */

		/* EARTH */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 4.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 4.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_earth);
		glm::mat4 model_earth = glm::mat4(1.0f);
		model_earth = glm::translate(model_earth, point);
		model_earth = glm::rotate(model_earth, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_earth = glm::rotate(model_earth, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_earth = glm::translate(model_earth, glm::vec3(xx, 0.0f, zz));
		glm::vec3 EarthPoint = glm::vec3(xx, 0.0f, zz);
		PlanetsPositions[2] = glm::vec3(xx, 0.0f, zz);
		model_earth = glm::rotate(model_earth, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_earth = glm::rotate(model_earth, glm::radians(-33.25f), glm::vec3(0.0f, 1.0f, 0.f));
		model_earth = glm::rotate(model_earth, (GLfloat)glfwGetTime() * glm::radians(-33.25f) * 2.0f, glm::vec3(0.0f, 0.0f, 1.f));
		camera.LookAtPos = glm::vec3(model_earth[3][0], model_earth[3][1], model_earth[3][2]);
		if (PlanetClicked && SelectedPlanet == 0)
			model_earth = glm::scale(model_earth, glm::vec3(1.5f));

		SimpleShader.setMat4("model", model_earth);
		Earth.Draw();  
		
		/* EARTH */
		
		/* MOON */
		glm::mat4 model_moon;
		xx = sin(glfwGetTime() * PlanetSpeed * 67.55f) * 100.0f * 0.5f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 67.55f) * 100.0f * 0.5f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_moon);
		model_moon = glm::rotate(model_moon, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_moon = glm::rotate(model_moon, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_moon = glm::translate(model_moon, EarthPoint);
		model_moon = glm::translate(model_moon, glm::vec3(xx, 0.0f, zz));
		model_moon = glm::rotate(model_moon, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_moon = glm::rotate(model_moon, glm::radians(-32.4f), glm::vec3(0.0f, 1.0f, 0.f));
		model_moon = glm::rotate(model_moon, (GLfloat)glfwGetTime() * glm::radians(-32.4f) * 3.1f, glm::vec3(0.0f, 0.0f, 1.f));
		SimpleShader.setMat4("model", model_moon);
		Moon.Draw();
		/* MOON */


		/* MARS */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.35f) * 100.0f * 5.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.35f) * 100.0f * 5.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_mars);
		glm::mat4 model_mars = glm::mat4(1.0f);
		model_mars = glm::translate(model_mars, point);
		model_mars = glm::rotate(model_mars, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_mars = glm::rotate(model_mars, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_mars = glm::translate(model_mars, glm::vec3(xx, 0.0f, zz));
		PlanetsPositions[3] = glm::vec3(xx, 0.0f, zz);
		model_mars = glm::rotate(model_mars, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_mars = glm::rotate(model_mars, glm::radians(-32.4f), glm::vec3(0.0f, 1.0f, 0.f));
		model_mars = glm::rotate(model_mars, (GLfloat)glfwGetTime() * glm::radians(-32.4f) * 2.1f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_mars = glm::scale(model_mars, glm::vec3(1.5f));
		SimpleShader.setMat4("model", model_mars);
		Mars.Draw();
		/* MARS */

		/* JUPITER */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.2f) * 100.0f * 6.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.2f) * 100.0f * 6.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_jupiter);
		glm::mat4 model_jupiter = glm::mat4(1.0f);
		model_jupiter = glm::translate(model_jupiter, point);
		model_jupiter = glm::rotate(model_jupiter, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_jupiter = glm::rotate(model_jupiter, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_jupiter = glm::translate(model_jupiter, glm::vec3(xx, 0.0f, zz));
		PlanetsPositions[4] = glm::vec3(xx, 0.0f, zz);
		model_jupiter = glm::rotate(model_jupiter, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_jupiter = glm::rotate(model_jupiter, glm::radians(-23.5f), glm::vec3(0.0f, 1.0f, 0.f));
		model_jupiter = glm::rotate(model_jupiter, (GLfloat)glfwGetTime() * glm::radians(-23.5f) * 4.5f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_jupiter = glm::scale(model_jupiter, glm::vec3(1.5f));
		SimpleShader.setMat4("model", model_jupiter);
		Jupiter.Draw();
		/* JUPITER */

		/* SATURN */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.15f) * 100.0f * 7.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.15f) * 100.0f * 7.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_saturn);
		glm::mat4 model_saturn = glm::mat4(1.0f);
		model_saturn = glm::translate(model_saturn, point);
		model_saturn = glm::rotate(model_saturn, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_saturn = glm::rotate(model_saturn, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_saturn = glm::translate(model_saturn, glm::vec3(xx, 0.0f, zz));
		glm::vec3 SatrunPoint = glm::vec3(xx, 0.0f, zz);
		PlanetsPositions[5] = glm::vec3(xx, 0.0f, zz);
		model_saturn = glm::rotate(model_saturn, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_saturn = glm::rotate(model_saturn, glm::radians(-34.7f), glm::vec3(0.0f, 1.0f, 0.f));
		model_saturn = glm::rotate(model_saturn, (GLfloat)glfwGetTime() * glm::radians(-34.7f) * 4.48f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_saturn = glm::scale(model_saturn, glm::vec3(1.5f));

		SimpleShader.setMat4("model", model_saturn);
		Saturn.Draw();
		/* SATURN */

		/* URANUS */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.1f) * 100.0f * 8.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.1f) * 100.0f * 8.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_uranus);
		glm::mat4 model_uranus = glm::mat4(1.0f);
		model_uranus = glm::translate(model_uranus, point);
		model_uranus = glm::rotate(model_uranus, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_uranus = glm::rotate(model_uranus, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_uranus = glm::translate(model_uranus, glm::vec3(xx, 0.0f, zz));
		PlanetsPositions[6] = glm::vec3(xx, 0.0f, zz);
		model_uranus = glm::rotate(model_uranus, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_uranus = glm::rotate(model_uranus, glm::radians(-99.0f), glm::vec3(0.0f, 1.0f, 0.f));
		model_uranus = glm::rotate(model_uranus, (GLfloat)glfwGetTime() * glm::radians(-99.0f) * 4.5f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_uranus = glm::scale(model_uranus, glm::vec3(1.5f));

		SimpleShader.setMat4("model", model_uranus);
		Uranus.Draw();
		/* URANUS */

		/* NEPTUNE */
		xx = sin(glfwGetTime() * PlanetSpeed * 0.08f) * 100.0f * 9.0f *1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.08f) * 100.0f * 9.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_neptune);

		glm::mat4 model_neptune = glm::mat4(1.0f);
		model_neptune = glm::translate(model_neptune, point);
		model_neptune = glm::rotate(model_neptune, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_neptune = glm::rotate(model_neptune, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_neptune = glm::translate(model_neptune, glm::vec3(xx, 0.0f, zz));
		PlanetsPositions[7] = glm::vec3(xx, 0.0f, zz);
		model_neptune = glm::rotate(model_neptune, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_neptune = glm::rotate(model_neptune, glm::radians(-30.2f), glm::vec3(0.0f, 1.0f, 0.f));
		model_neptune = glm::rotate(model_neptune, (GLfloat)glfwGetTime() * glm::radians(-30.2f) * 4.0f, glm::vec3(0.0f, 0.0f, 1.f));
		if (PlanetClicked && SelectedPlanet == 0)
			model_neptune = glm::scale(model_neptune, glm::vec3(1.5f));

		SimpleShader.setMat4("model", model_neptune);
		Neptune.Draw();
		/* NEPTUNE */

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_venus);

		/* ORBITS */
		glBindVertexArray(VAO_t);
		glLineWidth(1.0f);
		glm::mat4 modelorb;
		for (float i = 2; i < 10; i++)
		{
			modelorb = glm::mat4(1);
			modelorb = glm::translate(modelorb, point);
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
			modelorb = glm::scale(modelorb, glm::vec3(i *1.3f, i*1.3f, i*1.3f));
			SimpleShader.setMat4("model", modelorb);
			glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);

		}
		modelorb = glm::mat4(1);
		modelorb = glm::rotate(modelorb, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		modelorb = glm::rotate(modelorb, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		modelorb = glm::translate(modelorb, EarthPoint);
		modelorb = glm::scale(modelorb, glm::vec3(0.5f *1.3f , 0.5f *1.3f, 0.5f *1.3f));
		SimpleShader.setMat4("model", modelorb);
		glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);
		/* ORBITS */

		// =======================
		// DRAW SPARKLES
		// =======================
		glBindVertexArray(sparkleVAO);
		glPointSize(2.5f);  // besar bintik

		glm::mat4 sm = glm::mat4(1.0f);
		sm = glm::translate(sm, point); // sama pusatnya dengan Sun / orbit
		sm = glm::rotate(sm, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		sm = glm::rotate(sm, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		SimpleShader.setMat4("model", sm);

		glDrawArrays(GL_POINTS, 0, (GLsizei)sparkleVertices.size());
		glBindVertexArray(0);

		// =======================================
		// ANIMASI KAMERA: ZOOM-IN & ZOOM-OUT PLANET
		// =======================================
		if (!camera.FreeCam && PlanetView == 0)   // khusus OrbitCam
		{
			if (isMovingToPlanet && SelectedPlanet != -1)
			{
				// Posisi & target mendekati planet
				camera.Position = glm::mix(camera.Position, targetCameraPos, 0.03f);
				camera.LookAtPos = glm::mix(camera.LookAtPos, PlanetsPositions[SelectedPlanet], 0.1f);

				if (glm::distance(camera.Position, targetCameraPos) < 20.0f)
				{
					isMovingToPlanet = false;
					showPlanetInfo = true;

					// Jadikan planet sebagai pusat orbit lokal
					orbitTarget = PlanetsPositions[SelectedPlanet];
					orbitRadius = glm::distance(camera.Position, orbitTarget);

					// Hitung yaw & pitch dari vektor planet → kamera
					glm::vec3 dir = camera.Position - orbitTarget;
					dir = glm::normalize(dir);

					// yaw: sudut di plane XZ
					orbitYaw = atan2(dir.z, dir.x);
					// pitch: sudut vertikal
					orbitPitch = asin(dir.y);

					// Pastikan LookAtPos tepat ke planet
					camera.LookAtPos = orbitTarget;
				}
			}
			else if (isZoomingOut)
			{
				// Balik ke posisi orbit awal
				camera.Position = glm::mix(camera.Position, initialCameraPos, 0.05f);
				camera.LookAtPos = glm::mix(camera.LookAtPos, initialTargetPos, 0.05f);

				if (glm::distance(camera.Position, initialCameraPos) < 1.0f)
				{
					isZoomingOut = false;
					PlanetClicked = false;
					SelectedPlanet = -1;
					showPlanetInfo = false;
				}
			}
		}

		// =======================================
		// PLANET DESCRIPTION BOX
		// =======================================
		if (showPlanetInfo && PlanetClicked && SelectedPlanet != -1)
		{
			// Panel lebih besar dan agak turun dikit
			float boxW = 660.0f;
			float boxH = 240.0f;
			float boxX = 40.0f;
			float boxY = SCREEN_HEIGHT - 60.0f;

			DrawInfoBackground(TextShader, boxX, boxY, boxW, boxH);

			// Tombol X di pojok kanan atas panel
			float closeSize = 24.0f;
			float closeX = boxX + boxW - closeSize - 20.0f;
			float closeY = boxY - 30.0f;

			RenderText(TextShader, "X", closeX, closeY, 0.8f, glm::vec3(1.0f));

			// Layout teks: title + detail
			float titleX = boxX + 25.0f;
			float titleY = boxY - 40.0f;

			float labelX = boxX + 25.0f;
			float valueX = boxX + 230.0f;
			float rowY = boxY - 90.0f;
			float gap = 38.0f;

			switch (SelectedPlanet)
			{
			case 0: // MERCURY
				RenderText(TextShader, "Merkurius", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Planet paling dekat dengan Matahari",labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Ukurannya kecil dan berbatu", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Warnanya abu-abu seperti batu besar", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Siang hari sangat panas sekali", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 1: // VENUS
				RenderText(TextShader, "Venus", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Planet paling panas di Tata Surya", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Arah putarnya terbalik dibanding planet lain", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Diselimuti awan tebal berwarna kuning", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Permukaannya sangat kering dan berbatu", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Tidak punya bulan", labelX, rowY - gap * 4, 0.45f, glm::vec3(1.0f));
				break;

			case 2: // EARTH
				RenderText(TextShader, "Bumi", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Satu-satunya planet yang memiliki kehidupan", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki air dalam jumlah sangat banyak", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Udara dan suhunya cocok untuk makhluk hidup", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki satu bulan", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 3: // MARS
				RenderText(TextShader, "Mars", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Disebut Planet Merah karena warnanya kemerahan", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki gunung tertinggi di Tata Surya", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Dulunya mungkin punya air", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Punya dua bulan kecil", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 4: // JUPITER
				RenderText(TextShader, "Jupiter", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Planet terbesar di Tata Surya", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki Bintik Merah Raksasa (badai besar)", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Tersusun dari gas tebal", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Punya banyak sekali bulan", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 5: // SATURN
				RenderText(TextShader, "Saturnus", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Terkenal dengan cincin besarnya", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Cincinnya tersusun dari es dan batu kecil", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Merupakan planet gas", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Punya banyak bulan besar dan kecil", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 6: // URANUS
				RenderText(TextShader, "Uranus", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Warnanya biru pucat karena gas metana", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Planet ini miring hampir 90 derajat", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Sangat dingin dan jauh dari Matahari", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki beberapa cincin tipis", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;

			case 7: // NEPTUNE
				RenderText(TextShader, "Neptunus", titleX, titleY, 0.8f, glm::vec3(1.0f));

				RenderText(TextShader, "- Planet berwarna biru gelap", labelX, rowY, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Anginnya paling kencang di Tata Surya", labelX, rowY - gap, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Sangat dingin karena sangat jauh", labelX, rowY - gap * 2, 0.45f, glm::vec3(1.0f));
				RenderText(TextShader, "- Memiliki beberapa bulan", labelX, rowY - gap * 3, 0.45f, glm::vec3(1.0f));
				break;
			}
		}


		/* SATURN RINGS */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_saturn_ring);
		glLineWidth(2.0f);
		GLfloat rr = 0.55f;
		for (int i = 0; i < 25; i++)
		{
			modelorb = glm::mat4(1);
			
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
			modelorb = glm::translate(modelorb, SatrunPoint);
			modelorb = glm::rotate(modelorb, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelorb = glm::scale(modelorb, glm::vec3(rr, rr, rr));
			SimpleShader.setMat4("model", modelorb);
			glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);
			if (i == 15)
				rr += 0.030f;
			else
				rr += 0.01f;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_venus);
		glBindVertexArray(0);
		/* SATURN RINGS */


		/* DRAW SKYBOX */
		glDepthFunc(GL_LEQUAL);  
		SkyboxShader.Use();
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		SkyboxShader.setMat4("view", view);
		SkyboxShader.setMat4("projection", projection);
		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		if (SkyBoxExtra)
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureExtra);
		else
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);  
		/* DRAW SKYBOX */

		glm::vec3 target;          // target planet
		bool planetCamActive = false;
		planetCamActive = false;

		/* PLANET TRACKING + SHOW INFO OF PLANET */
		switch (PlanetView)
		{
		case 1: // MERCURY
		{
			viewX = sin(glfwGetTime() * PlanetSpeed) * 100.0f * 3.5f * 1.3f;
			viewZ = cos(glfwGetTime() * PlanetSpeed) * 100.0f * 3.5f * 1.3f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[0];
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 2: // VENUS
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.75f) * 100.0f * 4.5f * 1.2f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.75f) * 100.0f * 4.5f * 1.2f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[1];
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 3: // EARTH
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 5.5f * 1.2f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 5.5f * 1.2f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[2];
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 4: // MARS
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.35f) * 100.0f * 6.0f * 1.2f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.35f) * 100.0f * 6.0f * 1.2f;
			viewPos = glm::vec3(viewX, 20.0f, viewZ);

			target = PlanetsPositions[3]; // Mars
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 5: // JUPITER
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.2f) * 100.0f * 7.5f * 1.3f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.2f) * 100.0f * 7.5f * 1.3f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[4]; // Jupiter
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 6: // SATURN
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.15f) * 100.0f * 8.5f * 1.3f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.15f) * 100.0f * 8.5f * 1.3f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[5]; // Saturn
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 7: // URANUS
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.1f) * 100.0f * 9.5f * 1.3f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.1f) * 100.0f * 9.5f * 1.3f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[6]; // Uranus
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 8: // NEPTUNE
		{
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.08f) * 100.0f * 10.5f * 1.3f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.08f) * 100.0f * 10.5f * 1.3f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);

			target = PlanetsPositions[7]; // Neptune
			planetCamActive = true;
			ShowInfo(TextShader);
			break;
		}

		case 0:
		{
			float boxW = 480.0f;
			float boxH = 200.0f;
			float boxX = 40.0f;
			float boxY = SCREEN_HEIGHT - 70.0f; // jangan mepet banget ke atas

			DrawInfoBackground(TextShader, boxX, boxY, boxW, boxH);

			// Title
			RenderText(TextShader, "SOLAR SYSTEM INFO",
				boxX + 20, boxY - 40, 0.7f, glm::vec3(0.55f, 0.95f, 1.0f));

			// ---- Layout Settings ----
			float labelX = boxX + 20;
			float valueX = boxX + 200;   // <<< pindahin value lebih ke kanan
			float startY = boxY - 80;
			float gapY = 30.0f;          // <<< jarak antar baris

			// Sun
			RenderText(TextShader, "Sun", labelX, startY, 0.5f, glm::vec3(1.0f));
			RenderText(TextShader, ": 1", valueX, startY, 0.5f, glm::vec3(1.0f));

			// Planets
			RenderText(TextShader, "Planets", labelX, startY - gapY, 0.5f, glm::vec3(1.0f));
			RenderText(TextShader, ": 8", valueX, startY - gapY, 0.5f, glm::vec3(1.0f));

			// Satellites
			RenderText(TextShader, "Satellites", labelX, startY - gapY * 2, 0.5f, glm::vec3(1.0f));
			RenderText(TextShader, ": 415", valueX, startY - gapY * 2, 0.5f, glm::vec3(1.0f));

			// Comets
			RenderText(TextShader, "Comets", labelX, startY - gapY * 3, 0.5f, glm::vec3(1.0f));
			RenderText(TextShader, ": 3441", valueX, startY - gapY * 3, 0.5f, glm::vec3(1.0f));
		}
		break;
			
		}

		if (planetCamActive && PlanetView > 0)
		{
			glm::vec3 front = glm::normalize(target - viewPos);

			camera.Position = viewPos;
			camera.Front = front;
			camera.Right = glm::normalize(glm::cross(front, camera.WorldUp));
			camera.Up = glm::normalize(glm::cross(camera.Right, front));
			camera.FreeCam = false;
		}

		if (PlanetView > 0)
			RenderText(TextShader, "PLANET CAM ", SCREEN_WIDTH - 200.0f, SCREEN_HEIGHT - 30.0f, 0.35f, glm::vec3(0.7f, 0.7f, 0.11f));
		/* PLANET TRACKING + SHOW INFO OF PLANET */


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO_t);
	glDeleteBuffers(1, &VBO_t);
	glfwTerminate();
	return 0;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void ShowInfo(Shader &s)
{
	RenderText(s, "Planet: " + Info.Name, 25.0f, SCREEN_HEIGHT - 30.0f, 0.35f, glm::vec3(0.7f, 0.7f, 0.11f));
	RenderText(s, "Avarage Orbital Speed (km/s): " + Info.OrbitSpeed, 25.0f, SCREEN_HEIGHT - 50.0f, 0.35f, glm::vec3(0.7f, 0.7f, 0.11f));
	RenderText(s, "Mass (kg * 10^24): " + Info.Mass, 25.0f, SCREEN_HEIGHT - 70.0f, 0.35f, glm::vec3(0.7f, 0.7f, 0.11f));
	RenderText(s, "Gravity (g): " + Info.Gravity, 25.0f, SCREEN_HEIGHT - 90.0f, 0.35f, glm::vec3(0.7f, 0.7f, 0.11f));
}