#include "sundae.hpp"

#include "globals.hpp"

void start_icecream_sundae(std::unique_ptr<AbstractInterface> iface, std::unique_ptr<Scheduler> sched)
{
    interface = std::move(iface);
    scheduler = std::move(sched);

    main_loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(main_loop);
}

void stop_icecream_sundae()
{
    g_main_loop_quit(main_loop);
}

void teardown_icecream_sundae() {
    scheduler.reset();
    interface.reset();

    g_main_loop_unref(main_loop);

    Job::clearAll();
    Host::hosts.clear();
}