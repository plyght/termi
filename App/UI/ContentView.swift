import SwiftUI

struct ContentView: View {
    @EnvironmentObject var sessionManager: SessionManager
    @EnvironmentObject var themeManager: ThemeManager
    @State private var isEmulatorRunning = false

    var body: some View {
        VStack(spacing: 0) {
            if let session = sessionManager.sessions.first {
                TerminalView(session: session)
            } else {
                Text("No active session")
                    .foregroundColor(themeManager.currentTheme.foregroundColor)
            }
        }
        .edgesIgnoringSafeArea(.all)
        .onAppear {
            if sessionManager.sessions.isEmpty {
                sessionManager.createSession()
            }
        }
    }
}
