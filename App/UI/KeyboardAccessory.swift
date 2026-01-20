import SwiftUI

struct KeyboardAccessory: View {
    @ObservedObject var session: TerminalSession
    @EnvironmentObject var themeManager: ThemeManager
    @State private var inputText = ""
    @FocusState private var isInputFocused: Bool

    var body: some View {
        let _ = print("⌨️  [KeyboardAccessory] BODY - Rendering")
        let _ = print("📊 [KeyboardAccessory] BODY - Input focused: \(isInputFocused)")
        let _ = print("📝 [KeyboardAccessory] BODY - Current input: '\(inputText)'")
        
        return VStack(spacing: 0) {
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
                        print("↩️  [KeyboardAccessory] onSubmit - Submitting input: '\(inputText)'")
                        sendInput(inputText + "\n")
                        inputText = ""
                    }
                    .onChange(of: inputText) { newValue in
                        print("📝 [KeyboardAccessory] onChange - Input changed to: '\(newValue)'")
                    }

                Button(action: {
                    print("🔘 [KeyboardAccessory] Button - Send button tapped")
                    sendInput(inputText + "\n")
                    inputText = ""
                }, label: {
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.system(size: 28))
                        .foregroundColor(themeManager.currentTheme.foregroundColor)
                })
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(themeManager.currentTheme.backgroundColor.opacity(0.95))
        }
        .onAppear {
            print("👀 [KeyboardAccessory] onAppear - View appeared")
            print("📊 [KeyboardAccessory] onAppear - Setting focus to true")
            isInputFocused = true
            print("✅ [KeyboardAccessory] onAppear - Focus set")
        }
    }

    private func sendInput(_ text: String) {
        print("📤 [KeyboardAccessory] sendInput - Sending: '\(text)' (\(text.count) chars)")
        session.sendInput(text)
        print("✅ [KeyboardAccessory] sendInput - Sent to session")
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
        ("→", "\u{001B}[C"),
    ]

    var body: some View {
        let _ = print("🎹 [SpecialKeysBar] BODY - Rendering special keys bar")
        
        return ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(specialKeys, id: \.label) { key in
                    Button(action: {
                        let hexValue = key.value.unicodeScalars.map { String(format: "%02X", $0.value) }.joined(separator: " ")
                        print("🔘 [SpecialKeysBar] Button - Key tapped: '\(key.label)' (hex: \(hexValue))")
                        session.sendInput(key.value)
                        triggerHaptic()
                    }, label: {
                        Text(key.label)
                            .font(.system(size: 14, weight: .medium, design: .monospaced))
                            .foregroundColor(themeManager.currentTheme.foregroundColor)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 8)
                            .background(
                                RoundedRectangle(cornerRadius: 6)
                                    .fill(themeManager.currentTheme.foregroundColor.opacity(0.1))
                            )
                    })
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
        }
        .background(themeManager.currentTheme.backgroundColor.opacity(0.98))
    }

    private func triggerHaptic() {
        print("📳 [SpecialKeysBar] triggerHaptic - Haptic feedback triggered")
        let generator = UIImpactFeedbackGenerator(style: .light)
        generator.impactOccurred()
    }
}
