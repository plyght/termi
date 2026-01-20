import SwiftUI

struct TerminalView: View {
    @ObservedObject var session: TerminalSession
    @EnvironmentObject var themeManager: ThemeManager

    var body: some View {
        let _ = print("🖼️  [TerminalView] BODY - Rendering for session: \(session.id.uuidString.prefix(8))")
        let _ = print("📊 [TerminalView] BODY - Output length: \(session.output.count) characters")
        let _ = print("🎨 [TerminalView] BODY - Theme: \(themeManager.currentTheme.name), Font size: \(themeManager.fontSize)")
        let _ = print("📝 [TerminalView] BODY - Output preview: \(session.output.prefix(200))")
        
        let cleanedOutput = stripAnsiCodes(session.output)
        let _ = print("🧹 [TerminalView] BODY - Cleaned output length: \(cleanedOutput.count) characters")
        
        return VStack(spacing: 0) {
            ZStack {
                themeManager.currentTheme.backgroundColor

                ScrollView {
                    Text(cleanedOutput)
                        .font(.system(size: themeManager.fontSize, design: .monospaced))
                        .foregroundColor(themeManager.currentTheme.foregroundColor)
                        .padding()
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
            }
            
            KeyboardAccessory(session: session)
                .environmentObject(themeManager)
        }
        .onAppear {
            print("👀 [TerminalView] onAppear - View appeared for session: \(session.id.uuidString.prefix(8))")
            print("📊 [TerminalView] onAppear - Output isEmpty: \(session.output.isEmpty)")
            if session.output.isEmpty {
                print("📝 [TerminalView] onAppear - Sending initial message")
                let initialMessage = "termi - ARM64 Linux Terminal for iOS\n\nInitializing Alpine Linux...\n"
                if let data = initialMessage.data(using: .utf8) {
                    print("📤 [TerminalView] onAppear - Sending \(data.count) bytes")
                    session.receiveOutput(data)
                } else {
                    print("❌ [TerminalView] onAppear - Failed to encode initial message")
                }
            } else {
                print("✅ [TerminalView] onAppear - Session already has output (\(session.output.count) chars)")
            }
        }
    }
    
    private func stripAnsiCodes(_ text: String) -> String {
        print("🧹 [TerminalView] stripAnsiCodes - Input length: \(text.count)")
        let pattern = "\\x1B\\[[0-9;]*[a-zA-Z]"
        let cleaned = text.replacingOccurrences(of: pattern, with: "", options: .regularExpression)
        print("🧹 [TerminalView] stripAnsiCodes - Output length: \(cleaned.count), Removed \(text.count - cleaned.count) chars")
        return cleaned
    }
}
