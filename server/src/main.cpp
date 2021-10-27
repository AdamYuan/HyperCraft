#include <concurrentqueue.h>
#include <spdlog/spdlog.h>

int main() {
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
	spdlog::info("This is a server");
	return 0;
}
