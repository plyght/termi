import Foundation

class EmulatorBridge {
    init() {}

    func start() {
        print("EmulatorBridge: Starting emulator core...")
    }

    func stop() {
        print("EmulatorBridge: Stopping emulator core...")
    }

    func sendInput(_ input: String) {
        print("EmulatorBridge: Received input: \(input)")
    }
}
