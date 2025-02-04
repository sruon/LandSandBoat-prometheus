#include "common/logging.h"
#include "lua/luautils.h"
#include "map/utils/moduleutils.h"

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

enum PrometheusMetricType
{
    Counter,
    Gauge
};

class PrometheusModule : public CPPModule
{
    std::shared_ptr<prometheus::Registry>                           m_Registry;
    std::shared_ptr<prometheus::Exposer>                            m_Exposer;
    std::map<std::string, prometheus::Family<prometheus::Counter>*> m_Counters;
    std::map<std::string, prometheus::Family<prometheus::Gauge>*>   m_Gauges;

    void updateCounter(const std::string& name, const prometheus::Labels& labels, const double value)
    {
        if (const auto it = m_Counters.find(name); it != m_Counters.end())
        {
            it->second->Add(labels).Increment(value);
        }
    }

    void updateGauge(const std::string& name, const prometheus::Labels& labels, const double value)
    {
        if (const auto it = m_Gauges.find(name); it != m_Gauges.end())
        {
            it->second->Add(labels).Increment(value);
        }
    }

    void OnInit() override
    {
        std::map<std::string, PrometheusMetricType> defaultMetrics = {
            // LSB module hooks
            { "zone_ticks", Counter },
            { "time_server_ticks", Counter },
            { "char_zone", Gauge },
            { "packet_pushes", Counter },
            { "outgoing_packet_size", Counter },
        };

        m_Registry = std::make_shared<prometheus::Registry>();

        for (const auto& [name, type] : defaultMetrics)
        {
            if (type == Counter)
            {
                m_Counters[name] = &prometheus::BuildCounter()
                                        .Name(fmt::format("{}_counter", name))
                                        .Help("Counter for " + name)
                                        .Register(*m_Registry);
            }
            else if (type == Gauge)
            {
                m_Gauges[name] = &prometheus::BuildGauge()
                                      .Name(fmt::format("{}_gauge", name))
                                      .Help("Gauge for " + name)
                                      .Register(*m_Registry);
            }
        }

        int         port = 0;
        std::string listenAddress;
        if (port = settings::get<int>("map.PROMETHEUS_PORT"); port == 0)
        {
            port = 9865;
            ShowWarningFmt("[prometheus] No listen port defined, listening on default port: {}", port);
        }

        if (listenAddress = settings::get<std::string>("map.PROMETHEUS_LISTEN_ADDRESS"); listenAddress.empty())
        {
            listenAddress = "127.0.0.1";
            ShowWarningFmt("[prometheus] No listen address defined, listening on default address: {}", listenAddress);
        }

        std::string address = fmt::format("{}:{}", listenAddress, port);
        m_Exposer           = std::make_shared<prometheus::Exposer>(address);
        m_Exposer->RegisterCollectable(m_Registry);
        ShowInfo("[prometheus] Exposer started. Visit http://%s/metrics", address);
    }

    void OnZoneTick(CZone* PZone) override
    {
        updateCounter("zone_ticks", { { "zoneName", PZone->getName() } }, 1);
    }

    void OnTimeServerTick() override
    {
        updateCounter("time_server_ticks", {}, 1);
    }

    void OnCharZoneIn(CCharEntity* PChar) override
    {
        updateGauge("char_zone", { { "zoneName", PChar->loc.zone->getName() } }, 1);
    }

    void OnCharZoneOut(CCharEntity* PChar) override
    {
        updateGauge("char_zone", { { "zoneName", PChar->loc.zone->getName() } }, -1);
    }

    void OnPushPacket(CCharEntity* PChar, const std::unique_ptr<CBasicPacket>& packet) override
    {
        updateCounter("packet_pushes", { { "packetType", fmt::format("{:X}", packet->getType()) } }, 1);
        updateCounter("outgoing_packet_size", { { "packetType", fmt::format("{:X}", packet->getType()) } }, packet->getSize());
    }
};

REGISTER_CPP_MODULE(PrometheusModule);