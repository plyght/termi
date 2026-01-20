import SwiftUI

@main
struct TermiApp: App {
    @StateObject private var sessionManager = SessionManager.shared
    @StateObject private var themeManager = ThemeManager.shared

    init() {
        print("🚀 [TermiApp] INIT - App starting up")
        setupTerminalEnvironment()
        print("✅ [TermiApp] INIT - Setup complete")
    }

    var body: some Scene {
        print("🔄 [TermiApp] BODY - Building WindowGroup")
        return WindowGroup {
            MainTerminalView()
                .environmentObject(sessionManager)
                .environmentObject(themeManager)
                .preferredColorScheme(themeManager.currentScheme)
        }
    }

    private func setupTerminalEnvironment() {
        print("⚙️  [TermiApp] setupTerminalEnvironment - Initializing emulator bridge")
        EmulatorBridge.shared.initialize()
        print("✅ [TermiApp] setupTerminalEnvironment - Emulator bridge initialized")
    }
}

struct MainTerminalView: View {
    @EnvironmentObject var sessionManager: SessionManager
    @EnvironmentObject var themeManager: ThemeManager
    @State private var showSettings = false
    @State private var selectedTab = 0

    var body: some View {
        let _ = print("🔄 [MainTerminalView] BODY - Rendering")
        let _ = print("📊 [MainTerminalView] BODY - Sessions count: \(sessionManager.sessions.count)")
        let _ = print("📊 [MainTerminalView] BODY - Selected tab: \(selectedTab)")
        let _ = print("🎨 [MainTerminalView] BODY - Current theme: \(themeManager.currentTheme.name)")
        
        return ZStack {
            themeManager.currentTheme.backgroundColor
                .ignoresSafeArea()

            VStack(spacing: 0) {
                if sessionManager.sessions.count > 1 {
                    let _ = print("📑 [MainTerminalView] BODY - Showing TabBarView")
                    TabBarView(selectedTab: $selectedTab)
                }

                if let session = sessionManager.sessions[safe: selectedTab] {
                    let _ = print("✅ [MainTerminalView] BODY - Rendering TerminalView for session: \(session.id)")
                    TerminalView(session: session)
                } else {
                    let _ = print("⚠️  [MainTerminalView] BODY - No active session, showing placeholder")
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
            print("👀 [MainTerminalView] onAppear - View appeared")
            print("📊 [MainTerminalView] onAppear - Sessions count: \(sessionManager.sessions.count)")
            if sessionManager.sessions.isEmpty {
                print("🆕 [MainTerminalView] onAppear - No sessions, creating one")
                sessionManager.createSession()
            } else {
                print("✅ [MainTerminalView] onAppear - Sessions already exist")
            }
        }
    }

    private func handlePinchGesture(scale: CGFloat) {
        print("🤏 [MainTerminalView] handlePinchGesture - Scale: \(scale)")
        let newSize = themeManager.fontSize * scale
        print("📏 [MainTerminalView] handlePinchGesture - New font size: \(newSize)")
        themeManager.setFontSize(newSize)
    }
}

extension Collection {
    subscript(safe index: Index) -> Element? {
        indices.contains(index) ? self[index] : nil
    }
}
