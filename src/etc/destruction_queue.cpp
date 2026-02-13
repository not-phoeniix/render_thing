#include "etc/destruction_queue.h"

namespace RenderThing {
    DestructionQueue::DestructionQueue() { }

    DestructionQueue::~DestructionQueue() {
        Flush();
    }

    void DestructionQueue::QueueDelete(std::function<void()> func) {
        queue.push_back(func);
    }

    void DestructionQueue::Flush() {
        // reverse begin & end to make this flush LIFO
        for (auto i = queue.rbegin(); i != queue.rend(); i++) {
            (*i)();
        }

        queue.clear();
    }
}
