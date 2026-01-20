import SwiftUI

@main
struct TermiApp: App {
    @StateObject private var sessionManager = SessionManager.shared
    @StateObject private var themeManager = ThemeManager.shared
    
    init() {
        setupTerminalEnvironment()
    }
    
    var body: some Scene {
        WindowGroup {
            MainTerminalView()
                .environmentObject(sessionManager)
                .environmentObject(themeManager)
                .preferredColorScheme(themeManager.currentScheme)
        }
    }
    
    private func setupTerminalEnvironment() {
        EmulatorBridge.shared.initialize()
    }
}

struct MainTerminalView: View {
    @EnvironmentObject var sessionManager: SessionManager
    @EnvironmentObject var themeManager: ThemeManager
    @State private var showSettings = false
    @State private var selectedTab = 0
    
    var body: some View {
        ZStack {
            themeManager.currentTheme.backgroundColor
                .ignoresSafeArea()
            
            VStack(spacing: 0) {
                if sessionManager.sessions.count > 1 {
                    TabBarView(selectedTab: $selectedTab)
                }
                
                if let session = sessionManager.sessions[safe: selectedTab] {
                    TerminalView(session: session)
                } else {
                    Text("No active session")
                        .foregroundColor(themeManager.currentTheme.foregroundColor)
                }
            }
        }
        .sheet(isPresented: $showSettings) {
            SettingsView()
        }
        .gesture(
            MagnificationGesture()
                .onChanged { scale in
                    handlePinchGesture(scale: scale)
                }
        )
        .onAppear {
            if sessionManager.sessions.isEmpty {
                sessionManager.createSession()
            }
        }
    }
    
    private func handlePinchGesture(scale: CGFloat) {
        let newSize = themeManager.fontSize * scale
        themeManager.setFontSize(newSize)
    }
}

extension Collection {
    subscript(safe index: Index) -> Element? {
        return indices.contains(index) ? self[index] : nil
    }
}
