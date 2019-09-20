/*
 * Command line Icecream status monitor
 * Copyright (C) 2018-2019 by Garmin Ltd. or its subsidiaries.
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

#include <algorithm>
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <unordered_set>
#include <iomanip>
#include <iostream>

#include <assert.h>
#include <glib.h>
#include <glib-unix.h>
#include <math.h>

#include "cli.hpp"
#include "info.hpp"

class CliInterface: public UserInterface {
public:
    CliInterface();
    virtual ~CliInterface();

    virtual void update() override;
    virtual int processInput() override;

    virtual int getInputFd() override
    {
        return STDIN_FILENO;
    }

    virtual void suspend() override;
    virtual void resume() override;

    virtual void set_anonymize(bool a) override
    {
        anonymize = a;
    }

private:
    bool anonymize = false;
};

void CliInterface::update()
{
    auto hosts = Host::hosts;

    for (const auto &host : hosts) {
        const auto &h = host.second;
        std::cout << h->getName() << " (" << h->getCurrentJobs().size() << "/" << h->getMaxJobs() << ")" << ", ";
    }

    std::cout << std::endl;
}

int CliInterface::processInput()
{
    return 0;
}

void CliInterface::suspend()
{
}

void CliInterface::resume()
{
}

CliInterface::CliInterface() :
    UserInterface()
{
}

CliInterface::~CliInterface()
{
}

std::unique_ptr<UserInterface> create_cli_interface()
{
    return std::make_unique<CliInterface>();
}
