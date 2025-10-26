#pragma once

namespace kernel
{

class Thread;

class Scheduler
{
public:
    Scheduler() = default;

    void addTask();
    void removeTask();

    Thread* getNextThread()
    {
        // Implement logic to return the next thread to run
        return nullptr; // Placeholder
    }

private:
    std::vector<Thread*> activeTasks; // List of tasks to be scheduled
};

} // namespace kernel