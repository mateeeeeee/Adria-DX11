#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Logging/Logger.h"
#include "Editor/Editor.h"
#include "Utilities/MemoryDebugger.h"
#include "Utilities/CLIParser.h"

using namespace adria;

int MemoryAllocHook(int allocType, void* userData, std::size_t size, int blockType, long requestNumber,
	const unsigned char* filename, int lineNumber)
{
	return 1;
}

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
	CLIParser parser{};
	CLIArg& width = parser.AddArg(true, "-w", "--width");
	CLIArg& height = parser.AddArg(true, "-h", "--height");
	CLIArg& title = parser.AddArg(true, "-title");
	CLIArg& config = parser.AddArg(true, "-cfg", "--config");
	CLIArg& scene = parser.AddArg(true, "-scene", "--scenefile");
	CLIArg& log = parser.AddArg(true, "-log", "--logfile");
	CLIArg& loglevel = parser.AddArg(true, "-loglvl", "--loglevel");
	CLIArg& maximize = parser.AddArg(false, "-max", "--maximize");
	CLIArg& vsync = parser.AddArg(false, "-vsync");
	//MemoryDebugger::SetAllocHook(MemoryAllocHook);
    //MemoryDebugger::SetBreak(275);
	//MemoryDebugger::Checkpoint();
	parser.Parse(lpCmdLine);
    {
		std::string log_file = log.AsStringOr("adria.log");
		int32 log_level = loglevel.AsIntOr(0);
		ADRIA_REGISTER_LOGGER(new FileLogger(log_file.c_str(), static_cast<ELogLevel>(log_level)));
		ADRIA_REGISTER_LOGGER(new OutputDebugStringLogger(static_cast<ELogLevel>(log_level)));

		WindowInit window_init{};
		window_init.instance = hInstance;
		window_init.width = width.AsIntOr(1080);
		window_init.height = height.AsIntOr(720);
		window_init.title = title.AsStringOr("Adria");
		window_init.maximize = maximize;
		Window::Initialize(window_init);

        EngineInit engine_init{};
        engine_init.vsync = vsync;
        engine_init.scene_file = scene.AsStringOr("scene.json");

        EditorInit editor_init{};
        editor_init.engine_init = std::move(engine_init);

        Editor editor{ editor_init };
        Window::SetCallback([&editor](WindowMessage const& msg_data) {editor.HandleWindowMessage(msg_data); });

        while (Window::Loop())
        {
            editor.Run();
        }
        Window::Destroy();
    }
    //MemoryDebugger::CheckLeaks();
}


