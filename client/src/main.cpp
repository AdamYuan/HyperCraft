#include <client/Application.hpp>
#include <spdlog/spdlog.h>

int main() {
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
	Application app{};
	app.Run();
	return 0;
}
