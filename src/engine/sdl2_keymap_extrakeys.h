#ifndef SDL2_KEYMAP_EXTRAKEYS_H_
#define SDL2_KEYMAP_EXTRAKEYS_H_

struct sdl2_keymap{
	int code;
	const char* key;
	sdl2_keymap(int code, const char* key): code(code), key(key) {}
};

static const sdl2_keymap sdl2_keymap_extrakeys[] = {
		//x wheel, sdl2 only
		sdl2_keymap(-35, "WHEELXR"),
		sdl2_keymap(-36, "WHEELXL"),
		//extended keys and modifiers
		sdl2_keymap(0x40000059, "KP1"),
		sdl2_keymap(0x4000005a, "KP2"),
		sdl2_keymap(0x4000005b, "KP3"),
		sdl2_keymap(0x4000005c, "KP4"),
		sdl2_keymap(0x4000005d, "KP5"),
		sdl2_keymap(0x4000005e, "KP6"),
		sdl2_keymap(0x4000005f, "KP7"),
		sdl2_keymap(0x40000060, "KP8"),
		sdl2_keymap(0x40000061, "KP9"),
		sdl2_keymap(0x40000062, "KP0"),
		sdl2_keymap(0x40000063, "KP_PERIOD"),
		sdl2_keymap(0x40000054, "KP_DIVIDE"),
		sdl2_keymap(0x40000055, "KP_MULTIPLY"),
		sdl2_keymap(0x40000056, "KP_MINUS"),
		sdl2_keymap(0x40000057, "KP_PLUS"),
		sdl2_keymap(0x40000058, "KP_ENTER"),
		sdl2_keymap(0x40000067, "KP_EQUALS"),
		sdl2_keymap(0x40000052, "UP"),
		sdl2_keymap(0x40000051, "DOWN"),
		sdl2_keymap(0x4000004f, "RIGHT"),
		sdl2_keymap(0x40000050, "LEFT"),
		sdl2_keymap(0x40000048, "PAUSE"),
		sdl2_keymap(0x40000049, "INSERT"),
		sdl2_keymap(0x4000004a, "HOME"),
		sdl2_keymap(0x4000004d, "END"),
		sdl2_keymap(0x4000004b, "PAGEUP"),
		sdl2_keymap(0x4000004e, "PAGEDOWN"),
		sdl2_keymap(0x4000003a, "F1"),
		sdl2_keymap(0x4000003b, "F2"),
		sdl2_keymap(0x4000003c, "F3"),
		sdl2_keymap(0x4000003d, "F4"),
		sdl2_keymap(0x4000003e, "F5"),
		sdl2_keymap(0x4000003f, "F6"),
		sdl2_keymap(0x40000040, "F7"),
		sdl2_keymap(0x40000041, "F8"),
		sdl2_keymap(0x40000042, "F9"),
		sdl2_keymap(0x40000043, "F10"),
		sdl2_keymap(0x40000044, "F11"),
		sdl2_keymap(0x40000045, "F12"),
		sdl2_keymap(0x40000053, "NUMLOCK"),
		sdl2_keymap(0x40000039, "CAPSLOCK"),
		sdl2_keymap(0x40000047, "SCROLLOCK"),
		sdl2_keymap(0x400000e5, "RSHIFT"),
		sdl2_keymap(0x400000e1, "LSHIFT"),
		sdl2_keymap(0x400000e4, "RCTRL"),
		sdl2_keymap(0x400000e0, "LCTRL"),
		sdl2_keymap(0x400000e6, "RALT"),
		sdl2_keymap(0x400000e2, "LALT"),
		sdl2_keymap(0x400000e7, "RMETA"),
		sdl2_keymap(0x400000e3, "LMETA"),
		sdl2_keymap(0x40000065, "COMPOSE"),
		sdl2_keymap(0x40000075, "HELP"),
		sdl2_keymap(0x40000046, "PRINT"),
		sdl2_keymap(0x4000009a, "SYSREQ"),
		sdl2_keymap(0x40000076, "MENU")
};

