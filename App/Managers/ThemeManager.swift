import SwiftUI

class ThemeManager: ObservableObject {
    static let shared = ThemeManager()

    @Published var currentTheme: TerminalTheme = .solarizedDark
    @Published var fontSize: CGFloat = 14.0
    @Published var fontName: String = "Menlo-Regular"

    var currentScheme: ColorScheme? {
        switch currentTheme.appearance {
        case .dark: .dark
        case .light: .light
        case .auto: nil
        }
    }

    private init() {
        print("🏗️  [ThemeManager] INIT - Initializing ThemeManager")
        loadPreferences()
        print("✅ [ThemeManager] INIT - ThemeManager initialized")
    }

    func setTheme(_ theme: TerminalTheme) {
        print("🎨 [ThemeManager] setTheme - Changing theme to: \(theme.name)")
        currentTheme = theme
        savePreferences()
        print("✅ [ThemeManager] setTheme - Theme changed and saved")
    }

    func setFontSize(_ size: CGFloat) {
        let clampedSize = max(8, min(32, size))
        print("📏 [ThemeManager] setFontSize - Requested: \(size), Clamped: \(clampedSize)")
        fontSize = clampedSize
        savePreferences()
        print("✅ [ThemeManager] setFontSize - Font size changed and saved")
    }

    func setFont(_ fontName: String) {
        print("🔤 [ThemeManager] setFont - Changing font to: \(fontName)")
        self.fontName = fontName
        savePreferences()
        print("✅ [ThemeManager] setFont - Font changed and saved")
    }

    private func savePreferences() {
        print("💾 [ThemeManager] savePreferences - Saving theme: \(currentTheme.name), fontSize: \(fontSize), font: \(fontName)")
        UserDefaults.standard.set(currentTheme.name, forKey: "terminalTheme")
        UserDefaults.standard.set(fontSize, forKey: "fontSize")
        UserDefaults.standard.set(fontName, forKey: "fontName")
        print("✅ [ThemeManager] savePreferences - Preferences saved")
    }

    private func loadPreferences() {
        print("📂 [ThemeManager] loadPreferences - Loading saved preferences")
        if let themeName = UserDefaults.standard.string(forKey: "terminalTheme"),
           let theme = TerminalTheme.allThemes.first(where: { $0.name == themeName }) {
            print("🎨 [ThemeManager] loadPreferences - Found saved theme: \(themeName)")
            currentTheme = theme
        } else {
            print("🎨 [ThemeManager] loadPreferences - No saved theme, using default: \(currentTheme.name)")
        }
        let savedFontSize = UserDefaults.standard.double(forKey: "fontSize")
        fontSize = savedFontSize > 0 ? CGFloat(savedFontSize) : 14.0
        print("📏 [ThemeManager] loadPreferences - Font size: \(fontSize)")
        fontName = UserDefaults.standard.string(forKey: "fontName") ?? "Menlo-Regular"
        print("🔤 [ThemeManager] loadPreferences - Font name: \(fontName)")
    }
}

struct TerminalTheme: Hashable {
    let name: String
    let appearance: ThemeAppearance
    let backgroundColor: Color
    let foregroundColor: Color
    let cursorColor: Color
    let selectionColor: Color
    let ansiColors: [Color]

    enum ThemeAppearance {
        case light, dark, auto
    }

    func hash(into hasher: inout Hasher) {
        hasher.combine(name)
    }

    static func == (lhs: TerminalTheme, rhs: TerminalTheme) -> Bool {
        lhs.name == rhs.name
    }

    static let solarizedDark = TerminalTheme(
        name: "Solarized Dark",
        appearance: .dark,
        backgroundColor: Color(hex: "002b36"),
        foregroundColor: Color(hex: "839496"),
        cursorColor: Color(hex: "93a1a1"),
        selectionColor: Color(hex: "073642").opacity(0.7),
        ansiColors: [
            Color(hex: "073642"),
            Color(hex: "dc322f"),
            Color(hex: "859900"),
            Color(hex: "b58900"),
            Color(hex: "268bd2"),
            Color(hex: "d33682"),
            Color(hex: "2aa198"),
            Color(hex: "eee8d5"),
            Color(hex: "002b36"),
            Color(hex: "cb4b16"),
            Color(hex: "586e75"),
            Color(hex: "657b83"),
            Color(hex: "839496"),
            Color(hex: "6c71c4"),
            Color(hex: "93a1a1"),
            Color(hex: "fdf6e3"),
        ]
    )

