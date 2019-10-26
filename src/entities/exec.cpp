#include "entities/exec.h"

namespace Entity {

AccumulateTimer *k_entity_timer = nullptr;

std::mutex entity_timer_mutex;

void addEntityTime(std::chrono::duration< double > seconds) {
    std::unique_lock lock(entity_timer_mutex);
    k_entity_timer->add(seconds);
}

}
