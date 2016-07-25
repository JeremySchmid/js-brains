#if !defined(BASE_PLATFORM_H)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h> //implement sin and sqrt ourselves...

typedef int boolint;

typedef size_t memory_index;


typedef struct thread_context
{
	int Placeholder;
} thread_context;

int AsciiToIndex (int Character)
{
	return (Character - 'A');
}

//NOTE: Services that the platform layer provides to the game


#if INTERNAL_BUILD
//note: These are NOT for doing anything in the shipping game - they are blocking and the write doesn't protect against lost data!
//
typedef struct debug_read_file_result
{
	void *Contents;
	uint32_t ContentSize;

} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) boolint name(thread_context* Thread, char* Filename, uint64_t MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile);
#endif


//TODO: Services that the game provides to the platform layer
//May expand in the future - sound on separate thread, etc

//FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

//TODO: in the future, rendering _specifically_ will become a three-tiered abstraction
typedef struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
	
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
	
} game_sound_output_buffer;

typedef struct game_button_state
{
	int HalfTransitionCount;
	boolint EndedDown;
	
} game_button_state;

typedef struct average_analog
{
	float Average;
	int NumberSoFar;
	
} average_analog;

typedef struct keyboard_input
{

	union {
		game_button_state Keys[28];
		struct {
			game_button_state Alphabet[26];
			game_button_state Escape;
			game_button_state Space;
		};
	};

} keyboard_input;

typedef struct game_controller_input
{
	boolint IsConnected;
	boolint IsAnalog;
	
	union {
		average_analog Analogs[6];
		struct {
			average_analog LT;
			average_analog RT;
			average_analog LX;
			average_analog LY;
			average_analog RX;
			average_analog RY;
		};
	};
	
	union {
		game_button_state Buttons[24];
		struct {
			//OnOff for the analogs
			game_button_state LeftTrigger;
			
			game_button_state RightTrigger;
			
			game_button_state LeftStickRight;
			game_button_state LeftStickLeft;
			
			game_button_state LeftStickUp;
			game_button_state LeftStickDown;
			
			game_button_state RightStickRight;
			game_button_state RightStickLeft;
			
			game_button_state RightStickUp;
			game_button_state RightStickDown;
			
			game_button_state DpadUp;
			game_button_state DpadDown;
			game_button_state DpadLeft;
			game_button_state DpadRight;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
			game_button_state LeftAnalogPress;
			game_button_state RightAnalogPress;
			game_button_state AButton;
			game_button_state BButton;
			game_button_state XButton;
			game_button_state YButton;
			game_button_state Back;
			game_button_state Start;
			
			#if SLOW_CODE
			//do not use - for debug purposes!!
			//must be the last button - not included in the union array count
			game_button_state Terminator;
			#endif
		};
	};
	
} game_controller_input;

typedef struct game_input
{

	game_button_state MouseButtons[5];
	int MouseX, MouseY, MouseZ;

	game_controller_input Controllers[5];

	keyboard_input KeyboardInput;
	
	float dtForFrame;
	//Insert clock values here
	//TODO: what do we want to pass here?
	//TODO(Jeremy): use a fixed point of 64.64? floats are inaccurate at high numbers and people may leave the program running - http://home.comcast.net/~tom_forsyth/blog.wiki.html#[[A%20matter%20of%20precision]]
	//float SecondsElapsed;
} game_input;

typedef struct game_memory
{
	boolint IsInitialized;
	
	uint64_t PermanentStorageSize;
	void *PermanentStorage; //Note: REQUIRED to be cleared to zero at startup
	
	uint64_t TransientStorageSize;
	void *TransientStorage; //Note: REQUIRED to be cleared to zero at startup

	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
	debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
	
} game_memory;

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define GAME_UPDATE(name) void name(thread_context* Thread, game_memory* Memory, game_input *Input)
typedef GAME_UPDATE(game_update);

#define GAME_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_offscreen_buffer* Buffer)
typedef GAME_RENDER(game_render);

#ifdef __cplusplus
}
#endif


#define BASE_PLATFORM_H
#endif