    static let solarizedLight = TerminalTheme(
        name: "Solarized Light",
        appearance: .light,
        backgroundColor: Color(hex: "fdf6e3"),
        foregroundColor: Color(hex: "657b83"),
        cursorColor: Color(hex: "586e75"),
        selectionColor: Color(hex: "eee8d5").opacity(0.7),
        ansiColors: solarizedDark.ansiColors
    )

    static let dracula = TerminalTheme(
        name: "Dracula",
        appearance: .dark,
        backgroundColor: Color(hex: "282a36"),
        foregroundColor: Color(hex: "f8f8f2"),
        cursorColor: Color(hex: "f8f8f0"),
        selectionColor: Color(hex: "44475a").opacity(0.7),
        ansiColors: [
            Color(hex: "21222c"),
            Color(hex: "ff5555"),
            Color(hex: "50fa7b"),
            Color(hex: "f1fa8c"),
            Color(hex: "bd93f9"),
            Color(hex: "ff79c6"),
            Color(hex: "8be9fd"),
            Color(hex: "f8f8f2"),
            Color(hex: "6272a4"),
            Color(hex: "ff6e6e"),
            Color(hex: "69ff94"),
            Color(hex: "ffffa5"),
            Color(hex: "d6acff"),
            Color(hex: "ff92df"),
            Color(hex: "a4ffff"),
            Color(hex: "ffffff"),
        ]
    )

    static let nord = TerminalTheme(
        name: "Nord",
        appearance: .dark,
        backgroundColor: Color(hex: "2e3440"),
        foregroundColor: Color(hex: "d8dee9"),
        cursorColor: Color(hex: "d8dee9"),
        selectionColor: Color(hex: "434c5e").opacity(0.7),
        ansiColors: [
            Color(hex: "3b4252"),
            Color(hex: "bf616a"),
            Color(hex: "a3be8c"),
            Color(hex: "ebcb8b"),
            Color(hex: "81a1c1"),
            Color(hex: "b48ead"),
            Color(hex: "88c0d0"),
            Color(hex: "e5e9f0"),
            Color(hex: "4c566a"),
            Color(hex: "bf616a"),
            Color(hex: "a3be8c"),
            Color(hex: "ebcb8b"),
            Color(hex: "81a1c1"),
            Color(hex: "b48ead"),
            Color(hex: "8fbcbb"),
            Color(hex: "eceff4"),
        ]
    )

    static let catppuccin = TerminalTheme(
        name: "Catppuccin",
        appearance: .dark,
        backgroundColor: Color(hex: "1e1e2e"),
        foregroundColor: Color(hex: "cdd6f4"),
        cursorColor: Color(hex: "f5e0dc"),
        selectionColor: Color(hex: "313244").opacity(0.7),
        ansiColors: [
            Color(hex: "45475a"),
            Color(hex: "f38ba8"),
            Color(hex: "a6e3a1"),
            Color(hex: "f9e2af"),
            Color(hex: "89b4fa"),
            Color(hex: "f5c2e7"),
            Color(hex: "94e2d5"),
            Color(hex: "bac2de"),
            Color(hex: "585b70"),
            Color(hex: "f38ba8"),
            Color(hex: "a6e3a1"),
            Color(hex: "f9e2af"),
            Color(hex: "89b4fa"),
            Color(hex: "f5c2e7"),
            Color(hex: "94e2d5"),
            Color(hex: "a6adc8"),
        ]
    )

    static let allThemes: [TerminalTheme] = [
        .solarizedDark,
        .solarizedLight,
        .dracula,
        .nord,
        .catppuccin,
    ]
}

extension Color {
    init(hex: String) {
        let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        var int: UInt64 = 0
        Scanner(string: hex).scanHexInt64(&int)
        let a, r, g, b: UInt64
        switch hex.count {
        case 3:
            (a, r, g, b) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
        case 6:
            (a, r, g, b) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
        case 8:
            (a, r, g, b) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
        default:
            (a, r, g, b) = (255, 0, 0, 0)
        }

        self.init(
            .sRGB,
            red: Double(r) / 255,
            green: Double(g) / 255,
            blue: Double(b) / 255,
            opacity: Double(a) / 255
        )
    }
}
