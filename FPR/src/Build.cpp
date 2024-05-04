#include <Swift/Core/Application.hpp>
#include <Swift/Entrypoint.hpp>

#include "Core.hpp"

class SwiftFPR : public Swift::Application
{
public:
	SwiftFPR(const Swift::ApplicationSpecification& appInfo)
		: Swift::Application(appInfo)
	{
		AddLayer(new FPRCore());
	}
};



// ----------------------------------------------------------------
//                    Set Application specs here...
// ----------------------------------------------------------------
Swift::Application* Swift::CreateApplication(int argc, char* argv[])
{
	ApplicationSpecification appInfo = {};
	appInfo.WindowSpecs.Name = "SwiftFPR | Initializing...";
	appInfo.WindowSpecs.Width = 1280;
	appInfo.WindowSpecs.Height = 720;
	appInfo.WindowSpecs.VSync = false;

	return new SwiftFPR(appInfo);
}