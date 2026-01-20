import SwiftUI

struct ContentView: View {
    @EnvironmentObject var sessionManager: SessionManager
    @EnvironmentObject var themeManager: ThemeManager
    @State private var isEmulatorRunning = false

    var body: some View {
        let _ = print("🔄 [ContentView] BODY - Rendering ContentView")
        let _ = print("📊 [ContentView] BODY - Sessions count: \(sessionManager.sessions.count)")
        let _ = print("📊 [ContentView] BODY - Has first session: \(sessionManager.sessions.first != nil)")
        
        return VStack(spacing: 0) {
            if let session = sessionManager.sessions.first {
                let _ = print("✅ [ContentView] BODY - Rendering TerminalView for session: \(session.id)")
                TerminalView(session: session)
            } else {
                let _ = print("⚠️  [ContentView] BODY - No session, showing placeholder")
                Text("No active session")
                    .foregroundColor(themeManager.currentTheme.foregroundColor)
            }
        }
        .edgesIgnoringSafeArea(.all)
        .onAppear {
            print("👀 [ContentView] onAppear - View appeared")
            print("📊 [ContentView] onAppear - Sessions count: \(sessionManager.sessions.count)")
            if sessionManager.sessions.isEmpty {
                print("🆕 [ContentView] onAppear - Creating first session")
                sessionManager.createSession()
            } else {
                print("✅ [ContentView] onAppear - Session already exists")
            }
        }
    }
}
