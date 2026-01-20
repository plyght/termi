import SwiftUI

struct ContentView: View {
    @EnvironmentObject var emulatorManager: EmulatorManager
    @State private var isEmulatorRunning = false
    
    var body: some View {
        VStack(spacing: 0) {
            TerminalView()
                .environmentObject(emulatorManager)
        }
        .edgesIgnoringSafeArea(.all)
        .onAppear {
            startEmulator()
        }
    }
    
    private func startEmulator() {
        Task {
            await emulatorManager.start()
            isEmulatorRunning = true
        }
    }
}
