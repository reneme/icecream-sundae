/*
 * Command line Icecream status monitor
 * Copyright (C) 2018 by Garmin Ltd. or its subsidiaries.
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
#include "simulator.hpp"
#include "globals.hpp"

Job::Map Job::allJobs;
Job::Map Job::pendingJobs;
Job::Map Job::activeJobs;
Job::Map Job::localJobs;
Job::Map Job::remoteJobs;

std::vector<int> Host::host_color_ids;
int Host::localhost_color_id;
std::map<uint32_t, std::shared_ptr<Host> > Host::hosts;

std::shared_ptr<Job> Job::create(uint32_t id)
{
    class RealJob: public Job {
    public:
        explicit RealJob(uint32_t id): Job(id) {}
        virtual ~RealJob() {}
    };

    auto job = find(id);

    if (!job) {
        job = std::make_shared<RealJob>(id);
        allJobs[id] = job;
    }

    return job;
}

std::shared_ptr<Job> Job::find(uint32_t id)
{
    auto j = allJobs.find(id);

    if (j != allJobs.end())
        return j->second;
    return nullptr;
}

void Job::remove(uint32_t id)
{
    removeTypes(id);
    removeFromMap(allJobs, id);

    if (interface)
        interface->update();
}

void Job::removeFromMap(Map &map, uint32_t id)
{
    auto j = map.find(id);
    if (j != map.end())
        map.erase(j);
}

void Job::removeTypes(uint32_t id)
{
    removeFromMap(pendingJobs, id);
    removeFromMap(activeJobs, id);
    removeFromMap(localJobs, id);
    removeFromMap(remoteJobs, id);
}

void Job::createLocal(uint32_t id, uint32_t hostid, std::string const& filename)
{
    auto job = Job::create(id);

    job->active = true;
    job->clientid = hostid;
    job->hostid = hostid;
    job->is_local = true;
    job->filename = filename;
    job->start_time = g_get_monotonic_time();

    auto h = job->getClient();
    if (h)
        h->total_local++;
    total_local_jobs++;

    removeTypes(id);
    localJobs[id] = job;
    activeJobs[id] = job;

    if (interface)
        interface->update();
}

void Job::createPending(uint32_t id, uint32_t clientid, std::string const& filename)
{
    auto job = Job::create(id);

    job->clientid = clientid;
    job->filename = filename;

    removeTypes(id);
    pendingJobs[id] = job;

    if (interface)
        interface->update();
}

void Job::createRemote(uint32_t id, uint32_t hostid)
{
    auto job = Job::find(id);

    if (!job)
        return;

    job->active = true;
    job->hostid = hostid;
    job->start_time = g_get_monotonic_time();

    auto host = job->getHost();
    if (host)
        host->total_in++;

    auto client = job->getClient();
    if (client)
        client->total_out++;
    total_remote_jobs++;

    removeTypes(id);
    activeJobs[id] = job;
    remoteJobs[id] = job;

    if (interface)
        interface->update();
}

void Job::clearAll()
{
    allJobs.clear();
    pendingJobs.clear();
    activeJobs.clear();
    localJobs.clear();
    remoteJobs.clear();
}

std::shared_ptr<Host> Job::getClient() const
{
    if (!clientid)
        return nullptr;

    return Host::find(clientid);
}

std::shared_ptr<Host> Job::getHost() const
{
    if (!hostid)
        return nullptr;

    return Host::find(hostid);
}



std::shared_ptr<Host> Host::create(uint32_t id)
{
    class RealHost: public Host {
    public:
        explicit RealHost(uint32_t id): Host(id) {}
        virtual ~RealHost() {}
    };

    auto host = find(id);

    if (!host) {
        host = std::make_shared<RealHost>(id);
        hosts[id] = host;
    }

    if (interface)
        interface->update();
    return host;
}

std::shared_ptr<Host> Host::find(uint32_t id)
{
    auto h = hosts.find(id);

    if (h != hosts.end())
        return h->second;

    return nullptr;
}

void Host::remove(uint32_t id)
{
    auto h = hosts.find(id);

    if (h != hosts.end()) {
        hosts.erase(h);
        if (interface)
            interface->update();
    }
}

Job::Map Host::getPendingJobs() const
{
    Job::Map map;

    for (auto j : Job::pendingJobs) {
        if (j.second->clientid == id)
            map[j.first] = j.second;
    }

    return map;
}

Job::Map Host::getActiveJobs() const
{
    Job::Map map;

    for (auto j : Job::activeJobs) {
        if (j.second->clientid == id)
            map[j.first] = j.second;
    }

    return map;
}

Job::Map Host::getCurrentJobs() const
{
    Job::Map map;

    for (auto j : Job::activeJobs) {
        if (j.second->hostid == id)
            map[j.first] = j.second;
    }

    return map;
}

int Host::getColor() const
{
    char buffer[1024];

    if (gethostname(buffer, sizeof(buffer)) == 0 ) {
        buffer[sizeof(buffer) - 1] = '\0';
        if (getName() == buffer)
            return localhost_color_id;
    }

    return host_color_ids[hashName() % host_color_ids.size()];
}

size_t Host::hashName() const
{
    return std::hash<std::string>{}(getName());
}
