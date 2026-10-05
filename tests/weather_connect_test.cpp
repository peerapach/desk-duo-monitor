#include "../firmware/main/weather_connect.hpp"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <cerrno>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr int socketFd = 7;
uint64_t clockMs;
uint64_t readyAt;
fd_set retainedRead, retainedWrite;
bool connecting, handshaking;
int selectError, socketError, socketLookupError, captures, polls, yields;
struct Step { int result; uint64_t elapsed; int socketError = 0; esp_tls_last_error errors{0, 0, 0}; };
std::vector<Step> handshake, scripted;
std::vector<int> timeouts;
std::size_t nextStep;
bool useScript;
bool expectedSecure;

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
void reset(uint64_t now, bool secure = true) {
    clockMs = now; readyAt = now; connecting = handshaking = false;
    FD_ZERO(&retainedRead); FD_ZERO(&retainedWrite);
    selectError = socketError = socketLookupError = captures = polls = yields = 0;
    handshake = {{1, 0}}; scripted.clear(); timeouts.clear(); nextStep = 0;
    useScript = false; expectedSecure = secure; errno = 0;
}
int step(esp_tls_t* tls, const std::vector<Step>& steps) {
    require(nextStep < steps.size(), "unexpected async invocation after terminal result");
    const auto& current = steps[nextStep++];
    clockMs += current.elapsed; errno = current.socketError; tls->errors = current.errors;
    return current.result;
}
int connect(deskduo::FetchPolicy& policy, esp_tls_t& tls, deskduo::WeatherAttempt* attempt = nullptr) {
    return deskduo::connectResolvedTransport(policy, &tls, expectedSecure, "api.open-meteo.com", "192.0.2.1", expectedSecure ? 443 : 80, attempt);
}
deskduo::WeatherAttempt runningAttempt() {
    deskduo::WeatherAttempt attempt;
    attempt.attempted = true; attempt.outcome = deskduo::WeatherOutcome::Running;
    return attempt;
}
void requireFailure(const deskduo::WeatherAttempt& attempt, deskduo::WeatherOutcome outcome, deskduo::WeatherPhase phase, int error) {
    require(attempt.outcome == outcome && attempt.phase == phase, "terminal failure classification changed");
    require(attempt.socketErrno == error && captures == 1, "failure errors were not captured while handle is alive");
}
}

