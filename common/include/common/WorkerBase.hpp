#pragma once

namespace hc {

class WorkerBase {
public:
	virtual void Run() = 0;
	virtual ~WorkerBase() = default;
};

} // namespace hc