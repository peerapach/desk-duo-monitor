#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
namespace deskduo {
struct TouchSample {
    uint64_t sequence=0, timeMs=0;
    int rawX=0, rawY=0, pressure=0, x=0, y=0;
};
// sequence counts all polls; samples retain only pressure-accepted polls.
// pressureAccepted is input_read acceptance, before LVGL's two-poll debounce.
struct TouchDiagnosticsSnapshot {
    uint64_t sequence=0, lastPollTimeMs=0;
    int lastPressure=0;
    bool pressureAccepted=false;
    std::size_t count=0;
    std::array<TouchSample,32> samples{};
};
class TouchDiagnostics {
public:
    // Called only by the existing input poll; never allocates or reads hardware.
    void poll(uint64_t timeMs, int pressure, bool accepted, int rawX=0, int rawY=0, int x=0, int y=0) {
        const std::lock_guard<std::mutex> lock(mutex_);
        ++state_.sequence;
        state_.lastPollTimeMs=timeMs;
        state_.lastPressure=pressure;
        state_.pressureAccepted=accepted;
        if (!accepted) return;
        state_.samples[next_]={state_.sequence,timeMs,rawX,rawY,pressure,x,y};
        next_=(next_+1)%state_.samples.size();
        if (state_.count<state_.samples.size()) ++state_.count;
    }
    TouchDiagnosticsSnapshot snapshot() const {
        const std::lock_guard<std::mutex> lock(mutex_);
        auto result=state_;
        const auto first=state_.count==state_.samples.size()?next_:0;
        for (std::size_t i=0;i<state_.count;++i)
            result.samples[i]=state_.samples[(first+i)%state_.samples.size()];
        return result;
    }
private:
    mutable std::mutex mutex_;
    TouchDiagnosticsSnapshot state_{};
    std::size_t next_=0;
};
// Encoding happens after snapshot() has released the collector lock.
inline std::string touchDiagnosticsJson(const TouchDiagnosticsSnapshot& state) {
    std::string json="{\"sequence\":"+std::to_string(state.sequence)
        +",\"lastPollTimeMs\":"+std::to_string(state.lastPollTimeMs)
        +",\"lastPressure\":"+std::to_string(state.lastPressure)
        +",\"pressureAccepted\":"+(state.pressureAccepted?"true":"false")+",\"samples\":[";
    for (std::size_t i=0;i<state.count;++i) {
        const auto& sample=state.samples[i];
        if (i) json+=',';
        json+="{\"sequence\":"+std::to_string(sample.sequence)+",\"timeMs\":"+std::to_string(sample.timeMs)
            +",\"rawX\":"+std::to_string(sample.rawX)+",\"rawY\":"+std::to_string(sample.rawY)
            +",\"pressure\":"+std::to_string(sample.pressure)+",\"x\":"+std::to_string(sample.x)
            +",\"y\":"+std::to_string(sample.y)+"}";
    }
    return json+"]}";
}
}
