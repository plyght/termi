import SwiftUI

struct KeyboardAccessory: View {
    @ObservedObject var session: TerminalSession
    @EnvironmentObject var themeManager: ThemeManager
    @State private var inputText = ""
    @FocusState private var isInputFocused: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            SpecialKeysBar(session: session)
            
            HStack(spacing: 8) {
                TextField("", text: $inputText)
                    .textFieldStyle(.plain)
                    .font(.custom(themeManager.fontName, size: themeManager.fontSize))
                    .foregroundColor(themeManager.currentTheme.foregroundColor)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(themeManager.currentTheme.backgroundColor.opacity(0.9))
                    .cornerRadius(8)
                    .focused($isInputFocused)
                    .onSubmit {
                        sendInput(inputText + "\n")
                        inputText = ""
                    }
                
                Button(action: {
                    sendInput(inputText + "\n")
                    inputText = ""
                }) {
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.system(size: 28))
                        .foregroundColor(themeManager.currentTheme.foregroundColor)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(themeManager.currentTheme.backgroundColor.opacity(0.95))
        }
        .onAppear {
            isInputFocused = true
        }
    }
    
    private func sendInput(_ text: String) {
        session.sendInput(text)
    }
}

struct SpecialKeysBar: View {
    @ObservedObject var session: TerminalSession
    @EnvironmentObject var themeManager: ThemeManager
    
    private let specialKeys: [(label: String, value: String)] = [
        ("Ctrl", "\u{0003}"),
        ("Esc", "\u{001B}"),
        ("Tab", "\t"),
        ("↑", "\u{001B}[A"),
        ("↓", "\u{001B}[B"),
        ("←", "\u{001B}[D"),
        ("→", "\u{001B}[C")
    ]
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(specialKeys, id: \.label) { key in
                    Button(action: {
                        session.sendInput(key.value)
                        triggerHaptic()
                    }) {
                        Text(key.label)
                            .font(.system(size: 14, weight: .medium, design: .monospaced))
                            .foregroundColor(themeManager.currentTheme.foregroundColor)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 8)
                            .background(
                                RoundedRectangle(cornerRadius: 6)
                                    .fill(themeManager.currentTheme.foregroundColor.opacity(0.1))
                            )
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
        }
        .background(themeManager.currentTheme.backgroundColor.opacity(0.98))
    }
    
    private func triggerHaptic() {
        let generator = UIImpactFeedbackGenerator(style: .light)
        generator.impactOccurred()
    }
}
