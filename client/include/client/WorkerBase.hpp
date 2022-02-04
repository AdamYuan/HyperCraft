#ifndef CUBECRAFT3_CLIENT_WORKER_BASE_HPP
#define CUBECRAFT3_CLIENT_WORKER_BASE_HPP

class WorkerBase {
public:
	virtual void Run() = 0;
	virtual ~WorkerBase() = default;
};

#endif