namespace deskduo { uint64_t now_ms() { return clockMs; } }
void vTaskDelay(TickType_t ticks) {
    require(ticks == 1, "pending async/plain connection must retain the existing one-tick yield");
    ++yields; clockMs += ticks;
}
esp_err_t esp_crt_bundle_attach(void*) { return ESP_OK; }
// Model only the pinned ESP-IDF 5.5.3 CONNECTING/select contract: sets are
// initialized at INIT, retained on retries, and cleared by a timed-out select.
// It is deliberately not a DNS, HTTP, TLS or whole-network implementation.
int weather_test_select(int nfds, fd_set* reads, fd_set* writes, fd_set* excepts, timeval* interval) {
    ++polls;
    require(nfds == socketFd + 1 && writes && !excepts && interval, "unexpected readiness call");
    if (selectError) { errno = selectError; return -1; }
    const auto wait = static_cast<uint64_t>(interval->tv_sec) * 1000 + interval->tv_usec / 1000;
    const bool interested = FD_ISSET(socketFd, writes) || (reads && FD_ISSET(socketFd, reads));
    if (interested && readyAt <= clockMs + wait) {
        if (clockMs < readyAt) clockMs = readyAt;
        return 1;
    }
    clockMs += wait;
    FD_ZERO(writes); if (reads) FD_ZERO(reads);
    return 0;
}
int weather_test_getsockopt(int socket, int level, int option, void* result, socklen_t* length) {
    require(socket == socketFd && level == SOL_SOCKET && option == SO_ERROR && *length == sizeof(int), "socket completion check changed");
    *static_cast<int*>(result) = socketError;
    return 0;
}
int esp_tls_conn_new_async(const char* numeric, int length, int port, const esp_tls_cfg_t* cfg, esp_tls_t* tls) {
    require(tls && tls->alive && std::string(numeric, length) == "192.0.2.1", "numeric address or handle ownership changed");
    require(port == (expectedSecure ? 443 : 80) && cfg && cfg->non_block && cfg->is_plain_tcp == !expectedSecure, "nonblocking transport security changed");
    require(std::string(cfg->common_name) == "api.open-meteo.com" && cfg->crt_bundle_attach == (expectedSecure ? esp_crt_bundle_attach : nullptr), "SNI/hostname verification or certificate bundle changed");
    require(cfg->timeout_ms > 0, "SDK invoked with nonpositive timeout (SDK would wait without a bound)");
    timeouts.push_back(cfg->timeout_ms);
    if (useScript) return step(tls, scripted);
    if (cfg->is_plain_tcp) return 1; // pinned INIT returns success even for EINPROGRESS
    if (!connecting && !handshaking) {
        FD_SET(socketFd, &retainedRead); retainedWrite = retainedRead; connecting = true;
    }
    if (connecting) {
        timeval wait{cfg->timeout_ms / 1000, (cfg->timeout_ms % 1000) * 1000};
        if (weather_test_select(socketFd + 1, &retainedRead, &retainedWrite, nullptr, &wait) == 0) return 0;
        connecting = false; handshaking = true;
    }
    return step(tls, handshake);
}
esp_err_t esp_tls_get_conn_sockfd(esp_tls_t* tls, int* out) {
    require(tls && tls->alive, "plain completion accessed a destroyed handle");
    *out = socketFd; return socketLookupError;
}
esp_err_t esp_tls_get_error_handle(esp_tls_t* tls, esp_tls_error_handle_t* out) {
    require(tls && tls->alive, "failure accessed a destroyed handle");
    *out = &tls->errors; return ESP_OK;
}
esp_err_t esp_tls_get_and_clear_last_error(esp_tls_error_handle_t errors, int* code, int* flags) {
    ++captures; const auto result = errors->esp; *code = errors->tls; *flags = errors->flags; *errors = {0, 0, 0}; return result;
}
size_t heap_caps_get_free_size(unsigned flags) { require(flags == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT), "heap capability changed"); return 81920; }
size_t heap_caps_get_largest_free_block(unsigned flags) { require(flags == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT), "heap capability changed"); return 65536; }

