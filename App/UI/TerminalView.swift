import SwiftUI

struct TerminalView: View {
    @EnvironmentObject var emulatorManager: EmulatorManager
    @State private var terminalOutput: String = "termi - ARM64 Linux Terminal for iOS\n\n"
    
    var body: some View {
        ZStack {
            Color.black
            
            ScrollView {
                Text(terminalOutput)
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.green)
                    .padding()
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
        }
        .onAppear {
            terminalOutput += "Initializing Alpine Linux...\n"
        }
    }
}
