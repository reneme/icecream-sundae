/*
 * Command line Icecream status monitor
 * Copyright (C) 2018-2020 by Garmin Ltd. or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <cassert>
#include <algorithm>
#include <vector>
#include <memory>
#include <iostream>
#include <map>
#include <glib.h>
#include <glib-unix.h>

#include "cli.hpp"
#include "scheduler.hpp"
#include "sundae.hpp"
#include "simulator.hpp"


static std::string schedname = std::string();
static std::string netname = std::string();
static gboolean opt_simulate = FALSE;
static gboolean opt_anonymize = FALSE;
static gint opt_sim_seed = 12345;
static gint opt_sim_cycles = -1;
static gint opt_sim_speed = 20;

static Scheduler     *abstractScheduler;
static UserInterface *userInterface;


static bool parse_args(int *argc, char ***argv)
{
    class GOptionContextDelete
    {
    public:
        void operator()(GOptionContext* ptr) const
        {
            g_option_context_free(ptr);
        }
    };

    static gchar *opt_scheduler = NULL;
    static gchar *opt_netname = NULL;
    static gboolean opt_about = FALSE;
    static gboolean opt_version = FALSE;

    static const GOptionEntry opts[] =
    {
        { "scheduler", 's', 0, G_OPTION_ARG_STRING, &opt_scheduler, "Icecream scheduler hostname", NULL },
        { "netname", 'n', 0, G_OPTION_ARG_STRING, &opt_netname, "Icecream network name", NULL },
        { "simulate", 0, 0, G_OPTION_ARG_NONE, &opt_simulate, "Simulate activity", NULL },
        { "sim-seed", 0, 0, G_OPTION_ARG_INT, &opt_sim_seed, "Simulator seed", NULL },
        { "sim-cycles", 0, 0, G_OPTION_ARG_INT, &opt_sim_cycles, "Number of simulator cycles to run. -1 for no limit", NULL },
        { "sim-speed", 0, 0, G_OPTION_ARG_INT, &opt_sim_speed, "Simulator speed (milliseconds between cycles)", NULL },
        { "anonymize", 0, 0, G_OPTION_ARG_NONE, &opt_anonymize, "Anonymize hosts and files (for demos)", NULL },
        { "about", 0, 0, G_OPTION_ARG_NONE, &opt_about, "Show about", NULL },
        { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version", NULL },
        {}
    };

    std::unique_ptr<GOptionContext, GOptionContextDelete> context(g_option_context_new(nullptr));

    g_option_context_add_main_entries(context.get(), opts, NULL);

    GError *error = NULL;
    if (!g_option_context_parse(context.get(), argc, argv, &error)) {
        std::cout << "Option parsing failed: " << error->message << std::endl;
        g_clear_error(&error);
        return false;
    }

    if (opt_scheduler)
        schedname = opt_scheduler;

    if (opt_netname)
        netname = opt_netname;

    if (opt_version) {
        std::cout << VERSION << std::endl;
        return false;
    }

    if (opt_about) {
        std::cout << "Command line Icecream status monitor" << std::endl;
        std::cout << "Version: " << VERSION << std::endl;
        std::cout <<
            "Copyright (C) 2018 by Garmin Ltd. or its subsidiaries." << std::endl <<
            std::endl <<
            "This program is free software; you can redistribute it and/or" << std::endl <<
            "modify it under the terms of the GNU General Public License" << std::endl <<
            "as published by the Free Software Foundation; either version 2" << std::endl <<
            "of the License, or (at your option) any later version." << std::endl <<
            std::endl <<
            "This program is distributed in the hope that it will be useful," << std::endl <<
            "but WITHOUT ANY WARRANTY; without even the implied warranty of" << std::endl <<
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << std::endl <<
            "GNU General Public License for more details." << std::endl <<
            std::endl <<
            "You should have received a copy of the GNU General Public License" << std::endl<<
            "along with this program; if not, write to the Free Software" << std::endl <<
            "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." << std::endl;
        return false;
    }

    return true;
}

static gboolean on_quit_signal(gpointer)
{
    stop_icecream_sundae();
    return TRUE;
}

static gboolean process_input(gint, GIOCondition, gpointer)
{
    int c = userInterface->processInput();
    if (c && abstractScheduler)
        abstractScheduler->onInput(c);
    return TRUE;
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    if (!parse_args(&argc, &argv))
        return 1;

    std::cout <<
        "Command line Icecream status monitor, Version " << VERSION << std::endl <<
        "Copyright (C) 2018 by Garmin Ltd. or its subsidiaries." << std::endl <<
        "This is free software, and you are welcome to redistribute it" << std::endl <<
        "under certain conditions; run with '--about' for details." << std::endl;

    auto sched = (opt_simulate)
        ? create_simulator(opt_sim_seed, opt_sim_cycles, opt_sim_speed)
        : connect_to_scheduler(netname, schedname);
    abstractScheduler = sched.get();

    auto ui = create_cli_interface();
    userInterface = ui.get();

    userInterface->set_anonymize(opt_anonymize);

    int input_fd = userInterface->getInputFd();
    GlibSource input_source;

    if (input_fd >= 0)
        input_source.set(g_unix_fd_add(input_fd, G_IO_IN, process_input, nullptr));

    GlibSource sigint_source(g_unix_signal_add(SIGINT, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr));
    GlibSource sigterm_source(g_unix_signal_add(SIGTERM, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr));

    start_icecream_sundae(std::move(ui), std::move(sched));

    teardown_icecream_sundae();

    return 0;
}
