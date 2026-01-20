import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var themeManager: ThemeManager
    @Environment(\.dismiss) var dismiss

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Appearance")) {
                    Picker("Theme", selection: $themeManager.currentTheme) {
                        ForEach(TerminalTheme.allThemes, id: \.name) { theme in
                            Text(theme.name).tag(theme)
                        }
                    }

                    HStack {
                        Text("Font Size")
                        Spacer()
                        Stepper("\(Int(themeManager.fontSize))", value: $themeManager.fontSize, in: 8 ... 32, step: 1)
                    }

                    Picker("Font", selection: $themeManager.fontName) {
                        Text("Menlo").tag("Menlo-Regular")
                        Text("Monaco").tag("Monaco")
                        Text("Courier").tag("Courier")
                        Text("SF Mono").tag("SFMono-Regular")
                    }
                }

                Section(header: Text("Terminal")) {
                    Toggle("Enable Haptic Feedback", isOn: .constant(true))
                    Toggle("Show Cursor", isOn: .constant(true))
                }

                Section(header: Text("About")) {
                    HStack {
                        Text("Version")
                        Spacer()
                        Text("1.0.0")
                            .foregroundColor(.secondary)
                    }

                    Link("GitHub Repository", destination: URL(string: "https://github.com")!)
                }
            }
            .navigationTitle("Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
    }
}
