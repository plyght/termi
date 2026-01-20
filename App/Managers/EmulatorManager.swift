import Combine
import Foundation

class EmulatorManager: ObservableObject {
    @Published var isRunning: Bool = false
    @Published var output: String = ""

    private var emulatorBridge: EmulatorBridge

    init() {
        emulatorBridge = EmulatorBridge.shared
        emulatorBridge.initialize()
    }

    func start() async {
        guard !isRunning else { return }
        isRunning = true
    }

    func stop() {
        isRunning = false
    }

    func sendInput(_ input: String, to session: TerminalSession) {
        emulatorBridge.sendInput(input, to: session)
    }
}
