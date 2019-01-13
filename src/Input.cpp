// Copyright © 2008-2019 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Input.h"
#include "Pi.h"
#include "GameConfig.h"
#include "ui/Context.h"

void Input::Init()
{
	GameConfig *config = Pi::config;

	joystickEnabled = (config->Int("EnableJoystick")) ? true : false;
	mouseYInvert = (config->Int("InvertMouseY")) ? true : false;

	InitJoysticks();
}

void Input::InitGame()
{
	//reset input states
	keyState.clear();
	keyModState = 0;
	std::fill(mouseButton, mouseButton + COUNTOF(mouseButton), 0);
	std::fill(mouseMotion, mouseMotion + COUNTOF(mouseMotion), 0);
	for (std::map<SDL_JoystickID, JoystickState>::iterator stick = joysticks.begin(); stick != joysticks.end(); ++stick) {
		JoystickState &state = stick->second;
		std::fill(state.buttons.begin(), state.buttons.end(), false);
		std::fill(state.hats.begin(), state.hats.end(), 0);
		std::fill(state.axes.begin(), state.axes.end(), 0.f);
	}
}

KeyBindings::ActionBinding *Input::AddActionBinding(std::string id, BindingGroup *group, KeyBindings::ActionBinding binding)
{
	// TODO: should we throw an error if we attempt to bind over an already-bound action?
	group->bindings[id] = BindingGroup::ENTRY_ACTION;

	// Load from the config
	std::string config_str = Pi::config->String(id.c_str());
	if (config_str.length() > 0) binding.SetFromString(config_str);

	return &(actionBindings[id] = binding);
}

KeyBindings::AxisBinding *Input::AddAxisBinding(std::string id, BindingGroup *group, KeyBindings::AxisBinding binding)
{
	// TODO: should we throw an error if we attempt to bind over an already-bound axis?
	group->bindings[id] = BindingGroup::ENTRY_AXIS;

	// Load from the config
	std::string config_str = Pi::config->String(id.c_str());
	if (config_str.length() > 0) binding.SetFromString(config_str);

	return &(axisBindings[id] = binding);
}

void Input::HandleSDLEvent(SDL_Event &event)
{
	switch (event.type) {
	case SDL_KEYDOWN:
		keyState[event.key.keysym.sym] = true;
		keyModState = event.key.keysym.mod;
		onKeyPress.emit(&event.key.keysym);
		break;
	case SDL_KEYUP:
		keyState[event.key.keysym.sym] = false;
		keyModState = event.key.keysym.mod;
		onKeyRelease.emit(&event.key.keysym);
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button < COUNTOF(mouseButton)) {
			mouseButton[event.button.button] = 1;
			onMouseButtonDown.emit(event.button.button,
				event.button.x, event.button.y);
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if (event.button.button < COUNTOF(mouseButton)) {
			mouseButton[event.button.button] = 0;
			onMouseButtonUp.emit(event.button.button,
				event.button.x, event.button.y);
		}
		break;
	case SDL_MOUSEWHEEL:
		onMouseWheel.emit(event.wheel.y > 0); // true = up
		break;
	case SDL_MOUSEMOTION:
		mouseMotion[0] += event.motion.xrel;
		mouseMotion[1] += event.motion.yrel;
		break;
	case SDL_JOYAXISMOTION:
		if (!joysticks[event.jaxis.which].joystick)
			break;
		if (event.jaxis.value == -32768)
			joysticks[event.jaxis.which].axes[event.jaxis.axis] = 1.f;
		else
			joysticks[event.jaxis.which].axes[event.jaxis.axis] = -event.jaxis.value / 32767.f;
		break;
	case SDL_JOYBUTTONUP:
	case SDL_JOYBUTTONDOWN:
		if (!joysticks[event.jaxis.which].joystick)
			break;
		joysticks[event.jbutton.which].buttons[event.jbutton.button] = event.jbutton.state != 0;
		break;
	case SDL_JOYHATMOTION:
		if (!joysticks[event.jaxis.which].joystick)
			break;
		joysticks[event.jhat.which].hats[event.jhat.hat] = event.jhat.value;
		break;
	}
}

void Input::InitJoysticks()
{
	int joy_count = SDL_NumJoysticks();
	for (int n = 0; n < joy_count; n++) {
		JoystickState state;

		state.joystick = SDL_JoystickOpen(n);
		if (!state.joystick) {
			Output("SDL_JoystickOpen(%i): %s\n", n, SDL_GetError());
			continue;
		}
		state.guid = SDL_JoystickGetGUID(state.joystick);
		state.axes.resize(SDL_JoystickNumAxes(state.joystick));
		state.buttons.resize(SDL_JoystickNumButtons(state.joystick));
		state.hats.resize(SDL_JoystickNumHats(state.joystick));

		SDL_JoystickID joyID = SDL_JoystickInstanceID(state.joystick);
		joysticks[joyID] = state;
	}
}

std::string Input::JoystickName(int joystick)
{
	return std::string(SDL_JoystickName(joysticks[joystick].joystick));
}

std::string Input::JoystickGUIDString(int joystick)
{
	const int guidBufferLen = 33; // as documented by SDL
	char guidBuffer[guidBufferLen];

	SDL_JoystickGetGUIDString(joysticks[joystick].guid, guidBuffer, guidBufferLen);
	return std::string(guidBuffer);
}

// conveniance version of JoystickFromGUID below that handles the string mangling.
int Input::JoystickFromGUIDString(const std::string &guid)
{
	return JoystickFromGUIDString(guid.c_str());
}

// conveniance version of JoystickFromGUID below that handles the string mangling.
int Input::JoystickFromGUIDString(const char *guid)
{
	return JoystickFromGUID(SDL_JoystickGetGUIDFromString(guid));
}

// return the internal ID of the stated joystick guid.
// returns -1 if we couldn't find the joystick in question.
int Input::JoystickFromGUID(SDL_JoystickGUID guid)
{
	const int guidLength = 16; // as defined
	for (std::map<SDL_JoystickID, JoystickState>::iterator stick = joysticks.begin(); stick != joysticks.end(); ++stick) {
		JoystickState &state = stick->second;
		if (0 == memcmp(state.guid.data, guid.data, guidLength)) {
			return static_cast<int>(stick->first);
		}
	}
	return -1;
}

SDL_JoystickGUID Input::JoystickGUID(int joystick)
{
	return joysticks[joystick].guid;
}

int Input::JoystickButtonState(int joystick, int button)
{
	if (!joystickEnabled) return 0;
	if (joystick < 0 || joystick >= int(joysticks.size()))
		return 0;

	if (button < 0 || button >= int(joysticks[joystick].buttons.size()))
		return 0;

	return joysticks[joystick].buttons[button];
}

int Input::JoystickHatState(int joystick, int hat)
{
	if (!joystickEnabled) return 0;
	if (joystick < 0 || joystick >= int(joysticks.size()))
		return 0;

	if (hat < 0 || hat >= int(joysticks[joystick].hats.size()))
		return 0;

	return joysticks[joystick].hats[hat];
}

float Input::JoystickAxisState(int joystick, int axis)
{
	if (!joystickEnabled) return 0;
	if (joystick < 0 || joystick >= int(joysticks.size()))
		return 0;

	if (axis < 0 || axis >= int(joysticks[joystick].axes.size()))
		return 0;

	return joysticks[joystick].axes[axis];
}
