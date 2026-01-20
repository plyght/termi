import SwiftUI

struct TerminalView: View {
    let session: TerminalSession
    @EnvironmentObject var themeManager: ThemeManager

    var body: some View {
        ZStack {
            themeManager.currentTheme.backgroundColor

            ScrollView {
                Text(session.output)
                    .font(.system(size: themeManager.fontSize, design: .monospaced))
                    .foregroundColor(themeManager.currentTheme.foregroundColor)
                    .padding()
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
        }
        .onAppear {
            if session.output.isEmpty {
                session.receiveOutput("termi - ARM64 Linux Terminal for iOS\n\nInitializing Alpine Linux...\n".data(using: .utf8)!)
            }
        }
    }
}
