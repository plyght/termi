import Combine
import Foundation
import SwiftUI

class SessionManager: ObservableObject {
    static let shared = SessionManager()

    @Published var sessions: [TerminalSession] = []
    @Published var activeSessionIndex: Int = 0

    private var cancellables = Set<AnyCancellable>()

    private init() {
        print("🏗️  [SessionManager] INIT - Initializing SessionManager")
        setupNotifications()
        print("✅ [SessionManager] INIT - SessionManager initialized")
    }

    func createSession() {
        print("🆕 [SessionManager] createSession - Creating new session")
        let session = TerminalSession(id: UUID())
        print("📝 [SessionManager] createSession - Session ID: \(session.id)")
        sessions.append(session)
        activeSessionIndex = sessions.count - 1
        print("📊 [SessionManager] createSession - Total sessions: \(sessions.count), Active index: \(activeSessionIndex)")

        print("🔗 [SessionManager] createSession - Attaching session to EmulatorBridge")
        EmulatorBridge.shared.attachSession(session)
        print("✅ [SessionManager] createSession - Session created and attached")
    }

    func closeSession(at index: Int) {
        print("🗑️  [SessionManager] closeSession - Closing session at index: \(index)")
        guard index < sessions.count else {
            print("⚠️  [SessionManager] closeSession - Invalid index: \(index), total sessions: \(sessions.count)")
            return
        }
        let session = sessions[index]
        print("📝 [SessionManager] closeSession - Session ID: \(session.id)")
        EmulatorBridge.shared.detachSession(session)
        sessions.remove(at: index)
        print("📊 [SessionManager] closeSession - Remaining sessions: \(sessions.count)")

        if sessions.isEmpty {
            print("🆕 [SessionManager] closeSession - No sessions left, creating new one")
            createSession()
        } else if activeSessionIndex >= sessions.count {
            activeSessionIndex = sessions.count - 1
            print("📊 [SessionManager] closeSession - Adjusted active index to: \(activeSessionIndex)")
        }
    }

    func duplicateSession(at index: Int) {
        print("📋 [SessionManager] duplicateSession - Duplicating session at index: \(index)")
        guard index < sessions.count else {
            print("⚠️  [SessionManager] duplicateSession - Invalid index: \(index)")
            return
        }
        let originalSession = sessions[index]
        print("📝 [SessionManager] duplicateSession - Original session ID: \(originalSession.id)")
        let newSession = TerminalSession(
            id: UUID(),
            workingDirectory: originalSession.workingDirectory
        )
        print("📝 [SessionManager] duplicateSession - New session ID: \(newSession.id)")
        sessions.insert(newSession, at: index + 1)
        activeSessionIndex = index + 1
        print("📊 [SessionManager] duplicateSession - Total sessions: \(sessions.count)")

        EmulatorBridge.shared.attachSession(newSession)
    }

    private func setupNotifications() {
        NotificationCenter.default.publisher(for: UIApplication.willResignActiveNotification)
            .sink { [weak self] _ in
                self?.saveSessionState()
            }
            .store(in: &cancellables)

        NotificationCenter.default.publisher(for: UIApplication.didBecomeActiveNotification)
            .sink { [weak self] _ in
                self?.restoreSessionState()
            }
            .store(in: &cancellables)
    }

    private func saveSessionState() {
        let sessionData = sessions.map { session in
            [
                "id": session.id.uuidString,
                "workingDirectory": session.workingDirectory,
            ]
        }
        UserDefaults.standard.set(sessionData, forKey: "savedSessions")
    }

    private func restoreSessionState() {
        guard let sessionData = UserDefaults.standard.array(forKey: "savedSessions") as? [[String: String]] else {
            return
        }

        sessions.removeAll()
        for data in sessionData {
            if let idString = data["id"],
               let id = UUID(uuidString: idString),
               let workingDirectory = data["workingDirectory"] {
                let session = TerminalSession(id: id, workingDirectory: workingDirectory)
                sessions.append(session)
                EmulatorBridge.shared.attachSession(session)
            }
        }

        if sessions.isEmpty {
            createSession()
        }
    }
}

class TerminalSession: ObservableObject, Identifiable {
    let id: UUID
    @Published var workingDirectory: String
    @Published var output: String = ""
    @Published var scrollOffset: CGFloat = 0

    var terminalProxy: TerminalProxy

    init(id: UUID, workingDirectory: String = "/root") {
        print("🏗️  [TerminalSession] INIT - Session ID: \(id), Working dir: \(workingDirectory)")
        self.id = id
        self.workingDirectory = workingDirectory
        terminalProxy = TerminalProxy()
        print("✅ [TerminalSession] INIT - Session initialized")
    }

    func sendInput(_ data: String) {
        print("⌨️  [TerminalSession] sendInput - Session \(id.uuidString.prefix(8)), Data length: \(data.count)")
        print("📝 [TerminalSession] sendInput - Input: \(data.prefix(100))")
        EmulatorBridge.shared.sendInput(data, to: self)
    }

    func receiveOutput(_ data: Data) {
        print("📥 [TerminalSession] receiveOutput - Session \(id.uuidString.prefix(8)), Data size: \(data.count) bytes")
        DispatchQueue.main.async {
            if let text = String(data: data, encoding: .utf8) {
                print("📝 [TerminalSession] receiveOutput - Text: \(text.prefix(100))")
                self.output += text
                print("📊 [TerminalSession] receiveOutput - Total output length: \(self.output.count)")
                self.terminalProxy.feed(data: data)
            } else {
                print("⚠️  [TerminalSession] receiveOutput - Failed to decode data as UTF-8")
            }
        }
    }

    func resize(cols: Int, rows: Int) {
        print("📐 [TerminalSession] resize - Session \(id.uuidString.prefix(8)), Cols: \(cols), Rows: \(rows)")
        EmulatorBridge.shared.resize(session: self, cols: cols, rows: rows)
    }
}

class TerminalProxy {
    private var buffer: [[Character]] = []
    private var cols = 80
    private var rows = 24

    func feed(data _: Data) {}

    func resize(cols: Int, rows: Int) {
        self.cols = cols
        self.rows = rows
        resizeBuffer()
    }

    private func resizeBuffer() {
        buffer = Array(repeating: Array(repeating: " ", count: cols), count: rows)
    }
}
