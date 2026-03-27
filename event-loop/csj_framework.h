// =============================================================================
// CSJFramework - Server-Job Framework: the main event loop
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original file: SJFramework.cc (referenced in log messages).
//
// This is the core loop that drives every lapis-based server. All members
// are static (singleton pattern), matching the original binary where Services,
// Connections, EventQueue, etc. are file-scope statics.
//
// Main loop logic:
//   while (!shutdown) {
//     gettimeofday(&now)
//     EventQueue->Dispatch(now)         // fire ready events
//     if (EventQueue->Top())            epoll(timeout=0)      // non-blocking
//     else if (EventQueue->TopRealTime()) epoll(timeout=delta)  // sleep until next timer
//     else                              epoll(timeout=NULL)    // block until I/O
//   }
// =============================================================================

#pragma once

#include <list>
#include <string>
#include <cstdint>
#include <sys/time.h>

namespace lapis {

class CEvent;
class CRealTimeEvent;
class CEventQueue;
class CMSTCPConnection;
class CMSTCPService;
class IInterfaceFactory;

class CSJFramework {
public:
    // --- Initialization ---
    static void Init();              // Create event queue + socket manager
    static void InitEventQueue();    // Create event queue only
    static void InitNetwork();       // Create socket manager, generate RSA key, open services

    // --- Main loops ---
    static void Run();               // Event-driven (epoll blocks when idle)
    static void Run(const struct timeval& heartBeat);  // Fixed heartbeat interval
    static void RunRealtime();       // Alias for Run() -- process events ASAP
    static void Done();              // Cleanup all resources

    // --- Service / Connection registration ---
    // DeclareTcp: register a listener on a port (for accepting client connections)
    static void DeclareTcp(IInterfaceFactory* factory, int port);
    static void DeclareTcp(IInterfaceFactory* factory, uint32_t ip, int port);

    // ConnectTcp: connect out to another server (e.g., Zone -> World)
    static CMSTCPConnection* ConnectTcp(IInterfaceFactory* factory,
                                        const std::string& address, int port);

    // Lower-level registration
    static void DeclareService(CMSTCPService* service);
    static void DeclareConnection(CMSTCPConnection* connection);

    // --- Event helpers ---
    static void PushImmediateEvent(CEvent* e);      // Execute on next dispatch cycle
    static void PushRealTimeEvent(CRealTimeEvent* e); // Execute at scheduled time

    // --- Shutdown ---
    static void Shutdown();

    // --- Time accessors ---
    static unsigned long getCurrentSecond();
    static void setCurrentSecond(unsigned long sec);

    // --- Performance logging ---
    static void InitLoopPerfChecker(uint32_t size);
    static void BeginLoopPerfLog(const struct timeval& interval, uint32_t width);
    static void EndLoopPerfLog();

private:
    static std::list<CMSTCPService*>    Services;      // Listening sockets
    static std::list<CMSTCPConnection*> Connections;    // Outgoing connections
    static CEventQueue*                 EventQueue;     // Priority + real-time queue
    static bool                         m_shutdown;     // Shutdown flag
    static unsigned long                CurrentSecond;  // Cached epoch second
};

} // namespace lapis
