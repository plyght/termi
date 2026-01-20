import Foundation
import Combine
import SwiftUI

class SessionManager: ObservableObject {
    static let shared = SessionManager()
    
    @Published var sessions: [TerminalSession] = []
    @Published var activeSessionIndex: Int = 0
    
    private var cancellables = Set<AnyCancellable>()
    
    private init() {
        setupNotifications()
    }
    
    func createSession() {
        let session = TerminalSession(id: UUID())
        sessions.append(session)
        activeSessionIndex = sessions.count - 1
        
        EmulatorBridge.shared.attachSession(session)
    }
    
    func closeSession(at index: Int) {
        guard index < sessions.count else { return }
        let session = sessions[index]
        EmulatorBridge.shared.detachSession(session)
        sessions.remove(at: index)
        
        if sessions.isEmpty {
            createSession()
        } else if activeSessionIndex >= sessions.count {
            activeSessionIndex = sessions.count - 1
        }
    }
    
    func duplicateSession(at index: Int) {
        guard index < sessions.count else { return }
        let originalSession = sessions[index]
        let newSession = TerminalSession(
            id: UUID(),
            workingDirectory: originalSession.workingDirectory
        )
        sessions.insert(newSession, at: index + 1)
        activeSessionIndex = index + 1
        
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
                "workingDirectory": session.workingDirectory
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
        self.id = id
        self.workingDirectory = workingDirectory
        self.terminalProxy = TerminalProxy()
    }
    
    func sendInput(_ data: String) {
        EmulatorBridge.shared.sendInput(data, to: self)
    }
    
    func receiveOutput(_ data: Data) {
        DispatchQueue.main.async {
            if let text = String(data: data, encoding: .utf8) {
                self.output += text
                self.terminalProxy.feed(data: data)
            }
        }
    }
    
    func resize(cols: Int, rows: Int) {
        EmulatorBridge.shared.resize(session: self, cols: cols, rows: rows)
    }
}

class TerminalProxy {
    private var buffer: [[Character]] = []
    private var cols = 80
    private var rows = 24
    
    func feed(data: Data) {
    }
    
    func resize(cols: Int, rows: Int) {
        self.cols = cols
        self.rows = rows
        resizeBuffer()
    }
    
    private func resizeBuffer() {
        buffer = Array(repeating: Array(repeating: " ", count: cols), count: rows)
    }
}
