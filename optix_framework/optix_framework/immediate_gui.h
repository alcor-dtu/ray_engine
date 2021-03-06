#pragma once
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <algorithm>
namespace ImmediateGUIDraw = ImGui;

namespace ImGui
{
	inline bool InputString(const char * name, std::string & str, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL)
	{
		std::vector<char> data(str.begin(), str.end());
		data.resize(256, '\0');
		if (ImmediateGUIDraw::InputText(name, &data[0], 256, flags, callback, user_data))
		{
			str = std::string(data.data());			
			return true;
		}
		return false;
	}
}

struct GLFWwindow;

class ImmediateGUI
{
public:

	ImmediateGUI(GLFWwindow * window = nullptr);
	virtual ~ImmediateGUI();

	bool keyPressed(int key, int action, int modifier);
	// Use this to add additional keys. Some are already handled but
	// can be overridden.  Should return true if key was handled, false otherwise.
	bool mousePressed(int x, int y, int button, int action, int mods);
	bool mouseMoving(int x, int y);

	void start_window(const char * name, int x, int y, int w, int h);
	void end_window();

	void toggleVisibility() { visible = !visible; }
	bool isVisible() const
	{
		return visible;
	}	

private:
	bool visible = true;
	GLFWwindow * win;
	GLFWwindow * context_win = nullptr;
};