int main() {
    using namespace deskduo;
    int failures = 0;
    auto test = [&](const char* name, const std::function<void()>& run) {
        try { run(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cout << "FAIL " << name << ": " << error.what() << '\n'; }
    };
    test("TCP ready after 250 ms progresses with DNS/handshake time deducted", [] {
        // Fetch began at 1000; DNS/earlier work spent 2200 ms. TCP is ready
        // 250 ms later, well beyond the old 1 ms select's retained-set timeout.
        reset(3200); readyAt = 3450; handshake = {{0, 4}, {0, 7}, {1, 5}};
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls;
        require(connect(policy, tls) == 0, "connection stranded before delayed TCP readiness");
        require(timeouts == std::vector<int>({7800, 7545, 7537}), "spent DNS/select/handshake/yield time reset or failed to reduce the shared deadline");
        require(clockMs == 3468 && yields == 2 && polls == 1 && tls.alive, "async success/yields/handle ownership changed");
    });
    test("pending TCP exhausts the original 10 second deadline without a retry", [] {
        reset(3200); readyAt = 12000;
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1, "pending TCP must fail at the fetch deadline");
        require(timeouts == std::vector<int>({7800}) && polls == 1, "select timeout must consume the remaining budget, without retrying cleared sets");
        require(clockMs == 11001 && yields == 1 && errno == ETIMEDOUT && tls.alive, "deadline was extended beyond the original budget plus existing one-tick yield");
        requireFailure(attempt, WeatherOutcome::Deadline, WeatherPhase::Tls, ETIMEDOUT);
    });
    test("expiry before SDK call never supplies a zero or negative timeout", [] {
        for (const auto now : {uint64_t{11000}, uint64_t{11001}, uint64_t{12000}}) {
            reset(now); FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
            require(connect(policy, tls, &attempt) == -1 && timeouts.empty() && yields == 0, "expired deadline invoked the SDK");
            requireFailure(attempt, WeatherOutcome::Deadline, WeatherPhase::Tls, ETIMEDOUT);
        }
    });
    test("last positive millisecond is bounded and never followed by a nonpositive call", [] {
        reset(10999); readyAt = 11050;
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls;
        require(connect(policy, tls) == -1 && timeouts == std::vector<int>({1}) && clockMs == 11001, "last positive deadline or expiry handling changed");
    });
    test("redirect hop retains the original fetch deadline", [] {
        reset(9500); readyAt = 9700;
        FetchPolicy policy("https://api.open-meteo.com/start", 1000); esp_tls_t tls;
        require(policy.redirect("/next"), "fixture redirect rejected");
        require(connect(policy, tls) == 0 && timeouts == std::vector<int>({1500}) && clockMs == 9700, "redirect connect started a fresh 10 second budget");
    });
    test("pending handshake recomputes remaining time and exits when it is spent", [] {
        reset(9000); useScript = true; scripted = {{0, 1999}};
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1 && timeouts == std::vector<int>({2000}) && clockMs == 11000 && yields == 1, "pending handshake retried beyond the shared deadline");
        requireFailure(attempt, WeatherOutcome::Deadline, WeatherPhase::Tls, ETIMEDOUT);
    });
    test("async TLS error terminates and retains numeric error capture", [] {
        reset(2000); useScript = true; scripted = {{-1, 7, EIO, {0x801a, -0x2700, 8}}};
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1 && timeouts == std::vector<int>({9000}) && yields == 0 && tls.alive, "async error retried or changed handle ownership");
        requireFailure(attempt, WeatherOutcome::TlsFailure, WeatherPhase::Tls, EIO);
        require(attempt.espError == 0x801a && attempt.tlsCode == -0x2700 && attempt.verifyFlags == 8, "numeric TLS/verification errors changed");
    });
    test("SDK TCP error still reclassifies TLS setup failure", [] {
        reset(2000); useScript = true; scripted = {{-1, 0, ECONNREFUSED, {ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST, 0, 0}}};
        FetchPolicy policy("https://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1, "SDK TCP failure accepted");
        requireFailure(attempt, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, ECONNREFUSED);
    });
    test("plain async success still waits for socket completion without reconnecting", [] {
        reset(2000, false); readyAt = 2004;
        FetchPolicy policy("http://api.open-meteo.com/weather", 1000); esp_tls_t tls;
        require(connect(policy, tls) == 0 && timeouts == std::vector<int>({9000}) && polls == 3 && yields == 2 && clockMs == 2004, "plain pending socket completion/yields changed");
    });
    test("plain socket error remains a TCP failure with original numeric errno", [] {
        reset(2000, false); socketError = ECONNREFUSED;
        FetchPolicy policy("http://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1 && timeouts == std::vector<int>({9000}) && yields == 0, "plain socket error accepted or reconnected");
        requireFailure(attempt, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, ECONNREFUSED);
    });
    test("plain socket lookup/select failures retain existing failure path", [] {
        for (bool lookup : {true, false}) {
            reset(2000, false); errno = EBADF;
            if (lookup) socketLookupError = -1; else selectError = EBADF;
            FetchPolicy policy("http://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
            require(connect(policy, tls, &attempt) == -1 && timeouts == std::vector<int>({9000}), "plain socket failure accepted");
            requireFailure(attempt, WeatherOutcome::TcpFailure, WeatherPhase::Tcp, EBADF);
        }
    });
    test("plain completion polling retains the shared deadline", [] {
        reset(10997, false); readyAt = 12000;
        FetchPolicy policy("http://api.open-meteo.com/weather", 1000); esp_tls_t tls; WeatherAttempt attempt = runningAttempt();
        require(connect(policy, tls, &attempt) == -1 && timeouts == std::vector<int>({3}) && clockMs == 11001 && yields == 2, "plain completion extended or reset the fetch deadline");
        requireFailure(attempt, WeatherOutcome::Deadline, WeatherPhase::Tcp, ETIMEDOUT);
    });
    std::cout << "Production async-connect regression: " << failures << " failed\n";
    return failures ? 1 : 0;
}