#endif /* SDL2_KEYMAP_EXTRAKEYS_H_ */
#ifndef SDL2_KEYMAP_EXTRAKEYS_H_
#define SDL2_KEYMAP_EXTRAKEYS_H_

struct sdl2_keymap{
	int code;
	const char* key;
	sdl2_keymap(int code, const char* key): code(code), key(key) {}
};

static const sdl2_keymap sdl2_keymap_extrakeys[] = {
		//x wheel, sdl2 only
		sdl2_keymap(-35, "WHEELXR"),
		sdl2_keymap(-36, "WHEELXL"),
		//extended keys and modifiers
		sdl2_keymap(0x40000059, "KP1"),
		sdl2_keymap(0x4000005a, "KP2"),
		sdl2_keymap(0x4000005b, "KP3"),
		sdl2_keymap(0x4000005c, "KP4"),
		sdl2_keymap(0x4000005d, "KP5"),
		sdl2_keymap(0x4000005e, "KP6"),
		sdl2_keymap(0x4000005f, "KP7"),
		sdl2_keymap(0x40000060, "KP8"),
		sdl2_keymap(0x40000061, "KP9"),
		sdl2_keymap(0x40000062, "KP0"),
		sdl2_keymap(0x40000063, "KP_PERIOD"),
		sdl2_keymap(0x40000054, "KP_DIVIDE"),
		sdl2_keymap(0x40000055, "KP_MULTIPLY"),
		sdl2_keymap(0x40000056, "KP_MINUS"),
		sdl2_keymap(0x40000057, "KP_PLUS"),
		sdl2_keymap(0x40000058, "KP_ENTER"),
		sdl2_keymap(0x40000067, "KP_EQUALS"),
		sdl2_keymap(0x40000052, "UP"),
		sdl2_keymap(0x40000051, "DOWN"),
		sdl2_keymap(0x4000004f, "RIGHT"),
		sdl2_keymap(0x40000050, "LEFT"),
		sdl2_keymap(0x40000048, "PAUSE"),
		sdl2_keymap(0x40000049, "INSERT"),
		sdl2_keymap(0x4000004a, "HOME"),
		sdl2_keymap(0x4000004d, "END"),
		sdl2_keymap(0x4000004b, "PAGEUP"),
		sdl2_keymap(0x4000004e, "PAGEDOWN"),
		sdl2_keymap(0x4000003a, "F1"),
		sdl2_keymap(0x4000003b, "F2"),
		sdl2_keymap(0x4000003c, "F3"),
		sdl2_keymap(0x4000003d, "F4"),
		sdl2_keymap(0x4000003e, "F5"),
		sdl2_keymap(0x4000003f, "F6"),
		sdl2_keymap(0x40000040, "F7"),
		sdl2_keymap(0x40000041, "F8"),
		sdl2_keymap(0x40000042, "F9"),
		sdl2_keymap(0x40000043, "F10"),
		sdl2_keymap(0x40000044, "F11"),
		sdl2_keymap(0x40000045, "F12"),
		sdl2_keymap(0x40000053, "NUMLOCK"),
		sdl2_keymap(0x40000039, "CAPSLOCK"),
		sdl2_keymap(0x40000047, "SCROLLOCK"),
		sdl2_keymap(0x400000e5, "RSHIFT"),
		sdl2_keymap(0x400000e1, "LSHIFT"),
		sdl2_keymap(0x400000e4, "RCTRL"),
		sdl2_keymap(0x400000e0, "LCTRL"),
		sdl2_keymap(0x400000e6, "RALT"),
		sdl2_keymap(0x400000e2, "LALT"),
		sdl2_keymap(0x400000e7, "RMETA"),
		sdl2_keymap(0x400000e3, "LMETA"),
		sdl2_keymap(0x40000065, "COMPOSE"),
		sdl2_keymap(0x40000075, "HELP"),
		sdl2_keymap(0x40000046, "PRINT"),
		sdl2_keymap(0x4000009a, "SYSREQ"),
		sdl2_keymap(0x40000076, "MENU")
};

#endif /* SDL2_KEYMAP_EXTRAKEYS_H_ */
