#include "physics.h"
#include "tracker.h"
#include "core.h"

void updatePhysics(Core &core) {
    // find the entities with < A, B >
    // restrict remaining to entities with also < C >
    // restrict remaining to entities with also < D, E >
    // restrict remaining to entities with also < >

    typedef ComponentCollection< Position, Shape, Direction, Speed > Dynamics;
    typedef ComponentCollection< Position, Shape > Statics;
    core.tracker.partitionExec< Statics, Dynamics >([&](Statics &s, Dynamics &d) {
        for (size_t i = 0; i < d.get< Position >().size(); ++i) {
            d.at< Position >(i).v += d.at< Direction >(i).v * d.at< Speed >(i).d;
        }
    });

    /*
    core.tracker.fancyExec< Position, Shape >([&](auto &gatherer){
        core.tracker.restrict(gatherer, [&](Dynamics &d) {
            core.tracker.restrict< Position, Shape >(gatherer, [&](auto &s) {
                //const size_t x = std::get< 0 >(d.data).size();
                const size_t y = std::get< 0 >(s.data).size();
                const size_t x = d.get< Position >().size();
                std::cout << x << ' ' << y << '\n';
                std::cout << "There are " << s.get< Position >().size() << " static phys entities\n";
                std::cout << "There are " << d.get< Position >().size() << " dynamic phys entities\n";
                //const size_t sc = s.get< Position >().size();
                const size_t dc = d.get< Position >().size();
            });
        });
    });
    */
}
